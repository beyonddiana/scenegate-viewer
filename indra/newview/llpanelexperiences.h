/** 
 * @file llpanelpicks.h
 * @brief LLPanelPicks and related class definitions
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_LLPANELEXPERIENCES_H
#define LL_LLPANELEXPERIENCES_H

#include "llaccordionctrltab.h"
#include "llflatlistview.h"
#include "llpanelavatar.h"

class LLExperienceItem;
class LLPanelProfile; 


class LLPanelSearchExperiences 
    : public LLPanel
{
public:
    LLPanelSearchExperiences(){}
    static LLPanelSearchExperiences* create(const std::string& name);
    /*virtual*/ BOOL postBuild(void);

    void doSearch();
};

class LLPanelExperiences
	: public LLPanel 
{
public:
    LLPanelExperiences();

    static LLPanelExperiences* create(const std::string& name);

	/*virtual*/ BOOL postBuild(void);
	/*virtual*/ void onClosePanel();

    void setExperienceList(const LLSD& experiences);

    LLExperienceItem* getSelectedExperienceItem();
    void removeExperiences( const LLSD& ids );
protected:

private:
	LLFlatListView* mExperiencesList;
};


class LLExperienceItem 
	: public LLPanel
{
public:
	LLExperienceItem();
	~LLExperienceItem();

	void init(const LLUUID& experience_id);
protected:
};
#endif // LL_LLPANELEXPERIENCES_H
