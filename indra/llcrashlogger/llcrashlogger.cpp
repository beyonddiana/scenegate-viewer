 /** 
* @file llcrashlogger.cpp
* @brief Crash logger implementation
*
* $LicenseInfo:firstyear=2003&license=viewergpl$
* 
* Copyright (c) 2003-2009, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <map>

#include "llcrashlogger.h"
#include "linden_common.h"
#include "lldate.h"
#include "llfile.h"
#include "llstring.h"
#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "lliopipe.h"
#include "llpumpio.h"
#include "llhttpclient.h"
#include "llsdserialize.h"

LLPumpIO* gServicePump;
BOOL gBreak = false;
BOOL gSent = false;

class LLCrashLoggerResponder : public LLHTTPClient::Responder
{
public:
	LLCrashLoggerResponder() 
	{
	}

	virtual void error(U32 status, const std::string& reason)
	{
		gBreak = true;		
	}

	virtual void result(const LLSD& content)
	{	
		gBreak = true;
		gSent = true;
	}
};

bool LLCrashLoggerText::mainLoop()
{
	std::cout << "Entering main loop" << std::endl;
	sendCrashLogs();
	return true;	
}

void LLCrashLoggerText::updateApplication(const std::string& message)
{
	LLCrashLogger::updateApplication(message);
	std::cout << message << std::endl;
}

LLCrashLogger::LLCrashLogger() :
	mCrashBehavior(CRASH_BEHAVIOR_ASK),
	mCrashInPreviousExec(false),
	mCrashSettings("CrashSettings"),
	mSentCrashLogs(false),
	mCrashHost("")
{
	// Set up generic error handling
	setupErrorHandling();
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

void LLCrashLogger::findAndRenameLogFiles()
{
	// Find and rename the relevant log files so they won't be stomped on if 
	// SL is restarted before user sends crash report.
	
	std::string now = "." + LLDate::now().asString();
	
	std::string stats_log_original = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stats.log");
	std::string stats_log = stats_log_original;
	stats_log.insert(stats_log.length() - 4, now);
	if(LLFile::rename(stats_log_original, stats_log) == 0)
	{
		mFileMap["StatsLog"] = stats_log;
	}
	
	std::string second_life_log_original;
	std::string settings_file_original;

	std::string db_file_name_original = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
	std::string db_file_name = db_file_name_original;
	db_file_name.insert(db_file_name.length() - 4, now);
	LLFile::rename(db_file_name_original, db_file_name);
	std::ifstream debug_log_file(db_file_name.c_str());
	if (debug_log_file.is_open())
	{		
		LLSDSerialize::fromXML(mDebugLog, debug_log_file);		
		mCrashInPreviousExec = mDebugLog["CrashNotHandled"].asBoolean();
		second_life_log_original = mDebugLog["SLLog"].asString();
		settings_file_original = mDebugLog["SettingsFilename"].asString();
	}
	else
	{
		// Figure out the filename of the second life log
		second_life_log_original = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.log");
		settings_file_original = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.xml");
	}
	
	if(mCrashInPreviousExec)
	{
		// Restarting after freeze.
		// Replace the log file ext with .old, since the 
		// instance that launched this process has overwritten
		// SecondLife.log
		second_life_log_original.replace(second_life_log_original.size() - 4, 4, ".old");
	}
	
	std::string second_life_log = second_life_log_original;
	std::string settings_file = settings_file_original;
	second_life_log.insert(second_life_log.length() - 4, now);
	settings_file.insert(settings_file.length() - 4, now);
	if(LLFile::rename(second_life_log_original, second_life_log) == 0)
	{
		mFileMap["SecondLifeLog"] = second_life_log;
	}
	if(LLFile::rename(settings_file_original, settings_file) == 0)
	{
		mFileMap["SettingsXml"] = settings_file;
	}
}

void LLCrashLogger::gatherFiles()
{
	updateApplication("Gathering logs...");

	if(mDebugLog.has("CAFilename"))
	{
		LLCurl::setCAFile(mDebugLog["CAFilename"].asString());
	}
	else
	{
		LLCurl::setCAFile(gDirUtilp->getCAFile());
	}
	
	gatherPlatformSpecificFiles();

	//Use the debug log to reconstruct the URL to send the crash report to
	if(mDebugLog.has("CrashHostUrl"))
	{
		// Crash log receiver has been manually configured.
		mCrashHost = mDebugLog["CrashHostUrl"].asString();
	}
	else if(mDebugLog.has("CurrentSimHost"))
	{
		mCrashHost = "https://";
		mCrashHost += mDebugLog["CurrentSimHost"].asString();
		mCrashHost += ":12043/crash/report";
	}
	else if(mDebugLog.has("GridName"))
	{
		// This is a 'little' hacky, but its the best simple solution.
		std::string grid_host = mDebugLog["GridName"].asString();
		LLStringUtil::toLower(grid_host);

		mCrashHost = "https://login.";
		mCrashHost += grid_host;
		mCrashHost += ".lindenlab.com:12043/crash/report";
	}

	// Use login servers as the alternate, since they are already load balanced and have a known name
	mAltCrashHost = "https://login.agni.lindenlab.com:12043/crash/report";

	mCrashInfo["DebugLog"] = mDebugLog;
	
	updateApplication("Encoding files...");

	for(std::map<std::string, std::string>::iterator itr = mFileMap.begin(); itr != mFileMap.end(); ++itr)
	{
		std::ifstream f((*itr).second.c_str());
		if(!f.is_open())
		{
			std::cout << "Can't find file " << (*itr).second << std::endl;
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
	
	// Add minidump as binary.
	std::string minidump_path = mDebugLog["MinidumpPath"];
	if(minidump_path != "")
	{
		std::ifstream minidump_stream(minidump_path.c_str(), std::ios_base::in | std::ios_base::binary);
		if(minidump_stream.is_open())
		{
			minidump_stream.seekg(0, std::ios::end);
			size_t length = minidump_stream.tellg();
			minidump_stream.seekg(0, std::ios::beg);
			
			LLSD::Binary data;
			data.resize(length);
			
			minidump_stream.read(reinterpret_cast<char *>(&(data[0])),length);
			minidump_stream.close();
			
			mCrashInfo["Minidump"] = data;
		}
	}
	mCrashInfo["DebugLog"].erase("MinidumpPath");
}

LLSD LLCrashLogger::constructPostData()
{
	LLSD ret;
	return mCrashInfo;
}

S32 LLCrashLogger::loadCrashBehaviorSetting()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	mCrashSettings.loadFromFile(filename);
		
	S32 value = mCrashSettings.getS32(CRASH_BEHAVIOR_SETTING);
	
	if (value < CRASH_BEHAVIOR_ASK || CRASH_BEHAVIOR_NEVER_SEND < value) return CRASH_BEHAVIOR_ASK;

	return value;
}

bool LLCrashLogger::saveCrashBehaviorSetting(S32 crash_behavior)
{
	if (crash_behavior != CRASH_BEHAVIOR_ASK && crash_behavior != CRASH_BEHAVIOR_ALWAYS_SEND) return false;

	mCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, crash_behavior);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);

	mCrashSettings.saveToFile(filename, FALSE);

	return true;
}

bool LLCrashLogger::runCrashLogPost(std::string host, LLSD data, std::string msg, int retries, int timeout)
{
	gBreak = false;
	std::string status_message;
	for(int i = 0; i < retries; ++i)
	{
		status_message = llformat("%s, try %d...", msg.c_str(), i+1);
		LLHTTPClient::post(host, data, new LLCrashLoggerResponder(), timeout);
		while(!gBreak)
		{
			updateApplication(status_message);
		}
		if(gSent)
		{
			return gSent;
		}
	}
	return gSent;
}

bool LLCrashLogger::sendCrashLogs()
{
	gatherFiles();

	LLSD post_data;
	post_data = constructPostData();

	updateApplication("Sending reports...");

	std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
															   "SecondLifeCrashReport");
	std::string report_file = dump_path + ".log";

	std::ofstream out_file(report_file.c_str());
	LLSDSerialize::toPrettyXML(post_data, out_file);
	out_file.close();

	bool sent = false;

	//*TODO: Translate
	if(mCrashHost != "")
	{
		sent = runCrashLogPost(mCrashHost, post_data, std::string("Sending to server"), 3, 5);
	}

	if(!sent)
	{
		sent = runCrashLogPost(mAltCrashHost, post_data, std::string("Sending to alternate server"), 3, 5);
	}
	
	mSentCrashLogs = sent;

	return true;
}

void LLCrashLogger::updateApplication(const std::string& message)
{
	gServicePump->pump();
    gServicePump->callback();
}

bool LLCrashLogger::init()
{
	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	// Default to the product name "Second Life" (this is overridden by the -name argument)
	mProductName = "Second Life";
	
	mCrashSettings.declareS32(CRASH_BEHAVIOR_SETTING, CRASH_BEHAVIOR_ASK, "Controls behavior when viewer crashes "
		"(0 = ask before sending crash report, 1 = always send crash report, 2 = never send crash report)");

	llinfos << "Loading crash behavior setting" << llendl;
	mCrashBehavior = loadCrashBehaviorSetting();

	// If user doesn't want to send, bail out
	if (mCrashBehavior == CRASH_BEHAVIOR_NEVER_SEND)
	{
		llinfos << "Crash behavior is never_send, quitting" << llendl;
		return false;
	}

	gServicePump = new LLPumpIO(gAPRPoolp);
	gServicePump->prime(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);

	//If we've opened the crash logger, assume we can delete the marker file if it exists	
	if( gDirUtilp )
	{
		std::string marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.exec_marker");
		LLAPRFile::remove( marker_file );
	}
	
	findAndRenameLogFiles();
	
	return true;
}
