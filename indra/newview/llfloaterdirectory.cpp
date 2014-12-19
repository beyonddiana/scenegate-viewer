/*
 * @file llfloaterdirectory.cpp
 * @brief Legacy search facility
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterdirectory.h"

// llmessage
#include "lleventflags.h"
#include "llqueryflags.h"
#include "message.h"

// llui
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llpanel.h"
#include "llresmgr.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "lltextbase.h"
#include "lltrans.h"

// newview
#include "llagent.h"
#include "llpanelsearchbase.h"
#include "llpanelsearchweb.h"
#include "llpanelplaces.h"
#include "llproductinforequest.h"

SearchQuery::SearchQuery()
:	category("category", ""), query("query")
{}

static const std::array<std::string, 6> sSearchPanels{ {"panel_search_people", "panel_search_groups", "panel_search_places", "panel_search_classifieds", "panel_search_events", "panel_search_landsales"} };
static const std::array<std::string, 5> sDetailPanels{ {"detail_avatar", "detail_group", "detail_place", "detail_classified", "detail_event"} };

LLFloaterDirectory::LLFloaterDirectory(const Params& key)
:	LLFloater(key)
,	mQueryID(LLUUID())
,	mResultStart(0)
,	mNumResultsReceived(0)
,	mResultList(nullptr)
,	mTabContainer(nullptr)
,	mPanelWeb(nullptr)
,	mCurrentQuery()
,	mCurrentResultType(SE_UNDEFINED)
{
}

LLFloaterDirectory::~LLFloaterDirectory()
{
}

BOOL LLFloaterDirectory::postBuild()
{
	mResultList = findChild<LLScrollListCtrl>("results");
	mResultList->setCommitCallback(boost::bind(&LLFloaterDirectory::onCommitSelection, this));
	for (std::string panel_name: sSearchPanels)
	{
		LLPanelSearch* panel = static_cast<LLPanelSearch*>(findChild<LLUICtrl>(panel_name));
		if (panel)
			panel->setSearchFloater(this);
	}
	mPanelWeb = findChild<LLPanelSearchWeb>("panel_search_web");
	mTabContainer = findChild<LLTabContainer>("search_tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLFloaterDirectory::onTabChanged, this));
	setProgress(false);
	mResultsStatus = findChild<LLTextBase>("results_status");
	getChild<LLButton>("PageUp")->setCommitCallback(boost::bind(&LLFloaterDirectory::choosePage, this, _1));
	getChild<LLButton>("PageDn")->setCommitCallback(boost::bind(&LLFloaterDirectory::choosePage, this, _1));
	showDetailPanel(LLStringUtil::null); // hide all the panels
	paginate();
	return TRUE;
}

void LLFloaterDirectory::onOpen(const LLSD& key)
{
	Params p(key);
	mPanelWeb->loadUrl(p.search);
	if (key.has("query"))
	{
		mTabContainer->selectTabPanel(mPanelWeb);
	}
	onTabChanged();
}

void LLFloaterDirectory::setProgress(bool working)
{
	getChild<LLUICtrl>("loading")->setVisible(working);
}

void LLFloaterDirectory::setResultsComment(const std::string& message)
{
	mResultList->setCommentText(message);
}

void LLFloaterDirectory::onTabChanged()
{
	LLPanel* active_panel = mTabContainer->getCurrentPanel();
	bool show_detail = active_panel != mPanelWeb;
	findChild<LLLayoutStack>("results_stack")->setVisible(show_detail);
	findChild<LLButton>("PageUp")->setVisible(show_detail);
	findChild<LLButton>("PageDn")->setVisible(show_detail);
	mResultsStatus->setVisible(show_detail);
}

void LLFloaterDirectory::onCommitSelection()
{
	switch (mCurrentResultType)
	{
		case SE_PEOPLE:
		{
			LLSD params;
			params["avatar_id"] = mResultList->getSelectedValue().asUUID();
			getChild<LLPanel>("detail_avatar")->onOpen(params);
			showDetailPanel("detail_avatar");
			break;
		}
		case SE_GROUPS:
		{
			LLSD params;
			params["group_id"] = mResultList->getSelectedValue().asUUID();
			getChild<LLPanel>("detail_group")->onOpen(params);
			showDetailPanel("detail_group");
			break;
		}
		case SE_LANDSALES:
		case SE_PLACES:
		{
			LLSD params;
			params["type"] = "remote_place";
			params["id"] = mResultList->getSelectedValue().asUUID();
			getChild<LLPanel>("detail_place")->onOpen(params);
			showDetailPanel("detail_place");
			break;
		}
		case SE_CLASSIFIEDS:
		{
			LLSD params;
			params["classified_id"] = mResultList->getSelectedValue().asUUID();
			getChild<LLPanel>("detail_classified")->onOpen(params);
			showDetailPanel("detail_classified");
			break;
		}
		case SE_EVENTS:
		{
			getChild<LLPanel>("detail_event")->onOpen(mResultList->getSelectedValue().asInteger());
			showDetailPanel("detail_event");
			break;
		}
		case SE_UNDEFINED:
		default:
			LL_WARNS("Search") << "Unhandled search mode: " << mCurrentResultType << LL_ENDL;
			break;
	}
}

void LLFloaterDirectory::paginate()
{
	if (mNumResultsReceived)
	{
		LLStringUtil::format_map_t args;
		std::string total_str;
		LLResMgr::getInstance()->getIntegerString(total_str, mResultStart + mNumResultsReceived - 1);
		args["TOTAL"] = total_str;
		args["VISIBLE_END"] = total_str;
		total_str = LLStringUtil::null;
		LLResMgr::getInstance()->getIntegerString(total_str, mResultStart + 1);
		args["VISIBLE_BEGIN"] = total_str;
		mResultsStatus->setText(getString((mNumResultsReceived > mCurrentQuery.results_per_page)
										  ? "result_spillover" : "result_count",
										  args));
	}
	else
		mResultsStatus->setText(getString("no_results"));
	childSetEnabled("PageUp", mNumResultsReceived > mCurrentQuery.results_per_page);
	childSetEnabled("PageDn", mResultStart > 0);
}

void LLFloaterDirectory::choosePage(LLUICtrl* ctrl)
{
	if (ctrl->getName() == "PageUp")
		mResultStart += mCurrentQuery.results_per_page;
	else if (ctrl->getName() == "PageDn")
		mResultStart -= mCurrentQuery.results_per_page;
	else
	{
		LL_WARNS("Search") << "Unknown control: " << ctrl->getName() << LL_ENDL;
		return; // Fuck you, you lose.
	}
	queryDirectory(mCurrentQuery, false);
}

void LLFloaterDirectory::showDetailPanel(const std::string& panel_name)
{
	for (const std::string& panel_itr: sDetailPanels)
	{
		getChild<LLPanel>(panel_itr)->setVisible(panel_itr == panel_name);
	}
}

void LLFloaterDirectory::rebuildResultList()
{
	mResultList->clearColumns();
	switch (mCurrentResultType)
	{
		case SE_PEOPLE:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 1.f;
			mResultList->addColumn(name);
			break;
		}
		case SE_GROUPS:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 0.72f;
			mResultList->addColumn(name);
			
			LLScrollListColumn::Params members;
			members.name = "members";
			members.header.label = "Members";
			members.width.relative_width = 0.25f;
			mResultList->addColumn(members);
			
			LLScrollListColumn::Params score;
			score.name = "score";
			score.header.label = "Score";
			score.width.relative_width = 0.03f;
			mResultList->addColumn(score);
			break;
		}
		case SE_PLACES:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 0.81f;
			mResultList->addColumn(name);
			
			LLScrollListColumn::Params dwell;
			dwell.name = "dwell";
			dwell.header.label = "Traffic";
			dwell.width.relative_width = 0.19f;
			mResultList->addColumn(dwell);
			break;
		}
		case SE_LANDSALES:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 0.45f;
			mResultList->addColumn(name);
			
			LLScrollListColumn::Params price;
			price.name="price";
			price.header.label="Price";
			price.width.relative_width = 0.1f;
			mResultList->addColumn(price);
			
			LLScrollListColumn::Params area;
			area.name="area";
			area.header.label="Area";
			area.width.relative_width = 0.1f;
			mResultList->addColumn(area);
			
			LLScrollListColumn::Params ppm;
			ppm.name="ppm";
			ppm.header.label="L$/m";
			ppm.width.relative_width = 0.1f;
			mResultList->addColumn(ppm);
			
			LLScrollListColumn::Params type;
			type.name="type";
			type.header.label="Type";
			type.width.relative_width = 0.2f;
			mResultList->addColumn(type);
			break;
		}
		case SE_CLASSIFIEDS:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 0.7f;
			mResultList->addColumn(name);
			
			LLScrollListColumn::Params price;
			price.name="price";
			price.header.label="Price";
			price.width.relative_width = 0.3f;
			mResultList->addColumn(price);
			break;
		}
		case SE_EVENTS:
		{
			LLScrollListColumn::Params icon;
			icon.name = "icon";
			icon.width.pixel_width = 20;
			mResultList->addColumn(icon);
			
			LLScrollListColumn::Params name;
			name.name = "name";
			name.header.label = "Name";
			name.width.relative_width = 0.65f;
			mResultList->addColumn(name);
			
			//LLScrollListColumn::Params time;
			//time.name="time";
			//time.header.label="Time";
			//time.width.relative_width = 0.2f;
			//mResultList->addColumn(time);
			
			LLScrollListColumn::Params date;
			date.name="date";
			date.header.label="Date";
			date.width.relative_width = 0.3f;
			mResultList->addColumn(date);
			break;
		}
		case SE_UNDEFINED:
		default:
			LL_WARNS("Search") << "Unhandled search mode: " << mCurrentResultType << LL_ENDL;
			break;
			
	}
}

void LLFloaterDirectory::queryDirectory(const LLDirQuery& query, bool new_search)
{
	if (mCurrentResultType != query.type)
	{
		mCurrentResultType = query.type;
		rebuildResultList();
	}
	mResultList->clearRows();
	
	if (new_search)
	{
		mResultStart = 0;
	}
	
	mCurrentQuery = query;
	mNumResultsReceived = 0;
	mQueryID.generate();
	
	switch (mCurrentResultType)
	{
		case SE_PEOPLE:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirFindQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addStringFast(_PREHASH_QueryText, query.text);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, DFQ_PEOPLE);
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << LL_ENDL;
			break;
		}
		case SE_GROUPS:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirFindQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addStringFast(_PREHASH_QueryText, query.text);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, query.scope);
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << LL_ENDL;
			break;
		}
		case SE_EVENTS:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirFindQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addStringFast(_PREHASH_QueryText, query.text);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, query.scope);
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << " Search Text: " << query.text << LL_ENDL;
			break;
		}
		case SE_PLACES:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirPlacesQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addStringFast(_PREHASH_QueryText, query.text);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, query.scope);
			gMessageSystem->addS8Fast(_PREHASH_Category, query.category_char);
			// TODO: Search filter by region name.
			gMessageSystem->addStringFast(_PREHASH_SimName, "");
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << LL_ENDL;
			break;
		}
		case SE_LANDSALES:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirLandQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, query.scope);
			gMessageSystem->addU32Fast(_PREHASH_SearchType, query.category_int);
			gMessageSystem->addS32Fast(_PREHASH_Price, query.price);
			gMessageSystem->addS32Fast(_PREHASH_Area, query.area);
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << query.category_int << LL_ENDL;
			break;
		}
		case SE_CLASSIFIEDS:
		{
			gMessageSystem->newMessageFast(_PREHASH_DirClassifiedQuery);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_QueryData);
			gMessageSystem->addUUIDFast(_PREHASH_QueryID, mQueryID);
			gMessageSystem->addStringFast(_PREHASH_QueryText, query.text);
			gMessageSystem->addU32Fast(_PREHASH_QueryFlags, query.scope);
			gMessageSystem->addU32Fast(_PREHASH_Category, query.category_int);
			gMessageSystem->addS32Fast(_PREHASH_QueryStart, mResultStart);
			gAgent.sendReliableMessage();
			LL_DEBUGS("Search") << "Firing off search request: " << mQueryID << LL_ENDL;
			break;
		}
		case SE_UNDEFINED:
		default:
			break;
	}
	mResultList->setCommentText(getString("searching"));
	setProgress(true);
}

//static
void LLFloaterDirectory::processSearchPeopleReply(LLMessageSystem* msg, void**)
{
	LLUUID query_id;
	std::string first_name;
	std::string last_name;
	LLUUID agent_id;
	LLUUID avatar_id;
	U8 online;
	//S32 reputation;
	
	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	// *TODO: Get rid of this so we can have multiple search windows
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	self->setProgress(false);

	LLScrollListCtrl* pResults = self->mResultList;
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
		pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;

	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_FirstName,	first_name, i);
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_LastName,	last_name, i);
		msg->getUUIDFast(	_PREHASH_QueryReplies,	_PREHASH_AgentID,	agent_id, i);
		//msg->getS32Fast(	_PREHASH_QueryReplies,	_PREHASH_Reputation, reputation, i);
		msg->getU8Fast(		_PREHASH_QueryReplies,	_PREHASH_Online,	online, i);
		
		if (agent_id.isNull())
		{
			LL_INFOS("Search") << "Null result returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
		}
		else
		{
			LL_DEBUGS("Search") << "Got: " << first_name << " " << last_name << " AgentID: " << agent_id << LL_ENDL;
			pResults->setEnabled(TRUE);
			
			std::string avatar_name;
			avatar_name = LLCacheName::buildFullName(first_name, last_name);
			
			LLSD element;
			element["id"] = agent_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= online ? "icon_avatar_online.tga" : "icon_avatar_offline.tga";
			
			element["columns"][1]["column"]	= "name";
			element["columns"][1]["value"]	= avatar_name;
			
			pResults->addElement(element, ADD_BOTTOM);
		}
	}
	self->paginate();
}

//static
void LLFloaterDirectory::processSearchGroupsReply(LLMessageSystem* msg, void**)
{
	LLUUID query_id;
	LLUUID group_id;
	LLUUID agent_id;
	std::string group_name;
	S32 members;
	F32 search_order;
	
	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	
	self->setProgress(false);
	LLScrollListCtrl* pResults = self->mResultList;
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->getChild<LLUICtrl>("groups_edit")->getValue().asString();
			pResults->setCommentText(self->getString("not_found", map));
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			pResults->setCommentText(self->getString("search_short"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			pResults->setCommentText(self->getString("search_banned"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			pResults->setCommentText(self->getString("search_disabled"));
			return;
		}
	}
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
		pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getUUIDFast(	_PREHASH_QueryReplies,	_PREHASH_GroupID,		group_id,	i);
		msg->getStringFast(	_PREHASH_QueryReplies,	_PREHASH_GroupName,		group_name,	i);
		msg->getS32Fast(	_PREHASH_QueryReplies,	_PREHASH_Members,		members,	i);
		msg->getF32Fast(	_PREHASH_QueryReplies,	_PREHASH_SearchOrder,	search_order,i);
		if (group_id.isNull())
		{
			LL_DEBUGS("Search") << "No results returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
		}
		else
		{
			LL_DEBUGS("Search") << "Got: " << group_name << " GroupID: " << group_id << LL_ENDL;
			pResults->setEnabled(TRUE);
			
			LLSD element;
			
			element["id"] = group_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "Icon_Group";
			
			element["columns"][1]["column"]	= "name";
			element["columns"][1]["value"]	= group_name;
			
			element["columns"][2]["column"]	= "members";
			element["columns"][2]["value"]	= members;
			
			element["columns"][3]["column"]	= "score";
			element["columns"][3]["value"]	= search_order;
			
			pResults->addElement(element, ADD_BOTTOM);
		}
	}
	self->paginate();
}

//static
void LLFloaterDirectory::processSearchPlacesReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	parcel_id;
	std::string	name;
	BOOL	for_sale;
	BOOL	auction;
	F32		dwell;
	
	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	
	self->setProgress(false);
	LLScrollListCtrl* pResults = self->mResultList;
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			pResults->setCommentText(self->getString("search_short"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			pResults->setCommentText(self->getString("search_banned"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			pResults->setCommentText(self->getString("search_disabled"));
			return;
		}
	}
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
				pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getUUID(	"QueryReplies",	"ParcelID",	parcel_id,	i);
		msg->getString(	"QueryReplies",	"Name",		name,		i);
		msg->getBOOL(	"QueryReplies",	"ForSale",	for_sale,	i);
		msg->getBOOL(	"QueryReplies",	"Auction",	auction,	i);
		msg->getF32(	"QueryReplies",	"Dwell",	dwell,		i);
		if (parcel_id.isNull())
		{
			LL_DEBUGS("Search") << "Null result returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
						pResults->setCommentText(self->getString("not_found", map));
		}
		else
		{
			LL_DEBUGS("Search") << "Got: " << name << " ParcelID: " << parcel_id << LL_ENDL;
			pResults->setEnabled(TRUE);
			
			LLSD element;
			
			element["id"] = parcel_id;
			
			if (auction)
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_Auction";
			}
			else if (for_sale)
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_For_Sale";
			}
			else
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_Place";
			}
			
			element["columns"][1]["column"]	= "name";
			element["columns"][1]["value"]	= name;
			
			std::string buffer = llformat("%.0f", (F64)dwell);
			element["columns"][2]["column"]	= "dwell";
			element["columns"][2]["value"]	= buffer;
			
			pResults->addElement(element, ADD_BOTTOM);
		}
	}
	self->paginate();
}

//static
void LLFloaterDirectory::processSearchClassifiedsReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	classified_id;
	std::string name;
	U32		creation_date;
	U32		expiration_date;
	S32		price_for_listing;

	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	
	self->setProgress(false);
	LLScrollListCtrl* pResults = self->mResultList;
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			pResults->setCommentText(self->getString("search_short"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			pResults->setCommentText(self->getString("search_banned"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			pResults->setCommentText(self->getString("search_disabled"));
			return;
		}
	}
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
		pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getUUID(	"QueryReplies", "ClassifiedID",		classified_id,	i);
		msg->getString(	"QueryReplies", "Name",				name,			i);
		msg->getU32(	"QueryReplies", "CreationDate",		creation_date,	i);
		msg->getU32(	"QueryReplies", "ExpirationDate",	expiration_date,i);
		msg->getS32(	"QueryReplies", "PriceForListing",	price_for_listing,i);
		if (classified_id.isNull())
		{
			LL_DEBUGS("Search") << "No results returned for QueryID: " << query_id << LL_ENDL;
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
						pResults->setCommentText(self->getString("not_found", map));
		}
		else
		{
			LL_DEBUGS("Search") << "Got: " << name << " ClassifiedID: " << classified_id << LL_ENDL;
			pResults->setEnabled(TRUE);
			
			LLSD element;
			
			element["id"] = classified_id;
			
			element["columns"][0]["column"]	= "icon";
			element["columns"][0]["type"]	= "icon";
			element["columns"][0]["value"]	= "icon_top_pick.tga";
			
			element["columns"][1]["column"]	= "name";
			element["columns"][1]["value"]	= name;
			
			element["columns"][2]["column"]	= "price";
			element["columns"][2]["value"]	= price_for_listing;
			
			pResults->addElement(element, ADD_BOTTOM);
		}
	}
	self->paginate();
}

// static
void LLFloaterDirectory::processSearchLandReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	parcel_id;
	std::string	name;
	std::string land_sku;
	std::string land_type;
	BOOL	auction;
	BOOL	for_sale;
	S32		price;
	S32		area;
	
	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	
	self->setProgress(false);
	LLScrollListCtrl* pResults = self->mResultList;
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_PLACES_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
			return;
		}
		else if(status & STATUS_SEARCH_PLACES_SHORTSTRING)
		{
			pResults->setCommentText(self->getString("search_short"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
		{
			pResults->setCommentText(self->getString("search_banned"));
			return;
		}
		else if (status & STATUS_SEARCH_PLACES_SEARCHDISABLED)
		{
			pResults->setCommentText(self->getString("search_disabled"));
			return;
		}
	}
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
				pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;
	S32 not_auction = 0;
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getUUID(	"QueryReplies", "ParcelID",		parcel_id,	i);
		msg->getString(	"QueryReplies", "Name",			name,		i);
		msg->getBOOL(	"QueryReplies", "Auction",		auction,	i);
		msg->getBOOL(	"QueryReplies", "ForSale",		for_sale,	i);
		msg->getS32(	"QueryReplies", "SalePrice",	price,		i);
		msg->getS32(	"QueryReplies", "ActualArea",	area,		i);
		if (parcel_id.isNull())
		{
			LL_DEBUGS("Search") << "Null result returned for QueryID: " << query_id << LL_ENDL;
			pResults->setCommentText(self->getString("no_results"));
		}
		else
		{
			LL_DEBUGS("Search") << "Got: " << name << " ClassifiedID: " << parcel_id << LL_ENDL;
			pResults->setEnabled(TRUE);
			if ( msg->getSizeFast(_PREHASH_QueryReplies, i, _PREHASH_ProductSKU) > 0 )
			{
				msg->getStringFast(	_PREHASH_QueryReplies, _PREHASH_ProductSKU, land_sku, i);
				land_type = LLProductInfoRequestManager::instance().getDescriptionForSku(land_sku);
			}
			else
			{
				land_sku.clear();
				land_type = LLTrans::getString("land_type_unknown");
			}
			if (parcel_id.isNull())
				continue;
			
			LLSD element;
			
			element["id"] = parcel_id;
			if (auction)
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_Auction";
			}
			else if (for_sale)
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_For_Sale";
			}
			else
			{
				element["columns"][0]["column"]	= "icon";
				element["columns"][0]["type"]	= "icon";
				element["columns"][0]["value"]	= "Icon_Place";
			}
			
			element["columns"][1]["column"]	= "name";
			element["columns"][1]["value"]	= name;
			
			std::string buffer = "Auction";
			if (!auction)
			{
				buffer = llformat("%d", price);
				not_auction++;
			}
			element["columns"][2]["column"]	= "price";
			element["columns"][2]["value"]	= price;
			
			element["columns"][3]["column"]	= "area";
			element["columns"][3]["value"]	= area;
			if (!auction)
			{
				F32 ppm;
				if (area > 0)
					ppm = price / area;
				else
					ppm = 0.f;
				std::string buffer = llformat("%.1f", ppm);
				element["columns"][4]["column"]	= "ppm";
				element["columns"][4]["value"]	= buffer;
			}
			else
			{
				element["columns"][4]["column"]	= "ppm";
				element["columns"][4]["value"]	= "1.0";
			}
			
			element["columns"][5]["column"]	= "type";
			element["columns"][5]["value"]	= land_type;
			
			pResults->addElement(element, ADD_BOTTOM);
		}
	}
	self->paginate();
}

// static
void LLFloaterDirectory::processSearchEventsReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	owner_id;
	std::string	name;
	std::string	date;
	
	msg->getUUIDFast(_PREHASH_QueryData, _PREHASH_QueryID, query_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	
	if (agent_id != gAgent.getID()) return; // not for us
	LL_DEBUGS("Search") << "Received results for query id: " << query_id << LL_ENDL;
	
	LLFloaterDirectory* self = LLFloaterReg::findTypedInstance<LLFloaterDirectory>("search");
	if (self == NULL || query_id != self->mQueryID) return; // not the result we're waiting for
	
	self->setProgress(false);
	LLScrollListCtrl* pResults = self->mResultList;
	
	// Check for status messages
	if (msg->getNumberOfBlocks("StatusData"))
	{
		U32 status;
		msg->getU32("StatusData", "Status", status);
		if (status & STATUS_SEARCH_EVENTS_FOUNDNONE)
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = self->mCurrentQuery.text;
			pResults->setCommentText(self->getString("not_found", map));
			return;
		}
		else if(status & STATUS_SEARCH_EVENTS_SHORTSTRING)
		{
			pResults->setCommentText(self->getString("search_short"));
			return;
		}
		else if (status & STATUS_SEARCH_EVENTS_BANNEDWORD)
		{
			pResults->setCommentText(self->getString("search_banned"));
			return;
		}
		else if (status & STATUS_SEARCH_EVENTS_SEARCHDISABLED)
		{
			pResults->setCommentText(self->getString("search_disabled"));
			return;
		}
		else if (status & STATUS_SEARCH_EVENTS_NODATEOFFSET)
		{
			pResults->setCommentText(self->getString("search_no_date_offset"));
			return;
		}
		else if (status & STATUS_SEARCH_EVENTS_NOCATEGORY)
		{
			pResults->setCommentText(self->getString("search_no_events_category"));
			return;
		}
		else if (status & STATUS_SEARCH_EVENTS_NOQUERY)
		{
			pResults->setCommentText(self->getString("search_no_query"));
			return;
		}
	}
	
	S32 num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
	if (num_new_rows == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = self->mCurrentQuery.text;
				pResults->setCommentText(self->getString("not_found", map));
	}
	
	self->mNumResultsReceived += num_new_rows;
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		U32 event_id;
		//U32 unix_time;
		U32 event_flags;
		
		msg->getUUID(	"QueryReplies",	"OwnerID",		owner_id,	i);
		msg->getString(	"QueryReplies",	"Name",			name,		i);
		msg->getU32(	"QueryReplies",	"EventID",		event_id,	i);
		msg->getString(	"QueryReplies",	"Date",			date,		i);
		//msg->getU32(	"QueryReplies",	"UnixTime",		unix_time,	i);
		msg->getU32(	"QueryReplies",	"EventFlags",	event_flags,i);
		
		static LLUICachedControl<bool> inc_pg("ShowPGEvents", 1);
		static LLUICachedControl<bool> inc_mature("ShowMatureEvents", 0);
		static LLUICachedControl<bool> inc_adult("ShowAdultEvents", 0);
		
		// Skip empty events...
		if (owner_id.isNull())
		{
			LL_INFOS("Search") << "Skipped " << event_id << " because of a NULL owner result" << LL_ENDL;
			continue;
		}
		// Skips events that don't match our scope...
		if (((event_flags & (EVENT_FLAG_ADULT | EVENT_FLAG_MATURE)) == EVENT_FLAG_NONE) && !inc_pg)
		{
			LL_INFOS("Search") << "Skipped " << event_id << " because it was out of scope" << LL_ENDL;
			continue;
		}
		if ((event_flags & EVENT_FLAG_MATURE) && !inc_mature)
		{
			LL_INFOS("Search") << "Skipped " << event_id << " because it was out of scope" << LL_ENDL;
			continue;
		}
		if ((event_flags & EVENT_FLAG_ADULT) && !inc_adult)
		{
			LL_INFOS("Search") << "Skipped " << event_id << " because it was out of scope" << LL_ENDL;
			continue;
		}
		pResults->setEnabled(TRUE);

		LLSD element;
		
		element["id"] = llformat("%u", event_id);
		
		if (event_flags == EVENT_FLAG_ADULT)
		{
			element["columns"][0]["column"] = "icon";
			element["columns"][0]["type"] = "icon";
			element["columns"][0]["value"] = "Parcel_R_Dark";
		}
		else if (event_flags == EVENT_FLAG_MATURE)
		{
			element["columns"][0]["column"] = "icon";
			element["columns"][0]["type"] = "icon";
			element["columns"][0]["value"] = "Parcel_M_Dark";
		}
		else
		{
			element["columns"][0]["column"] = "icon";
			element["columns"][0]["type"] = "icon";
			element["columns"][0]["value"] = "Parcel_PG_Dark";
		}
		
		element["columns"][1]["column"] = "name";
		element["columns"][1]["value"] = name;
		
		element["columns"][2]["column"] = "date";
		element["columns"][2]["value"] = date;
		
		//element["columns"][3]["column"] = "time";
		//element["columns"][3]["value"] = llformat("%u", unix_time);
		
		pResults->addElement(element, ADD_BOTTOM);
	}
	self->paginate();
}
