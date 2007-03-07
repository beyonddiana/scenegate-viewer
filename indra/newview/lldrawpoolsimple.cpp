/** 
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolsimple.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"

class LLRenderPassGlow : public LLRenderPass
{
public:
	LLRenderPassGlow(): LLRenderPass(LLRenderPass::PASS_GLOW) { }
	
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	virtual void prerender() { }

	void render(S32 pass = 0)
	{
		LLGLEnable blend(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		renderTexture(LLRenderPass::PASS_GLOW, getVertexDataMask());
		renderActive(LLRenderPass::PASS_GLOW, getVertexDataMask());
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture = TRUE)
	{
		glColor4ubv(params.mGlowColor.mV);
		LLRenderPass::pushBatch(params, mask, texture);
	}
};

LLDrawPoolSimple::LLDrawPoolSimple() :
	LLRenderPass(POOL_SIMPLE)
{
}

void LLDrawPoolSimple::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT);
}

void LLDrawPoolSimple::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void LLDrawPoolSimple::render(S32 pass)
{
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);
	
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
		gPipeline.enableLightsDynamic(1.f);
		renderTexture(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
		renderActive(LLRenderPass::PASS_SIMPLE, getVertexDataMask());
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_GRASS);
		LLGLEnable blend(GL_BLEND);
		LLGLEnable alpha_test(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//render grass
		LLRenderPass::renderTexture(LLRenderPass::PASS_GRASS, getVertexDataMask());
		glAlphaFunc(GL_GREATER, 0.01f);
	}
		
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_FULLBRIGHT);
		U32 fullbright_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD | LLVertexBuffer::MAP_COLOR;
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		glDisableClientState(GL_NORMAL_ARRAY);
		renderTexture(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);
		renderActive(LLRenderPass::PASS_FULLBRIGHT, fullbright_mask);
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_GLOW);
		glDisableClientState(GL_COLOR_ARRAY);
		LLRenderPassGlow glow;
		glow.render();
	}

	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_INVISIBLE);
		U32 invisi_mask = LLVertexBuffer::MAP_VERTEX;
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		renderInvisible(invisi_mask);
		renderActive(LLRenderPass::PASS_INVISIBLE, invisi_mask);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
}

