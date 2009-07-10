/** 
 * @file llfloaterinventory.cpp
 * @brief Implementation of the inventory view and associated stuff.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include <utility> // for std::pair<>

#include "llfloaterinventory.h"

// library includes
#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llfloaterreg.h"
#include "llsdserialize.h"
#include "llsearcheditor.h"
#include "llspinctrl.h"
#include "llui.h"
#include "message.h"

// newview includes
#include "llappviewer.h"
#include "llfirstuse.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewlandmark.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lluictrlfactory.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

static LLDefaultChildRegistry::Register<LLInventoryPanel> r("inventory_panel");

//BOOL LLFloaterInventory::sOpenNextNewItem = FALSE;
BOOL LLFloaterInventory::sWearNewClothing = FALSE;
LLUUID LLFloaterInventory::sWearNewClothingTransactionID;

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

LLFloaterInventoryFinder::LLFloaterInventoryFinder(LLFloaterInventory* inventory_view)
:	LLFloater(),
	mFloaterInventory(inventory_view),
	mFilter(inventory_view->mActivePanel->getFilter())
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inventory_view_finder.xml");
	updateElementsFromFilter();
}


void LLFloaterInventoryFinder::onCheckSinceLogoff(LLUICtrl *ctrl, void *user_data)
{
	LLFloaterInventoryFinder *self = (LLFloaterInventoryFinder *)user_data;
	if (!self) return;

	bool since_logoff= self->childGetValue("check_since_logoff");
	
	if (!since_logoff && 
	    !(  self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() ) )
	{
		self->mSpinSinceHours->set(1.0f);
	}	
}
BOOL LLFloaterInventoryFinder::postBuild()
{
	const LLRect& viewrect = mFloaterInventory->getRect();
	setRect(LLRect(viewrect.mLeft - getRect().getWidth(), viewrect.mTop, viewrect.mLeft, viewrect.mTop - getRect().getHeight()));

	childSetAction("All", selectAllTypes, this);
	childSetAction("None", selectNoTypes, this);

	mSpinSinceHours = getChild<LLSpinCtrl>("spin_hours_ago");
	childSetCommitCallback("spin_hours_ago", onTimeAgo, this);

	mSpinSinceDays = getChild<LLSpinCtrl>("spin_days_ago");
	childSetCommitCallback("spin_days_ago", onTimeAgo, this);

	//	mCheckSinceLogoff   = getChild<LLSpinCtrl>("check_since_logoff");
	childSetCommitCallback("check_since_logoff", onCheckSinceLogoff, this);

	childSetAction("Close", onCloseBtn, this);

	updateElementsFromFilter();
	return TRUE;
}
void LLFloaterInventoryFinder::onTimeAgo(LLUICtrl *ctrl, void *user_data)
{
	LLFloaterInventoryFinder *self = (LLFloaterInventoryFinder *)user_data;
	if (!self) return;
	
	bool since_logoff=true;
	if ( self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() )
	{
		since_logoff = false;
	}
	self->childSetValue("check_since_logoff", since_logoff);
}

void LLFloaterInventoryFinder::changeFilter(LLInventoryFilter* filter)
{
	mFilter = filter;
	updateElementsFromFilter();
}

void LLFloaterInventoryFinder::updateElementsFromFilter()
{
	if (!mFilter)
		return;

	// Get data needed for filter display
	U32 filter_types = mFilter->getFilterTypes();
	std::string filter_string = mFilter->getFilterSubString();
	LLInventoryFilter::EFolderShow show_folders = mFilter->getShowFolderState();
	U32 hours = mFilter->getHoursAgo();

	// update the ui elements
	LLFloater::setTitle(mFilter->getName());
	childSetValue("check_animation", (S32) (filter_types & 0x1 << LLInventoryType::IT_ANIMATION));

	childSetValue("check_calling_card", (S32) (filter_types & 0x1 << LLInventoryType::IT_CALLINGCARD));
	childSetValue("check_clothing", (S32) (filter_types & 0x1 << LLInventoryType::IT_WEARABLE));
	childSetValue("check_gesture", (S32) (filter_types & 0x1 << LLInventoryType::IT_GESTURE));
	childSetValue("check_landmark", (S32) (filter_types & 0x1 << LLInventoryType::IT_LANDMARK));
	childSetValue("check_notecard", (S32) (filter_types & 0x1 << LLInventoryType::IT_NOTECARD));
	childSetValue("check_object", (S32) (filter_types & 0x1 << LLInventoryType::IT_OBJECT));
	childSetValue("check_script", (S32) (filter_types & 0x1 << LLInventoryType::IT_LSL));
	childSetValue("check_sound", (S32) (filter_types & 0x1 << LLInventoryType::IT_SOUND));
	childSetValue("check_texture", (S32) (filter_types & 0x1 << LLInventoryType::IT_TEXTURE));
	childSetValue("check_snapshot", (S32) (filter_types & 0x1 << LLInventoryType::IT_SNAPSHOT));
	childSetValue("check_show_empty", show_folders == LLInventoryFilter::SHOW_ALL_FOLDERS);
	childSetValue("check_since_logoff", mFilter->isSinceLogoff());
	mSpinSinceHours->set((F32)(hours % 24));
	mSpinSinceDays->set((F32)(hours / 24));
}

void LLFloaterInventoryFinder::draw()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_DRAW);
	U32 filter = 0xffffffff;
	BOOL filtered_by_all_types = TRUE;

	if (!childGetValue("check_animation"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_ANIMATION);
		filtered_by_all_types = FALSE;
	}


	if (!childGetValue("check_calling_card"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_CALLINGCARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_clothing"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_WEARABLE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_gesture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_GESTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_landmark"))


	{
		filter &= ~(0x1 << LLInventoryType::IT_LANDMARK);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_notecard"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_NOTECARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_object"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_OBJECT);
		filter &= ~(0x1 << LLInventoryType::IT_ATTACHMENT);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_script"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_LSL);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_sound"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SOUND);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_texture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_TEXTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_snapshot"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SNAPSHOT);
		filtered_by_all_types = FALSE;
	}

	if (!filtered_by_all_types)
	{
		// don't include folders in filter, unless I've selected everything
		filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
	}

	// update the panel, panel will update the filter
	mFloaterInventory->mActivePanel->setShowFolderState(getCheckShowEmpty() ?
		LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mFloaterInventory->mActivePanel->setFilterTypes(filter);
	if (getCheckSinceLogoff())
	{
		mSpinSinceDays->set(0);
		mSpinSinceHours->set(0);
	}
	U32 days = (U32)mSpinSinceDays->get();
	U32 hours = (U32)mSpinSinceHours->get();
	if (hours > 24)
	{
		days += hours / 24;
		hours = (U32)hours % 24;
		mSpinSinceDays->set((F32)days);
		mSpinSinceHours->set((F32)hours);
	}
	hours += days * 24;
	mFloaterInventory->mActivePanel->setHoursAgo(hours);
	mFloaterInventory->mActivePanel->setSinceLogoff(getCheckSinceLogoff());
	mFloaterInventory->setFilterTextFromFilter();

	LLFloater::draw();
}

void  LLFloaterInventoryFinder::onClose(bool app_quitting)
{
	gSavedSettings.setBOOL("Inventory.ShowFilters", FALSE);
	// If you want to reset the filter on close, do it here.  This functionality was
	// hotly debated - Paulm
#if 0
	if (mFloaterInventory)
	{
		LLFloaterInventory::onResetFilter((void *)mFloaterInventory);
	}
#endif
	destroy();
}


BOOL LLFloaterInventoryFinder::getCheckShowEmpty()
{
	return childGetValue("check_show_empty");
}

BOOL LLFloaterInventoryFinder::getCheckSinceLogoff()
{
	return childGetValue("check_since_logoff");
}

void LLFloaterInventoryFinder::onCloseBtn(void* user_data)
{
	LLFloaterInventoryFinder* finderp = (LLFloaterInventoryFinder*)user_data;
	finderp->closeFloater();
}

// static
void LLFloaterInventoryFinder::selectAllTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	self->childSetValue("check_animation", TRUE);
	self->childSetValue("check_calling_card", TRUE);
	self->childSetValue("check_clothing", TRUE);
	self->childSetValue("check_gesture", TRUE);
	self->childSetValue("check_landmark", TRUE);
	self->childSetValue("check_notecard", TRUE);
	self->childSetValue("check_object", TRUE);
	self->childSetValue("check_script", TRUE);
	self->childSetValue("check_sound", TRUE);
	self->childSetValue("check_texture", TRUE);
	self->childSetValue("check_snapshot", TRUE);

/*
	self->mCheckCallingCard->set(TRUE);
	self->mCheckClothing->set(TRUE);
	self->mCheckGesture->set(TRUE);
	self->mCheckLandmark->set(TRUE);
	self->mCheckNotecard->set(TRUE);
	self->mCheckObject->set(TRUE);
	self->mCheckScript->set(TRUE);
	self->mCheckSound->set(TRUE);
	self->mCheckTexture->set(TRUE);
	self->mCheckSnapshot->set(TRUE);*/
}

//static
void LLFloaterInventoryFinder::selectNoTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	/*
	self->childSetValue("check_animation", FALSE);
	self->mCheckCallingCard->set(FALSE);
	self->mCheckClothing->set(FALSE);
	self->mCheckGesture->set(FALSE);
	self->mCheckLandmark->set(FALSE);
	self->mCheckNotecard->set(FALSE);
	self->mCheckObject->set(FALSE);
	self->mCheckScript->set(FALSE);
	self->mCheckSound->set(FALSE);
	self->mCheckTexture->set(FALSE);
	self->mCheckSnapshot->set(FALSE);*/


	self->childSetValue("check_animation", FALSE);
	self->childSetValue("check_calling_card", FALSE);
	self->childSetValue("check_clothing", FALSE);
	self->childSetValue("check_gesture", FALSE);
	self->childSetValue("check_landmark", FALSE);
	self->childSetValue("check_notecard", FALSE);
	self->childSetValue("check_object", FALSE);
	self->childSetValue("check_script", FALSE);
	self->childSetValue("check_sound", FALSE);
	self->childSetValue("check_texture", FALSE);
	self->childSetValue("check_snapshot", FALSE);
}


///----------------------------------------------------------------------------
/// LLFloaterInventory
///----------------------------------------------------------------------------
void LLSaveFolderState::setApply(BOOL apply)
{
	mApply = apply; 
	// before generating new list of open folders, clear the old one
	if(!apply) 
	{
		clearOpenFolders(); 
	}
}

void LLSaveFolderState::doFolder(LLFolderViewFolder* folder)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_DO_FOLDER);
	if(mApply)
	{
		// we're applying the open state
		LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
		if(!bridge) return;
		LLUUID id(bridge->getUUID());
		if(mOpenFolders.find(id) != mOpenFolders.end())
		{
			folder->setOpen(TRUE);
		}
		else
		{
			// keep selected filter in its current state, this is less jarring to user
			if (!folder->isSelected())
			{
				folder->setOpen(FALSE);
			}
		}
	}
	else
	{
		// we're recording state at this point
		if(folder->isOpen())
		{
			LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
			if(!bridge) return;
			mOpenFolders.insert(bridge->getUUID());
		}
	}
}

LLFloaterInventory::LLFloaterInventory(const LLSD& key)
	: LLFloater(key)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_VIEW_INIT);
	// Menu Callbacks (non contex menus)
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLFloaterInventory::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.CloseAllFolders", boost::bind(&LLFloaterInventory::closeAllFolders, this));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLAssetType::AT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLAssetType::AT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&LLFloaterInventory::doCreate, this, _2));
	mCommitCallbackRegistrar.add("Inventory.NewWindow", boost::bind(&LLFloaterInventory::newWindow, this));
	mCommitCallbackRegistrar.add("Inventory.ShowFilters", boost::bind(&LLFloaterInventory::toggleFindOptions, this));
	mCommitCallbackRegistrar.add("Inventory.ResetFilters", boost::bind(&LLFloaterInventory::resetFilters, this));
	mCommitCallbackRegistrar.add("Inventory.SetSortBy", boost::bind(&LLFloaterInventory::setSortBy, this, _2));

	// Controls
	// *TODO: Just use persistant settings for each of these
	U32 sort_order = gSavedSettings.getU32("InventorySortOrder");
	BOOL sort_by_name = ! ( sort_order & LLInventoryFilter::SO_DATE );
	BOOL sort_folders_by_name = ( sort_order & LLInventoryFilter::SO_FOLDERS_BY_NAME );
	BOOL sort_system_folders_to_top = ( sort_order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP );
	
	gSavedSettings.declareBOOL("Inventory.ShowFilters", FALSE, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.SortByName", sort_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.SortByDate", !sort_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.FoldersAlwaysByName", sort_folders_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.SystemFoldersToTop", sort_system_folders_to_top, "Declared in code", FALSE);
	
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);

	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inventory.xml");
}

BOOL LLFloaterInventory::postBuild()
{
	gInventory.addObserver(this);
	
	mFilterTabs = getChild<LLTabContainer>("inventory filter tabs");
	mFilterTabs->setCommitCallback(boost::bind(&LLFloaterInventory::onFilterSelected, this));
	
	//panel->getFilter()->markDefault();

	// Set up the default inv. panel/filter settings.
	mActivePanel = getChild<LLInventoryPanel>("All Items");
	if (mActivePanel)
	{
		// "All Items" is the previous only view, so it gets the InventorySortOrder
		mActivePanel->setSortOrder(gSavedSettings.getU32("InventorySortOrder"));
		mActivePanel->getFilter()->markDefault();
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		mActivePanel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, mActivePanel, _1, _2));
	}
	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		recent_items_panel->setSinceLogoff(TRUE);
		recent_items_panel->setSortOrder(LLInventoryFilter::SO_DATE);
		recent_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		recent_items_panel->getFilter()->markDefault();
		recent_items_panel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, recent_items_panel, _1, _2));
	}

	// Now load the stored settings from disk, if available.
	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "filters.xml");
	llinfos << "LLFloaterInventory::init: reading from " << filterSaveName << llendl;
	llifstream file(filterSaveName.str());
	LLSD savedFilterState;
	if (file.is_open())
	{
		LLSDSerialize::fromXML(savedFilterState, file);
		file.close();

		// Load the persistent "Recent Items" settings.
		// Note that the "All Items" settings do not persist.
		if(recent_items_panel)
		{
			if(savedFilterState.has(recent_items_panel->getFilter()->getName()))
			{
				LLSD recent_items = savedFilterState.get(
					recent_items_panel->getFilter()->getName());
				recent_items_panel->getFilter()->fromLLSD(recent_items);
			}
		}

	}


	mSearchEditor = getChild<LLSearchEditor>("inventory search editor");
	if (mSearchEditor)
	{
		mSearchEditor->setSearchCallback(boost::bind(&LLFloaterInventory::onSearchEdit, this, _1));
	}

	// *TODO:Get the cost info from the server
	const std::string upload_cost("10");
	childSetLabelArg("Upload Image", "[COST]", upload_cost);
	childSetLabelArg("Upload Sound", "[COST]", upload_cost);
	childSetLabelArg("Upload Animation", "[COST]", upload_cost);
	childSetLabelArg("Bulk Upload", "[COST]", upload_cost);
	
	return TRUE;
}

// Destroys the object
LLFloaterInventory::~LLFloaterInventory( void )
{
	// Save the filters state.
	LLSD filterRoot;
	LLInventoryPanel* all_items_panel = getChild<LLInventoryPanel>("All Items");
	if (all_items_panel)
	{
		LLInventoryFilter* filter = all_items_panel->getFilter();
		LLSD filterState;
		filter->toLLSD(filterState);
		filterRoot[filter->getName()] = filterState;
	}

	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		LLInventoryFilter* filter = recent_items_panel->getFilter();
		LLSD filterState;
		filter->toLLSD(filterState);
		filterRoot[filter->getName()] = filterState;
	}

	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "filters.xml");
	llofstream filtersFile(filterSaveName.str());
	if(!LLSDSerialize::toPrettyXML(filterRoot, filtersFile))
	{
		llwarns << "Could not write to filters save file " << filterSaveName << llendl;
	}
	else
		filtersFile.close();

	gInventory.removeObserver(this);
	delete mSavedFolderState;
}

void LLFloaterInventory::draw()
{
 	if (LLInventoryModel::isEverythingFetched())
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		std::ostringstream title;
		//title << "Inventory";
		title<<getString("Title");
		std::string item_count_string;
		LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());
		title << " (" << item_count_string << getString("Items")<<")";
		//TODO:: Translate mFilterText
		title << mFilterText;
		setTitle(title.str());
	}
	if (mActivePanel && mSearchEditor)
	{
		mSearchEditor->setText(mActivePanel->getFilterSubString());
	}
	LLFloater::draw();
}

void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && folder->getParentFolder())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
	// if this folder didn't pass the filter, and none of its descendants did
	else if (!folder->getFiltered() && !folder->hasFilteredDescendants())
	{
		folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_NO);
	}
}

void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered() && !mItemSelected)
	{
		item->getRoot()->setSelection(item, FALSE, FALSE);
		if (item->getParentFolder())
		{
			item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		item->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}

void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && !mItemSelected)
	{
		folder->getRoot()->setSelection(folder, FALSE, FALSE);
		if (folder->getParentFolder())
		{
			folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		folder->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}

void LLOpenFoldersWithSelection::doItem(LLFolderViewItem *item)
{
	if (item->getParentFolder() && item->isSelected())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFoldersWithSelection::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getParentFolder() && folder->isSelected())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLFloaterInventory::startSearch()
{
	// this forces focus to line editor portion of search editor
	if (mSearchEditor)
	{
		mSearchEditor->focusFirstItem(TRUE);
	}
}

// virtual, from LLView
void LLFloaterInventory::setVisible( BOOL visible )
{
	LLFloater::setVisible(visible);
}

void LLFloaterInventory::onOpen(const LLSD& key)
{
	LLFirstUse::useInventory();
}

// Destroy all but the last floater, which is made invisible.
void LLFloaterInventory::onClose(bool app_quitting)
{
	if (getKey().asInteger() != 0)
	{
		destroy();
	}
	else
	{
		// clear filters, but save user's folder state first
		if (!mActivePanel->getRootFolder()->isFilterModified())
		{
			mSavedFolderState->setApply(FALSE);
			mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		}
		setVisible(FALSE);
	}
}

BOOL LLFloaterInventory::handleKeyHere(KEY key, MASK mask)
{
	LLFolderView* root_folder = mActivePanel ? mActivePanel->getRootFolder() : NULL;
	if (root_folder)
	{
		// first check for user accepting current search results
		if (mSearchEditor 
			&& mSearchEditor->hasFocus()
		    && (key == KEY_RETURN 
		    	|| key == KEY_DOWN)
		    && mask == MASK_NONE)
		{
			// move focus to inventory proper
			mActivePanel->setFocus(TRUE);
			root_folder->scrollToShowSelection();
			return TRUE;
		}

		if (mActivePanel->hasFocus() && key == KEY_UP)
		{
			startSearch();
		}
	}

	return LLFloater::handleKeyHere(key, mask);

}

void LLFloaterInventory::changed(U32 mask)
{
	std::ostringstream title;
	//title << "Inventory";
 	title<<getString("Title");
	if (LLInventoryModel::backgroundFetchActive())
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		std::string item_count_string;
		LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());
		title << " ( "<< getString("Fetched") << item_count_string << getString("Items")<<")";
	}
	//TODO:: Translate mFilterText
	title << mFilterText;
	setTitle(title.str());

}

//static
LLFloaterInventory* LLFloaterInventory::newInstance()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_VIEW_SHOW);
	static S32 inst_count = 1;
	return LLFloaterReg::getTypedInstance<LLFloaterInventory>("inventory", LLSD(inst_count++));
}

//----------------------------------------------------------------------------
// menu callbacks

void LLFloaterInventory::doToSelected(const LLSD& userdata)
{
	getPanel()->getRootFolder()->doToSelected(&gInventory, userdata);
}

void LLFloaterInventory::closeAllFolders()
{
	getPanel()->getRootFolder()->closeAllFolders();
}

void LLFloaterInventory::doCreate(const LLSD& userdata)
{
	menu_create_inventory_item(getPanel()->getRootFolder(), NULL, userdata);
}

void LLFloaterInventory::newWindow()
{
	LLFloaterInventory* iv = newInstance();
	iv->getActivePanel()->setFilterTypes(getActivePanel()->getFilterTypes());
	iv->getActivePanel()->setFilterSubString(getActivePanel()->getFilterSubString());
	iv->openFloater();

	// force onscreen
	gFloaterView->adjustToFitScreen(iv, FALSE);
}

void LLFloaterInventory::resetFilters()
{
	LLFloaterInventoryFinder *finder = getFinder();
	getActivePanel()->getFilter()->resetDefault();
	if (finder)
	{
		finder->updateElementsFromFilter();
	}

	setFilterTextFromFilter();
}

void LLFloaterInventory::setSortBy(const LLSD& userdata)
{
	std::string sort_field = userdata.asString();
	if (sort_field == "name")
	{
		U32 order = getActivePanel()->getSortOrder();
		getActivePanel()->setSortOrder( order & ~LLInventoryFilter::SO_DATE );
			
		gSavedSettings.setBOOL("Inventory.SortByName", TRUE );
		gSavedSettings.setBOOL("Inventory.SortByDate", FALSE );
	}
	else if (sort_field == "date")
	{
		U32 order = getActivePanel()->getSortOrder();
		getActivePanel()->setSortOrder( order | LLInventoryFilter::SO_DATE );

		gSavedSettings.setBOOL("Inventory.SortByName", FALSE );
		gSavedSettings.setBOOL("Inventory.SortByDate", TRUE );
	}
	else if (sort_field == "foldersalwaysbyname")
	{
		U32 order = getActivePanel()->getSortOrder();
		if ( order & LLInventoryFilter::SO_FOLDERS_BY_NAME )
		{
			order &= ~LLInventoryFilter::SO_FOLDERS_BY_NAME;

			gSavedSettings.setBOOL("Inventory.FoldersAlwaysByName", FALSE );
		}
		else
		{
			order |= LLInventoryFilter::SO_FOLDERS_BY_NAME;

			gSavedSettings.setBOOL("Inventory.FoldersAlwaysByName", TRUE );
		}
		getActivePanel()->setSortOrder( order );
	}
	else if (sort_field == "systemfolderstotop")
	{
		U32 order = getActivePanel()->getSortOrder();
		if ( order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP )
		{
			order &= ~LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;

			gSavedSettings.setBOOL("Inventory.SystemFoldersToTop", FALSE );
		}
		else
		{
			order |= LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;

			gSavedSettings.setBOOL("Inventory.SystemFoldersToTop", TRUE );
		}
		getActivePanel()->setSortOrder( order );
	}
}

//----------------------------------------------------------------------------

// static
LLFloaterInventory* LLFloaterInventory::showAgentInventory()
{
	LLFloaterInventory* iv = NULL;
	if (!gAgent.cameraMouselook())
	{
		iv = LLFloaterReg::showTypedInstance<LLFloaterInventory>("inventory", LLSD());
	}
	return iv;
}

// static
LLFloaterInventory* LLFloaterInventory::getActiveInventory()
{
	LLFloaterInventory* res = NULL;
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	S32 z_min = S32_MAX;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterInventory* iv = dynamic_cast<LLFloaterInventory*>(*iter);
		if (iv)
		{
			S32 z_order = gFloaterView->getZOrder(iv);
			if (z_order < z_min)
			{
				res = iv;
				z_min = z_order;
			}
		}
	}
	return res;
}

// static
void LLFloaterInventory::cleanup()
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
	{
		LLFloaterInventory* iv = dynamic_cast<LLFloaterInventory*>(*iter++);
		if (iv)
		{
			iv->destroy();
		}
	}
}

void LLFloaterInventory::toggleFindOptions()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_VIEW_TOGGLE);
	LLFloater *floater = getFinder();
	if (!floater)
	{
		LLFloaterInventoryFinder * finder = new LLFloaterInventoryFinder(this);
		mFinderHandle = finder->getHandle();
		finder->openFloater();
		addDependentFloater(mFinderHandle);

		// start background fetch of folders
		gInventory.startBackgroundFetch();

		gSavedSettings.setBOOL("Inventory.ShowFilters", TRUE);
	}
	else
	{
		floater->closeFloater();

		gSavedSettings.setBOOL("Inventory.ShowFilters", FALSE);
	}
}

// static
BOOL LLFloaterInventory::filtersVisible(void* user_data)
{
	LLFloaterInventory* self = (LLFloaterInventory*)user_data;
	if(!self) return FALSE;

	return self->getFinder() != NULL;
}

void LLFloaterInventory::onClearSearch()
{
	LLFloater *finder = getFinder();
	if (mActivePanel)
	{
		mActivePanel->setFilterSubString(LLStringUtil::null);
		mActivePanel->setFilterTypes(0xffffffff);
	}

	if (finder)
	{
		LLFloaterInventoryFinder::selectAllTypes(finder);
	}

	// re-open folders that were initially open
	if (mActivePanel)
	{
		mSavedFolderState->setApply(TRUE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mActivePanel->getRootFolder()->applyFunctorRecursively(opener);
		mActivePanel->getRootFolder()->scrollToShowSelection();
	}
}

void LLFloaterInventory::onSearchEdit(const std::string& search_string )
{
	if (search_string == "")
	{
		onClearSearch();
	}
	if (!mActivePanel)
	{
		return;
	}

	gInventory.startBackgroundFetch();

	std::string filter_text = search_string;
	std::string uppercase_search_string = filter_text;
	LLStringUtil::toUpper(uppercase_search_string);
	if (mActivePanel->getFilterSubString().empty() && uppercase_search_string.empty())
	{
			// current filter and new filter empty, do nothing
			return;
	}

	// save current folder open state if no filter currently applied
	if (!mActivePanel->getRootFolder()->isFilterModified())
	{
		mSavedFolderState->setApply(FALSE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	mActivePanel->setFilterSubString(uppercase_search_string);
}


 //static
 BOOL LLFloaterInventory::incrementalFind(LLFolderViewItem* first_item, const char *find_text, BOOL backward)
 {
 	LLFloaterInventory* active_view = NULL;
	
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterInventory* iv = dynamic_cast<LLFloaterInventory*>(*iter);
		if (iv)
		{
			if (gFocusMgr.childHasKeyboardFocus(iv))
			{
				active_view = iv;
				break;
			}
 		}
 	}

 	if (!active_view)
 	{
 		return FALSE;
 	}

 	std::string search_string(find_text);

 	if (search_string.empty())
 	{
 		return FALSE;
 	}

 	if (active_view->mActivePanel &&
 		active_view->mActivePanel->getRootFolder()->search(first_item, search_string, backward))
 	{
 		return TRUE;
 	}

 	return FALSE;
 }

void LLFloaterInventory::onFilterSelected()
{
	// Find my index
	mActivePanel = (LLInventoryPanel*)childGetVisibleTab("inventory filter tabs");

	if (!mActivePanel)
	{
		return;
	}
	LLInventoryFilter* filter = mActivePanel->getFilter();
	LLFloaterInventoryFinder *finder = getFinder();
	if (finder)
	{
		finder->changeFilter(filter);
	}
	if (filter->isActive())
	{
		// If our filter is active we may be the first thing requiring a fetch so we better start it here.
		gInventory.startBackgroundFetch();
	}
	setFilterTextFromFilter();
}

BOOL LLFloaterInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	// Check to see if we are auto scrolling from the last frame
	LLInventoryPanel* panel = (LLInventoryPanel*)this->getActivePanel();
	BOOL needsToScroll = panel->getScrollableContainer()->needsToScroll(x, y, LLScrollContainer::VERTICAL);
	if(mFilterTabs)
	{
		if(needsToScroll)
		{
			mFilterTabs->startDragAndDropDelayTimer();
		}
	}
	
	BOOL handled = LLFloater::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	return handled;
}
const std::string& get_item_icon_name(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point,
							 BOOL item_is_multi )
{
	EInventoryIcon idx = OBJECT_ICON_NAME;
	if ( item_is_multi )
	{
		idx = OBJECT_MULTI_ICON_NAME;
	}
	
	switch(asset_type)
	{
	case LLAssetType::AT_TEXTURE:
		if(LLInventoryType::IT_SNAPSHOT == inventory_type)
		{
			idx = SNAPSHOT_ICON_NAME;
		}
		else
		{
			idx = TEXTURE_ICON_NAME;
		}
		break;

	case LLAssetType::AT_SOUND:
		idx = SOUND_ICON_NAME;
		break;
	case LLAssetType::AT_CALLINGCARD:
		if(attachment_point!= 0)
		{
			idx = CALLINGCARD_ONLINE_ICON_NAME;
		}
		else
		{
			idx = CALLINGCARD_OFFLINE_ICON_NAME;
		}
		break;
	case LLAssetType::AT_LANDMARK:
		if(attachment_point!= 0)
		{
			idx = LANDMARK_VISITED_ICON_NAME;
		}
		else
		{
			idx = LANDMARK_ICON_NAME;
		}
		break;
	case LLAssetType::AT_SCRIPT:
	case LLAssetType::AT_LSL_TEXT:
	case LLAssetType::AT_LSL_BYTECODE:
		idx = SCRIPT_ICON_NAME;
		break;
	case LLAssetType::AT_CLOTHING:
		idx = CLOTHING_ICON_NAME;
	case LLAssetType::AT_BODYPART :
		if(LLAssetType::AT_BODYPART == asset_type)
		{
			idx = BODYPART_ICON_NAME;
		}
		switch(LLInventoryItem::II_FLAGS_WEARABLES_MASK & attachment_point)
		{
		case WT_SHAPE:
			idx = BODYPART_SHAPE_ICON_NAME;
			break;
		case WT_SKIN:
			idx = BODYPART_SKIN_ICON_NAME;
			break;
		case WT_HAIR:
			idx = BODYPART_HAIR_ICON_NAME;
			break;
		case WT_EYES:
			idx = BODYPART_EYES_ICON_NAME;
			break;
		case WT_SHIRT:
			idx = CLOTHING_SHIRT_ICON_NAME;
			break;
		case WT_PANTS:
			idx = CLOTHING_PANTS_ICON_NAME;
			break;
		case WT_SHOES:
			idx = CLOTHING_SHOES_ICON_NAME;
			break;
		case WT_SOCKS:
			idx = CLOTHING_SOCKS_ICON_NAME;
			break;
		case WT_JACKET:
			idx = CLOTHING_JACKET_ICON_NAME;
			break;
		case WT_GLOVES:
			idx = CLOTHING_GLOVES_ICON_NAME;
			break;
		case WT_UNDERSHIRT:
			idx = CLOTHING_UNDERSHIRT_ICON_NAME;
			break;
		case WT_UNDERPANTS:
			idx = CLOTHING_UNDERPANTS_ICON_NAME;
			break;
		case WT_SKIRT:
			idx = CLOTHING_SKIRT_ICON_NAME;
			break;
		case WT_ALPHA:
			idx = CLOTHING_ALPHA_ICON_NAME;
			break;
		case WT_TATTOO:
			idx = CLOTHING_TATTOO_ICON_NAME;
			break;
		default:
			// no-op, go with choice above
			break;
		}
		break;
	case LLAssetType::AT_NOTECARD:
		idx = NOTECARD_ICON_NAME;
		break;
	case LLAssetType::AT_ANIMATION:
		idx = ANIMATION_ICON_NAME;
		break;
	case LLAssetType::AT_GESTURE:
		idx = GESTURE_ICON_NAME;
		break;
	case LLAssetType::AT_FAVORITE:
		//TODO - need bette idx
		idx = LANDMARK_ICON_NAME;
		break;
	default:
		break;
	}
	
	return ICON_NAME[idx];
}

LLUIImagePtr get_item_icon(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point,
							 BOOL item_is_multi)
{
	const std::string& icon_name = get_item_icon_name(asset_type, inventory_type, attachment_point, item_is_multi );
	return LLUI::getUIImage(icon_name);
}

const std::string LLInventoryPanel::DEFAULT_SORT_ORDER = std::string("InventorySortOrder");
const std::string LLInventoryPanel::RECENTITEMS_SORT_ORDER = std::string("RecentItemsSortOrder");
const std::string LLInventoryPanel::INHERIT_SORT_ORDER = std::string("");

LLInventoryPanel::LLInventoryPanel(const LLInventoryPanel::Params& p)
:	LLPanel(p),
	mInventoryObserver(NULL),
	mFolders(NULL),
	mScroller(NULL),
	mSortOrderSetting(p.sort_order_setting),
	mInventory(p.inventory),
	mAllowMultiSelect(p.allow_multi_select)
{
	// contex menu callbacks
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLInventoryPanel::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLAssetType::AT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLAssetType::AT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&LLInventoryPanel::doCreate, this, _2));
	mCommitCallbackRegistrar.add("Inventory.AttachObject", boost::bind(&LLInventoryPanel::attachObject, this, _2));
	mCommitCallbackRegistrar.add("Inventory.BeginIMSession", boost::bind(&LLInventoryPanel::beginIMSession, this));
	
	setBackgroundColor(LLUIColorTable::instance().getColor("InventoryBackgroundColor"));
	setBackgroundVisible(TRUE);
	setBackgroundOpaque(TRUE);
}

BOOL LLInventoryPanel::postBuild()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_POST_BUILD);

	mCommitCallbackRegistrar.pushScope(); // registered as a widget; need to push callback scope ourselves
	
	// create root folder
	{
		LLRect folder_rect(0,
						   0,
						   getRect().getWidth(),
						   0);
		LLFolderView::Params p;
		p.name = getName();
		p.rect = folder_rect;
		p.parent_panel = this;
		mFolders = LLUICtrlFactory::create<LLFolderView>(p);
		mFolders->setAllowMultiSelect(mAllowMultiSelect);
	}

	mCommitCallbackRegistrar.popScope();
	
	mFolders->setCallbackRegistrar(&mCommitCallbackRegistrar);
	
	// scroller
	{
		LLRect scroller_view_rect = getRect();
		scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
		LLScrollContainer::Params p;
		p.name("Inventory Scroller");
		p.rect(scroller_view_rect);
		p.follows.flags(FOLLOWS_ALL);
		p.reserve_scroll_corner(true);
		p.tab_stop(true);
		mScroller = LLUICtrlFactory::create<LLScrollContainer>(p);
	}
	addChild(mScroller);
	mScroller->addChild(mFolders);
	
	mFolders->setScrollContainer(mScroller);

	// set up the callbacks from the inventory we're viewing, and then
	// build everything.
	mInventoryObserver = new LLInventoryPanelObserver(this);
	mInventory->addObserver(mInventoryObserver);
	rebuildViewsFor(LLUUID::null, LLInventoryObserver::ADD);

	// bit of a hack to make sure the inventory is open.
	mFolders->openFolder(std::string("My Inventory"));

	if (mSortOrderSetting != INHERIT_SORT_ORDER)
	{
		setSortOrder(gSavedSettings.getU32(mSortOrderSetting));
	}
	else
	{
		setSortOrder(gSavedSettings.getU32(DEFAULT_SORT_ORDER));
	}
	mFolders->setSortOrder(mFolders->getFilter()->getSortOrder());

	return TRUE;
}

LLInventoryPanel::~LLInventoryPanel()
{
	// should this be a global setting?
	U32 sort_order = mFolders->getSortOrder();
	if (mSortOrderSetting != INHERIT_SORT_ORDER)
	{
		gSavedSettings.setU32(mSortOrderSetting, sort_order);
	}

	// LLView destructor will take care of the sub-views.
	mInventory->removeObserver(mInventoryObserver);
	delete mInventoryObserver;
	mScroller = NULL;
}

	LLMemType mt(LLMemType::MTYPE_INVENTORY_FROM_XML);
void LLInventoryPanel::draw()
{
	// select the desired item (in case it wasn't loaded when the selection was requested)
	mFolders->updateSelection();
	LLPanel::draw();
}

void LLInventoryPanel::setFilterTypes(U32 filter_types)
{
	mFolders->getFilter()->setFilterTypes(filter_types);
}	

void LLInventoryPanel::setFilterPermMask(PermissionMask filter_perm_mask)
{
	mFolders->getFilter()->setFilterPermissions(filter_perm_mask);
}

void LLInventoryPanel::setFilterSubString(const std::string& string)
{
	mFolders->getFilter()->setFilterSubString(string);
}

void LLInventoryPanel::setSortOrder(U32 order)
{
	mFolders->getFilter()->setSortOrder(order);
	if (mFolders->getFilter()->isModified())
	{
		mFolders->setSortOrder(order);
		// try to keep selection onscreen, even if it wasn't to start with
		mFolders->scrollToShowSelection();
	}
}

void LLInventoryPanel::setSinceLogoff(BOOL sl)
{
	mFolders->getFilter()->setDateRangeLastLogoff(sl);
}

void LLInventoryPanel::setHoursAgo(U32 hours)
{
	mFolders->getFilter()->setHoursAgo(hours);
}

void LLInventoryPanel::setShowFolderState(LLInventoryFilter::EFolderShow show)
{
	mFolders->getFilter()->setShowFolderState(show);
}

LLInventoryFilter::EFolderShow LLInventoryPanel::getShowFolderState()
{
	return mFolders->getFilter()->getShowFolderState();
}

void LLInventoryPanel::modelChanged(U32 mask)
{
	LLFastTimer t2(LLFastTimer::FTM_REFRESH);

	bool handled = false;
	if(mask & LLInventoryObserver::LABEL)
	{
		handled = true;
		// label change - empty out the display name for each object
		// in this change set.
		const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();
		std::set<LLUUID>::const_iterator id_it = changed_items.begin();
		std::set<LLUUID>::const_iterator id_end = changed_items.end();
		LLFolderViewItem* view = NULL;
		LLInvFVBridge* bridge = NULL;
		for (;id_it != id_end; ++id_it)
		{
			view = mFolders->getItemByID(*id_it);
			if(view)
			{
				// request refresh on this item (also flags for filtering)
				bridge = (LLInvFVBridge*)view->getListener();
				if(bridge)
				{	// Clear the display name first, so it gets properly re-built during refresh()
					bridge->clearDisplayName();
				}
				view->refresh();
			}
		}
	}
	if((mask & (LLInventoryObserver::STRUCTURE
				| LLInventoryObserver::ADD
				| LLInventoryObserver::REMOVE)) != 0)
	{
		handled = true;
		// Record which folders are open by uuid.
		LLInventoryModel* model = getModel();
		if (model)
		{
			const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				// sync view with model
				LLInventoryObject* model_item = model->getObject(*id_it);
				LLFolderViewItem* view_item = mFolders->getItemByID(*id_it);

				if (model_item)
				{
					if (!view_item)
					{
						// this object was just created, need to build a view for it
						if ((mask & LLInventoryObserver::ADD) != LLInventoryObserver::ADD)
						{
							llwarns << *id_it << " is in model but not in view, but ADD flag not set" << llendl;
						}
						buildNewViews(*id_it);
						
						// select any newly created object
						// that has the auto rename at top of folder
						// root set
						if(mFolders->getRoot()->needsAutoRename())
						{
							setSelection(*id_it, FALSE);
						}
					}
					else
					{
						// this object was probably moved, check its parent
						if ((mask & LLInventoryObserver::STRUCTURE) != LLInventoryObserver::STRUCTURE)
						{
							llwarns << *id_it << " is in model and in view, but STRUCTURE flag not set" << llendl;
						}

						LLFolderViewFolder* new_parent = (LLFolderViewFolder*)mFolders->getItemByID(model_item->getParentUUID());
						if (view_item->getParentFolder() != new_parent)
						{
							view_item->getParentFolder()->extractItem(view_item);
							view_item->addToFolder(new_parent, mFolders);
						}
					}
				}
				else
				{
					if (view_item)
					{
						if ((mask & LLInventoryObserver::REMOVE) != LLInventoryObserver::REMOVE)
						{
							llwarns << *id_it << " is not in model but in view, but REMOVE flag not set" << llendl;
						}
						// item in view but not model, need to delete view
						view_item->destroyView();
					}
					else
					{
						llwarns << *id_it << "Item does not exist in either view or model, but notification triggered" << llendl;
					}
				}
			}
		}
	}

	if (!handled)
	{
		// it's a small change that only requires a refresh.
		// *TODO: figure out a more efficient way to do the refresh
		// since it is expensive on large inventories
		mFolders->refresh();
	}
}

void LLInventoryPanel::rebuildViewsFor(const LLUUID& id, U32 mask)
{
	LLFolderViewItem* old_view = NULL;

	// get old LLFolderViewItem
	old_view = mFolders->getItemByID(id);
	if (old_view && id.notNull())
	{
		old_view->destroyView();
	}

	buildNewViews(id);
}

void LLInventoryPanel::buildNewViews(const LLUUID& id)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_BUILD_NEW_VIEWS);
	LLFolderViewItem* itemp = NULL;
	LLInventoryObject* objectp = gInventory.getObject(id);

	if (objectp)
	{		
		if (objectp->getType() <= LLAssetType::AT_NONE ||
			objectp->getType() >= LLAssetType::AT_COUNT)
		{
			llwarns << "LLInventoryPanel::buildNewViews called with objectp->mType == " 
				<< ((S32) objectp->getType())
				<< " (shouldn't happen)" << llendl;
		}
		else if (objectp->getType() == LLAssetType::AT_CATEGORY) // build new view for category
		{
			LLInvFVBridge* new_listener = LLInvFVBridge::createBridge(objectp->getType(),
													LLInventoryType::IT_CATEGORY,
													this,
													objectp->getUUID());

			if (new_listener)
			{
				LLFolderViewFolder::Params p;
				p.name = new_listener->getDisplayName();
				p.icon = new_listener->getIcon();
				p.root = mFolders;
				p.listener = new_listener;
				LLFolderViewFolder* folderp = LLUICtrlFactory::create<LLFolderViewFolder>(p);
				
				folderp->setItemSortOrder(mFolders->getSortOrder());
				itemp = folderp;
			}
		}
		else // build new view for item
		{
			LLInventoryItem* item = (LLInventoryItem*)objectp;
			LLInvFVBridge* new_listener = LLInvFVBridge::createBridge(
				item->getType(),
				item->getInventoryType(),
				this,
				item->getUUID(),
				item->getFlags());
			if (new_listener)
			{
				LLFolderViewItem::Params params;
				params.name(new_listener->getDisplayName());
				params.icon(new_listener->getIcon());
				params.creation_date(new_listener->getCreationDate());
				params.root(mFolders);
				params.listener(new_listener);
				params.rect(LLRect (0, 0, 0, 0));
				itemp = LLUICtrlFactory::create<LLFolderViewItem> (params);
			}
		}

		LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)mFolders->getItemByID(objectp->getParentUUID());

		if (itemp)
		{
			if (parent_folder)
			{
				itemp->addToFolder(parent_folder, mFolders);
			}
			else
			{
				llwarns << "Couldn't find parent folder for child " << itemp->getLabel() << llendl;
				delete itemp;
			}
		}
	}
	if ((id.isNull() ||
		(objectp && objectp->getType() == LLAssetType::AT_CATEGORY)))
	{
		LLViewerInventoryCategory::cat_array_t* categories;
		LLViewerInventoryItem::item_array_t* items;

		mInventory->lockDirectDescendentArrays(id, categories, items);
		if(categories)
		{
			S32 count = categories->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryCategory* cat = categories->get(i);
				buildNewViews(cat->getUUID());
			}
		}
		if(items)
		{
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryItem* item = items->get(i);
				buildNewViews(item->getUUID());
			}
		}
		mInventory->unlockDirectDescendentArrays(id);
	}
}

struct LLConfirmPurgeData
{
	LLUUID mID;
	LLInventoryModel* mModel;
};

class LLIsNotWorn : public LLInventoryCollectFunctor
{
public:
	LLIsNotWorn() {}
	virtual ~LLIsNotWorn() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		return !gAgentWearables.isWearingItem(item->getUUID());
	}
};

class LLOpenFolderByID : public LLFolderViewFunctor
{
public:
	LLOpenFolderByID(const LLUUID& id) : mID(id) {}
	virtual ~LLOpenFolderByID() {}
	virtual void doFolder(LLFolderViewFolder* folder)
		{
			if (folder->getListener() && folder->getListener()->getUUID() == mID) folder->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
	virtual void doItem(LLFolderViewItem* item) {}
protected:
	const LLUUID& mID;
};


void LLInventoryPanel::openSelected()
{
	LLFolderViewItem* folder_item = mFolders->getCurSelectedItem();
	if(!folder_item) return;
	LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getListener();
	if(!bridge) return;
	bridge->openItem();
}

BOOL LLInventoryPanel::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x, y, mask);
	if(handled)
	{
		ECursorType cursor = getWindow()->getCursor();
		if (LLInventoryModel::backgroundFetchActive() && cursor == UI_CURSOR_ARROW)
		{
			// replace arrow cursor with arrow and hourglass cursor
			getWindow()->setCursor(UI_CURSOR_WORKING);
		}
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
	}
	return TRUE;
}

BOOL LLInventoryPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg)
{

	BOOL handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	if (handled)
	{
		mFolders->setDragAndDropThisFrame();
	}

	return handled;
}

void LLInventoryPanel::onFocusLost()
{
	// inventory no longer handles cut/copy/paste/delete
	if (LLEditMenuHandler::gEditMenuHandler == mFolders)
	{
		LLEditMenuHandler::gEditMenuHandler = NULL;
	}

	LLPanel::onFocusLost();
}

void LLInventoryPanel::onFocusReceived()
{
	// inventory now handles cut/copy/paste/delete
	LLEditMenuHandler::gEditMenuHandler = mFolders;

	LLPanel::onFocusReceived();
}


void LLInventoryPanel::openAllFolders()
{
	mFolders->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_DOWN);
	mFolders->arrangeAll();
}

void LLInventoryPanel::openDefaultFolderForType(LLAssetType::EType type)
{
	LLUUID category_id = mInventory->findCategoryUUIDForType(type);
	LLOpenFolderByID opener(category_id);
	mFolders->applyFunctorRecursively(opener);
}

void LLInventoryPanel::setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus)
{
	mFolders->setSelectionByID(obj_id, take_keyboard_focus);
}

void LLInventoryPanel::clearSelection()
{
	mFolders->clearSelection();
}

void LLInventoryPanel::onSelectionChange(const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	LLFolderView* fv = getRootFolder();
	if (fv->needsAutoRename()) // auto-selecting a new user-created asset and preparing to rename
	{
		fv->setNeedsAutoRename(FALSE);
		if (items.size()) // new asset is visible and selected
		{
			fv->startRenamingSelectedItem();
		}
	}
}

//----------------------------------------------------------------------------

void LLInventoryPanel::doToSelected(const LLSD& userdata)
{
	mFolders->doToSelected(&gInventory, userdata);
}

void LLInventoryPanel::doCreate(const LLSD& userdata)
{
	menu_create_inventory_item(mFolders, LLFolderBridge::sSelf, userdata);
}

bool LLInventoryPanel::beginIMSession()
{
	std::set<LLUUID> selected_items;
	mFolders->getSelectionList(selected_items);

	std::string name;
	static int session_num = 1;

	LLDynamicArray<LLUUID> members;
	EInstantMessage type = IM_SESSION_CONFERENCE_START;

	std::set<LLUUID>::const_iterator iter;
	for (iter = selected_items.begin(); iter != selected_items.end(); iter++)
	{

		LLUUID item = *iter;
		LLFolderViewItem* folder_item = mFolders->getItemByID(item);
			
		if(folder_item) 
		{
			LLFolderViewEventListener* fve_listener = folder_item->getListener();
			if (fve_listener && (fve_listener->getInventoryType() == LLInventoryType::IT_CATEGORY))
			{

				LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getListener();
				if(!bridge) return true;
				LLViewerInventoryCategory* cat = bridge->getCategory();
				if(!cat) return true;
				name = cat->getName();
				LLUniqueBuddyCollector is_buddy;
				LLInventoryModel::cat_array_t cat_array;
				LLInventoryModel::item_array_t item_array;
				gInventory.collectDescendentsIf(bridge->getUUID(),
												cat_array,
												item_array,
												LLInventoryModel::EXCLUDE_TRASH,
												is_buddy);
				S32 count = item_array.count();
				if(count > 0)
				{
					LLFloaterReg::showInstance("communicate");
					// create the session
					LLAvatarTracker& at = LLAvatarTracker::instance();
					LLUUID id;
					for(S32 i = 0; i < count; ++i)
					{
						id = item_array.get(i)->getCreatorUUID();
						if(at.isBuddyOnline(id))
						{
							members.put(id);
						}
					}
				}
			}
			else
			{
				LLFolderViewItem* folder_item = mFolders->getItemByID(item);
				if(!folder_item) return true;
				LLInvFVBridge* listenerp = (LLInvFVBridge*)folder_item->getListener();

				if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
				{
					LLInventoryItem* inv_item = gInventory.getItem(listenerp->getUUID());

					if (inv_item)
					{
						LLAvatarTracker& at = LLAvatarTracker::instance();
						LLUUID id = inv_item->getCreatorUUID();

						if(at.isBuddyOnline(id))
						{
							members.put(id);
						}
					}
				} //if IT_CALLINGCARD
			} //if !IT_CATEGORY
		}
	} //for selected_items	

	// the session_id is randomly generated UUID which will be replaced later
	// with a server side generated number

	if (name.empty())
	{
		name = llformat("Session %d", session_num++);
	}

	gIMMgr->addSession(name, type, members[0], members);
		
	return true;
}

bool LLInventoryPanel::attachObject(const LLSD& userdata)
{
	std::set<LLUUID> selected_items;
	mFolders->getSelectionList(selected_items);
	LLUUID id = *selected_items.begin();

	std::string joint_name = userdata.asString();
	LLVOAvatar *avatarp = static_cast<LLVOAvatar*>(gAgent.getAvatarObject());
	LLViewerJointAttachment* attachmentp = NULL;
	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getName() == joint_name)
		{
			attachmentp = attachment;
			break;
		}
	}
	if (attachmentp == NULL)
	{
		return true;
	}
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(id);

	if(item && gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID()))
	{
		rez_attachment(item, attachmentp);
	}
	else if(item && item->isComplete())
	{
		// must be in library. copy it to our inventory and put it on.
		LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(attachmentp);
		copy_inventory_item(gAgent.getID(),
							item->getPermissions().getOwner(),
							item->getUUID(),
							LLUUID::null,
							std::string(),
							cb);
	}
	gFocusMgr.setKeyboardFocus(NULL);

	return true;
}


//----------------------------------------------------------------------------

// static DEBUG ONLY:
void LLInventoryPanel::dumpSelectionInformation(void* user_data)
{
	LLInventoryPanel* iv = (LLInventoryPanel*)user_data;
	iv->mFolders->dumpSelectionInformation();
}

BOOL LLInventoryPanel::getSinceLogoff()
{
	return mFolders->getFilter()->isSinceLogoff();
}

void example_param_block_usage()
{
	LLInventoryPanel::Params param_block;
	param_block.name(std::string("inventory"));

	param_block.sort_order_setting(LLInventoryPanel::RECENTITEMS_SORT_ORDER);
	param_block.allow_multi_select(true);
	param_block.filter(LLInventoryPanel::Filter()
			.sort_order(1)
			.types(0xffff0000));
	param_block.inventory(&gInventory);
	param_block.has_border(true);

	LLUICtrlFactory::create<LLInventoryPanel>(param_block);

	param_block = LLInventoryPanel::Params();
	param_block.name(std::string("inventory"));

	//LLSD param_block_sd;
	//param_block_sd["sort_order_setting"] = LLInventoryPanel::RECENTITEMS_SORT_ORDER;
	//param_block_sd["allow_multi_select"] = true;
	//param_block_sd["filter"]["sort_order"] = 1;
	//param_block_sd["filter"]["types"] = (S32)0xffff0000;
	//param_block_sd["has_border"] = true;

	//LLInitParam::LLSDParser(param_block_sd).parse(param_block);

	LLUICtrlFactory::create<LLInventoryPanel>(param_block);
}
