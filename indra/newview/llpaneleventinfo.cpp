/*
 * @file llpaneleventinfo.cpp
 * @brief Event info panel
 *
 * Copyright (c) 2014, Cinder Roxley <cinder@sdf.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "llviewerprecompiledheaders.h"
#include "llpaneleventinfo.h"

#include "llbutton.h"
#include "lleventflags.h"
#include "lliconctrl.h"
#include "llfloaterreg.h"
#include "llresmgr.h"
#include "llslurl.h"
#include "lltrans.h"

#include "llagent.h"
#include "llfloaterworldmap.h"
#include "llviewercontrol.h"

static LLPanelInjector<LLPanelEventInfo> t_event_info("panel_event_info");

LLPanelEventInfo::LLPanelEventInfo()
:	LLPanel()
,	mEvent()
,	mEventID(0)
{
	mEventNotifierConnection = gEventNotifier.setNewEventCallback(boost::bind(&LLPanelEventInfo::processEventReply, this, _1));
}

LLPanelEventInfo::~LLPanelEventInfo()
{
	if (mEventNotifierConnection.connected())
	{
		mEventNotifierConnection.disconnect();
	}
}

BOOL LLPanelEventInfo::postBuild()
{
	childSetAction("teleport_btn", boost::bind(&LLPanelEventInfo::onBtnTeleport, this));
	childSetAction("show_on_map_btn", boost::bind(&LLPanelEventInfo::onBtnMap, this));
	childSetAction("remind_btn", boost::bind(&LLPanelEventInfo::onBtnRemind, this));
	return TRUE;
}

void LLPanelEventInfo::onOpen(const LLSD& key)
{
	U32 id(key.asInteger());
	setEventID(id);
	
	gEventNotifier.add(getEventID());
}

// *TODO: localize this bitch.
std::string LLPanelEventInfo::formatFromMinutes(U32 time)
{
	U32 hours = time / 60;
	U32 minutes = time % 60;
	
	std::ostringstream output;
	if (hours)
		output << hours << " " << getString("hours") << " ";
	if (minutes)
		output << minutes << " " << getString("minutes");
	return output.str();
}

bool LLPanelEventInfo::processEventReply(const LLEventStruct& event)
{
	if (event.eventId != getEventID()) return false; // no
	mEvent = event;
	getChild<LLUICtrl>("name")->setValue(mEvent.eventName);
	getChild<LLUICtrl>("desc")->setValue(mEvent.desc);
	getChild<LLUICtrl>("duration")->setValue(formatFromMinutes(mEvent.duration));
	getChild<LLUICtrl>("host")->setValue(LLSLURL("agent", LLUUID(mEvent.creator), "inspect").getSLURLString());
	getChild<LLUICtrl>("time")->setValue(mEvent.eventDateStr);
	// *TODO: Prettier
	//std::string time;
	//LLStringUtil::formatDatetime(time, "%a %d %b %Y %H:%M", "slt", mEvent.eventEpoch);
	//getChild<LLUICtrl>("time")->setValue(time);
	
	// *TODO: Add translation strings
	//getChild<LLUICtrl>("category")->setValue(LLTrans::getString(mEvent.category));
	getChild<LLUICtrl>("category")->setValue(mEvent.category);
	getChild<LLUICtrl>("cover")->setValue(mEvent.cover
										  ? LLResMgr::getInstance()->getMonetaryString(mEvent.amount)
										  : getString("free"));
	bool mature = (mEvent.flags & EVENT_FLAG_MATURE);
	getChild<LLUICtrl>("content_type")->setValue(getString(mature ? "type_mature" : "type_pg"));
	getChild<LLIconCtrl>("content_type_moderate")->setVisible(mature);
	getChild<LLIconCtrl>("content_type_general")->setVisible(!mature);
	getChild<LLUICtrl>("location")->setValue(LLSLURL(mEvent.simName, mEvent.globalPos).getSLURLString());
	getChild<LLButton>("remind_btn")->setLabel(getString(gEventNotifier.hasNotification(mEvent.eventId)
														 ? "no_reminder"
														 : "reminder"));
	return true;
}

void LLPanelEventInfo::onBtnTeleport()
{
	if (!mEvent.globalPos.isExactlyZero())
	{
		gAgent.teleportViaLocation(mEvent.globalPos);
		LLFloaterWorldMap* worldmap = LLFloaterWorldMap::getInstance();
		if (worldmap)
			worldmap->trackLocation(mEvent.globalPos);
	}
}

void LLPanelEventInfo::onBtnMap()
{
	LLFloaterWorldMap* worldmap = LLFloaterWorldMap::getInstance();
	if (!mEvent.globalPos.isExactlyZero() && worldmap)
	{
		worldmap->trackLocation(mEvent.globalPos);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

void LLPanelEventInfo::onBtnRemind()
{
	if (gEventNotifier.hasNotification(mEvent.eventId))
	{
		gEventNotifier.remove(mEvent.eventId);
		getChild<LLButton>("remind_btn")->setLabel(getString("reminder"));
	}
	else
	{
		gEventNotifier.add(mEvent.eventId);
		getChild<LLButton>("remind_btn")->setLabel(getString("no_reminder"));
	}
}
