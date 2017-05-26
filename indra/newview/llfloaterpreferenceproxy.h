/**
* @file llfloaterpreferenceproxy.h
* @brief LLFloaterPreferenceProxy class definition
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/*
* App-wide preferences.  Note that these are not per-user,
* because we need to load many preferences before we have
* a login name.
*/

#ifndef LL_LLFLOATERPREFERENCEPROXY_H
#define LL_LLFLOATERPREFERENCEPROXY_H

#include "llsd.h"
#include "llfloater.h"

#include <map>

class LLFloaterPreferenceProxy : public LLFloater
{
public:
	LLFloaterPreferenceProxy(const LLSD& key);
	~LLFloaterPreferenceProxy();

	/// show off our menu
	static void show();
	void cancel();

protected:
	BOOL postBuild() override;
	void onOpen(const LLSD& key) override;
	void onClose(bool app_quitting) override;
	void saveSettings();
	void onBtnOk();
	void onBtnCancel();
	void onClickCloseBtn(bool app_quitting = false) override;

	void onChangeSocksSettings();

private:

	bool mSocksSettingsDirty;
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
	LOG_CLASS(LLFloaterPreferenceProxy);
};

#endif // LL_LLFLOATERPREFERENCEPROXY_H