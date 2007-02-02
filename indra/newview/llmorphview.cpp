/** 
 * @file llmorphview.cpp
 * @brief Container for Morph functionality
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llmorphview.h"

#include "lljoint.h"

#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "llfirstuse.h"
#include "llfloatercustomize.h"
#include "llfloatertools.h"
#include "llresmgr.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llvoavatar.h"
#include "llviewerwindow.h"
#include "pipeline.h"
#include "viewer.h"

LLMorphView *gMorphView = NULL;


const F32 EDIT_AVATAR_ORBIT_SPEED = 0.1f;
const F32 EDIT_AVATAR_MAX_CAMERA_PITCH = 0.5f;

const F32 CAMERA_MOVE_TIME = 0.5f;
const F32 MORPH_NEAR_CLIP = 0.1f;

const F32 CAMERA_DIST_MIN  = 0.4f;
const F32 CAMERA_DIST_MAX  = 4.0f;
const F32 CAMERA_DIST_STEP = 1.5f;

//-----------------------------------------------------------------------------
// LLMorphView()
//-----------------------------------------------------------------------------
LLMorphView::LLMorphView(const std::string& name, const LLRect& rect)
	: 
	LLView(name, rect, FALSE, FOLLOWS_ALL),
	mCameraTargetJoint( NULL ),
	mCameraOffset(-0.5f, 0.05f, 0.07f ),
	mCameraTargetOffset(0.f, 0.f, 0.05f ),
	mOldCameraNearClip( 0.f ),
	mCameraPitch( 0.f ),
	mCameraYaw( 0.f ),
	mCameraDist( -1.f ),
	mCameraDrivenByKeys( FALSE )
{
}

EWidgetType LLMorphView::getWidgetType() const
{
	return WIDGET_TYPE_MORPH_VIEW;
}

LLString LLMorphView::getWidgetTag() const
{
	return LL_MORPH_VIEW_TAG;
}

//-----------------------------------------------------------------------------
// initialize()
//-----------------------------------------------------------------------------
void	LLMorphView::initialize()
{
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraDist = -1.f;

	LLVOAvatar *avatarp = gAgent.getAvatarObject();
	if (!avatarp || avatarp->isDead())
	{
		gAgent.changeCameraToDefault();
		return;
	}

	avatarp->stopMotion( ANIM_AGENT_BODY_NOISE );
	avatarp->mSpecialRenderMode = 3;
	
	// set up camera for close look at avatar
	mOldCameraNearClip = gCamera->getNear();
	gCamera->setNear(MORPH_NEAR_CLIP);	
}

//-----------------------------------------------------------------------------
// shutdown()
//-----------------------------------------------------------------------------
void	LLMorphView::shutdown()
{
	LLVOAvatar::onCustomizeEnd();

	LLVOAvatar *avatarp = gAgent.getAvatarObject();
	if(avatarp && !avatarp->isDead())
	{
		avatarp->startMotion( ANIM_AGENT_BODY_NOISE );
		avatarp->mSpecialRenderMode = 0;
		// reset camera
		gCamera->setNear(mOldCameraNearClip);
	}
}


//-----------------------------------------------------------------------------
// setVisible()
//-----------------------------------------------------------------------------
void LLMorphView::setVisible(BOOL visible)
{
	if( visible != getVisible() )
	{
		LLView::setVisible(visible);

		if (visible)
		{
			llassert( !gFloaterCustomize );
			gFloaterCustomize = new LLFloaterCustomize();
			gFloaterCustomize->fetchInventory();
			gFloaterCustomize->open();	/*Flawfinder: ignore*/

			// Must do this _after_ gFloaterView is initialized.
			gFloaterCustomize->switchToDefaultSubpart();

			initialize();

			// First run dialog
			LLFirstUse::useAppearance();
		}
		else
		{
			if( gFloaterCustomize )
			{
				gFloaterView->removeChild( gFloaterCustomize );
				delete gFloaterCustomize;
				gFloaterCustomize = NULL;
			}

			shutdown();
		}
	}
}

void LLMorphView::updateCamera()
{
	if (!mCameraTargetJoint)
	{
		setCameraTargetJoint(gAgent.getAvatarObject()->getJoint("mHead"));
	}
	
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		return;
	}
	LLJoint* root_joint = avatar->getRootJoint();
	if( !root_joint )
	{
		return;
	}

	const LLQuaternion& avatar_rot = root_joint->getWorldRotation();

	LLVector3d joint_pos = gAgent.getPosGlobalFromAgent(mCameraTargetJoint->getWorldPosition());
	LLVector3d target_pos = joint_pos + mCameraTargetOffset * avatar_rot;

	LLQuaternion camera_rot_yaw(mCameraYaw, LLVector3::z_axis);
	LLQuaternion camera_rot_pitch(mCameraPitch, LLVector3::y_axis);

	LLVector3d camera_pos = joint_pos + mCameraOffset * camera_rot_pitch * camera_rot_yaw * avatar_rot;

	gAgent.setCameraPosAndFocusGlobal( camera_pos, target_pos, gAgent.getID() );
}

void LLMorphView::setCameraDrivenByKeys(BOOL b)
{
	if( mCameraDrivenByKeys != b )
	{
		if( b )
		{
			// Reset to the default camera position specified by mCameraPitch, mCameraYaw, etc.
			updateCamera();
		}
		mCameraDrivenByKeys = b;
	}
}
