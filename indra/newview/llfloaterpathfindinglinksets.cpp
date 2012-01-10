/** 
 * @file llfloaterpathfindinglinksets.cpp
 * @author William Todd Stinson
 * @brief "Pathfinding linksets" floater, allowing manipulation of the Havok AI pathfinding settings.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llfloaterpathfindinglinksets.h"
#include "llsd.h"
#include "llagent.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llhttpclient.h"
#include "lltextbase.h"
#include "lluuid.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"

//---------------------------------------------------------------------------
// NavmeshDataGetResponder
//---------------------------------------------------------------------------

class NavmeshDataGetResponder : public LLHTTPClient::Responder
{
public:
	NavmeshDataGetResponder(const std::string& pNavmeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater);
	virtual ~NavmeshDataGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavmeshDataGetResponder(const NavmeshDataGetResponder& pOther);

	std::string                  mNavmeshDataGetURL;
	LLFloaterPathfindingLinksets *mLinksetsFloater;
};

//---------------------------------------------------------------------------
// LLFloaterPathfindingLinksets
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	childSetAction("refresh_linksets_list", boost::bind(&LLFloaterPathfindingLinksets::onRefreshLinksetsClicked, this));
	childSetAction("select_all_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked, this));
	childSetAction("select_none_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked, this));

	mLinksetsScrollList = findChild<LLScrollListCtrl>("pathfinding_linksets");
	llassert(mLinksetsScrollList != NULL);
	mLinksetsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onLinksetsSelectionChange, this));
	mLinksetsScrollList->setCommitOnSelectionChange(true);

	mLinksetsStatus = findChild<LLTextBase>("linksets_status");
	llassert(mLinksetsStatus != NULL);

	setFetchState(kFetchInitial);

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	sendNavmeshDataGetRequest();
}

void LLFloaterPathfindingLinksets::openLinksetsEditor()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_linksets");
}

LLFloaterPathfindingLinksets::EFetchState LLFloaterPathfindingLinksets::getFetchState() const
{
	return mFetchState;
}

BOOL LLFloaterPathfindingLinksets::isFetchInProgress() const
{
	BOOL retVal;
	switch (getFetchState())
	{
	case kFetchStarting :
	case kFetchInProgress :
	case kFetchInProgress_MultiRequested :
	case kFetchReceived :
		retVal = true;
		break;
	default :
		retVal = false;
		break;
	}

	return retVal;
}

LLFloaterPathfindingLinksets::LLFloaterPathfindingLinksets(const LLSD& pSeed)
	: LLFloater(pSeed),
	mFetchState(kFetchInitial),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL)
{
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

void LLFloaterPathfindingLinksets::sendNavmeshDataGetRequest()
{
	if (isFetchInProgress())
	{
		if (getFetchState() == kFetchInProgress)
		{
			setFetchState(kFetchInProgress_MultiRequested);
		}
	}
	else
	{
		setFetchState(kFetchStarting);
		clearLinksetsList();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			std::string navmeshDataURL = region->getCapability("ObjectNavmesh");
			if (navmeshDataURL.empty())
			{
				setFetchState(kFetchComplete);
				llwarns << "cannot query navmesh data from current region '" << region->getName() << "'" << llendl;
			}
			else
			{
				setFetchState(kFetchInProgress);
				LLHTTPClient::get(navmeshDataURL, new NavmeshDataGetResponder(navmeshDataURL, this));
			}
		}
	}
}

void LLFloaterPathfindingLinksets::handleNavmeshDataGetReply(const LLSD& pNavmeshData)
{
	setFetchState(kFetchReceived);
	clearLinksetsList();

	const LLVector3& avatarPosition = gAgent.getPositionAgent();

	for (LLSD::map_const_iterator itemsIter = pNavmeshData.beginMap();
		itemsIter != pNavmeshData.endMap(); ++itemsIter)
	{
		LLUUID uuid(itemsIter->first);
		const LLSD& itemData = itemsIter->second;

		const LLSD::String& itemName = itemData.get("name").asString();
		const LLSD::String& itemDescription = itemData.get("description").asString();
		LLSD::Integer itemLandImpact = itemData.get("landimpact").asInteger();
		LLSD::Boolean isItemFixed = itemData.get("fixed").asBoolean();
		LLSD::Boolean isItemWalkable = true; // XXX stinson: walkable data not coming from service yet
		LLSD::Boolean isItemPhantom = itemData.get("phantom").asBoolean();
		LLSD::Real itemA = itemData.get("A").asReal();
		LLSD::Real itemB = itemData.get("B").asReal();
		LLSD::Real itemC = itemData.get("C").asReal();
		LLSD::Real itemD = itemData.get("D").asReal();

		// XXX stinson: get a better way to get all objects locations in the region as the
		// following calculation only returns objects of which the viewer is aware.
		LLViewerObject *viewerObject = gObjectList.findObject(uuid);
		bool hasDistance = (viewerObject != NULL);
		F32 itemDistance = -999.0f;
		if (hasDistance)
		{
			const LLVector3& itemLocation = viewerObject->getPositionAgent();
			itemDistance = dist_vec(avatarPosition, itemLocation);
		}

		LLSD columns;

		columns[0]["column"] = "name";
		columns[0]["value"] = itemName;
		columns[0]["font"] = "SANSSERIF";

		columns[1]["column"] = "description";
		columns[1]["value"] = itemDescription;
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "land_impact";
		columns[2]["value"] = llformat("%1d", itemLandImpact);
		columns[2]["font"] = "SANSSERIF";

		columns[3]["column"] = "dist_from_you";
		if (hasDistance)
		{
			columns[3]["value"] = llformat("%1.0f m", itemDistance);
		}
		else
		{
			columns[3]["value"] = "--";
		}
		columns[3]["font"] = "SANSSERIF";

		columns[4]["column"] = "is_fixed";
		columns[4]["value"] = getString(isItemFixed ? "linkset_is_fixed" : "linkset_is_not_fixed");
		columns[4]["font"] = "SANSSERIF";

		columns[5]["column"] = "is_walkable";
		columns[5]["value"] = getString(isItemWalkable ? "linkset_is_walkable" : "linkset_is_not_walkable");
		columns[5]["font"] = "SANSSERIF";

		columns[6]["column"] = "is_phantom";
		columns[6]["value"] = getString(isItemPhantom ? "linkset_is_phantom" : "linkset_is_not_phantom");
		columns[6]["font"] = "SANSSERIF";

		columns[7]["column"] = "a_percent";
		columns[7]["value"] = llformat("%2.0f", itemA);
		columns[7]["font"] = "SANSSERIF";

		columns[8]["column"] = "b_percent";
		columns[8]["value"] = llformat("%2.0f", itemB);
		columns[8]["font"] = "SANSSERIF";

		columns[9]["column"] = "c_percent";
		columns[9]["value"] = llformat("%2.0f", itemC);
		columns[9]["font"] = "SANSSERIF";

		columns[10]["column"] = "d_percent";
		columns[10]["value"] = llformat("%2.0f", itemD);
		columns[10]["font"] = "SANSSERIF";

		LLSD element;
		element["id"] = uuid;
		element["column"] = columns;

		mLinksetsScrollList->addElement(element);
	}

	setFetchState(kFetchComplete);
}

void LLFloaterPathfindingLinksets::handleNavmeshDataGetError(const std::string& pURL, const std::string& pErrorReason)
{
	setFetchState(kFetchError);
	clearLinksetsList();
	llwarns << "Error fetching navmesh data from URL '" << pURL << "' because " << pErrorReason << llendl;
}

void LLFloaterPathfindingLinksets::setFetchState(EFetchState pFetchState)
{
	mFetchState = pFetchState;
	updateLinksetsStatusMessage();
}

void LLFloaterPathfindingLinksets::onLinksetsSelectionChange()
{
	updateLinksetsStatusMessage();
}

void LLFloaterPathfindingLinksets::onRefreshLinksetsClicked()
{
	sendNavmeshDataGetRequest();
}

void LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked()
{
	selectAllLinksets();
}

void LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked()
{
	selectNoneLinksets();
}

void LLFloaterPathfindingLinksets::clearLinksetsList()
{
	mLinksetsScrollList->deleteAllItems();
	updateLinksetsStatusMessage();
}

void LLFloaterPathfindingLinksets::selectAllLinksets()
{
	mLinksetsScrollList->selectAll();
}

void LLFloaterPathfindingLinksets::selectNoneLinksets()
{
	mLinksetsScrollList->deselectAllItems();
}

void LLFloaterPathfindingLinksets::updateLinksetsStatusMessage()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getFetchState())
	{
	case kFetchStarting :
		statusText = getString("linksets_fetching_starting");
		break;
	case kFetchInProgress :
		statusText = getString("linksets_fetching_inprogress");
		break;
	case kFetchInProgress_MultiRequested :
		statusText = getString("linksets_fetching_inprogress_multi_request");
		break;
	case kFetchReceived :
		statusText = getString("linksets_fetching_received");
		break;
	case kFetchError :
		statusText = getString("linksets_fetching_error");
		styleParams.color = warningColor;
		break;
	case kFetchComplete :
		if (mLinksetsScrollList->isEmpty())
		{
			statusText = getString("linksets_fetching_done_none_found");
		}
		else
		{
			S32 numItems = mLinksetsScrollList->getItemCount();
			S32 numSelectedItems = mLinksetsScrollList->getNumSelected();

			LLLocale locale(LLStringUtil::getLocale());
			std::string numItemsString;
			LLResMgr::getInstance()->getIntegerString(numItemsString, numItems);

			std::string numSelectedItemsString;
			LLResMgr::getInstance()->getIntegerString(numSelectedItemsString, numSelectedItems);

			LLStringUtil::format_map_t string_args;
			string_args["[NUM_SELECTED]"] = numSelectedItemsString;
			string_args["[NUM_TOTAL]"] = numItemsString;
			statusText = getString("linksets_fetching_done_available", string_args);
		}
		break;
	case kFetchInitial:
	default:
		statusText = getString("linksets_fetching_initial");
		break;
	}

	mLinksetsStatus->setText((LLStringExplicit)statusText, styleParams);
}

NavmeshDataGetResponder::NavmeshDataGetResponder(const std::string& pNavmeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater)
	: mNavmeshDataGetURL(pNavmeshDataGetURL),
	mLinksetsFloater(pLinksetsFloater)
{
}

NavmeshDataGetResponder::~NavmeshDataGetResponder()
{
	mLinksetsFloater = NULL;
}

void NavmeshDataGetResponder::result(const LLSD& pContent)
{
	mLinksetsFloater->handleNavmeshDataGetReply(pContent);
}

void NavmeshDataGetResponder::error(U32 status, const std::string& reason)
{
	mLinksetsFloater->handleNavmeshDataGetError(mNavmeshDataGetURL, reason);
}
