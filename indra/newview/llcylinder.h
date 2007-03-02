/** 
 * @file llcylinder.h
 * @brief Draws a cylinder, and a cone, which is a special case cylinder
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCYLINDER_H
#define LL_LLCYLINDER_H

//#include "stdtypes.h"
//#include "llgl.h"

//
// Cylinders
//
const S32 CYLINDER_LEVELS_OF_DETAIL = 4;
const S32 CYLINDER_FACES = 3;

class LLCylinder
{
public:
	void prerender();
	void drawTop(S32 detail);
	void drawSide(S32 detail);
	void drawBottom(S32 detail);
	void cleanupGL();

	void render(F32 pixel_area);
	void renderface(F32 pixel_area, S32 face);
};


//
// Cones
//

const S32 CONE_LOD_HIGHEST = 0;
const S32 CONE_LEVELS_OF_DETAIL = 4;
const S32 CONE_FACES = 2;

class LLCone
{	
public:
	void prerender();
	void cleanupGL();
	void drawSide(S32 detail);
	void drawBottom(S32 detail);
	void render(S32 level_of_detail);
	void renderface(S32 level_of_detail, S32 face);
};

extern LLCylinder gCylinder;
extern LLCone gCone;
#endif
