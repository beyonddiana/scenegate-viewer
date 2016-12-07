// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llfloaterhandler.h"

#include "llfloater.h"
#include "llmediactrl.h"

// register with dispatch via global object
LLFloaterHandler gFloaterHandler;


LLFloater* get_parent_floater(LLView* view)
{
	LLFloater* floater = NULL;
	LLView* parent = view->getParent();
	while (parent)
	{
		floater = dynamic_cast<LLFloater*>(parent);
		if (floater)
		{
			break;
		}
		parent = parent->getParent();
	}
	return floater;
}


bool LLFloaterHandler::handle(const LLSD &params, const LLSD &query_map, LLMediaCtrl *web)
{
	if (params.size() < 2) return false;
	LLFloater* floater = NULL;
	// *TODO: implement floater lookup by name
	if (params[0].asString() == "self")
	{
		if (web)
		{
			floater = get_parent_floater(web);
		}
	}
	if (params[1].asString() == "close")
	{
		if (floater)
		{
			floater->closeFloater();
			return true;
		}
	}
	return false;
}
