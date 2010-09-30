/**
 * @file llpanelwearing.h
 * @brief List of agent's worn items.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLPANELWEARING_H
#define LL_LLPANELWEARING_H

#include "llpanel.h"

// newview
#include "llpanelappearancetab.h"

class LLInventoryCategoriesObserver;
class LLListContextMenu;
class LLWearableItemsList;
class LLWearingGearMenu;

/**
 * @class LLPanelWearing
 *
 * A list of agents's currently worn items represented by
 * a flat list view.
 * Starts fetching necessary inventory content on first opening.
 */
class LLPanelWearing : public LLPanelAppearanceTab
{
public:
	LLPanelWearing();
	virtual ~LLPanelWearing();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& info);

	/*virtual*/ void setFilterSubString(const std::string& string);

	/*virtual*/ bool isActionEnabled(const LLSD& userdata);

	/*virtual*/ void showGearMenu(LLView* spawning_view);

	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

	boost::signals2::connection setSelectionChangeCallback(commit_callback_t cb);

	bool hasItemSelected();

private:
	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);

	LLInventoryCategoriesObserver* 	mCategoriesObserver;
	LLWearableItemsList* 			mCOFItemsList;
	LLWearingGearMenu*				mGearMenu;
	LLListContextMenu*				mContextMenu;

	bool							mIsInitialized;
};

#endif //LL_LLPANELWEARING_H
