/** 
 * @file llsyntaxid.h
 * @brief Contains methods to access the LSLSyntaxId feature and LSLSyntax capability
 * to use the appropriate syntax file for the current region's LSL version.
 * @author Ima Mechanique, Cinder Roxley
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_SYNTAXID_H
#define LL_SYNTAXID_H

#include "llviewerprecompiledheaders.h"

#include "llsingleton.h"

class fetchKeywordsFileResponder;

class LLSyntaxIdLSL : public LLSingleton<LLSyntaxIdLSL>
{
	friend class LLSingleton<LLSyntaxIdLSL>;
	friend class fetchKeywordsFileResponder;
	
private:
	std::list<std::string> mInflightFetches;
	typedef boost::signals2::signal<void()> syntax_id_changed_signal_t;
	syntax_id_changed_signal_t mSyntaxIDChangedSignal;
	boost::signals2::connection mRegionChangedCallback;
	
	bool	syntaxIdChanged();
	bool	isSupportedVersion(const LLSD& content);
	void	handleRegionChanged();
	void	handleCapsReceived(const LLUUID& region_uuid);
	void	handleFileFetched(const std::string& filepath);
	void	setKeywordsXml(const LLSD& content) { mKeywordsXml = content; };
	void	buildFullFileSpec();
	void	fetchKeywordsFile(const std::string& filespec);
	void	loadDefaultKeywordsIntoLLSD();
	void	loadKeywordsIntoLLSD();
	
	std::string		mCapabilityURL;
	std::string		mFullFileSpec;
	ELLPath			mFilePath;
	LLUUID			mSyntaxId;
	LLSD			mKeywordsXml;
	
public:
	LLSyntaxIdLSL();
	void initialize();
	bool keywordFetchInProgress();
	LLSD getKeywordsXML() const { return mKeywordsXml; };
	boost::signals2::connection addSyntaxIDCallback(const syntax_id_changed_signal_t::slot_type& cb);
};

#endif // LLSYNTAXID_H
