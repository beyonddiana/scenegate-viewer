/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Jonathan Yap
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterperms.h"

#include "llagent.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llpermissions.h"
#include "llsdserialize.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

// String equivalents of enum Categories - initialization order must match enum order!
const std::array<std::string, 6> LLFloaterPermsDefault::sCategoryNames {{
	"Objects",
	"Uploads",
	"Scripts",
	"Notecards",
	"Gestures",
	"Wearables"
}};

// Oh hi! I'm a caps responder. I'm here because thing
class LLFloaterPermsResponder : public LLHTTPClient::Responder
{
public:
	LLFloaterPermsResponder() : LLHTTPClient::Responder() {}
	
private:
	void httpFailure()
	{
		const std::string& reason = getReason();
		//LLNotificationsUtil::add("DefaultObjectPermissions", LLSD().with("REASON", reason));
		// This happens often enough to be irritating... Let's just drop it into the log.
		LL_WARNS("ObjectPermissions") << "There was a problem saving the default object permissions: " << reason << LL_ENDL;
	}
	
	void httpSuccess()
	{
		// Since we have had a successful POST call be sure to display the next error message
		// even if it is the same as a previous one.
		LL_INFOS("ObjectPermissionsFloater") << "Default permissions successfully sent to simulator" << LL_ENDL;
	}
};


LLFloaterPerms::LLFloaterPerms(const LLSD& key)
:	LLFloater(key)
{
}

BOOL LLFloaterPerms::postBuild()
{
	return TRUE;
}

//static 
U32 LLFloaterPerms::getGroupPerms(const std::string& prefix)
{	
	return gSavedSettings.getBOOL(prefix+"ShareWithGroup") ? PERM_COPY | PERM_MOVE | PERM_MODIFY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getEveryonePerms(const std::string& prefix)
{
	return gSavedSettings.getBOOL(prefix+"EveryoneCopy") ? PERM_COPY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getNextOwnerPerms(const std::string& prefix)
{
	U32 flags = PERM_MOVE;
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}


LLFloaterPermsDefault::LLFloaterPermsDefault(const LLSD& key)
:	LLFloater(key)
{
	mCommitCallbackRegistrar.add("PermsDefault.Copy", boost::bind(&LLFloaterPermsDefault::onCommitCopy, this, _2));
	mCommitCallbackRegistrar.add("PermsDefault.OK", boost::bind(&LLFloaterPermsDefault::onClickOK, this));
	mCommitCallbackRegistrar.add("PermsDefault.Cancel", boost::bind(&LLFloaterPermsDefault::onClickCancel, this));
}

BOOL LLFloaterPermsDefault::postBuild()
{
	if(!gSavedSettings.getBOOL("DefaultUploadPermissionsConverted"))
	{
		gSavedSettings.setBOOL("UploadsEveryoneCopy", gSavedSettings.getBOOL("EveryoneCopy"));
		gSavedSettings.setBOOL("UploadsNextOwnerCopy", gSavedSettings.getBOOL("NextOwnerCopy"));
		gSavedSettings.setBOOL("UploadsNextOwnerModify", gSavedSettings.getBOOL("NextOwnerModify"));
		gSavedSettings.setBOOL("UploadsNextOwnerTransfer", gSavedSettings.getBOOL("NextOwnerTransfer"));
		gSavedSettings.setBOOL("UploadsShareWithGroup", gSavedSettings.getBOOL("ShareWithGroup"));
		gSavedSettings.setBOOL("DefaultUploadPermissionsConverted", true);
	}

	mCloseSignal.connect(boost::bind(&LLFloaterPermsDefault::cancel, this));

	reload();
	
	return true;
}

void LLFloaterPermsDefault::onClickOK()
{
	ok();
	closeFloater();
}

void LLFloaterPermsDefault::onClickCancel()
{
	cancel();
	closeFloater();
}

void LLFloaterPermsDefault::onCommitCopy(const LLSD& user_data)
{
	// Implements fair use
	const std::string& prefix = user_data.asString();

	BOOL copyable = gSavedSettings.getBOOL(prefix+"NextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL(prefix+"NextOwnerTransfer", TRUE);
	}
	LLUICtrl* xfer = getChild<LLUICtrl>(prefix+"_transfer");
	xfer->setEnabled(copyable);
}

//static
void LLFloaterPermsDefault::setCapsReceivedCallback(LLViewerRegion* regionp)
{
	LLFloaterPermsDefault* floater = dynamic_cast<LLFloaterPermsDefault*>(LLFloaterReg::getInstance("perms_default"));
	regionp->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterPermsDefault::updateCap, floater));
}

void LLFloaterPermsDefault::updateCap()
{
	const std::string& object_url = gAgent.getRegion()->getCapability("AgentPreferences");

	if(!object_url.empty())
	{
		LLSD report = LLSD::emptyMap();
		report["default_object_perm_masks"]["Group"] =
			(LLSD::Integer)LLFloaterPerms::getGroupPerms(sCategoryNames[0]);
		report["default_object_perm_masks"]["Everyone"] =
			(LLSD::Integer)LLFloaterPerms::getEveryonePerms(sCategoryNames[0]);
		report["default_object_perm_masks"]["NextOwner"] =
			(LLSD::Integer)LLFloaterPerms::getNextOwnerPerms(sCategoryNames[0]);

        {
            LL_DEBUGS("ObjectPermissionsFloater") << "Sending default permissions to '"
                                                  << object_url << "'\n";
            std::ostringstream sent_perms_log;
            LLSDSerialize::toPrettyXML(report, sent_perms_log);
            LL_CONT << sent_perms_log.str() << LL_ENDL;
        }
    
		LLHTTPClient::post(object_url, report, new LLFloaterPermsResponder());
	}
    else
    {
        LL_DEBUGS("ObjectPermissionsFloater") << "AgentPreferences cap not available." << LL_ENDL;
    }
}

void LLFloaterPermsDefault::ok()
{
//	Changes were already applied automatically to saved settings.
//	Refreshing internal values makes it official.
	reload();

// We know some setting has changed but not which one.  Just in case it was a setting for
// object permissions tell the server what the values are.
	updateCap();
}

void LLFloaterPermsDefault::cancel()
{
	for (U32 iter = 0; iter < sCategoryNames.size() ; iter++)
	{
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerCopy",		mNextOwnerCopy[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerModify",		mNextOwnerModify[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerTransfer",	mNextOwnerTransfer[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"ShareWithGroup",		mShareWithGroup[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"EveryoneCopy",			mEveryoneCopy[iter]);
	}
}

void LLFloaterPermsDefault::reload()
{
	for (U32 iter = 0; iter < sCategoryNames.size(); iter++)
	{
		mShareWithGroup[iter]    = gSavedSettings.getBOOL(sCategoryNames[iter]+"ShareWithGroup");
		mEveryoneCopy[iter]      = gSavedSettings.getBOOL(sCategoryNames[iter]+"EveryoneCopy");
		mNextOwnerCopy[iter]     = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerCopy");
		mNextOwnerModify[iter]   = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerModify");
		mNextOwnerTransfer[iter] = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerTransfer");
	}
}
