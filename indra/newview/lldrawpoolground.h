/** 
 * @file lldrawpoolground.h
 * @brief LLDrawPoolGround class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLGROUND_H
#define LL_LLDRAWPOOLGROUND_H

#include "lldrawpool.h"


class LLDrawPoolGround : public LLFacePool
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolGround();

	/*virtual*/ LLDrawPool *instancePool();
	virtual S32 getMaterialAttribIndex() { return 0; }

	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderForSelect();
};

#endif // LL_LLDRAWPOOLGROUND_H
