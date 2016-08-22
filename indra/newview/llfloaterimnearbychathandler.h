/** 
 * @file llfloaterimnearbychathandler.h
 * @brief nearby chat notify
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERIMNEARBYCHATHANDLER_H
#define LL_LLFLOATERIMNEARBYCHATHANDLER_H

#include "llnotificationhandler.h"

typedef enum e_nearby_chat_output
{
	E_NEARBY_OUTPUT_TOAST = 0,
	E_NEARBY_OUTPUT_BUBBLE,
	E_NEARBY_OUTPUT_BOTH,
	E_NEARBY_OUTPUT_NONE
}ENearbyChatOutput;

class LLEventPump;

//add LLFloaterIMNearbyChatHandler to LLNotificationsUI namespace
namespace LLNotificationsUI{

class LLFloaterIMNearbyChatHandler : public LLChatHandler
{
public:

	LLFloaterIMNearbyChatHandler();
	virtual ~LLFloaterIMNearbyChatHandler();


	virtual void processChat(const LLChat& chat_msg, const LLSD &args);

protected:
	virtual void initChannel();

	static boost::scoped_ptr<LLEventPump> sChatWatcher;
};

}

#endif /* LL_LLFLOATERIMNEARBYCHATHANDLER_H */
