/** 
 * @file llpreviewanim.h
 * @brief LLPreviewAnim class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#ifndef LL_LLPREVIEWANIM_H
#define LL_LLPREVIEWANIM_H

#include "llpreview.h"
#include "llcharacter.h"

class LLPreviewAnim : public LLPreview
{
public:
	LLPreviewAnim(const std::string& name, const LLRect& rect, const std::string& title,
					const LLUUID& item_uuid,
					const S32&    activate,
					const LLUUID& object_uuid = LLUUID::null);

	static void playAnim( void* userdata );
	static void auditionAnim( void* userdata );
	static void saveAnim( void* userdata );
	static void endAnimCallback( void *userdata );

protected:
	virtual void onClose(bool app_quitting);

	LLAnimPauseRequest	mPauseRequest;
	LLUUID		mItemID;
	LLString	mTitle;
	LLUUID		mObjectID;
	LLButton*	mPlayBtn;
	LLButton*	mAuditionBtn;
};

#endif  // LL_LLPREVIEWSOUND_H
