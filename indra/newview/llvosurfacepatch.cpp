/** 
 * @file llvosurfacepatch.cpp
 * @brief Viewer-object derived "surface patch", which is a piece of terrain
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvosurfacepatch.h"

#include "lldrawpoolterrain.h"

#include "lldrawable.h"
#include "llface.h"
#include "llprimitive.h"
#include "llsky.h"
#include "llsurfacepatch.h"
#include "llsurface.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "llvovolume.h"
#include "pipeline.h"

//============================================================================

class LLVertexBufferTerrain : public LLVertexBuffer
{
public:
	LLVertexBufferTerrain() :
		LLVertexBuffer(MAP_VERTEX | MAP_NORMAL | MAP_TEXCOORD | MAP_TEXCOORD2 | MAP_COLOR, GL_DYNAMIC_DRAW_ARB)
	{
	};

	// virtual
	void setupVertexBuffer(U32 data_mask) const
	{		
		if (LLDrawPoolTerrain::getDetailMode() == 0)
		{
			LLVertexBuffer::setupVertexBuffer(data_mask);
		}
		else if (data_mask & LLVertexBuffer::MAP_TEXCOORD2)
		{
			U8* base = useVBOs() ? NULL : mMappedData;
	
			glVertexPointer(3,GL_FLOAT, mStride, (void*)(base + 0));
			glNormalPointer(GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_NORMAL]));
			glColorPointer(4, GL_UNSIGNED_BYTE, mStride, (void*)(base + mOffsets[TYPE_COLOR]));
		
			glClientActiveTextureARB(GL_TEXTURE3_ARB);
			glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
		}
		else
		{
			LLVertexBuffer::setupVertexBuffer(data_mask);
		}
		llglassertok();		
	}
};

//============================================================================

LLVOSurfacePatch::LLVOSurfacePatch(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	:	LLStaticViewerObject(id, LL_VO_SURFACE_PATCH, regionp),
		mDirtiedPatch(FALSE),
		mPool(NULL),
		mBaseComp(0),
		mPatchp(NULL),
		mDirtyTexture(FALSE),
		mDirtyTerrain(FALSE),
		mLastNorthStride(0),
		mLastEastStride(0),
		mLastStride(0),
		mLastLength(0)
{
	// Terrain must draw during selection passes so it can block objects behind it.
	mbCanSelect = TRUE;
	setScale(LLVector3(16.f, 16.f, 16.f)); // Hack for setting scale for bounding boxes/visibility.
}


LLVOSurfacePatch::~LLVOSurfacePatch()
{
	mPatchp = NULL;
}


void LLVOSurfacePatch::markDead()
{
	if (mPatchp)
	{
		mPatchp->clearVObj();
		mPatchp = NULL;
	}
	LLViewerObject::markDead();
}


BOOL LLVOSurfacePatch::isActive() const
{
	return FALSE;
}


void LLVOSurfacePatch::setPixelAreaAndAngle(LLAgent &agent)
{
	mAppAngle = 50;
	mPixelArea = 500*500;
}


void LLVOSurfacePatch::updateTextures(LLAgent &agent)
{
}


LLFacePool *LLVOSurfacePatch::getPool()
{
	mPool = (LLDrawPoolTerrain*) gPipeline.getPool(LLDrawPool::POOL_TERRAIN, mPatchp->getSurface()->getSTexture());

	return mPool;
}


LLDrawable *LLVOSurfacePatch::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);

	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_TERRAIN);
	
	mBaseComp = llfloor(mPatchp->getMinComposition());
	S32 min_comp, max_comp, range;
	min_comp = llfloor(mPatchp->getMinComposition());
	max_comp = llceil(mPatchp->getMaxComposition());
	range = (max_comp - min_comp);
	range++;
	if (range > 3)
	{
		if ((mPatchp->getMinComposition() - min_comp) > (max_comp - mPatchp->getMaxComposition()))
		{
			// The top side runs over more
			mBaseComp++;
		}
		range = 3;
	}

	LLFacePool *poolp = getPool();

	mDrawable->addFace(poolp, NULL);

	return mDrawable;
}


BOOL LLVOSurfacePatch::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_TERRAIN);

	S32 min_comp, max_comp, range;
	min_comp = lltrunc(mPatchp->getMinComposition());
	max_comp = lltrunc(ceil(mPatchp->getMaxComposition()));
	range = (max_comp - min_comp);
	range++;
	S32 new_base_comp = lltrunc(mPatchp->getMinComposition());
	if (range > 3)
	{
		if ((mPatchp->getMinComposition() - min_comp) > (max_comp - mPatchp->getMaxComposition()))
		{
			// The top side runs over more
			new_base_comp++;
		}
		range = 3;
	}

	// Pick the two closest detail textures for this patch...
	// Then create the draw pool for it.
	// Actually, should get the average composition instead of the center.
	mBaseComp = new_base_comp;

	//////////////////////////
	//
	// Figure out the strides
	//
	//

	U32 patch_width, render_stride, north_stride, east_stride, length;
	render_stride = mPatchp->getRenderStride();
	patch_width = mPatchp->getSurface()->getGridsPerPatchEdge();

	length = patch_width / render_stride;

	if (mPatchp->getNeighborPatch(NORTH))
	{
		north_stride = mPatchp->getNeighborPatch(NORTH)->getRenderStride();
	}
	else
	{
		north_stride = render_stride;
	}

	if (mPatchp->getNeighborPatch(EAST))
	{
		east_stride = mPatchp->getNeighborPatch(EAST)->getRenderStride();
	}
	else
	{
		east_stride = render_stride;
	}

	mLastLength = length;
	mLastStride = render_stride;
	mLastNorthStride = north_stride;
	mLastEastStride = east_stride;

	return TRUE;
}

void LLVOSurfacePatch::updateFaceSize(S32 idx)
{
	if (idx != 0)
	{
		llwarns << "Terrain partition requested invalid face!!!" << llendl;
		return;
	}

	LLFace* facep = mDrawable->getFace(idx);

	S32 num_vertices = 0;
	S32 num_indices = 0;
	
	if (mLastStride)
	{
		getGeomSizesMain(mLastStride, num_vertices, num_indices);
		getGeomSizesNorth(mLastStride, mLastNorthStride, num_vertices, num_indices);
		getGeomSizesEast(mLastStride, mLastEastStride, num_vertices, num_indices);
	}

	facep->setSize(num_vertices, num_indices);	
}

BOOL LLVOSurfacePatch::updateLOD()
{
	//mDrawable->updateLightSet();
	mDrawable->setState(LLDrawable::LIGHTING_BUILT);
	return TRUE;
}

void LLVOSurfacePatch::getGeometry(LLStrider<LLVector3> &verticesp,
								LLStrider<LLVector3> &normalsp,
								LLStrider<LLColor4U> &colorsp,
								LLStrider<LLVector2> &texCoords0p,
								LLStrider<LLVector2> &texCoords1p,
								LLStrider<U32> &indicesp)
{
	LLFace* facep = mDrawable->getFace(0);

	U32 index_offset = facep->getGeomIndex();

	updateMainGeometry(facep, 
					verticesp,
					normalsp,
					colorsp,
					texCoords0p,
					texCoords1p,
					indicesp,
					index_offset);
	updateNorthGeometry(facep, 
						verticesp,
						normalsp,
						colorsp,
						texCoords0p,
						texCoords1p,
						indicesp,
						index_offset);
	updateEastGeometry(facep, 
						verticesp,
						normalsp,
						colorsp,
						texCoords0p,
						texCoords1p,
						indicesp,
						index_offset);
}

void LLVOSurfacePatch::updateMainGeometry(LLFace *facep,
										LLStrider<LLVector3> &verticesp,
										LLStrider<LLVector3> &normalsp,
										LLStrider<LLColor4U> &colorsp,
										LLStrider<LLVector2> &texCoords0p,
										LLStrider<LLVector2> &texCoords1p,
										LLStrider<U32> &indicesp,
										U32 &index_offset)
{
	S32 i, j, x, y;

	U32 patch_size, render_stride;
	S32 num_vertices, num_indices;
	U32 index;

	render_stride = mLastStride;
	patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();
	S32 vert_size = patch_size / render_stride;

	///////////////////////////
	//
	// Render the main patch
	//
	//

	num_vertices = 0;
	num_indices = 0;
	// First, figure out how many vertices we need...
	getGeomSizesMain(render_stride, num_vertices, num_indices);

	if (num_vertices > 0)
	{
		facep->mCenterAgent = mPatchp->getPointAgent(8, 8);

		// Generate patch points first
		for (j = 0; j < vert_size; j++)
		{
			for (i = 0; i < vert_size; i++)
			{
				x = i * render_stride;
				y = j * render_stride;
				mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
				calcColor(verticesp.get(), normalsp.get(), colorsp.get());
				verticesp++;
				normalsp++;
				colorsp++;
				texCoords0p++;
				texCoords1p++;
			}
		}

		for (j = 0; j < (vert_size - 1); j++)
		{
			if (j % 2)
			{
				for (i = (vert_size - 1); i > 0; i--)
				{
					index = (i - 1)+ j*vert_size;
					*(indicesp++) = index_offset + index;

					index = i + (j+1)*vert_size;
					*(indicesp++) = index_offset + index;

					index = (i - 1) + (j+1)*vert_size;
					*(indicesp++) = index_offset + index;

					index = (i - 1) + j*vert_size;
					*(indicesp++) = index_offset + index;

					index = i + j*vert_size;
					*(indicesp++) = index_offset + index;

					index = i + (j+1)*vert_size;
					*(indicesp++) = index_offset + index;
				}
			}
			else
			{
				for (i = 0; i < (vert_size - 1); i++)
				{
					index = i + j*vert_size;
					*(indicesp++) = index_offset + index;

					index = (i + 1) + (j+1)*vert_size;
					*(indicesp++) = index_offset + index;

					index = i + (j+1)*vert_size;
					*(indicesp++) = index_offset + index;

					index = i + j*vert_size;
					*(indicesp++) = index_offset + index;

					index = (i + 1) + j*vert_size;
					*(indicesp++) = index_offset + index;

					index = (i + 1) + (j + 1)*vert_size;
					*(indicesp++) = index_offset + index;
				}
			}
		}
	}
	index_offset += num_vertices;
}


void LLVOSurfacePatch::updateNorthGeometry(LLFace *facep,
										LLStrider<LLVector3> &verticesp,
										LLStrider<LLVector3> &normalsp,
										LLStrider<LLColor4U> &colorsp,
										LLStrider<LLVector2> &texCoords0p,
										LLStrider<LLVector2> &texCoords1p,
										LLStrider<U32> &indicesp,
										U32 &index_offset)
{
	S32 vertex_count = 0;
	S32 i, x, y;

	S32 num_vertices, num_indices;

	U32 render_stride = mLastStride;
	S32 patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();
	S32 length = patch_size / render_stride;
	S32 half_length = length / 2;
	U32 north_stride = mLastNorthStride;
	
	///////////////////////////
	//
	// Render the north strip
	//
	//

	// Stride lengths are the same
	if (north_stride == render_stride)
	{
		num_vertices = 2 * length + 1;
		num_indices = length * 6 - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(8, 15) + mPatchp->getPointAgent(8, 16))*0.5f;

		// Main patch
		for (i = 0; i < length; i++)
		{
			x = i * render_stride;
			y = 16 - render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}

		// North patch
		for (i = 0; i <= length; i++)
		{
			x = i * render_stride;
			y = 16;
			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}


		for (i = 0; i < length; i++)
		{
			// Generate indices
			*(indicesp++) = index_offset + i;
			*(indicesp++) = index_offset + length + i + 1;
			*(indicesp++) = index_offset + length + i;

			if (i != length - 1)
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + i + 1;
				*(indicesp++) = index_offset + length + i + 1;
			}
		}
	}
	else if (north_stride > render_stride)
	{
		// North stride is longer (has less vertices)
		num_vertices = length + length/2 + 1;
		num_indices = half_length*9 - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(7, 15) + mPatchp->getPointAgent(8, 16))*0.5f;

		// Iterate through this patch's points
		for (i = 0; i < length; i++)
		{
			x = i * render_stride;
			y = 16 - render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}

		// Iterate through the north patch's points
		for (i = 0; i <= length; i+=2)
		{
			x = i * render_stride;
			y = 16;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}


		for (i = 0; i < length; i++)
		{
			if (!(i % 2))
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + i + 1;
				*(indicesp++) = index_offset + length + (i/2);

				*(indicesp++) = index_offset + i + 1;
				*(indicesp++) = index_offset + length + (i/2) + 1;
				*(indicesp++) = index_offset + length + (i/2);
			}
			else if (i < (length - 1))
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + i + 1;
				*(indicesp++) = index_offset + length + (i/2) + 1;
			}
		}
	}
	else
	{
		// North stride is shorter (more vertices)
		length = patch_size / north_stride;
		half_length = length / 2;
		num_vertices = length + half_length + 1;
		num_indices = 9*half_length - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(15, 7) + mPatchp->getPointAgent(16, 8))*0.5f;

		// Iterate through this patch's points
		for (i = 0; i < length; i+=2)
		{
			x = i * north_stride;
			y = 16 - render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}

		// Iterate through the north patch's points
		for (i = 0; i <= length; i++)
		{
			x = i * north_stride;
			y = 16;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
			vertex_count++;
		}

		for (i = 0; i < length; i++)
		{
			if (!(i%2))
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + i/2;
				*(indicesp++) = index_offset + half_length + i + 1;
			}
			else if (i < (length - 2))
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + i/2;
				*(indicesp++) = index_offset + i/2 + 1;

				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + i/2 + 1;
				*(indicesp++) = index_offset + half_length + i + 1;
			}
			else
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + i/2;
				*(indicesp++) = index_offset + half_length + i + 1;
			}
		}
	}
	index_offset += num_vertices;
}

void LLVOSurfacePatch::updateEastGeometry(LLFace *facep,
										  LLStrider<LLVector3> &verticesp,
										  LLStrider<LLVector3> &normalsp,
										  LLStrider<LLColor4U> &colorsp,
										  LLStrider<LLVector2> &texCoords0p,
										  LLStrider<LLVector2> &texCoords1p,
										  LLStrider<U32> &indicesp,
										  U32 &index_offset)
{
	S32 i, x, y;

	S32 num_vertices, num_indices;

	U32 render_stride = mLastStride;
	S32 patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();
	S32 length = patch_size / render_stride;
	S32 half_length = length / 2;

	U32 east_stride = mLastEastStride;

	// Stride lengths are the same
	if (east_stride == render_stride)
	{
		num_vertices = 2 * length + 1;
		num_indices = length * 6 - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(8, 15) + mPatchp->getPointAgent(8, 16))*0.5f;

		// Main patch
		for (i = 0; i < length; i++)
		{
			x = 16 - render_stride;
			y = i * render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}

		// East patch
		for (i = 0; i <= length; i++)
		{
			x = 16;
			y = i * render_stride;
			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}


		for (i = 0; i < length; i++)
		{
			// Generate indices
			*(indicesp++) = index_offset + i;
			*(indicesp++) = index_offset + length + i;
			*(indicesp++) = index_offset + length + i + 1;

			if (i != length - 1)
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + length + i + 1;
				*(indicesp++) = index_offset + i + 1;
			}
		}
	}
	else if (east_stride > render_stride)
	{
		// East stride is longer (has less vertices)
		num_vertices = length + half_length + 1;
		num_indices = half_length*9 - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(7, 15) + mPatchp->getPointAgent(8, 16))*0.5f;

		// Iterate through this patch's points
		for (i = 0; i < length; i++)
		{
			x = 16 - render_stride;
			y = i * render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}
		// Iterate through the east patch's points
		for (i = 0; i <= length; i+=2)
		{
			x = 16;
			y = i * render_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}

		for (i = 0; i < length; i++)
		{
			if (!(i % 2))
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + length + (i/2);
				*(indicesp++) = index_offset + i + 1;

				*(indicesp++) = index_offset + i + 1;
				*(indicesp++) = index_offset + length + (i/2);
				*(indicesp++) = index_offset + length + (i/2) + 1;
			}
			else if (i < (length - 1))
			{
				*(indicesp++) = index_offset + i;
				*(indicesp++) = index_offset + length + (i/2) + 1;
				*(indicesp++) = index_offset + i + 1;
			}
		}
	}
	else
	{
		// East stride is shorter (more vertices)
		length = patch_size / east_stride;
		half_length = length / 2;
		num_vertices = length + length/2 + 1;
		num_indices = 9*(length/2) - 3;

		facep->mCenterAgent = (mPatchp->getPointAgent(15, 7) + mPatchp->getPointAgent(16, 8))*0.5f;

		// Iterate through this patch's points
		for (i = 0; i < length; i+=2)
		{
			x = 16 - render_stride;
			y = i * east_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}
		// Iterate through the east patch's points
		for (i = 0; i <= length; i++)
		{
			x = 16;
			y = i * east_stride;

			mPatchp->eval(x, y, render_stride, verticesp.get(), normalsp.get(), texCoords0p.get(), texCoords1p.get());
			calcColor(verticesp.get(), normalsp.get(), colorsp.get());
			verticesp++;
			normalsp++;
			colorsp++;
			texCoords0p++;
			texCoords1p++;
		}

		for (i = 0; i < length; i++)
		{
			if (!(i%2))
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + half_length + i + 1;
				*(indicesp++) = index_offset + i/2;
			}
			else if (i < (length - 2))
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + i/2 + 1;
				*(indicesp++) = index_offset + i/2;

				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + half_length + i + 1;
				*(indicesp++) = index_offset + i/2 + 1;
			}
			else
			{
				*(indicesp++) = index_offset + half_length + i;
				*(indicesp++) = index_offset + half_length + i + 1;
				*(indicesp++) = index_offset + i/2;
			}
		}
	}
	index_offset += num_vertices;
}

void LLVOSurfacePatch::calcColor(const LLVector3* vertex, const LLVector3* normal, LLColor4U* colorp)
{
	LLColor4 color(0,0,0,0);
	if (gPipeline.getLightingDetail() >= 2)
	{
		for (LLDrawable::drawable_set_t::iterator iter = mDrawable->mLightSet.begin();
			 iter != mDrawable->mLightSet.end(); ++iter)
		{
			LLDrawable* light_drawable = *iter;
			LLVOVolume* light = light_drawable->getVOVolume();
			if (!light)
			{
				continue;
			}
			LLColor4 light_color;
			light->calcLightAtPoint(*vertex, *normal, light_color);
			color += light_color;
		}

		color.mV[3] = 1.0f;
	}
	colorp->setVecScaleClamp(color);
}

BOOL LLVOSurfacePatch::updateShadows(BOOL use_shadow_factor)
{
	return FALSE; //terrain updates its shadows during standard relight
}

void LLVOSurfacePatch::setPatch(LLSurfacePatch *patchp)
{
	mPatchp = patchp;

	dirtyPatch();
};


void LLVOSurfacePatch::dirtyPatch()
{
	mDirtiedPatch = TRUE;
	dirtyGeom();
	mDirtyTerrain = TRUE;
	LLVector3 center = mPatchp->getCenterRegion();
	LLSurface *surfacep = mPatchp->getSurface();

	setPositionRegion(center);

	F32 scale_factor = surfacep->getGridsPerPatchEdge() * surfacep->getMetersPerGrid();
	setScale(LLVector3(scale_factor, scale_factor, mPatchp->getMaxZ() - mPatchp->getMinZ()));
}

void LLVOSurfacePatch::dirtyGeom()
{
	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
		mDrawable->getFace(0)->mVertexBuffer = NULL;
		mDrawable->movePartition();
	}
}

void LLVOSurfacePatch::getGeomSizesMain(const S32 stride, S32 &num_vertices, S32 &num_indices)
{
	S32 patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();

	// First, figure out how many vertices we need...
	S32 vert_size = patch_size / stride;
	if (vert_size >= 2)
	{
		num_vertices += vert_size * vert_size;
		num_indices += 6 * (vert_size - 1)*(vert_size - 1);
	}
}

void LLVOSurfacePatch::getGeomSizesNorth(const S32 stride, const S32 north_stride,
										 S32 &num_vertices, S32 &num_indices)
{
	S32 patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();
	S32 length = patch_size / stride;
	// Stride lengths are the same
	if (north_stride == stride)
	{
		num_vertices += 2 * length + 1;
		num_indices += length * 6 - 3;
	}
	else if (north_stride > stride)
	{
		// North stride is longer (has less vertices)
		num_vertices += length + (length/2) + 1;
		num_indices += (length/2)*9 - 3;
	}
	else
	{
		// North stride is shorter (more vertices)
		length = patch_size / north_stride;
		num_vertices += length + (length/2) + 1;
		num_indices += 9*(length/2) - 3;
	}
}

void LLVOSurfacePatch::getGeomSizesEast(const S32 stride, const S32 east_stride,
										S32 &num_vertices, S32 &num_indices)
{
	S32 patch_size = mPatchp->getSurface()->getGridsPerPatchEdge();
	S32 length = patch_size / stride;
	// Stride lengths are the same
	if (east_stride == stride)
	{
		num_vertices += 2 * length + 1;
		num_indices += length * 6 - 3;
	}
	else if (east_stride > stride)
	{
		// East stride is longer (has less vertices)
		num_vertices += length + (length/2) + 1;
		num_indices += (length/2)*9 - 3;
	}
	else
	{
		// East stride is shorter (more vertices)
		length = patch_size / east_stride;
		num_vertices += length + (length/2) + 1;
		num_indices += 9*(length/2) - 3;
	}
}

void LLVOSurfacePatch::updateSpatialExtents(LLVector3& newMin, LLVector3 &newMax)
{
	LLVector3 posAgent = getPositionAgent();
	LLVector3 scale = getScale();
	newMin = posAgent-scale*0.5f;
	newMax = posAgent+scale*0.5f;
	mDrawable->setPositionGroup((newMin+newMax)*0.5f);
}

U32 LLVOSurfacePatch::getPartitionType() const
{ 
	return LLPipeline::PARTITION_TERRAIN; 
}

LLTerrainPartition::LLTerrainPartition()
: LLSpatialPartition(LLDrawPoolTerrain::VERTEX_DATA_MASK)
{
	mRenderByGroup = FALSE;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
	mDrawableType = LLPipeline::RENDER_TYPE_TERRAIN;
	mPartitionType = LLPipeline::PARTITION_TERRAIN;
}

LLVertexBuffer* LLTerrainPartition::createVertexBuffer(U32 type_mask, U32 usage)
{
	return new LLVertexBufferTerrain();
}

void LLTerrainPartition::getGeometry(LLSpatialGroup* group)
{
	LLVertexBuffer* buffer = group->mVertexBuffer;

	//get vertex buffer striders
	LLStrider<LLVector3> vertices;
	LLStrider<LLVector3> normals;
	LLStrider<LLVector2> texcoords2;
	LLStrider<LLVector2> texcoords;
	LLStrider<LLColor4U> colors;
	LLStrider<U32> indices;

	buffer->getVertexStrider(vertices);
	buffer->getNormalStrider(normals);
	buffer->getTexCoordStrider(texcoords);
	buffer->getTexCoord2Strider(texcoords2);
	buffer->getColorStrider(colors);
	buffer->getIndexStrider(indices);

	U32 indices_index = 0;
	U32 index_offset = 0;

	for (std::vector<LLFace*>::iterator i = mFaceList.begin(); i != mFaceList.end(); ++i)
	{
		LLFace* facep = *i;

		facep->setIndicesIndex(indices_index);
		facep->setGeomIndex(index_offset);
		facep->mVertexBuffer = buffer;

		LLVOSurfacePatch* patchp = (LLVOSurfacePatch*) facep->getViewerObject();
		patchp->getGeometry(vertices, normals, colors, texcoords, texcoords2, indices);

		indices_index += facep->getIndicesCount();
		index_offset += facep->getGeomCount();
	}

	mFaceList.clear();
}

