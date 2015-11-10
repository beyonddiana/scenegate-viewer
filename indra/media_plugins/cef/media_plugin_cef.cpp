/**
* @file media_plugin_cef.cpp
* @brief CEF (Chromium Embedding Framework) plugin for LLMedia API plugin system
*
* @cond
* $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
* @endcond
*/

#include "linden_common.h"
#include "indra_constants.h" // for indra keyboard codes

#include "llgl.h"
#include "llsdutil.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "llCEFLib.h"
#include "volume_catcher.h"

////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginCEF :
	public MediaPluginBase
{
public:
	MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginCEF();

	/*virtual*/
	void receiveMessage(const char* message_string);

private:
	bool init();

	void onPageChangedCallback(unsigned char* pixels, int width, int height);
	void onCustomSchemeURLCallback(std::string url);
	void onConsoleMessageCallback(std::string message, std::string source, int line);
	void onStatusMessageCallback(std::string value);
	void onTitleChangeCallback(std::string title);
	void onLoadStartCallback();
	void onLoadEndCallback(int httpStatusCode);
	void onAddressChangeCallback(std::string url);
	void onNavigateURLCallback(std::string url, std::string target);
	bool onHTTPAuthCallback(const std::string host, const std::string realm, std::string& username, std::string& password);
	void onCursorChangedCallback(LLCEFLib::ECursorType type, unsigned int handle);

	void postDebugMessage(const std::string& msg);
	void authResponse(LLPluginMessage &message);

	LLCEFLib::EKeyboardModifier decodeModifiers(std::string &modifiers);
	void deserializeKeyboardData(LLSD native_key_data, uint32_t& native_scan_code, uint32_t& native_virtual_key, uint32_t& native_modifiers);
	void keyEvent(LLCEFLib::EKeyEvent key_event, int key, LLCEFLib::EKeyboardModifier modifiers, LLSD native_key_data);
	void unicodeInput(const std::string &utf8str, LLCEFLib::EKeyboardModifier modifiers, LLSD native_key_data);

	void checkEditState();
    void setVolume(F32 vol);

	bool mEnableMediaPluginDebugging;
	std::string mHostLanguage;
	bool mCookiesEnabled;
	bool mPluginsEnabled;
	bool mJavascriptEnabled;
	std::string mUserAgentSubtring;
	std::string mAuthUsername;
	std::string mAuthPassword;
	bool mAuthOK;
	bool mCanCut;
	bool mCanCopy;
	bool mCanPaste;
	std::string mCachePath;
	std::string mCookiePath;
	LLCEFLib* mLLCEFLib;

    VolumeCatcher mVolumeCatcher;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data) :
MediaPluginBase(host_send_func, host_user_data)
{
	mWidth = 0;
	mHeight = 0;
	mDepth = 4;
	mPixels = 0;
	mEnableMediaPluginDebugging = true;
	mHostLanguage = "en";
	mCookiesEnabled = true;
	mPluginsEnabled = false;
	mJavascriptEnabled = true;
	mUserAgentSubtring = "";
	mAuthUsername = "";
	mAuthPassword = "";
	mAuthOK = false;
	mCanCut = false;
	mCanCopy = false;
	mCanPaste = false;
	mCachePath = "";
	mCookiePath = "";
	mLLCEFLib = new LLCEFLib();
}

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::~MediaPluginCEF()
{
	mLLCEFLib->requestExit();
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::postDebugMessage(const std::string& msg)
{
	if (mEnableMediaPluginDebugging)
	{
		std::stringstream str;
		str << "@Media Msg> " << msg;

		LLPluginMessage debug_message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "debug_message");
		debug_message.setValue("message_text", str.str());
		debug_message.setValue("message_level", "info");
		sendMessage(debug_message);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onPageChangedCallback(unsigned char* pixels, int width, int height)
{
	if (mPixels && pixels)
	{
		if (mWidth == width && mHeight == height)
		{
			memcpy(mPixels, pixels, mWidth * mHeight * mDepth);
		}
		setDirty(0, 0, mWidth, mHeight);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onConsoleMessageCallback(std::string message, std::string source, int line)
{
	std::stringstream str;
	str << "Console message: " << message << " in file(" << source << ") at line " << line;
	postDebugMessage(str.str());
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onStatusMessageCallback(std::string value)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "status_text");
	message.setValue("status", value);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onTitleChangeCallback(std::string title)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", title);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onLoadStartCallback()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_begin");
	//message.setValue("uri", event.getEventUri());  // not easily available here in CEF - needed?
	message.setValueBoolean("history_back_available", mLLCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available", mLLCEFLib->canGoForward());
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onLoadEndCallback(int httpStatusCode)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_complete");
	//message.setValue("uri", event.getEventUri());  // not easily available here in CEF - needed?
	message.setValueS32("result_code", httpStatusCode);
	message.setValueBoolean("history_back_available", mLLCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available", mLLCEFLib->canGoForward());
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onAddressChangeCallback(std::string url)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "location_changed");
	message.setValue("uri", url);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onNavigateURLCallback(std::string url, std::string target)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "click_href");
	message.setValue("uri", url);
	message.setValue("target", target);
	message.setValue("uuid", "");	// not used right now
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onCustomSchemeURLCallback(std::string url)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "click_nofollow");
	message.setValue("uri", url);
	message.setValue("nav_type", "clicked");	// TODO: differentiate between click and navigate to
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::onHTTPAuthCallback(const std::string host, const std::string realm, std::string& username, std::string& password)
{
	mAuthOK = false;

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "auth_request");
	message.setValue("url", host);
	message.setValue("realm", realm);
	message.setValueBoolean("blocking_request", true);

	// The "blocking_request" key in the message means this sendMessage call will block until a response is received.
	sendMessage(message);

	if (mAuthOK)
	{
		username = mAuthUsername;
		password = mAuthPassword;
	}

	return mAuthOK;
}

void MediaPluginCEF::onCursorChangedCallback(LLCEFLib::ECursorType type, unsigned int handle)
{
	std::string name = "";

	switch (type)
	{
		case LLCEFLib::CT_POINTER:
			name = "arrow";
			break;
		case LLCEFLib::CT_IBEAM:
			name = "ibeam";
			break;
		case LLCEFLib::CT_NORTHSOUTHRESIZE:
			name = "splitv";
			break;
		case LLCEFLib::CT_EASTWESTRESIZE:
			name = "splith";
			break;
		case LLCEFLib::CT_HAND:
			name = "hand";
			break;

		default:
			LL_WARNS() << "Unknown cursor ID: " << (int)type << LL_ENDL;
			break;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "cursor_changed");
	message.setValue("name", name);
	sendMessage(message);
}

void MediaPluginCEF::authResponse(LLPluginMessage &message)
{
	mAuthOK = message.getValueBoolean("ok");
	if (mAuthOK)
	{
		mAuthUsername = message.getValue("username");
		mAuthPassword = message.getValue("password");
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::receiveMessage(const char* message_string)
{
	//  std::cerr << "MediaPluginWebKit::receiveMessage: received message: \"" << message_string << "\"" << std::endl;
	LLPluginMessage message_in;

	if (message_in.parse(message_string) >= 0)
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();
		if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
		{
			if (message_name == "init")
			{
				LLPluginMessage message("base", "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
				message.setValueLLSD("versions", versions);

				std::string plugin_version = "CEF plugin 1.0.0";
				message.setValue("plugin_version", plugin_version);
				sendMessage(message);
			}
			else if (message_name == "idle")
			{
				mLLCEFLib->update();

                mVolumeCatcher.pump();
				// this seems bad but unless the state changes (it won't until we figure out
				// how to get CEF to tell us if copy/cut/paste is available) then this function
				// will return immediately
				checkEditState();
			}
			else if (message_name == "cleanup")
			{
			}
			else if (message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

			}
			else if (message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if (iter != mSharedSegments.end())
				{
					if (mPixels == iter->second.mAddress)
					{
						mPixels = NULL;
						mTextureSegmentName.clear();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
					//std::cerr << "MediaPluginWebKit::receiveMessage: unknown shared memory region!" << std::endl;
				}

				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
				//std::cerr << "MediaPluginWebKit::receiveMessage: unknown base message: " << message_name << std::endl;
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if (message_name == "init")
			{
				// event callbacks from LLCefLib
				mLLCEFLib->setOnPageChangedCallback(boost::bind(&MediaPluginCEF::onPageChangedCallback, this, _1, _2, _3));
				mLLCEFLib->setOnCustomSchemeURLCallback(boost::bind(&MediaPluginCEF::onCustomSchemeURLCallback, this, _1));
				mLLCEFLib->setOnConsoleMessageCallback(boost::bind(&MediaPluginCEF::onConsoleMessageCallback, this, _1, _2, _3));
				mLLCEFLib->setOnStatusMessageCallback(boost::bind(&MediaPluginCEF::onStatusMessageCallback, this, _1));
				mLLCEFLib->setOnTitleChangeCallback(boost::bind(&MediaPluginCEF::onTitleChangeCallback, this, _1));
				mLLCEFLib->setOnLoadStartCallback(boost::bind(&MediaPluginCEF::onLoadStartCallback, this));
				mLLCEFLib->setOnLoadEndCallback(boost::bind(&MediaPluginCEF::onLoadEndCallback, this, _1));
				mLLCEFLib->setOnAddressChangeCallback(boost::bind(&MediaPluginCEF::onAddressChangeCallback, this, _1));
				mLLCEFLib->setOnNavigateURLCallback(boost::bind(&MediaPluginCEF::onNavigateURLCallback, this, _1, _2));
				mLLCEFLib->setOnHTTPAuthCallback(boost::bind(&MediaPluginCEF::onHTTPAuthCallback, this, _1, _2, _3, _4));
				mLLCEFLib->setOnCursorChangedCallback(boost::bind(&MediaPluginCEF::onCursorChangedCallback, this, _1, _2));

				LLCEFLib::LLCEFLibSettings settings;
				settings.initial_width = 1024;
				settings.initial_height = 1024;
				settings.plugins_enabled = mPluginsEnabled;
				settings.javascript_enabled = mJavascriptEnabled;
				settings.cookies_enabled = mCookiesEnabled;
				settings.cookie_store_path = mCookiePath;
				settings.cache_enabled = true;
				settings.cache_path = mCachePath;
				settings.accept_language_list = mHostLanguage;
				settings.user_agent_substring = mUserAgentSubtring;

				bool result = mLLCEFLib->init(settings);
				if (!result)
				{
					// if this fails, the media system in viewer will put up a message
				}

				// Plugin gets to decide the texture parameters to use.
				mDepth = 4;
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				message.setValueS32("default_width", 1024);
				message.setValueS32("default_height", 1024);
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueU32("format", GL_BGRA);
				message.setValueU32("type", GL_UNSIGNED_BYTE);
				message.setValueBoolean("coords_opengl", true);
				sendMessage(message);
			}
			else if (message_name == "set_user_data_path")
			{
				std::string user_data_path_cache = message_in.getValue("cache_path");
				std::string user_data_path_cookies = message_in.getValue("cookies_path");
				mCachePath = user_data_path_cache + "cef_cache";
				mCookiePath = user_data_path_cookies + "cef_cookies";
			}
			else if (message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				if (!name.empty())
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find(name);
					if (iter != mSharedSegments.end())
					{
						mPixels = (unsigned char*)iter->second.mAddress;
						mWidth = width;
						mHeight = height;

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;
					};
				};

				mLLCEFLib->setSize(mWidth, mHeight);

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);

			}
			else if (message_name == "set_language_code")
			{
				mHostLanguage = message_in.getValue("language");
			}
			else if (message_name == "load_uri")
			{
				std::string uri = message_in.getValue("uri");
				mLLCEFLib->navigate(uri);
			}
			else if (message_name == "set_cookie")
			{
				std::string uri = message_in.getValue("uri");
				std::string name = message_in.getValue("name");
				std::string value = message_in.getValue("value");
				std::string domain = message_in.getValue("domain");
				std::string path = message_in.getValue("path");
				mLLCEFLib->setCookie(uri, name, value, domain, path);
			}
			else if (message_name == "mouse_event")
			{
				std::string event = message_in.getValue("event");

				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");

				//std::string modifiers = message_in.getValue("modifiers");

				S32 button = message_in.getValueS32("button");
				LLCEFLib::EMouseButton btn = LLCEFLib::MB_MOUSE_BUTTON_LEFT;
				if (button == 0) btn = LLCEFLib::MB_MOUSE_BUTTON_LEFT;
				if (button == 1) btn = LLCEFLib::MB_MOUSE_BUTTON_RIGHT;
				if (button == 2) btn = LLCEFLib::MB_MOUSE_BUTTON_MIDDLE;

				if (event == "down")
				{
					mLLCEFLib->mouseButton(btn, LLCEFLib::ME_MOUSE_DOWN, x, y);
					mLLCEFLib->setFocus(true);

					std::stringstream str;
					str << "Mouse down at = " << x << ", " << y;
					postDebugMessage(str.str());
				}
				else if (event == "up")
				{
					mLLCEFLib->mouseButton(btn, LLCEFLib::ME_MOUSE_UP, x, y);

					std::stringstream str;
					str << "Mouse up at = " << x << ", " << y;
					postDebugMessage(str.str());
				}
				else if (event == "double_click")
				{
					// TODO: do we need this ?
				}
				else
				{
					mLLCEFLib->mouseMove(x, y);
				}
			}
			else if (message_name == "scroll_event")
			{
				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");
				const int scaling_factor = 40;
				y *= -scaling_factor;

				mLLCEFLib->mouseWheel(x, y);
			}
			else if (message_name == "text_event")
			{
				std::string text = message_in.getValue("text");
				std::string modifiers = message_in.getValue("modifiers");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");

				unicodeInput(text, decodeModifiers(modifiers), native_key_data);
			}
			else if (message_name == "key_event")
			{
#if LL_DARWIN
				std::string event = message_in.getValue("event");
				S32 key = message_in.getValueS32("key");
                LLSD native_key_data = message_in.getValueLLSD("native_key_data");

#if 0
				if (event == "down")
				{
					//mLLCEFLib->keyPress(key, true);
					mLLCEFLib->keyboardEvent(LLCEFLib::KE_KEY_DOWN, (uint32_t)key, 0, LLCEFLib::KM_MODIFIER_NONE, 0, 0, 0);

				}
				else if (event == "up")
				{
					//mLLCEFLib->keyPress(key, false);
					mLLCEFLib->keyboardEvent(LLCEFLib::KE_KEY_UP, (uint32_t)key, 0, LLCEFLib::KM_MODIFIER_NONE, 0, 0, 0);
				}
#else
                // Treat unknown events as key-up for safety.
                LLCEFLib::EKeyEvent key_event = LLCEFLib::KE_KEY_UP;
                if (event == "down")
                {
                    key_event = LLCEFLib::KE_KEY_DOWN;
                }
                else if (event == "repeat")
                {
                    key_event = LLCEFLib::KE_KEY_REPEAT;
                }
                
                keyEvent(key_event, key, LLCEFLib::KM_MODIFIER_NONE, native_key_data);
              
#endif
#elif LL_WINDOWS
				std::string event = message_in.getValue("event");
				S32 key = message_in.getValueS32("key");
				std::string modifiers = message_in.getValue("modifiers");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");

				// Treat unknown events as key-up for safety.
				LLCEFLib::EKeyEvent key_event = LLCEFLib::KE_KEY_UP;
				if (event == "down")
				{
					key_event = LLCEFLib::KE_KEY_DOWN;
				}
				else if (event == "repeat")
				{
					key_event = LLCEFLib::KE_KEY_REPEAT;
				}

				keyEvent(key_event, key, decodeModifiers(modifiers), native_key_data);
#endif
			}
			else if (message_name == "enable_media_plugin_debugging")
			{
				mEnableMediaPluginDebugging = message_in.getValueBoolean("enable");
			}
			if (message_name == "auth_response")
			{
				authResponse(message_in);
			}
			if (message_name == "edit_cut")
			{
				mLLCEFLib->editCut();
			}
			if (message_name == "edit_copy")
			{
				mLLCEFLib->editCopy();
			}
			if (message_name == "edit_paste")
			{
				mLLCEFLib->editPaste();
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER)
		{
			if (message_name == "set_page_zoom_factor")
			{
				F32 factor = (F32)message_in.getValueReal("factor");
				mLLCEFLib->setPageZoom(factor);
			}
			if (message_name == "browse_stop")
			{
				mLLCEFLib->stop();
			}
			else if (message_name == "browse_reload")
			{
				bool ignore_cache = true;
				mLLCEFLib->reload(ignore_cache);
			}
			else if (message_name == "browse_forward")
			{
				mLLCEFLib->goForward();
			}
			else if (message_name == "browse_back")
			{
				mLLCEFLib->goBack();
			}
			else if (message_name == "cookies_enabled")
			{
				mCookiesEnabled = message_in.getValueBoolean("enable");
			}
			else if (message_name == "set_user_agent")
			{
				mUserAgentSubtring = message_in.getValue("user_agent");
			}
			else if (message_name == "plugins_enabled")
			{
				mPluginsEnabled = message_in.getValueBoolean("enable");
			}
			else if (message_name == "javascript_enabled")
			{
				mJavascriptEnabled = message_in.getValueBoolean("enable");
			}
		}
        else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
        {
            if (message_name == "set_volume")
            {
                F32 volume = (F32)message_in.getValueReal("volume");
                setVolume(volume);
            }
        }
        else
		{
			//std::cerr << "MediaPluginWebKit::receiveMessage: unknown message class: " << message_class << std::endl;
		};
	}
}

LLCEFLib::EKeyboardModifier MediaPluginCEF::decodeModifiers(std::string &modifiers)
{
	int result = 0;

	if (modifiers.find("shift") != std::string::npos)
		result |= LLCEFLib::KM_MODIFIER_SHIFT;

	if (modifiers.find("alt") != std::string::npos)
		result |= LLCEFLib::KM_MODIFIER_ALT;

	if (modifiers.find("control") != std::string::npos)
		result |= LLCEFLib::KM_MODIFIER_CONTROL;

	if (modifiers.find("meta") != std::string::npos)
		result |= LLCEFLib::KM_MODIFIER_META;

	return (LLCEFLib::EKeyboardModifier)result;
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::deserializeKeyboardData(LLSD native_key_data, uint32_t& native_scan_code, uint32_t& native_virtual_key, uint32_t& native_modifiers)
{
	native_scan_code = 0;
	native_virtual_key = 0;
	native_modifiers = 0;

	if (native_key_data.isMap())
	{
#if LL_DARWIN
		native_scan_code = (uint32_t)(native_key_data["char_code"].asInteger());
		native_virtual_key = (uint32_t)(native_key_data["key_code"].asInteger());
		native_modifiers = (uint32_t)(native_key_data["modifiers"].asInteger());
#elif LL_WINDOWS
		native_scan_code = (uint32_t)(native_key_data["scan_code"].asInteger());
		native_virtual_key = (uint32_t)(native_key_data["virtual_key"].asInteger());
		// TODO: I don't think we need to do anything with native modifiers here -- please verify
#endif
	};
};

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::keyEvent(LLCEFLib::EKeyEvent key_event, int key, LLCEFLib::EKeyboardModifier modifiers_x, LLSD native_key_data = LLSD::emptyMap())
{
#if LL_DARWIN
	std::string utf8_text;

    uint32_t native_char_code = native_key_data["char_code"].asInteger();
    uint32_t native_scan_code = native_key_data["scan_code"].asInteger();
    uint32_t native_virtual_key = native_key_data["key_code"].asInteger();
    uint32_t native_modifiers = native_key_data["modifiers"].asInteger();
    
	if (key < 128)
	{
		utf8_text = (char)native_virtual_key;
	}
    
    unsigned int modifers = LLCEFLib::KM_MODIFIER_NONE;

    if (native_modifiers & (MASK_CONTROL | MASK_MAC_CONTROL))
        modifers |= LLCEFLib::KM_MODIFIER_CONTROL;
    if (native_modifiers & MASK_SHIFT)
        modifers |= LLCEFLib::KM_MODIFIER_SHIFT;
    if (native_modifiers & MASK_ALT)
        modifers |= LLCEFLib::KM_MODIFIER_ALT;

    //modifers |= LLCEFLib::KM_MODIFIER_META;
    
	switch ((KEY)key)
    
	{
		case KEY_BACKSPACE:		utf8_text = (char)8;		break;
		case KEY_TAB:			utf8_text = (char)9;		break;
		case KEY_RETURN:		utf8_text = (char)13;		break;
		case KEY_PAD_RETURN:	utf8_text = (char)13;		break;
		case KEY_ESCAPE:		utf8_text = (char)27;		break;

	default:
		break;
	}

	mLLCEFLib->keyboardEvent(key_event, native_char_code, utf8_text.c_str(),
            static_cast<LLCEFLib::EKeyboardModifier>(modifers),
            native_scan_code, native_virtual_key, native_modifiers);
    
#elif LL_WINDOWS
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);
    
	mLLCEFLib->nativeKeyboardEvent(msg, wparam, lparam);
#endif
};

void MediaPluginCEF::unicodeInput(const std::string &utf8str, LLCEFLib::EKeyboardModifier modifiers, LLSD native_key_data = LLSD::emptyMap())
{
#if LL_DARWIN
	//mLLCEFLib->keyPress(utf8str[0], true);
	//mLLCEFLib->keyboardEvent(LLCEFLib::KE_KEY_DOWN, (uint32_t)(utf8str[0]), 0, LLCEFLib::KM_MODIFIER_NONE, 0, 0, 0);
#elif LL_WINDOWS
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);
	mLLCEFLib->nativeKeyboardEvent(msg, wparam, lparam);
#endif
};

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::checkEditState()
{
	bool can_cut = mLLCEFLib->editCanCut();
	bool can_copy = mLLCEFLib->editCanCopy();
	bool can_paste = mLLCEFLib->editCanPaste();

	if ((can_cut != mCanCut) || (can_copy != mCanCopy) || (can_paste != mCanPaste))
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_state");

		if (can_cut != mCanCut)
		{
			mCanCut = can_cut;
			message.setValueBoolean("cut", can_cut);
		}

		if (can_copy != mCanCopy)
		{
			mCanCopy = can_copy;
			message.setValueBoolean("copy", can_copy);
		}

		if (can_paste != mCanPaste)
		{
			mCanPaste = can_paste;
			message.setValueBoolean("paste", can_paste);
		}

		sendMessage(message);
	}
}

void MediaPluginCEF::setVolume(F32 vol)
{
    mVolumeCatcher.setVolume(vol);
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::init()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", "CEF Plugin");
	sendMessage(message);

	return true;
};

////////////////////////////////////////////////////////////////////////////////
//
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
	void* host_user_data,
	LLPluginInstance::sendMessageFunction *plugin_send_func,
	void **plugin_user_data)
{
	MediaPluginCEF* self = new MediaPluginCEF(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginCEF::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}
