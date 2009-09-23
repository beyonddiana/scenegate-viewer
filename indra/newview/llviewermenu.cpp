/** 
 * @file llviewermenu.cpp
 * @brief Builds menus out of items.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewermenu.h" 

// system library includes
#include <iostream>
#include <fstream>
#include <sstream>

// linden library includes
#include "llaudioengine.h"
#include "llfloaterreg.h"
#include "indra_constants.h"
#include "llassetstorage.h"
#include "llchat.h"
#include "llcombobox.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llinstantmessage.h"
#include "llpermissionsflags.h"
#include "llrect.h"
#include "llsecondlifeurls.h"
#include "lltransactiontypes.h"
#include "llui.h"
#include "llview.h"
#include "llxfermanager.h"
#include "message.h"
#include "raytrace.h"
#include "llsdserialize.h"
#include "lltimer.h"
#include "llvfile.h"
#include "llvolumemgr.h"

// newview includes
#include "llagent.h"
#include "llagentwearables.h"
#include "llagentpilot.h"
#include "llbox.h"
#include "llcallingcard.h"
#include "llclipboard.h"
#include "llcompilequeue.h"
#include "llconsole.h"
#include "llviewercontrol.h"
#include "lldebugview.h"
#include "lldir.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpooltree.h"
#include "llface.h"
#include "llfirstuse.h"
#include "llfirsttimetipmanager.h"
#include "llfloater.h"
#include "llfloaterabout.h"
#include "llfloaterbuycurrency.h"
#include "llfloateractivespeakers.h"
#include "llfloateranimpreview.h"
#include "llfloateravatartextures.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloaterdaycycle.h"
#include "llfloaterdirectory.h"
#include "llfloaterchatterbox.h"
#include "llfloaterfonttest.h"
#include "llfloatergodtools.h"
#include "llfloatergroupinvite.h"
#include "llfloatergroups.h"
#include "llfloaterhtmlcurrency.h"
#include "llfloatermediabrowser.h"			// gViewerHtmlHelp
#include "llfloaterhtmlsimple.h"
#include "llfloaterhud.h"
#include "llfloaterinspect.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloateropenobject.h"
#include "llfloaterperms.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloatersettingsdebug.h"
#include "llfloaterenvsettings.h"
#include "llfloatertools.h"
#include "llfloaterwater.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llfloatermemleak.h"
#include "llfasttimerview.h"
#include "llavataractions.h"
#include "lllandmarkactions.h"
#include "llmemoryview.h"
#include "llgivemoney.h"
#include "llgroupmgr.h"
#include "lltooltip.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llfloaterinventory.h"
#include "llkeyboard.h"
#include "llpanellogin.h"
#include "llpanelblockedlist.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "llmimetypes.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llmutelist.h"
#include "llnotify.h"
#include "llpanelobject.h"
#include "llparcel.h"
#include "llprimitive.h"
#include "llresmgr.h"
#include "llrootview.h"
#include "llselectmgr.h"
#include "llsidetray.h"
#include "llsky.h"
#include "llstatusbar.h"
#include "llstatview.h"
#include "llstring.h"
#include "llsurfacepatch.h"
#include "llimview.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolcomp.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolselectland.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "lluploaddialog.h"
#include "lluserauth.h"
#include "lluuid.h"
#include "llviewercamera.h"
#include "llviewergenericmessage.h"
#include "llviewertexturelist.h"	// gTextureList
#include "llviewerinventory.h"
#include "llviewermenufile.h"	// init_menu_file()
#include "llviewermessage.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvolume.h"
#include "llweb.h"
#include "llworld.h"
#include "llworldmap.h"
#include "object_flags.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "roles_constants.h"
#include "llviewerjoystick.h"
#include "llwlanimator.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llfloaternotificationsconsole.h"
#include "llfloatercamera.h"

#include "lltexlayer.h"
#include "llappearancemgr.h"

using namespace LLVOAvatarDefines;

BOOL enable_land_build(void*);
BOOL enable_object_build(void*);

LLVOAvatar* find_avatar_from_object( LLViewerObject* object );
LLVOAvatar* find_avatar_from_object( const LLUUID& object_id );

void handle_test_load_url(void*);

//
// Evil hackish imported globals

//extern BOOL	gHideSelectedObjects;
//extern BOOL gAllowSelectAvatar;
//extern BOOL gDebugAvatarRotation;
extern BOOL gDebugClicks;
extern BOOL gDebugWindowProc;
//extern BOOL gDebugTextEditorTips;
//extern BOOL gDebugSelectMgr;

//
// Globals
//

LLMenuBarGL		*gMenuBarView = NULL;
LLViewerMenuHolderGL	*gMenuHolder = NULL;
LLMenuGL		*gPopupMenuView = NULL;
LLMenuBarGL		*gLoginMenuBarView = NULL;

// Pie menus
LLContextMenu	*gPieSelf	= NULL;
LLContextMenu	*gPieAvatar = NULL;
LLContextMenu	*gPieObject = NULL;
LLContextMenu	*gPieAttachment = NULL;
LLContextMenu	*gPieLand	= NULL;

const std::string SAVE_INTO_INVENTORY("Save Object Back to My Inventory");
const std::string SAVE_INTO_TASK_INVENTORY("Save Object Back to Object Contents");

LLMenuGL* gAttachSubMenu = NULL;
LLMenuGL* gDetachSubMenu = NULL;
LLMenuGL* gTakeOffClothes = NULL;
LLContextMenu* gPieRate = NULL;
LLContextMenu* gAttachScreenPieMenu = NULL;
LLContextMenu* gAttachPieMenu = NULL;
LLContextMenu* gAttachBodyPartPieMenus[8];
LLContextMenu* gDetachPieMenu = NULL;
LLContextMenu* gDetachScreenPieMenu = NULL;
LLContextMenu* gDetachBodyPartPieMenus[8];

LLMenuItemCallGL* gAFKMenu = NULL;
LLMenuItemCallGL* gBusyMenu = NULL;

//
// Local prototypes

// File Menu
const char* upload_pick(void* data);
void handle_compress_image(void*);


// Edit menu
void handle_dump_group_info(void *);
void handle_dump_capabilities_info(void *);
void handle_dump_focus(void*);

// Advanced->Consoles menu
void handle_region_dump_settings(void*);
void handle_region_dump_temp_asset_data(void*);
void handle_region_clear_temp_asset_data(void*);

// Object pie menu
BOOL sitting_on_selection();

void near_sit_object();
//void label_sit_or_stand(std::string& label, void*);
// buy and take alias into the same UI positions, so these
// declarations handle this mess.
BOOL is_selection_buy_not_take();
S32 selection_price();
BOOL enable_take();
void handle_take();
bool confirm_take(const LLSD& notification, const LLSD& response);
BOOL enable_buy(void*); 
void handle_buy(void *);
void handle_buy_object(LLSaleInfo sale_info);
void handle_buy_contents(LLSaleInfo sale_info);

// Land pie menu
void near_sit_down_point(BOOL success, void *);

// Avatar pie menu

// Debug menu


void velocity_interpolate( void* );

void handle_rebake_textures(void*);
BOOL check_admin_override(void*);
void handle_admin_override_toggle(void*);
#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
void handle_toggle_hacked_godmode(void*);
BOOL check_toggle_hacked_godmode(void*);
bool enable_toggle_hacked_godmode(void*);
#endif

void toggle_show_xui_names(void *);
BOOL check_show_xui_names(void *);

// Debug UI

void handle_web_browser_test(void*);
void handle_buy_currency_test(void*);
void handle_save_to_xml(void*);
void handle_load_from_xml(void*);

void handle_god_mode(void*);

// God menu
void handle_leave_god_mode(void*);


void handle_reset_view();

void handle_duplicate_in_place(void*);


void handle_object_owner_self(void*);
void handle_object_owner_permissive(void*);
void handle_object_lock(void*);
void handle_object_asset_ids(void*);
void force_take_copy(void*);
#ifdef _CORY_TESTING
void force_export_copy(void*);
void force_import_geometry(void*);
#endif

void handle_force_parcel_owner_to_me(void*);
void handle_force_parcel_to_content(void*);
void handle_claim_public_land(void*);

void handle_god_request_avatar_geometry(void *);	// Hack for easy testing of new avatar geometry
void reload_vertex_shader(void *);
void handle_disconnect_viewer(void *);

void force_error_breakpoint(void *);
void force_error_llerror(void *);
void force_error_bad_memory_access(void *);
void force_error_infinite_loop(void *);
void force_error_software_exception(void *);
void force_error_driver_crash(void *);

void handle_force_delete(void*);
void print_object_info(void*);
void print_agent_nvpairs(void*);
void toggle_debug_menus(void*);
void upload_done_callback(const LLUUID& uuid, void* user_data, S32 result, LLExtStat ext_status);
void dump_select_mgr(void*);

void dump_inventory(void*);
void edit_ui(void*);
void toggle_visibility(void*);
BOOL get_visibility(void*);

// Avatar Pie menu
void request_friendship(const LLUUID& agent_id);

// Tools menu
void handle_selected_texture_info(void*);

void handle_dump_followcam(void*);
void handle_viewer_enable_message_log(void*);
void handle_viewer_disable_message_log(void*);

BOOL enable_buy_land(void*);

// Help menu

void handle_test_male(void *);
void handle_test_female(void *);
void handle_toggle_pg(void*);
void handle_dump_attachments(void *);
void handle_dump_avatar_local_textures(void*);
void handle_debug_avatar_textures(void*);
void handle_grab_texture(void*);
BOOL enable_grab_texture(void*);
void handle_dump_region_object_cache(void*);

BOOL enable_save_into_inventory(void*);
BOOL enable_save_into_task_inventory(void*);

BOOL enable_detach(const LLSD& = LLSD());
void menu_toggle_attached_lights(void* user_data);
void menu_toggle_attached_particles(void* user_data);

class LLMenuParcelObserver : public LLParcelObserver
{
public:
	LLMenuParcelObserver();
	~LLMenuParcelObserver();
	virtual void changed();
};

static LLMenuParcelObserver* gMenuParcelObserver = NULL;

LLMenuParcelObserver::LLMenuParcelObserver()
{
	LLViewerParcelMgr::getInstance()->addObserver(this);
}

LLMenuParcelObserver::~LLMenuParcelObserver()
{
	LLViewerParcelMgr::getInstance()->removeObserver(this);
}

void LLMenuParcelObserver::changed()
{
	gMenuHolder->childSetEnabled("Land Buy Pass", LLPanelLandGeneral::enableBuyPass(NULL));
	
	BOOL buyable = enable_buy_land(NULL);
	gMenuHolder->childSetEnabled("Land Buy", buyable);
	gMenuHolder->childSetEnabled("Buy Land...", buyable);
}


void initialize_menus();

//-----------------------------------------------------------------------------
// Initialize main menus
//
// HOW TO NAME MENUS:
//
// First Letter Of Each Word Is Capitalized, Even At Or And
//
// Items that lead to dialog boxes end in "..."
//
// Break up groups of more than 6 items with separators
//-----------------------------------------------------------------------------

void set_underclothes_menu_options()
{
	if (gMenuHolder && gAgent.isTeen())
	{
		gMenuHolder->getChild<LLView>("Self Underpants")->setVisible(FALSE);
		gMenuHolder->getChild<LLView>("Self Undershirt")->setVisible(FALSE);
	}
	if (gMenuBarView && gAgent.isTeen())
	{
		gMenuBarView->getChild<LLView>("Menu Underpants")->setVisible(FALSE);
		gMenuBarView->getChild<LLView>("Menu Undershirt")->setVisible(FALSE);
	}
}

void init_menus()
{
	S32 top = gViewerWindow->getRootView()->getRect().getHeight();
	S32 width = gViewerWindow->getRootView()->getRect().getWidth();

	//
	// Main menu bar
	//
	gMenuHolder = new LLViewerMenuHolderGL();
	gMenuHolder->setRect(LLRect(0, top, width, 0));
	gMenuHolder->setFollowsAll();

	LLMenuGL::sMenuContainer = gMenuHolder;

	// Initialize actions
	initialize_menus();

	///
	/// Popup menu
	///
	/// The popup menu is now populated by the show_context_menu()
	/// method.
	
	LLMenuGL::Params menu_params;
	menu_params.name = "Popup";
	menu_params.visible = false;
	gPopupMenuView = LLUICtrlFactory::create<LLMenuGL>(menu_params);
	gMenuHolder->addChild( gPopupMenuView );

	///
	/// Pie menus
	///
	gPieSelf = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_pie_self.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	// TomY TODO: what shall we do about these?
	gDetachScreenPieMenu = gMenuHolder->getChild<LLContextMenu>("Object Detach HUD", true);
	gDetachPieMenu = gMenuHolder->getChild<LLContextMenu>("Object Detach", true);

	gPieAvatar = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_pie_avatar.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	gPieObject = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_pie_object.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	gAttachScreenPieMenu = gMenuHolder->getChild<LLContextMenu>("Object Attach HUD");
	gAttachPieMenu = gMenuHolder->getChild<LLContextMenu>("Object Attach");
	gPieRate = gMenuHolder->getChild<LLContextMenu>("Rate Menu");

	gPieAttachment = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_pie_attachment.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	gPieLand = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_pie_land.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	///
	/// set up the colors
	///
	LLColor4 color;

	LLColor4 context_menu_color = LLUIColorTable::instance().getColor("MenuPopupBgColor");
	
	gPieSelf->setBackgroundColor( context_menu_color );
	gPieAvatar->setBackgroundColor( context_menu_color );
	gPieObject->setBackgroundColor( context_menu_color );
	gPieAttachment->setBackgroundColor( context_menu_color );

	gPieLand->setBackgroundColor( context_menu_color );

	color = LLUIColorTable::instance().getColor( "MenuPopupBgColor" );
	gPopupMenuView->setBackgroundColor( color );

	// If we are not in production, use a different color to make it apparent.
	if (LLViewerLogin::getInstance()->isInProductionGrid())
	{
		color = LLUIColorTable::instance().getColor( "MenuBarBgColor" );
	}
	else
	{
		color = LLUIColorTable::instance().getColor( "MenuNonProductionBgColor" );
	}
	gMenuBarView = LLUICtrlFactory::getInstance()->createFromFile<LLMenuBarGL>("menu_viewer.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	gMenuBarView->setRect(LLRect(0, top, 0, top - MENU_BAR_HEIGHT));
	gMenuBarView->setBackgroundColor( color );

	gMenuHolder->addChild(gMenuBarView);
	
	// menu holder appears on top of menu bar so you can see the menu title
	// flash when an item is triggered (the flash occurs in the holder)
	gViewerWindow->getRootView()->addChild(gMenuHolder);
   
    gViewerWindow->setMenuBackgroundColor(false, 
        LLViewerLogin::getInstance()->isInProductionGrid());

	// Assume L$10 for now, the server will tell us the real cost at login
	// *TODO:Also fix cost in llfolderview.cpp for Inventory menus
	const std::string upload_cost("10");
	gMenuHolder->childSetLabelArg("Upload Image", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Upload Sound", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Upload Animation", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Bulk Upload", "[COST]", upload_cost);

	gAFKMenu = gMenuBarView->getChild<LLMenuItemCallGL>("Set Away", TRUE);
	gBusyMenu = gMenuBarView->getChild<LLMenuItemCallGL>("Set Busy", TRUE);
	gAttachSubMenu = gMenuBarView->findChildMenuByName("Attach Object", TRUE);
	gDetachSubMenu = gMenuBarView->findChildMenuByName("Detach Object", TRUE);

	gMenuBarView->createJumpKeys();

	// Let land based option enable when parcel changes
	gMenuParcelObserver = new LLMenuParcelObserver();

	gLoginMenuBarView = LLUICtrlFactory::getInstance()->createFromFile<LLMenuBarGL>("menu_login.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	gLoginMenuBarView->arrangeAndClear();
	LLRect menuBarRect = gLoginMenuBarView->getRect();
	gLoginMenuBarView->setRect(LLRect(menuBarRect.mLeft, menuBarRect.mTop, gViewerWindow->getRootView()->getRect().getWidth() - menuBarRect.mLeft,  menuBarRect.mBottom));
	gLoginMenuBarView->setBackgroundColor( color );
	gMenuHolder->addChild(gLoginMenuBarView);
	
	// tooltips are on top of EVERYTHING, including menus
	gViewerWindow->getRootView()->sendChildToFront(gToolTipView);
}

///////////////////
// SHOW CONSOLES //
///////////////////


class LLAdvancedToggleConsole : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string console_type = userdata.asString();
		if ("texture" == console_type)
		{
			toggle_visibility( (void*)gTextureView );
		}
		else if ("debug" == console_type)
		{
			toggle_visibility( (void*)((LLView*)gDebugView->mDebugConsolep) );
		}
		else if ("fast timers" == console_type)
		{
			toggle_visibility( (void*)gDebugView->mFastTimerView );
		}
#if MEM_TRACK_MEM
		else if ("memory view" == console_type)
		{
			toggle_visibility( (void*)gDebugView->mMemoryView );
		}
#endif
		return true;
	}
};
class LLAdvancedCheckConsole : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string console_type = userdata.asString();
		bool new_value = false;
		if ("texture" == console_type)
		{
			new_value = get_visibility( (void*)gTextureView );
		}
		else if ("debug" == console_type)
		{
			new_value = get_visibility( (void*)((LLView*)gDebugView->mDebugConsolep) );
		}
		else if ("fast timers" == console_type)
		{
			new_value = get_visibility( (void*)gDebugView->mFastTimerView );
		}
#if MEM_TRACK_MEM
		else if ("memory view" == console_type)
		{
			new_value = get_visibility( (void*)gDebugView->mMemoryView );
		}
#endif

		return new_value;
	}
};


//////////////////////////
// DUMP INFO TO CONSOLE //
//////////////////////////


class LLAdvancedDumpInfoToConsole : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string info_type = userdata.asString();
		if ("region" == info_type)
		{
			handle_region_dump_settings(NULL);
		}
		else if ("group" == info_type)
		{
			handle_dump_group_info(NULL);
		}
		else if ("capabilities" == info_type)
		{
			handle_dump_capabilities_info(NULL);
		}
		return true;
	}
};


//////////////
// HUD INFO //
//////////////


class LLAdvancedToggleHUDInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string info_type = userdata.asString();

		if ("camera" == info_type)
		{
			gDisplayCameraPos = !(gDisplayCameraPos);
		}
		else if ("wind" == info_type)
		{
			gDisplayWindInfo = !(gDisplayWindInfo);
		}
		else if ("fov" == info_type)
		{
			gDisplayFOV = !(gDisplayFOV);
		}
		return true;
	}
};

class LLAdvancedCheckHUDInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string info_type = userdata.asString();
		bool new_value = false;
		if ("camera" == info_type)
		{
			new_value = gDisplayCameraPos;
		}
		else if ("wind" == info_type)
		{
			new_value = gDisplayWindInfo;
		}
		else if ("fov" == info_type)
		{
			new_value = gDisplayFOV;
		}
		return new_value;
	}
};

///////////////////////
// CLEAR GROUP CACHE //
///////////////////////

class LLAdvancedClearGroupCache : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLGroupMgr::debugClearAllGroups(NULL);
		return true;
	}
};




/////////////////
// RENDER TYPE //
/////////////////
U32 render_type_from_string(std::string render_type)
{
	if ("simple" == render_type)
	{
		return LLPipeline::RENDER_TYPE_SIMPLE;
	}
	else if ("alpha" == render_type)
	{
		return LLPipeline::RENDER_TYPE_ALPHA;
	}
	else if ("tree" == render_type)
	{
		return LLPipeline::RENDER_TYPE_TREE;
	}
	else if ("character" == render_type)
	{
		return LLPipeline::RENDER_TYPE_AVATAR;
	}
	else if ("surfacePath" == render_type)
	{
		return LLPipeline::RENDER_TYPE_TERRAIN;
	}
	else if ("sky" == render_type)
	{
		return LLPipeline::RENDER_TYPE_SKY;
	}
	else if ("water" == render_type)
	{
		return LLPipeline::RENDER_TYPE_WATER;
	}
	else if ("ground" == render_type)
	{
		return LLPipeline::RENDER_TYPE_GROUND;
	}
	else if ("volume" == render_type)
	{
		return LLPipeline::RENDER_TYPE_VOLUME;
	}
	else if ("grass" == render_type)
	{
		return LLPipeline::RENDER_TYPE_GRASS;
	}
	else if ("clouds" == render_type)
	{
		return LLPipeline::RENDER_TYPE_CLOUDS;
	}
	else if ("particles" == render_type)
	{
		return LLPipeline::RENDER_TYPE_PARTICLES;
	}
	else if ("bump" == render_type)
	{
		return LLPipeline::RENDER_TYPE_BUMP;
	}
	else
	{
		return 0;
	}
}


class LLAdvancedToggleRenderType : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		U32 render_type = render_type_from_string( userdata.asString() );
		if ( render_type != 0 )
		{
			LLPipeline::toggleRenderTypeControl( (void*)render_type );
		}
		return true;
	}
};


class LLAdvancedCheckRenderType : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		U32 render_type = render_type_from_string( userdata.asString() );
		bool new_value = false;

		if ( render_type != 0 )
		{
			new_value = LLPipeline::hasRenderTypeControl( (void*)render_type );
		}

		return new_value;
	}
};


/////////////
// FEATURE //
/////////////
U32 feature_from_string(std::string feature)
{ 
	if ("ui" == feature)
	{ 
		return LLPipeline::RENDER_DEBUG_FEATURE_UI;
	}
	else if ("selected" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_SELECTED;
	}
	else if ("highlighted" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_HIGHLIGHTED;
	}
	else if ("dynamic textures" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES;
	}
	else if ("foot shadows" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_FOOT_SHADOWS;
	}
	else if ("fog" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_FOG;
	}
	else if ("fr info" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_FR_INFO;
	}
	else if ("flexible" == feature)
	{
		return LLPipeline::RENDER_DEBUG_FEATURE_FLEXIBLE;
	}
	else
	{
		return 0;
	}
};


class LLAdvancedToggleFeature : public view_listener_t{
	bool handleEvent(const LLSD& userdata)
	{
		U32 feature = feature_from_string( userdata.asString() );
		if ( feature != 0 )
		{
			LLPipeline::toggleRenderDebugFeature( (void*)feature );
		}
		return true;
	}
};

class LLAdvancedCheckFeature : public view_listener_t
{bool handleEvent(const LLSD& userdata)
{
	U32 feature = feature_from_string( userdata.asString() );
	bool new_value = false;

	if ( feature != 0 )
	{
		new_value = LLPipeline::toggleRenderDebugFeatureControl( (void*)feature );
	}

	return new_value;
}
};


//////////////////
// INFO DISPLAY //
//////////////////
U32 info_display_from_string(std::string info_display)
{
	if ("verify" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_VERIFY;
	}
	else if ("bboxes" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_BBOXES;
	}
	else if ("points" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_POINTS;
	}
	else if ("octree" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_OCTREE;
	}
	else if ("shadow frusta" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA;
	}
	else if ("occlusion" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_OCCLUSION;
	}
	else if ("render batches" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_BATCH_SIZE;
	}
	else if ("texture anim" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_TEXTURE_ANIM;
	}
	else if ("texture priority" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY;
	}
	else if ("shame" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_SHAME;
	}
	else if ("texture area" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_TEXTURE_AREA;
	}
	else if ("face area" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_FACE_AREA;
	}
	else if ("lights" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_LIGHTS;
	}
	else if ("particles" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_PARTICLES;
	}
	else if ("composition" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_COMPOSITION;
	}
	else if ("glow" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_GLOW;
	}
	else if ("collision skeleton" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_AVATAR_VOLUME;
	}
	else if ("raycast" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_RAYCAST;
	}
	else if ("agent target" == info_display)
	{
		return LLPipeline::RENDER_DEBUG_AGENT_TARGET;
	}
	else
	{
		return 0;
	}
};

class LLAdvancedToggleInfoDisplay : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		U32 info_display = info_display_from_string( userdata.asString() );

		if ( info_display != 0 )
		{
			LLPipeline::toggleRenderDebug( (void*)info_display );
		}

		return true;
	}
};


class LLAdvancedCheckInfoDisplay : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		U32 info_display = info_display_from_string( userdata.asString() );
		bool new_value = false;

		if ( info_display != 0 )
		{
			new_value = LLPipeline::toggleRenderDebugControl( (void*)info_display );
		}

		return new_value;
	}
};


///////////////////////////
//// RANDOMIZE FRAMERATE //
///////////////////////////


class LLAdvancedToggleRandomizeFramerate : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gRandomizeFramerate = !(gRandomizeFramerate);
		return true;
	}
};

class LLAdvancedCheckRandomizeFramerate : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gRandomizeFramerate;
		return new_value;
	}
};

void run_vectorize_perf_test(void *)
{
	gSavedSettings.setBOOL("VectorizePerfTest", TRUE);
}


////////////////////////////////
// RUN Vectorized Perform Test//
////////////////////////////////


class LLAdvancedVectorizePerfTest : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		run_vectorize_perf_test(NULL);
		return true;
	}
};

///////////////////////////
//// PERIODIC SLOW FRAME //
///////////////////////////


class LLAdvancedTogglePeriodicSlowFrame : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gPeriodicSlowFrame = !(gPeriodicSlowFrame);
		return true;
	}
};

class LLAdvancedCheckPeriodicSlowFrame : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gPeriodicSlowFrame;
		return new_value;
	}
};



////////////////
// FRAME TEST //
////////////////


class LLAdvancedToggleFrameTest : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLPipeline::sRenderFrameTest = !(LLPipeline::sRenderFrameTest);
		return true;
	}
};

class LLAdvancedCheckFrameTest : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLPipeline::sRenderFrameTest;
		return new_value;
	}
};


///////////////////////////
// SELECTED TEXTURE INFO //
///////////////////////////


class LLAdvancedSelectedTextureInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_selected_texture_info(NULL);
		return true;
	}
};

//////////////////////
// TOGGLE WIREFRAME //
//////////////////////

class LLAdvancedToggleWireframe : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gUseWireframe = !(gUseWireframe);
		return true;
	}
};

class LLAdvancedCheckWireframe : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gUseWireframe;
		return new_value;
	}
};
	
//////////////////////
// DISABLE TEXTURES //
//////////////////////

class LLAdvancedToggleDisableTextures : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerTexture::sDontLoadVolumeTextures = !LLViewerTexture::sDontLoadVolumeTextures;
		return true;
	}
};

class LLAdvancedCheckDisableTextures : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLViewerTexture::sDontLoadVolumeTextures; // <-- make this using LLCacheControl
		return new_value;
	}
};

//////////////////////
// TEXTURE ATLAS //
//////////////////////

class LLAdvancedToggleTextureAtlas : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerTexture::sUseTextureAtlas = !LLViewerTexture::sUseTextureAtlas;
		gSavedSettings.setBOOL("EnableTextureAtlas", LLViewerTexture::sUseTextureAtlas) ;
		return true;
	}
};

class LLAdvancedCheckTextureAtlas : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLViewerTexture::sUseTextureAtlas; // <-- make this using LLCacheControl
		return new_value;
	}
};

//////////////////////////
// DUMP SCRIPTED CAMERA //
//////////////////////////
	
class LLAdvancedDumpScriptedCamera : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_dump_followcam(NULL);
		return true;
}
};



//////////////////////////////
// DUMP REGION OBJECT CACHE //
//////////////////////////////


class LLAdvancedDumpRegionObjectCache : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
{
		handle_dump_region_object_cache(NULL);
		return true;
	}
};

class LLAdvancedWebBrowserTest : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_web_browser_test(NULL);
		return true;
	}
};
	
class LLAdvancedBuyCurrencyTest : public view_listener_t
	{
	bool handleEvent(const LLSD& userdata)
	{
		handle_buy_currency_test(NULL);
		return true;
	}
};


////////////////////////
// TOGGLE EDITABLE UI //
////////////////////////


class LLAdvancedToggleEditableUI : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		edit_ui(NULL);
		return true;
	}
};

	
/////////////////////
// DUMP SELECT MGR //
/////////////////////


class LLAdvancedDumpSelectMgr : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		dump_select_mgr(NULL);
		return true;
	}
};



////////////////////
// DUMP INVENTORY //
////////////////////


class LLAdvancedDumpInventory : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		dump_inventory(NULL);
		return true;
	}
};



///////////////////////
// DUMP FOCUS HOLDER //
///////////////////////
	

class LLAdvancedDumpFocusHolder : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_dump_focus(NULL);
		return true;
	}
};


	
////////////////////////////////
// PRINT SELECTED OBJECT INFO //
////////////////////////////////


class LLAdvancedPrintSelectedObjectInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		print_object_info(NULL);
		return true;
	}
};



//////////////////////
// PRINT AGENT INFO //
//////////////////////


class LLAdvancedPrintAgentInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		print_agent_nvpairs(NULL);
		return true;
	}
};



////////////////////////////////
// PRINT TEXTURE MEMORY STATS //
////////////////////////////////


class LLAdvancedPrintTextureMemoryStats : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		output_statistics(NULL);
		return true;
	}
};

//////////////////
// DEBUG CLICKS //
//////////////////


class LLAdvancedToggleDebugClicks : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gDebugClicks = !(gDebugClicks);
		return true;
	}
};

class LLAdvancedCheckDebugClicks : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gDebugClicks;
		return new_value;
	}
};



/////////////////
// DEBUG VIEWS //
/////////////////


class LLAdvancedToggleDebugViews : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLView::sDebugRects = !(LLView::sDebugRects);
		return true;
	}
};

class LLAdvancedCheckDebugViews : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLView::sDebugRects;
		return new_value;
	}
};



///////////////////////
// XUI NAME TOOLTIPS //
///////////////////////


class LLAdvancedToggleXUINameTooltips : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		toggle_show_xui_names(NULL);
		return true;
	}
};

class LLAdvancedCheckXUINameTooltips : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = check_show_xui_names(NULL);
		return new_value;
	}
};



////////////////////////
// DEBUG MOUSE EVENTS //
////////////////////////


class LLAdvancedToggleDebugMouseEvents : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLView::sDebugMouseHandling = !(LLView::sDebugMouseHandling);
		return true;
	}
};

class LLAdvancedCheckDebugMouseEvents : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLView::sDebugMouseHandling;
		return new_value;
	}
};



////////////////
// DEBUG KEYS //
////////////////


class LLAdvancedToggleDebugKeys : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLView::sDebugKeys = !(LLView::sDebugKeys);
		return true;
	}
};
	
class LLAdvancedCheckDebugKeys : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLView::sDebugKeys;
		return new_value;
	}
};
	


///////////////////////
// DEBUG WINDOW PROC //
///////////////////////


class LLAdvancedToggleDebugWindowProc : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gDebugWindowProc = !(gDebugWindowProc);
		return true;
	}
};

class LLAdvancedCheckDebugWindowProc : public view_listener_t
	{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gDebugWindowProc;
		return new_value;
	}
};

// ------------------------------XUI MENU ---------------------------

//////////////////////
// LOAD UI FROM XML //
//////////////////////


class LLAdvancedLoadUIFromXML : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_load_from_xml(NULL);
		return true;
}
};



////////////////////
// SAVE UI TO XML //
////////////////////


class LLAdvancedSaveUIToXML : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_save_to_xml(NULL);
		return true;
}
};


class LLAdvancedSendTestIms : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLIMModel::instance().testMessages();
		return true;
}
};


///////////////
// XUI NAMES //
///////////////


class LLAdvancedToggleXUINames : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		toggle_show_xui_names(NULL);
		return true;
	}
};

class LLAdvancedCheckXUINames : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = check_show_xui_names(NULL);
		return new_value;
	}
};


////////////////////////
// GRAB BAKED TEXTURE //
////////////////////////


class LLAdvancedGrabBakedTexture : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string texture_type = userdata.asString();
		if ("iris" == texture_type)
		{
			handle_grab_texture( (void*)TEX_EYES_BAKED );
		}
		else if ("head" == texture_type)
		{
			handle_grab_texture( (void*)TEX_HEAD_BAKED );
		}
		else if ("upper" == texture_type)
		{
			handle_grab_texture( (void*)TEX_UPPER_BAKED );
		}
		else if ("lower" == texture_type)
		{
			handle_grab_texture( (void*)TEX_SKIRT_BAKED );
		}
		else if ("skirt" == texture_type)
		{
			handle_grab_texture( (void*)TEX_SKIRT_BAKED );
		}
		else if ("hair" == texture_type)
		{
			handle_grab_texture( (void*)TEX_HAIR_BAKED );
}

		return true;
	}
};

class LLAdvancedEnableGrabBakedTexture : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
{
		std::string texture_type = userdata.asString();
		bool new_value = false;

		if ("iris" == texture_type)
		{
			new_value = enable_grab_texture( (void*)TEX_EYES_BAKED );
		}
		else if ("head" == texture_type)
		{
			new_value = enable_grab_texture( (void*)TEX_HEAD_BAKED );
		}
		else if ("upper" == texture_type)
		{
			new_value = enable_grab_texture( (void*)TEX_UPPER_BAKED );
		}
		else if ("lower" == texture_type)
		{
			new_value = enable_grab_texture( (void*)TEX_LOWER_BAKED );
		}
		else if ("skirt" == texture_type)
		{
			new_value = enable_grab_texture( (void*)TEX_SKIRT_BAKED );
		}
	
		return new_value;
}
};

///////////////////////
// APPEARANCE TO XML //
///////////////////////


class LLAdvancedAppearanceToXML : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar::dumpArchetypeXML(NULL);
		return true;
	}
};



///////////////////////////////
// TOGGLE CHARACTER GEOMETRY //
///////////////////////////////


class LLAdvancedToggleCharacterGeometry : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_god_request_avatar_geometry(NULL);
		return true;
}
};

class LLEnableGodCustomerService : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
{
		bool new_value = enable_god_customer_service(NULL);
		return new_value;
	}
};



	/////////////////////////////
// TEST MALE / TEST FEMALE //
/////////////////////////////

class LLAdvancedTestMale : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_test_male(NULL);
		return true;
	}
};


class LLAdvancedTestFemale : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_test_female(NULL);
		return true;
	}
};



///////////////
// TOGGLE PG //
///////////////


class LLAdvancedTogglePG : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_toggle_pg(NULL);
		return true;
	}
};



////////////////////////////
// ALLOW TAP-TAP-HOLD RUN //
////////////////////////////


class LLAdvancedToggleAllowTapTapHoldRun : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAllowTapTapHoldRun = !(gAllowTapTapHoldRun);
		return true;
	}
};

class LLAdvancedCheckAllowTapTapHoldRun : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gAllowTapTapHoldRun;
		return new_value;
	}
};





class LLAdvancedForceParamsToDefault : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLAgent::clearVisualParams(NULL);
		return true;
	}
};



//////////////////////////
// RELOAD VERTEX SHADER //
//////////////////////////


class LLAdvancedReloadVertexShader : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		reload_vertex_shader(NULL);
		return true;
	}
};



////////////////////
// ANIMATION INFO //
////////////////////


class LLAdvancedToggleAnimationInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar::sShowAnimationDebug = !(LLVOAvatar::sShowAnimationDebug);
		return true;
	}
};

class LLAdvancedCheckAnimationInfo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLVOAvatar::sShowAnimationDebug;
		return new_value;
	}
};


//////////////////
// SHOW LOOK AT //
//////////////////


class LLAdvancedToggleShowLookAt : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLHUDEffectLookAt::sDebugLookAt = !(LLHUDEffectLookAt::sDebugLookAt);
		return true;
	}
};

class LLAdvancedCheckShowLookAt : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLHUDEffectLookAt::sDebugLookAt;
		return new_value;
	}
};



///////////////////
// SHOW POINT AT //
///////////////////


class LLAdvancedToggleShowPointAt : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLHUDEffectPointAt::sDebugPointAt = !(LLHUDEffectPointAt::sDebugPointAt);
		return true;
	}
};

class LLAdvancedCheckShowPointAt : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLHUDEffectPointAt::sDebugPointAt;
		return new_value;
	}
};



/////////////////////////
// DEBUG JOINT UPDATES //
/////////////////////////


class LLAdvancedToggleDebugJointUpdates : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar::sJointDebug = !(LLVOAvatar::sJointDebug);
		return true;
	}
};

class LLAdvancedCheckDebugJointUpdates : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLVOAvatar::sJointDebug;
		return new_value;
	}
};



/////////////////
// DISABLE LOD //
/////////////////


class LLAdvancedToggleDisableLOD : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerJoint::sDisableLOD = !(LLViewerJoint::sDisableLOD);
		return true;
	}
};
		
class LLAdvancedCheckDisableLOD : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLViewerJoint::sDisableLOD;
		return new_value;
	}
};



/////////////////////////
// DEBUG CHARACTER VIS //
/////////////////////////


class LLAdvancedToggleDebugCharacterVis : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar::sDebugInvisible = !(LLVOAvatar::sDebugInvisible);
		return true;
	}
};

class LLAdvancedCheckDebugCharacterVis : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLVOAvatar::sDebugInvisible;
		return new_value;
	}
};


//////////////////////
// DUMP ATTACHMENTS //
//////////////////////

	
class LLAdvancedDumpAttachments : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_dump_attachments(NULL);
		return true;
	}
};


	
/////////////////////
// REBAKE TEXTURES //
/////////////////////
	
	
class LLAdvancedRebakeTextures : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_rebake_textures(NULL);
		return true;
	}
};
	
	
#ifndef LL_RELEASE_FOR_DOWNLOAD
///////////////////////////
// DEBUG AVATAR TEXTURES //
///////////////////////////


class LLAdvancedDebugAvatarTextures : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_debug_avatar_textures(NULL);
		return true;
	}
};

////////////////////////////////
// DUMP AVATAR LOCAL TEXTURES //
////////////////////////////////


class LLAdvancedDumpAvatarLocalTextures : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_dump_avatar_local_textures(NULL);
		return true;
	}
};

#endif
	
/////////////////
// MESSAGE LOG //
/////////////////


class LLAdvancedEnableMessageLog : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_viewer_enable_message_log(NULL);
		return true;
	}
};

class LLAdvancedDisableMessageLog : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_viewer_disable_message_log(NULL);
		return true;
	}
};

/////////////////
// DROP PACKET //
/////////////////


class LLAdvancedDropPacket : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gMessageSystem->mPacketRing.dropPackets(1);
		return true;
	}
};



/////////////////
// AGENT PILOT //
/////////////////


class LLAdvancedAgentPilot : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string command = userdata.asString();
		if ("start playback" == command)
		{
			LLAgentPilot::startPlayback(NULL);
		}
		else if ("stop playback" == command)
		{
			LLAgentPilot::stopPlayback(NULL);
		}
		else if ("start record" == command)
		{
			LLAgentPilot::startRecord(NULL);
		}
		else if ("stop record" == command)
		{
			LLAgentPilot::saveRecord(NULL);
		}

		return true;
	}		
};



//////////////////////
// AGENT PILOT LOOP //
//////////////////////


class LLAdvancedToggleAgentPilotLoop : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLAgentPilot::sLoop = !(LLAgentPilot::sLoop);
		return true;
	}
};

class LLAdvancedCheckAgentPilotLoop : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLAgentPilot::sLoop;
		return new_value;
	}
};


/////////////////////////
// SHOW OBJECT UPDATES //
/////////////////////////


class LLAdvancedToggleShowObjectUpdates : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gShowObjectUpdates = !(gShowObjectUpdates);
		return true;
	}
};

class LLAdvancedCheckShowObjectUpdates : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gShowObjectUpdates;
		return new_value;
	}
};



////////////////////
// COMPRESS IMAGE //
////////////////////


class LLAdvancedCompressImage : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_compress_image(NULL);
		return true;
	}
};



/////////////////////////
// SHOW DEBUG SETTINGS //
/////////////////////////


class LLAdvancedShowDebugSettings : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterReg::showInstance("settings_debug",userdata);
		return true;
	}
};



////////////////////////
// VIEW ADMIN OPTIONS //
////////////////////////


class LLAdvancedToggleViewAdminOptions : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_admin_override_toggle(NULL);
		return true;
	}
};

class LLAdvancedCheckViewAdminOptions : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = check_admin_override(NULL);
		return new_value;
	}
};

/////////////////////////////////////
// Enable Object Object Occlusion ///
/////////////////////////////////////
class LLAdvancedEnableObjectObjectOcclusion: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
	
		bool new_value = gGLManager.mHasOcclusionQuery; // && LLFeatureManager::getInstance()->isFeatureAvailable(userdata.asString());
		return new_value;
}
};

/////////////////////////////////////
// Enable Framebuffer Objects	  ///
/////////////////////////////////////
class LLAdvancedEnableRenderFBO: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gGLManager.mHasFramebufferObject;
		return new_value;
	}
};

/////////////////////////////////////
// Enable Deferred Rendering	  ///
/////////////////////////////////////
class LLAdvancedEnableRenderDeferred: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gSavedSettings.getBOOL("RenderUseFBO") && LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_WINDLIGHT > 0) &&
			LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) > 0;
		return new_value;
	}
};

/////////////////////////////////////
// Enable Global Illumination 	  ///
/////////////////////////////////////
class LLAdvancedEnableRenderDeferredGI: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gSavedSettings.getBOOL("RenderUseFBO") && gSavedSettings.getBOOL("RenderDeferred");
		return new_value;
	}
};



//////////////////
// ADMIN STATUS //
//////////////////


class LLAdvancedRequestAdminStatus : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_god_mode(NULL);
		return true;
	}
};

class LLAdvancedLeaveAdminStatus : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_leave_god_mode(NULL);
		return true;
	}
};

//////////////////////////
// Advanced > Debugging //
//////////////////////////


class LLAdvancedForceErrorBreakpoint : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_breakpoint(NULL);
		return true;
	}
};

class LLAdvancedForceErrorLlerror : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_llerror(NULL);
		return true;
	}
};
class LLAdvancedForceErrorBadMemoryAccess : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_bad_memory_access(NULL);
		return true;
	}
};

class LLAdvancedForceErrorInfiniteLoop : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_infinite_loop(NULL);
		return true;
	}
};

class LLAdvancedForceErrorSoftwareException : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_software_exception(NULL);
		return true;
	}
};

class LLAdvancedForceErrorDriverCrash : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_error_driver_crash(NULL);
		return true;
	}
};

class LLAdvancedForceErrorDisconnectViewer : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_disconnect_viewer(NULL);
		return true;
}
};


#ifdef TOGGLE_HACKED_GODLIKE_VIEWER

class LLAdvancedHandleToggleHackedGodmode : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_toggle_hacked_godmode(NULL);
		return true;
	}
};

class LLAdvancedCheckToggleHackedGodmode : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		check_toggle_hacked_godmode(NULL);
		return true;
	}
};

class LLAdvancedEnableToggleHackedGodmode : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = enable_toggle_hacked_godmode(NULL);
		return new_value;
	}
};
#endif


//
////-------------------------------------------------------------------
//// Advanced menu
////-------------------------------------------------------------------

//////////////////
// ADMIN MENU   //
//////////////////

// Admin > Object
class LLAdminForceTakeCopy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		force_take_copy(NULL);
		return true;
	}
};

class LLAdminHandleObjectOwnerSelf : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_object_owner_self(NULL);
		return true;
	}
};
class LLAdminHandleObjectOwnerPermissive : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_object_owner_permissive(NULL);
		return true;
	}
};

class LLAdminHandleForceDelete : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_force_delete(NULL);
		return true;
	}
};

class LLAdminHandleObjectLock : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_object_lock(NULL);
		return true;
	}
};

class LLAdminHandleObjectAssetIDs: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_object_asset_ids(NULL);
		return true;
	}	
};

//Admin >Parcel
class LLAdminHandleForceParcelOwnerToMe: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_force_parcel_owner_to_me(NULL);
		return true;
	}
};
class LLAdminHandleForceParcelToContent: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_force_parcel_to_content(NULL);
		return true;
	}
};
class LLAdminHandleClaimPublicLand: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_claim_public_land(NULL);
		return true;
	}
};

// Admin > Region
class LLAdminHandleRegionDumpTempAssetData: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_region_dump_temp_asset_data(NULL);
		return true;
	}
};
//Admin (Top Level)

class LLAdminOnSaveState: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLPanelRegionTools::onSaveState(NULL);
		return true;
}
};


//-----------------------------------------------------------------------------
// cleanup_menus()
//-----------------------------------------------------------------------------
void cleanup_menus()
{
	delete gMenuParcelObserver;
	gMenuParcelObserver = NULL;

	delete gPieSelf;
	gPieSelf = NULL;

	delete gPieAvatar;
	gPieAvatar = NULL;

	delete gPieObject;
	gPieObject = NULL;

	delete gPieAttachment;
	gPieAttachment = NULL;

	delete gPieLand;
	gPieLand = NULL;

	delete gMenuBarView;
	gMenuBarView = NULL;

	delete gPopupMenuView;
	gPopupMenuView = NULL;

	delete gMenuHolder;
	gMenuHolder = NULL;
}

//-----------------------------------------------------------------------------
// Object pie menu
//-----------------------------------------------------------------------------

class LLObjectReportAbuse : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (objectp)
		{
			LLFloaterReporter::showFromObject(objectp->getID());
		}
		return true;
	}
};

// Enabled it you clicked an object
class LLObjectEnableReportAbuse : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->getSelection()->getObjectCount() != 0;
		return new_value;
	}
};

class LLObjectTouch : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object) return true;

		LLPickInfo pick = LLToolPie::getInstance()->getPick();

		LLMessageSystem	*msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_ObjectGrab);
		msg->nextBlockFast( _PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast( _PREHASH_ObjectData);
		msg->addU32Fast(    _PREHASH_LocalID, object->mLocalID);
		msg->addVector3Fast(_PREHASH_GrabOffset, LLVector3::zero );
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);
		msg->sendMessage( object->getRegion()->getHost());

		// *NOTE: Hope the packets arrive safely and in order or else
		// there will be some problems.
		// *TODO: Just fix this bad assumption.
		msg->newMessageFast(_PREHASH_ObjectDeGrab);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);
		msg->sendMessage(object->getRegion()->getHost());

		return true;
	}
};


// One object must have touch sensor
class LLObjectEnableTouch : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = obj && obj->flagHandleTouch();

		// Update label based on the node touch name if available.
		std::string touch_text;
		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
		if (node && node->mValid && !node->mTouchName.empty())
		{
			touch_text = node->mTouchName;
		}
		else
		{
			touch_text = userdata.asString();
		}
		gMenuHolder->childSetText("Object Touch", touch_text);
		gMenuHolder->childSetText("Attachment Object Touch", touch_text);

		return new_value;
	}
};

//void label_touch(std::string& label, void*)
//{
//	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
//	if (node && node->mValid && !node->mTouchName.empty())
//	{
//		label.assign(node->mTouchName);
//	}
//	else
//	{
//		label.assign("Touch");
//	}
//}
/*
bool handle_object_open()
{
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if(!obj) return true;

	LLFloaterOpenObject::show();
	return true;
}

class LLObjectOpen : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return handle_object_open();
	}
};
*/
class LLObjectEnableOpen : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// Look for contents in root object, which is all the LLFloaterOpenObject
		// understands.
		LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = (obj != NULL);
		if (new_value)
		{
			LLViewerObject* root = obj->getRootEdit();
			if (!root) new_value = false;
			else new_value = root->allowOpen();
		}
		return new_value;
	}
};


class LLViewJoystickFlycam : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_toggle_flycam();
		return true;
	}
};

class LLViewCheckJoystickFlycam : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
		{
		bool new_value = LLViewerJoystick::getInstance()->getOverrideCamera();
		return new_value;
	}
};

void handle_toggle_flycam()
{
	LLViewerJoystick::getInstance()->toggleFlycam();
}

class LLObjectBuild : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement") )
		{
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolCompCreate::getInstance() );

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLObjectEdit : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerParcelMgr::getInstance()->deselectLand();

		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit())
		{
			LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

			if (selection->getSelectType() == SELECT_TYPE_HUD || !gSavedSettings.getBOOL("EditCameraMovement"))
			{
				// always freeze camera in space, even if camera doesn't move
				// so, for example, follow cam scripts can't affect you when in build mode
				gAgent.setFocusGlobal(gAgent.calcFocusPositionTargetGlobal(), LLUUID::null);
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			}
			else
			{
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
				LLViewerObject* selected_objectp = selection->getFirstRootObject();
				if (selected_objectp)
				{
				// zoom in on object center instead of where we clicked, as we need to see the manipulator handles
					gAgent.setFocusGlobal(selected_objectp->getPositionGlobal(), selected_objectp->getID());
				gAgent.cameraZoomIn(0.666f);
				gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
				gViewerWindow->moveCursorToCenter();
			}
		}
		}

		LLFloaterReg::showInstance("build");
	
		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		gFloaterTools->setEditTool( LLToolCompTranslate::getInstance() );

		LLViewerJoystick::getInstance()->moveObjects(true);
		LLViewerJoystick::getInstance()->setNeedsReset(true);

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

//---------------------------------------------------------------------------
// Land pie menu
//---------------------------------------------------------------------------
class LLLandBuild : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerParcelMgr::getInstance()->deselectLand();

		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement")  )
		{
			// otherwise just move focus
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}


		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolCompCreate::getInstance() );

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLLandBuyPass : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLPanelLandGeneral::onClickBuyPass((void *)FALSE);
		return true;
	}
};

class LLLandEnableBuyPass : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLPanelLandGeneral::enableBuyPass(NULL);
		return new_value;
	}
};

// BUG: Should really check if CLICK POINT is in a parcel where you can build.
BOOL enable_land_build(void*)
{
	if (gAgent.isGodlike()) return TRUE;
	if (gAgent.inPrelude()) return FALSE;

	BOOL can_build = FALSE;
	LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (agent_parcel)
	{
		can_build = agent_parcel->getAllowModify();
	}
	return can_build;
}

// BUG: Should really check if OBJECT is in a parcel where you can build.
BOOL enable_object_build(void*)
{
	if (gAgent.isGodlike()) return TRUE;
	if (gAgent.inPrelude()) return FALSE;

	BOOL can_build = FALSE;
	LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (agent_parcel)
	{
		can_build = agent_parcel->getAllowModify();
	}
	return can_build;
}

class LLEnableEdit : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// *HACK:  The new "prelude" Help Islands have a build sandbox area,
		// so users need the Edit and Create pie menu options when they are
		// there.  Eventually this needs to be replaced with code that only 
		// lets you edit objects if you have permission to do so (edit perms,
		// group edit, god).  See also lltoolbar.cpp.  JC
		bool enable = true;
		if (gAgent.inPrelude())
		{
			enable = LLViewerParcelMgr::getInstance()->agentCanBuild()
				|| LLSelectMgr::getInstance()->getSelection()->isAttachment();
		}
		return enable;
	}
};

class LLSelfRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLAgentWearables::userRemoveAllAttachments(NULL);
		return true;
	}
};

class LLSelfEnableRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = false;
		if (gAgent.getAvatarObject())
		{
			LLVOAvatar* avatarp = gAgent.getAvatarObject();
			for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
				 iter != avatarp->mAttachmentPoints.end(); )
			{
				LLVOAvatar::attachment_map_t::iterator curiter = iter++;
				LLViewerJointAttachment* attachment = curiter->second;
				if (attachment->getObject())
				{
					new_value = true;
					break;
				}
			}
		}
		return new_value;
	}
};

BOOL enable_has_attachments(void*)
{

	return FALSE;
}

//---------------------------------------------------------------------------
// Avatar pie menu
//---------------------------------------------------------------------------
//void handle_follow(void *userdata)
//{
//	// follow a given avatar by ID
//	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
//	if (objectp)
//	{
//		gAgent.startFollowPilot(objectp->getID());
//	}
//}

class LLObjectEnableMute : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = (object != NULL);
		if (new_value)
		{
			LLVOAvatar* avatar = find_avatar_from_object(object); 
			if (avatar)
			{
				// It's an avatar
				LLNameValue *lastname = avatar->getNVPair("LastName");
				BOOL is_linden = lastname && !LLStringUtil::compareStrings(lastname->getString(), "Linden");
				BOOL is_self = avatar->isSelf();
				new_value = !is_linden && !is_self;
			}
		}
		return new_value;
	}
};

class LLObjectMute : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object) return true;
		
		LLUUID id;
		std::string name;
		LLMute::EType type;
		LLVOAvatar* avatar = find_avatar_from_object(object); 
		if (avatar)
		{
			id = avatar->getID();

			LLNameValue *firstname = avatar->getNVPair("FirstName");
			LLNameValue *lastname = avatar->getNVPair("LastName");
			if (firstname && lastname)
			{
				name = firstname->getString();
				name += " ";
				name += lastname->getString();
			}
			
			type = LLMute::AGENT;
		}
		else
		{
			// it's an object
			id = object->getID();

			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}
			
			type = LLMute::OBJECT;
		}
		
		LLMute mute(id, name, type);
		if (LLMuteList::getInstance()->isMuted(mute.mID, mute.mName))
		{
			LLMuteList::getInstance()->remove(mute);
		}
		else
		{
			LLMuteList::getInstance()->add(mute);
			LLPanelBlockedList::showPanelAndSelect(mute.mID);
		}
		
		return true;
	}
};

bool handle_go_to()
{
	// try simulator autopilot
	std::vector<std::string> strings;
	std::string val;
	LLVector3d pos = LLToolPie::getInstance()->getPick().mPosGlobal;
	val = llformat("%g", pos.mdV[VX]);
	strings.push_back(val);
	val = llformat("%g", pos.mdV[VY]);
	strings.push_back(val);
	val = llformat("%g", pos.mdV[VZ]);
	strings.push_back(val);
	send_generic_message("autopilot", strings);

	LLViewerParcelMgr::getInstance()->deselectLand();

	if (gAgent.getAvatarObject() && !gSavedSettings.getBOOL("AutoPilotLocksCamera"))
	{
		gAgent.setFocusGlobal(gAgent.getFocusTargetGlobal(), gAgent.getAvatarObject()->getID());
	}
	else 
	{
		// Snap camera back to behind avatar
		gAgent.setFocusOnAvatar(TRUE, ANIMATE);
	}

	// Could be first use
	LLFirstUse::useGoTo();
	return true;
}

class LLGoToObject : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return handle_go_to();
	}
};

class LLAvatarReportAbuse : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if(avatar)
		{
			LLFloaterReporter::showFromObject(avatar->getID());
		}
		return true;
	}
};


//---------------------------------------------------------------------------
// Parcel freeze, eject, etc.
//---------------------------------------------------------------------------
bool callback_freeze(const LLSD& notification, const LLSD& response)
{
	LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (0 == option || 1 == option)
	{
		U32 flags = 0x0;
		if (1 == option)
		{
			// unfreeze
			flags |= 0x1;
		}

		LLMessageSystem* msg = gMessageSystem;
		LLViewerObject* avatar = gObjectList.findObject(avatar_id);

		if (avatar)
		{
			msg->newMessage("FreezeUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id );
			msg->addU32("Flags", flags );
			msg->sendReliable( avatar->getRegion()->getHost() );
		}
	}
	return false;
}


class LLAvatarFreeze : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if( avatar )
		{
			std::string fullname = avatar->getFullname();
			LLSD payload;
			payload["avatar_id"] = avatar->getID();

			if (!fullname.empty())
			{
				LLSD args;
				args["AVATAR_NAME"] = fullname;
				LLNotifications::instance().add("FreezeAvatarFullname",
							args,
							payload,
							callback_freeze);
			}
			else
			{
				LLNotifications::instance().add("FreezeAvatar",
							LLSD(),
							payload,
							callback_freeze);
			}
		}
		return true;
	}
};

class LLAvatarVisibleDebug : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return gAgent.isGodlike();
	}
};

class LLAvatarEnableDebug : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return gAgent.isGodlike();
	}
};

class LLAvatarDebug : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if( avatar )
		{
			if (avatar->isSelf())
			{
				((LLVOAvatarSelf *)avatar)->dumpLocalTextures();
			}
			llinfos << "Dumping temporary asset data to simulator logs for avatar " << avatar->getID() << llendl;
			std::vector<std::string> strings;
			strings.push_back(avatar->getID().asString());
			LLUUID invoice;
			send_generic_message("dumptempassetdata", strings, invoice);
			LLFloaterReg::showInstance( "avatar_tetures", LLSD(avatar->getID()) );
		}
		return true;
	}
};

bool callback_eject(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (2 == option)
	{
		// Cancel button.
		return false;
	}
	LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
	bool ban_enabled = notification["payload"]["ban_enabled"].asBoolean();

	if (0 == option)
	{
		// Eject button
		LLMessageSystem* msg = gMessageSystem;
		LLViewerObject* avatar = gObjectList.findObject(avatar_id);

		if (avatar)
		{
			U32 flags = 0x0;
			msg->newMessage("EjectUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID() );
			msg->addUUID("SessionID", gAgent.getSessionID() );
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id );
			msg->addU32("Flags", flags );
			msg->sendReliable( avatar->getRegion()->getHost() );
		}
	}
	else if (ban_enabled)
	{
		// This is tricky. It is similar to say if it is not an 'Eject' button,
		// and it is also not an 'Cancle' button, and ban_enabled==ture, 
		// it should be the 'Eject and Ban' button.
		LLMessageSystem* msg = gMessageSystem;
		LLViewerObject* avatar = gObjectList.findObject(avatar_id);

		if (avatar)
		{
			U32 flags = 0x1;
			msg->newMessage("EjectUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID() );
			msg->addUUID("SessionID", gAgent.getSessionID() );
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id );
			msg->addU32("Flags", flags );
			msg->sendReliable( avatar->getRegion()->getHost() );
		}
	}
	return false;
}

class LLAvatarEject : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if( avatar )
		{
			LLSD payload;
			payload["avatar_id"] = avatar->getID();
			std::string fullname = avatar->getFullname();

			const LLVector3d& pos = avatar->getPositionGlobal();
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos)->getParcel();
			
			if (LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_MANAGE_BANNED))
			{
                payload["ban_enabled"] = true;
				if (!fullname.empty())
				{
    				LLSD args;
    				args["AVATAR_NAME"] = fullname;
    				LLNotifications::instance().add("EjectAvatarFullname",
    							args,
    							payload,
    							callback_eject);
				}
				else
				{
    				LLNotifications::instance().add("EjectAvatarFullname",
    							LLSD(),
    							payload,
    							callback_eject);
				}
			}
			else
			{
                payload["ban_enabled"] = false;
				if (!fullname.empty())
				{
    				LLSD args;
    				args["AVATAR_NAME"] = fullname;
    				LLNotifications::instance().add("EjectAvatarFullnameNoBan",
    							args,
    							payload,
    							callback_eject);
				}
				else
				{
    				LLNotifications::instance().add("EjectAvatarNoBan",
    							LLSD(),
    							payload,
    							callback_eject);
				}
			}
		}
		return true;
	}
};

class LLAvatarEnableFreezeEject : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		bool new_value = (avatar != NULL);

		if (new_value)
		{
			const LLVector3& pos = avatar->getPositionRegion();
			const LLVector3d& pos_global = avatar->getPositionGlobal();
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos_global)->getParcel();
			LLViewerRegion* region = avatar->getRegion();
			new_value = (region != NULL);
						
			if (new_value)
			{
				new_value = region->isOwnedSelf(pos);
				if (!new_value || region->isOwnedGroup(pos))
				{
					new_value = LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_ADMIN);
				}
			}
		}

		return new_value;
	}
};

class LLAvatarGiveCard : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		llinfos << "handle_give_card()" << llendl;
		LLViewerObject* dest = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if(dest && dest->isAvatar())
		{
			bool found_name = false;
			LLSD args;
			LLSD old_args;
			LLNameValue* nvfirst = dest->getNVPair("FirstName");
			LLNameValue* nvlast = dest->getNVPair("LastName");
			if(nvfirst && nvlast)
			{
				args["FIRST"] = nvfirst->getString();
				args["LAST"] = nvlast->getString();
				old_args["FIRST"] = nvfirst->getString();
				old_args["LAST"] = nvlast->getString();
				found_name = true;
			}
			LLViewerRegion* region = dest->getRegion();
			LLHost dest_host;
			if(region)
			{
				dest_host = region->getHost();
			}
			if(found_name && dest_host.isOk())
			{
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("OfferCallingCard");
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_AgentBlock);
				msg->addUUIDFast(_PREHASH_DestID, dest->getID());
				LLUUID transaction_id;
				transaction_id.generate();
				msg->addUUIDFast(_PREHASH_TransactionID, transaction_id);
				msg->sendReliable(dest_host);
				LLNotifications::instance().add("OfferedCard", args);
			}
			else
			{
				LLNotifications::instance().add("CantOfferCallingCard", old_args);
			}
		}
		return true;
	}
};



void login_done(S32 which, void *user)
{
	llinfos << "Login done " << which << llendl;

	LLPanelLogin::closePanel();
}


bool callback_leave_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLMessageSystem *msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID() );
		gAgent.sendReliableMessage();
	}
	return false;
}

void append_aggregate(std::string& string, const LLAggregatePermissions& ag_perm, PermissionBit bit, const char* txt)
{
	LLAggregatePermissions::EValue val = ag_perm.getValue(bit);
	std::string buffer;
	switch(val)
	{
	  case LLAggregatePermissions::AP_NONE:
		buffer = llformat( "* %s None\n", txt);
		break;
	  case LLAggregatePermissions::AP_SOME:
		buffer = llformat( "* %s Some\n", txt);
		break;
	  case LLAggregatePermissions::AP_ALL:
		buffer = llformat( "* %s All\n", txt);
		break;
	  case LLAggregatePermissions::AP_EMPTY:
	  default:
		break;
	}
	string.append(buffer);
}

BOOL enable_buy(void*)
{
    // In order to buy, there must only be 1 purchaseable object in
    // the selection manger.
	if(LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() != 1) return FALSE;
    LLViewerObject* obj = NULL;
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if(node)
    {
        obj = node->getObject();
        if(!obj) return FALSE;

		if(node->mSaleInfo.isForSale() && node->mPermissions->getMaskOwner() & PERM_TRANSFER &&
			(node->mPermissions->getMaskOwner() & PERM_COPY || node->mSaleInfo.getSaleType() != LLSaleInfo::FS_COPY))
		{
			if(obj->permAnyOwner()) return TRUE;
		}
    }
	return FALSE;
}

class LLObjectEnableBuy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = enable_buy(NULL);
		return new_value;
	}
};

// Note: This will only work if the selected object's data has been
// received by the viewer and cached in the selection manager.
void handle_buy_object(LLSaleInfo sale_info)
{
	if(!LLSelectMgr::getInstance()->selectGetAllRootsValid())
	{
		LLNotifications::instance().add("UnableToBuyWhileDownloading");
		return;
	}

	LLUUID owner_id;
	std::string owner_name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		LLNotifications::instance().add("CannotBuyObjectsFromDifferentOwners");
		return;
	}

	LLPermissions perm;
	BOOL valid = LLSelectMgr::getInstance()->selectGetPermissions(perm);
	LLAggregatePermissions ag_perm;
	valid &= LLSelectMgr::getInstance()->selectGetAggregatePermissions(ag_perm);
	if(!valid || !sale_info.isForSale() || !perm.allowTransferTo(gAgent.getID()))
	{
		LLNotifications::instance().add("ObjectNotForSale");
		return;
	}

	S32 price = sale_info.getSalePrice();
	
	if (price > 0 && price > gStatusBar->getBalance())
	{
		LLFloaterBuyCurrency::buyCurrency("This object costs", price);
		return;
	}

	LLFloaterBuy::show(sale_info);
}


void handle_buy_contents(LLSaleInfo sale_info)
{
	LLFloaterBuyContents::show(sale_info);
}

void handle_region_dump_temp_asset_data(void*)
{
	llinfos << "Dumping temporary asset data to simulator logs" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("dumptempassetdata", strings, invoice);
}

void handle_region_clear_temp_asset_data(void*)
{
	llinfos << "Clearing temporary asset data" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("cleartempassetdata", strings, invoice);
}

void handle_region_dump_settings(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		llinfos << "Damage:    " << (regionp->getAllowDamage() ? "on" : "off") << llendl;
		llinfos << "Landmark:  " << (regionp->getAllowLandmark() ? "on" : "off") << llendl;
		llinfos << "SetHome:   " << (regionp->getAllowSetHome() ? "on" : "off") << llendl;
		llinfos << "ResetHome: " << (regionp->getResetHomeOnTeleport() ? "on" : "off") << llendl;
		llinfos << "SunFixed:  " << (regionp->getSunFixed() ? "on" : "off") << llendl;
		llinfos << "BlockFly:  " << (regionp->getBlockFly() ? "on" : "off") << llendl;
		llinfos << "AllowP2P:  " << (regionp->getAllowDirectTeleport() ? "on" : "off") << llendl;
		llinfos << "Water:     " << (regionp->getWaterHeight()) << llendl;
	}
}

void handle_dump_group_info(void *)
{
	gAgent.dumpGroupInfo();
}

void handle_dump_capabilities_info(void *)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->logActiveCapabilities();
	}
}

void handle_dump_region_object_cache(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->dumpCache();
	}
}

void handle_dump_focus(void *)
{
	LLUICtrl *ctrl = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());

	llinfos << "Keyboard focus " << (ctrl ? ctrl->getName() : "(none)") << llendl;
}

class LLSelfStandUp : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.standUp();
		return true;
	}
};

class LLSelfEnableStandUp : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gAgent.getAvatarObject() && gAgent.getAvatarObject()->isSitting();
		return new_value;
	}
};

class LLSelfFriends : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// Open "Friends" tab of the "People" panel in side tray.
		LLSD param;
		param["people_panel_tab_name"] = "friends_panel";

		LLSideTray::getInstance()->showPanel("panel_people", param);
		return true;
	}
};

class LLSelfGroups : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// Open "Groups" tab of the "People" panel in side tray.
		LLSD param;
		param["people_panel_tab_name"] = "groups_panel";
		LLSideTray::getInstance()->showPanel("panel_people", param);
		return true;
	}
};

BOOL check_admin_override(void*)
{
	return gAgent.getAdminOverride();
}

void handle_admin_override_toggle(void*)
{
	gAgent.setAdminOverride(!gAgent.getAdminOverride());

	// The above may have affected which debug menus are visible
	show_debug_menus();
}

void handle_god_mode(void*)
{
	gAgent.requestEnterGodMode();
}

void handle_leave_god_mode(void*)
{
	gAgent.requestLeaveGodMode();
}

void set_god_level(U8 god_level)
{
	U8 old_god_level = gAgent.getGodLevel();
	gAgent.setGodLevel( god_level );
	gIMMgr->refresh();
	LLViewerParcelMgr::getInstance()->notifyObservers();

	// Some classifieds change visibility on god mode
	LLFloaterDirectory::requestClassifieds();

	// God mode changes sim visibility
	LLWorldMap::getInstance()->reset();
	LLWorldMap::getInstance()->setCurrentLayer(0);

	// inventory in items may change in god mode
	gObjectList.dirtyAllObjectInventory();

        if(gViewerWindow)
        {
            gViewerWindow->setMenuBackgroundColor(god_level > GOD_NOT,
            LLViewerLogin::getInstance()->isInProductionGrid());
        }
    
        LLSD args;
	if(god_level > GOD_NOT)
	{
		args["LEVEL"] = llformat("%d",(S32)god_level);
		LLNotifications::instance().add("EnteringGodMode", args);
	}
	else
	{
		args["LEVEL"] = llformat("%d",(S32)old_god_level);
		LLNotifications::instance().add("LeavingGodMode", args);
	}

	// changing god-level can affect which menus we see
	show_debug_menus();
}

#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
void handle_toggle_hacked_godmode(void*)
{
	gHackGodmode = !gHackGodmode;
	set_god_level(gHackGodmode ? GOD_MAINTENANCE : GOD_NOT);
}

BOOL check_toggle_hacked_godmode(void*)
{
	return gHackGodmode;
}

bool enable_toggle_hacked_godmode(void*)
{
  return !LLViewerLogin::getInstance()->isInProductionGrid();
}
#endif

void process_grant_godlike_powers(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((agent_id == gAgent.getID()) && (session_id == gAgent.getSessionID()))
	{
		U8 god_level;
		msg->getU8Fast(_PREHASH_GrantData, _PREHASH_GodLevel, god_level);
		set_god_level(god_level);
	}
	else
	{
		llwarns << "Grant godlike for wrong agent " << agent_id << llendl;
	}
}

/*
class LLHaveCallingcard : public LLInventoryCollectFunctor
{
public:
	LLHaveCallingcard(const LLUUID& agent_id);
	virtual ~LLHaveCallingcard() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL isThere() const { return mIsThere;}
protected:
	LLUUID mID;
	BOOL mIsThere;
};

LLHaveCallingcard::LLHaveCallingcard(const LLUUID& agent_id) :
	mID(agent_id),
	mIsThere(FALSE)
{
}

bool LLHaveCallingcard::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CALLINGCARD)
		   && (item->getCreatorUUID() == mID))
		{
			mIsThere = TRUE;
		}
	}
	return FALSE;
}
*/

BOOL is_agent_mappable(const LLUUID& agent_id)
{
	return (LLAvatarActions::isFriend(agent_id) &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isOnline() &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)
		);
}


// Enable a menu item when you don't have someone's card.
class LLAvatarEnableAddFriend : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		bool new_value = avatar && !LLAvatarActions::isFriend(avatar->getID());
		return new_value;
	}
};

void request_friendship(const LLUUID& dest_id)
{
	LLViewerObject* dest = gObjectList.findObject(dest_id);
	if(dest && dest->isAvatar())
	{
		std::string fullname;
		LLSD args;
		LLNameValue* nvfirst = dest->getNVPair("FirstName");
		LLNameValue* nvlast = dest->getNVPair("LastName");
		if(nvfirst && nvlast)
		{
			args["FIRST"] = nvfirst->getString();
			args["LAST"] = nvlast->getString();
			fullname = nvfirst->getString();
			fullname += " ";
			fullname += nvlast->getString();
		}
		if (!fullname.empty())
		{
			LLAvatarActions::requestFriendshipDialog(dest_id, fullname);
		}
		else
		{
			LLNotifications::instance().add("CantOfferFriendship");
		}
	}
}


class LLEditEnableCustomizeAvatar : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gAgentWearables.areWearablesLoaded();
		return new_value;
	}
};

// only works on pie menu
bool handle_sit_or_stand()
{
	LLPickInfo pick = LLToolPie::getInstance()->getPick();
	LLViewerObject *object = pick.getObject();;
	if (!object || pick.mPickType == LLPickInfo::PICK_FLORA)
	{
		return true;
	}

	if (sitting_on_selection())
	{
		gAgent.standUp();
		return true;
	}

	// get object selection offset 

	if (object && object->getPCode() == LL_PCODE_VOLUME)
	{

		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset, pick.mObjectOffset);

		object->getRegion()->sendReliableMessage();
	}
	return true;
}

class LLObjectSitOrStand : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return handle_sit_or_stand();
	}
};

void near_sit_down_point(BOOL success, void *)
{
	if (success)
	{
		gAgent.setFlying(FALSE);
		gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);

		// Might be first sit
		LLFirstUse::useSit();
	}
}

class LLLandSit : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.standUp();
		LLViewerParcelMgr::getInstance()->deselectLand();

		LLVector3d posGlobal = LLToolPie::getInstance()->getPick().mPosGlobal;
		
		LLQuaternion target_rot;
		if (gAgent.getAvatarObject())
		{
			target_rot = gAgent.getAvatarObject()->getRotation();
		}
		else
		{
			target_rot = gAgent.getFrameAgent().getQuaternion();
		}
		gAgent.startAutoPilotGlobal(posGlobal, "Sit", &target_rot, near_sit_down_point, NULL, 0.7f);
		return true;
	}
};

//-------------------------------------------------------------------
// Help menu functions
//-------------------------------------------------------------------

//
// Major mode switching
//
void reset_view_final( BOOL proceed );

void handle_reset_view()
{
	if( (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode()) && gFloaterCustomize )
	{
		// Show dialog box if needed.
		gFloaterCustomize->askToSaveIfDirty( reset_view_final );
	}
	else
	{
		reset_view_final( TRUE );
		LLFloaterCamera::resetCameraMode();
	}
}

class LLViewResetView : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_reset_view();
		return true;
	}
};

// Note: extra parameters allow this function to be called from dialog.
void reset_view_final( BOOL proceed ) 
{
	if( !proceed )
	{
		return;
	}

	gAgent.resetView(TRUE, TRUE);
}

class LLViewLookAtLastChatter : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.lookAtLastChat();
		return true;
	}
};

class LLViewMouselook : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (!gAgent.cameraMouselook())
		{
			gAgent.changeCameraToMouselook();
		}
		else
		{
			gAgent.changeCameraToDefault();
		}
		return true;
	}
};

class LLViewFullscreen : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gViewerWindow->toggleFullscreen(TRUE);
		return true;
	}
};

class LLViewDefaultUISize : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gSavedSettings.setF32("UIScaleFactor", 1.0f);
		gSavedSettings.setBOOL("UIAutoScale", FALSE);	
		gViewerWindow->reshape(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		return true;
	}
};

class LLEditDuplicate : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if(LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->duplicate();
		}
		return true;
	}
};

class LLEditEnableDuplicate : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDuplicate();
		return new_value;
	}
};

void handle_duplicate_in_place(void*)
{
	llinfos << "handle_duplicate_in_place" << llendl;

	LLVector3 offset(0.f, 0.f, 0.f);
	LLSelectMgr::getInstance()->selectDuplicate(offset, TRUE);
}

/* dead code 30-apr-2008
void handle_deed_object_to_group(void*)
{
	LLUUID group_id;
	
	LLSelectMgr::getInstance()->selectGetGroup(group_id);
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_RELEASE_COUNT);
}

BOOL enable_deed_object_to_group(void*)
{
	if(LLSelectMgr::getInstance()->getSelection()->isEmpty()) return FALSE;
	LLPermissions perm;
	LLUUID group_id;

	if (LLSelectMgr::getInstance()->selectGetGroup(group_id) &&
		gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) &&
		LLSelectMgr::getInstance()->selectGetPermissions(perm) &&
		perm.deedToGroup(gAgent.getID(), group_id))
	{
		return TRUE;
	}
	return FALSE;
}

*/


/*
 * No longer able to support viewer side manipulations in this way
 *
void god_force_inv_owner_permissive(LLViewerObject* object,
									InventoryObjectList* inventory,
									S32 serial_num,
									void*)
{
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	item_array_t items;

	InventoryObjectList::const_iterator inv_it = inventory->begin();
	InventoryObjectList::const_iterator inv_end = inventory->end();
	for ( ; inv_it != inv_end; ++inv_it)
	{
		if(((*inv_it)->getType() != LLAssetType::AT_CATEGORY)
		   && ((*inv_it)->getType() != LLAssetType::AT_ROOT_CATEGORY))
		{
			LLInventoryObject* obj = *inv_it;
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem((LLViewerInventoryItem*)obj);
			LLPermissions perm(new_item->getPermissions());
			perm.setMaskBase(PERM_ALL);
			perm.setMaskOwner(PERM_ALL);
			new_item->setPermissions(perm);
			items.push_back(new_item);
		}
	}
	item_array_t::iterator end = items.end();
	item_array_t::iterator it;
	for(it = items.begin(); it != end; ++it)
	{
		// since we have the inventory item in the callback, it should not
		// invalidate iteration through the selection manager.
		object->updateInventory((*it), TASK_INVENTORY_ITEM_KEY, false);
	}
}
*/

void handle_object_owner_permissive(void*)
{
	// only send this if they're a god.
	if(gAgent.isGodlike())
	{
		// do the objects.
		LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_BASE, TRUE, PERM_ALL, TRUE);
		LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_OWNER, TRUE, PERM_ALL, TRUE);
	}
}

void handle_object_owner_self(void*)
{
	// only send this if they're a god.
	if(gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendOwner(gAgent.getID(), gAgent.getGroupID(), TRUE);
	}
}

// Shortcut to set owner permissions to not editable.
void handle_object_lock(void*)
{
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_OWNER, FALSE, PERM_MODIFY);
}

void handle_object_asset_ids(void*)
{
	// only send this if they're a god.
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendGodlikeRequest("objectinfo", "assetids");
	}
}

void handle_force_parcel_owner_to_me(void*)
{
	LLViewerParcelMgr::getInstance()->sendParcelGodForceOwner( gAgent.getID() );
}

void handle_force_parcel_to_content(void*)
{
	LLViewerParcelMgr::getInstance()->sendParcelGodForceToContent();
}

void handle_claim_public_land(void*)
{
	if (LLViewerParcelMgr::getInstance()->getSelectionRegion() != gAgent.getRegion())
	{
		LLNotifications::instance().add("ClaimPublicLand");
		return;
	}

	LLVector3d west_south_global;
	LLVector3d east_north_global;
	LLViewerParcelMgr::getInstance()->getSelection(west_south_global, east_north_global);
	LLVector3 west_south = gAgent.getPosAgentFromGlobal(west_south_global);
	LLVector3 east_north = gAgent.getPosAgentFromGlobal(east_north_global);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GodlikeMessage");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", "claimpublicland");
	msg->addUUID("Invoice", LLUUID::null);
	std::string buffer;
	buffer = llformat( "%f", west_south.mV[VX]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat( "%f", west_south.mV[VY]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat( "%f", east_north.mV[VX]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat( "%f", east_north.mV[VY]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	gAgent.sendReliableMessage();
}



// HACK for easily testing new avatar geometry
void handle_god_request_avatar_geometry(void *)
{
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendGodlikeRequest("avatar toggle", NULL);
	}
}


void derez_objects(EDeRezDestination dest, const LLUUID& dest_id)
{
	if(gAgent.cameraMouselook())
	{
		gAgent.changeCameraToDefault();
	}
	//gInventoryView->setPanelOpen(TRUE);

	std::string error;
	LLDynamicArray<LLViewerObject*> derez_objects;
	
	// Check conditions that we can't deal with, building a list of
	// everything that we'll actually be derezzing.
	LLViewerRegion* first_region = NULL;
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		LLViewerRegion* region = object->getRegion();
		if (!first_region)
		{
			first_region = region;
		}
		else
		{
			if(region != first_region)
			{
				// Derez doesn't work at all if the some of the objects
				// are in regions besides the first object selected.
				
				// ...crosses region boundaries
				error = "AcquireErrorObjectSpan";
				break;
			}
		}
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

		// If AssetContainers are being sent back, they will appear as 
		// boxes in the owner's inventory.
		if (object->getNVPair("AssetContainer")
			&& dest != DRD_RETURN_TO_OWNER)
		{
			// this object is an asset container, derez its contents, not it
			llwarns << "Attempt to derez deprecated AssetContainer object type not supported." << llendl;
			/*
			object->requestInventory(container_inventory_arrived, 
				(void *)(BOOL)(DRD_TAKE_INTO_AGENT_INVENTORY == dest));
			*/
			continue;
		}
		BOOL can_derez_current = FALSE;
		switch(dest)
		{
		case DRD_TAKE_INTO_AGENT_INVENTORY:
		case DRD_TRASH:
			if( (node->mPermissions->allowTransferTo(gAgent.getID()) && object->permModify())
				|| (node->allowOperationOnNode(PERM_OWNER, GP_OBJECT_MANIPULATE)) )
			{
				can_derez_current = TRUE;
			}
			break;

		case DRD_RETURN_TO_OWNER:
			can_derez_current = TRUE;
			break;

		default:
			if((node->mPermissions->allowTransferTo(gAgent.getID())
				&& object->permCopy())
			   || gAgent.isGodlike())
			{
				can_derez_current = TRUE;
			}
			break;
		}
		if(can_derez_current)
		{
			derez_objects.put(object);
		}
	}

	// This constant is based on (1200 - HEADER_SIZE) / 4 bytes per
	// root.  I lopped off a few (33) to provide a bit
	// pad. HEADER_SIZE is currently 67 bytes, most of which is UUIDs.
	// This gives us a maximum of 63500 root objects - which should
	// satisfy anybody.
	const S32 MAX_ROOTS_PER_PACKET = 250;
	const S32 MAX_PACKET_COUNT = 254;
	F32 packets = ceil((F32)derez_objects.count() / (F32)MAX_ROOTS_PER_PACKET);
	if(packets > (F32)MAX_PACKET_COUNT)
	{
		error = "AcquireErrorTooManyObjects";
	}

	if(error.empty() && derez_objects.count() > 0)
	{
		U8 d = (U8)dest;
		LLUUID tid;
		tid.generate();
		U8 packet_count = (U8)packets;
		S32 object_index = 0;
		S32 objects_in_packet = 0;
		LLMessageSystem* msg = gMessageSystem;
		for(U8 packet_number = 0;
			packet_number < packet_count;
			++packet_number)
		{
			msg->newMessageFast(_PREHASH_DeRezObject);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_AgentBlock);
			msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
			msg->addU8Fast(_PREHASH_Destination, d);	
			msg->addUUIDFast(_PREHASH_DestinationID, dest_id);
			msg->addUUIDFast(_PREHASH_TransactionID, tid);
			msg->addU8Fast(_PREHASH_PacketCount, packet_count);
			msg->addU8Fast(_PREHASH_PacketNumber, packet_number);
			objects_in_packet = 0;
			while((object_index < derez_objects.count())
				  && (objects_in_packet++ < MAX_ROOTS_PER_PACKET))

			{
				LLViewerObject* object = derez_objects.get(object_index++);
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
				// VEFFECT: DerezObject
				LLHUDEffectSpiral* effectp = (LLHUDEffectSpiral*)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
				effectp->setPositionGlobal(object->getPositionGlobal());
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			}
			msg->sendReliable(first_region->getHost());
		}
		make_ui_sound("UISndObjectRezOut");

		// Busy count decremented by inventory update, so only increment
		// if will be causing an update.
		if (dest != DRD_RETURN_TO_OWNER)
		{
			gViewerWindow->getWindow()->incBusyCount();
		}
	}
	else if(!error.empty())
	{
		LLNotifications::instance().add(error);
	}
}

class LLToolsTakeCopy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return true;

		const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
		derez_objects(DRD_ACQUIRE_TO_AGENT_INVENTORY, category_id);

		return true;
	}
};


// You can return an object to its owner if it is on your land.
class LLObjectReturn : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return true;
		
		mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();

		LLNotifications::instance().add("ReturnToOwner", LLSD(), LLSD(), boost::bind(&LLObjectReturn::onReturnToOwner, this, _1, _2));
		return true;
	}

	bool onReturnToOwner(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotification::getSelectedOption(notification, response);
		if (0 == option)
		{
			// Ignore category ID for this derez destination.
			derez_objects(DRD_RETURN_TO_OWNER, LLUUID::null);
		}

		// drop reference to current selection
		mObjectSelection = NULL;
		return false;
	}

protected:
	LLObjectSelectionHandle mObjectSelection;
};


// Allow return to owner if one or more of the selected items is
// over land you own.
class LLObjectEnableReturn : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
#ifdef HACKED_GODLIKE_VIEWER
		bool new_value = true;
#else
		bool new_value = false;
		if (gAgent.isGodlike())
		{
			new_value = true;
		}
		else
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (region)
			{
				// Estate owners and managers can always return objects.
				if (region->canManageEstate())
				{
					new_value = true;
				}
				else
				{
					struct f : public LLSelectedObjectFunctor
					{
						virtual bool apply(LLViewerObject* obj)
						{
							return (obj->isOverAgentOwnedLand() ||
									obj->isOverGroupOwnedLand() ||
									obj->permModify());
						}
					} func;
					const bool firstonly = true;
					new_value = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
				}
			}
		}
#endif
		return new_value;
	}
};

void force_take_copy(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return;
	const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
	derez_objects(DRD_FORCE_TO_GOD_INVENTORY, category_id);
}

void handle_take()
{
	// we want to use the folder this was derezzed from if it's
	// available. Otherwise, derez to the normal place.
	if(LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		return;
	}
	
	BOOL you_own_everything = TRUE;
	BOOL locked_but_takeable_object = FALSE;
	LLUUID category_id;
	
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if(object)
		{
			if(!object->permYouOwner())
			{
				you_own_everything = FALSE;
			}

			if(!object->permMove())
			{
				locked_but_takeable_object = TRUE;
			}
		}
		if(node->mFolderID.notNull())
		{
			if(category_id.isNull())
			{
				category_id = node->mFolderID;
			}
			else if(category_id != node->mFolderID)
			{
				// we have found two potential destinations. break out
				// now and send to the default location.
				category_id.setNull();
				break;
			}
		}
	}
	if(category_id.notNull())
	{
		// there is an unambiguous destination. See if this agent has
		// such a location and it is not in the trash or library
		if(!gInventory.getCategory(category_id))
		{
			// nope, set to NULL.
			category_id.setNull();
		}
		if(category_id.notNull())
		{
		        // check trash
			LLUUID trash;
			trash = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
			if(category_id == trash || gInventory.isObjectDescendentOf(category_id, trash))
			{
				category_id.setNull();
			}

			// check library
			if(gInventory.isObjectDescendentOf(category_id, gInventory.getLibraryRootFolderID()))
			{
				category_id.setNull();
			}

		}
	}
	if(category_id.isNull())
	{
		category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
	}
	LLSD payload;
	payload["folder_id"] = category_id;

	LLNotification::Params params("ConfirmObjectTakeLock");
	params.payload(payload);
	params.functor.function(confirm_take);

	if(locked_but_takeable_object ||
	   !you_own_everything)
	{
		if(locked_but_takeable_object && you_own_everything)
		{
			params.name("ConfirmObjectTakeLock");
		}
		else if(!locked_but_takeable_object && !you_own_everything)
		{
			params.name("ConfirmObjectTakeNoOwn");
		}
		else
		{
			params.name("ConfirmObjectTakeLockNoOwn");
		}
	
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}

bool confirm_take(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if(enable_take() && (option == 0))
	{
		derez_objects(DRD_TAKE_INTO_AGENT_INVENTORY, notification["payload"]["folder_id"].asUUID());
	}
	return false;
}

// You can take an item when it is public and transferrable, or when
// you own it. We err on the side of enabling the item when at least
// one item selected can be copied to inventory.
BOOL enable_take()
{
	if (sitting_on_selection())
	{
		return FALSE;
	}

	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLViewerLogin::getInstance()->isInProductionGrid() 
            && gAgent.isGodlike())
		{
			return TRUE;
		}
# endif
		if((node->mPermissions->allowTransferTo(gAgent.getID())
			&& object->permModify())
		   || (node->mPermissions->getOwner() == gAgent.getID()))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}

class LLToolsBuyOrTake : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty())
		{
			return true;
		}

		if (is_selection_buy_not_take())
		{
			S32 total_price = selection_price();

			if (total_price <= gStatusBar->getBalance() || total_price == 0)
			{
				handle_buy(NULL);
			}
			else
			{
				LLFloaterBuyCurrency::buyCurrency(
					"Buying this costs", total_price);
			}
		}
		else
		{
			handle_take();
		}
		return true;
	}
};

class LLToolsEnableBuyOrTake : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool is_buy = is_selection_buy_not_take();
		bool new_value = is_buy ? enable_buy(NULL) : enable_take();

		// Update label
		std::string label;
		std::string buy_text;
		std::string take_text;
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			buy_text = param.substr(0, offset);
			take_text = param.substr(offset+1);
		}
		if (is_buy)
		{
			label = buy_text;
		}
		else
		{
			label = take_text;
		}
		gMenuHolder->childSetText("Pie Object Take", label);
		gMenuHolder->childSetText("Menu Object Take", label);

		return new_value;
	}
};

// This is a small helper function to determine if we have a buy or a
// take in the selection. This method is to help with the aliasing
// problems of putting buy and take in the same pie menu space. After
// a fair amont of discussion, it was determined to prefer buy over
// take. The reasoning follows from the fact that when users walk up
// to buy something, they will click on one or more items. Thus, if
// anything is for sale, it becomes a buy operation, and the server
// will group all of the buy items, and copyable/modifiable items into
// one package and give the end user as much as the permissions will
// allow. If the user wanted to take something, they will select fewer
// and fewer items until only 'takeable' items are left. The one
// exception is if you own everything in the selection that is for
// sale, in this case, you can't buy stuff from yourself, so you can
// take it.
// return value = TRUE if selection is a 'buy'.
//                FALSE if selection is a 'take'
BOOL is_selection_buy_not_take()
{
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if(obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale, thus,
			// it's a buy
			return TRUE;
		}
	}
	return FALSE;
}

S32 selection_price()
{
	S32 total_price = 0;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if(obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale.
			// Add its price.
			total_price += node->mSaleInfo.getSalePrice();
		}
	}

	return total_price;
}
/*
bool callback_show_buy_currency(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		llinfos << "Loading page " << LLNotifications::instance().getGlobalString("BUY_CURRENCY_URL") << llendl;
		LLWeb::loadURL(LLNotifications::instance().getGlobalString("BUY_CURRENCY_URL"));
	}
	return false;
}
*/

void show_buy_currency(const char* extra)
{
	// Don't show currency web page for branded clients.
/*
	std::ostringstream mesg;
	if (extra != NULL)
	{	
		mesg << extra << "\n \n";
	}
	mesg << "Go to " << LLNotifications::instance().getGlobalString("BUY_CURRENCY_URL")<< "\nfor information on purchasing currency?";
*/
	LLSD args;
	if (extra != NULL)
	{
		args["EXTRA"] = extra;
	}
	LLNotifications::instance().add("PromptGoToCurrencyPage", args);//, LLSD(), callback_show_buy_currency);
}

void handle_buy(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return;

	LLSaleInfo sale_info;
	BOOL valid = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
	if (!valid) return;

	if (sale_info.getSaleType() == LLSaleInfo::FS_CONTENTS)
	{
		handle_buy_contents(sale_info);
	}
	else
	{
		handle_buy_object(sale_info);
	}
}

class LLObjectBuy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		handle_buy(NULL);
		return true;
	}
};

BOOL sitting_on_selection()
{
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if (!node)
	{
		return FALSE;
	}

	if (!node->mValid)
	{
		return FALSE;
	}

	LLViewerObject* root_object = node->getObject();
	if (!root_object)
	{
		return FALSE;
	}

	// Need to determine if avatar is sitting on this object
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (!avatar)
	{
		return FALSE;
	}

	return (avatar->isSitting() && avatar->getRoot() == root_object);
}

class LLToolsSaveToInventory : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if(enable_save_into_inventory(NULL))
		{
			derez_objects(DRD_SAVE_INTO_AGENT_INVENTORY, LLUUID::null);
		}
		return true;
	}
};

class LLToolsSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
		if(node && (node->mValid) && (!node->mFromTaskID.isNull()))
		{
			// *TODO: check to see if the fromtaskid object exists.
			derez_objects(DRD_SAVE_INTO_TASK_INVENTORY, node->mFromTaskID);
		}
		return true;
	}
};

// Round the position of all root objects to the grid
class LLToolsSnapObjectXY : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		F64 snap_size = (F64)gSavedSettings.getF32("GridResolution");

		for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
			 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
		{
			LLSelectNode* node = *iter;
			LLViewerObject* obj = node->getObject();
			if (obj->permModify())
			{
				LLVector3d pos_global = obj->getPositionGlobal();
				F64 round_x = fmod(pos_global.mdV[VX], snap_size);
				if (round_x < snap_size * 0.5)
				{
					// closer to round down
					pos_global.mdV[VX] -= round_x;
				}
				else
				{
					// closer to round up
					pos_global.mdV[VX] -= round_x;
					pos_global.mdV[VX] += snap_size;
				}

				F64 round_y = fmod(pos_global.mdV[VY], snap_size);
				if (round_y < snap_size * 0.5)
				{
					pos_global.mdV[VY] -= round_y;
				}
				else
				{
					pos_global.mdV[VY] -= round_y;
					pos_global.mdV[VY] += snap_size;
				}

				obj->setPositionGlobal(pos_global, FALSE);
			}
		}
		LLSelectMgr::getInstance()->sendMultipleUpdate(UPD_POSITION);
		return true;
	}
};

// in order to link, all objects must have the same owner, and the
// agent must have the ability to modify all of the objects. However,
// we're not answering that question with this method. The question
// we're answering is: does the user have a reasonable expectation
// that a link operation should work? If so, return true, false
// otherwise. this allows the handle_link method to more finely check
// the selection and give an error message when the uer has a
// reasonable expectation for the link to work, but it will fail.
class LLToolsEnableLink : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = false;
		// check if there are at least 2 objects selected, and that the
		// user can modify at least one of the selected objects.

		// in component mode, can't link
		if (!gSavedSettings.getBOOL("EditLinkedParts"))
		{
			if(LLSelectMgr::getInstance()->selectGetAllRootsValid() && LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() >= 2)
			{
				struct f : public LLSelectedObjectFunctor
				{
					virtual bool apply(LLViewerObject* object)
					{
						return object->permModify();
					}
				} func;
				const bool firstonly = true;
				new_value = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
			}
		}
		return new_value;
	}
};

class LLToolsLink : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if(!LLSelectMgr::getInstance()->selectGetAllRootsValid())
		{
			LLNotifications::instance().add("UnableToLinkWhileDownloading");
			return true;
		}

		S32 object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		if (object_count > MAX_CHILDREN_PER_TASK + 1)
		{
			LLSD args;
			args["COUNT"] = llformat("%d", object_count);
			int max = MAX_CHILDREN_PER_TASK+1;
			args["MAX"] = llformat("%d", max);
			LLNotifications::instance().add("UnableToLinkObjects", args);
			return true;
		}

		if(LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() < 2)
		{
			LLNotifications::instance().add("CannotLinkIncompleteSet");
			return true;
		}
		if(!LLSelectMgr::getInstance()->selectGetRootsModify())
		{
			LLNotifications::instance().add("CannotLinkModify");
			return true;
		}
		LLUUID owner_id;
		std::string owner_name;
		if(!LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name))
		{
			// we don't actually care if you're the owner, but novices are
			// the most likely to be stumped by this one, so offer the
			// easiest and most likely solution.
			LLNotifications::instance().add("CannotLinkDifferentOwners");
			return true;
		}
		LLSelectMgr::getInstance()->sendLink();
		return true;
	}
};

class LLToolsEnableUnlink : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->selectGetAllRootsValid() &&
			LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject() &&
			!LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject()->isAttachment();
		return new_value;
	}
};

class LLToolsUnlink : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLSelectMgr::getInstance()->sendDelink();
		return true;
	}
};


class LLToolsStopAllAnimations : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.stopCurrentAnimations();
		return true;
	}
};

class LLToolsReleaseKeys : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.forceReleaseControls();

		return true;
	}
};

class LLToolsEnableReleaseKeys : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return gAgent.anyControlGrabbed();
	}
};


class LLEditEnableCut : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canCut();
		return new_value;
	}
};

class LLEditCut : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler )
		{
			LLEditMenuHandler::gEditMenuHandler->cut();
		}
		return true;
	}
};

class LLEditEnableCopy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canCopy();
		return new_value;
	}
};

class LLEditCopy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler )
		{
			LLEditMenuHandler::gEditMenuHandler->copy();
		}
		return true;
	}
};

class LLEditEnablePaste : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canPaste();
		return new_value;
	}
};

class LLEditPaste : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler )
		{
			LLEditMenuHandler::gEditMenuHandler->paste();
		}
		return true;
	}
};

class LLEditEnableDelete : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDoDelete();
		return new_value;
	}
};

class LLEditDelete : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// If a text field can do a deletion, it gets precedence over deleting
		// an object in the world.
		if( LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDoDelete())
		{
			LLEditMenuHandler::gEditMenuHandler->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide();
		return true;
	}
};

class LLObjectEnableDelete : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = 
#ifdef HACKED_GODLIKE_VIEWER
			TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
			(!LLViewerLogin::getInstance()->isInProductionGrid()
             && gAgent.isGodlike()) ||
# endif
			LLSelectMgr::getInstance()->canDoDelete();
#endif
		return new_value;
	}
};

class LLObjectDelete : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance())
		{
			LLSelectMgr::getInstance()->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide();
		return true;
	}
};

void handle_force_delete(void*)
{
	LLSelectMgr::getInstance()->selectForceDelete();
}

class LLViewEnableJoystickFlycam : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = (gSavedSettings.getBOOL("JoystickEnabled") && gSavedSettings.getBOOL("JoystickFlycamEnabled"));
		return new_value;
	}
};

class LLViewEnableLastChatter : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// *TODO: add check that last chatter is in range
		bool new_value = (gAgent.cameraThirdPerson() && gAgent.getLastChatter().notNull());
		return new_value;
	}
};

class LLEditEnableDeselect : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDeselect();
		return new_value;
	}
};

class LLEditDeselect : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler )
		{
			LLEditMenuHandler::gEditMenuHandler->deselect();
		}
		return true;
	}
};

class LLEditEnableSelectAll : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canSelectAll();
		return new_value;
	}
};


class LLEditSelectAll : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler )
		{
			LLEditMenuHandler::gEditMenuHandler->selectAll();
		}
		return true;
	}
};


class LLEditEnableUndo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canUndo();
		return new_value;
	}
};

class LLEditUndo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canUndo() )
		{
			LLEditMenuHandler::gEditMenuHandler->undo();
		}
		return true;
	}
};

class LLEditEnableRedo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canRedo();
		return new_value;
	}
};

class LLEditRedo : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canRedo() )
		{
			LLEditMenuHandler::gEditMenuHandler->redo();
		}
		return true;
	}
};



void print_object_info(void*)
{
	LLSelectMgr::getInstance()->selectionDump();
}

void print_agent_nvpairs(void*)
{
	LLViewerObject *objectp;

	llinfos << "Agent Name Value Pairs" << llendl;

	objectp = gObjectList.findObject(gAgentID);
	if (objectp)
	{
		objectp->printNameValuePairs();
	}
	else
	{
		llinfos << "Can't find agent object" << llendl;
	}

	llinfos << "Camera at " << gAgent.getCameraPositionGlobal() << llendl;
}

void show_debug_menus()
{
	// this might get called at login screen where there is no menu so only toggle it if one exists
	if ( gMenuBarView )
	{
		BOOL debug = gSavedSettings.getBOOL("UseDebugMenus");
		BOOL qamode = gSavedSettings.getBOOL("QAMode");

		gMenuBarView->setItemVisible("Advanced", debug);
// 		gMenuBarView->setItemEnabled("Advanced", debug); // Don't disable Advanced keyboard shortcuts when hidden
		
		gMenuBarView->setItemVisible("Debug", qamode);
		gMenuBarView->setItemEnabled("Debug", qamode);

		gMenuBarView->setItemVisible("Develop", qamode);
		gMenuBarView->setItemEnabled("Develop", qamode);

		// Server ('Admin') menu hidden when not in godmode.
		const bool show_server_menu = debug && (gAgent.getGodLevel() > GOD_NOT);
		gMenuBarView->setItemVisible("Admin", show_server_menu);
		gMenuBarView->setItemEnabled("Admin", show_server_menu);
	}
	if (gLoginMenuBarView)
	{
		BOOL debug = gSavedSettings.getBOOL("UseDebugMenus");
		gLoginMenuBarView->setItemVisible("Debug", debug);
		gLoginMenuBarView->setItemEnabled("Debug", debug);
	}
}

void toggle_debug_menus(void*)
{
	BOOL visible = ! gSavedSettings.getBOOL("UseDebugMenus");
	gSavedSettings.setBOOL("UseDebugMenus", visible);
	if(visible)
	{
		LLFirstUse::useDebugMenus();
	}
	show_debug_menus();
}


// LLUUID gExporterRequestID;
// std::string gExportDirectory;

// LLUploadDialog *gExportDialog = NULL;

// void handle_export_selected( void * )
// {
// 	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
// 	if (selection->isEmpty())
// 	{
// 		return;
// 	}
// 	llinfos << "Exporting selected objects:" << llendl;

// 	gExporterRequestID.generate();
// 	gExportDirectory = "";

// 	LLMessageSystem* msg = gMessageSystem;
// 	msg->newMessageFast(_PREHASH_ObjectExportSelected);
// 	msg->nextBlockFast(_PREHASH_AgentData);
// 	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
// 	msg->addUUIDFast(_PREHASH_RequestID, gExporterRequestID);
// 	msg->addS16Fast(_PREHASH_VolumeDetail, 4);

// 	for (LLObjectSelection::root_iterator iter = selection->root_begin();
// 		 iter != selection->root_end(); iter++)
// 	{
// 		LLSelectNode* node = *iter;
// 		LLViewerObject* object = node->getObject();
// 		msg->nextBlockFast(_PREHASH_ObjectData);
// 		msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
// 		llinfos << "Object: " << object->getID() << llendl;
// 	}
// 	msg->sendReliable(gAgent.getRegion()->getHost());

// 	gExportDialog = LLUploadDialog::modalUploadDialog("Exporting selected objects...");
// }
//


class LLWorldSetHomeLocation : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// we just send the message and let the server check for failure cases
		// server will echo back a "Home position set." alert if it succeeds
		// and the home location screencapture happens when that alert is recieved
		gAgent.setStartPosition(START_LOCATION_ID_HOME);
		return true;
	}
};

class LLWorldTeleportHome : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gAgent.teleportHome();
		return true;
	}
};

class LLWorldAlwaysRun : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// as well as altering the default walk-vs-run state,
		// we also change the *current* walk-vs-run state.
		if (gAgent.getAlwaysRun())
		{
			gAgent.clearAlwaysRun();
			gAgent.clearRunning();
		}
		else
		{
			gAgent.setAlwaysRun();
			gAgent.setRunning();
		}

		// tell the simulator.
		gAgent.sendWalkRun(gAgent.getAlwaysRun());

		// Update Movement Controls according to AlwaysRun mode
		LLFloaterMove::setAlwaysRunMode(gAgent.getAlwaysRun());

		return true;
	}
};

class LLWorldCheckAlwaysRun : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gAgent.getAlwaysRun();
		return new_value;
	}
};

class LLWorldSetAway : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgent.getAFK())
		{
			gAgent.clearAFK();
		}
		else
		{
			gAgent.setAFK();
		}
		return true;
	}
};

class LLWorldSetBusy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgent.getBusy())
		{
			gAgent.clearBusy();
		}
		else
		{
			gAgent.setBusy();
			LLNotifications::instance().add("BusyModeSet");
		}
		return true;
	}
};

class LLWorldCreateLandmark : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));

		return true;
	}
};

class LLToolsLookAtSelection : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		const F32 PADDING_FACTOR = 2.f;
		BOOL zoom = (userdata.asString() == "zoom");
		if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
		{
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);

			LLBBox selection_bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
			F32 angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getAspect() > 1.f ? LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect() : LLViewerCamera::getInstance()->getView());
			F32 distance = selection_bbox.getExtentLocal().magVec() * PADDING_FACTOR / atan(angle_of_view);

			LLVector3 obj_to_cam = LLViewerCamera::getInstance()->getOrigin() - selection_bbox.getCenterAgent();
			obj_to_cam.normVec();

			LLUUID object_id;
			if (LLSelectMgr::getInstance()->getSelection()->getPrimaryObject())
			{
				object_id = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject()->mID;
			}
			if (zoom)
			{
				gAgent.setCameraPosAndFocusGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal() + LLVector3d(obj_to_cam * distance), 
												LLSelectMgr::getInstance()->getSelectionCenterGlobal(), 
												object_id );
			}
			else
			{
				gAgent.setFocusGlobal( LLSelectMgr::getInstance()->getSelectionCenterGlobal(), object_id );
			}
		}
		return true;
	}
};

void callback_invite_to_group(LLUUID group_id, LLUUID dest_id)
{
	std::vector<LLUUID> agent_ids;
	agent_ids.push_back(dest_id);
	
	LLFloaterGroupInvite::showForGroup(group_id, &agent_ids);
}

void invite_to_group(const LLUUID& dest_id)
{
	LLViewerObject* dest = gObjectList.findObject(dest_id);
	if(dest && dest->isAvatar())
	{
		LLFloaterGroupPicker* widget = LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
		if (widget)
		{
			widget->center();
			widget->setPowersMask(GP_MEMBER_INVITE);
			widget->setSelectGroupCallback(boost::bind(callback_invite_to_group, _1, dest_id));
		}
	}
}

class LLAvatarInviteToGroup : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if(avatar)
		{
			invite_to_group(avatar->getID());
		}
		return true;
	}
};

class LLAvatarAddFriend : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if(avatar && !LLAvatarActions::isFriend(avatar->getID()))
		{
			request_friendship(avatar->getID());
		}
		return true;
	}
};

class LLAvatarAddContact : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if(avatar)
		{
			create_inventory_callingcard(avatar->getID());
		}
		return true;
	}
};

bool complete_give_money(const LLSD& notification, const LLSD& response, LLObjectSelectionHandle handle)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		gAgent.clearBusy();
	}

	LLViewerObject* objectp = handle->getPrimaryObject();

	// Show avatar's name if paying attachment
	if (objectp && objectp->isAttachment())
	{
		while (objectp && !objectp->isAvatar())
		{
			objectp = (LLViewerObject*)objectp->getParent();
		}
	}

	if (objectp)
	{
		if (objectp->isAvatar())
		{
			const BOOL is_group = FALSE;
			LLFloaterPay::payDirectly(&give_money,
									  objectp->getID(),
									  is_group);
		}
		else
		{
			LLFloaterPay::payViaObject(&give_money, objectp->getID());
		}
	}
	return false;
}

bool handle_give_money_dialog()
{
	LLNotification::Params params("BusyModePay");
	params.functor.function(boost::bind(complete_give_money, _1, _2, LLSelectMgr::getInstance()->getSelection()));

	if (gAgent.getBusy())
	{
		// warn users of being in busy mode during a transaction
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 1);
	}
	return true;
}

class LLPayObject : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return handle_give_money_dialog();
	}
};

class LLEnablePayObject : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		bool new_value = (avatar != NULL);
		if (!new_value)
		{
			LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
			if( object )
			{
				LLViewerObject *parent = (LLViewerObject *)object->getParent();
				if((object->flagTakesMoney()) || (parent && parent->flagTakesMoney()))
				{
					new_value = true;
				}
			}
		}
		return new_value;
	}
};

class LLObjectEnableSitOrStand : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = false;
		LLViewerObject* dest_object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();

		if(dest_object)
		{
			if(dest_object->getPCode() == LL_PCODE_VOLUME)
			{
				new_value = true;
			}
		}
		// Update label
		std::string label;
		std::string sit_text;
		std::string stand_text;
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			sit_text = param.substr(0, offset);
			stand_text = param.substr(offset+1);
		}
		if (sitting_on_selection())
		{
			label = stand_text;
		}
		else
		{
			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
			if (node && node->mValid && !node->mSitName.empty())
			{
				label.assign(node->mSitName);
			}
			else
			{
				label = sit_text;
			}
		}
		gMenuHolder->childSetText("Object Sit", label);

		return new_value;
	}
};

void edit_ui(void*)
{
	LLFloater::setEditModeEnabled(!LLFloater::getEditModeEnabled());
}

void dump_select_mgr(void*)
{
	LLSelectMgr::getInstance()->dump();
}

void dump_inventory(void*)
{
	gInventory.dumpInventory();
}


void handle_dump_followcam(void*)
{
	LLFollowCamMgr::dump();
}

void handle_viewer_enable_message_log(void*)
{
	gMessageSystem->startLogging();
}

void handle_viewer_disable_message_log(void*)
{
	gMessageSystem->stopLogging();
}

class LLShowFloater : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string floater_name = userdata.asString();
		if (floater_name == "appearance")
		{
			if (gAgentWearables.areWearablesLoaded())
			{
				gAgent.changeCameraToCustomizeAvatar();
			}
		}
		else if (floater_name == "toolbar")
		{
			LLToolBar::toggle(NULL);
		}
		else if (floater_name == "buy land")
		{
			if (LLViewerParcelMgr::getInstance()->selectionEmpty())
			{
				LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
			}
			
			LLViewerParcelMgr::getInstance()->startBuyLand();
		}
		else if (floater_name == "script errors")
		{
			LLFloaterScriptDebug::show(LLUUID::null);
		}
		else if (floater_name == "help f1")
		{
			llinfos << "Spawning HTML help window" << llendl;
			gViewerHtmlHelp.show();
		}
		else if (floater_name == "complaint reporter")
		{
			// Prevent menu from appearing in screen shot.
			gMenuHolder->hideMenus();
			LLFloaterReporter::showFromMenu(COMPLAINT_REPORT);
		}
		else if (floater_name == "buy currency")
		{
			LLFloaterBuyCurrency::buyCurrency();
		}
		else
		{
			LLFloaterReg::toggleInstance(floater_name);
		}
		return true;
	}
};

class LLFloaterVisible : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string floater_name = userdata.asString();
		bool new_value = false;
		if (floater_name == "toolbar")
		{
			new_value = LLToolBar::visible(NULL);
		}
		else
		{
			new_value = LLFloaterReg::instanceVisible(floater_name);
		}
		return new_value;
	}
};

bool callback_show_url(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"].asString());
	}
	return false;
}

class LLPromptShowURL : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			std::string alert = param.substr(0, offset);
			std::string url = param.substr(offset+1);

			if(gSavedSettings.getBOOL("UseExternalBrowser"))
			{ 
    			LLSD payload;
    			payload["url"] = url;
    			LLNotifications::instance().add(alert, LLSD(), payload, callback_show_url);
			}
			else
			{
		        LLWeb::loadURL(url);
			}
		}
		else
		{
			llinfos << "PromptShowURL invalid parameters! Expecting \"ALERT,URL\"." << llendl;
		}
		return true;
	}
};

bool callback_show_file(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"]);
	}
	return false;
}

class LLPromptShowFile : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			std::string alert = param.substr(0, offset);
			std::string file = param.substr(offset+1);

			LLSD payload;
			payload["url"] = file;
			LLNotifications::instance().add(alert, LLSD(), payload, callback_show_file);
		}
		else
		{
			llinfos << "PromptShowFile invalid parameters! Expecting \"ALERT,FILE\"." << llendl;
		}
		return true;
	}
};

class LLShowAgentProfile : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLUUID agent_id;
		if (userdata.asString() == "agent")
		{
			agent_id = gAgent.getID();
		}
		else if (userdata.asString() == "hit object")
		{
			LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
			if (objectp)
			{
				agent_id = objectp->getID();
			}
		}
		else
		{
			agent_id = userdata.asUUID();
		}

		LLVOAvatar* avatar = find_avatar_from_object(agent_id);
		if (avatar)
		{
			LLAvatarActions::showProfile(avatar->getID());
		}
		return true;
	}
};

class LLLandEdit : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgent.getFocusOnAvatar() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());

			gAgent.cameraOrbitOver( F_PI * 0.25f );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement") )
		{
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}


		LLViewerParcelMgr::getInstance()->selectParcelAt( LLToolPie::getInstance()->getPick().mPosGlobal );

		LLFloaterReg::showInstance("build");

		// Switch to land edit toolset
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolSelectLand::getInstance() );
		return true;
	}
};

class LLWorldEnableBuyLand : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLViewerParcelMgr::getInstance()->canAgentBuyParcel(
								LLViewerParcelMgr::getInstance()->selectionEmpty()
									? LLViewerParcelMgr::getInstance()->getAgentParcel()
									: LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel(),
								false);
		return new_value;
	}
};

BOOL enable_buy_land(void*)
{
	return LLViewerParcelMgr::getInstance()->canAgentBuyParcel(
				LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel(), false);
}



class LLObjectAttachToAvatar : public view_listener_t
{
public:
	static void setObjectSelection(LLObjectSelectionHandle selection) { sObjectSelection = selection; }

private:
	bool handleEvent(const LLSD& userdata)
	{
		setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		LLViewerObject* selectedObject = sObjectSelection->getFirstRootObject();
		if (selectedObject)
		{
			S32 index = userdata.asInteger();
			LLViewerJointAttachment* attachment_point = NULL;
			if (index > 0)
				attachment_point = get_if_there(gAgent.getAvatarObject()->mAttachmentPoints, index, (LLViewerJointAttachment*)NULL);
			confirm_replace_attachment(0, attachment_point);
		}
		return true;
	}

protected:
	static LLObjectSelectionHandle sObjectSelection;
};

LLObjectSelectionHandle LLObjectAttachToAvatar::sObjectSelection;

void near_attach_object(BOOL success, void *user_data)
{
	if (success)
	{
		LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;
		
		U8 attachment_id = 0;
		if (attachment)
		{
			for (LLVOAvatar::attachment_map_t::iterator iter = gAgent.getAvatarObject()->mAttachmentPoints.begin();
				 iter != gAgent.getAvatarObject()->mAttachmentPoints.end(); ++iter)
			{
				if (iter->second == attachment)
				{
					attachment_id = iter->first;
					break;
				}
			}
		}
		else
		{
			// interpret 0 as "default location"
			attachment_id = 0;
		}
		LLSelectMgr::getInstance()->sendAttach(attachment_id);
	}		
	LLObjectAttachToAvatar::setObjectSelection(NULL);
}

void confirm_replace_attachment(S32 option, void* user_data)
{
	if (option == 0/*YES*/)
	{
		LLViewerObject* selectedObject = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
		if (selectedObject)
		{
			const F32 MIN_STOP_DISTANCE = 1.f;	// meters
			const F32 ARM_LENGTH = 0.5f;		// meters
			const F32 SCALE_FUDGE = 1.5f;

			F32 stop_distance = SCALE_FUDGE * selectedObject->getMaxScale() + ARM_LENGTH;
			if (stop_distance < MIN_STOP_DISTANCE)
			{
				stop_distance = MIN_STOP_DISTANCE;
			}

			LLVector3 walkToSpot = selectedObject->getPositionAgent();
			
			// make sure we stop in front of the object
			LLVector3 delta = walkToSpot - gAgent.getPositionAgent();
			delta.normVec();
			delta = delta * 0.5f;
			walkToSpot -= delta;

			gAgent.startAutoPilotGlobal(gAgent.getPosGlobalFromAgent(walkToSpot), "Attach", NULL, near_attach_object, user_data, stop_distance);
			gAgent.clearFocusObject();
		}
	}
}

class LLAttachmentDrop : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Drop".
		LLViewerObject *object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object)
		{
			llwarns << "handle_drop_attachment() - no object to drop" << llendl;
			return true;
		}

		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if(parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDropAttachment() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		LLSelectMgr::getInstance()->sendDropAttachment();
		return true;
	}
};

// called from avatar pie menu
class LLAttachmentDetachFromPoint : public view_listener_t
{
	bool handleEvent(const LLSD& user_data)
{
		LLViewerJointAttachment *attachment = get_if_there(gAgent.getAvatarObject()->mAttachmentPoints, user_data.asInteger(), (LLViewerJointAttachment*)NULL);
	
		LLViewerObject* attached_object = attachment ? attachment->getObject() : NULL;

	if (attached_object)
	{
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, attached_object->getLocalID());
		gMessageSystem->sendReliable( gAgent.getRegionHost() );
	}
		return true;
}
};

static bool onEnableAttachmentLabel(LLUICtrl* ctrl, const LLSD& data)
{
	std::string label;
	LLMenuItemGL* menu = dynamic_cast<LLMenuItemGL*>(ctrl);
	if (menu)
	{
		LLViewerJointAttachment *attachmentp = get_if_there(gAgent.getAvatarObject()->mAttachmentPoints, data["index"].asInteger(), (LLViewerJointAttachment*)NULL);
	if (attachmentp)
	{
			label = data["label"].asString();
		if (attachmentp->getObject())
		{
			LLViewerInventoryItem* itemp = gInventory.getItem(attachmentp->getItemID());
			if (itemp)
			{
				label += std::string(" (") + itemp->getName() + std::string(")");
			}
		}
	}
		menu->setLabel(label);
}
	return true;
}


class LLAttachmentDetach : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Detach".
		LLViewerObject *object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if(parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDetach() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		// We use deselectAll to update the simulator's notion of what's
		// selected, and removeAll just to change things locally.
		//RN: I thought it was more useful to detach everything that was selected
		if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
		{
			LLSelectMgr::getInstance()->sendDetach();
		}
		return true;
	}
};

//Adding an observer for a Jira 2422 and needs to be a fetch observer
//for Jira 3119
class LLWornItemFetchedObserver : public LLInventoryFetchObserver
{
public:
	LLWornItemFetchedObserver() {}
	virtual ~LLWornItemFetchedObserver() {}

protected:
	virtual void done()
	{
		gPieAttachment->buildDrawLabels();
		gInventory.removeObserver(this);
		delete this;
	}
};

// You can only drop items on parcels where you can build.
class LLAttachmentEnableDrop : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		BOOL can_build   = gAgent.isGodlike() || (LLViewerParcelMgr::getInstance()->agentCanBuild());

		//Add an inventory observer to only allow dropping the newly attached item
		//once it exists in your inventory.  Look at Jira 2422.
		//-jwolk

		// A bug occurs when you wear/drop an item before it actively is added to your inventory
		// if this is the case (you're on a slow sim, etc.) a copy of the object,
		// well, a newly created object with the same properties, is placed
		// in your inventory.  Therefore, we disable the drop option until the
		// item is in your inventory

		LLViewerObject*              object         = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		LLViewerJointAttachment*     attachment_pt  = NULL;
		LLInventoryItem*             item           = NULL;

		if ( object )
		{
    		S32 attachmentID  = ATTACHMENT_ID_FROM_STATE(object->getState());
			attachment_pt = get_if_there(gAgent.getAvatarObject()->mAttachmentPoints, attachmentID, (LLViewerJointAttachment*)NULL);

			if ( attachment_pt )
			{
				// make sure item is in your inventory (it could be a delayed attach message being sent from the sim)
				// so check to see if the item is in the inventory already
				item = gInventory.getItem(attachment_pt->getItemID());
				
				if ( !item )
				{
					// Item does not exist, make an observer to enable the pie menu 
					// when the item finishes fetching worst case scenario 
					// if a fetch is already out there (being sent from a slow sim)
					// we refetch and there are 2 fetches
					LLWornItemFetchedObserver* wornItemFetched = new LLWornItemFetchedObserver();
					LLInventoryFetchObserver::item_ref_t items; //add item to the inventory item to be fetched

					items.push_back(attachment_pt->getItemID());
				
					wornItemFetched->fetchItems(items);
					gInventory.addObserver(wornItemFetched);
				}
			}
		}
		
		//now check to make sure that the item is actually in the inventory before we enable dropping it
		bool new_value = enable_detach() && can_build && item;

		return new_value;
	}
};

BOOL enable_detach(const LLSD&)
{
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if (!object) return FALSE;
	if (!object->isAttachment()) return FALSE;

	// Find the avatar who owns this attachment
	LLViewerObject* avatar = object;
	while (avatar)
	{
		// ...if it's you, good to detach
		if (avatar->getID() == gAgent.getID())
		{
			return TRUE;
		}

		avatar = (LLViewerObject*)avatar->getParent();
	}

	return FALSE;
}

class LLAttachmentEnableDetach : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = enable_detach();
		return new_value;
	}
};

// Used to tell if the selected object can be attached to your avatar.
BOOL object_selected_and_point_valid(const LLSD&)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	for (LLObjectSelection::root_iterator iter = selection->root_begin();
		 iter != selection->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			if (child->isAvatar())
			{
				return FALSE;
			}
		}
	}

	return (selection->getRootObjectCount() == 1) && 
		(selection->getFirstRootObject()->getPCode() == LL_PCODE_VOLUME) && 
		selection->getFirstRootObject()->permYouOwner() &&
		!((LLViewerObject*)selection->getFirstRootObject()->getRoot())->isAvatar() && 
		(selection->getFirstRootObject()->getNVPair("AssetContainer") == NULL);
}


BOOL object_is_wearable()
{
	if (!object_selected_and_point_valid(LLSD()))
	{
		return FALSE;
	}
	if (sitting_on_selection())
	{
		return FALSE;
	}
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;		
		if (node->mPermissions->getOwner() == gAgent.getID())
		{
			return TRUE;
		}
	}
	return FALSE;
}


// Also for seeing if object can be attached.  See above.
class LLObjectEnableWear : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return object_selected_and_point_valid(LLSD());
	}
};

class LLAttachmentPointFilled : public view_listener_t
{
	bool handleEvent(const LLSD& user_data)
	{
		bool enable = false;
		LLVOAvatar::attachment_map_t::iterator found_it = gAgent.getAvatarObject()->mAttachmentPoints.find(user_data.asInteger());
		if (found_it != gAgent.getAvatarObject()->mAttachmentPoints.end())
{
			enable = found_it->second->getObject() != NULL;
		}
		return enable;
}
};

class LLAvatarSendIM : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( LLSelectMgr::getInstance()->getSelection()->getPrimaryObject() );
		if(avatar)
		{
			std::string name("IM");
			LLNameValue *first = avatar->getNVPair("FirstName");
			LLNameValue *last = avatar->getNVPair("LastName");
			if (first && last)
			{
				name.assign( first->getString() );
				name.append(" ");
				name.append( last->getString() );
			}

			//EInstantMessage type = have_agent_callingcard(gLastHitObjectID)
			//	? IM_SESSION_ADD : IM_SESSION_CARDLESS_START;
			gIMMgr->addSession(name,
								IM_NOTHING_SPECIAL,
								avatar->getID());
		}
		return true;
	}
};

namespace
{
	struct QueueObjects : public LLSelectedObjectFunctor
	{
		BOOL scripted;
		BOOL modifiable;
		LLFloaterScriptQueue* mQueue;
		QueueObjects(LLFloaterScriptQueue* q) : mQueue(q), scripted(FALSE), modifiable(FALSE) {}
		virtual bool apply(LLViewerObject* obj)
		{
			scripted = obj->flagScripted();
			modifiable = obj->permModify();

			if( scripted && modifiable )
			{
				mQueue->addObject(obj->getID());
				return false;
			}
			else
			{
				return true; // fail: stop applying
			}
		}
	};
}

void queue_actions(LLFloaterScriptQueue* q, const std::string& msg)
{
	QueueObjects func(q);
	LLSelectMgr *mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle selectHandle = mgr->getSelection();
	bool fail = selectHandle->applyToObjects(&func);
	if(fail)
	{
		if ( !func.scripted )
		{
			std::string noscriptmsg = std::string("Cannot") + msg + "SelectObjectsNoScripts";
			LLNotifications::instance().add(noscriptmsg);
		}
		else if ( !func.modifiable )
		{
			std::string nomodmsg = std::string("Cannot") + msg + "SelectObjectsNoPermission";
			LLNotifications::instance().add(nomodmsg);
		}
		else
		{
			llerrs << "Bad logic." << llendl;
		}
	}
	else
	{
		if (!q->start())
		{
			llwarns << "Unexpected script compile failure." << llendl;
		}
	}
}

class LLToolsSelectedScriptAction : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string action = userdata.asString();
		bool mono = false;
		std::string msg, name;
		if (action == "compile mono")
		{
			name = "compile_queue";
			mono = true;
			msg = "Recompile";
		}
		if (action == "compile lsl")
		{
			name = "compile_queue";
			msg = "Recompile";
		}
		else if (action == "reset")
		{
			name = "reset_queue";
			msg = "Reset";
		}
		else if (action == "start")
		{
			name = "start_queue";
			msg = "Running";
		}
		else if (action == "stop")
		{
			name = "stop_queue";
			msg = "RunningNot";
		}
		LLUUID id; id.generate();
		
		LLFloaterScriptQueue* queue =LLFloaterReg::getTypedInstance<LLFloaterScriptQueue>(name, LLSD(id));
		if (queue)
		{
			queue->setMono(mono);
			queue_actions(queue, msg);
		}
		else
		{
			llwarns << "Failed to generate LLFloaterScriptQueue with action: " << action << llendl;
			delete queue;
		}
		return true;
	}
};

void handle_selected_texture_info(void*)
{
	for (LLObjectSelection::valid_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_end(); iter++)
	{
		LLSelectNode* node = *iter;
		
		std::string msg;
		msg.assign("Texture info for: ");
		msg.append(node->mName);
		LLChat chat(msg);
		LLFloaterChat::addChat(chat);

		U8 te_count = node->getObject()->getNumTEs();
		// map from texture ID to list of faces using it
		typedef std::map< LLUUID, std::vector<U8> > map_t;
		map_t faces_per_texture;
		for (U8 i = 0; i < te_count; i++)
		{
			if (!node->isTESelected(i)) continue;

			LLViewerTexture* img = node->getObject()->getTEImage(i);
			LLUUID image_id = img->getID();
			faces_per_texture[image_id].push_back(i);
		}
		// Per-texture, dump which faces are using it.
		map_t::iterator it;
		for (it = faces_per_texture.begin(); it != faces_per_texture.end(); ++it)
		{
			LLUUID image_id = it->first;
			U8 te = it->second[0];
			LLViewerTexture* img = node->getObject()->getTEImage(te);
			S32 height = img->getHeight();
			S32 width = img->getWidth();
			S32 components = img->getComponents();
			msg = llformat("%dx%d %s on face ",
								width,
								height,
								(components == 4 ? "alpha" : "opaque"));
			for (U8 i = 0; i < it->second.size(); ++i)
			{
				msg.append( llformat("%d ", (S32)(it->second[i])));
			}
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
		}
	}
}

void handle_test_male(void*)
{
	LLAppearanceManager::wearOutfitByName("Male Shape & Outfit");
	//gGestureList.requestResetFromServer( TRUE );
}

void handle_test_female(void*)
{
	LLAppearanceManager::wearOutfitByName("Female Shape & Outfit");
	//gGestureList.requestResetFromServer( FALSE );
}

void handle_toggle_pg(void*)
{
	gAgent.setTeen( !gAgent.isTeen() );

	LLFloaterWorldMap::reloadIcons(NULL);

	llinfos << "PG status set to " << (S32)gAgent.isTeen() << llendl;
}

void handle_dump_attachments(void*)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		llinfos << "NO AVATAR" << llendl;
		return;
	}

	for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin(); 
		 iter != avatar->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		S32 key = curiter->first;
		BOOL visible = (attachment->getObject() != NULL &&
						attachment->getObject()->mDrawable.notNull() && 
						!attachment->getObject()->mDrawable->isRenderType(0));
		LLVector3 pos;
		if (visible) pos = attachment->getObject()->mDrawable->getPosition();
		llinfos << "ATTACHMENT " << key << ": item_id=" << attachment->getItemID()
				<< (attachment->getObject() ? " present " : " absent ")
				<< (visible ? "visible " : "invisible ")
				<<  " at " << pos
				<< " and " << (visible ? attachment->getObject()->getPosition() : LLVector3::zero)
				<< llendl;
	}
}


// these are used in the gl menus to set control values.
class LLToggleControl : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string control_name = userdata.asString();
		BOOL checked = gSavedSettings.getBOOL( control_name );
		gSavedSettings.setBOOL( control_name, !checked );
		return true;
	}
};

class LLCheckControl : public view_listener_t
{
	bool handleEvent( const LLSD& userdata)
	{
		std::string callback_data = userdata.asString();
		bool new_value = gSavedSettings.getBOOL(callback_data);
		return new_value;
}

};

void menu_toggle_attached_lights(void* user_data)
{
	LLPipeline::sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
}

void menu_toggle_attached_particles(void* user_data)
{
	LLPipeline::sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");
}

class LLAdvancedHandleAttchedLightParticles: public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string control_name = userdata.asString();
		if (control_name == "RenderAttachedLights")
{
			menu_toggle_attached_lights(NULL);
}
		else if (control_name == "RenderAttachedParticles")
{
			menu_toggle_attached_particles(NULL);
}
		return true;
}
};

class LLSomethingSelected : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = !(LLSelectMgr::getInstance()->getSelection()->isEmpty());
		return new_value;
	}
};

class LLSomethingSelectedNoHUD : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		bool new_value = !(selection->isEmpty()) && !(selection->getSelectType() == SELECT_TYPE_HUD);
		return new_value;
	}
};

static bool is_editable_selected()
{
	return (LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject() != NULL);
}

class LLEditableSelected : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return is_editable_selected();
	}
};

class LLEditableSelectedMono : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = false;
		LLViewerRegion* region = gAgent.getRegion();
		if(region && gMenuHolder)
		{
			bool have_cap = (! region->getCapability("UpdateScriptTask").empty());
			new_value = is_editable_selected() && have_cap;
		}
		return new_value;
	}
};

class LLToolsEnableTakeCopy : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool all_valid = false;
		if (LLSelectMgr::getInstance())
		{
			if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
			{
			all_valid = true;
#ifndef HACKED_GODLIKE_VIEWER
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
			if (LLViewerLogin::getInstance()->isInProductionGrid()
                || !gAgent.isGodlike())
# endif
			{
				struct f : public LLSelectedObjectFunctor
				{
					virtual bool apply(LLViewerObject* obj)
					{
						return (!obj->permCopy() || obj->isAttachment());
					}
				} func;
				const bool firstonly = true;
				bool any_invalid = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
				all_valid = !any_invalid;
			}
#endif // HACKED_GODLIKE_VIEWER
		}
		}

		return all_valid;
	}
};


class LLHasAsset : public LLInventoryCollectFunctor
{
public:
	LLHasAsset(const LLUUID& id) : mAssetID(id), mHasAsset(FALSE) {}
	virtual ~LLHasAsset() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL hasAsset() const { return mHasAsset; }

protected:
	LLUUID mAssetID;
	BOOL mHasAsset;
};

bool LLHasAsset::operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
{
	if(item && item->getAssetUUID() == mAssetID)
	{
		mHasAsset = TRUE;
	}
	return FALSE;
}

BOOL enable_save_into_inventory(void*)
{
	// *TODO: clean this up
	// find the last root
	LLSelectNode* last_node = NULL;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		last_node = *iter;
	}

#ifdef HACKED_GODLIKE_VIEWER
	return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
	if (!LLViewerLogin::getInstance()->isInProductionGrid()
        && gAgent.isGodlike())
	{
		return TRUE;
	}
# endif
	// check all pre-req's for save into inventory.
	if(last_node && last_node->mValid && !last_node->mItemID.isNull()
	   && (last_node->mPermissions->getOwner() == gAgent.getID())
	   && (gInventory.getItem(last_node->mItemID) != NULL))
	{
		LLViewerObject* obj = last_node->getObject();
		if( obj && !obj->isAttachment() )
		{
			return TRUE;
		}
	}
#endif
	return FALSE;
}

class LLToolsEnableSaveToInventory : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = enable_save_into_inventory(NULL);
		return new_value;
	}
};

BOOL enable_save_into_task_inventory(void*)
{
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if(node && (node->mValid) && (!node->mFromTaskID.isNull()))
	{
		// *TODO: check to see if the fromtaskid object exists.
		LLViewerObject* obj = node->getObject();
		if( obj && !obj->isAttachment() )
		{
			return TRUE;
		}
	}
	return FALSE;
}

class LLToolsEnableSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = enable_save_into_task_inventory(NULL);
		return new_value;
	}
};


class LLViewEnableMouselook : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// You can't go directly from customize avatar to mouselook.
		// TODO: write code with appropriate dialogs to handle this transition.
		bool new_value = (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() && !gSavedSettings.getBOOL("FreezeTime"));
		return new_value;
	}
};

class LLToolsEnableToolNotPie : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = ( LLToolMgr::getInstance()->getBaseTool() != LLToolPie::getInstance() );
		return new_value;
	}
};

class LLWorldEnableCreateLandmark : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return !LLLandmarkActions::landmarkAlreadyExists();
	}
};

class LLWorldEnableSetHomeLocation : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike() || 
			(gAgent.getRegion() && gAgent.getRegion()->getAllowSetHome());
		return new_value;
	}
};

class LLWorldEnableTeleportHome : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		bool agent_on_prelude = (regionp && regionp->isPrelude());
		bool enable_teleport_home = gAgent.isGodlike() || !agent_on_prelude;
		return enable_teleport_home;
	}
};

BOOL enable_god_full(void*)
{
	return gAgent.getGodLevel() >= GOD_FULL;
}

BOOL enable_god_liaison(void*)
{
	return gAgent.getGodLevel() >= GOD_LIAISON;
}

BOOL enable_god_customer_service(void*)
{
	return gAgent.getGodLevel() >= GOD_CUSTOMER_SERVICE;
}

BOOL enable_god_basic(void*)
{
	return gAgent.getGodLevel() > GOD_NOT;
}


void toggle_show_xui_names(void *)
{
	gSavedSettings.setBOOL("DebugShowXUINames", !gSavedSettings.getBOOL("DebugShowXUINames"));
}

BOOL check_show_xui_names(void *)
{
	return gSavedSettings.getBOOL("DebugShowXUINames");
}

class LLToolsSelectOnlyMyObjects : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectOwnedOnly");

		gSavedSettings.setBOOL("SelectOwnedOnly", ! cur_val );

		return true;
	}
};

class LLToolsSelectOnlyMovableObjects : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectMovableOnly");

		gSavedSettings.setBOOL("SelectMovableOnly", ! cur_val );

		return true;
	}
};

class LLToolsSelectBySurrounding : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLSelectMgr::sRectSelectInclusive = !LLSelectMgr::sRectSelectInclusive;

		gSavedSettings.setBOOL("RectangleSelectInclusive", LLSelectMgr::sRectSelectInclusive);
		return true;
	}
};

class LLToolsShowHiddenSelection : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// TomY TODO Merge these
		LLSelectMgr::sRenderHiddenSelections = !LLSelectMgr::sRenderHiddenSelections;

		gSavedSettings.setBOOL("RenderHiddenSelections", LLSelectMgr::sRenderHiddenSelections);
		return true;
	}
};

class LLToolsShowSelectionLightRadius : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		// TomY TODO merge these
		LLSelectMgr::sRenderLightRadius = !LLSelectMgr::sRenderLightRadius;

		gSavedSettings.setBOOL("RenderLightRadius", LLSelectMgr::sRenderLightRadius);
		return true;
	}
};

class LLToolsEditLinkedParts : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		BOOL select_individuals = gSavedSettings.getBOOL("EditLinkedParts");
		if (select_individuals)
		{
			LLSelectMgr::getInstance()->demoteSelectionToIndividuals();
		}
		else
		{
			LLSelectMgr::getInstance()->promoteSelectionToRoot();
		}
		return true;
	}
};

void reload_vertex_shader(void *)
{
	//THIS WOULD BE AN AWESOME PLACE TO RELOAD SHADERS... just a thought	- DaveP
}

void handle_dump_avatar_local_textures(void*)
{
	gAgent.getAvatarObject()->dumpLocalTextures();
}

void handle_debug_avatar_textures(void*)
{
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if (objectp)
	{
		LLFloaterReg::showInstance( "avatar_tetures", LLSD(objectp->getID()) );
	}
}

void handle_grab_texture(void* data)
{
	ETextureIndex index = (ETextureIndex)((intptr_t)data);
	const LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if ( avatar )
	{
		const LLUUID& asset_id = avatar->grabLocalTexture(index);
		LL_INFOS("texture") << "Adding baked texture " << asset_id << " to inventory." << llendl;
		LLAssetType::EType asset_type = LLAssetType::AT_TEXTURE;
		LLInventoryType::EType inv_type = LLInventoryType::IT_TEXTURE;
		LLUUID folder_id(gInventory.findCategoryUUIDForType(asset_type));
		if(folder_id.notNull())
		{
			std::string name = "Unknown";
			const LLVOAvatarDictionary::TextureEntry *texture_dict = LLVOAvatarDictionary::getInstance()->getTexture(index);
			if (texture_dict->mIsBakedTexture)
			{
				EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
				name = "Baked " + LLVOAvatarDictionary::getInstance()->getBakedTexture(baked_index)->mNameCapitalized;
			}
			name += " Texture";

			LLUUID item_id;
			item_id.generate();
			LLPermissions perm;
			perm.init(gAgentID,
					  gAgentID,
					  LLUUID::null,
					  LLUUID::null);
			U32 next_owner_perm = PERM_MOVE | PERM_TRANSFER;
			perm.initMasks(PERM_ALL,
						   PERM_ALL,
						   PERM_NONE,
						   PERM_NONE,
						   next_owner_perm);
			time_t creation_date_now = time_corrected();
			LLPointer<LLViewerInventoryItem> item
				= new LLViewerInventoryItem(item_id,
											folder_id,
											perm,
											asset_id,
											asset_type,
											inv_type,
											name,
											LLStringUtil::null,
											LLSaleInfo::DEFAULT,
											LLInventoryItem::II_FLAGS_NONE,
											creation_date_now);

			item->updateServer(TRUE);
			gInventory.updateItem(item);
			gInventory.notifyObservers();

			LLFloaterInventory* view = LLFloaterInventory::getActiveInventory();

			// Show the preview panel for textures to let
			// user know that the image is now in inventory.
			if(view)
			{
				LLFocusableElement* focus_ctrl = gFocusMgr.getKeyboardFocus();

				view->getPanel()->setSelection(item_id, TAKE_FOCUS_NO);
				view->getPanel()->openSelected();
				//LLFloaterInventory::dumpSelectionInformation((void*)view);
				// restore keyboard focus
				gFocusMgr.setKeyboardFocus(focus_ctrl);
			}
		}
		else
		{
			llwarns << "Can't find a folder to put it in" << llendl;
		}
	}
}

BOOL enable_grab_texture(void* data)
{
	ETextureIndex index = (ETextureIndex)((intptr_t)data);
	const LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if ( avatar )
	{
		return avatar->canGrabLocalTexture(index);
	}
	return FALSE;
}

// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object( LLViewerObject* object )
{
	if (object)
	{
		if( object->isAttachment() )
		{
			do
			{
				object = (LLViewerObject*) object->getParent();
			}
			while( object && !object->isAvatar() );
		}
		else if( !object->isAvatar() )
		{
			object = NULL;
		}
	}

	return (LLVOAvatar*) object;
}


// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object( const LLUUID& object_id )
{
	return find_avatar_from_object( gObjectList.findObject(object_id) );
}


void handle_disconnect_viewer(void *)
{
	LLAppViewer::instance()->forceDisconnect("Testing viewer disconnect");
}

void force_error_breakpoint(void *)
{
    LLAppViewer::instance()->forceErrorBreakpoint();
}

void force_error_llerror(void *)
{
    LLAppViewer::instance()->forceErrorLLError();
}

void force_error_bad_memory_access(void *)
{
    LLAppViewer::instance()->forceErrorBadMemoryAccess();
}

void force_error_infinite_loop(void *)
{
    LLAppViewer::instance()->forceErrorInifiniteLoop();
}

void force_error_software_exception(void *)
{
    LLAppViewer::instance()->forceErrorSoftwareException();
}

void force_error_driver_crash(void *)
{
    LLAppViewer::instance()->forceErrorDriverCrash();
}

class LLToolsUseSelectionForGrid : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLSelectMgr::getInstance()->clearGridObjects();
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* objectp)
			{
				LLSelectMgr::getInstance()->addGridObject(objectp);
				return true;
			}
		} func;
		LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func);
		LLSelectMgr::getInstance()->setGridMode(GRID_MODE_REF_OBJECT);
		if (gFloaterTools)
		{
			gFloaterTools->mComboGridMode->setCurrentByIndex((S32)GRID_MODE_REF_OBJECT);
		}
		return true;
	}
};

void handle_test_load_url(void*)
{
	LLWeb::loadURL("");
	LLWeb::loadURL("hacker://www.google.com/");
	LLWeb::loadURL("http");
	LLWeb::loadURL("http://www.google.com/");
}

//
// LLViewerMenuHolderGL
//

BOOL LLViewerMenuHolderGL::hideMenus()
{
	BOOL handled = LLMenuHolderGL::hideMenus();

	// drop pie menu selection
	mParcelSelection = NULL;
	mObjectSelection = NULL;

	gMenuBarView->clearHoverItem();
	gMenuBarView->resetMenuTrigger();

	return handled;
}

void LLViewerMenuHolderGL::setParcelSelection(LLSafeHandle<LLParcelSelection> selection) 
{ 
	mParcelSelection = selection; 
}

void LLViewerMenuHolderGL::setObjectSelection(LLSafeHandle<LLObjectSelection> selection) 
{ 
	mObjectSelection = selection; 
}


const LLRect LLViewerMenuHolderGL::getMenuRect() const
{
	return LLRect(0, getRect().getHeight() - MENU_BAR_HEIGHT, getRect().getWidth(), STATUS_BAR_HEIGHT);
}

void handle_save_to_xml(void*)
{
	LLFloater* frontmost = gFloaterView->getFrontmost();
	if (!frontmost)
	{
        LLNotifications::instance().add("NoFrontmostFloater");
		return;
	}

	std::string default_name = "floater_";
	default_name += frontmost->getTitle();
	default_name += ".xml";

	LLStringUtil::toLower(default_name);
	LLStringUtil::replaceChar(default_name, ' ', '_');
	LLStringUtil::replaceChar(default_name, '/', '_');
	LLStringUtil::replaceChar(default_name, ':', '_');
	LLStringUtil::replaceChar(default_name, '"', '_');

	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getSaveFile(LLFilePicker::FFSAVE_XML, default_name))
	{
		std::string filename = picker.getFirstFile();
		LLUICtrlFactory::getInstance()->saveToXML(frontmost, filename);
	}
}

void handle_load_from_xml(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getOpenFile(LLFilePicker::FFLOAD_XML))
	{
		std::string filename = picker.getFirstFile();
		LLFloater* floater = new LLFloater(LLSD());
		LLUICtrlFactory::getInstance()->buildFloater(floater, filename, NULL);
	}
}

void handle_web_browser_test(void*)
{
	LLWeb::loadURL("http://secondlife.com/app/search/slurls.html");
}

void handle_buy_currency_test(void*)
{
	std::string url =
		"http://sarahd-sl-13041.webdev.lindenlab.com/app/lindex/index.php?agent_id=[AGENT_ID]&secure_session_id=[SESSION_ID]&lang=[LANGUAGE]";

	LLStringUtil::format_map_t replace;
	replace["[AGENT_ID]"] = gAgent.getID().asString();
	replace["[SESSION_ID]"] = gAgent.getSecureSessionID().asString();

	// *TODO: Replace with call to LLUI::getLanguage() after windows-setup
	// branch merges in. JC
	std::string language = "en";
	language = gSavedSettings.getString("Language");
	if (language.empty() || language == "default")
	{
		language = gSavedSettings.getString("InstallLanguage");
	}
	if (language.empty() || language == "default")
	{
		language = gSavedSettings.getString("SystemLanguage");
	}
	if (language.empty() || language == "default")
	{
		language = "en";
	}

	replace["[LANGUAGE]"] = language;
	LLStringUtil::format(url, replace);

	llinfos << "buy currency url " << url << llendl;

	LLFloaterReg::showInstance("buy_currency_html", LLSD(url));
}

void handle_rebake_textures(void*)
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	if (!avatar) return;

	// Slam pending upload count to "unstick" things
	bool slam_for_debug = true;
	avatar->forceBakeAllTextures(slam_for_debug);
}

void toggle_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	viewp->setVisible(!viewp->getVisible());
}

BOOL get_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	return viewp->getVisible();
}

// TomY TODO: Get rid of these?
class LLViewShowHoverTips : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		gSavedSettings.setBOOL("ShowHoverTips", !gSavedSettings.getBOOL("ShowHoverTips"));
		return true;
	}
};

class LLViewCheckShowHoverTips : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = gSavedSettings.getBOOL("ShowHoverTips");
		return new_value;
	}
};

// TomY TODO: Get rid of these?
class LLViewHighlightTransparent : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLDrawPoolAlpha::sShowDebugAlpha = !LLDrawPoolAlpha::sShowDebugAlpha;
		return true;
	}
};

class LLViewCheckHighlightTransparent : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLDrawPoolAlpha::sShowDebugAlpha;
		return new_value;
	}
};

class LLViewBeaconWidth : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string width = userdata.asString();
		if(width == "1")
		{
			gSavedSettings.setS32("DebugBeaconLineWidth", 1);
		}
		else if(width == "4")
		{
			gSavedSettings.setS32("DebugBeaconLineWidth", 4);
		}
		else if(width == "16")
		{
			gSavedSettings.setS32("DebugBeaconLineWidth", 16);
		}
		else if(width == "32")
		{
			gSavedSettings.setS32("DebugBeaconLineWidth", 32);
		}

		return true;
	}
};


class LLViewToggleBeacon : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string beacon = userdata.asString();
		if (beacon == "scriptsbeacon")
		{
			LLPipeline::toggleRenderScriptedBeacons(NULL);
			gSavedSettings.setBOOL( "scriptsbeacon", LLPipeline::getRenderScriptedBeacons(NULL) );
			// toggle the other one off if it's on
			if (LLPipeline::getRenderScriptedBeacons(NULL) && LLPipeline::getRenderScriptedTouchBeacons(NULL))
			{
				LLPipeline::toggleRenderScriptedTouchBeacons(NULL);
				gSavedSettings.setBOOL( "scripttouchbeacon", LLPipeline::getRenderScriptedTouchBeacons(NULL) );
			}
		}
		else if (beacon == "physicalbeacon")
		{
			LLPipeline::toggleRenderPhysicalBeacons(NULL);
			gSavedSettings.setBOOL( "physicalbeacon", LLPipeline::getRenderPhysicalBeacons(NULL) );
		}
		else if (beacon == "soundsbeacon")
		{
			LLPipeline::toggleRenderSoundBeacons(NULL);
			gSavedSettings.setBOOL( "soundsbeacon", LLPipeline::getRenderSoundBeacons(NULL) );
		}
		else if (beacon == "particlesbeacon")
		{
			LLPipeline::toggleRenderParticleBeacons(NULL);
			gSavedSettings.setBOOL( "particlesbeacon", LLPipeline::getRenderParticleBeacons(NULL) );
		}
		else if (beacon == "scripttouchbeacon")
		{
			LLPipeline::toggleRenderScriptedTouchBeacons(NULL);
			gSavedSettings.setBOOL( "scripttouchbeacon", LLPipeline::getRenderScriptedTouchBeacons(NULL) );
			// toggle the other one off if it's on
			if (LLPipeline::getRenderScriptedBeacons(NULL) && LLPipeline::getRenderScriptedTouchBeacons(NULL))
			{
				LLPipeline::toggleRenderScriptedBeacons(NULL);
				gSavedSettings.setBOOL( "scriptsbeacon", LLPipeline::getRenderScriptedBeacons(NULL) );
			}
		}
		else if (beacon == "renderbeacons")
		{
			LLPipeline::toggleRenderBeacons(NULL);
			gSavedSettings.setBOOL( "renderbeacons", LLPipeline::getRenderBeacons(NULL) );
			// toggle the other one on if it's not
			if (!LLPipeline::getRenderBeacons(NULL) && !LLPipeline::getRenderHighlights(NULL))
			{
				LLPipeline::toggleRenderHighlights(NULL);
				gSavedSettings.setBOOL( "renderhighlights", LLPipeline::getRenderHighlights(NULL) );
			}
		}
		else if (beacon == "renderhighlights")
		{
			LLPipeline::toggleRenderHighlights(NULL);
			gSavedSettings.setBOOL( "renderhighlights", LLPipeline::getRenderHighlights(NULL) );
			// toggle the other one on if it's not
			if (!LLPipeline::getRenderBeacons(NULL) && !LLPipeline::getRenderHighlights(NULL))
			{
				LLPipeline::toggleRenderBeacons(NULL);
				gSavedSettings.setBOOL( "renderbeacons", LLPipeline::getRenderBeacons(NULL) );
			}
		}

		return true;
	}
};

class LLViewCheckBeaconEnabled : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string beacon = userdata.asString();
		bool new_value = false;
		if (beacon == "scriptsbeacon")
		{
			new_value = gSavedSettings.getBOOL( "scriptsbeacon");
			LLPipeline::setRenderScriptedBeacons(new_value);
		}
		else if (beacon == "physicalbeacon")
		{
			new_value = gSavedSettings.getBOOL( "physicalbeacon");
			LLPipeline::setRenderPhysicalBeacons(new_value);
		}
		else if (beacon == "soundsbeacon")
		{
			new_value = gSavedSettings.getBOOL( "soundsbeacon");
			LLPipeline::setRenderSoundBeacons(new_value);
		}
		else if (beacon == "particlesbeacon")
		{
			new_value = gSavedSettings.getBOOL( "particlesbeacon");
			LLPipeline::setRenderParticleBeacons(new_value);
		}
		else if (beacon == "scripttouchbeacon")
		{
			new_value = gSavedSettings.getBOOL( "scripttouchbeacon");
			LLPipeline::setRenderScriptedTouchBeacons(new_value);
		}
		else if (beacon == "renderbeacons")
		{
			new_value = gSavedSettings.getBOOL( "renderbeacons");
			LLPipeline::setRenderBeacons(new_value);
		}
		else if (beacon == "renderhighlights")
		{
			new_value = gSavedSettings.getBOOL( "renderhighlights");
			LLPipeline::setRenderHighlights(new_value);
		}
		return new_value;
	}
};

class LLViewToggleRenderType : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string type = userdata.asString();
		if (type == "hideparticles")
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		}
		return true;
	}
};

class LLViewCheckRenderType : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string type = userdata.asString();
		bool new_value = false;
		if (type == "hideparticles")
		{
			new_value = LLPipeline::toggleRenderTypeControlNegated((void *)LLPipeline::RENDER_TYPE_PARTICLES);
		}
		return new_value;
	}
};

class LLViewShowHUDAttachments : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLPipeline::sShowHUDAttachments = !LLPipeline::sShowHUDAttachments;
		return true;
	}
};

class LLViewCheckHUDAttachments : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool new_value = LLPipeline::sShowHUDAttachments;
		return new_value;
	}
};

class LLEditEnableTakeOff : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string clothing = userdata.asString();
		bool new_value = false;
		if (clothing == "shirt")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_SHIRT);
		}
		if (clothing == "pants")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_PANTS);
		}
		if (clothing == "shoes")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_SHOES);
		}
		if (clothing == "socks")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_SOCKS);
		}
		if (clothing == "jacket")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_JACKET);
		}
		if (clothing == "gloves")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_GLOVES);
		}
		if (clothing == "undershirt")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_UNDERSHIRT);
		}
		if (clothing == "underpants")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_UNDERPANTS);
		}
		if (clothing == "skirt")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_SKIRT);
		}
		if (clothing == "alpha")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_ALPHA);
		}
		if (clothing == "tattoo")
		{
			new_value = LLAgentWearables::selfHasWearable(WT_TATTOO);
		}
		return new_value;
	}
};

class LLEditTakeOff : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string clothing = userdata.asString();
		if (clothing == "shirt")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_SHIRT);
		}
		else if (clothing == "pants")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_PANTS);
		}
		else if (clothing == "shoes")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_SHOES);
		}
		else if (clothing == "socks")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_SOCKS);
		}
		else if (clothing == "jacket")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_JACKET);
		}
		else if (clothing == "gloves")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_GLOVES);
		}
		else if (clothing == "undershirt")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_UNDERSHIRT);
		}
		else if (clothing == "underpants")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_UNDERPANTS);
		}
		else if (clothing == "skirt")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_SKIRT);
		}
		else if (clothing == "alpha")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_ALPHA);
		}
		else if (clothing == "tattoo")
		{
			LLAgentWearables::userRemoveWearable((void*)WT_TATTOO);
		}
		else if (clothing == "all")
		{
			LLAgentWearables::userRemoveAllClothes(NULL);
		}
		return true;
	}
};

class LLToolsSelectTool : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string tool_name = userdata.asString();
		if (tool_name == "focus")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(1);
		}
		else if (tool_name == "move")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(2);
		}
		else if (tool_name == "edit")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(3);
		}
		else if (tool_name == "create")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(4);
		}
		else if (tool_name == "land")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(5);
		}
		return true;
	}
};

/// WINDLIGHT callbacks
class LLWorldEnvSettings : public view_listener_t
{	
	bool handleEvent(const LLSD& userdata)
	{
		std::string tod = userdata.asString();
		LLVector3 sun_direction;
		
		if (tod == "editor")
		{
			// if not there or is hidden, show it
			LLFloaterReg::toggleInstance("env_settings");
			return true;
		}
		
		if (tod == "sunrise")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.25);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "noon")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.567);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "sunset")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.75);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "midnight")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.0);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else
		{
			LLWLParamManager::instance()->mAnimator.mIsRunning = true;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;	
		}
		return true;
	}
};

/// Water Menu callbacks
class LLWorldWaterSettings : public view_listener_t
{	
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterReg::toggleInstance("env_water");
		return true;
	}
};

/// Post-Process callbacks
class LLWorldPostProcess : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterReg::showInstance("env_post_process");
		return true;
	}
};

/// Day Cycle callbacks
class LLWorldDayCycle : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterReg::showInstance("env_day_cycle");
		return true;
	}
};

/// Show First Time Tips calbacks
class LLHelpCheckShowFirstTimeTip : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return LLFirstTimeTipsManager::tipsEnabled();
	}
};

class LLHelpShowFirstTimeTip : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFirstTimeTipsManager::enabledTip(!userdata.asBoolean());
		return true;
	}
};

void initialize_menus()
{
	// A parameterized event handler used as ctrl-8/9/0 zoom controls below.
	class LLZoomer : public view_listener_t
	{
	public:
		// The "mult" parameter says whether "val" is a multiplier or used to set the value.
		LLZoomer(F32 val, bool mult=true) : mVal(val), mMult(mult) {}
		bool handleEvent(const LLSD& userdata)
		{
			F32 new_fov_rad = mMult ? LLViewerCamera::getInstance()->getDefaultFOV() * mVal : mVal;
			LLViewerCamera::getInstance()->setDefaultFOV(new_fov_rad);
			gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
			return true;
		}
	private:
		F32 mVal;
		bool mMult;
	};
	
	LLUICtrl::EnableCallbackRegistry::Registrar& enable = LLUICtrl::EnableCallbackRegistry::currentRegistrar();
	LLUICtrl::CommitCallbackRegistry::Registrar& commit = LLUICtrl::CommitCallbackRegistry::currentRegistrar();
	
	// Enable God Mode
	view_listener_t::addMenu(new LLEnableGodCustomerService(), "EnableGodCustomerService");

	// Agent
	commit.add("Agent.toggleFlying", boost::bind(&LLAgent::toggleFlying));
	enable.add("Agent.emableFlying", boost::bind(&LLAgent::enableFlying));

	// File menu
	init_menu_file();

	// Edit menu
	view_listener_t::addMenu(new LLEditUndo(), "Edit.Undo");
	view_listener_t::addMenu(new LLEditRedo(), "Edit.Redo");
	view_listener_t::addMenu(new LLEditCut(), "Edit.Cut");
	view_listener_t::addMenu(new LLEditCopy(), "Edit.Copy");
	view_listener_t::addMenu(new LLEditPaste(), "Edit.Paste");
	view_listener_t::addMenu(new LLEditDelete(), "Edit.Delete");
	view_listener_t::addMenu(new LLEditSelectAll(), "Edit.SelectAll");
	view_listener_t::addMenu(new LLEditDeselect(), "Edit.Deselect");
	view_listener_t::addMenu(new LLEditDuplicate(), "Edit.Duplicate");
	view_listener_t::addMenu(new LLEditTakeOff(), "Edit.TakeOff");

	view_listener_t::addMenu(new LLEditEnableUndo(), "Edit.EnableUndo");
	view_listener_t::addMenu(new LLEditEnableRedo(), "Edit.EnableRedo");
	view_listener_t::addMenu(new LLEditEnableCut(), "Edit.EnableCut");
	view_listener_t::addMenu(new LLEditEnableCopy(), "Edit.EnableCopy");
	view_listener_t::addMenu(new LLEditEnablePaste(), "Edit.EnablePaste");
	view_listener_t::addMenu(new LLEditEnableDelete(), "Edit.EnableDelete");
	view_listener_t::addMenu(new LLEditEnableSelectAll(), "Edit.EnableSelectAll");
	view_listener_t::addMenu(new LLEditEnableDeselect(), "Edit.EnableDeselect");
	view_listener_t::addMenu(new LLEditEnableDuplicate(), "Edit.EnableDuplicate");
	view_listener_t::addMenu(new LLEditEnableTakeOff(), "Edit.EnableTakeOff");
	view_listener_t::addMenu(new LLEditEnableCustomizeAvatar(), "Edit.EnableCustomizeAvatar");

	// View menu
	view_listener_t::addMenu(new LLViewMouselook(), "View.Mouselook");
	view_listener_t::addMenu(new LLViewJoystickFlycam(), "View.JoystickFlycam");
	view_listener_t::addMenu(new LLViewResetView(), "View.ResetView");
	view_listener_t::addMenu(new LLViewLookAtLastChatter(), "View.LookAtLastChatter");
	view_listener_t::addMenu(new LLViewShowHoverTips(), "View.ShowHoverTips");
	view_listener_t::addMenu(new LLViewHighlightTransparent(), "View.HighlightTransparent");
	view_listener_t::addMenu(new LLViewToggleRenderType(), "View.ToggleRenderType");
	view_listener_t::addMenu(new LLViewShowHUDAttachments(), "View.ShowHUDAttachments");
	view_listener_t::addMenu(new LLZoomer(1.2f), "View.ZoomOut");
	view_listener_t::addMenu(new LLZoomer(1/1.2f), "View.ZoomIn");
	view_listener_t::addMenu(new LLZoomer(DEFAULT_FIELD_OF_VIEW, false), "View.ZoomDefault");
	view_listener_t::addMenu(new LLViewFullscreen(), "View.Fullscreen");
	view_listener_t::addMenu(new LLViewDefaultUISize(), "View.DefaultUISize");

	view_listener_t::addMenu(new LLViewEnableMouselook(), "View.EnableMouselook");
	view_listener_t::addMenu(new LLViewEnableJoystickFlycam(), "View.EnableJoystickFlycam");
	view_listener_t::addMenu(new LLViewEnableLastChatter(), "View.EnableLastChatter");

	view_listener_t::addMenu(new LLViewCheckJoystickFlycam(), "View.CheckJoystickFlycam");
	view_listener_t::addMenu(new LLViewCheckShowHoverTips(), "View.CheckShowHoverTips");
	view_listener_t::addMenu(new LLViewCheckHighlightTransparent(), "View.CheckHighlightTransparent");
	view_listener_t::addMenu(new LLViewCheckRenderType(), "View.CheckRenderType");
	view_listener_t::addMenu(new LLViewCheckHUDAttachments(), "View.CheckHUDAttachments");

	// World menu
	commit.add("World.Chat", boost::bind(&handle_chat, (void*)NULL));
	view_listener_t::addMenu(new LLWorldAlwaysRun(), "World.AlwaysRun");
	view_listener_t::addMenu(new LLWorldCreateLandmark(), "World.CreateLandmark");
	view_listener_t::addMenu(new LLWorldSetHomeLocation(), "World.SetHomeLocation");
	view_listener_t::addMenu(new LLWorldTeleportHome(), "World.TeleportHome");
	view_listener_t::addMenu(new LLWorldSetAway(), "World.SetAway");
	view_listener_t::addMenu(new LLWorldSetBusy(), "World.SetBusy");

	view_listener_t::addMenu(new LLWorldEnableCreateLandmark(), "World.EnableCreateLandmark");
	view_listener_t::addMenu(new LLWorldEnableSetHomeLocation(), "World.EnableSetHomeLocation");
	view_listener_t::addMenu(new LLWorldEnableTeleportHome(), "World.EnableTeleportHome");
	view_listener_t::addMenu(new LLWorldEnableBuyLand(), "World.EnableBuyLand");

	view_listener_t::addMenu(new LLWorldCheckAlwaysRun(), "World.CheckAlwaysRun");
	
	view_listener_t::addMenu(new LLWorldEnvSettings(), "World.EnvSettings");
	view_listener_t::addMenu(new LLWorldWaterSettings(), "World.WaterSettings");
	view_listener_t::addMenu(new LLWorldPostProcess(), "World.PostProcess");
	view_listener_t::addMenu(new LLWorldDayCycle(), "World.DayCycle");

	view_listener_t::addMenu(new LLHelpCheckShowFirstTimeTip(), "Help.CheckShowFirstTimeTip");
	view_listener_t::addMenu(new LLHelpShowFirstTimeTip(), "Help.ShowQuickTips");

	// Tools menu
	view_listener_t::addMenu(new LLToolsSelectTool(), "Tools.SelectTool");
	view_listener_t::addMenu(new LLToolsSelectOnlyMyObjects(), "Tools.SelectOnlyMyObjects");
	view_listener_t::addMenu(new LLToolsSelectOnlyMovableObjects(), "Tools.SelectOnlyMovableObjects");
	view_listener_t::addMenu(new LLToolsSelectBySurrounding(), "Tools.SelectBySurrounding");
	view_listener_t::addMenu(new LLToolsShowHiddenSelection(), "Tools.ShowHiddenSelection");
	view_listener_t::addMenu(new LLToolsShowSelectionLightRadius(), "Tools.ShowSelectionLightRadius");
	view_listener_t::addMenu(new LLToolsEditLinkedParts(), "Tools.EditLinkedParts");
	view_listener_t::addMenu(new LLToolsSnapObjectXY(), "Tools.SnapObjectXY");
	view_listener_t::addMenu(new LLToolsUseSelectionForGrid(), "Tools.UseSelectionForGrid");
	view_listener_t::addMenu(new LLToolsLink(), "Tools.Link");
	view_listener_t::addMenu(new LLToolsUnlink(), "Tools.Unlink");
	view_listener_t::addMenu(new LLToolsStopAllAnimations(), "Tools.StopAllAnimations");
	view_listener_t::addMenu(new LLToolsReleaseKeys(), "Tools.ReleaseKeys");
	view_listener_t::addMenu(new LLToolsEnableReleaseKeys(), "Tools.EnableReleaseKeys");
	view_listener_t::addMenu(new LLToolsLookAtSelection(), "Tools.LookAtSelection");
	view_listener_t::addMenu(new LLToolsBuyOrTake(), "Tools.BuyOrTake");
	view_listener_t::addMenu(new LLToolsTakeCopy(), "Tools.TakeCopy");
	view_listener_t::addMenu(new LLToolsSaveToInventory(), "Tools.SaveToInventory");
	view_listener_t::addMenu(new LLToolsSaveToObjectInventory(), "Tools.SaveToObjectInventory");
	view_listener_t::addMenu(new LLToolsSelectedScriptAction(), "Tools.SelectedScriptAction");

	view_listener_t::addMenu(new LLToolsEnableToolNotPie(), "Tools.EnableToolNotPie");
	view_listener_t::addMenu(new LLToolsEnableLink(), "Tools.EnableLink");
	view_listener_t::addMenu(new LLToolsEnableUnlink(), "Tools.EnableUnlink");
	view_listener_t::addMenu(new LLToolsEnableBuyOrTake(), "Tools.EnableBuyOrTake");
	view_listener_t::addMenu(new LLToolsEnableTakeCopy(), "Tools.EnableTakeCopy");
	view_listener_t::addMenu(new LLToolsEnableSaveToInventory(), "Tools.EnableSaveToInventory");
	view_listener_t::addMenu(new LLToolsEnableSaveToObjectInventory(), "Tools.EnableSaveToObjectInventory");

	/*view_listener_t::addMenu(new LLToolsVisibleBuyObject(), "Tools.VisibleBuyObject");
	view_listener_t::addMenu(new LLToolsVisibleTakeObject(), "Tools.VisibleTakeObject");*/

	// Help menu
	// most items use the ShowFloater method

	// Advance menu
	view_listener_t::addMenu(new LLAdvancedToggleConsole(), "Advanced.ToggleConsole");
	view_listener_t::addMenu(new LLAdvancedCheckConsole(), "Advanced.CheckConsole");
	view_listener_t::addMenu(new LLAdvancedDumpInfoToConsole(), "Advanced.DumpInfoToConsole");
	// Advanced > HUD Info
	view_listener_t::addMenu(new LLAdvancedToggleHUDInfo(), "Advanced.ToggleHUDInfo");
	view_listener_t::addMenu(new LLAdvancedCheckHUDInfo(), "Advanced.CheckHUDInfo");

	// Advanced Other Settings	
	view_listener_t::addMenu(new LLAdvancedClearGroupCache(), "Advanced.ClearGroupCache");

	// Advanced > Render > Types
	view_listener_t::addMenu(new LLAdvancedToggleRenderType(), "Advanced.ToggleRenderType");
	view_listener_t::addMenu(new LLAdvancedCheckRenderType(), "Advanced.CheckRenderType");

	//// Advanced > Render > Features
	view_listener_t::addMenu(new LLAdvancedToggleFeature(), "Advanced.ToggleFeature");
	view_listener_t::addMenu(new LLAdvancedCheckFeature(), "Advanced.CheckFeature");
	// Advanced > Render > Info Displays
	view_listener_t::addMenu(new LLAdvancedToggleInfoDisplay(), "Advanced.ToggleInfoDisplay");
	view_listener_t::addMenu(new LLAdvancedCheckInfoDisplay(), "Advanced.CheckInfoDisplay");
	view_listener_t::addMenu(new LLAdvancedSelectedTextureInfo(), "Advanced.SelectedTextureInfo");
	view_listener_t::addMenu(new LLAdvancedToggleWireframe(), "Advanced.ToggleWireframe");
	view_listener_t::addMenu(new LLAdvancedCheckWireframe(), "Advanced.CheckWireframe");
	view_listener_t::addMenu(new LLAdvancedToggleDisableTextures(), "Advanced.ToggleDisableTextures");
	view_listener_t::addMenu(new LLAdvancedCheckDisableTextures(), "Advanced.CheckDisableTextures");
	view_listener_t::addMenu(new LLAdvancedToggleTextureAtlas(), "Advanced.ToggleTextureAtlas");
	view_listener_t::addMenu(new LLAdvancedCheckTextureAtlas(), "Advanced.CheckTextureAtlas");
	view_listener_t::addMenu(new LLAdvancedEnableObjectObjectOcclusion(), "Advanced.EnableObjectObjectOcclusion");
	view_listener_t::addMenu(new LLAdvancedEnableRenderFBO(), "Advanced.EnableRenderFBO");
	view_listener_t::addMenu(new LLAdvancedEnableRenderDeferred(), "Advanced.EnableRenderDeferred");
	view_listener_t::addMenu(new LLAdvancedEnableRenderDeferredGI(), "Advanced.EnableRenderDeferredGI");
	view_listener_t::addMenu(new LLAdvancedToggleRandomizeFramerate(), "Advanced.ToggleRandomizeFramerate");
	view_listener_t::addMenu(new LLAdvancedCheckRandomizeFramerate(), "Advanced.CheckRandomizeFramerate");
	view_listener_t::addMenu(new LLAdvancedTogglePeriodicSlowFrame(), "Advanced.TogglePeriodicSlowFrame");
	view_listener_t::addMenu(new LLAdvancedCheckPeriodicSlowFrame(), "Advanced.CheckPeriodicSlowFrame");
	view_listener_t::addMenu(new LLAdvancedVectorizePerfTest(), "Advanced.VectorizePerfTest");
	view_listener_t::addMenu(new LLAdvancedToggleFrameTest(), "Advanced.ToggleFrameTest");
	view_listener_t::addMenu(new LLAdvancedCheckFrameTest(), "Advanced.CheckFrameTest");
	view_listener_t::addMenu(new LLAdvancedHandleAttchedLightParticles(), "Advanced.HandleAttchedLightParticles");
	

	#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
	view_listener_t::addMenu(new LLAdvancedHandleToggleHackedGodmode(), "Advanced.HandleToggleHackedGodmode");
	view_listener_t::addMenu(new LLAdvancedCheckToggleHackedGodmode(), "Advanced.CheckToggleHackedGodmode");
	view_listener_t::addMenu(new LLAdvancedEnableToggleHackedGodmode(), "Advanced.EnableToggleHackedGodmode");
	#endif

	// Advanced > World
	view_listener_t::addMenu(new LLAdvancedDumpScriptedCamera(), "Advanced.DumpScriptedCamera");
	view_listener_t::addMenu(new LLAdvancedDumpRegionObjectCache(), "Advanced.DumpRegionObjectCache");

	// Advanced > UI
	view_listener_t::addMenu(new LLAdvancedWebBrowserTest(), "Advanced.WebBrowserTest");
	view_listener_t::addMenu(new LLAdvancedBuyCurrencyTest(), "Advanced.BuyCurrencyTest");
	view_listener_t::addMenu(new LLAdvancedToggleEditableUI(), "Advanced.ToggleEditableUI");
	view_listener_t::addMenu(new LLAdvancedDumpSelectMgr(), "Advanced.DumpSelectMgr");
	view_listener_t::addMenu(new LLAdvancedDumpInventory(), "Advanced.DumpInventory");
	view_listener_t::addMenu(new LLAdvancedDumpFocusHolder(), "Advanced.DumpFocusHolder");
	view_listener_t::addMenu(new LLAdvancedPrintSelectedObjectInfo(), "Advanced.PrintSelectedObjectInfo");
	view_listener_t::addMenu(new LLAdvancedPrintAgentInfo(), "Advanced.PrintAgentInfo");
	view_listener_t::addMenu(new LLAdvancedPrintTextureMemoryStats(), "Advanced.PrintTextureMemoryStats");
	view_listener_t::addMenu(new LLAdvancedToggleDebugClicks(), "Advanced.ToggleDebugClicks");
	view_listener_t::addMenu(new LLAdvancedCheckDebugClicks(), "Advanced.CheckDebugClicks");
	view_listener_t::addMenu(new LLAdvancedCheckDebugViews(), "Advanced.CheckDebugViews");
	view_listener_t::addMenu(new LLAdvancedToggleDebugViews(), "Advanced.ToggleDebugViews");
	view_listener_t::addMenu(new LLAdvancedToggleXUINameTooltips(), "Advanced.ToggleXUINameTooltips");
	view_listener_t::addMenu(new LLAdvancedCheckXUINameTooltips(), "Advanced.CheckXUINameTooltips");
	view_listener_t::addMenu(new LLAdvancedToggleDebugMouseEvents(), "Advanced.ToggleDebugMouseEvents");
	view_listener_t::addMenu(new LLAdvancedCheckDebugMouseEvents(), "Advanced.CheckDebugMouseEvents");
	view_listener_t::addMenu(new LLAdvancedToggleDebugKeys(), "Advanced.ToggleDebugKeys");
	view_listener_t::addMenu(new LLAdvancedCheckDebugKeys(), "Advanced.CheckDebugKeys");
	view_listener_t::addMenu(new LLAdvancedToggleDebugWindowProc(), "Advanced.ToggleDebugWindowProc");
	view_listener_t::addMenu(new LLAdvancedCheckDebugWindowProc(), "Advanced.CheckDebugWindowProc");


	// Advanced > XUI
	commit.add("Advanced.ReloadColorSettings", boost::bind(&LLUIColorTable::loadFromSettings, LLUIColorTable::getInstance()));
	view_listener_t::addMenu(new LLAdvancedLoadUIFromXML(), "Advanced.LoadUIFromXML");
	view_listener_t::addMenu(new LLAdvancedSaveUIToXML(), "Advanced.SaveUIToXML");
	view_listener_t::addMenu(new LLAdvancedToggleXUINames(), "Advanced.ToggleXUINames");
	view_listener_t::addMenu(new LLAdvancedCheckXUINames(), "Advanced.CheckXUINames");
	view_listener_t::addMenu(new LLAdvancedSendTestIms(), "Advanced.SendTestIMs");

	// Advanced > Character > Grab Baked Texture
	view_listener_t::addMenu(new LLAdvancedGrabBakedTexture(), "Advanced.GrabBakedTexture");
	view_listener_t::addMenu(new LLAdvancedEnableGrabBakedTexture(), "Advanced.EnableGrabBakedTexture");

	// Advanced > Character > Character Tests
	view_listener_t::addMenu(new LLAdvancedAppearanceToXML(), "Advanced.AppearanceToXML");
	view_listener_t::addMenu(new LLAdvancedToggleCharacterGeometry(), "Advanced.ToggleCharacterGeometry");

	view_listener_t::addMenu(new LLAdvancedTestMale(), "Advanced.TestMale");
	view_listener_t::addMenu(new LLAdvancedTestFemale(), "Advanced.TestFemale");
	view_listener_t::addMenu(new LLAdvancedTogglePG(), "Advanced.TogglePG");
	
	// Advanced > Character (toplevel)
	view_listener_t::addMenu(new LLAdvancedToggleAllowTapTapHoldRun(), "Advanced.ToggleAllowTapTapHoldRun");
	view_listener_t::addMenu(new LLAdvancedCheckAllowTapTapHoldRun(), "Advanced.CheckAllowTapTapHoldRun");
	view_listener_t::addMenu(new LLAdvancedForceParamsToDefault(), "Advanced.ForceParamsToDefault");
	view_listener_t::addMenu(new LLAdvancedReloadVertexShader(), "Advanced.ReloadVertexShader");
	view_listener_t::addMenu(new LLAdvancedToggleAnimationInfo(), "Advanced.ToggleAnimationInfo");
	view_listener_t::addMenu(new LLAdvancedCheckAnimationInfo(), "Advanced.CheckAnimationInfo");
	view_listener_t::addMenu(new LLAdvancedToggleShowLookAt(), "Advanced.ToggleShowLookAt");
	view_listener_t::addMenu(new LLAdvancedCheckShowLookAt(), "Advanced.CheckShowLookAt");
	view_listener_t::addMenu(new LLAdvancedToggleShowPointAt(), "Advanced.ToggleShowPointAt");
	view_listener_t::addMenu(new LLAdvancedCheckShowPointAt(), "Advanced.CheckShowPointAt");
	view_listener_t::addMenu(new LLAdvancedToggleDebugJointUpdates(), "Advanced.ToggleDebugJointUpdates");
	view_listener_t::addMenu(new LLAdvancedCheckDebugJointUpdates(), "Advanced.CheckDebugJointUpdates");
	view_listener_t::addMenu(new LLAdvancedToggleDisableLOD(), "Advanced.ToggleDisableLOD");
	view_listener_t::addMenu(new LLAdvancedCheckDisableLOD(), "Advanced.CheckDisableLOD");
	view_listener_t::addMenu(new LLAdvancedToggleDebugCharacterVis(), "Advanced.ToggleDebugCharacterVis");
	view_listener_t::addMenu(new LLAdvancedCheckDebugCharacterVis(), "Advanced.CheckDebugCharacterVis");
	view_listener_t::addMenu(new LLAdvancedDumpAttachments(), "Advanced.DumpAttachments");
	view_listener_t::addMenu(new LLAdvancedRebakeTextures(), "Advanced.RebakeTextures");
	#ifndef LL_RELEASE_FOR_DOWNLOAD
	view_listener_t::addMenu(new LLAdvancedDebugAvatarTextures(), "Advanced.DebugAvatarTextures");
	view_listener_t::addMenu(new LLAdvancedDumpAvatarLocalTextures(), "Advanced.DumpAvatarLocalTextures");
	#endif
	// Advanced > Network
	view_listener_t::addMenu(new LLAdvancedEnableMessageLog(), "Advanced.EnableMessageLog");
	view_listener_t::addMenu(new LLAdvancedDisableMessageLog(), "Advanced.DisableMessageLog");
	view_listener_t::addMenu(new LLAdvancedDropPacket(), "Advanced.DropPacket");

	// Advanced > Recorder
	view_listener_t::addMenu(new LLAdvancedAgentPilot(), "Advanced.AgentPilot");
	view_listener_t::addMenu(new LLAdvancedToggleAgentPilotLoop(), "Advanced.ToggleAgentPilotLoop");
	view_listener_t::addMenu(new LLAdvancedCheckAgentPilotLoop(), "Advanced.CheckAgentPilotLoop");

	// Advanced > Debugging
	view_listener_t::addMenu(new LLAdvancedForceErrorBreakpoint(), "Advanced.ForceErrorBreakpoint");
	view_listener_t::addMenu(new LLAdvancedForceErrorLlerror(), "Advanced.ForceErrorLlerror");
	view_listener_t::addMenu(new LLAdvancedForceErrorBadMemoryAccess(), "Advanced.ForceErrorBadMemoryAccess");
	view_listener_t::addMenu(new LLAdvancedForceErrorInfiniteLoop(), "Advanced.ForceErrorInfiniteLoop");
	view_listener_t::addMenu(new LLAdvancedForceErrorSoftwareException(), "Advanced.ForceErrorSoftwareException");
	view_listener_t::addMenu(new LLAdvancedForceErrorDriverCrash(), "Advanced.ForceErrorDriverCrash");
	view_listener_t::addMenu(new LLAdvancedForceErrorDisconnectViewer(), "Advanced.ForceErrorDisconnectViewer");

	// Advanced (toplevel)
	view_listener_t::addMenu(new LLAdvancedToggleShowObjectUpdates(), "Advanced.ToggleShowObjectUpdates");
	view_listener_t::addMenu(new LLAdvancedCheckShowObjectUpdates(), "Advanced.CheckShowObjectUpdates");
	view_listener_t::addMenu(new LLAdvancedCompressImage(), "Advanced.CompressImage");
	view_listener_t::addMenu(new LLAdvancedShowDebugSettings(), "Advanced.ShowDebugSettings");
	view_listener_t::addMenu(new LLAdvancedToggleViewAdminOptions(), "Advanced.ToggleViewAdminOptions");
	view_listener_t::addMenu(new LLAdvancedCheckViewAdminOptions(), "Advanced.CheckViewAdminOptions");
	view_listener_t::addMenu(new LLAdvancedRequestAdminStatus(), "Advanced.RequestAdminStatus");
	view_listener_t::addMenu(new LLAdvancedLeaveAdminStatus(), "Advanced.LeaveAdminStatus");


	// Admin >Object
	view_listener_t::addMenu(new LLAdminForceTakeCopy(), "Admin.ForceTakeCopy");
	view_listener_t::addMenu(new LLAdminHandleObjectOwnerSelf(), "Admin.HandleObjectOwnerSelf");
	view_listener_t::addMenu(new LLAdminHandleObjectOwnerPermissive(), "Admin.HandleObjectOwnerPermissive");
	view_listener_t::addMenu(new LLAdminHandleForceDelete(), "Admin.HandleForceDelete");
	view_listener_t::addMenu(new LLAdminHandleObjectLock(), "Admin.HandleObjectLock");
	view_listener_t::addMenu(new LLAdminHandleObjectAssetIDs(), "Admin.HandleObjectAssetIDs");

	// Admin >Parcel 
	view_listener_t::addMenu(new LLAdminHandleForceParcelOwnerToMe(), "Admin.HandleForceParcelOwnerToMe");
	view_listener_t::addMenu(new LLAdminHandleForceParcelToContent(), "Admin.HandleForceParcelToContent");
	view_listener_t::addMenu(new LLAdminHandleClaimPublicLand(), "Admin.HandleClaimPublicLand");

	// Admin >Region
	view_listener_t::addMenu(new LLAdminHandleRegionDumpTempAssetData(), "Admin.HandleRegionDumpTempAssetData");
	// Admin top level
	view_listener_t::addMenu(new LLAdminOnSaveState(), "Admin.OnSaveState");

	// Self pie menu
	view_listener_t::addMenu(new LLSelfStandUp(), "Self.StandUp");
	view_listener_t::addMenu(new LLSelfRemoveAllAttachments(), "Self.RemoveAllAttachments");

	view_listener_t::addMenu(new LLSelfEnableStandUp(), "Self.EnableStandUp");
	view_listener_t::addMenu(new LLSelfEnableRemoveAllAttachments(), "Self.EnableRemoveAllAttachments");

	// we don't use boost::bind directly to delay side tray construction
	view_listener_t::addMenu(new LLSelfFriends(), "Self.Friends");
	view_listener_t::addMenu(new LLSelfGroups(), "Self.Groups");

	 // Avatar pie menu
	view_listener_t::addMenu(new LLObjectMute(), "Avatar.Mute");
	view_listener_t::addMenu(new LLAvatarAddFriend(), "Avatar.AddFriend");
	view_listener_t::addMenu(new LLAvatarAddContact(), "Avatar.AddContact");
	view_listener_t::addMenu(new LLAvatarFreeze(), "Avatar.Freeze");
	view_listener_t::addMenu(new LLAvatarDebug(), "Avatar.Debug");
	view_listener_t::addMenu(new LLAvatarVisibleDebug(), "Avatar.VisibleDebug");
	view_listener_t::addMenu(new LLAvatarEnableDebug(), "Avatar.EnableDebug");
	view_listener_t::addMenu(new LLAvatarInviteToGroup(), "Avatar.InviteToGroup");
	view_listener_t::addMenu(new LLAvatarGiveCard(), "Avatar.GiveCard");
	view_listener_t::addMenu(new LLAvatarEject(), "Avatar.Eject");
	view_listener_t::addMenu(new LLAvatarSendIM(), "Avatar.SendIM");
	view_listener_t::addMenu(new LLAvatarReportAbuse(), "Avatar.ReportAbuse");
	
	view_listener_t::addMenu(new LLObjectEnableMute(), "Avatar.EnableMute");
	view_listener_t::addMenu(new LLAvatarEnableAddFriend(), "Avatar.EnableAddFriend");
	view_listener_t::addMenu(new LLAvatarEnableFreezeEject(), "Avatar.EnableFreezeEject");

	// Object pie menu
	view_listener_t::addMenu(new LLObjectBuild(), "Object.Build");
	view_listener_t::addMenu(new LLObjectTouch(), "Object.Touch");
	view_listener_t::addMenu(new LLObjectSitOrStand(), "Object.SitOrStand");
	view_listener_t::addMenu(new LLObjectDelete(), "Object.Delete");
	view_listener_t::addMenu(new LLObjectAttachToAvatar(), "Object.AttachToAvatar");
	view_listener_t::addMenu(new LLObjectReturn(), "Object.Return");
	view_listener_t::addMenu(new LLObjectReportAbuse(), "Object.ReportAbuse");
	view_listener_t::addMenu(new LLObjectMute(), "Object.Mute");
	view_listener_t::addMenu(new LLObjectBuy(), "Object.Buy");
	view_listener_t::addMenu(new LLObjectEdit(), "Object.Edit");

	view_listener_t::addMenu(new LLObjectEnableOpen(), "Object.EnableOpen");
	view_listener_t::addMenu(new LLObjectEnableTouch(), "Object.EnableTouch");
	view_listener_t::addMenu(new LLObjectEnableSitOrStand(), "Object.EnableSitOrStand");
	view_listener_t::addMenu(new LLObjectEnableDelete(), "Object.EnableDelete");
	view_listener_t::addMenu(new LLObjectEnableWear(), "Object.EnableWear");
	view_listener_t::addMenu(new LLObjectEnableReturn(), "Object.EnableReturn");
	view_listener_t::addMenu(new LLObjectEnableReportAbuse(), "Object.EnableReportAbuse");
	view_listener_t::addMenu(new LLObjectEnableMute(), "Object.EnableMute");
	view_listener_t::addMenu(new LLObjectEnableBuy(), "Object.EnableBuy");

	/*view_listener_t::addMenu(new LLObjectVisibleTouch(), "Object.VisibleTouch");
	view_listener_t::addMenu(new LLObjectVisibleCustomTouch(), "Object.VisibleCustomTouch");
	view_listener_t::addMenu(new LLObjectVisibleStandUp(), "Object.VisibleStandUp");
	view_listener_t::addMenu(new LLObjectVisibleSitHere(), "Object.VisibleSitHere");
	view_listener_t::addMenu(new LLObjectVisibleCustomSit(), "Object.VisibleCustomSit");*/

	// Attachment pie menu
	enable.add("Attachment.Label", boost::bind(&onEnableAttachmentLabel, _1, _2));
	view_listener_t::addMenu(new LLAttachmentDrop(), "Attachment.Drop");
	view_listener_t::addMenu(new LLAttachmentDetachFromPoint(), "Attachment.DetachFromPoint");
	view_listener_t::addMenu(new LLAttachmentDetach(), "Attachment.Detach");
	view_listener_t::addMenu(new LLAttachmentPointFilled(), "Attachment.PointFilled");
	view_listener_t::addMenu(new LLAttachmentEnableDrop(), "Attachment.EnableDrop");
	view_listener_t::addMenu(new LLAttachmentEnableDetach(), "Attachment.EnableDetach");

	// Land pie menu
	view_listener_t::addMenu(new LLLandBuild(), "Land.Build");
	view_listener_t::addMenu(new LLLandSit(), "Land.Sit");
	view_listener_t::addMenu(new LLLandBuyPass(), "Land.BuyPass");
	view_listener_t::addMenu(new LLLandEdit(), "Land.Edit");

	view_listener_t::addMenu(new LLLandEnableBuyPass(), "Land.EnableBuyPass");

	// Generic actions
	view_listener_t::addMenu(new LLShowFloater(), "ShowFloater");
	view_listener_t::addMenu(new LLPromptShowURL(), "PromptShowURL");
	view_listener_t::addMenu(new LLShowAgentProfile(), "ShowAgentProfile");
	view_listener_t::addMenu(new LLToggleControl(), "ToggleControl");
	view_listener_t::addMenu(new LLCheckControl(), "CheckControl");
	view_listener_t::addMenu(new LLGoToObject(), "GoToObject");
	view_listener_t::addMenu(new LLPayObject(), "PayObject");

	view_listener_t::addMenu(new LLEnablePayObject(), "EnablePayObject");
	view_listener_t::addMenu(new LLEnableEdit(), "EnableEdit");

	view_listener_t::addMenu(new LLFloaterVisible(), "FloaterVisible");
	view_listener_t::addMenu(new LLSomethingSelected(), "SomethingSelected");
	view_listener_t::addMenu(new LLSomethingSelectedNoHUD(), "SomethingSelectedNoHUD");
	view_listener_t::addMenu(new LLEditableSelected(), "EditableSelected");
	view_listener_t::addMenu(new LLEditableSelectedMono(), "EditableSelectedMono");

}
