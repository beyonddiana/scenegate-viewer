/** 
 * @file llfloaterexperiences.cpp
 * @brief LLFloaterExperiences class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llpanelexperiences.h"
#include "llfloaterexperiences.h"
#include "llagent.h"
#include "llfloaterregioninfo.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llexperiencecache.h"
#include "llevents.h"


class LLExperienceListResponder : public LLHTTPClient::Responder
{
public:
    typedef std::map<std::string, std::string> NameMap;
    LLExperienceListResponder(const LLHandle<LLFloaterExperiences>& parent, NameMap& nameMap):mParent(parent)
    {
        mNameMap.swap(nameMap);
    }

    LLHandle<LLFloaterExperiences> mParent;
    NameMap mNameMap;

    virtual void result(const LLSD& content)
    {
        if(mParent.isDead())
            return;

        LLFloaterExperiences* parent=mParent.get();
        LLTabContainer* tabs = parent->getChild<LLTabContainer>("xp_tabs");
 
        NameMap::iterator it = mNameMap.begin();
        while(it != mNameMap.end())
        {
            if(content.has(it->first))
            {
                LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName(it->second);
                if(tab)
                {
                    const LLSD& ids = content[it->first];
                    tab->setExperienceList(ids);
                    //parent->clearFromRecent(ids);
                }
            }
            ++it;
        }
    }
};



LLFloaterExperiences::LLFloaterExperiences(const LLSD& data)
	:LLFloater(data)
{
}

void LLFloaterExperiences::addTab(const std::string& name, bool select)
{
    getChild<LLTabContainer>("xp_tabs")->addTabPanel(LLTabContainer::TabPanelParams().
        panel(LLPanelExperiences::create(name)).
        label(LLTrans::getString(name)).
        select_tab(select));
}

BOOL LLFloaterExperiences::postBuild()
{
    addTab("Allowed_Experiences_Tab", true);
    addTab("Blocked_Experiences_Tab", false);
    addTab("Admin_Experiences_Tab", false);
    addTab("Contrib_Experiences_Tab", false);
    addTab("Recent_Experiences_Tab", false);
    resizeToTabs();

   
    LLEventPumps::instance().obtain("experience_permission").listen("LLFloaterExperiences", 
        boost::bind(&LLFloaterExperiences::updatePermissions, this, _1));
     
   	return TRUE;
}

void LLFloaterExperiences::clearFromRecent(const LLSD& ids)
{
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");

    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Recent_Experiences_Tab");
    if(!tab)
        return;

    tab->removeExperiences(ids);
}

void LLFloaterExperiences::setupRecentTabs()
{
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");

    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Recent_Experiences_Tab");
    if(!tab)
        return;

    LLSD recent;

    const LLExperienceCache::cache_t& experiences = LLExperienceCache::getCached();

    LLExperienceCache::cache_t::const_iterator it = experiences.begin();
    while( it != experiences.end() )
    {
        if(!it->second.has(LLExperienceCache::MISSING))
        {
            recent.append(it->first);
        }
        ++it;
    }

    tab->setExperienceList(recent);
}

void LLFloaterExperiences::resizeToTabs()
{
    const S32 TAB_WIDTH_PADDING = 16;

    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
    LLRect rect = getRect();
    if(rect.getWidth() < tabs->getTotalTabWidth() + TAB_WIDTH_PADDING)
    {
        rect.mRight = rect.mLeft + tabs->getTotalTabWidth() + TAB_WIDTH_PADDING;
    }
    reshape(rect.getWidth(), rect.getHeight(), FALSE);
}

void LLFloaterExperiences::refreshContents()
{
    setupRecentTabs();

    LLViewerRegion* region = gAgent.getRegion();

    if (region)
    {
        LLExperienceListResponder::NameMap nameMap;
        std::string lookup_url=region->getCapability("GetExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experiences"]="Allowed_Experiences_Tab";
            nameMap["blocked"]="Blocked_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }

        lookup_url = region->getCapability("GetAdminExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experience_ids"]="Admin_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }

        lookup_url = region->getCapability("GetCreatorExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experience_ids"]="Contrib_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }
    }
}

void LLFloaterExperiences::onOpen( const LLSD& key )
{
    LLViewerRegion* region = gAgent.getRegion();
    if(region)
    {
        if(region->capabilitiesReceived())
        {
            refreshContents();
            return;
        }
        region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterExperiences::refreshContents, this));
        return;
    }
}

bool LLFloaterExperiences::updatePermissions( const LLSD& permission )
{
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
    LLUUID experience;
    std::string permission_string;
    if(permission.has("experience"))
    {
        experience = permission["experience"].asUUID();
        permission_string = permission[experience.asString()]["permission"].asString();

    }
    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Allowed_Experiences_Tab");
    if(tab)
    {
        if(permission.has("experiences"))
        {
            tab->setExperienceList(permission["experiences"]);
        }
        else if(experience.notNull())
        {
            if(permission_string != "Allow")
            {
                tab->removeExperience(experience);
            }
            else
            {
                tab->addExperience(experience);
            }
        }
    }
    
    tab = (LLPanelExperiences*)tabs->getPanelByName("Blocked_Experiences_Tab");
    if(tab)
    {
        if(permission.has("blocked"))
        {
            tab->setExperienceList(permission["blocked"]);
        }
        else if(experience.notNull())
        {
            if(permission_string != "Block")
            {
                tab->removeExperience(experience);
            }
            else
            {
                tab->addExperience(experience);
            }
        }
    }
    return false;
}

void LLFloaterExperiences::onClose( bool app_quitting )
{
    LLEventPumps::instance().obtain("experience_permission").stopListening("LLFloaterExperiences");
    LLFloater::onClose(app_quitting);
}
