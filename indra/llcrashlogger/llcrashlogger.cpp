 /** 
* @file llcrashlogger.cpp
* @brief Crash logger implementation
*
* $LicenseInfo:firstyear=2003&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
* 
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* 
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* 
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <map>

#include "llcrashlogger.h"
#include "llcrashlock.h"
#include "linden_common.h"
#include "llstring.h"
#include "indra_constants.h"	// CRASH_BEHAVIOR_...
#include "llerror.h"
#include "llerrorcontrol.h"
#include "lltimer.h"
#include "lldir.h"
#include "llfile.h"
#include "lliopipe.h"
#include "llpumpio.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llproxy.h"
#include "llsdutil.h"  //remove

#include <boost/regex.hpp>
 
LLPumpIO* gServicePump = NULL;
BOOL gBreak = false;
BOOL gSent = false;

class LLCrashLoggerResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLCrashLoggerResponder);
public:
	LLCrashLoggerResponder() 
	{
	}

protected:
	virtual void httpFailure()
	{
		LL_WARNS() << dumpResponse() << LL_ENDL;
		gBreak = true;
	}

	virtual void httpSuccess()
	{
		gBreak = true;
		gSent = true;
	}
};

LLCrashLogger::LLCrashLogger() :
	mCrashBehavior(CRASH_BEHAVIOR_ALWAYS_SEND),
	mCrashInPreviousExec(false),
	mCrashSettings("CrashSettings"),
	mSentCrashLogs(false),
	mCrashHost("")
{
}

LLCrashLogger::~LLCrashLogger()
{

}

// TRIM_SIZE must remain larger than LINE_SEARCH_SIZE.
const int TRIM_SIZE = 128000;
const int LINE_SEARCH_DIST = 500;
const std::string SKIP_TEXT = "\n ...Skipping... \n";
void trimSLLog(std::string& sllog)
{
	if(sllog.length() > TRIM_SIZE * 2)
	{
		std::string::iterator head = sllog.begin() + TRIM_SIZE;
		std::string::iterator tail = sllog.begin() + sllog.length() - TRIM_SIZE;
		std::string::iterator new_head = std::find(head, head - LINE_SEARCH_DIST, '\n');
		if(new_head != head - LINE_SEARCH_DIST)
		{
			head = new_head;
		}

		std::string::iterator new_tail = std::find(tail, tail + LINE_SEARCH_DIST, '\n');
		if(new_tail != tail + LINE_SEARCH_DIST)
		{
			tail = new_tail;
		}

		sllog.erase(head, tail);
		sllog.insert(head, SKIP_TEXT.begin(), SKIP_TEXT.end());
	}
}

std::string getStartupStateFromLog(std::string& sllog)
{
	std::string startup_state = "STATE_FIRST";
	std::string startup_token = "Startup state changing from ";

	int index = sllog.rfind(startup_token);
	if (index < 0 || index + startup_token.length() > sllog.length()) {
		return startup_state;
	}

	// find new line
	char cur_char = sllog[index + startup_token.length()];
	std::string::size_type newline_loc = index + startup_token.length();
	while(cur_char != '\n' && newline_loc < sllog.length())
	{
		newline_loc++;
		cur_char = sllog[newline_loc];
	}
	
	// get substring and find location of " to "
	std::string state_line = sllog.substr(index, newline_loc - index);
	std::string::size_type state_index = state_line.find(" to ");
	startup_state = state_line.substr(state_index + 4, state_line.length() - state_index - 4);

	return startup_state;
}

bool LLCrashLogger::readFromXML(LLSD& dest, const std::string& filename )
{
    std::string db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,filename);
    std::ifstream log_file(db_file_name.c_str());
    
	// Look for it in the given file
	if (log_file.is_open())
	{
		LLSDSerialize::fromXML(dest, log_file);
        log_file.close();
        return true;
    }
    return false;
}

void LLCrashLogger::mergeLogs( LLSD src_sd )
{
    LLSD::map_iterator iter = src_sd.beginMap();
	LLSD::map_iterator end = src_sd.endMap();
	for( ; iter != end; ++iter)
    {
        mDebugLog[iter->first] = iter->second;
    }
}

bool LLCrashLogger::readMinidump(std::string minidump_path)
{
	size_t length=0;

	std::ifstream minidump_stream(minidump_path.c_str(), std::ios_base::in | std::ios_base::binary);
	if(minidump_stream.is_open())
	{
		minidump_stream.seekg(0, std::ios::end);
		length = (size_t)minidump_stream.tellg();
		minidump_stream.seekg(0, std::ios::beg);
		
		LLSD::Binary data;
		data.resize(length);
		
		minidump_stream.read(reinterpret_cast<char *>(&(data[0])),length);
		minidump_stream.close();
		
		mCrashInfo["Minidump"] = data;
	}
	return (length>0?true:false);
}

void LLCrashLogger::gatherFiles()
{
	updateApplication("Gathering logs...");

    LLSD static_sd;
    LLSD dynamic_sd;
    //if we ever want to change the endpoint we send crashes to
    //we can construct a file download ( a la feature table filename for example)
    //containing the new endpoint
    LLSD endpoint;
    std::string grid;
    std::string fqdn;
    
    bool has_logs = readFromXML( static_sd, "static_debug_info.log" );
    has_logs |= readFromXML( dynamic_sd, "dynamic_debug_info.log" );

    
    if ( has_logs )
    {
        mDebugLog = static_sd;
        mergeLogs(dynamic_sd);
		mCrashInPreviousExec = mDebugLog["CrashNotHandled"].asBoolean();

		mFileMap["SecondLifeLog"] = mDebugLog["SLLog"].asString();
		mFileMap["SettingsXml"] = mDebugLog["SettingsFilename"].asString();
        mFileMap["CrashHostUrl"] = loadCrashURLSetting();
		if(mDebugLog.has("CAFilename"))
		{
			LLCurl::setCAFile(mDebugLog["CAFilename"].asString());
		}
		else
		{
			LLCurl::setCAFile(gDirUtilp->getCAFile());
		}

		LL_INFOS() << "Using log file from debug log " << mFileMap["SecondLifeLog"] << LL_ENDL;
		LL_INFOS() << "Using settings file from debug log " << mFileMap["SettingsXml"] << LL_ENDL;
	}
	else
	{
		// Figure out the filename of the second life log
		LLCurl::setCAFile(gDirUtilp->getCAFile());
        
		mFileMap["SecondLifeLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"SecondLife.log");
        mFileMap["SettingsXml"] = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.xml");
	}

    if (!gDirUtilp->fileExists(mFileMap["SecondLifeLog"]) ) //We would prefer to get this from the per-run but here's our fallback.
    {
        mFileMap["SecondLifeLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.old");
    }

	gatherPlatformSpecificFiles();


    if ( has_logs && (mFileMap["CrashHostUrl"] != "") )
    {
        mCrashHost = mFileMap["CrashHostUrl"];
    }

	//default to agni, per product
	mAltCrashHost = "http://viewercrashreport.agni.lindenlab.com/cgi-bin/viewercrashreceiver.py";

	mCrashInfo["DebugLog"] = mDebugLog;
	mFileMap["StatsLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"stats.log");
	
	updateApplication("Encoding files...");

	for(std::map<std::string, std::string>::iterator itr = mFileMap.begin(); itr != mFileMap.end(); ++itr)
	{
		std::ifstream f((*itr).second.c_str());
		if(!f.is_open())
		{
			LL_INFOS("CRASHREPORT") << "Can't find file " << (*itr).second << LL_ENDL;
			continue;
		}
		std::stringstream s;
		s << f.rdbuf();

		std::string crash_info = s.str();
		if(itr->first == "SecondLifeLog")
		{
			if(!mCrashInfo["DebugLog"].has("StartupState"))
			{
				mCrashInfo["DebugLog"]["StartupState"] = getStartupStateFromLog(crash_info);
			}
			trimSLLog(crash_info);
		}

		mCrashInfo[(*itr).first] = LLStringFn::strip_invalid_xml(rawstr_to_utf8(crash_info));
	}
	
	std::string minidump_path;
	// Add minidump as binary.
    bool has_minidump = mDebugLog.has("MinidumpPath");
    
	if (has_minidump)
	{
		minidump_path = mDebugLog["MinidumpPath"].asString();
	}

	if (has_minidump)
	{
		has_minidump = readMinidump(minidump_path);
	}

    if (!has_minidump)  //Viewer was probably so hosed it couldn't write remaining data.  Try brute force.
    {
       //Look for a filename at least 30 characters long in the dump dir which contains the characters MDMP as the first 4 characters in the file.
        typedef std::vector<std::string> vec;
        std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"");
        vec file_vec = gDirUtilp->getFilesInDir(pathname);
        for(vec::const_iterator iter=file_vec.begin(); iter!=file_vec.end(); ++iter)
        {
            if ( ( iter->length() > 30 ) && (iter->rfind(".dmp") == (iter->length()-4) ) )
            {
                std::string fullname = pathname + *iter;
                std::ifstream fdat( fullname.c_str(), std::ifstream::binary);
                if (fdat)
                {
                    char buf[5];
                    fdat.read(buf,4);
                    fdat.close();  
                    if (!strncmp(buf,"MDMP",4))
                    {
                        minidump_path = *iter;
                        has_minidump = readMinidump(fullname);
						mDebugLog["MinidumpPath"] = fullname;
						if (has_minidump) 
						{
							break;
						}
                    }
                }
            }
        }
    }
}

LLSD LLCrashLogger::constructPostData()
{
	return mCrashInfo;
}

const char* const CRASH_SETTINGS_FILE = "settings_crash_behavior.xml";

std::string LLCrashLogger::loadCrashURLSetting()
{

	// First check user_settings (in the user's home dir)
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	if (! mCrashSettings.loadFromFile(filename))
	{
		// Next check app_settings (in the SL program dir)
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, CRASH_SETTINGS_FILE);
		mCrashSettings.loadFromFile(filename);
	}

    if (! mCrashSettings.controlExists("CrashHostUrl"))
    {
        return "";
    }
    else
    {
        return mCrashSettings.getString("CrashHostUrl");
    }
}

bool LLCrashLogger::runCrashLogPost(std::string host, LLSD data, std::string msg, int retries, int timeout)
{
	for(int i = 0; i < retries; ++i)
	{
		updateApplication(llformat("%s, try %d...", msg.c_str(), i+1));
		LLHTTPClient::post(host, data, new LLCrashLoggerResponder(), timeout);
        while(!gBreak)
        {
            ms_sleep(250);
            updateApplication(); // No new message, just pump the IO
        }
		if(gSent)
		{
			return gSent;
		}
	}
	return gSent;
}

bool LLCrashLogger::sendCrashLog(std::string dump_dir)
{

    gDirUtilp->setDumpDir( dump_dir );
    
    std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
                                                           "SecondLifeCrashReport");
    std::string report_file = dump_path + ".log";

	gatherFiles();
    
	LLSD post_data;
	post_data = constructPostData();
    
	updateApplication("Sending reports...");

	std::ofstream out_file(report_file.c_str());
	LLSDSerialize::toPrettyXML(post_data, out_file);
    out_file.flush();
	out_file.close();
    
	bool sent = false;
    
	//*TODO: Translate
    updateApplication("DEBUG: crash host in send logs "+mCrashHost);
	if(mCrashHost != "")
	{   
        std::string msg = "Using derived crash server... ";
        msg = msg+mCrashHost.c_str();
        updateApplication(msg.c_str());
        
		sent = runCrashLogPost(mCrashHost, post_data, std::string("Sending to server"), 3, 5);
	}
    
	if(!sent)
	{
        updateApplication("Using default server...");
		sent = runCrashLogPost(mAltCrashHost, post_data, std::string("Sending to default server"), 3, 5);
	}
    
	mSentCrashLogs = sent;
    
	return sent;
}

bool LLCrashLogger::sendCrashLogs()
{
    
	LLSD opts = getOptionData(PRIORITY_COMMAND_LINE);
    LLSD rec;

	if ( opts.has("dumpdir") )
    {
        rec["pid"]=opts["pid"];
        rec["dumpdir"]=opts["dumpdir"];
        rec["procname"]=opts["procname"];
    }
    else
    {
        return false;
    }       

    return sendCrashLog(rec["dumpdir"].asString());
}

void LLCrashLogger::updateApplication(const std::string& message)
{
	gServicePump->pump();
    gServicePump->callback();
	if (!message.empty()) LL_INFOS() << message << LL_ENDL;
}

bool LLCrashLogger::init()
{
	LLCurl::initClass(false);

	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	LLError::initForApplication(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));

	// Default to the product name "Second Life" (this is overridden by the -name argument)
	mProductName = "Second Life";

	// Rename current log file to ".old"
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "crashreport.log.old");
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "crashreport.log");

#if LL_WINDOWS
	LLAPRFile::remove(old_log_file);
#endif 

	LLFile::rename(log_file.c_str(), old_log_file.c_str());
    
	// Set the log file to crashreport.log
	LLError::logToFile(log_file);  //NOTE:  Until this line, LL_INFOS LL_WARNS, etc are blown to the ether. 

    LL_INFOS() << "Crash reporter file rotation complete." << LL_ENDL;

    mCrashSettings.declareS32("CrashSubmitBehavior", CRASH_BEHAVIOR_ALWAYS_SEND,
							  "Controls behavior when viewer crashes "
							  "(0 = ask before sending crash report, "
							  "1 = always send crash report, "
							  "2 = never send crash report)");
    
	// LL_INFOS() << "Loading crash behavior setting" << LL_ENDL;
	// mCrashBehavior = loadCrashBehaviorSetting();
       
	gServicePump = new LLPumpIO(gAPRPoolp);
	gServicePump->prime(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);
 	
	return true;
}

// For cleanup code common to all platforms.
void LLCrashLogger::commonCleanup()
{
	LLError::logToFile("");   //close crashreport.log
	LLProxy::cleanupClass();
}
