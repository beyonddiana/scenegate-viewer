/* 
 * @file llfolderviewmodelinventory.cpp
 * @brief Implementation of the inventory-specific view model
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
#include "llfolderviewmodelinventory.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "lltooldraganddrop.h"
#include "llfavoritesbar.h"

//
// class LLFolderViewModelInventory
//
static LLFastTimer::DeclareTimer FTM_INVENTORY_SORT("Sort");

bool LLFolderViewModelInventory::startDrag(std::vector<LLFolderViewModelItem*>& items)
{
	std::vector<EDragAndDropType> types;
	uuid_vec_t cargo_ids;
	std::vector<LLFolderViewModelItem*>::iterator item_it;
	bool can_drag = true;
	if (!items.empty())
	{
		for (item_it = items.begin(); item_it != items.end(); ++item_it)
		{
			EDragAndDropType type = DAD_NONE;
			LLUUID id = LLUUID::null;
			can_drag = can_drag && static_cast<LLFolderViewModelItemInventory*>(*item_it)->startDrag(&type, &id);

			types.push_back(type);
			cargo_ids.push_back(id);
		}

		LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, 
			static_cast<LLFolderViewModelItemInventory*>(items.front())->getDragSource(), mTaskID); 
	}
	return can_drag;
}


void LLFolderViewModelInventory::sort( LLFolderViewFolder* folder )
{
	LLFastTimer _(FTM_INVENTORY_SORT);

	if (!needsSort(folder->getViewModelItem())) return;

	LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem());
	if (modelp->getUUID().isNull()) return;

	for (std::list<LLFolderViewFolder*>::iterator it =   folder->getFoldersBegin(), end_it = folder->getFoldersEnd();
		it != end_it;
		++it)
	{
		LLFolderViewFolder* child_folderp = *it;
		sort(child_folderp);

		if (child_folderp->getFoldersCount() > 0)
		{
			time_t most_recent_folder_time =
				static_cast<LLFolderViewModelItemInventory*>((*child_folderp->getFoldersBegin())->getViewModelItem())->getCreationDate();
			LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(child_folderp->getViewModelItem());
			if (most_recent_folder_time > modelp->getCreationDate())
			{
				modelp->setCreationDate(most_recent_folder_time);
			}
		}
		if (child_folderp->getItemsCount() > 0)			
		{
			time_t most_recent_item_time =
				static_cast<LLFolderViewModelItemInventory*>((*child_folderp->getItemsBegin())->getViewModelItem())->getCreationDate();

			LLFolderViewModelItemInventory* modelp =   static_cast<LLFolderViewModelItemInventory*>(child_folderp->getViewModelItem());
			if (most_recent_item_time > modelp->getCreationDate())
			{
				modelp->setCreationDate(most_recent_item_time);
			}
		}
	}
	base_t::sort(folder);
}

bool LLFolderViewModelInventory::contentsReady()
{
	return !LLInventoryModelBackgroundFetch::instance().folderFetchActive();
}

void LLFolderViewModelItemInventory::requestSort()
{
	LLFolderViewModelItemCommon::requestSort();
	LLFolderViewFolder* folderp = dynamic_cast<LLFolderViewFolder*>(mFolderViewItem);
	if (folderp)
	{
		folderp->requestArrange();
	}
	if (static_cast<LLFolderViewModelInventory&>(mRootViewModel).getSorter().isByDate())
	{
		// sort by date potentially affects parent folders which use a date
		// derived from newest item in them
		if (mParent)
		{
			mParent->requestSort();
		}
	}
}

void LLFolderViewModelItemInventory::setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset, std::string::size_type string_size)
{
	LLFolderViewModelItemCommon::setPassedFilter(passed, filter_generation, string_offset, string_size);

	bool passed_filter_before = mPrevPassedAllFilters;
	mPrevPassedAllFilters = passedFilter(filter_generation);

	if (passed_filter_before != mPrevPassedAllFilters)
	{
		//TODO RN: ensure this still happens, but without dependency on folderview
		LLFolderViewFolder* parent_folder = mFolderViewItem->getParentFolder();
		if (parent_folder)
		{
			parent_folder->requestArrange();
		}
	}
}

bool LLFolderViewModelItemInventory::filterChildItem( LLFolderViewModelItem* item, LLFolderViewFilter& filter )
{
	S32 filter_generation = filter.getCurrentGeneration();

	bool continue_filtering = true;
	if (item->getLastFilterGeneration() < filter_generation)
	{
		// recursive application of the filter for child items
		continue_filtering = item->filter( filter );
	}

	// track latest generation to pass any child items, for each folder up to root
	if (item->passedFilter())
	{
		LLFolderViewModelItemInventory* view_model = this;
		
		while(view_model && view_model->mMostFilteredDescendantGeneration < filter_generation)
		{
			view_model->mMostFilteredDescendantGeneration = filter_generation;
			view_model = static_cast<LLFolderViewModelItemInventory*>(view_model->mParent);
		}
	}

	return continue_filtering;
}

bool LLFolderViewModelItemInventory::filter( LLFolderViewFilter& filter)
{
	const S32 filter_generation = filter.getCurrentGeneration();
	const S32 must_pass_generation = filter.getFirstRequiredGeneration();

	if (getLastFilterGeneration() >= must_pass_generation 
		&& getLastFolderFilterGeneration() >= must_pass_generation
		&& !passedFilter(must_pass_generation))
	{
		// failed to pass an earlier filter that was a subset of the current one
		// go ahead and flag this item as done
		setPassedFilter(false, filter_generation);
		setPassedFolderFilter(false, filter_generation);
		return true;
	}

	const bool passed_filter_folder = (getInventoryType() == LLInventoryType::IT_CATEGORY) 
		? filter.checkFolder(this)
		: true;
	setPassedFolderFilter(passed_filter_folder, filter_generation);

	if(!mChildren.empty()
		&& (getLastFilterGeneration() < must_pass_generation // haven't checked descendants against minimum required generation to pass
			|| descendantsPassedFilter(must_pass_generation))) // or at least one descendant has passed the minimum requirement
	{
		// now query children
		for (child_list_t::iterator iter = mChildren.begin(), end_iter = mChildren.end();
			iter != end_iter && filter.getFilterCount() > 0;
			++iter)
		{
			if (!filterChildItem((*iter), filter))
			{
				break;
			}
		}
	}

	// if we didn't use all filter iterations
	// that means we filtered all of our descendants
	// so filter ourselves now
	if (filter.getFilterCount() > 0)
	{
		filter.decrementFilterCount();

		const bool passed_filter = filter.check(this);
		setPassedFilter(passed_filter, filter_generation, filter.getStringMatchOffset(this), filter.getFilterStringSize());
		return true;
	}
	else
	{
		return false;
	}
}

LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel()
{
	return &mInventoryViewModel;
}


const LLFolderViewModelInventory* LLInventoryPanel::getFolderViewModel() const
{
	return &mInventoryViewModel;
}

bool LLInventorySort::operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b) const
{
	// Ignore sort order for landmarks in the Favorites folder.
	// In that folder, landmarks should be always sorted as in the Favorites bar. See EXT-719
	if (a->getSortGroup() == SG_ITEM
		&& b->getSortGroup() == SG_ITEM
		&& a->getInventoryType() == LLInventoryType::IT_LANDMARK
		&& b->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		static const LLUUID& favorites_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
		// If both landmarks are in the favorite folder...
		if (gInventory.isObjectDescendentOf(a->getUUID(), favorites_folder_id) && gInventory.isObjectDescendentOf(b->getUUID(), favorites_folder_id))
		{
			// Get their index in that folder
			S32 a_sort = LLFavoritesOrderStorage::instance().getSortIndex(a->getUUID());
			S32 b_sort = LLFavoritesOrderStorage::instance().getSortIndex(b->getUUID());
			// Note: since there are both in the favorite, we shouldn't get negative index value...
			if (!((a_sort < 0) && (b_sort < 0)))
			{
				return a_sort < b_sort;
			}
		}
	}

	// We sort by name if we aren't sorting by date
	// OR if these are folders and we are sorting folders by name.
	bool by_name = (!mByDate || (mFoldersByName && (a->getSortGroup() != SG_ITEM)));

	if (a->getSortGroup() != b->getSortGroup())
	{
		if (mSystemToTop)
		{
			// Group order is System Folders, Trash, Normal Folders, Items
			return (a->getSortGroup() < b->getSortGroup());
		}
		else if (mByDate)
		{
			// Trash needs to go to the bottom if we are sorting by date
			if ( (a->getSortGroup() == SG_TRASH_FOLDER)
				|| (b->getSortGroup() == SG_TRASH_FOLDER))
			{
				return (b->getSortGroup() == SG_TRASH_FOLDER);
			}
		}
	}

	if (by_name)
	{
		S32 compare = LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName());
		if (0 == compare)
		{
			return (a->getCreationDate() > b->getCreationDate());
		}
		else
		{
			return (compare < 0);
		}
	}
	else
	{
		time_t first_create = a->getCreationDate();
		time_t second_create = b->getCreationDate();
		if (first_create == second_create)
		{
			return (LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName()) < 0);
		}
		else
		{
			return (first_create > second_create);
		}
	}
}

LLFolderViewModelItemInventory::LLFolderViewModelItemInventory( class LLFolderViewModelInventory& root_view_model ) 
	:	LLFolderViewModelItemCommon(root_view_model),
	mPrevPassedAllFilters(false)
{
}
