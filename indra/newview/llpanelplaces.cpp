/**
 * @file llpanelplaces.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpanelplaces.h"

#include "llassettype.h"
#include "llwindow.h"

#include "llinventory.h"
#include "lllandmark.h"
#include "llparcel.h"

#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llnotifications.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llavatarpropertiesprocessor.h"
#include "llfloaterworldmap.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "llpanelplaceinfo.h"
#include "llpanellandmarks.h"
#include "llpanelpick.h"
#include "llpanelteleporthistory.h"
#include "llteleporthistorystorage.h"
#include "lltoggleablemenu.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

static const S32 LANDMARK_FOLDERS_MENU_WIDTH = 250;
static const std::string AGENT_INFO_TYPE			= "agent";
static const std::string CREATE_LANDMARK_INFO_TYPE	= "create_landmark";
static const std::string LANDMARK_INFO_TYPE			= "landmark";
static const std::string REMOTE_PLACE_INFO_TYPE		= "remote_place";
static const std::string TELEPORT_HISTORY_INFO_TYPE	= "teleport_history";

// Helper functions
static bool is_agent_in_selected_parcel(LLParcel* parcel);
static void onSLURLBuilt(std::string& slurl);
static void setAllChildrenVisible(LLView* view, BOOL visible);

//Observer classes
class LLPlacesParcelObserver : public LLParcelObserver
{
public:
	LLPlacesParcelObserver(LLPanelPlaces* places_panel)
	: mPlaces(places_panel) {}

	/*virtual*/ void changed()
	{
		if (mPlaces)
			mPlaces->changedParcelSelection();
	}

private:
	LLPanelPlaces*		mPlaces;
};

class LLPlacesInventoryObserver : public LLInventoryObserver
{
public:
	LLPlacesInventoryObserver(LLPanelPlaces* places_panel)
	: mPlaces(places_panel) {}

	/*virtual*/ void changed(U32 mask)
	{
		if (mPlaces)
			mPlaces->changedInventory(mask);
	}

private:
	LLPanelPlaces*		mPlaces;
};

static LLRegisterPanelClassWrapper<LLPanelPlaces> t_places("panel_places");

LLPanelPlaces::LLPanelPlaces()
	:	LLPanel(),
		mFilterSubString(LLStringUtil::null),
		mActivePanel(NULL),
		mFilterEditor(NULL),
		mPlaceInfo(NULL),
		mPickPanel(NULL),
		mItem(NULL),
		mPlaceMenu(NULL),
		mLandmarkMenu(NULL),
		mPosGlobal(),
		isLandmarkEditModeOn(false)
{
	mParcelObserver = new LLPlacesParcelObserver(this);
	mInventoryObserver = new LLPlacesInventoryObserver(this);

	gInventory.addObserver(mInventoryObserver);

	LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(
			boost::bind(&LLPanelPlaces::onAgentParcelChange, this));

	//LLUICtrlFactory::getInstance()->buildPanel(this, "panel_places.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLPanelPlaces::~LLPanelPlaces()
{
	if (gInventory.containsObserver(mInventoryObserver))
		gInventory.removeObserver(mInventoryObserver);

	LLViewerParcelMgr::getInstance()->removeObserver(mParcelObserver);

	delete mInventoryObserver;
	delete mParcelObserver;
}

BOOL LLPanelPlaces::postBuild()
{
	mTeleportBtn = getChild<LLButton>("teleport_btn");
	mTeleportBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onTeleportButtonClicked, this));
	
	mShowOnMapBtn = getChild<LLButton>("map_btn");
	mShowOnMapBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShowOnMapButtonClicked, this));
	
	mShareBtn = getChild<LLButton>("share_btn");
	//mShareBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShareButtonClicked, this));

	mEditBtn = getChild<LLButton>("edit_btn");
	mEditBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));

	mSaveBtn = getChild<LLButton>("save_btn");
	mSaveBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onSaveButtonClicked, this));

	mCancelBtn = getChild<LLButton>("cancel_btn");
	mCancelBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onCancelButtonClicked, this));

	mCloseBtn = getChild<LLButton>("close_btn");
	mCloseBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));

	mOverflowBtn = getChild<LLButton>("overflow_btn");
	mOverflowBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onOverflowButtonClicked, this));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Places.OverflowMenu.Action",  boost::bind(&LLPanelPlaces::onOverflowMenuItemClicked, this, _2));
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	enable_registrar.add("Places.OverflowMenu.Enable",  boost::bind(&LLPanelPlaces::onOverflowMenuItemEnable, this, _2));

	mPlaceMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_place.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mPlaceMenu)
	{
		llwarns << "Error loading Place menu" << llendl;
	}

	mLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mLandmarkMenu)
	{
		llwarns << "Error loading Landmark menu" << llendl;
	}

	mTabContainer = getChild<LLTabContainer>("Places Tabs");
	if (mTabContainer)
	{
		mTabContainer->setCommitCallback(boost::bind(&LLPanelPlaces::onTabSelected, this));
	}

	mFilterEditor = getChild<LLFilterEditor>("Filter");
	if (mFilterEditor)
	{
		mFilterEditor->setCommitCallback(boost::bind(&LLPanelPlaces::onFilterEdit, this, _2, false));
	}

	mPlaceInfo = getChild<LLPanelPlaceInfo>("panel_place_info");

	LLButton* back_btn = mPlaceInfo->getChild<LLButton>("back_btn");
	back_btn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));

	LLLineEditor* title_editor = mPlaceInfo->getChild<LLLineEditor>("title_editor");
	title_editor->setKeystrokeCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this), NULL);

	LLTextEditor* notes_editor = mPlaceInfo->getChild<LLTextEditor>("notes_editor");
	notes_editor->setKeystrokeCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));

	LLComboBox* folder_combo = mPlaceInfo->getChild<LLComboBox>("folder_combo");
	folder_combo->setSelectionCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));
	return TRUE;
}

void LLPanelPlaces::onOpen(const LLSD& key)
{
	if(mPlaceInfo == NULL || key.size() == 0)
		return;

	mFilterEditor->clear();
	onFilterEdit("", false);

	mPlaceInfoType = key["type"].asString();
	mPosGlobal.setZero();
	mItem = NULL;
	isLandmarkEditModeOn = false;
	togglePlaceInfoPanel(TRUE);
	updateVerbs();

	if (mPlaceInfoType == AGENT_INFO_TYPE)
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::AGENT);
	}
	else if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::CREATE_LANDMARK);

		if (key.has("x") && key.has("y") && key.has("z"))
		{
			mPosGlobal = LLVector3d(key["x"].asReal(),
									key["y"].asReal(),
									key["z"].asReal());
		}
		else
		{
			mPosGlobal = gAgent.getPositionGlobal();
		}

		mPlaceInfo->displayParcelInfo(LLUUID(), mPosGlobal);
	}
	else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::LANDMARK);

		LLInventoryItem* item = gInventory.getItem(key["id"].asUUID());
		if (!item)
			return;

		setItem(item);
	}
	else if (mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
	{
		if (mPlaceInfo->isMediaPanelVisible())
		{
			toggleMediaPanel();
		}

		mPosGlobal = LLVector3d(key["x"].asReal(),
								key["y"].asReal(),
								key["z"].asReal());

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayParcelInfo(LLUUID(), mPosGlobal);
	}
	else if (mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
	{
		S32 index = key["id"].asInteger();

		const LLTeleportHistoryStorage::slurl_list_t& hist_items =
					LLTeleportHistoryStorage::getInstance()->getItems();

		mPosGlobal = hist_items[index].mGlobalPos;

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::TELEPORT_HISTORY);
		mPlaceInfo->updateLastVisitedText(hist_items[index].mDate);
		mPlaceInfo->displayParcelInfo(LLUUID(), mPosGlobal);
	}

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
	if (!parcel_mgr)
		return;

	// Start using LLViewerParcelMgr for land selection if
	// information about nearby land is requested.
	// Otherwise stop using land selection and deselect land.
	if (mPlaceInfoType == AGENT_INFO_TYPE)
	{
		parcel_mgr->addObserver(mParcelObserver);
		parcel_mgr->selectParcelAt(gAgent.getPositionGlobal());
	}
	else
	{
		parcel_mgr->removeObserver(mParcelObserver);

		if (!parcel_mgr->selectionEmpty())
		{
			parcel_mgr->deselectLand();
		}
	}
}

void LLPanelPlaces::setItem(LLInventoryItem* item)
{
	if (!mPlaceInfo || !item)
		return;

	mItem = item;

	LLAssetType::EType item_type = mItem->getActualType();
	if (item_type == LLAssetType::AT_LANDMARK || item_type == LLAssetType::AT_LINK)
	{
		// If the item is a link get a linked item
		if (item_type == LLAssetType::AT_LINK)
		{
			mItem = gInventory.getItem(mItem->getLinkedUUID());
			if (mItem.isNull())
				return;
		}
	}
	else
	{
		return;
	}

	// Check if item is in agent's inventory and he has the permission to modify it.
	BOOL is_landmark_editable = gInventory.isObjectDescendentOf(mItem->getUUID(), gInventory.getRootFolderID()) &&
								mItem->getPermissions().allowModifyBy(gAgent.getID());

	mEditBtn->setEnabled(is_landmark_editable);
	mSaveBtn->setEnabled(is_landmark_editable);

	if (is_landmark_editable)
	{
		if(!mPlaceInfo->setLandmarkFolder(mItem->getParentUUID()) && !mItem->getParentUUID().isNull())
		{
			const LLViewerInventoryCategory* cat = gInventory.getCategory(mItem->getParentUUID());
			if(cat)
			{
				std::string cat_fullname = LLPanelPlaceInfo::getFullFolderName(cat);
				LLComboBox* folderList = mPlaceInfo->getChild<LLComboBox>("folder_combo");
				folderList->add(cat_fullname, cat->getUUID(),ADD_TOP);
			}
		}
	}

	mPlaceInfo->displayItemInfo(mItem);

	LLLandmark* lm = gLandmarkList.getAsset(mItem->getAssetUUID(),
											boost::bind(&LLPanelPlaces::onLandmarkLoaded, this, _1));
	if (lm)
	{
		onLandmarkLoaded(lm);
	}
}

void LLPanelPlaces::onLandmarkLoaded(LLLandmark* landmark)
{
	if (!mPlaceInfo)
		return;

	LLUUID region_id;
	landmark->getRegionID(region_id);
	landmark->getGlobalPos(mPosGlobal);
	mPlaceInfo->displayParcelInfo(region_id, mPosGlobal);
}

void LLPanelPlaces::onFilterEdit(const std::string& search_string, bool force_filter)
{
	if (force_filter || mFilterSubString != search_string)
	{
		mFilterSubString = search_string;

		// Searches are case-insensitive
		LLStringUtil::toUpper(mFilterSubString);
		LLStringUtil::trimHead(mFilterSubString);

		if (mActivePanel)
			mActivePanel->onSearchEdit(mFilterSubString);
	}
}

void LLPanelPlaces::onTabSelected()
{
	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());
	if (!mActivePanel)
		return;

	onFilterEdit(mFilterSubString, true);
	mActivePanel->updateVerbs();
}

/*
void LLPanelPlaces::onShareButtonClicked()
{
	// TODO: Launch the "Things" Share wizard
}
*/

void LLPanelPlaces::onTeleportButtonClicked()
{
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->getVisible())
	{
		if (mPlaceInfoType == LANDMARK_INFO_TYPE)
		{
			LLSD payload;
			payload["asset_id"] = mItem->getAssetUUID();
			LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
		}
		else if (mPlaceInfoType == AGENT_INFO_TYPE ||
				 mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
				 mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
		{
			LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
			if (!mPosGlobal.isExactlyZero() && worldmap_instance)
			{
				gAgent.teleportViaLocation(mPosGlobal);
				worldmap_instance->trackLocation(mPosGlobal);
			}
		}
	}
	else
	{
		if (mActivePanel)
			mActivePanel->onTeleport();
	}
}

void LLPanelPlaces::onShowOnMapButtonClicked()
{
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->getVisible())
	{
		LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
		if(!worldmap_instance)
			return;

		if (mPlaceInfoType == AGENT_INFO_TYPE ||
			mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE ||
			mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
			mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
		{
			if (!mPosGlobal.isExactlyZero())
			{
				worldmap_instance->trackLocation(mPosGlobal);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
		{
			LLLandmark* landmark = gLandmarkList.getAsset(mItem->getAssetUUID());
			if (!landmark)
				return;

			LLVector3d landmark_global_pos;
			if (!landmark->getGlobalPos(landmark_global_pos))
				return;
			
			if (!landmark_global_pos.isExactlyZero())
			{
				worldmap_instance->trackLocation(landmark_global_pos);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
	}
	else
	{
		if (mActivePanel)
			mActivePanel->onShowOnMap();
	}
}

void LLPanelPlaces::onEditButtonClicked()
{
	if (!mPlaceInfo || isLandmarkEditModeOn)
		return;

	isLandmarkEditModeOn = true;

	mPlaceInfo->toggleLandmarkEditMode(TRUE);

	updateVerbs();
}

void LLPanelPlaces::onSaveButtonClicked()
{
	if (!mPlaceInfo || mItem.isNull())
		return;

	std::string current_title_value = mPlaceInfo->getLandmarkTitle();
	std::string item_title_value = mItem->getName();
	std::string current_notes_value = mPlaceInfo->getLandmarkNotes();
	std::string item_notes_value = mItem->getDescription();

	LLStringUtil::trim(current_title_value);
	LLStringUtil::trim(current_notes_value);

	LLUUID item_id = mItem->getUUID();
	LLUUID folder_id = mPlaceInfo->getLandmarkFolder();

	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(mItem);

	if (!current_title_value.empty() &&
		(item_title_value != current_title_value || item_notes_value != current_notes_value))
	{
		new_item->rename(current_title_value);
		new_item->setDescription(current_notes_value);
		new_item->updateServer(FALSE);
	}

	if(folder_id != mItem->getParentUUID())
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(mItem->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(folder_id, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		new_item->setParent(folder_id);
		new_item->updateParentOnServer(FALSE);
	}

	gInventory.updateItem(new_item);
	gInventory.notifyObservers();

	onCancelButtonClicked();
}

void LLPanelPlaces::onCancelButtonClicked()
{
	if (!mPlaceInfo)
		return;

	if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
	{
		onBackButtonClicked();
	}
	else
	{
		mPlaceInfo->toggleLandmarkEditMode(FALSE);
		isLandmarkEditModeOn = false;

		updateVerbs();

		// Reload the landmark properties.
		mPlaceInfo->displayItemInfo(mItem);
	}
}

void LLPanelPlaces::onOverflowButtonClicked()
{
	LLToggleableMenu* menu;

	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;

	if ((is_agent_place_info_visible ||
		 mPlaceInfoType == "remote_place" ||
		 mPlaceInfoType == "teleport_history") && mPlaceMenu != NULL)
	{
		menu = mPlaceMenu;

		// Enable adding a landmark only for agent current parcel and if
		// there is no landmark already pointing to that parcel in agent's inventory.
		menu->getChild<LLMenuItemCallGL>("landmark")->setEnabled(is_agent_place_info_visible &&
																 !LLLandmarkActions::landmarkAlreadyExists());
	}
	else if (mPlaceInfoType == LANDMARK_INFO_TYPE && mLandmarkMenu != NULL)
	{
		menu = mLandmarkMenu;

		BOOL is_landmark_removable = FALSE;
		if (mItem.notNull())
		{
			const LLUUID& item_id = mItem->getUUID();
			const LLUUID& trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
			is_landmark_removable = gInventory.isObjectDescendentOf(item_id, gInventory.getRootFolderID()) &&
									!gInventory.isObjectDescendentOf(item_id, trash_id);
		}

		menu->getChild<LLMenuItemCallGL>("delete")->setEnabled(is_landmark_removable);
	}
	else
	{
		return;
	}

	if (!menu->toggleVisibility())
		return;

	menu->updateParent(LLMenuGL::sMenuContainer);
	LLRect rect = mOverflowBtn->getRect();
	menu->setButtonRect(rect, this);
	LLMenuGL::showPopup(this, menu, rect.mRight, rect.mTop);
}

bool LLPanelPlaces::onOverflowMenuItemEnable(const LLSD& param)
{
	std::string value = param.asString();
	if("can_create_pick" == value)
	{
		return !LLAgentPicksInfo::getInstance()->isPickLimitReached();
	}
	return true;
}

void LLPanelPlaces::onOverflowMenuItemClicked(const LLSD& param)
{
	std::string item = param.asString();
	if (item == "landmark")
	{
		LLSD key;
		key["type"] = CREATE_LANDMARK_INFO_TYPE;
		key["x"] = mPosGlobal.mdV[VX];
		key["y"] = mPosGlobal.mdV[VY];
		key["z"] = mPosGlobal.mdV[VZ];
		onOpen(key);
	}
	else if (item == "copy")
	{
		LLLandmarkActions::getSLURLfromPosGlobal(mPosGlobal, boost::bind(&onSLURLBuilt, _1));
	}
	else if (item == "delete")
	{
		gInventory.removeItem(mItem->getUUID());

		onBackButtonClicked();
	}
	else if (item == "pick")
	{
		if (!mPlaceInfo)
			return;

		if (mPickPanel == NULL)
		{
			mPickPanel = LLPanelPickEdit::create();
			addChild(mPickPanel);

			mPickPanel->setExitCallback(boost::bind(&LLPanelPlaces::togglePickPanel, this, FALSE));
			mPickPanel->setCancelCallback(boost::bind(&LLPanelPlaces::togglePickPanel, this, FALSE));
			mPickPanel->setSaveCallback(boost::bind(&LLPanelPlaces::togglePickPanel, this, FALSE));
		}

		togglePickPanel(TRUE);
		mPickPanel->onOpen(LLSD());
		mPlaceInfo->createPick(mPosGlobal, mPickPanel);

		LLRect rect = getRect();
		mPickPanel->reshape(rect.getWidth(), rect.getHeight());
		mPickPanel->setRect(rect);
	}
    else if (item == "add_to_favbar")
    {
        if ( mItem.notNull() ) 
        {
            LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_FAVORITE);
            if ( favorites_id.notNull() )
            {
                copy_inventory_item(gAgent.getID(),
                                    mItem->getPermissions().getOwner(),
                                    mItem->getUUID(),
                                    favorites_id,
                                    std::string(),
                                    LLPointer<LLInventoryCallback>(NULL));
                llinfos << "Copied inventory item #" << mItem->getUUID() << " to favorites." << llendl;
            }
        }
    }
}

void LLPanelPlaces::onBackButtonClicked()
{
	if (!mPlaceInfo)
		return;
	
	if (mPlaceInfo->isMediaPanelVisible())
	{
		toggleMediaPanel();
	}
	else
	{
		togglePlaceInfoPanel(FALSE);

		// Resetting mPlaceInfoType when Place Info panel is closed.
		mPlaceInfoType = LLStringUtil::null;

		isLandmarkEditModeOn = false;
	}

	updateVerbs();
}

void LLPanelPlaces::toggleMediaPanel()
{
	if (!mPlaceInfo)
		return;

	mPlaceInfo->toggleMediaPanel(!mPlaceInfo->isMediaPanelVisible());

	// Refresh the current place info because
	// the media panel controls can't refer to
	// the remote parcel media.
	onOpen(LLSD().insert("type", AGENT_INFO_TYPE));
}

void LLPanelPlaces::togglePickPanel(BOOL visible)
{
	setAllChildrenVisible(this, !visible);

	if (mPickPanel)
		mPickPanel->setVisible(visible);
}

void LLPanelPlaces::togglePlaceInfoPanel(BOOL visible)
{
	if (!mPlaceInfo)
		return;

	mPlaceInfo->setVisible(visible);
	mFilterEditor->setVisible(!visible);
	mTabContainer->setVisible(!visible);

	if (visible)
	{
		mPlaceInfo->resetLocation();

		LLRect rect = getRect();
		LLRect new_rect = LLRect(rect.mLeft, rect.mTop, rect.mRight, mTabContainer->getRect().mBottom);
		mPlaceInfo->reshape(new_rect.getWidth(),new_rect.getHeight());
	}
}

void LLPanelPlaces::changedParcelSelection()
{
	if (!mPlaceInfo)
		return;

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
	mParcel = parcel_mgr->getFloatingParcelSelection();
	LLParcel* parcel = mParcel->getParcel();
	LLViewerRegion* region = parcel_mgr->getSelectionRegion();
	if (!region || !parcel)
		return;

	// If agent is inside the selected parcel show agent's region<X, Y, Z>,
	// otherwise show region<X, Y, Z> of agent's selection point.
	bool is_current_parcel = is_agent_in_selected_parcel(parcel);
	if (is_current_parcel)
	{
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else
	{
		LLVector3d pos_global = gViewerWindow->getLastPick().mPosGlobal;
		if (!pos_global.isExactlyZero())
		{
			mPosGlobal = pos_global;
		}
	}

	mPlaceInfo->resetLocation();
	mPlaceInfo->displaySelectedParcelInfo(parcel, region, mPosGlobal, is_current_parcel);

	updateVerbs();
}

void LLPanelPlaces::changedInventory(U32 mask)
{
	if (!(gInventory.isInventoryUsable() && LLTeleportHistory::getInstance()))
		return;

	LLLandmarksPanel* landmarks_panel = new LLLandmarksPanel();
	if (landmarks_panel)
	{
		landmarks_panel->setPanelPlacesButtons(this);

		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(landmarks_panel).
			label(getString("landmarks_tab_title")).
			insert_at(LLTabContainer::END));
	}

	LLTeleportHistoryPanel* teleport_history_panel = new LLTeleportHistoryPanel();
	if (teleport_history_panel)
	{
		teleport_history_panel->setPanelPlacesButtons(this);

		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(teleport_history_panel).
			label(getString("teleport_history_tab_title")).
			insert_at(LLTabContainer::END));
	}

	mTabContainer->selectFirstTab();

	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());

	// Filter applied to show all items.
	if (mActivePanel)
		mActivePanel->onSearchEdit(mFilterSubString);

	// we don't need to monitor inventory changes anymore,
	// so remove the observer
	gInventory.removeObserver(mInventoryObserver);
}

void LLPanelPlaces::onAgentParcelChange()
{
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->isMediaPanelVisible())
	{
		onOpen(LLSD().insert("type", AGENT_INFO_TYPE));
	}
	else
	{
		updateVerbs();
	}
}

void LLPanelPlaces::updateVerbs()
{
	if (!mPlaceInfo)
		return;

	bool is_place_info_visible = mPlaceInfo->getVisible();
	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;
	bool is_create_landmark_visible = mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE;
	bool is_media_panel_visible = mPlaceInfo->isMediaPanelVisible();

	mTeleportBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mShowOnMapBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mShareBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mOverflowBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mEditBtn->setVisible(mPlaceInfoType == LANDMARK_INFO_TYPE && !isLandmarkEditModeOn);
	mSaveBtn->setVisible(isLandmarkEditModeOn);
	mCancelBtn->setVisible(isLandmarkEditModeOn);
	mCloseBtn->setVisible(is_create_landmark_visible && !isLandmarkEditModeOn);

	mOverflowBtn->setEnabled(is_place_info_visible && !is_media_panel_visible && !is_create_landmark_visible);

	if (is_place_info_visible)
	{
		if (is_agent_place_info_visible)
		{
			// We don't need to teleport to the current location
			// so check if the location is not within the current parcel.
			mTeleportBtn->setEnabled(!is_media_panel_visible &&
									 !mPosGlobal.isExactlyZero() &&
									 !LLViewerParcelMgr::getInstance()->inAgentParcel(mPosGlobal));
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE || mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
		{
			mTeleportBtn->setEnabled(TRUE);
		}

		mShowOnMapBtn->setEnabled(!is_media_panel_visible);
	}
	else
	{
		if (mActivePanel)
			mActivePanel->updateVerbs();
	}
}

static bool is_agent_in_selected_parcel(LLParcel* parcel)
{
	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();

	LLViewerRegion* region = parcel_mgr->getSelectionRegion();
	if (!region || !parcel)
		return false;

	return	region == gAgent.getRegion() &&
			parcel->getLocalID() == parcel_mgr->getAgentParcel()->getLocalID();
}

static void onSLURLBuilt(std::string& slurl)
{
	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
		
	LLSD args;
	args["SLURL"] = slurl;

	LLNotifications::instance().add("CopySLURL", args);
}

static void setAllChildrenVisible(LLView* view, BOOL visible)
{
	const LLView::child_list_t* children = view->getChildList();
	for (LLView::child_list_const_iter_t child_it = children->begin(); child_it != children->end(); ++child_it)
	{
		LLView* child = *child_it;
		if (child->getParent() == view)
		{
			child->setVisible(visible);
		}
	}
}
