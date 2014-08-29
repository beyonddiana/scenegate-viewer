/*
 * @file llfloaterdirectory.h
 * @brief Legacy search facility definitions
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

#ifndef LL_FLOATERDIRECTORY_H
#define LL_FLOATERDIRECTORY_H

#include "llfloater.h"

class LLUICtrl;
class LLPanel;
class LLScrollListCtrl;
class LLTextBase;

static const size_t MIN_SEARCH_STRING_SIZE = 3;

typedef enum {
	SE_UNDEFINED = 0,
	SE_PEOPLE,
	SE_GROUPS,
	SE_PLACES,
	SE_LANDSALES,
	SE_EVENTS,
	SE_CLASSIFIEDS
} ESearch;

typedef struct dir_query
{
public:
	dir_query()
		: type(SE_UNDEFINED), text(LLStringUtil::null), scope(0),
		category_int(0), category_char(0x0), price(0), area(0), results_per_page(100) {}
	ESearch type;
	std::string text;
	U32 scope;
	U32 category_int;
	S8 category_char;
	S32 price;
	S32 area;
	U32 results_per_page;
} LLDirQuery;

class LLFloaterDirectory : public LLFloater
{
	friend class LLPanelSearchClassifieds;
	friend class LLPanelSearchEvents;
	friend class LLPanelSearchGroups;
	friend class LLPanelSearchLandSales;
	friend class LLPanelSearchPeople;
	friend class LLPanelSearchPlaces;
public:
	LLFloaterDirectory(const LLSD& key);
	BOOL postBuild();
	
	static void processSearchPeopleReply(LLMessageSystem* msg, void**);
	static void processSearchGroupsReply(LLMessageSystem* msg, void**);
	static void processSearchPlacesReply(LLMessageSystem* msg, void**);
	static void processSearchClassifiedsReply(LLMessageSystem* msg, void**);
	static void processSearchLandReply(LLMessageSystem* msg, void**);
	static void processSearchEventsReply(LLMessageSystem* msg, void**);
	
protected:
	void setProgress(bool working);
	void queryDirectory(const LLDirQuery& query, bool new_search = false);
	
private:
	~LLFloaterDirectory();
	void onCommitSelection();
	void choosePage(const LLSD& userdata);
	void paginate();
	void showDetailPanel(const std::string& panel_name);
	void rebuildResultList();

	ESearch mCurrentResultType;
	LLDirQuery mCurrentQuery;
	S32 mResultStart;
	S32 mNumResultsReceived;
	LLUUID mQueryID;
	
	LLPanel* mDetailPeople;
	LLScrollListCtrl* mResultList;
	LLTextBase* mResultsStatus;
};

#endif // LL_FLOATERDIRECTORY_H
