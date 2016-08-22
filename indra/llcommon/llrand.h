/** 
 * @file llrand.h
 * @brief Information, functions, and typedefs for randomness.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLRAND_H
#define LL_LLRAND_H

/**
 * Use the boost random number generators if you want a stateful
 * random numbers. If you want more random numbers, use the
 * c-functions since they will generate faster/better randomness
 * across the process.
 *
 * I tested some of the boost random engines, and picked a good double
 * generator and a good integer generator. I also took some timings
 * for them on linux using gcc 3.3.5. The harness also did some other
 * fairly trivial operations to try to limit compiler optimizations,
 * so these numbers are only good for relative comparisons.
 *
 * usec/inter		algorithm
 * 0.21				boost::minstd_rand0
 * 0.039			boost:lagged_fibonacci19937
 * 0.036			boost:lagged_fibonacci607
 * 0.44				boost::hellekalek1995
 * 0.44				boost::ecuyer1988
 * 0.042			boost::rand48
 * 0.043			boost::mt11213b
 * 0.028			stdlib random() 
 * 0.05				stdlib lrand48()
 * 0.034			stdlib rand()
 * 0.020			the old & lame LLRand
 */

/**
 *@brief Generate a float from [0, RAND_MAX).
 */
S32 LL_COMMON_API ll_rand();

/**
 *@brief Generate a float from [0, val) or (val, 0].
 */
S32 LL_COMMON_API ll_rand(S32 val);

/**
 *@brief Generate a float from [0, 1.0).
 */
F32 LL_COMMON_API ll_frand();

/**
 *@brief Generate a float from [0, val) or (val, 0].
 */
F32 LL_COMMON_API ll_frand(F32 val);

/**
 *@brief Generate a double from [0, 1.0).
 */
F64 LL_COMMON_API ll_drand();

/**
 *@brief Generate a double from [0, val) or (val, 0].
 */
F64 LL_COMMON_API ll_drand(F64 val);

#endif
