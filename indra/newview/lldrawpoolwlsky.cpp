/** 
 * @file lldrawpoolwlsky.cpp
 * @brief LLDrawPoolWLSky class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolwlsky.h"

#include "llerror.h"
#include "llgl.h"
#include "pipeline.h"
#include "llviewercamera.h"
#include "llimage.h"
#include "llwlparammanager.h"
#include "llviewershadermgr.h"
#include "llglslshader.h"
#include "llsky.h"
#include "llvowlsky.h"
#include "llviewerregion.h"
#include "llface.h"
#include "llrender.h"

LLPointer<LLViewerTexture> LLDrawPoolWLSky::sCloudNoiseTexture = NULL;

LLPointer<LLImageRaw> LLDrawPoolWLSky::sCloudNoiseRawImage = NULL;

static LLGLSLShader* cloud_shader = NULL;
static LLGLSLShader* sky_shader = NULL;
static LLGLSLShader* star_shader = NULL;

LLDrawPoolWLSky::LLDrawPoolWLSky(void) :
	LLDrawPool(POOL_WL_SKY)
{
	const std::string cloudNoiseFilename(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", "clouds2.tga"));
	LL_INFOS() << "loading WindLight cloud noise from " << cloudNoiseFilename << LL_ENDL;

	LLPointer<LLImageFormatted> cloudNoiseFile(LLImageFormatted::createFromExtension(cloudNoiseFilename));

	if(cloudNoiseFile.isNull()) {
		LL_ERRS() << "Error: Failed to load cloud noise image " << cloudNoiseFilename << LL_ENDL;
	}

	if(cloudNoiseFile->load(cloudNoiseFilename))
	{
		sCloudNoiseRawImage = new LLImageRaw();

		if(cloudNoiseFile->decode(sCloudNoiseRawImage, 0.0f))
		{
			//debug use			
			LL_DEBUGS() << "cloud noise raw image width: " << sCloudNoiseRawImage->getWidth() << " : height: " << sCloudNoiseRawImage->getHeight() << " : components: " << 
				(S32)sCloudNoiseRawImage->getComponents() << " : data size: " << sCloudNoiseRawImage->getDataSize() << LL_ENDL ;
			llassert_always(sCloudNoiseRawImage->getData()) ;

			sCloudNoiseTexture = LLViewerTextureManager::getLocalTexture(sCloudNoiseRawImage.get(), TRUE);
		}
		else
		{
			sCloudNoiseRawImage = NULL ;
		}
	}

	LLWLParamManager::getInstance()->propagateParameters();
}

LLDrawPoolWLSky::~LLDrawPoolWLSky()
{
	//LL_INFOS() << "destructing wlsky draw pool." << LL_ENDL;
	sCloudNoiseTexture = NULL;
	sCloudNoiseRawImage = NULL;
}

LLViewerTexture *LLDrawPoolWLSky::getDebugTexture()
{
	return NULL;
}

void LLDrawPoolWLSky::beginRenderPass( S32 pass )
{
	sky_shader =
		LLPipeline::sUnderWaterRender ?
			&gObjectFullbrightNoColorWaterProgram :
			&gWLSkyProgram;

	cloud_shader =
			LLPipeline::sUnderWaterRender ?
				&gObjectFullbrightNoColorWaterProgram :
				&gWLCloudProgram;

	star_shader = &gCustomAlphaProgram;
}

void LLDrawPoolWLSky::endRenderPass( S32 pass )
{
}

void LLDrawPoolWLSky::beginDeferredPass(S32 pass)
{
	sky_shader = &gDeferredWLSkyProgram;
	cloud_shader = &gDeferredWLCloudProgram;
	star_shader = &gDeferredStarProgram;
}

void LLDrawPoolWLSky::endDeferredPass(S32 pass)
{

}

void LLDrawPoolWLSky::renderDome(F32 camHeightLocal, LLGLSLShader * shader) const
{
	LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

	llassert_always(NULL != shader);

	gGL.pushMatrix();

	//chop off translation
	if (LLPipeline::sReflectionRender && origin.mV[2] > 256.f)
	{
		gGL.translatef(origin.mV[0], origin.mV[1], 256.f-origin.mV[2]*0.5f);
	}
	else
	{
		gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);
	}
		

	// the windlight sky dome works most conveniently in a coordinate system
	// where Y is up, so permute our basis vectors accordingly.
	gGL.rotatef(120.f, 1.f / F_SQRT3, 1.f / F_SQRT3, 1.f / F_SQRT3);

	gGL.scalef(0.333f, 0.333f, 0.333f);

	gGL.translatef(0.f,-camHeightLocal, 0.f);
	
	// Draw WL Sky	
	static LLStaticHashedString sCamPosLocal("camPosLocal");
	shader->uniform3f(sCamPosLocal, 0.f, camHeightLocal, 0.f);

	gSky.mVOWLSkyp->drawDome();

	gGL.popMatrix();
}

void LLDrawPoolWLSky::renderSkyHaze(F32 camHeightLocal) const
{
	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		LLGLDisable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);

		sky_shader->bind();

		/// Render the skydome
		renderDome(camHeightLocal, sky_shader);	

		sky_shader->unbind();
	}
}

void LLDrawPoolWLSky::renderStars(void) const
{
	// *NOTE: we divide by two here and GL_ALPHA_SCALE by two below to avoid
	// clamping and allow the star_alpha param to brighten the stars.
	bool error;
	LLColor4 star_alpha(LLColor4::black);
	star_alpha.mV[3] = LLWLParamManager::getInstance()->mCurParams.getFloat("star_brightness", error) / 2.f;
	llassert_always(!error);
	if(star_alpha.mV[3] <= 0)
		return;

	LLGLSPipelineSkyBox gls_sky;
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	
	// *NOTE: have to have bound the cloud noise texture already since register
	// combiners blending below requires something to be bound
	// and we might as well only bind once.
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	
	gPipeline.disableLights();
	

	gGL.pushMatrix();
	gGL.rotatef(gFrameTimeSeconds*0.01f, 0.f, 0.f, 1.f);
	gGL.getTexUnit(0)->bind(gSky.mVOSkyp->getBloomTex());

	if (gPipeline.canUseVertexShaders())
	{
		static LLStaticHashedString sCustomAlpha("custom_alpha");
		star_shader->uniform1f(sCustomAlpha, star_alpha.mV[3]);
	}
	else
	{
		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_VERT_COLOR);
		gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT_X2, LLTexUnit::TBS_CONST_ALPHA, LLTexUnit::TBS_TEX_ALPHA);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, star_alpha.mV);
	}

	gSky.mVOWLSkyp->drawStars();

	gGL.popMatrix();

	if (!gPipeline.canUseVertexShaders())
	{
		// and disable the combiner states
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
}

void LLDrawPoolWLSky::renderSkyClouds(F32 camHeightLocal) const
{
	if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS) && sCloudNoiseTexture.notNull())
	{
		LLGLEnable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		
		gGL.getTexUnit(0)->bind(sCloudNoiseTexture);

		cloud_shader->bind();

		/// Render the skydome
		renderDome(camHeightLocal, cloud_shader);

		cloud_shader->unbind();
	}
}

void LLDrawPoolWLSky::renderHeavenlyBodies()
{
	LLColor4 color(gSky.mVOSkyp->getMoon().getInterpColor(), gSky.mVOSkyp->getMoon().getDirection().mV[2]);
	if (color.mV[VW] <= 0.f)
		return;
	
	color.mV[VW] = llclamp(color.mV[VW]*color.mV[VW]*4.f,0.f,1.f);

	LLGLSPipelineSkyBox gls_skybox;
	LLGLEnable blend_on(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	gPipeline.disableLights();

#if 0 // when we want to re-add a texture sun disc, here's where to do it.
	LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_SUN];
	if (gSky.mVOSkyp->getSun().getDraw() && face->getGeomCount())
	{
		LLViewerTexture * tex  = face->getTexture();
		gGL.getTexUnit(0)->bind(tex);
		LLColor4 color(gSky.mVOSkyp->getSun().getInterpColor());
		LLFacePool::LLOverrideFaceColor color_override(this, color);
		face->renderIndexed();
	}
#endif

	LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON];

	if (gSky.mVOSkyp->getMoon().getDraw() && face->getGeomCount() && face->getVertexBuffer())
	{
		// *NOTE: even though we already bound this texture above for the
		// stars register combiners, we bind again here for defensive reasons,
		// since LLImageGL::bind detects that it's a noop, and optimizes it out.
		gGL.getTexUnit(0)->bind(face->getTexture());
		
		if (gPipeline.canUseVertexShaders())
		{
			// Okay, so the moon isn't a star, but it's close enough.
			static LLStaticHashedString sCustomAlpha("custom_alpha");
			star_shader->uniform1f(sCustomAlpha, color.mV[VW]);
		}
		else
		{
			gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_VERT_COLOR);
			gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT_X2, LLTexUnit::TBS_CONST_ALPHA, LLTexUnit::TBS_TEX_ALPHA);
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color.mV);
		}

		face->getVertexBuffer()->setBuffer(LLDrawPoolWLSky::STAR_VERTEX_DATA_MASK);
		face->getVertexBuffer()->draw(LLRender::TRIANGLES, face->getVertexBuffer()->getNumIndices(), 0);
		
		if (!gPipeline.canUseVertexShaders())
		{
			gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
		}
	}
}

void LLDrawPoolWLSky::renderDeferred(S32 pass)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	LL_RECORD_BLOCK_TIME(FTM_RENDER_WL_SKY);

	const F32 camHeightLocal = LLWLParamManager::getInstance()->getDomeOffset() * LLWLParamManager::getInstance()->getDomeRadius();

	LLGLSNoFog disableFog;
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	LLGLDisable clip(GL_CLIP_PLANE0);

	gGL.setColorMask(true, false);

	LLGLSquashToFarClip far_clip(glh_get_current_projection());

	renderSkyHaze(camHeightLocal);

	LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();
	gGL.pushMatrix();

		
		gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

		star_shader->bind();
		// *NOTE: have to bind a texture here since register combiners blending in
		// renderStars() requires something to be bound and we might as well only
		// bind the moon's texture once.		
		gGL.getTexUnit(0)->bind(gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON]->getTexture());

		renderHeavenlyBodies();

		renderStars();
		
		star_shader->unbind();

	gGL.popMatrix();

	renderSkyClouds(camHeightLocal);

	gGL.setColorMask(true, true);
	//gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

}

void LLDrawPoolWLSky::render(S32 pass)
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	LL_RECORD_BLOCK_TIME(FTM_RENDER_WL_SKY);

	const F32 camHeightLocal = LLWLParamManager::getInstance()->getDomeOffset() * LLWLParamManager::getInstance()->getDomeRadius();

	LLGLSNoFog disableFog;
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	LLGLDisable clip(GL_CLIP_PLANE0);

	LLGLSquashToFarClip far_clip(glh_get_current_projection());

	renderSkyHaze(camHeightLocal);

	LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();
	gGL.pushMatrix();

		gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

		if(gPipeline.canUseVertexShaders())
			star_shader->bind();
		// *NOTE: have to bind a texture here since register combiners blending in
		// renderStars() requires something to be bound and we might as well only
		// bind the moon's texture once.		
		gGL.getTexUnit(0)->bind(gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON]->getTexture());

		renderHeavenlyBodies();

		renderStars();
		
		if(gPipeline.canUseVertexShaders())
			star_shader->unbind(); 

	gGL.popMatrix();

	renderSkyClouds(camHeightLocal);

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void LLDrawPoolWLSky::prerender()
{
	//LL_INFOS() << "wlsky prerendering pass." << LL_ENDL;
}

LLDrawPoolWLSky *LLDrawPoolWLSky::instancePool()
{
	return new LLDrawPoolWLSky();
}

LLViewerTexture* LLDrawPoolWLSky::getTexture()
{
	return NULL;
}

void LLDrawPoolWLSky::resetDrawOrders()
{
}

//static
void LLDrawPoolWLSky::cleanupGL()
{
	sCloudNoiseTexture = NULL;
}

//static
void LLDrawPoolWLSky::restoreGL()
{
	if(sCloudNoiseRawImage.notNull())
	{
		sCloudNoiseTexture = LLViewerTextureManager::getLocalTexture(sCloudNoiseRawImage.get(), TRUE);
	}
}

