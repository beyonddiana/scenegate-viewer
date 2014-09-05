/** 
* @file llinventoryfilter.h
* @brief Support for filtering your inventory to only display a subset of the
* available items.
*
* $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LLINVENTORYFILTER_H
#define LLINVENTORYFILTER_H

#include "llinventorytype.h"
#include "llpermissionsflags.h"
#include "llfolderviewmodel.h"

class LLFolderViewItem;
class LLFolderViewFolder;
class LLInventoryItem;

class LLInventoryFilter : public LLFolderViewFilter
{
public:
	enum EFolderShow
	{
		SHOW_ALL_FOLDERS,
		SHOW_NON_EMPTY_FOLDERS,
		SHOW_NO_FOLDERS
	};

	enum EFilterType	{
		FILTERTYPE_NONE = 0,
		FILTERTYPE_OBJECT = 0x1 << 0,	// normal default search-by-object-type
		FILTERTYPE_CATEGORY = 0x1 << 1,	// search by folder type
		FILTERTYPE_UUID	= 0x1 << 2,		// find the object with UUID and any links to it
		FILTERTYPE_DATE = 0x1 << 3,		// search by date range
		FILTERTYPE_WEARABLE = 0x1 << 4,	// search by wearable type
		FILTERTYPE_EMPTYFOLDERS = 0x1 << 5		// pass if folder is not a system   folder to be hidden if
	};

	enum EFilterLink
	{
		FILTERLINK_INCLUDE_LINKS,	// show links too
		FILTERLINK_EXCLUDE_LINKS,	// don't show links
		FILTERLINK_ONLY_LINKS		// only show links
	};

	enum ESortOrderType
	{
		SO_NAME = 0,						// Sort inventory by name
		SO_DATE = 0x1,						// Sort inventory by date
		SO_FOLDERS_BY_NAME = 0x1 << 1,		// Force folder sort by name
		SO_SYSTEM_FOLDERS_TO_TOP = 0x1 << 2	// Force system folders to be on top
	};

	struct FilterOps
	{
		struct DateRange : public LLInitParam::Block<DateRange>
		{
			Optional<time_t>	min_date,
								max_date;

			DateRange()
			:	min_date("min_date", time_min()),
				max_date("max_date", time_max())
			{}

			bool validateBlock(bool emit_errors = true) const;
		};

		struct Params : public LLInitParam::Block<Params>
		{
			Optional<U32>				types;
			Optional<U64>				object_types,
										wearable_types,
										category_types;
			Optional<EFilterLink>		links;
			Optional<LLUUID>			uuid;
			Optional<DateRange>			date_range;
			Optional<S32>				hours_ago;
			Optional<EFolderShow>		show_folder_state;
			Optional<PermissionMask>	permissions;

			Params()
			:	types("filter_types", FILTERTYPE_OBJECT),
				object_types("object_types", 0xffffFFFFffffFFFFULL),
				wearable_types("wearable_types", 0xffffFFFFffffFFFFULL),
				category_types("category_types", 0xffffFFFFffffFFFFULL),
				links("links", FILTERLINK_INCLUDE_LINKS),
				uuid("uuid"),
				date_range("date_range"),
				hours_ago("hours_ago", 0),
				show_folder_state("show_folder_state", SHOW_NON_EMPTY_FOLDERS),
				permissions("permissions", PERM_NONE)
			{}
		};

		FilterOps(const Params& = Params());

		U32 			mFilterTypes;
		U64				mFilterObjectTypes,   // For _OBJECT
						mFilterWearableTypes,
						mFilterLinks,
						mFilterCategoryTypes; // For _CATEGORY
		LLUUID      	mFilterUUID; 		  // for UUID

		time_t			mMinDate,
						mMaxDate;
		U32				mHoursAgo;

		EFolderShow		mShowFolderState;
		PermissionMask	mPermissions;
	};
							
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>		name;
		Optional<FilterOps::Params>	filter_ops;
		Optional<std::string>		substring;
		Optional<bool>				since_logoff;

		Params()
		:	name("name"),
			filter_ops(""),
			substring("substring"),
			since_logoff("since_logoff")
		{}
	};
									
	LLInventoryFilter(const Params& p = Params());
	LLInventoryFilter(const LLInventoryFilter& other) { *this = other; }
	virtual ~LLInventoryFilter() {}

	// +-------------------------------------------------------------------+
	// + Parameters
	// +-------------------------------------------------------------------+
	U64 				getFilterTypes() const;
	U64 				getFilterObjectTypes() const;
	U64					getFilterCategoryTypes() const;
	U64					getFilterWearableTypes() const;
	bool 				isFilterObjectTypesWith(LLInventoryType::EType t) const;
	void 				setFilterObjectTypes(U64 types);
	void 				setFilterCategoryTypes(U64 types);
	void 				setFilterUUID(const LLUUID &object_id);
	void				setFilterWearableTypes(U64 types);
	void				setFilterEmptySystemFolders();
	void				updateFilterTypes(U64 types, U64& current_types);

	void 				setFilterSubString(const std::string& string);
	const std::string& 	getFilterSubString(BOOL trim = FALSE) const;
	const std::string& 	getFilterSubStringOrig() const { return mFilterSubStringOrig; } 
	bool 				hasFilterString() const;

	void 				setFilterPermissions(PermissionMask perms);
	PermissionMask 		getFilterPermissions() const;

	void 				setDateRange(time_t min_date, time_t max_date);
	void 				setDateRangeLastLogoff(BOOL sl);
	time_t 				getMinDate() const;
	time_t 				getMaxDate() const;

	void 				setHoursAgo(U32 hours);
	U32 				getHoursAgo() const;

	void 				setFilterLinks(U64 filter_link);
	U64					getFilterLinks() const;

	// +-------------------------------------------------------------------+
	// + Execution And Results
	// +-------------------------------------------------------------------+
	bool				check(const LLFolderViewModelItem* listener);
	bool				check(const LLInventoryItem* item);
	bool				checkFolder(const LLFolderViewModelItem* listener) const;
	bool				checkFolder(const LLUUID& folder_id) const;

	bool				showAllResults() const;

	std::string::size_type getStringMatchOffset(LLFolderViewModelItem* item) const;
	std::string::size_type getFilterStringSize() const;
	// +-------------------------------------------------------------------+
	// + Presentation
	// +-------------------------------------------------------------------+
	void 				setShowFolderState( EFolderShow state);
	EFolderShow 		getShowFolderState() const;

	void 				setEmptyLookupMessage(const std::string& message);
	std::string			getEmptyLookupMessage() const;

	// +-------------------------------------------------------------------+
	// + Status
	// +-------------------------------------------------------------------+
	bool 				isActive() const;
	bool 				isModified() const;
	bool 				isSinceLogoff() const;
	void 				clearModified();
	const std::string& 	getName() const { return mName; }
	const std::string& 	getFilterText();
	//RN: this is public to allow system to externally force a global refilter
	void 				setModified(EFilterModified behavior = FILTER_RESTART);

	// +-------------------------------------------------------------------+
	// + Time
	// +-------------------------------------------------------------------+
	void 				resetTime(S32 timeout);
    bool                isTimedOut();
    
	// +-------------------------------------------------------------------+
	// + Default
	// +-------------------------------------------------------------------+
	bool 				isDefault() const;
	bool 				isNotDefault() const;
	void 				markDefault();
	void 				resetDefault();

	// +-------------------------------------------------------------------+
	// + Generation
	// +-------------------------------------------------------------------+
	S32 				getCurrentGeneration() const;
	S32 				getFirstSuccessGeneration() const;
	S32 				getFirstRequiredGeneration() const;


	// +-------------------------------------------------------------------+
	// + Conversion
	// +-------------------------------------------------------------------+
	void 				toParams(Params& params) const;
	void 				fromParams(const Params& p);

	LLInventoryFilter& operator =(const LLInventoryFilter& other);

private:
	bool				areDateLimitsSet();
	bool 				checkAgainstFilterType(const class LLFolderViewModelItemInventory* listener) const;
	bool 				checkAgainstFilterType(const LLInventoryItem* item) const;
	bool 				checkAgainstPermissions(const class LLFolderViewModelItemInventory* listener) const;
	bool 				checkAgainstPermissions(const LLInventoryItem* item) const;
	bool 				checkAgainstFilterLinks(const class LLFolderViewModelItemInventory* listener) const;
	bool				checkAgainstClipboard(const LLUUID& object_id) const;

	FilterOps				mFilterOps;
	FilterOps				mDefaultFilterOps;

	std::string				mFilterSubString;
	std::string				mFilterSubStringOrig;
	const std::string		mName;

	S32						mCurrentGeneration;
    // The following makes checking for pass/no pass possible even if the item is not checked against the current generation
    // Any item that *did not pass* the "required generation" will *not pass* the current one
    // Any item that *passes* the "success generation" will *pass* the current one
	S32						mFirstRequiredGeneration;
	S32						mFirstSuccessGeneration;

	EFilterModified 		mFilterModified;
	LLTimer                 mFilterTime;
    
	std::string 			mFilterText;
	std::string 			mEmptyLookupMessage;
};

#endif
