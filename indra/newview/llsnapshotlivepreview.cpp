/** 
* @file llsnapshotlivepreview.cpp
* @brief Implementation of llsnapshotlivepreview
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

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llcombobox.h"
#include "lleconomy.h"
#include "llfloaterperms.h"
#include "llfloaterreg.h"
#include "llfloaterfacebook.h"
#include "llfloaterflickr.h"
#include "llfloatertwitter.h"
#include "llimagefilter.h"
#include "llimagefiltersmanager.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "lllandmarkactions.h"
#include "lllocalcliprect.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "llsnapshotlivepreview.h"
#include "lltoolfocus.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llviewerstats.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llwebsharing.h"
#include "llwindow.h"
#include "llworld.h"

const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;

F32 SHINE_TIME = 0.5f;
F32 SHINE_WIDTH = 0.6f;
F32 SHINE_OPACITY = 0.3f;
F32 FALL_TIME = 0.6f;
S32 BORDER_WIDTH = 6;

const S32 MAX_TEXTURE_SIZE = 512 ; //max upload texture size 512 * 512

std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;

LLSnapshotLivePreview::LLSnapshotLivePreview (const LLSnapshotLivePreview::Params& p) 
	:	LLView(p),
	mColor(1.f, 0.f, 0.f, 0.5f), 
	mCurImageIndex(0),
	mPreviewImage(NULL),
	mThumbnailImage(NULL) ,
	mThumbnailWidth(0),
	mThumbnailHeight(0),
    mThumbnailSubsampled(FALSE),
	mPreviewImageEncoded(NULL),
	mFormattedImage(NULL),
	mShineCountdown(0),
	mFlashAlpha(0.f),
	mNeedsFlash(TRUE),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mDataSize(0),
	mSnapshotType(SNAPSHOT_POSTCARD),
	mSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat"))),
	mSnapshotUpToDate(FALSE),
	mCameraPos(LLViewerCamera::getInstance()->getOrigin()),
	mCameraRot(LLViewerCamera::getInstance()->getQuaternion()),
	mSnapshotActive(FALSE),
	mSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR),
    mFilterName(""),
    mAllowRenderUI(TRUE),
    mViewContainer(NULL)
{
	setSnapshotQuality(gSavedSettings.getS32("SnapshotQuality"));
	mSnapshotDelayTimer.setTimerExpirySec(0.0f);
	mSnapshotDelayTimer.start();
	// 	gIdleCallbacks.addFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.insert(this);
	setFollowsAll();
	mWidth[0] = gViewerWindow->getWindowWidthRaw();
	mWidth[1] = gViewerWindow->getWindowWidthRaw();
	mHeight[0] = gViewerWindow->getWindowHeightRaw();
	mHeight[1] = gViewerWindow->getWindowHeightRaw();
	mImageScaled[0] = FALSE;
	mImageScaled[1] = FALSE;

	mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;
	mThumbnailUpdateLock = FALSE ;
	mThumbnailUpToDate   = FALSE ;
}

LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
	// delete images
	mPreviewImage = NULL;
	mPreviewImageEncoded = NULL;
	mFormattedImage = NULL;

	// 	gIdleCallbacks.deleteFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.erase(this);
}

void LLSnapshotLivePreview::setMaxImageSize(S32 size) 
{
    mMaxImageSize = llmin(size,(S32)(MAX_SNAPSHOT_IMAGE_SIZE));
}

LLViewerTexture* LLSnapshotLivePreview::getCurrentImage()
{
	return mViewerImage[mCurImageIndex];
}

F32 LLSnapshotLivePreview::getImageAspect()
{
	if (!getCurrentImage())
	{
		return 0.f;
	}
	// mKeepAspectRatio) == gSavedSettings.getBOOL("KeepAspectForSnapshot"))
    return (mKeepAspectRatio ? ((F32)getRect().getWidth()) / ((F32)getRect().getHeight()) : ((F32)getWidth()) / ((F32)getHeight()));
}

void LLSnapshotLivePreview::updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail, F32 delay)
{
	lldebugs << "updateSnapshot: mSnapshotUpToDate = " << getSnapshotUpToDate() << llendl;

	// Update snapshot if requested.
	if (new_snapshot)
	{
        if (getSnapshotUpToDate())
        {
            S32 old_image_index = mCurImageIndex;
            mCurImageIndex = (mCurImageIndex + 1) % 2; 
            setSize(mWidth[old_image_index], mHeight[old_image_index]);
            mFallAnimTimer.start();		
        }
        mSnapshotUpToDate = FALSE; 		

        // Update snapshot source rect depending on whether we keep the aspect ratio.
        LLRect& rect = mImageRect[mCurImageIndex];
        rect.set(0, getRect().getHeight(), getRect().getWidth(), 0);

        F32 image_aspect_ratio = ((F32)getWidth()) / ((F32)getHeight());
        F32 window_aspect_ratio = ((F32)getRect().getWidth()) / ((F32)getRect().getHeight());

        if (mKeepAspectRatio)//gSavedSettings.getBOOL("KeepAspectForSnapshot"))
        {
            if (image_aspect_ratio > window_aspect_ratio)
            {
                // trim off top and bottom
                S32 new_height = llround((F32)getRect().getWidth() / image_aspect_ratio); 
                rect.mBottom += (getRect().getHeight() - new_height) / 2;
                rect.mTop -= (getRect().getHeight() - new_height) / 2;
            }
            else if (image_aspect_ratio < window_aspect_ratio)
            {
                // trim off left and right
                S32 new_width = llround((F32)getRect().getHeight() * image_aspect_ratio); 
                rect.mLeft += (getRect().getWidth() - new_width) / 2;
                rect.mRight -= (getRect().getWidth() - new_width) / 2;
            }
        }

        // Stop shining animation.
        mShineAnimTimer.stop();
		mSnapshotDelayTimer.start();
		mSnapshotDelayTimer.setTimerExpirySec(delay);
        
        // Tell the floater container that the snapshot is in the process of updating itself
        if (mViewContainer)
        {
            mViewContainer->notify(LLSD().with("snapshot-updating", true));
        }
	}

	// Update thumbnail if requested.
	if (new_thumbnail)
	{
		mThumbnailUpToDate = FALSE ;
	}
}

// Return true if the quality has been changed, false otherwise
bool LLSnapshotLivePreview::setSnapshotQuality(S32 quality, bool set_by_user)
{
	llclamp(quality, 0, 100);
	if (quality != mSnapshotQuality)
	{
		mSnapshotQuality = quality;
        if (set_by_user)
        {
            gSavedSettings.setS32("SnapshotQuality", quality);
        }
        mFormattedImage = NULL;     // Invalidate the already formatted image if any
        return true;
	}
    return false;
}

void LLSnapshotLivePreview::drawPreviewRect(S32 offset_x, S32 offset_y)
{
	F32 line_width ; 
	glGetFloatv(GL_LINE_WIDTH, &line_width) ;
	glLineWidth(2.0f * line_width) ;
	LLColor4 color(0.0f, 0.0f, 0.0f, 1.0f) ;
	gl_rect_2d( mPreviewRect.mLeft + offset_x, mPreviewRect.mTop + offset_y,
		mPreviewRect.mRight + offset_x, mPreviewRect.mBottom + offset_y, color, FALSE ) ;
	glLineWidth(line_width) ;

	//draw four alpha rectangles to cover areas outside of the snapshot image
	if(!mKeepAspectRatio)
	{
		LLColor4 alpha_color(0.5f, 0.5f, 0.5f, 0.8f) ;
		S32 dwl = 0, dwr = 0 ;
		if(mThumbnailWidth > mPreviewRect.getWidth())
		{
			dwl = (mThumbnailWidth - mPreviewRect.getWidth()) >> 1 ;
			dwr = mThumbnailWidth - mPreviewRect.getWidth() - dwl ;

			gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y,
				mPreviewRect.mLeft + offset_x, mPreviewRect.mBottom + offset_y, alpha_color, TRUE ) ;
			gl_rect_2d( mPreviewRect.mRight + offset_x, mPreviewRect.mTop + offset_y,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y, alpha_color, TRUE ) ;
		}

		if(mThumbnailHeight > mPreviewRect.getHeight())
		{
			S32 dh = (mThumbnailHeight - mPreviewRect.getHeight()) >> 1 ;
			gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mBottom + offset_y ,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y - dh, alpha_color, TRUE ) ;

			dh = mThumbnailHeight - mPreviewRect.getHeight() - dh ;
			gl_rect_2d( mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y + dh,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mTop + offset_y, alpha_color, TRUE ) ;
		}
	}
}

//called when the frame is frozen.
void LLSnapshotLivePreview::draw()
{
	if (getCurrentImage() &&
		mPreviewImageEncoded.notNull() &&
		getSnapshotUpToDate())
	{
		LLColor4 bg_color(0.f, 0.f, 0.3f, 0.4f);
		gl_rect_2d(getRect(), bg_color);
		const LLRect& rect = getImageRect();
		LLRect shadow_rect = rect;
		shadow_rect.stretch(BORDER_WIDTH);
		gl_drop_shadow(shadow_rect.mLeft, shadow_rect.mTop, shadow_rect.mRight, shadow_rect.mBottom, LLColor4(0.f, 0.f, 0.f, mNeedsFlash ? 0.f :0.5f), 10);

		LLColor4 image_color(1.f, 1.f, 1.f, 1.f);
		gGL.color4fv(image_color.mV);
		gGL.getTexUnit(0)->bind(getCurrentImage());
		// calculate UV scale
		F32 uv_width = isImageScaled() ? 1.f : llmin((F32)getWidth() / (F32)getCurrentImage()->getWidth(), 1.f);
		F32 uv_height = isImageScaled() ? 1.f : llmin((F32)getHeight() / (F32)getCurrentImage()->getHeight(), 1.f);
		gGL.pushMatrix();
		{
			gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom, 0.f);
			gGL.begin(LLRender::QUADS);
			{
				gGL.texCoord2f(uv_width, uv_height);
				gGL.vertex2i(rect.getWidth(), rect.getHeight() );

				gGL.texCoord2f(0.f, uv_height);
				gGL.vertex2i(0, rect.getHeight() );

				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2i(0, 0);

				gGL.texCoord2f(uv_width, 0.f);
				gGL.vertex2i(rect.getWidth(), 0);
			}
			gGL.end();
		}
		gGL.popMatrix();

		gGL.color4f(1.f, 1.f, 1.f, mFlashAlpha);
		gl_rect_2d(getRect());
		if (mNeedsFlash)
		{
			if (mFlashAlpha < 1.f)
			{
				mFlashAlpha = lerp(mFlashAlpha, 1.f, LLCriticalDamp::getInterpolant(0.02f));
			}
			else
			{
				mNeedsFlash = FALSE;
			}
		}
		else
		{
			mFlashAlpha = lerp(mFlashAlpha, 0.f, LLCriticalDamp::getInterpolant(0.15f));
		}

		// Draw shining animation if appropriate.
		if (mShineCountdown > 0)
		{
			mShineCountdown--;
			if (mShineCountdown == 0)
			{
				mShineAnimTimer.start();
			}
		}
		else if (mShineAnimTimer.getStarted())
		{
			lldebugs << "Drawing shining animation" << llendl;
			F32 shine_interp = llmin(1.f, mShineAnimTimer.getElapsedTimeF32() / SHINE_TIME);

			// draw "shine" effect
			LLLocalClipRect clip(getLocalRect());
			{
				// draw diagonal stripe with gradient that passes over screen
				S32 x1 = gViewerWindow->getWindowWidthScaled() * llround((clamp_rescale(shine_interp, 0.f, 1.f, -1.f - SHINE_WIDTH, 1.f)));
				S32 x2 = x1 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 x3 = x2 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 y1 = 0;
				S32 y2 = gViewerWindow->getWindowHeightScaled();

				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gGL.begin(LLRender::QUADS);
				{
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x1, y1);
					gGL.vertex2i(x1 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x2, y1);

					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2, y1);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x3 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x3, y1);
				}
				gGL.end();
			}

			// if we're at the end of the animation, stop
			if (shine_interp >= 1.f)
			{
				mShineAnimTimer.stop();
			}
		}
	}

	// draw framing rectangle
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		const LLRect& outline_rect = getImageRect();
		gGL.begin(LLRender::QUADS);
		{
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
		}
		gGL.end();
	}

	// draw old image dropping away
	if (mFallAnimTimer.getStarted())
	{
		S32 old_image_index = (mCurImageIndex + 1) % 2;
		if (mViewerImage[old_image_index].notNull() && mFallAnimTimer.getElapsedTimeF32() < FALL_TIME)
		{
			lldebugs << "Drawing fall animation" << llendl;
			F32 fall_interp = mFallAnimTimer.getElapsedTimeF32() / FALL_TIME;
			F32 alpha = clamp_rescale(fall_interp, 0.f, 1.f, 0.8f, 0.4f);
			LLColor4 image_color(1.f, 1.f, 1.f, alpha);
			gGL.color4fv(image_color.mV);
			gGL.getTexUnit(0)->bind(mViewerImage[old_image_index]);
			// calculate UV scale
			// *FIX get this to work with old image
			BOOL rescale = !mImageScaled[old_image_index] && mViewerImage[mCurImageIndex].notNull();
			F32 uv_width = rescale ? llmin((F32)mWidth[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getWidth(), 1.f) : 1.f;
			F32 uv_height = rescale ? llmin((F32)mHeight[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getHeight(), 1.f) : 1.f;
			gGL.pushMatrix();
			{
				LLRect& rect = mImageRect[old_image_index];
				gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom - llround(getRect().getHeight() * 2.f * (fall_interp * fall_interp)), 0.f);
				gGL.rotatef(-45.f * fall_interp, 0.f, 0.f, 1.f);
				gGL.begin(LLRender::QUADS);
				{
					gGL.texCoord2f(uv_width, uv_height);
					gGL.vertex2i(rect.getWidth(), rect.getHeight() );

					gGL.texCoord2f(0.f, uv_height);
					gGL.vertex2i(0, rect.getHeight() );

					gGL.texCoord2f(0.f, 0.f);
					gGL.vertex2i(0, 0);

					gGL.texCoord2f(uv_width, 0.f);
					gGL.vertex2i(rect.getWidth(), 0);
				}
				gGL.end();
			}
			gGL.popMatrix();
		}
	}
}

/*virtual*/ 
void LLSnapshotLivePreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect old_rect = getRect();
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		lldebugs << "window reshaped, updating snapshot" << llendl;
		updateSnapshot(TRUE);
	}
}

BOOL LLSnapshotLivePreview::setThumbnailImageSize()
{
	if (getWidth() < 10 || getHeight() < 10)
	{
		return FALSE ;
	}
	S32 width  = (mThumbnailSubsampled ? mPreviewImage->getWidth()  : gViewerWindow->getWindowWidthRaw());
	S32 height = (mThumbnailSubsampled ? mPreviewImage->getHeight() : gViewerWindow->getWindowHeightRaw()) ;

	F32 aspect_ratio = ((F32)width) / ((F32)height);

	// UI size for thumbnail
	S32 max_width  = mThumbnailPlaceholderRect.getWidth();
	S32 max_height = mThumbnailPlaceholderRect.getHeight();

	if (aspect_ratio > (F32)max_width / (F32)max_height)
	{
		// image too wide, shrink to width
		mThumbnailWidth = max_width;
		mThumbnailHeight = llround((F32)max_width / aspect_ratio);
	}
	else
	{
		// image too tall, shrink to height
		mThumbnailHeight = max_height;
		mThumbnailWidth = llround((F32)max_height * aspect_ratio);
	}
    
	if (mThumbnailWidth > width || mThumbnailHeight > height)
	{
		return FALSE ;//if the window is too small, ignore thumbnail updating.
	}

	S32 left = 0 , top = mThumbnailHeight, right = mThumbnailWidth, bottom = 0 ;
	if (!mKeepAspectRatio)
	{
		F32 ratio_x = (F32)getWidth()  / width ;
		F32 ratio_y = (F32)getHeight() / height ;

        if (ratio_x > ratio_y)
        {
            top = (S32)(top * ratio_y / ratio_x) ;
        }
        else
        {
            right = (S32)(right * ratio_x / ratio_y) ;
        }
		left = (S32)((mThumbnailWidth - right) * 0.5f) ;
		bottom = (S32)((mThumbnailHeight - top) * 0.5f) ;
		top += bottom ;
		right += left ;
	}
	mPreviewRect.set(left - 1, top + 1, right + 1, bottom - 1) ;

	return TRUE ;
}

void LLSnapshotLivePreview::generateThumbnailImage(BOOL force_update)
{	
	if(mThumbnailUpdateLock) //in the process of updating
	{
		return ;
	}
	if(getThumbnailUpToDate() && !force_update)//already updated
	{
		return ;
	}
	if(getWidth() < 10 || getHeight() < 10)
	{
		return ;
	}

	////lock updating
	mThumbnailUpdateLock = TRUE ;

	if(!setThumbnailImageSize())
	{
		mThumbnailUpdateLock = FALSE ;
		mThumbnailUpToDate = TRUE ;
		return ;
	}

	if(mThumbnailImage)
	{
		resetThumbnailImage() ;
	}		

	LLPointer<LLImageRaw> raw = new LLImageRaw;
    
    if (mThumbnailSubsampled)
    {
        // The thumbnail is be a subsampled version of the preview (used in SL Share previews, i.e. Flickr, Twitter, Facebook)
		raw->resize( mPreviewImage->getWidth(),
                     mPreviewImage->getHeight(),
                     mPreviewImage->getComponents());
        raw->copy(mPreviewImage);
        // Scale to the thumbnal size modulo a power of 2
        S32 width  = LLImageRaw::expandDimToPowerOfTwo(mThumbnailWidth,MAX_IMAGE_SIZE);
        S32 height = LLImageRaw::expandDimToPowerOfTwo(mThumbnailHeight,MAX_IMAGE_SIZE);
        if (!raw->scale(width, height))
        {
            raw = NULL ;
        }
    }
    else
    {
        // The thumbnail is a screen view with screen grab positioning preview
        if(!gViewerWindow->thumbnailSnapshot(raw,
                                         mThumbnailWidth, mThumbnailHeight,
                                         mAllowRenderUI && gSavedSettings.getBOOL("RenderUIInSnapshot"),
                                         FALSE,
                                         mSnapshotBufferType) )
        {
            raw = NULL ;
        }
        else
        {
            raw->expandToPowerOfTwo();
        }
    }
    
	if (raw)
	{
        // Filter the thumbnail
        if (getFilter() != "")
        {
            std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
            if (filter_path != "")
            {
                LLImageFilter filter(filter_path);
                filter.executeFilter(raw);
            }
            else
            {
                llwarns << "Couldn't find a path to the following filter : " << getFilter() << llendl;
            }
        }
		mThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mThumbnailUpToDate = TRUE ;
	}

	//unlock updating
	mThumbnailUpdateLock = FALSE ;		
}


// Called often. Checks whether it's time to grab a new snapshot and if so, does it.
// Returns TRUE if new snapshot generated, FALSE otherwise.
//static 
BOOL LLSnapshotLivePreview::onIdle( void* snapshot_preview )
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)snapshot_preview;
	if (previewp->getWidth() == 0 || previewp->getHeight() == 0)
	{
		llwarns << "Incorrect dimensions: " << previewp->getWidth() << "x" << previewp->getHeight() << llendl;
		return FALSE;
	}

	// If we're in freeze-frame mode and camera has moved, update snapshot.
	LLVector3 new_camera_pos = LLViewerCamera::getInstance()->getOrigin();
	LLQuaternion new_camera_rot = LLViewerCamera::getInstance()->getQuaternion();
	if (gSavedSettings.getBOOL("FreezeTime") && 
		(new_camera_pos != previewp->mCameraPos || dot(new_camera_rot, previewp->mCameraRot) < 0.995f))
	{
		previewp->mCameraPos = new_camera_pos;
		previewp->mCameraRot = new_camera_rot;
		// request a new snapshot whenever the camera moves, with a time delay
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		lldebugs << "camera moved, updating thumbnail" << llendl;
		previewp->updateSnapshot(
			autosnap, // whether a new snapshot is needed or merely invalidate the existing one
			FALSE, // or if 1st arg is false, whether to produce a new thumbnail image.
			autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f); // shutter delay if 1st arg is true.
	}

	// see if it's time yet to snap the shot and bomb out otherwise.
	previewp->mSnapshotActive = 
		(previewp->mSnapshotDelayTimer.getStarted() &&	previewp->mSnapshotDelayTimer.hasExpired())
		&& !LLToolCamera::getInstance()->hasMouseCapture(); // don't take snapshots while ALT-zoom active
	if (!previewp->mSnapshotActive && previewp->getSnapshotUpToDate() && previewp->getThumbnailUpToDate())
	{
		return FALSE;
	}

	// time to produce a snapshot
	if(!previewp->getSnapshotUpToDate())
    {
        lldebugs << "producing snapshot" << llendl;
        if (!previewp->mPreviewImage)
        {
            previewp->mPreviewImage = new LLImageRaw;
        }

        previewp->setVisible(FALSE);
        previewp->setEnabled(FALSE);

        previewp->getWindow()->incBusyCount();
        previewp->setImageScaled(FALSE);

        // grab the raw image
        if (gViewerWindow->rawSnapshot(
                previewp->mPreviewImage,
                previewp->getWidth(),
                previewp->getHeight(),
                previewp->mKeepAspectRatio,//gSavedSettings.getBOOL("KeepAspectForSnapshot"),
                previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE,
                previewp->mAllowRenderUI && gSavedSettings.getBOOL("RenderUIInSnapshot"),
                FALSE,
                previewp->mSnapshotBufferType,
                previewp->getMaxImageSize()))
        {
            // Invalidate/delete any existing encoded image
            previewp->mPreviewImageEncoded = NULL;
            // Invalidate/delete any existing formatted image
            previewp->mFormattedImage = NULL;
            // Update the data size
            previewp->estimateDataSize();

            // Full size preview is set: get the decoded image result and save it for animation
            if (gSavedSettings.getBOOL("UseFreezeFrame"))
            {
                // Get the decoded version of the formatted image
                previewp->getEncodedImage();
            
                // We need to scale that a bit for display...
                LLPointer<LLImageRaw> scaled = new LLImageRaw(
                    previewp->mPreviewImageEncoded->getData(),
                    previewp->mPreviewImageEncoded->getWidth(),
                    previewp->mPreviewImageEncoded->getHeight(),
                    previewp->mPreviewImageEncoded->getComponents());

                if (!scaled->isBufferInvalid())
                {
                    // leave original image dimensions, just scale up texture buffer
                    if (previewp->mPreviewImageEncoded->getWidth() > 1024 || previewp->mPreviewImageEncoded->getHeight() > 1024)
                    {
                        // go ahead and shrink image to appropriate power of 2 for display
                        scaled->biasedScaleToPowerOfTwo(1024);
                        previewp->setImageScaled(TRUE);
                    }
                    else
                    {
                        // expand image but keep original image data intact
                        scaled->expandToPowerOfTwo(1024, FALSE);
                    }

                    previewp->mViewerImage[previewp->mCurImageIndex] = LLViewerTextureManager::getLocalTexture(scaled.get(), FALSE);
                    LLPointer<LLViewerTexture> curr_preview_image = previewp->mViewerImage[previewp->mCurImageIndex];
                    gGL.getTexUnit(0)->bind(curr_preview_image);
                    curr_preview_image->setFilteringOption(previewp->getSnapshotType() == SNAPSHOT_TEXTURE ? LLTexUnit::TFO_ANISOTROPIC : LLTexUnit::TFO_POINT);
                    curr_preview_image->setAddressMode(LLTexUnit::TAM_CLAMP);

                    previewp->mPosTakenGlobal = gAgentCamera.getCameraPositionGlobal();
                    previewp->mShineCountdown = 4; // wait a few frames to avoid animation glitch due to readback this frame
                }
            }
            // The snapshot is updated now...
            previewp->mSnapshotUpToDate = TRUE;
        
            // We need to update the thumbnail though
            previewp->setThumbnailImageSize();
            previewp->generateThumbnailImage(TRUE) ;
        }
        previewp->getWindow()->decBusyCount();
        previewp->setVisible(gSavedSettings.getBOOL("UseFreezeFrame")); // only show fullscreen preview when in freeze frame mode
        previewp->mSnapshotDelayTimer.stop();
        previewp->mSnapshotActive = FALSE;
        lldebugs << "done creating snapshot" << llendl;
    }
    
    if (!previewp->getThumbnailUpToDate())
	{
		previewp->generateThumbnailImage() ;
	}
    
    // Tell the floater container that the snapshot is updated now
    if (previewp->mViewContainer)
    {
        previewp->mViewContainer->notify(LLSD().with("snapshot-updated", true));
    }

	return TRUE;
}

S32 LLSnapshotLivePreview::getEncodedImageWidth() const
{
    S32 width = getWidth();
    if (getSnapshotType() == SNAPSHOT_TEXTURE)
    {
        width = LLImageRaw::biasedDimToPowerOfTwo(width,MAX_TEXTURE_SIZE);
    }
    return width;
}
S32 LLSnapshotLivePreview::getEncodedImageHeight() const
{
    S32 height = getHeight();
    if (getSnapshotType() == SNAPSHOT_TEXTURE)
    {
        height = LLImageRaw::biasedDimToPowerOfTwo(height,MAX_TEXTURE_SIZE);
    }
    return height;
}

LLPointer<LLImageRaw> LLSnapshotLivePreview::getEncodedImage()
{
	if (!mPreviewImageEncoded)
	{
		mPreviewImageEncoded = new LLImageRaw;
    
		mPreviewImageEncoded->resize(
            mPreviewImage->getWidth(),
            mPreviewImage->getHeight(),
            mPreviewImage->getComponents());
        
		if (getSnapshotType() == SNAPSHOT_TEXTURE)
		{
            // We don't store the intermediate formatted image in mFormattedImage in the J2C case 
			lldebugs << "Encoding new image of format J2C" << llendl;
			LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
            // Copy the preview
			LLPointer<LLImageRaw> scaled = new LLImageRaw(
                                                          mPreviewImage->getData(),
                                                          mPreviewImage->getWidth(),
                                                          mPreviewImage->getHeight(),
                                                          mPreviewImage->getComponents());
            // Scale it as required by J2C
			scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
			setImageScaled(TRUE);
            // Compress to J2C
			if (formatted->encode(scaled, 0.f))
			{
                // We can update the data size precisely at that point
				mDataSize = formatted->getDataSize();
                // Decompress back
				formatted->decode(mPreviewImageEncoded, 0);
			}
		}
		else
		{
            // Update mFormattedImage if necessary
            getFormattedImage();
            if (getSnapshotFormat() == LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP)
            {
                // BMP hack : copy instead of decode otherwise decode will crash.
                mPreviewImageEncoded->copy(mPreviewImage);
            }
            else
            {
                // Decode back
                mFormattedImage->decode(mPreviewImageEncoded, 0);
            }
		}
	}
    return mPreviewImageEncoded;
}

// We actually estimate the data size so that we do not require actual compression when showing the preview
// Note : whenever formatted image is computed, mDataSize will be updated to reflect the true size
void LLSnapshotLivePreview::estimateDataSize()
{
    // Compression ratio
    F32 ratio = 1.0;
    
    if (getSnapshotType() == SNAPSHOT_TEXTURE)
    {
        ratio = 8.0;    // This is what we shoot for when compressing to J2C
    }
    else
    {
        LLFloaterSnapshot::ESnapshotFormat format = getSnapshotFormat();
        switch (format)
        {
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
                ratio = 3.0;    // Average observed PNG compression ratio
                break;
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
                // Observed from JPG compression tests
                ratio = (110 - mSnapshotQuality) / 2;
                break;
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
                ratio = 1.0;    // No compression with BMP
                break;
        }
    }
    mDataSize = (S32)((F32)mPreviewImage->getDataSize() / ratio);
}

LLPointer<LLImageFormatted>	LLSnapshotLivePreview::getFormattedImage()
{
    if (!mFormattedImage)
    {
        // Apply the filter to mPreviewImage
        if (getFilter() != "")
        {
            std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
            if (filter_path != "")
            {
                LLImageFilter filter(filter_path);
                filter.executeFilter(mPreviewImage);
            }
            else
            {
                llwarns << "Couldn't find a path to the following filter : " << getFilter() << llendl;
            }
        }
        
        // Create the new formatted image of the appropriate format.
        LLFloaterSnapshot::ESnapshotFormat format = getSnapshotFormat();
        lldebugs << "Encoding new image of format " << format << llendl;
            
        switch (format)
        {
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
                mFormattedImage = new LLImagePNG();
                break;
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
                mFormattedImage = new LLImageJPEG(mSnapshotQuality);
                break;
            case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
                mFormattedImage = new LLImageBMP();
                break;
        }
        if (mFormattedImage->encode(mPreviewImage, 0))
        {
            // We can update the data size precisely at that point
            mDataSize = mFormattedImage->getDataSize();
        }
    }
    return mFormattedImage;
}

void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
	lldebugs << "setSize(" << w << ", " << h << ")" << llendl;
	setWidth(w);
	setHeight(h);
}

void LLSnapshotLivePreview::setSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat format)
{
    if (mSnapshotFormat != format)
    {
        mFormattedImage = NULL;     // Invalidate the already formatted image if any
        mSnapshotFormat = format;
    }
}

void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
	w = getWidth();
	h = getHeight();
}

void LLSnapshotLivePreview::saveTexture()
{
	lldebugs << "saving texture: " << mPreviewImage->getWidth() << "x" << mPreviewImage->getHeight() << llendl;
	// gen a new uuid for this asset
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
	LLPointer<LLImageRaw> scaled = new LLImageRaw(mPreviewImage->getData(),
		mPreviewImage->getWidth(),
		mPreviewImage->getHeight(),
		mPreviewImage->getComponents());

	scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
	lldebugs << "scaled texture to " << scaled->getWidth() << "x" << scaled->getHeight() << llendl;

	if (formatted->encode(scaled, 0.0f))
	{
		LLVFile::writeFile(formatted->getData(), formatted->getDataSize(), gVFS, new_asset_id, LLAssetType::AT_TEXTURE);
		std::string pos_string;
		LLAgentUI::buildLocationString(pos_string, LLAgentUI::LOCATION_FORMAT_FULL);
		std::string who_took_it;
		LLAgentUI::buildFullname(who_took_it);
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
		void *userdata = NULL;
		upload_new_resource(tid,	// tid
			LLAssetType::AT_TEXTURE,
			"Snapshot : " + pos_string,
			"Taken by " + who_took_it + " at " + pos_string,
			0,
			LLFolderType::FT_SNAPSHOT_CATEGORY,
			LLInventoryType::IT_SNAPSHOT,
			PERM_ALL,  // Note: Snapshots to inventory is a special case of content upload
			LLFloaterPerms::getGroupPerms(), // that is more permissive than other uploads
			LLFloaterPerms::getEveryonePerms(),
			"Snapshot : " + pos_string,
			callback, expected_upload_cost, userdata);
		gViewerWindow->playSnapshotAnimAndSound();
	}
	else
	{
		LLNotificationsUtil::add("ErrorEncodingSnapshot");
		llwarns << "Error encoding snapshot" << llendl;
	}

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_SNAPSHOT_COUNT );
}

BOOL LLSnapshotLivePreview::saveLocal()
{
    // Update mFormattedImage if necessary
    getFormattedImage();
    
    // Save the formatted image
	BOOL success = gViewerWindow->saveImageNumbered(mFormattedImage);

	if(success)
	{
		gViewerWindow->playSnapshotAnimAndSound();
	}
	return success;
}

void LLSnapshotLivePreview::saveWeb()
{
    // Update mFormattedImage if necessary
    getFormattedImage();
    
	// *FIX: Will break if the window closes because of CloseSnapshotOnKeep!
	// Needs to pass on ownership of the image.
	LLImageJPEG* jpg = dynamic_cast<LLImageJPEG*>(mFormattedImage.get());
	if(!jpg)
	{
		llwarns << "Formatted image not a JPEG" << llendl;
		return;
	}

	LLSD metadata;
	metadata["description"] = getChild<LLLineEditor>("description")->getText();

	LLLandmarkActions::getRegionNameAndCoordsFromPosGlobal(gAgentCamera.getCameraPositionGlobal(),
		boost::bind(&LLSnapshotLivePreview::regionNameCallback, this, jpg, metadata, _1, _2, _3, _4));

	gViewerWindow->playSnapshotAnimAndSound();
}

void LLSnapshotLivePreview::regionNameCallback(LLImageJPEG* snapshot, LLSD& metadata, const std::string& name, S32 x, S32 y, S32 z)
{
	metadata["slurl"] = LLSLURL(name, LLVector3d(x, y, z)).getSLURLString();

	LLWebSharing::instance().shareSnapshot(snapshot, metadata);
}
