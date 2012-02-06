/** 
 * @file llclipboard.cpp
 * @brief LLClipboard base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llclipboard.h"

#include "llerror.h"
#include "llmath.h"
#include "llstring.h"
#include "llview.h"
#include "llwindow.h"

LLClipboard::LLClipboard()
: mCutMode(false)
{
}

LLClipboard::~LLClipboard()
{
	reset();
}

void LLClipboard::add(const LLUUID& object)
{
	mObjects.put(object);
}

void LLClipboard::store(const LLUUID& object)
{
	reset();
	mObjects.put(object);
}

void LLClipboard::store(const LLDynamicArray<LLUUID>& inv_objects)
{
	reset();
	S32 count = inv_objects.count();
	for(S32 i = 0; i < count; i++)
	{
		mObjects.put(inv_objects[i]);
	}
}

void LLClipboard::cut(const LLUUID& object)
{
	if(!mCutMode && !mObjects.empty())
	{
		//looks like there are some stored items, reset clipboard state
		reset();
	}
	mCutMode = true;
	add(object);
}
void LLClipboard::retrieve(LLDynamicArray<LLUUID>& inv_objects) const
{
	inv_objects.reset();
	S32 count = mObjects.count();
	for(S32 i = 0; i < count; i++)
	{
		inv_objects.put(mObjects[i]);
	}
}

void LLClipboard::reset()
{
	mObjects.reset();
	mCutMode = false;
}

// Returns true if the LL Clipboard has pasteable items in it
BOOL LLClipboard::hasContents() const
{
	return (mObjects.count() > 0);
}

// Copy the input string to the LL and the system clipboard
bool LLClipboard::copyToClipboard(const LLWString &src, S32 pos, S32 len, bool use_primary)
{
	reset();
	mString = src.substr(pos, len);
	return (use_primary ? LLView::getWindow()->copyTextToPrimary(mString) : LLView::getWindow()->copyTextToClipboard(mString));
}

// Copy the input uuid to the LL clipboard
// Convert the uuid to string and copy that string to the system clipboard if legit
bool LLClipboard::copyToClipboard(const LLUUID& src, const LLAssetType::EType type)
{
	bool res = false;
	reset();
	if (src.notNull())
	{
		res = true;
		if (LLAssetType::lookupIsAssetIDKnowable(type))
		{
			LLWString source = utf8str_to_wstring(src.asString());
			res = copyToClipboard(source, 0, source.size());
		}
		if (res)
		{
			store(src);
		}
	}
	return res;
}

// Copy the System clipboard to the output string.
// Manage the LL Clipboard / System clipboard consistency
bool LLClipboard::pasteFromClipboard(LLWString &dst, bool use_primary)
{
	bool res = (use_primary ? LLView::getWindow()->pasteTextFromPrimary(dst) : LLView::getWindow()->pasteTextFromClipboard(dst));
	if (res)
	{
		if (dst != mString)
		{
			// Invalidate the LL clipboard if the System had a different string in it (i.e. some copy/cut was done in some other app)
			reset();
		}
		mString = dst;
	}
	return res;
}

// Return true if there's something on the System clipboard
bool LLClipboard::isTextAvailable(bool use_primary) const
{
	return (use_primary ? LLView::getWindow()->isPrimaryTextAvailable() : LLView::getWindow()->isClipboardTextAvailable());
}

