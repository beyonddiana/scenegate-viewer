/** 
 * @file llclipboard.cpp
 * @brief LLClipboard base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "llclipboard.h"

#include "llerror.h"
#include "llmath.h"
#include "llstring.h"
#include "llview.h"
#include "llwindow.h"

// Global singleton
LLClipboard gClipboard;


LLClipboard::LLClipboard()
{
}


LLClipboard::~LLClipboard()
{
}


void LLClipboard::copyFromSubstring(const LLWString &src, S32 pos, S32 len, const LLUUID& source_id )
{
	mSourceID = source_id;
	mString = src.substr(pos, len);
	LLView::getWindow()->copyTextToClipboard( mString );
}


LLWString LLClipboard::getPasteWString( LLUUID* source_id )
{
	if( mSourceID.notNull() )
	{
		LLWString temp_string;
		LLView::getWindow()->pasteTextFromClipboard(temp_string);

		if( temp_string != mString )
		{
			mSourceID.setNull();
			mString = temp_string;
		}
	}
	else
	{
		LLView::getWindow()->pasteTextFromClipboard(mString);
	}

	if( source_id )
	{
		*source_id = mSourceID;
	}

	return mString;
}


BOOL LLClipboard::canPasteString()
{
	return LLView::getWindow()->isClipboardTextAvailable();
}
