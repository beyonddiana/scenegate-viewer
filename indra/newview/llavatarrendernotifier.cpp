/**
 * @file   llavatarrendernotifier.cpp
 * @author andreykproductengine
 * @date   2015-08-05
 * @brief  
 * 
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

// Pre-compiled headers
#include "llviewerprecompiledheaders.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "lltimer.h"
#include "llvoavatarself.h"
#include "llviewercontrol.h"
#include "lltrans.h"
// associated header
#include "llavatarrendernotifier.h"

// when change exceeds this ration, notification is shown
static const F32 RENDER_ALLOWED_CHANGE_PCT = 0.1;
// wait seconds before processing over limit updates after last complexity change
static const U32 OVER_LIMIT_UPDATE_DELAY = 70;


LLAvatarRenderNotifier::LLAvatarRenderNotifier() :
mAgentsCount(0),
mOverLimitAgents(0),
mAgentComplexity(0),
mOverLimitPct(0.0f),
mLatestAgentsCount(0),
mLatestOverLimitAgents(0),
mLatestAgentComplexity(0),
mLatestOverLimitPct(0.0f),
mShowOverLimitAgents(false),
mNotifyOutfitLoading(false)
{
    mPopUpDelayTimer.resetWithExpiry(OVER_LIMIT_UPDATE_DELAY);
}

std::string LLAvatarRenderNotifier::overLimitMessage()
{
    static const char* everyone_now = "av_render_everyone_now";
    static const char* not_everyone = "av_render_not_everyone";
    static const char* over_half = "av_render_over_half";
    static const char* most = "av_render_most_of";
    static const char* anyone = "av_render_anyone";

    std::string message;
    if ( mLatestOverLimitPct >= 99.0 )
    {
        message = anyone;
    }
    else if ( mLatestOverLimitPct >= 75.0 )
    {
        message = most;
    }
    else if ( mLatestOverLimitPct >= 50.0 )
    {
        message = over_half;
    }
    else if ( mLatestOverLimitPct > 10.0 )
    {
        message = not_everyone;
    }
    else
    {
        // Will be shown only after overlimit was > 0
        message = everyone_now;
    }
    return LLTrans::getString(message);
}

void LLAvatarRenderNotifier::displayNotification(bool show_over_limit)
{
    mAgentComplexity = mLatestAgentComplexity;
    mShowOverLimitAgents = show_over_limit;
	static LLCachedControl<U32> expire_delay(gSavedSettings, "ShowMyComplexityChanges", 20);

	LLDate expire_date(LLDate::now().secondsSinceEpoch() + expire_delay);
	LLSD args;
	args["AGENT_COMPLEXITY"] = LLSD::Integer(mLatestAgentComplexity);
	std::string notification_name;
    if (mShowOverLimitAgents)
    {
        mAgentsCount = mLatestAgentsCount;
        mOverLimitAgents = mLatestOverLimitAgents;
        mOverLimitPct = mLatestOverLimitPct;

        std::string notification_message = overLimitMessage();
        notification_name = "RegionAndAgentComplexity";
        args["OVERLIMIT_MSG"] = notification_message;
	}
	else
	{
		notification_name = "AgentComplexity";
	}

	if (mNotificationPtr != NULL && mNotificationPtr->getName() != notification_name)
	{
		// since unique tag works only for same notification,
		// old notification needs to be canceled manually
		LLNotifications::instance().cancel(mNotificationPtr);
	}

    LL_INFOS("AvatarRenderInfo") << notification_name << " " << args << LL_ENDL;

	mNotificationPtr = LLNotifications::instance().add(LLNotification::Params()
		.name(notification_name)
		.expiry(expire_date)
		.substitutions(args));
}

bool LLAvatarRenderNotifier::isNotificationVisible()
{
	return mNotificationPtr != NULL && mNotificationPtr->isActive();
}

void LLAvatarRenderNotifier::updateNotificationRegion(U32 agentcount, U32 overLimit)
{
	if (agentcount == 0)
	{
		// Data not ready
		return;
	}

	// save current values for later use
	mLatestAgentsCount = agentcount > overLimit ? agentcount - 1 : agentcount; // subtract self
	mLatestOverLimitAgents = overLimit;
	mLatestOverLimitPct = mLatestAgentsCount != 0 ? ((F32)overLimit / (F32)mLatestAgentsCount) * 100.0 : 0;

    if (mAgentsCount == mLatestAgentsCount
        && mOverLimitAgents == mLatestOverLimitAgents)
    {
        // no changes since last notification
        return;
    }

    if ((mPopUpDelayTimer.hasExpired() || (isNotificationVisible() && mShowOverLimitAgents))
        && (mOverLimitPct > 0 || mLatestOverLimitPct > 0)
        && std::abs(mOverLimitPct - mLatestOverLimitPct) > mLatestOverLimitPct * RENDER_ALLOWED_CHANGE_PCT
        )
    {
        // display in case of drop to/from zero and in case of significant (RENDER_ALLOWED_CHANGE_PCT) changes
        displayNotification(true);

        // default timeout before next notification
        static LLCachedControl<U32> pop_up_delay(gSavedSettings, "ComplexityChangesPopUpDelay", 300);
        mPopUpDelayTimer.resetWithExpiry(pop_up_delay);
    }
}

void LLAvatarRenderNotifier::updateNotificationAgent(U32 agentComplexity)
{
    // save the value for use in following messages
    mLatestAgentComplexity = agentComplexity;

    if (!gAgentWearables.areWearablesLoaded())
    {
        // data not ready, nothing to show.
        return;
    }

    if (!mNotifyOutfitLoading)
    {
        // We should not notify about initial outfit and it's load process without reason

        if (!isAgentAvatarValid())
        {
            return;
        }

        static S32 initial_cof_version(-1);
        static S32 rez_status(0);

        if (initial_cof_version < 0
            && gAgentWearables.areWearablesLoaded()
            && !LLAttachmentsMgr::getInstance()->hasPendingAttachments()
            && !LLAttachmentsMgr::getInstance()->hasAttachmentRequests()
            && !LLAttachmentsMgr::getInstance()->hasRecentlyArrivedAttachments())
        {
            // cof formed
            initial_cof_version = LLAppearanceMgr::instance().getCOFVersion();

            // outfit might have been pre-loaded in one go, we are adding/removing items in such case
            mNotifyOutfitLoading = gAgentAvatarp->isAllLocalTextureDataFinal();
        }

        if (initial_cof_version >= 0 && initial_cof_version != gAgentAvatarp->mLastUpdateRequestCOFVersion)
        {
            // version mismatch in comparison to initial outfit - outfit changed
            mNotifyOutfitLoading = true;
        }
        else if (mLatestOverLimitAgents > 0)
        {
            // Some users can't see agent already, notify user about complexity growth
            mNotifyOutfitLoading = true;
        }
        else if (gAgentAvatarp->mLastRezzedStatus >= rez_status)
        {
            rez_status = gAgentAvatarp->mLastRezzedStatus;
        }
        else
        {
            // rez status decreased - outfit related action was initiated
            mNotifyOutfitLoading = true;
        }

        if (!mNotifyOutfitLoading)
        {
            // avatar or outfit not ready
            mAgentComplexity = mLatestAgentComplexity;
            return;
        }
    }

    if (mAgentComplexity != mLatestAgentComplexity)
    {
        // if we have an agent complexity change, we always display it and hide 'over limit'
        displayNotification(false);

        // next 'over limit' update should be displayed after delay to make sure information got updated at server side
        mPopUpDelayTimer.resetWithExpiry(OVER_LIMIT_UPDATE_DELAY);
    }
}

