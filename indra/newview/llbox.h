/** 
 * @file llbox.h
 * @brief Draws a box using display lists for speed.
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

#ifndef LL_LLBOX_H
#define LL_LLBOX_H

class LLBox
{
protected:
//	GLuint	mDisplayList;
	F32		mVertex[8][3];
	U32		mTriangleCount;
public:
	void	prerender();
	void	cleanupGL();

	void	renderface(S32 which_face);
	void	render();

	U32		getTriangleCount()			{ return mTriangleCount; }
};

extern LLBox gBox;

#endif
