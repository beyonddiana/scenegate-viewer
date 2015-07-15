/*
 * @file llstreaminfo.cpp
 * @brief Class enables display of audio stream metadata
 *
 * Copyright (c) 2014, Cinder Roxley <cinder@sdf.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "llviewerprecompiledheaders.h"
#include "llstreaminfo.h"

#include "llaudioengine.h"
#include "llnotificationsutil.h"
#include "llstreamingaudio.h"
#include "lltrans.h"
#include "llurlaction.h"
#include "llviewercontrol.h" // LLCachedControl

#include "llfloater.h"
#include "llfloaterreg.h"

LLStreamInfo::LLStreamInfo()
:	LLEventTimer(1.f)
{}

BOOL LLStreamInfo::tick()
{
	static LLCachedControl<bool> show_stream_info(gSavedSettings, "ShowStreamInfo", false);
	if (!show_stream_info) return FALSE;

	LLFloater* music_ticker = LLFloaterReg::findInstance("music_ticker");
	if (music_ticker)
		return FALSE;
	
	if (!gAudiop)
		return FALSE;

	LLStreamingAudioInterface *stream = gAudiop->getStreamingAudioImpl();
	if (!stream || !stream->getMetaData() || !stream->hasNewMetaData()) 
		return FALSE;
	
	const LLSD& data = *(stream->getMetaData());
	if (data)
	{
		std::string station = data.has("icy-name") ? data["icy-name"].asString() : LLTrans::getString("NowPlaying");
		// Some stations get a little ridiculous with the length.
		if (station.length() > 64)
		{
			LLStringUtil::truncate(station, 64);
			station.append("...");
		}
		LLSD args;
		LLSD payload;
		args["STATION"] = station;
		std::stringstream info;
		if (data.has("TITLE"))
			info << data["TITLE"].asString();
		if (data.has("TITLE") && data.has("ARTIST"))
			info << "\n";
		if (data.has("ARTIST"))
			info << data["ARTIST"].asString();
		args["INFO"] = info.str();
		if (data.has("icy-url"))
			LLNotificationsUtil::add("StreamInfo", args,
									 LLSD().with("respond_on_mousedown", TRUE),
									 boost::bind(&LLUrlAction::openURL, data["icy-url"].asString()));
		else
			LLNotificationsUtil::add("StreamInfo", args);
	}
	
	return FALSE;
}
