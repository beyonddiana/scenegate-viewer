/**
 * @file llflashtimer.h
 * @brief LLFlashTimer class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_FLASHTIMER_H
#define LL_FLASHTIMER_H

#include "lleventtimer.h"

class LLFlashTimer : public LLEventTimer
{
public:

	typedef boost::function<void (bool)> callback_t;

	/**
	 * Constructor.
	 *
	 * @param count - how many times callback should be called (twice to not change original state)
	 * @param period - how frequently callback should be called
	 * @param cb - callback to be called each tick
	 */
	LLFlashTimer(callback_t cb, S32 count = 0, F32 period = 0.0);
	~LLFlashTimer() {};

	/*virtual*/ BOOL tick();

	void startFlashing();
	void stopFlashing();

private:
	callback_t		mCallback;
	/**
	 * How many times Well will blink.
	 */
	S32 mFlashCount;
	S32 mCurrentTickCount;
};

#endif /* LL_FLASHTIMER_H */
