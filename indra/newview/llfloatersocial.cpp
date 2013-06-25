/** 
* @file llfloatersocial.cpp
* @brief Implementation of llfloatersocial
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

#include "llfloatersocial.h"

#include "llagent.h"
#include "llagentui.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "llslurl.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"

static LLRegisterPanelClassWrapper<LLSocialPhotoPanel> t_panel_photo("llsocialphotopanel");
static LLRegisterPanelClassWrapper<LLSocialCheckinPanel> t_panel_checkin("llsocialcheckinpanel");

std::string get_map_url()
{
    LLVector3d center_agent = gAgent.getRegion()->getCenterGlobal();
    int x_pos = center_agent[0] / 256.0;
    int y_pos = center_agent[1] / 256.0;
    std::string map_url = gSavedSettings.getString("CurrentMapServerURL") + llformat("map-1-%d-%d-objects.jpg", x_pos, y_pos);
    return map_url;
}

LLSocialPhotoPanel::LLSocialPhotoPanel() :
mRefreshBtn(NULL),
mRefreshLabel(NULL),
mSucceessLblPanel(NULL),
mFailureLblPanel(NULL),
mThumbnailPlaceholder(NULL)
{
	mCommitCallbackRegistrar.add("PostToFacebook.Send", boost::bind(&LLSocialPhotoPanel::onSend, this));
}



LLSocialPhotoPanel::~LLSocialPhotoPanel()
{
	if(mPreviewHandle.get())
	{
		mPreviewHandle.get()->die();
	}
}

BOOL LLSocialPhotoPanel::postBuild()
{
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", boost::bind(&LLSocialPhotoPanel::onClickNewSnapshot, this));
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	mPreviewHandle = previewp->getHandle();	

	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

	return LLPanel::postBuild();
}

void LLSocialPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = static_cast<LLSnapshotLivePreview*>(mPreviewHandle.get());
	//LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (previewp /*&& view*/)
	{
		//view->impl.setStatus(Impl::STATUS_READY);
		lldebugs << "updating snapshot" << llendl;
		previewp->updateSnapshot(TRUE);
	}
}

void LLSocialPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

	LLPanel::draw();

	if(previewp && previewp->getThumbnailImage())
	{
		bool working = false; //impl.getStatus() == Impl::STATUS_WORKING;
		const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
		const S32 thumbnail_w = previewp->getThumbnailWidth();
		const S32 thumbnail_h = previewp->getThumbnailHeight();

		// calc preview offset within the preview rect
		const S32 local_offset_x = (thumbnail_rect.getWidth() - thumbnail_w) / 2 ;
		const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ; // preview y pos within the preview rect

		// calc preview offset within the floater rect
		S32 offset_x = thumbnail_rect.mLeft + local_offset_x;
		S32 offset_y = thumbnail_rect.mBottom + local_offset_y;

		LLUICtrl * snapshot_panel = getChild<LLUICtrl>("snapshot_panel");
		snapshot_panel->localPointToOtherView(offset_x, offset_y, &offset_x, &offset_y, gFloaterView->getParentFloater(this));

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		// Apply floater transparency to the texture unless the floater is focused.
		F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
		LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
		gl_draw_scaled_image(offset_x, offset_y, 
			thumbnail_w, thumbnail_h,
			previewp->getThumbnailImage(), color % alpha);

		previewp->drawPreviewRect(offset_x, offset_y) ;

		// Draw some controls on top of the preview thumbnail.
		static const S32 PADDING = 5;
		static const S32 REFRESH_LBL_BG_HEIGHT = 32;

		// Reshape and position the posting result message panels at the top of the thumbnail.
		// Do this regardless of current posting status (finished or not) to avoid flicker
		// when the result message is displayed for the first time.
		// if (impl.getStatus() == Impl::STATUS_FINISHED)
		{
			LLRect result_lbl_rect = mSucceessLblPanel->getRect();
			const S32 result_lbl_h = result_lbl_rect.getHeight();
			result_lbl_rect.setLeftTopAndSize(local_offset_x, local_offset_y + thumbnail_h, thumbnail_w - 1, result_lbl_h);
			mSucceessLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
			mSucceessLblPanel->setRect(result_lbl_rect);
			mFailureLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
			mFailureLblPanel->setRect(result_lbl_rect);
		}

		// Position the refresh button in the bottom left corner of the thumbnail.
		mRefreshBtn->setOrigin(local_offset_x + PADDING, local_offset_y + PADDING);

		if (/*impl.mNeedRefresh*/false)
		{
			// Place the refresh hint text to the right of the refresh button.
			const LLRect& refresh_btn_rect = mRefreshBtn->getRect();
			mRefreshLabel->setOrigin(refresh_btn_rect.mLeft + refresh_btn_rect.getWidth() + PADDING, refresh_btn_rect.mBottom);

			// Draw the refresh hint background.
			LLRect refresh_label_bg_rect(offset_x, offset_y + REFRESH_LBL_BG_HEIGHT, offset_x + thumbnail_w - 1, offset_y);
			gl_rect_2d(refresh_label_bg_rect, LLColor4::white % 0.9f, TRUE);
		}

		gGL.pushUIMatrix();
		LLUI::translate((F32) thumbnail_rect.mLeft, (F32) thumbnail_rect.mBottom);
		mThumbnailPlaceholder->draw();
		gGL.popUIMatrix();
	}
}

void LLSocialPhotoPanel::onSend()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	if (add_location)
	{
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		if (caption.empty())
			caption = slurl.getSLURLString();
		else
			caption = caption + " " + slurl.getSLURLString();
	}
	//LLFacebookConnect::instance().sharePhoto(LLFloaterSnapshot::getImageData(), caption);
	//LLWebProfile::uploadImage(LLFloaterSnapshot::getImageData(), caption, add_location, boost::bind(&LLPanelSnapshotFacebook::onImageUploaded, this, caption, _1));
	//LLFloaterSnapshot::postSave();
}


LLSocialCheckinPanel::LLSocialCheckinPanel() :
    mMapUrl("")
{
	mCommitCallbackRegistrar.add("SocialSharing.SendCheckin", boost::bind(&LLSocialCheckinPanel::onSend, this));
}

/*virtual*/
void LLSocialCheckinPanel::setVisible(BOOL visible)
{
    if (visible)
    {
        mMapUrl = get_map_url();
    }
    LLPanel::setVisible(visible);
}

void LLSocialCheckinPanel::onSend()
{
	// Get the location SLURL
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	std::string slurl_string = slurl.getSLURLString();
    
	// Get the region name
	std::string region_name = gAgent.getRegion()->getName();
    
	// Get the region description
	std::string description;
	LLAgentUI::buildLocationString(description, LLAgentUI::LOCATION_FORMAT_NORMAL_COORDS, gAgent.getPositionAgent());
    
    
	// Optionally add the region map view
	bool add_map_view = getChild<LLUICtrl>("add_place_view_cb")->getValue().asBoolean();
    std::string map_url = (add_map_view ? mMapUrl : "");
    
	// Get the caption
	std::string caption = getChild<LLUICtrl>("place_caption")->getValue().asString();

    // Post all that to Facebook
	LLFacebookConnect::instance().postCheckin(slurl_string, region_name, description, map_url, caption);
    
    // Close the floater once "Post" has been pushed
	LLFloater* floater = getParentByType<LLFloater>();
    if (floater)
    {
        floater->closeFloater();
    }
}


LLFloaterSocial::LLFloaterSocial(const LLSD& key) : 
LLFloater(key),
mSocialPhotoPanel(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterSocial::onCancel, this));
}

void LLFloaterSocial::onCancel()
{
    closeFloater();
}

BOOL LLFloaterSocial::postBuild()
{
	mSocialPhotoPanel = static_cast<LLSocialPhotoPanel*>(getChild<LLUICtrl>("social_photo_tab"));

	return LLFloater::postBuild();
}

void LLFloaterSocial::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = static_cast<LLSnapshotLivePreview *>(mSocialPhotoPanel->mPreviewHandle.get());
	if(preview)
	{
		lldebugs << "opened, updating snapshot" << llendl;
		preview->updateSnapshot(TRUE);
	}
}
