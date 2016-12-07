// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
 * @file llfloatertransactionlog.h
 * @brief Transaction log floater
 *
 * (C) 2014 Cinder Roxley @ Second Life <cinder@sdf.org>
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
 */

#include "llviewerprecompiledheaders.h"
#include "llfloatertransactionlog.h"

#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lltextbase.h"

LLFloaterTransactionLog::LLFloaterTransactionLog(const LLSD& key)
:	LLFloater(key)
,	mList(nullptr)
,	mTotalText(nullptr)
,	mTotal(0)
,	mAvatarNameCacheConnection()
{
	mCommitCallbackRegistrar.add("TL.Reset", boost::bind(&LLFloaterTransactionLog::reset, this));
}

LLFloaterTransactionLog::~LLFloaterTransactionLog()
{
	if (mAvatarNameCacheConnection.connected())
		mAvatarNameCacheConnection.disconnect();
}

BOOL LLFloaterTransactionLog::postBuild()
{
	mList = getChild<LLScrollListCtrl>("transaction_list");
	mList->setDoubleClickCallback(boost::bind(&LLFloaterTransactionLog::onDoubleClick, this));
	mTotalText = getChild<LLTextBase>("total");
	return TRUE;
}

void LLFloaterTransactionLog::addTransaction(const LLDate& date, const LLUUID& sender, S32 amount)
{
	// drop it
	if (!getVisible()) return;
	
	LLStringUtil::format_map_t args;
	args["TOTAL"] = std::to_string(mTotal += amount);
	mTotalText->setValue(getString("total_fmt", args));
	
	LLSD row;
	row["value"] = sender;
	row["column"][0]["column"] = "time";
	row["column"][0]["value"] = date.toHTTPDateString(LLStringExplicit("%H:%M:%S"));;
	row["column"][2]["column"] = "amount";
	row["column"][2]["value"] = llformat("L$%d", amount);
	row["column"][2]["halign"] = LLFontGL::RIGHT;
	
	mAvatarNameCacheConnection = LLAvatarNameCache::get(sender, boost::bind(&LLFloaterTransactionLog::onAvatarNameCache, this, _1, _2, row));
}

void LLFloaterTransactionLog::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name, LLSD& row)
{
	row["column"][1]["column"] = "name";
	row["column"][1]["value"] = av_name.getDisplayName();
	mList->addElement(row);
}

void LLFloaterTransactionLog::onDoubleClick()
{
	const LLUUID& id = mList->getFirstSelected()->getValue().asUUID();
	LLAvatarActions::showProfile(id);
}

void LLFloaterTransactionLog::reset()
{
	mList->deleteAllItems();
	mTotal = 0;
	LLStringUtil::format_map_t args;
	args["TOTAL"] = std::to_string(mTotal);
	mTotalText->setValue(getString("total_fmt", args));
}
