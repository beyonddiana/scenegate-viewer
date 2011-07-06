/** 
 * @file llsys.h
 * @brief System information debugging classes.
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

#ifndef LL_SYS_H
#define LL_SYS_H

//
// The LLOSInfo, LLCPUInfo, and LLMemoryInfo classes are essentially
// the same, but query different machine subsystems. Here's how you
// use an LLCPUInfo object:
//
//  LLCPUInfo info;
//  llinfos << info << llendl;
//

#include "llsd.h"
#include <iosfwd>
#include <string>

class LL_COMMON_API LLOSInfo
{
public:
	LLOSInfo();
	void stream(std::ostream& s) const;

	const std::string& getOSString() const;
	const std::string& getOSStringSimple() const;

	S32 mMajorVer;
	S32 mMinorVer;
	S32 mBuild;

#ifndef LL_WINDOWS
	static S32 getMaxOpenFiles();
#endif

	static U32 getProcessVirtualSizeKB();
	static U32 getProcessResidentSizeKB();
private:
	std::string mOSString;
	std::string mOSStringSimple;
};


class LL_COMMON_API LLCPUInfo
{
public:
	LLCPUInfo();	
	void stream(std::ostream& s) const;

	std::string getCPUString() const;

	bool hasAltivec() const;
	bool hasSSE() const;
	bool hasSSE2() const;
	F64 getMHz() const;

	// Family is "AMD Duron" or "Intel Pentium Pro"
	const std::string& getFamily() const { return mFamily; }

private:
	bool mHasSSE;
	bool mHasSSE2;
	bool mHasAltivec;
	F64 mCPUMHz;
	std::string mFamily;
	std::string mCPUString;
};

//=============================================================================
//
//	CLASS		LLMemoryInfo

class LL_COMMON_API LLMemoryInfo

/*!	@brief		Class to query the memory subsystem

	@details
		Here's how you use an LLMemoryInfo:
		
		LLMemoryInfo info;
<br>	llinfos << info << llendl;
*/
{
public:
	LLMemoryInfo(); ///< Default constructor
	void stream(std::ostream& s) const;	///< output text info to s

	U32 getPhysicalMemoryKB() const; ///< Memory size in KiloBytes
	
	/*! Memory size in bytes, if total memory is >= 4GB then U32_MAX will
	**  be returned.
	*/
	U32 getPhysicalMemoryClamped() const; ///< Memory size in clamped bytes

	//get the available memory infomation in KiloBytes.
	static void getAvailableMemoryKB(U32& avail_physical_mem_kb, U32& avail_virtual_mem_kb);

	// Retrieve a map of memory statistics. The keys of the map are platform-
	// dependent. The values are in kilobytes.
	LLSD getStatsMap() const;

	// Retrieve memory statistics: an array of pair arrays [name, value]. This
	// is the same data as presented in getStatsMap(), but it preserves the
	// order in which we retrieved it from the OS in case that's useful. The
	// set of statistics names is platform-dependent. The values are in
	// kilobytes to try to avoid integer overflow.
	LLSD getStatsArray() const;

	// Re-fetch memory data (as reported by stream() and getStats*()) from the
	// system. Normally this is fetched at construction time. Return (*this)
	// to permit usage of the form:
	// @code
	// LLMemoryInfo info;
	// ...
	// info.refresh().getStatsArray();
	// @endcode
	LLMemoryInfo& refresh();

private:
	// These methods are used to set mStatsArray and mStatsMap.
	static LLSD loadStatsArray();
	static LLSD loadStatsMap(const LLSD&);

	// Memory stats for getStatsArray(). It's straightforward to convert that
	// to getStatsMap() form, less so to reconstruct the original order when
	// converting the other way.
	LLSD mStatsArray;
	// Memory stats for getStatsMap().
	LLSD mStatsMap;
};


LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLOSInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info);

// gunzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gunzip_file(const std::string& srcfile, const std::string& dstfile);
// gzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gzip_file(const std::string& srcfile, const std::string& dstfile);

extern LL_COMMON_API LLCPUInfo gSysCPU;

#endif // LL_LLSYS_H
