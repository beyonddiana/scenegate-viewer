/** 
 * @file llfloaterimagepreview.cpp
 * @brief LLFloaterImagePreview class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llfloaterimagepreview.h"

#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llglimmediate.h"
#include "llface.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "llviewerimagelist.h"

//static
S32 LLFloaterImagePreview::sUploadAmount = 10;

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16 + 7 + 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;


//-----------------------------------------------------------------------------
// LLFloaterImagePreview()
//-----------------------------------------------------------------------------
LLFloaterImagePreview::LLFloaterImagePreview(const char* filename) : 
	LLFloaterNameDesc(filename)
{
	mLastMouseX = 0;
	mLastMouseY = 0;
	mGLName = 0;
	loadImage(mFilenameAndPath.c_str());
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}

	childSetLabelArg("ok_btn", "[AMOUNT]", llformat("%d",sUploadAmount));

	LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
	if (iface)
	{
		iface->selectFirstItem();
	}
	childSetCommitCallback("clothing_type_combo", onPreviewTypeCommit, this);

	mPreviewRect.set(PREVIEW_HPAD, 
		PREVIEW_TEXTURE_HEIGHT,
		getRect().getWidth() - PREVIEW_HPAD, 
		PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
	mPreviewImageRect.set(0.f, 1.f, 1.f, 0.f);

	childHide("bad_image_text");

	if (mRawImagep.notNull())
	{
		mAvatarPreview = new LLImagePreviewAvatar(256, 256);
		mAvatarPreview->setPreviewTarget("mPelvis", "mUpperBodyMesh0", mRawImagep, 2.f, FALSE);

		mSculptedPreview = new LLImagePreviewSculpted(256, 256);
		mSculptedPreview->setPreviewTarget(mRawImagep, 2.0f);

		if ((mRawImagep->getWidth() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF) &&
			(mRawImagep->getHeight() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF))
			childEnable("lossless_check");
	}
	else
	{
		mAvatarPreview = NULL;
		mSculptedPreview = NULL;
		childShow("bad_image_text");
		childDisable("clothing_type_combo");
		childDisable("ok_btn");
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterImagePreview()
//-----------------------------------------------------------------------------
LLFloaterImagePreview::~LLFloaterImagePreview()
{
	mRawImagep = NULL;
	delete mAvatarPreview;
	delete mSculptedPreview;
	
	if (mGLName)
	{
		glDeleteTextures(1, &mGLName );
	}
}

//static 
//-----------------------------------------------------------------------------
// onPreviewTypeCommit()
//-----------------------------------------------------------------------------
void	LLFloaterImagePreview::onPreviewTypeCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterImagePreview *fp =(LLFloaterImagePreview *)userdata;
	
	if (!fp->mAvatarPreview || !fp->mSculptedPreview)
	{
		return;
	}

	S32 which_mode = 0;

	LLCtrlSelectionInterface* iface = fp->childGetSelectionInterface("clothing_type_combo");
	if (iface)
	{
		which_mode = iface->getFirstSelectedIndex();
	}

	switch(which_mode)
	{
	case 0:
		break;
	case 1:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHairMesh0", fp->mRawImagep, 0.4f, FALSE);
		break;
	case 2:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHeadMesh0", fp->mRawImagep, 0.4f, FALSE);
		break;
	case 3:
		fp->mAvatarPreview->setPreviewTarget("mChest", "mUpperBodyMesh0", fp->mRawImagep, 1.0f, FALSE);
		break;
	case 4:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mLowerBodyMesh0", fp->mRawImagep, 1.2f, FALSE);
		break;
	case 5:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHeadMesh0", fp->mRawImagep, 0.4f, TRUE);
		break;
	case 6:
		fp->mAvatarPreview->setPreviewTarget("mChest", "mUpperBodyMesh0", fp->mRawImagep, 1.2f, TRUE);
		break;
	case 7:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mLowerBodyMesh0", fp->mRawImagep, 1.2f, TRUE);
		break;
	case 8:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mSkirtMesh0", fp->mRawImagep, 1.3f, FALSE);
		break;
	case 9:
		fp->mSculptedPreview->setPreviewTarget(fp->mRawImagep, 2.0f);
		break;
	default:
		break;
	}
	
	fp->mAvatarPreview->refresh();
	fp->mSculptedPreview->refresh();
}

//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterImagePreview::draw()
{
	LLFloater::draw();
	LLRect r = getRect();

	if (mRawImagep.notNull())
	{
		LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
		U32 selected = 0;
		if (iface)
			selected = iface->getFirstSelectedIndex();
		
		if (selected <= 0)
		{
			gl_rect_2d_checkerboard(mPreviewRect);
			LLGLDisable gls_alpha(GL_ALPHA_TEST);

			GLenum format_options[4] = { GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
			GLenum format = format_options[mRawImagep->getComponents()-1];

			GLenum internal_format_options[4] = { GL_LUMINANCE8, GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8 };
			GLenum internal_format = internal_format_options[mRawImagep->getComponents()-1];
		
			if (mGLName)
			{
				LLImageGL::bindExternalTexture( mGLName, 0, GL_TEXTURE_2D ); 
			}
			else
			{
				glGenTextures(1, &mGLName );
				stop_glerror();

				LLImageGL::bindExternalTexture( mGLName, 0, GL_TEXTURE_2D ); 
				stop_glerror();

				glTexImage2D(
					GL_TEXTURE_2D, 0, internal_format, 
					mRawImagep->getWidth(), mRawImagep->getHeight(),
					0, format, GL_UNSIGNED_BYTE, mRawImagep->getData());
				stop_glerror();

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (mAvatarPreview)
				{
					mAvatarPreview->setTexture(mGLName);
					mSculptedPreview->setTexture(mGLName);
				}
			}

			gGL.color3f(1.f, 1.f, 1.f);
			gGL.begin( GL_QUADS );
			{
				gGL.texCoord2f(mPreviewImageRect.mLeft, mPreviewImageRect.mTop);
				gGL.vertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
				gGL.texCoord2f(mPreviewImageRect.mLeft, mPreviewImageRect.mBottom);
				gGL.vertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
				gGL.texCoord2f(mPreviewImageRect.mRight, mPreviewImageRect.mBottom);
				gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
				gGL.texCoord2f(mPreviewImageRect.mRight, mPreviewImageRect.mTop);
				gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
			}
			gGL.end();

			LLImageGL::unbindTexture(0, GL_TEXTURE_2D);

			stop_glerror();
		}
		else
		{
			if ((mAvatarPreview) && (mSculptedPreview))
			{
				gGL.color3f(1.f, 1.f, 1.f);

				if (selected == 9)
					mSculptedPreview->bindTexture();
				else
					mAvatarPreview->bindTexture();

				gGL.begin( GL_QUADS );
				{
					gGL.texCoord2f(0.f, 1.f);
					gGL.vertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
					gGL.texCoord2f(0.f, 0.f);
					gGL.vertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
					gGL.texCoord2f(1.f, 0.f);
					gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
					gGL.texCoord2f(1.f, 1.f);
					gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
				}
				gGL.end();

				if (selected == 9)
					mSculptedPreview->unbindTexture();
				else
					mAvatarPreview->unbindTexture();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// loadImage()
//-----------------------------------------------------------------------------
bool LLFloaterImagePreview::loadImage(const char *src_filename)
{
	// U32 length = strlen(src_filename);
	const char* ext = strrchr(src_filename, '.');
	char error_message[MAX_STRING];
	error_message[0] = '\0';

	U32 codec = IMG_CODEC_INVALID;
	LLString temp_str;
	if( 0 == strnicmp(ext, ".bmp", 4) )
	{
		codec = IMG_CODEC_BMP;
	}
	else if( 0 == strnicmp(ext, ".tga", 4) )
	{
		codec = IMG_CODEC_TGA;
	}
	else if( 0 == strnicmp(ext, ".jpg", 4) || 0 == strnicmp(ext, ".jpeg", 5))
	{
		codec = IMG_CODEC_JPEG;
	}
	else if( 0 == strnicmp(ext, ".png", 4) )
	{
		codec = IMG_CODEC_PNG;
	}

	LLPointer<LLImageRaw> raw_image = new LLImageRaw;

	switch (codec)
	{
	case IMG_CODEC_BMP:
		{
			LLPointer<LLImageBMP> bmp_image = new LLImageBMP;

			if (!bmp_image->load(src_filename))
			{
				return false;
			}
			
			if (!bmp_image->decode(raw_image))
			{
				return false;
			}
		}
		break;
	case IMG_CODEC_TGA:
		{
			LLPointer<LLImageTGA> tga_image = new LLImageTGA;

			if (!tga_image->load(src_filename))
			{
				return false;
			}
			
			if (!tga_image->decode(raw_image))
			{
				return false;
			}

			if(	(tga_image->getComponents() != 3) &&
				(tga_image->getComponents() != 4) )
			{
				tga_image->setLastError( "Image files with less than 3 or more than 4 components are not supported." );
				return false;
			}
		}
		break;
	case IMG_CODEC_JPEG:
		{
			LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG;

			if (!jpeg_image->load(src_filename))
			{
				return false;
			}
			
			if (!jpeg_image->decode(raw_image))
			{
				return false;
			}
		}
		break;
	case IMG_CODEC_PNG:
		{
			LLPointer<LLImagePNG> png_image = new LLImagePNG;

			if (!png_image->load(src_filename))
			{
				return false;
			}
			
			if (!png_image->decode(raw_image))
			{
				return false;
			}
		}
		break;
	default:
		return false;
	}

	raw_image->biasedScaleToPowerOfTwo(1024);
	mRawImagep = raw_image;
	
	return true;
}

//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPreviewRect.pointInRect(x, y))
	{
		bringToFront( x, y );
		gViewerWindow->setMouseCapture(this);
		gViewerWindow->hideCursor();
		mLastMouseX = x;
		mLastMouseY = y;
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleMouseUp()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gViewerWindow->setMouseCapture(FALSE);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleHover(S32 x, S32 y, MASK mask)
{
	MASK local_mask = mask & ~MASK_ALT;

	if (mAvatarPreview && hasMouseCapture())
	{
		if (local_mask == MASK_PAN)
		{
			// pan here
			LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
			if (iface && iface->getFirstSelectedIndex() <= 0)
			{
				mPreviewImageRect.translate((F32)(x - mLastMouseX) * -0.005f * mPreviewImageRect.getWidth(), 
					(F32)(y - mLastMouseY) * -0.005f * mPreviewImageRect.getHeight());
			}
			else
			{
				mAvatarPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
				mSculptedPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
			}
		}
		else if (local_mask == MASK_ORBIT)
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 pitch_radians = (F32)(y - mLastMouseY) * 0.02f;
			
			mAvatarPreview->rotate(yaw_radians, pitch_radians);
			mSculptedPreview->rotate(yaw_radians, pitch_radians);
		}
		else 
		{
			LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
			if (iface && iface->getFirstSelectedIndex() <= 0)
			{
				F32 zoom_amt = (F32)(y - mLastMouseY) * -0.002f;
				mPreviewImageRect.stretch(zoom_amt);
			}
			else
			{
				F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
				F32 zoom_amt = (F32)(y - mLastMouseY) * 0.02f;
				
				mAvatarPreview->rotate(yaw_radians, 0.f);
				mAvatarPreview->zoom(zoom_amt);
				mSculptedPreview->rotate(yaw_radians, 0.f);
				mSculptedPreview->zoom(zoom_amt);
			}
		}

		LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
		if (iface && iface->getFirstSelectedIndex() <= 0)
		{
			if (mPreviewImageRect.getWidth() > 1.f)
			{
				mPreviewImageRect.stretch((1.f - mPreviewImageRect.getWidth()) * 0.5f);
			}
			else if (mPreviewImageRect.getWidth() < 0.1f)
			{
				mPreviewImageRect.stretch((0.1f - mPreviewImageRect.getWidth()) * 0.5f);
			}

			if (mPreviewImageRect.getHeight() > 1.f)
			{
				mPreviewImageRect.stretch((1.f - mPreviewImageRect.getHeight()) * 0.5f);
			}
			else if (mPreviewImageRect.getHeight() < 0.1f)
			{
				mPreviewImageRect.stretch((0.1f - mPreviewImageRect.getHeight()) * 0.5f);
			}

			if (mPreviewImageRect.mLeft < 0.f)
			{
				mPreviewImageRect.translate(-mPreviewImageRect.mLeft, 0.f);
			}
			else if (mPreviewImageRect.mRight > 1.f)
			{
				mPreviewImageRect.translate(1.f - mPreviewImageRect.mRight, 0.f);
			}

			if (mPreviewImageRect.mBottom < 0.f)
			{
				mPreviewImageRect.translate(0.f, -mPreviewImageRect.mBottom);
			}
			else if (mPreviewImageRect.mTop > 1.f)
			{
				mPreviewImageRect.translate(0.f, 1.f - mPreviewImageRect.mTop);
			}
		}
		else
		{
			mAvatarPreview->refresh();
			mSculptedPreview->refresh();
		}

		LLUI::setCursorPositionLocal(this, mLastMouseX, mLastMouseY);
	}

	if (!mPreviewRect.pointInRect(x, y) || !mAvatarPreview || !mSculptedPreview)
	{
		return LLFloater::handleHover(x, y, mask);
	}
	else if (local_mask == MASK_ORBIT)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (local_mask == MASK_PAN)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mPreviewRect.pointInRect(x, y) && mAvatarPreview)
	{
		mAvatarPreview->zoom((F32)clicks * -0.2f);
		mAvatarPreview->refresh();

		mSculptedPreview->zoom((F32)clicks * -0.2f);
		mSculptedPreview->refresh();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// onMouseCaptureLost()
//-----------------------------------------------------------------------------
void LLFloaterImagePreview::onMouseCaptureLost(LLMouseHandler* handler)
{
	gViewerWindow->showCursor();
}


//-----------------------------------------------------------------------------
// LLImagePreviewAvatar
//-----------------------------------------------------------------------------
LLImagePreviewAvatar::LLImagePreviewAvatar(S32 width, S32 height) : LLDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
{
	mNeedsUpdate = TRUE;
	mTargetJoint = NULL;
	mTargetMesh = NULL;
	mCameraDistance = 0.f;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;

	mDummyAvatar = new LLVOAvatar(LLUUID::null, LL_PCODE_LEGACY_AVATAR, gAgent.getRegion());
	mDummyAvatar->createDrawable(&gPipeline);
	mDummyAvatar->mIsDummy = TRUE;
	mDummyAvatar->mSpecialRenderMode = 2;
	mDummyAvatar->setPositionAgent(LLVector3::zero);
	mDummyAvatar->slamPosition();
	mDummyAvatar->updateJointLODs();
	mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	// gPipeline.markVisible(mDummyAvatar->mDrawable, *LLViewerCamera::getInstance());

	mTextureName = 0;
}


LLImagePreviewAvatar::~LLImagePreviewAvatar()
{
	mDummyAvatar->markDead();
}


void LLImagePreviewAvatar::setPreviewTarget(const char* joint_name, const char* mesh_name, LLImageRaw* imagep, F32 distance, BOOL male) 
{ 
	mTargetJoint = mDummyAvatar->mRoot.findJoint(joint_name);
	// clear out existing test mesh
	if (mTargetMesh)
	{
		mTargetMesh->setTestTexture(0);
	}

	if (male)
	{
		mDummyAvatar->setVisualParamWeight( "male", 1.f );
		mDummyAvatar->updateVisualParams();
		mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	}
	else
	{
		mDummyAvatar->setVisualParamWeight( "male", 0.f );
		mDummyAvatar->updateVisualParams();
		mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	}
	mDummyAvatar->mRoot.setVisible(FALSE, TRUE);

	mTargetMesh = (LLViewerJointMesh*)mDummyAvatar->mRoot.findJoint(mesh_name);
	mTargetMesh->setTestTexture(mTextureName);
	mTargetMesh->setVisible(TRUE, FALSE);
	mCameraDistance = distance;
	mCameraZoom = 1.f;
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraOffset.clearVec();
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
BOOL LLImagePreviewAvatar::render()
{
	mNeedsUpdate = FALSE;
	LLVOAvatar* avatarp = mDummyAvatar;

	glMatrixMode(GL_PROJECTION);
	gGL.pushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	glLoadIdentity();

	LLGLSUIDefault def;
	gGL.color4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mWidth, mHeight );

	glMatrixMode(GL_PROJECTION);
	gGL.popMatrix();

	glMatrixMode(GL_MODELVIEW);
	gGL.popMatrix();

	gGL.stop();
	LLVector3 target_pos = mTargetJoint->getWorldPosition();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = avatarp->mPelvisp->getWorldRotation() * camera_rot;
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	stop_glerror();

	LLViewerCamera::getInstance()->setAspect((F32)mWidth / mHeight);
	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);
	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mWidth, mHeight, FALSE);

	LLVertexBuffer::stopRender();
	avatarp->updateLOD();
	LLVertexBuffer::startRender();

	if (avatarp->mDrawable.notNull())
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
		// make sure alpha=0 shows avatar material color
		LLGLDisable no_blend(GL_BLEND);

		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)avatarp->mDrawable->getFace(0)->getPool();
		
		avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
	}

	gGL.start();

	return TRUE;
}

//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::refresh()
{ 
	mNeedsUpdate = TRUE; 
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::zoom(F32 zoom_amt)
{
	mCameraZoom	= llclamp(mCameraZoom + zoom_amt, 1.f, 10.f);
}

void LLImagePreviewAvatar::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}


//-----------------------------------------------------------------------------
// LLImagePreviewSculpted
//-----------------------------------------------------------------------------

LLImagePreviewSculpted::LLImagePreviewSculpted(S32 width, S32 height) : LLDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
{
	mNeedsUpdate = TRUE;
	mCameraDistance = 0.f;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;
	mTextureName = 0;

	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_CIRCLE);
	volume_params.setSculptID(LLUUID::null, LL_SCULPT_TYPE_SPHERE);
	mVolume = new LLVolume(volume_params, (F32) MAX_LOD);

	/*
	mDummyAvatar = new LLVOAvatar(LLUUID::null, LL_PCODE_LEGACY_AVATAR, gAgent.getRegion());
	mDummyAvatar->createDrawable(&gPipeline);
	mDummyAvatar->mIsDummy = TRUE;
	mDummyAvatar->mSpecialRenderMode = 2;
	mDummyAvatar->setPositionAgent(LLVector3::zero);
	mDummyAvatar->slamPosition();
	mDummyAvatar->updateJointLODs();
	mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	gPipeline.markVisible(mDummyAvatar->mDrawable, *LLViewerCamera::getInstance());
	mTextureName = 0;
	*/
}


LLImagePreviewSculpted::~LLImagePreviewSculpted()
{
	/*
	mDummyAvatar->markDead();
	*/
}


void LLImagePreviewSculpted::setPreviewTarget(LLImageRaw* imagep, F32 distance)
{ 
	mCameraDistance = distance;
	mCameraZoom = 1.f;
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraOffset.clearVec();

	if (imagep)
	{
		mVolume->sculpt(imagep->getWidth(), imagep->getHeight(), imagep->getComponents(), imagep->getData(), 0);
	}
	
}


//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLImagePreviewSculpted::render()
{
	mNeedsUpdate = FALSE;

	LLGLSUIDefault def;
	LLGLDisable no_blend(GL_BLEND);
	LLGLEnable cull(GL_CULL_FACE);
	LLGLDepthTest depth(GL_TRUE);

	glMatrixMode(GL_PROJECTION);
	gGL.pushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	glLoadIdentity();
		
	gGL.color4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mWidth, mHeight );

	glMatrixMode(GL_PROJECTION);
	gGL.popMatrix();

	glMatrixMode(GL_MODELVIEW);
	gGL.popMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	LLVector3 target_pos(0, 0, 0);

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = camera_rot;
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	stop_glerror();

	LLViewerCamera::getInstance()->setAspect((F32) mWidth / mHeight);
	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);
	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mWidth, mHeight, FALSE);

	gPipeline.enableLightsAvatar();
		
	gGL.pushMatrix();
	glScalef(0.5, 0.5, 0.5);
	
	const LLVolumeFace &vf = mVolume->getVolumeFace(0);
	U32 num_indices = vf.mIndices.size();
	U32 num_vertices = vf.mVertices.size();

	if (num_vertices > 0 && num_indices > 0)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		// build vertices and normals
		F32* vertices = new F32[num_vertices * 3];
		F32* normals = new F32[num_vertices * 3];

		for (U32 i = 0; (S32)i < num_vertices; i++)
		{
			LLVector3 position = vf.mVertices[i].mPosition;
			vertices[i*3]   = position.mV[VX];
			vertices[i*3+1] = position.mV[VY];
			vertices[i*3+2] = position.mV[VZ];
			
			LLVector3 normal = vf.mVertices[i].mNormal;
			normals[i*3]   = normal.mV[VX];
			normals[i*3+1] = normal.mV[VY];
			normals[i*3+2] = normal.mV[VZ];
		}

		// build indices
		U16* indices = new U16[num_indices];
		for (U16 i = 0; i < num_indices; i++)
		{
			indices[i] = vf.mIndices[i];
		}

		gGL.color3f(0.4f, 0.4f, 0.4f);
		glVertexPointer(3, GL_FLOAT, 0, (void *)vertices);
		glNormalPointer(GL_FLOAT, 0, (void *)normals);
		glDrawRangeElements(GL_TRIANGLES, 0, num_indices-1, num_indices, GL_UNSIGNED_SHORT, (void *)indices);
		
		gGL.popMatrix();
		glDisableClientState(GL_NORMAL_ARRAY);

		delete [] indices;
		delete [] vertices;
		delete [] normals;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLImagePreviewSculpted::refresh()
{ 
	mNeedsUpdate = TRUE; 
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLImagePreviewSculpted::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLImagePreviewSculpted::zoom(F32 zoom_amt)
{
	mCameraZoom	= llclamp(mCameraZoom + zoom_amt, 1.f, 10.f);
}

void LLImagePreviewSculpted::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}
