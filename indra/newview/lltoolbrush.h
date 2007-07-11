/** 
 * @file lltoolbrush.h
 * @brief toolbrush class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLBRUSH_H
#define LL_LLTOOLBRUSH_H

#include "lltool.h"
#include "v3math.h"
#include "lleditmenuhandler.h"

class LLSurface;
class LLVector3d;
class LLViewerRegion;

template<class DATA_TYPE> class LLLinkedList;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLToolBrushLand
//
// A toolbrush that modifies the land.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLToolBrushLand : public LLTool, public LLEditMenuHandler
{
public:
	LLToolBrushLand();
	
	// x,y in window coords, 0,0 = left,bot
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );		
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual void handleSelect();
	virtual void handleDeselect();

	// isAlwaysRendered() - return true if this is a tool that should
	// always be rendered regardless of selection.
	virtual BOOL isAlwaysRendered() { return TRUE; }

	// Draw the area that will be affected.
	virtual void render();

	// on Idle is where the land modification actually occurs
	static void onIdle(void* brush_tool);  

	void			onMouseCaptureLost();

	void modifyLandInSelectionGlobal();
	virtual void	undo();
	virtual BOOL	canUndo()	{ return TRUE; }

	//virtual void	redo();
	virtual BOOL	canRedo()	{ return FALSE; }


protected:
	void brush( void );
	void modifyLandAtPointGlobal( const LLVector3d &spot, MASK mask );

	void determineAffectedRegions(LLLinkedList<LLViewerRegion>& regions,
								  const LLVector3d& spot) const;
	void renderOverlay(LLSurface& land, const LLVector3& pos_region,
					   const LLVector3& pos_world);

	// Does region allow terraform, or are we a god?
	bool canTerraform(LLViewerRegion* regionp) const;

	// Modal dialog that you can't terraform the region
	void alertNoTerraform(LLViewerRegion* regionp);

protected:
	F32 mStartingZ;
	S32 mMouseX;
	S32 mMouseY;
	S32 mBrushIndex;
	BOOL mGotHover;
	BOOL mLastShowParcelOwners;
	BOOL mBrushSelected;
	LLLinkedList<LLViewerRegion> mLastAffectedRegions;
};

extern LLToolBrushLand *gToolLand;

#endif // LL_LLTOOLBRUSH_H
