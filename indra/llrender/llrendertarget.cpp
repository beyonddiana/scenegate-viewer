/** 
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "llrendertarget.h"
#include "llglimmediate.h"


BOOL LLRenderTarget::sUseFBO = FALSE;

LLRenderTarget::LLRenderTarget()
{
	mResX = mResY = mTex = mFBO = mDepth = 0;
	mUseDepth = FALSE;
	mUsage = GL_TEXTURE_2D;
}

LLRenderTarget::~LLRenderTarget()
{
	release();
}

void LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, U32 usage, BOOL force_fbo)
{
	mResX = resx;
	mResY = resy;

	mUsage = usage;
	mUseDepth = depth;
	release();

	glGenTextures(1, (GLuint *) &mTex);
	glBindTexture(mUsage, mTex);
	glTexImage2D(mUsage, 0, color_fmt, mResX, mResY, 0, color_fmt, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(mUsage, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(mUsage, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (mUsage != GL_TEXTURE_RECTANGLE_ARB)
	{
		glTexParameteri(mUsage, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(mUsage, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	}
	else
	{
		// ATI doesn't support mirrored repeat for rectangular textures.
		glTexParameteri(mUsage, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(mUsage, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	stop_glerror();

	if (sUseFBO || force_fbo)
	{
		if (depth)
		{
			glGenRenderbuffersEXT(1, (GLuint *) &mDepth);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepth);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,mResX,mResY);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);	
		}

		glGenFramebuffersEXT(1, (GLuint *) &mFBO);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);

		if (mDepth)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
											GL_RENDERBUFFER_EXT, mDepth);	
		}
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
						mUsage, mTex, 0);


		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void LLRenderTarget::release()
{
	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex)
	{
		glDeleteTextures(1, (GLuint *) &mTex);
		mTex = 0;
	}

	if (mDepth)
	{
		glDeleteRenderbuffersEXT(1, (GLuint *) &mDepth);
		mDepth = 0;
	}
}

void LLRenderTarget::bindTarget()
{
	if (mFBO)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
	}

	glViewport(0, 0, mResX, mResY);
}

void LLRenderTarget::clear()
{
	U32 mask = GL_COLOR_BUFFER_BIT;
	if (mUseDepth)
	{
		mask |= GL_DEPTH_BUFFER_BIT;
	}
	if (mFBO)
	{
		glClear(mask);
	}
	else
	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, mResX, mResY);
		glClear(mask);
	}
}

void LLRenderTarget::bindTexture()
{
	glBindTexture(mUsage, mTex);
}

void LLRenderTarget::flush()
{
	gGL.flush();
	if (!mFBO)
	{
		bindTexture();
		glCopyTexSubImage2D(mUsage, 0, 0, 0, 0, 0, mResX, mResY);
	}
}

BOOL LLRenderTarget::isComplete() const
{
	return (mTex || mDepth) ? TRUE : FALSE;
}

void LLRenderTarget::getViewport(S32* viewport)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = mResX;
	viewport[3] = mResY;
}

