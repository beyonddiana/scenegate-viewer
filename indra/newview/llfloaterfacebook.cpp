/** 
* @file llfloaterfacebook.cpp
* @brief Implementation of llfloaterfacebook
* @author Gilbert@lindenlab.com
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterfacebook.h"

#include "llagent.h"
#include "llagentui.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "lliconctrl.h"
#include "llimagefiltersmanager.h"
#include "llresmgr.h"		// LLLocale
#include "llsdserialize.h"
#include "llloadingindicator.h"
#include "llplugincookiestore.h"
#include "llslurl.h"
#include "lltrans.h"
#include "llsnapshotlivepreview.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "lltabcontainer.h"

static LLRegisterPanelClassWrapper<LLFacebookStatusPanel> t_panel_status("llfacebookstatuspanel");
static LLRegisterPanelClassWrapper<LLFacebookPhotoPanel> t_panel_photo("llfacebookphotopanel");
static LLRegisterPanelClassWrapper<LLFacebookCheckinPanel> t_panel_checkin("llfacebookcheckinpanel");
static LLRegisterPanelClassWrapper<LLFacebookAccountPanel> t_panel_account("llfacebookaccountpanel");

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const std::string DEFAULT_CHECKIN_LOCATION_URL = "http://maps.secondlife.com/";
const std::string DEFAULT_CHECKIN_ICON_URL = "http://map.secondlife.com.s3.amazonaws.com/map_placeholder.png";
const std::string DEFAULT_CHECKIN_QUERY_PARAMETERS = "?sourceid=slshare_checkin&utm_source=facebook&utm_medium=checkin&utm_campaign=slshare";
const std::string DEFAULT_PHOTO_QUERY_PARAMETERS = "?sourceid=slshare_photo&utm_source=facebook&utm_medium=photo&utm_campaign=slshare";

const S32 MAX_QUALITY = 100;        // Max quality value for jpeg images
const S32 MIN_QUALITY = 0;          // Min quality value for jpeg images
const S32 TARGET_DATA_SIZE = 95000; // Size of the image (compressed) we're trying to send to Facebook

std::string get_map_url()
{
    LLVector3d center_agent;
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
        center_agent = regionp->getCenterGlobal();
    }
    int x_pos = center_agent[0] / 256.0;
    int y_pos = center_agent[1] / 256.0;
    std::string map_url = gSavedSettings.getString("CurrentMapServerURL") + llformat("map-1-%d-%d-objects.jpg", x_pos, y_pos);
    return map_url;
}

// Compute target jpeg quality : see https://wiki.lindenlab.com/wiki/Facebook_Image_Quality for details
S32 compute_jpeg_quality(S32 width, S32 height)
{
    F32 target_compression_ratio = (F32)(width * height * 3) / (F32)(TARGET_DATA_SIZE);
    S32 quality = (S32)(110.0f - (2.0f * target_compression_ratio));
    return llclamp(quality,MIN_QUALITY,MAX_QUALITY);
}

///////////////////////////
//LLFacebookStatusPanel//////
///////////////////////////

LLFacebookStatusPanel::LLFacebookStatusPanel() :
	mMessageTextEditor(NULL),
	mPostButton(NULL),
    mCancelButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendStatus", boost::bind(&LLFacebookStatusPanel::onSend, this));
}

BOOL LLFacebookStatusPanel::postBuild()
{
	mMessageTextEditor = getChild<LLUICtrl>("status_message");
	mPostButton = getChild<LLUICtrl>("post_status_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_status_btn");

	return LLPanel::postBuild();
}

void LLFacebookStatusPanel::draw()
{
    if (mMessageTextEditor && mPostButton && mCancelButton)
	{
        bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
        std::string message = mMessageTextEditor->getValue().asString();
        mMessageTextEditor->setEnabled(no_ongoing_connection);
        mCancelButton->setEnabled(no_ongoing_connection);
        mPostButton->setEnabled(no_ongoing_connection && !message.empty());
    }

	LLPanel::draw();
}

void LLFacebookStatusPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookStatusPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLFacebookStatusPanel", boost::bind(&LLFacebookStatusPanel::onFacebookConnectStateChange, this, _1));
		
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendStatus();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLFacebookStatusPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendStatus();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookStatusPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLFacebookStatusPanel::sendStatus()
{
	std::string message = mMessageTextEditor->getValue().asString();
	if (!message.empty())
	{
		LLFacebookConnect::instance().updateStatus(message);
	}
}

void LLFacebookStatusPanel::clearAndClose()
{
	mMessageTextEditor->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

///////////////////////////
//LLFacebookPhotoPanel///////
///////////////////////////

LLFacebookPhotoPanel::LLFacebookPhotoPanel() :
mSnapshotPanel(NULL),
mResolutionComboBox(NULL),
mRefreshBtn(NULL),
mWorkingLabel(NULL),
mThumbnailPlaceholder(NULL),
mCaptionTextBox(NULL),
mLocationCheckbox(NULL),
mPostButton(NULL),
mQuality(MAX_QUALITY)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendPhoto", boost::bind(&LLFacebookPhotoPanel::onSend, this));
	mCommitCallbackRegistrar.add("SocialSharing.RefreshPhoto", boost::bind(&LLFacebookPhotoPanel::onClickNewSnapshot, this));
}

LLFacebookPhotoPanel::~LLFacebookPhotoPanel()
{
	if(mPreviewHandle.get())
	{
		mPreviewHandle.get()->die();
	}
}

BOOL LLFacebookPhotoPanel::postBuild()
{
	setVisibleCallback(boost::bind(&LLFacebookPhotoPanel::onVisibilityChange, this, _2));
	
	mSnapshotPanel = getChild<LLUICtrl>("snapshot_panel");
	mResolutionComboBox = getChild<LLUICtrl>("resolution_combobox");
	mResolutionComboBox->setValue("[i1200,i630]"); // hardcoded defaults ftw!
	mResolutionComboBox->setCommitCallback(boost::bind(&LLFacebookPhotoPanel::updateResolution, this, TRUE));
	mFilterComboBox = getChild<LLUICtrl>("filters_combobox");
	mFilterComboBox->setCommitCallback(boost::bind(&LLFacebookPhotoPanel::updateResolution, this, TRUE));
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
    mWorkingLabel = getChild<LLUICtrl>("working_lbl");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");
	mCaptionTextBox = getChild<LLUICtrl>("photo_caption");
	mLocationCheckbox = getChild<LLUICtrl>("add_location_cb");
	mPostButton = getChild<LLUICtrl>("post_photo_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_photo_btn");

	// Update filter list
    std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
	LLComboBox* filterbox = static_cast<LLComboBox *>(mFilterComboBox);
    for (U32 i = 0; i < filter_list.size(); i++)
	{
        filterbox->add(filter_list[i]);
    }

	return LLPanel::postBuild();
}

// virtual
S32 LLFacebookPhotoPanel::notify(const LLSD& info)
{
	if (info.has("snapshot-updating"))
	{
        // Disable the Post button and whatever else while the snapshot is not updated
        // updateControls();
		return 1;
	}
    
	if (info.has("snapshot-updated"))
	{
        // Enable the send/post/save buttons.
        updateControls();
        
		// The refresh button is initially hidden. We show it after the first update,
		// i.e. after snapshot is taken
		LLUICtrl * refresh_button = getRefreshBtn();
		if (!refresh_button->getVisible())
		{
			refresh_button->setVisible(true);
		}
		return 1;
	}
    
	return 0;
}

void LLFacebookPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

    // Enable interaction only if no transaction with the service is on-going (prevent duplicated posts)
    bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
    mCancelButton->setEnabled(no_ongoing_connection);
    mCaptionTextBox->setEnabled(no_ongoing_connection);
    mResolutionComboBox->setEnabled(no_ongoing_connection);
    mFilterComboBox->setEnabled(no_ongoing_connection);
    mRefreshBtn->setEnabled(no_ongoing_connection);
    mLocationCheckbox->setEnabled(no_ongoing_connection);
    
    // Display the preview if one is available
	if (previewp && previewp->getThumbnailImage())
	{
		const LLRect& thumbnail_rect = mThumbnailPlaceholder->getRect();
		const S32 thumbnail_w = previewp->getThumbnailWidth();
		const S32 thumbnail_h = previewp->getThumbnailHeight();

		// calc preview offset within the preview rect
		const S32 local_offset_x = (thumbnail_rect.getWidth()  - thumbnail_w) / 2 ;
		const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ;

		// calc preview offset within the floater rect
        // Hack : To get the full offset, we need to take into account each and every offset of each widgets up to the floater.
        // This is almost as arbitrary as using a fixed offset so that's what we do here for the sake of simplicity.
        // *TODO : Get the offset looking through the hierarchy of widgets, should be done in postBuild() so to avoid traversing the hierarchy each time.
		S32 offset_x = thumbnail_rect.mLeft + local_offset_x - 1;
		S32 offset_y = thumbnail_rect.mBottom + local_offset_y - 39;
        
		mSnapshotPanel->localPointToOtherView(offset_x, offset_y, &offset_x, &offset_y, getParentByType<LLFloater>());
        
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		// Apply floater transparency to the texture unless the floater is focused.
		F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
		LLColor4 color = LLColor4::white;
		gl_draw_scaled_image(offset_x, offset_y, 
			thumbnail_w, thumbnail_h,
			previewp->getThumbnailImage(), color % alpha);
	}

    // Update the visibility of the working (computing preview) label
    mWorkingLabel->setVisible(!(previewp && previewp->getSnapshotUpToDate()));
    
    // Enable Post if we have a preview to send and no on going connection being processed
    mPostButton->setEnabled(no_ongoing_connection && (previewp && previewp->getSnapshotUpToDate()));
    
    // Draw the rest of the panel on top of it
	LLPanel::draw();
}

LLSnapshotLivePreview* LLFacebookPhotoPanel::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

void LLFacebookPhotoPanel::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();
	if (visible)
	{
		if (mPreviewHandle.get())
		{
			LLSnapshotLivePreview* preview = getPreviewView();
			if(preview)
			{
				lldebugs << "opened, updating snapshot" << llendl;
				preview->updateSnapshot(TRUE);
			}
		}
		else
		{
			LLRect full_screen_rect = getRootView()->getRect();
			LLSnapshotLivePreview::Params p;
			p.rect(full_screen_rect);
			LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
			mPreviewHandle = previewp->getHandle();	
            mQuality = MAX_QUALITY;

            previewp->setContainer(this);
			previewp->setSnapshotType(previewp->SNAPSHOT_WEB);
			previewp->setSnapshotFormat(LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);
			previewp->setSnapshotQuality(mQuality, false);
            previewp->setThumbnailSubsampled(TRUE);     // We want the preview to reflect the *saved* image
            previewp->setAllowRenderUI(FALSE);          // We do not want the rendered UI in our snapshots
            previewp->setAllowFullScreenPreview(FALSE);  // No full screen preview in SL Share mode
			previewp->setThumbnailPlaceholderRect(mThumbnailPlaceholder->getRect());

			updateControls();
		}
	}
}

void LLFacebookPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->updateSnapshot(TRUE);
	}
}

void LLFacebookPhotoPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookPhotoPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLFacebookPhotoPanel", boost::bind(&LLFacebookPhotoPanel::onFacebookConnectStateChange, this, _1));
	
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendPhoto();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLFacebookPhotoPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendPhoto();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookPhotoPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLFacebookPhotoPanel::sendPhoto()
{
	// Get the caption
	std::string caption = mCaptionTextBox->getValue().asString();

	// Add the location if required
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	if (add_location)
	{
		// Get the SLURL for the location
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		std::string slurl_string = slurl.getSLURLString();

		// Add query parameters so Google Analytics can track incoming clicks!
		slurl_string += DEFAULT_PHOTO_QUERY_PARAMETERS;

		// Add it to the caption (pretty crude, but we don't have a better option with photos)
		if (caption.empty())
			caption = slurl_string;
		else
			caption = caption + " " + slurl_string;
	}

	// Get the image
	LLSnapshotLivePreview* previewp = getPreviewView();
	
	// Post to Facebook
	LLFacebookConnect::instance().sharePhoto(previewp->getFormattedImage(), caption);

	updateControls();
}

void LLFacebookPhotoPanel::clearAndClose()
{
	mCaptionTextBox->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

void LLFacebookPhotoPanel::updateControls()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();
    
	// *TODO: Separate maximum size for Web images from postcards
	lldebugs << "Is snapshot up-to-date? " << got_snap << llendl;
    
	updateResolution(FALSE);
}

void LLFacebookPhotoPanel::updateResolution(BOOL do_update)
{
	LLComboBox* combobox = static_cast<LLComboBox *>(mResolutionComboBox);
	LLComboBox* filterbox = static_cast<LLComboBox *>(mFilterComboBox);

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());

	S32 width = sdres[0];
	S32 height = sdres[1];

    const std::string& filter_name = filterbox->getSimple();

	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;

		if (width == 0 || height == 0)
		{
			// take resolution from current window size
			lldebugs << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << llendl;
			previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			lldebugs << "Setting preview res selected from combo: " << width << "x" << height << llendl;
			previewp->setSize(width, height);
		}

		checkAspectRatio(width);

		previewp->getSize(width, height);
        
        // Recompute quality setting
        mQuality = compute_jpeg_quality(width, height);
        previewp->setSnapshotQuality(mQuality, false);
		
		if (original_width != width || original_height != height)
		{
			previewp->setSize(width, height);
			if (do_update)
			{
                previewp->updateSnapshot(TRUE);
				updateControls();
			}
		}
        // Get the old filter, compare to the current one "filter_name" and set if changed
        std::string original_filter = previewp->getFilter();
		if (original_filter != filter_name)
		{
            previewp->setFilter(filter_name);
			if (do_update)
			{
                previewp->updateSnapshot(FALSE, TRUE);
				updateControls();
			}
		}
	}
}

void LLFacebookPhotoPanel::checkAspectRatio(S32 index)
{
	LLSnapshotLivePreview *previewp = getPreviewView() ;

	BOOL keep_aspect = FALSE;

	if (0 == index) // current window size
	{
		keep_aspect = TRUE;
	}
	else // predefined resolution
	{
		keep_aspect = FALSE;
	}

	if (previewp)
	{
		previewp->mKeepAspectRatio = keep_aspect;
	}
}

LLUICtrl* LLFacebookPhotoPanel::getRefreshBtn()
{
	return mRefreshBtn;
}

////////////////////////
//LLFacebookCheckinPanel//
////////////////////////

LLFacebookCheckinPanel::LLFacebookCheckinPanel() :
    mMapUrl(""),
    mReloadingMapTexture(false)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendCheckin", boost::bind(&LLFacebookCheckinPanel::onSend, this));
}

BOOL LLFacebookCheckinPanel::postBuild()
{
    // Keep pointers to widgets so we don't traverse the UI hierarchy too often
	mPostButton = getChild<LLUICtrl>("post_place_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_place_btn");
	mMessageTextEditor = getChild<LLUICtrl>("place_caption");
    mMapLoadingIndicator = getChild<LLUICtrl>("map_loading_indicator");
    mMapPlaceholder = getChild<LLIconCtrl>("map_placeholder");
    mMapDefault = getChild<LLIconCtrl>("map_default");
    mMapCheckBox = getChild<LLCheckBoxCtrl>("add_place_view_cb");
    
	return LLPanel::postBuild();
}

void LLFacebookCheckinPanel::draw()
{
    bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
    mPostButton->setEnabled(no_ongoing_connection);
    mCancelButton->setEnabled(no_ongoing_connection);
    mMessageTextEditor->setEnabled(no_ongoing_connection);
    mMapCheckBox->setEnabled(no_ongoing_connection);

    std::string map_url = get_map_url();
    // Did we change location?
    if (map_url != mMapUrl)
    {
        mMapUrl = map_url;
        // Load the map tile
        mMapTexture = LLViewerTextureManager::getFetchedTextureFromUrl(mMapUrl, FTT_MAP_TILE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
        mMapTexture->setBoostLevel(LLGLTexture::BOOST_MAP);
        mReloadingMapTexture = true;
        // In the meantime, put the "loading" indicator on, hide the tile map and disable the checkbox
        mMapLoadingIndicator->setVisible(true);
        mMapPlaceholder->setVisible(false);
    }
    // Are we done loading the map tile?
    if (mReloadingMapTexture && mMapTexture->isFullyLoaded())
    {
        // Don't do it again next time around
        mReloadingMapTexture = false;
        // Convert the map texture to the appropriate image object
        LLPointer<LLUIImage> ui_image = new LLUIImage(mMapUrl, mMapTexture);
        // Load the map widget with the correct map tile image
        mMapPlaceholder->setImage(ui_image);
        // Now hide the loading indicator, bring the tile in view and reenable the checkbox with its previous value
        mMapLoadingIndicator->setVisible(false);
        mMapPlaceholder->setVisible(true);
    }
    // Show the default icon if that's the checkbox value (the real one...)
    // This will hide/show the loading indicator and/or tile underneath
    mMapDefault->setVisible(!(mMapCheckBox->get()));

	LLPanel::draw();
}

void LLFacebookCheckinPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookCheckinPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLFacebookCheckinPanel", boost::bind(&LLFacebookCheckinPanel::onFacebookConnectStateChange, this, _1));
	
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendCheckin();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLFacebookCheckinPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendCheckin();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookCheckinPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLFacebookCheckinPanel::sendCheckin()
{
	// Get the location SLURL
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	std::string slurl_string = slurl.getSLURLString();

	// Use a valid http:// URL if the scheme is secondlife:// 
	LLURI slurl_uri(slurl_string);
	if (slurl_uri.scheme() == LLSLURL::SLURL_SECONDLIFE_SCHEME)
	{
		slurl_string = DEFAULT_CHECKIN_LOCATION_URL;
	}

	// Add query parameters so Google Analytics can track incoming clicks!
	slurl_string += DEFAULT_CHECKIN_QUERY_PARAMETERS;
    
	// Get the region name
	std::string region_name("");
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
        region_name = regionp->getName();
    }
    
	// Get the region description
	std::string description;
	LLAgentUI::buildLocationString(description, LLAgentUI::LOCATION_FORMAT_NORMAL_COORDS, gAgent.getPositionAgent());
    
	// Optionally add the region map view
	bool add_map_view = mMapCheckBox->getValue().asBoolean();
    std::string map_url = (add_map_view ? get_map_url() : DEFAULT_CHECKIN_ICON_URL);
    
	// Get the caption
	std::string caption = mMessageTextEditor->getValue().asString();

	// Post to Facebook
	LLFacebookConnect::instance().postCheckin(slurl_string, region_name, description, map_url, caption);
}

void LLFacebookCheckinPanel::clearAndClose()
{
	mMessageTextEditor->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

///////////////////////////
//LLFacebookAccountPanel//////
///////////////////////////

LLFacebookAccountPanel::LLFacebookAccountPanel() : 
mAccountCaptionLabel(NULL),
mAccountNameLabel(NULL),
mPanelButtons(NULL),
mConnectButton(NULL),
mDisconnectButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Connect", boost::bind(&LLFacebookAccountPanel::onConnect, this));
	mCommitCallbackRegistrar.add("SocialSharing.Disconnect", boost::bind(&LLFacebookAccountPanel::onDisconnect, this));

	setVisibleCallback(boost::bind(&LLFacebookAccountPanel::onVisibilityChange, this, _2));
}

BOOL LLFacebookAccountPanel::postBuild()
{
	mAccountCaptionLabel = getChild<LLTextBox>("account_caption_label");
	mAccountNameLabel = getChild<LLTextBox>("account_name_label");
	mPanelButtons = getChild<LLUICtrl>("panel_buttons");
	mConnectButton = getChild<LLUICtrl>("connect_btn");
	mDisconnectButton = getChild<LLUICtrl>("disconnect_btn");

	return LLPanel::postBuild();
}

void LLFacebookAccountPanel::draw()
{
	LLFacebookConnect::EConnectionState connection_state = LLFacebookConnect::instance().getConnectionState();

	//Disable the 'disconnect' button and the 'use another account' button when disconnecting in progress
	bool disconnecting = connection_state == LLFacebookConnect::FB_DISCONNECTING;
	mDisconnectButton->setEnabled(!disconnecting);

	//Disable the 'connect' button when a connection is in progress
	bool connecting = connection_state == LLFacebookConnect::FB_CONNECTION_IN_PROGRESS;
	mConnectButton->setEnabled(!connecting);

	LLPanel::draw();
}

void LLFacebookAccountPanel::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();

	if(visible)
	{
		LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectState").listen("LLFacebookAccountPanel", boost::bind(&LLFacebookAccountPanel::onFacebookConnectStateChange, this, _1));

		LLEventPumps::instance().obtain("FacebookConnectInfo").stopListening("LLFacebookAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectInfo").listen("LLFacebookAccountPanel", boost::bind(&LLFacebookAccountPanel::onFacebookConnectInfoChange, this));

		//Connected
		if(LLFacebookConnect::instance().isConnected())
		{
			showConnectedLayout();
		}
		//Check if connected (show disconnected layout in meantime)
		else
		{
			showDisconnectedLayout();
		}
        if ((LLFacebookConnect::instance().getConnectionState() == LLFacebookConnect::FB_NOT_CONNECTED) ||
            (LLFacebookConnect::instance().getConnectionState() == LLFacebookConnect::FB_CONNECTION_FAILED))
        {
            LLFacebookConnect::instance().checkConnectionToFacebook();
        }
	}
	else
	{
		LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLFacebookAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectInfo").stopListening("LLFacebookAccountPanel");
	}
}

bool LLFacebookAccountPanel::onFacebookConnectStateChange(const LLSD& data)
{
	if(LLFacebookConnect::instance().isConnected())
	{
		//In process of disconnecting so leave the layout as is
		if(data.get("enum").asInteger() != LLFacebookConnect::FB_DISCONNECTING)
		{
			showConnectedLayout();
		}
	}
	else
	{
		showDisconnectedLayout();
	}

	return false;
}

bool LLFacebookAccountPanel::onFacebookConnectInfoChange()
{
	LLSD info = LLFacebookConnect::instance().getInfo();
	std::string clickable_name;

	//Strings of format [http://www.somewebsite.com Click Me] become clickable text
	if(info.has("link") && info.has("name"))
	{
		clickable_name = "[" + info["link"].asString() + " " + info["name"].asString() + "]";
	}

	mAccountNameLabel->setText(clickable_name);

	return false;
}

void LLFacebookAccountPanel::showConnectButton()
{
	if(!mConnectButton->getVisible())
	{
		mConnectButton->setVisible(TRUE);
		mDisconnectButton->setVisible(FALSE);
	}
}

void LLFacebookAccountPanel::hideConnectButton()
{
	if(mConnectButton->getVisible())
	{
		mConnectButton->setVisible(FALSE);
		mDisconnectButton->setVisible(TRUE);
	}
}

void LLFacebookAccountPanel::showDisconnectedLayout()
{
	mAccountCaptionLabel->setText(getString("facebook_disconnected"));
	mAccountNameLabel->setText(std::string(""));
	showConnectButton();
}

void LLFacebookAccountPanel::showConnectedLayout()
{
	LLFacebookConnect::instance().loadFacebookInfo();

	mAccountCaptionLabel->setText(getString("facebook_connected"));
	hideConnectButton();
}

void LLFacebookAccountPanel::onConnect()
{
	LLFacebookConnect::instance().checkConnectionToFacebook(true);

	//Clear only the facebook browser cookies so that the facebook login screen appears
	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".facebook.com"); 
}

void LLFacebookAccountPanel::onDisconnect()
{
	LLFacebookConnect::instance().disconnectFromFacebook();

	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".facebook.com"); 
}

////////////////////////
//LLFloaterFacebook///////
////////////////////////

LLFloaterFacebook::LLFloaterFacebook(const LLSD& key) : LLFloater(key),
    mFacebookPhotoPanel(NULL),
    mStatusErrorText(NULL),
    mStatusLoadingText(NULL),
    mStatusLoadingIndicator(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterFacebook::onCancel, this));
}

void LLFloaterFacebook::onCancel()
{
    closeFloater();
}

BOOL LLFloaterFacebook::postBuild()
{
    // Keep tab of the Photo Panel
	mFacebookPhotoPanel = static_cast<LLFacebookPhotoPanel*>(getChild<LLUICtrl>("panel_facebook_photo"));
    // Connection status widgets
    mStatusErrorText = getChild<LLTextBox>("connection_error_text");
    mStatusLoadingText = getChild<LLTextBox>("connection_loading_text");
    mStatusLoadingIndicator = getChild<LLUICtrl>("connection_loading_indicator");
	return LLFloater::postBuild();
}

void LLFloaterFacebook::showPhotoPanel()
{
	LLTabContainer* parent = dynamic_cast<LLTabContainer*>(mFacebookPhotoPanel->getParent());
	if (!parent)
	{
		llwarns << "Cannot find panel container" << llendl;
		return;
	}

	parent->selectTabPanel(mFacebookPhotoPanel);
}

void LLFloaterFacebook::draw()
{
    if (mStatusErrorText && mStatusLoadingText && mStatusLoadingIndicator)
    {
        mStatusErrorText->setVisible(false);
        mStatusLoadingText->setVisible(false);
        mStatusLoadingIndicator->setVisible(false);
        LLFacebookConnect::EConnectionState connection_state = LLFacebookConnect::instance().getConnectionState();
        std::string status_text;
        
        switch (connection_state)
        {
        case LLFacebookConnect::FB_NOT_CONNECTED:
            // No status displayed when first opening the panel and no connection done
        case LLFacebookConnect::FB_CONNECTED:
            // When successfully connected, no message is displayed
        case LLFacebookConnect::FB_POSTED:
            // No success message to show since we actually close the floater after successful posting completion
            break;
        case LLFacebookConnect::FB_CONNECTION_IN_PROGRESS:
            // Connection loading indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookConnecting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
            break;
        case LLFacebookConnect::FB_POSTING:
            // Posting indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookPosting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
			break;
        case LLFacebookConnect::FB_CONNECTION_FAILED:
            // Error connecting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookErrorConnecting");
            mStatusErrorText->setValue(status_text);
            break;
        case LLFacebookConnect::FB_POST_FAILED:
            // Error posting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookErrorPosting");
            mStatusErrorText->setValue(status_text);
            break;
		case LLFacebookConnect::FB_DISCONNECTING:
			// Disconnecting loading indicator
			mStatusLoadingText->setVisible(true);
			status_text = LLTrans::getString("SocialFacebookDisconnecting");
			mStatusLoadingText->setValue(status_text);
			mStatusLoadingIndicator->setVisible(true);
			break;
		case LLFacebookConnect::FB_DISCONNECT_FAILED:
			// Error disconnecting from the service
			mStatusErrorText->setVisible(true);
			status_text = LLTrans::getString("SocialFacebookErrorDisconnecting");
			mStatusErrorText->setValue(status_text);
			break;
        }
    }
	LLFloater::draw();
}

