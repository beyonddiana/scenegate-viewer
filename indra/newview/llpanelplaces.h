/** 
 * @file llpanelplaces.h
 * @brief Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELPLACES_H
#define LL_LLPANELPLACES_H

#include "lltimer.h"

#include "llpanel.h"

#include "llinventory.h"

#include "llinventorymodel.h"
#include "llpanelplaceinfo.h"

class LLInventoryItem;
class LLLandmark;
class LLPanelPlacesTab;
class LLFilterEditor;
class LLTabContainer;

typedef std::pair<LLUUID, std::string>	folder_pair_t;

class LLPanelPlaces : public LLPanel, LLInventoryObserver
{
public:
	LLPanelPlaces();
	virtual ~LLPanelPlaces();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void changed(U32 mask);
	/*virtual*/ void onOpen(const LLSD& key);

	void setItem(LLInventoryItem* item);

private:
	void onLandmarkLoaded(LLLandmark* landmark);
	void onFilterEdit(const std::string& search_string);
	void onTabSelected();

	//void onShareButtonClicked();
	void onTeleportButtonClicked();
	void onShowOnMapButtonClicked();
	void onOverflowButtonClicked();
	void onOverflowMenuItemClicked(const LLSD& param);
	void onCreateLandmarkButtonClicked(const LLUUID& folder_id);
	void onBackButtonClicked();

	void toggleMediaPanel();
	void togglePlaceInfoPanel(BOOL visible);

	void onAgentParcelChange();
	void updateVerbs();

	void showLandmarkFoldersMenu();

	LLFilterEditor*				mFilterEditor;
	LLPanelPlacesTab*			mActivePanel;
	LLTabContainer*				mTabContainer;
	LLPanelPlaceInfo*			mPlaceInfo;
	LLToggleableMenu*			mPlaceMenu;
	LLToggleableMenu*			mLandmarkMenu;

	LLButton*					mCreateLandmarkBtn;
	LLButton*					mFolderMenuBtn;
	LLButton*					mTeleportBtn;
	LLButton*					mShowOnMapBtn;
	LLButton*					mShareBtn;
	LLButton*					mOverflowBtn;

	// Pointer to a landmark item or to a linked landmark
	LLPointer<LLInventoryItem>	mItem;
	
	// Absolute position of the location for teleport, may not
	// be available (hence zero)
	LLVector3d					mPosGlobal;

	// Search string for filtering landmarks and teleport
	// history locations
	std::string					mFilterSubString;

	// Information type currently shown in Place Information panel
	std::string					mPlaceInfoType;

	// Menu handle for pop-up menu to chose a landmark saving
	// folder when creating a new landmark
	LLHandle<LLView> 			mLandmarkFoldersMenuHandle;

	typedef std::vector<folder_pair_t>	folder_vec_t;

	// List of folders to choose from when creating a landmark
	folder_vec_t				mLandmarkFoldersCache;
	
	// If root view width or height is changed
	// the pop-up menu must be updated
	S32							mRootViewWidth;
	S32							mRootViewHeight;
};

#endif //LL_LLPANELPLACES_H
