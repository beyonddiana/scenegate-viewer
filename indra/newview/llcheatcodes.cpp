/**
 * @file llcheatcodes.cpp
 * @brief Cheatcode slurls for virtual worlds, yo.
 *
 * Copyright (c) 2015, Cinder Roxley <cinder@sdf.org>
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
#include "llcommandhandler.h"

#include "llchat.h"
#include "llfloaterimnearbychat.h"
#include "llfloaterreg.h"
#include "llstatusbar.h"
#include "lltrans.h"

class LLXyzzyHandler : public LLCommandHandler
{
public:
	LLXyzzyHandler() : LLCommandHandler("xyzzy", UNTRUSTED_THROTTLE) {}
	
	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
		if (nearby_chat)
		{
			LLChat chat(LLTrans::getString("NothingHappens"));
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			nearby_chat->addMessage(chat);
		}
		return true;
	}
};

class LLDyeMenuHandler : public LLCommandHandler
{
public:
	LLDyeMenuHandler() : LLCommandHandler("dyemenu", UNTRUSTED_THROTTLE) {}
	
	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (params.size() != 1)
			return false;
		
		LLColor4 color = LLUIColorTable::instance().getColor(params[0].asString(),
															 gMenuBarView->getBackgroundColor().get());
		gMenuBarView->setBackgroundColor(color);
		gStatusBar->setBackgroundColor(color);
		return true;
	}
};

LLXyzzyHandler gXyzzyHandler;
LLDyeMenuHandler gDyeMenuHandler;
