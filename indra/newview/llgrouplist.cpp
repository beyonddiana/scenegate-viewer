/** 
 * @file llgrouplist.cpp
 * @brief List of the groups the agent belongs to.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llgrouplist.h"

// newview
#include "llagent.h"

static LLDefaultWidgetRegistry::Register<LLGroupList> r("group_list");

LLGroupList::LLGroupList(const Params& p)
:	LLAvatarList(p)
{
}

BOOL LLGroupList::updateList()
{
	LLCtrlListInterface *group_list		= getListInterface();
	const LLUUID& 		highlight_id	= gAgent.getGroupID();
	S32					count			= gAgent.mGroups.count();
	LLUUID				id;

	group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	for(S32 i = 0; i < count; ++i)
	{
		// *TODO: check powers mask?
		id = gAgent.mGroups.get(i).mID;
		const LLGroupData& group_data = gAgent.mGroups.get(i);
		addItem(id, group_data.mName, highlight_id == id, ADD_BOTTOM);
	}

	// add "none" to list at top
	//name = LLTrans::getString("GroupsNone")
	addItem(LLUUID::null, std::string("none"), highlight_id.isNull(), ADD_TOP); // *TODO: localize

	group_list->selectByValue(highlight_id);
	return TRUE;
}
