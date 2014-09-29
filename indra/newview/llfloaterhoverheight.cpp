/** 
* @file llfloaterhoverheight.cpp
* @brief Controller for self avatar hover height
* @author vir@lindenlab.com
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#include "llfloaterhoverheight.h"
#include "llsliderctrl.h"
#include "llviewercontrol.h"

LLFloaterHoverHeight::LLFloaterHoverHeight(const LLSD& key) : LLFloater(key)
{
}

BOOL LLFloaterHoverHeight::postBuild()
{
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	LLVector3 offset = gSavedSettings.getVector3("AvatarPosFinalOffset");
	F32 value = offset[2];
	sldrCtrl->setValue(value,FALSE);
	childSetCommitCallback("HoverHeightSlider", &LLFloaterHoverHeight::onSliderMoved, NULL);

	return TRUE;
}

// static
void LLFloaterHoverHeight::onSliderMoved(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	F32 value = sldrCtrl->getValueF32();

	LLVector3 offset = gSavedSettings.getVector3("AvatarPosFinalOffset");
	offset[2] = value;
	gSavedSettings.setVector3("AvatarPosFinalOffset",offset);
}
