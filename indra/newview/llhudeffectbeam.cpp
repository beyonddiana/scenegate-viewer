/** 
 * @file llhudeffectbeam.cpp
 * @brief LLHUDEffectBeam class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudeffectbeam.h"
#include "message.h"

#include "llviewerobjectlist.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llhudrender.h"
#include "llimagegl.h"
#include "llsphere.h"
#include "llviewercamera.h"
#include "llvoavatar.h"
#include "llviewercontrol.h"

const F32 BEAM_SPACING = 0.075f;

LLHUDEffectBeam::LLHUDEffectBeam(const U8 type) : LLHUDEffect(type)
{
	mKillTime = mDuration;

	// Initialize all of these to defaults
	S32 i;
	for (i = 0; i < NUM_POINTS; i++)
	{
		mInterp[i].setStartTime(BEAM_SPACING*i);
		mInterp[i].setEndTime(BEAM_SPACING*NUM_POINTS + BEAM_SPACING*i);
		mInterp[i].start();
		mInterpFade[i].setStartTime(BEAM_SPACING*NUM_POINTS + BEAM_SPACING*i - 0.5f*NUM_POINTS*BEAM_SPACING);
		mInterpFade[i].setEndTime(BEAM_SPACING*NUM_POINTS + BEAM_SPACING*i);
		mInterpFade[i].setStartVal(1.f);
		mInterpFade[i].setEndVal(0.f);
	}

	// Setup default timeouts and fade animations.
	F32 fade_length;
	fade_length = llmin(0.5f, mDuration);
	mFadeInterp.setStartTime(mKillTime - fade_length);
	mFadeInterp.setEndTime(mKillTime);
	mFadeInterp.setStartVal(1.f);
	mFadeInterp.setEndVal(0.f);
}

LLHUDEffectBeam::~LLHUDEffectBeam()
{
}

void LLHUDEffectBeam::packData(LLMessageSystem *mesgsys)
{
	if (!mSourceObject)
	{
		llwarns << "Missing source object!" << llendl;
	}

	// Pack the default data
	LLHUDEffect::packData(mesgsys);

	// Pack the type-specific data.  Uses a fun packed binary format.  Whee!
	// 16 + 24 + 1 = 41
	U8 packed_data[41];
	memset(packed_data, 0, 41);
	if (mSourceObject)
	{
		htonmemcpy(packed_data, mSourceObject->mID.mData, MVT_LLUUID, 16);
	}

	if (mTargetObject)
	{
		packed_data[16] = 1;
	}
	else
	{
		packed_data[16] = 0;
	}

	if (mTargetObject)
	{
		htonmemcpy(&(packed_data[17]), mTargetObject->mID.mData, MVT_LLUUID, 16);
	}
	else
	{
		htonmemcpy(&(packed_data[17]), mTargetPos.mdV, MVT_LLVector3d, 24);
	}
	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, 41);
}

void LLHUDEffectBeam::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	llerrs << "Got beam!" << llendl;
	BOOL use_target_object;
	LLVector3d new_target;
	U8 packed_data[41];

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID source_id;
	LLUUID target_id;
	S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != 41)
	{
		llwarns << "Beam effect with bad size " << size << llendl;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, packed_data, 41, blocknum);
	
	htonmemcpy(source_id.mData, packed_data, MVT_LLUUID, 16);

	LLViewerObject *objp = gObjectList.findObject(source_id);
	if (objp)
	{
		setSourceObject(objp);
	}

	use_target_object = packed_data[16];

	if (use_target_object)
	{
		htonmemcpy(target_id.mData, &packed_data[17], MVT_LLUUID, 16);

		LLViewerObject *objp = gObjectList.findObject(target_id);
		if (objp)
		{
			setTargetObject(objp);
		}
	}
	else
	{
		htonmemcpy(new_target.mdV, &(packed_data[17]), MVT_LLVector3d, 24);
		setTargetPos(new_target);
	}

	// We've received an update for the effect, update the various timeouts
	// and fade animations.
	mKillTime = mTimer.getElapsedTimeF32() + mDuration;
	F32 fade_length;
	fade_length = llmin(0.5f, mDuration);
	mFadeInterp.setStartTime(mKillTime - fade_length);
	mFadeInterp.setEndTime(mKillTime);
	mFadeInterp.setStartVal(1.f);
	mFadeInterp.setEndVal(0.f);
}

void LLHUDEffectBeam::setSourceObject(LLViewerObject *objp)
{
	if (objp->isDead())
	{
		llwarns << "HUDEffectBeam: Source object is dead!" << llendl;
		mSourceObject = NULL;
		return;
	}

	if (mSourceObject == objp)
	{
		return;
	}

	mSourceObject = objp;
	if (mSourceObject)
	{
		S32 i;
		for (i = 0; i < NUM_POINTS; i++)
		{
			if (mSourceObject->isAvatar())
			{
				LLViewerObject *objp = mSourceObject;
				LLVOAvatar *avatarp = (LLVOAvatar *)objp;
				LLVector3d hand_pos_global = gAgent.getPosGlobalFromAgent(avatarp->mWristLeftp->getWorldPosition());
				mInterp[i].setStartVal(hand_pos_global);
				mInterp[i].start();
			}
			else
			{
				mInterp[i].setStartVal(mSourceObject->getPositionGlobal());
				mInterp[i].start();
			}
		}
	}
}


void LLHUDEffectBeam::setTargetObject(LLViewerObject *objp)
{
	if (mTargetObject->isDead())
	{
		llwarns << "HUDEffectBeam: Target object is dead!" << llendl;
	}

	mTargetObject = objp;
}

void LLHUDEffectBeam::setTargetPos(const LLVector3d &pos_global)
{
	mTargetPos = pos_global;
	mTargetObject = NULL;
}

void LLHUDEffectBeam::render()
{
	if (!mSourceObject)
	{
		markDead();
		return;
	}
	if (mSourceObject->isDead())
	{
		markDead();
		return;
	}

	F32 time = mTimer.getElapsedTimeF32();

	// Kill us if our time is over...
	if (mKillTime < time)
	{
		markDead();
		return;
	}

	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);


	// Interpolate the global fade alpha
	mFadeInterp.update(time);

	if (mTargetObject.notNull() && mTargetObject->mDrawable.notNull())
	{
		// use viewer object position on freshly created objects
		if (mTargetObject->mDrawable->getGeneration() == -1)
		{
			mTargetPos = mTargetObject->getPositionGlobal();
		}
		// otherwise use drawable
		else
		{
			mTargetPos = gAgent.getPosGlobalFromAgent(mTargetObject->mDrawable->getPositionAgent());
		}
	}


	// Init the color of the particles
	LLColor4U coloru = mColor;

	/*
	// This is disabled for now - DJS

	// Fade the alpha
	coloru.mV[3] = mFadeInterp.getCurVal()*mColor.mV[3];

	// Draw a regular "beam" that connects the source and target

	// First, figure out start and end positions relative to the camera
	LLVector3 start_pos_agent;
	if (mSourceObject->getPCode() == LL_PCODE_LEGACY_AVATAR)
	{
		LLViewerObject *objp = mSourceObject;
		LLVOAvatar *avatarp = (LLVOAvatar *)objp;
		LLVector3d hand_pos_global = gAgent.getPosGlobalFromAgent(avatarp->mWristLeftp->getWorldPosition());
		start_pos_agent = gAgent.getPosAgentFromGlobal(hand_pos_global);
	}
	else
	{
		start_pos_agent = mSourceObject->getPositionAgent();
	}
	LLVector3 start_pos_camera = (start_pos_agent - gAgent.getCameraPositionAgent());
	LLVector3 target_pos_agent = gAgent.getPosAgentFromGlobal(mTargetPos);
	LLVector3 target_pos_camera = target_pos_agent - gAgent.getCameraPositionAgent();

	// Generate the right "up" vector which is perpendicular to the beam, make it 1/10 meter wide, going to a point.
	LLVector3 camera_up = gCamera->getUpAxis();
	LLVector3 camera_at = gCamera->getAtAxis();
	LLVector3 up = target_pos_camera % start_pos_camera;
	up.normVec();
	up *= 0.1f;

	// Draw the triangle for the beam.
	LLVector3 vertex;
	glColor4ubv(coloru.mV);
	glBegin(GL_TRIANGLE_STRIP);
	vertex = start_pos_agent + up;
	glVertex3fv(vertex.mV);
	vertex = start_pos_agent - up;
	glVertex3fv(vertex.mV);
	vertex = target_pos_agent;
	glVertex3fv(vertex.mV);
	glEnd();
	*/
	
	// Draw the particles
	S32 i;
	for (i = 0; i < NUM_POINTS; i++)
	{
		mInterp[i].update(time);
		if (!mInterp[i].isActive())
		{
			continue;
		}
		mInterpFade[i].update(time);

		if (mInterp[i].isDone())
		{
			// Reinitialize the particle when the particle has finished its animation.
			setupParticle(i);
		}

		F32 frac = mInterp[i].getCurFrac();
		F32 scale = 0.025f + fabs(0.05f*sin(2.f*F_PI*(frac - time)));
		scale *= mInterpFade[i].getCurVal();

		LLVector3 pos_agent = gAgent.getPosAgentFromGlobal(mInterp[i].getCurVal());

		F32 alpha = mFadeInterp.getCurVal()*mColor.mV[3];
		alpha *= mInterpFade[i].getCurVal();
		coloru.mV[3] = (U8)alpha;
		glColor4ubv(coloru.mV);

		glPushMatrix();
		glTranslatef(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]);
		glScalef(scale, scale, scale);
		gSphere.render(0);
		glPopMatrix();
	}
}

void LLHUDEffectBeam::setupParticle(const S32 i)
{
	LLVector3d start_pos_global;
	if (mSourceObject->getPCode() == LL_PCODE_LEGACY_AVATAR)
	{
		LLViewerObject *objp = mSourceObject;
		LLVOAvatar *avatarp = (LLVOAvatar *)objp;
		start_pos_global = gAgent.getPosGlobalFromAgent(avatarp->mWristLeftp->getWorldPosition());
	}
	else
	{
		start_pos_global = mSourceObject->getPositionGlobal();
	}

	// Generate a random offset for the target point.
	const F32 SCALE = 0.5f;
	F32 x, y, z;
	x = ll_frand(SCALE) - 0.5f*SCALE;
	y = ll_frand(SCALE) - 0.5f*SCALE;
	z = ll_frand(SCALE) - 0.5f*SCALE;

	LLVector3d target_pos_global(mTargetPos);
	target_pos_global += LLVector3d(x, y, z);

	mInterp[i].setStartTime(mInterp[i].getEndTime());
	mInterp[i].setEndTime(mInterp[i].getStartTime() + BEAM_SPACING*NUM_POINTS);
	mInterp[i].setStartVal(start_pos_global);
	mInterp[i].setEndVal(target_pos_global);
	mInterp[i].start();


	// Setup the interpolator that fades out the alpha.
	mInterpFade[i].setStartTime(mInterp[i].getStartTime() + BEAM_SPACING*NUM_POINTS - 0.5f*NUM_POINTS*BEAM_SPACING);
	mInterpFade[i].setEndTime(mInterp[i].getStartTime() + BEAM_SPACING*NUM_POINTS - 0.05f);
	mInterpFade[i].start();
}
