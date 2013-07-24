/** 
 * @file lltextureinfo.h
 * @brief Object for managing texture information.
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

#ifndef LL_LLTEXTUREINFO_H
#define LL_LLTEXTUREINFO_H

#include "lluuid.h"
#include "lltextureinfodetails.h"
#include <map>

class LLTextureInfo
{
public:
	LLTextureInfo();
	~LLTextureInfo();

	void setUpLogging(bool writeToViewerLog, bool sendToSim, U32 textureLogThreshold);
	bool has(const LLUUID& id);
	void setRequestStartTime(const LLUUID& id, U64 startTime);
	void setRequestSize(const LLUUID& id, U32 size);
	void setRequestOffset(const LLUUID& id, U32 offset);
	void setRequestType(const LLUUID& id, LLTextureInfoDetails::LLRequestType type);
	void setRequestCompleteTimeAndLog(const LLUUID& id, U64 completeTime);
	U32 getRequestStartTime(const LLUUID& id);
	U32 getRequestSize(const LLUUID& id);
	U32 getRequestOffset(const LLUUID& id);
	LLTextureInfoDetails::LLRequestType getRequestType(const LLUUID& id);
	U32 getRequestCompleteTime(const LLUUID& id);
	void resetTextureStatistics();
	U32 getTextureInfoMapSize();
	LLSD getAverages();

private:
	void addRequest(const LLUUID& id);

	std::map<LLUUID, LLTextureInfoDetails *> mTextures;

	LLSD mAverages;

	bool mLogTextureDownloadsToViewerLog;
	bool mLogTextureDownloadsToSimulator;
	S32 mTotalBytes;
	S32 mTotalMilliseconds;
	S32 mTextureDownloadsStarted;
	S32 mTextureDownloadsCompleted;
	std::string mTextureDownloadProtocol;
	U32 mTextureLogThreshold; // in bytes
	U64 mCurrentStatsBundleStartTime;
};

#endif // LL_LLTEXTUREINFO_H
