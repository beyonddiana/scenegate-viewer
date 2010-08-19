/** 
 * @file llfloaterevent.h
 * @brief Display for events in the finder
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATEREVENT_H
#define LL_LLFLOATEREVENT_H

#include "llfloater.h"
#include "lleventinfo.h"
#include "lluuid.h"
#include "v3dmath.h"

class LLTextBox;
class LLTextEditor;
class LLButton;
class LLExpandableTextBox;
class LLMessageSystem;

class LLFloaterEvent : public LLFloater
{
public:
	LLFloaterEvent(const LLSD& key);
	/*virtual*/ ~LLFloaterEvent();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void setEventID(const U32 event_id);
	void sendEventInfoRequest();

	static void processEventInfoReply(LLMessageSystem *msg, void **);

	U32 getEventID() { return mEventID; }

protected:
	void resetInfo();

	static void onClickTeleport(void*);
	static void onClickMap(void*);
	//static void onClickLandmark(void*);
	static void onClickCreateEvent(void*);
	static void onClickNotify(void*);
	void onClickDeleteEvent();

	static void regionInfoCallback(U32 event_id, U64 region_handle);


protected:
	U32				mEventID;
	LLEventInfo		mEventInfo;

	LLTextBox*		mTBName;
	LLTextBox*		mTBCategory;
	LLTextBox*		mTBDate;
	LLTextBox*		mTBDuration;
	LLExpandableTextBox*	mTBDesc;

	LLTextBox*		mTBRunBy;
	LLTextBox*		mTBLocation;
	LLTextBox*		mTBCover;

	LLButton*		mTeleportBtn;
	LLButton*		mMapBtn;
	LLButton*		mCreateEventBtn;
	LLButton*		mGodDeleteEventBtn;
	LLButton*		mNotifyBtn;
};

#endif // LL_LLFLOATEREVENT_H
