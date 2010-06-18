/**
 * @file lloutfitslist.h
 * @brief List of agent's outfits for My Appearance side panel.
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

#ifndef LL_LLOUTFITSLIST_H
#define LL_LLOUTFITSLIST_H

#include "llaccordionctrl.h"
#include "llpanel.h"

// newview
#include "llinventorymodel.h"
#include "llinventoryobserver.h"

class LLAccordionCtrlTab;
class LLWearableItemsList;
class LLListContextMenu;

/**
 * @class LLOutfitTabNameComparator
 *
 * Comparator of outfit tabs.
 */
class LLOutfitTabNameComparator : public LLAccordionCtrl::LLTabComparator
{
	LOG_CLASS(LLOutfitTabNameComparator);

public:
	LLOutfitTabNameComparator() {};
	virtual ~LLOutfitTabNameComparator() {};

	/*virtual*/ bool compare(const LLAccordionCtrlTab* tab1, const LLAccordionCtrlTab* tab2) const;
};

/**
 * @class LLOutfitsList
 *
 * A list of agents's outfits from "My Outfits" inventory category
 * which displays each outfit in an accordion tab with a flat list
 * of items inside it.
 *
 * Starts fetching nevessary inventory content on first openning.
 */
class LLOutfitsList : public LLPanel
{
public:
	typedef boost::function<void (const LLUUID&)> selection_change_callback_t;
	typedef boost::signals2::signal<void (const LLUUID&)> selection_change_signal_t;

	LLOutfitsList();
	virtual ~LLOutfitsList();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& info);

	void refreshList(const LLUUID& category_id);

	// highlits currently worn outfit tab text and unhighlights previously worn
	void highlightBaseOutfit();

	void performAction(std::string action);

	void setFilterSubString(const std::string& string);

	const LLUUID& getSelectedOutfitUUID() const { return mSelectedOutfitUUID; }

	boost::signals2::connection addSelectionChangeCallback(selection_change_callback_t cb);

	/**
	 * Returns true if there is a selection inside currently selected outfit
	 */
	bool hasItemSelected();

private:
	/**
	 * Reads xml with accordion tab and Flat list from xml file.
	 *
	 * @return LLPointer to XMLNode with accordion tab and flat list.
	 */
	LLXMLNodePtr getAccordionTabXMLNode();

	/**
	 * Wrapper for LLCommonUtils::computeDifference. @see LLCommonUtils::computeDifference
	 */
	void computeDifference(const LLInventoryModel::cat_array_t& vcats, uuid_vec_t& vadded, uuid_vec_t& vremoved);

	/**
	 * Updates tab displaying outfit identified by category_id.
	 */
	void updateOutfitTab(const LLUUID& category_id);

	/**
	 * Resets previous selection and stores newly selected list and outfit id.
	 */
	void changeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id);

	/**
	 * Saves newly selected outfit ID.
	 */
	void setSelectedOutfitUUID(const LLUUID& category_id);

	/**
	 * Removes the outfit from selection.
	 */
	void deselectOutfit(const LLUUID& category_id);

	/**
	 * Try restoring selection for a temporary hidden tab.
	 *
	 * A tab may be hidden if it doesn't match current filter.
	 */
	void restoreOutfitSelection(LLAccordionCtrlTab* tab, const LLUUID& category_id);

	/**
	 * Called upon list refresh event to update tab visibility depending on
	 * the results of applying filter to the title and list items of the tab.
	 */
	void onFilteredWearableItemsListRefresh(LLUICtrl* ctrl);

	/**
	 * Highlights filtered items and hides tabs which haven't passed filter.
	 */
	void applyFilter(const std::string& new_filter_substring);

	/**
	 * Applies filter to the given tab
	 *
	 * @see applyFilter()
	 */
	void applyFilterToTab(const LLUUID& category_id, LLAccordionCtrlTab* tab, const std::string& filter_substring);

	void onAccordionTabRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id);
	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);
	void onCOFChanged();

	void onSelectionChange(LLUICtrl* ctrl);

	static void onOutfitRename(const LLSD& notification, const LLSD& response);

	LLInventoryCategoriesObserver* 	mCategoriesObserver;

	LLAccordionCtrl*				mAccordion;
	LLPanel*						mListCommands;

	typedef	std::map<LLUUID, LLWearableItemsList*>		wearables_lists_map_t;
	typedef wearables_lists_map_t::value_type			wearables_lists_map_value_t;
	wearables_lists_map_t			mSelectedListsMap;

	LLUUID							mSelectedOutfitUUID;
	// id of currently highlited outfit
	LLUUID							mHighlightedOutfitUUID;
	selection_change_signal_t		mSelectionChangeSignal;

	std::string 					mFilterSubString;

	typedef	std::map<LLUUID, LLAccordionCtrlTab*>		outfits_map_t;
	typedef outfits_map_t::value_type					outfits_map_value_t;
	outfits_map_t					mOutfitsMap;

	LLListContextMenu*			mOutfitMenu;

	bool							mIsInitialized;
	/**
	 * True if there is a selection inside currently selected outfit
	 */
	bool							mItemSelected;
};

#endif //LL_LLOUTFITSLIST_H
