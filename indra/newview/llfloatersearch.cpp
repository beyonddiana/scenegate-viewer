/** 
 * @file llfloatersearch.cpp
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
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
#include "llfloatersearch.h"
#include "llmediactrl.h"
#include "lllogininstance.h"
#include "lluri.h"
#include "llagent.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llweb.h"

LLFloaterSearch::LLFloaterSearch(const LLSD& key) :
	LLFloater(key),
	LLViewerMediaObserver(),
	mBrowser(NULL),
	mSearchGodLevel(0)
{
	// declare a map that transforms a category name into
	// the URL suffix that is used to search that category
	mCategoryPaths = LLSD::emptyMap();
	mCategoryPaths["all"]          = "search";
	mCategoryPaths["people"]       = "search/people";
	mCategoryPaths["places"]       = "search/places";
	mCategoryPaths["events"]       = "search/events";
	mCategoryPaths["groups"]       = "search/groups";
	mCategoryPaths["wiki"]         = "search/wiki";
	mCategoryPaths["destinations"] = "destinations";
	mCategoryPaths["classifieds"]  = "classifieds";
}

BOOL LLFloaterSearch::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	if (mBrowser)
	{
		mBrowser->addObserver(this);
		mBrowser->setTrusted(true);
	}

	return TRUE;
}

void LLFloaterSearch::onOpen(const LLSD& key)
{
	search(key);
}

void LLFloaterSearch::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
	switch (event) 
	{
	case MEDIA_EVENT_NAVIGATE_BEGIN:
		childSetText("status_text", getString("loading_text"));
		break;
		
	case MEDIA_EVENT_NAVIGATE_COMPLETE:
		childSetText("status_text", getString("done_text"));
		break;

	default:
		break;
	}
}

void LLFloaterSearch::godLevelChanged(U8 godlevel)
{
	// search results can change based upon god level - if the user
	// changes god level, then give them a warning (we don't refresh
	// the search as this might undo any page navigation or
	// AJAX-driven changes since the last search).
	childSetVisible("refresh_search", (godlevel != mSearchGodLevel));
}

void LLFloaterSearch::search(const LLSD &key)
{
	if (! mBrowser)
	{
		return;
	}

	// reset the god level warning as we're sending the latest state
	childHide("refresh_search");
	mSearchGodLevel = gAgent.getGodLevel();

	// work out the subdir to use based on the requested category
	LLSD subs;
	std::string category = key.has("category") ? key["category"].asString() : "";
	if (mCategoryPaths.has(category))
	{
		subs["CATEGORY"] = mCategoryPaths[category].asString();
	}
	else
	{
		subs["CATEGORY"] = mCategoryPaths["all"].asString();
	}

	// add the search query string
	std::string search_text = key.has("id") ? key["id"].asString() : "";
	subs["QUERY"] = LLURI::escape(search_text);

	// add the permissions token that login.cgi gave us
	LLSD search_token = LLLoginInstance::getInstance()->getResponse("search_token");
	subs["AUTH_KEY"] = search_token.asString();

	// add the user's preferred maturity (can be changed via prefs)
	std::string maturity;
	if (gAgent.prefersAdult())
	{
		maturity = "42";  // PG,Mature,Adult
	}
	else if (gAgent.prefersMature())
	{
		maturity = "21";  // PG,Mature
	}
	else
	{
		maturity = "13";  // PG
	}
	subs["MATURITY"] = maturity;

	// add the user's god status
	subs["GODLIKE"] = gAgent.isGodlike() ? "1" : "0";

	// get the search URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string url = gSavedSettings.getString("SearchURL");
	url = LLWeb::expandURLSubstitutions(url, subs);

	// and load the URL in the web view
	mBrowser->navigateTo(url);
}
