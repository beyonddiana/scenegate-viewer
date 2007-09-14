/** 
 * @file xform.h
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_XFORM_H
#define LL_XFORM_H

#include "v3math.h"
#include "m4math.h"
#include "llquaternion.h"

#define CHECK_FOR_FINITE


const F32 MAX_OBJECT_Z 		= 768.f;
const F32 MIN_OBJECT_Z 		= -256.f;
const F32 MIN_OBJECT_SCALE 	= 0.01f;
const F32 MAX_OBJECT_SCALE 	= 10.f;

class LLXform
{
protected:
	LLVector3	  mPosition;
	LLQuaternion  mRotation; 
	LLVector3	  mScale;
	
	//RN: TODO: move these world transform members to LLXformMatrix
	// as they are *never* updated or accessed in the base class
	LLVector3	  mWorldPosition;
	LLQuaternion  mWorldRotation;

	LLXform*      mParent;
	U32			  mChanged;

	BOOL		  mScaleChildOffset;

public:
	typedef enum e_changed_flags
	{
		UNCHANGED  	= 0x00,
		TRANSLATED 	= 0x01,
		ROTATED		= 0x02,
		SCALED		= 0x04,
		SHIFTED		= 0x08,
		GEOMETRY	= 0x10,
		TEXTURE		= 0x20,
		MOVED       = TRANSLATED|ROTATED|SCALED,
		SILHOUETTE	= 0x40,
		ALL_CHANGED = 0x7f
	}EChangedFlags;

	void init()
	{
		mParent  = NULL;
		mChanged = UNCHANGED;
		mPosition.setVec(0,0,0);
		mRotation.loadIdentity();
		mScale.   setVec(1,1,1);
		mWorldPosition.clearVec();
		mWorldRotation.loadIdentity();
		mScaleChildOffset = FALSE;
	}

	 LLXform();
	virtual ~LLXform();

	void getLocalMat4(LLMatrix4 &mat) const { mat.initAll(mScale, mRotation, mPosition); }

	inline BOOL setParent(LLXform *parent);

	inline void setPosition(const LLVector3& pos);
	inline void setPosition(const F32 x, const F32 y, const F32 z);
	inline void setPositionX(const F32 x);
	inline void setPositionY(const F32 y);
	inline void setPositionZ(const F32 z);
	inline void addPosition(const LLVector3& pos);


	inline void setScale(const LLVector3& scale);
	inline void setScale(const F32 x, const F32 y, const F32 z);
	inline void setRotation(const LLQuaternion& rot);
	inline void setRotation(const F32 x, const F32 y, const F32 z);
	inline void setRotation(const F32 x, const F32 y, const F32 z, const F32 s);
	
	void 		setChanged(const U32 bits)					{ mChanged |= bits; }
	BOOL		isChanged() const							{ return mChanged; }
	BOOL 		isChanged(const U32 bits) const				{ return mChanged & bits; }
	void 		clearChanged()								{ mChanged = 0; }
	void        clearChanged(U32 bits)                      { mChanged &= ~bits; }

	void		setScaleChildOffset(BOOL scale)				{ mScaleChildOffset = scale; }
	BOOL		getScaleChildOffset()						{ return mScaleChildOffset; }

	LLXform* getParent() const { return mParent; }
	LLXform* getRoot() const;
	virtual BOOL isRoot() const;
	virtual BOOL isRootEdit() const;

	const LLVector3&	getPosition()  const	    { return mPosition; }
	const LLVector3&	getScale() const			{ return mScale; }
	const LLQuaternion& getRotation() const			{ return mRotation; }
	const LLVector3&	getPositionW() const		{ return mWorldPosition; }
	const LLQuaternion& getWorldRotation() const	{ return mWorldRotation; }
	const LLVector3&	getWorldPosition() const	{ return mWorldPosition; }
};

class LLXformMatrix : public LLXform
{
public:
	LLXformMatrix() : LLXform() {};
	virtual ~LLXformMatrix();

	const LLMatrix4&    getWorldMatrix() const      { return mWorldMatrix; }
	void setWorldMatrix (const LLMatrix4& mat)   { mWorldMatrix = mat; }

	void init()
	{
		mWorldMatrix.identity();
		mMin.clearVec();
		mMax.clearVec();

		LLXform::init();
	}

	void update();
	void updateMatrix(BOOL update_bounds = TRUE);
	void getMinMax(LLVector3& min,LLVector3& max) const;

protected:
	LLMatrix4	mWorldMatrix;
	LLVector3	mMin;
	LLVector3	mMax;

};

BOOL LLXform::setParent(LLXform* parent)
{
	// Validate and make sure we're not creating a loop
	if (parent == mParent)
	{
		return TRUE;
	}
	if (parent)
	{
		LLXform *cur_par = parent->mParent;
		while (cur_par)
		{
			if (cur_par == this)
			{
				llwarns << "LLXform::setParent Creating loop when setting parent!" << llendl;
				return FALSE;
			}
			cur_par = cur_par->mParent;
		}
	}
	mParent = parent;
	return TRUE;
}


// Don't blow up on release versions
#if LL_RELEASE_FOR_DOWNLOAD
	#define llxformtrouble(msg, num)	llwarning(msg, num)
#else
	#define llxformtrouble(msg, num)	llerror(msg, num)
#endif


#ifdef CHECK_FOR_FINITE
void LLXform::setPosition(const LLVector3& pos)			
{
	setChanged(TRANSLATED);
	if (pos.isFinite())
	{
		mPosition = pos; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setPosition(LLVector3)", 0);
	}
}

void LLXform::setPosition(const F32 x, const F32 y, const F32 z)
{
	setChanged(TRANSLATED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
	{
		mPosition.setVec(x,y,z); 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setPosition(F32,F32,F32)", 0);
	}
}

void LLXform::setPositionX(const F32 x)
{ 
	setChanged(TRANSLATED);
	if (llfinite(x))
	{
		mPosition.mV[VX] = x; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setPositionX", 0);
	}
}

void LLXform::setPositionY(const F32 y)
{ 
	setChanged(TRANSLATED);
	if (llfinite(y))
	{
		mPosition.mV[VY] = y; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setPositionY", 0);
	}
}

void LLXform::setPositionZ(const F32 z)
{ 
	setChanged(TRANSLATED);
	if (llfinite(z))
	{
		mPosition.mV[VZ] = z; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setPositionZ", 0);
	}
}

void LLXform::addPosition(const LLVector3& pos)
{ 
	setChanged(TRANSLATED);
	if (pos.isFinite())
	{
		mPosition += pos; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::addPosition", 0);
	}
}

void LLXform::setScale(const LLVector3& scale)
{ 
	setChanged(SCALED);
	if (scale.isFinite())
	{
		mScale = scale; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setScale(LLVector)", 0);
	}
}
void LLXform::setScale(const F32 x, const F32 y, const F32 z)
{ 
	setChanged(SCALED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
	{
		mScale.setVec(x,y,z); 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setScale(F32,F32,F32)", 0);
	}
}
void LLXform::setRotation(const LLQuaternion& rot)
{ 
	setChanged(ROTATED);
	if (rot.isFinite())
	{
		mRotation = rot; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setRotation(LLQuaternion)", 0);
	}
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z) 
{ 
	setChanged(ROTATED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
	{
		mRotation.setQuat(x,y,z); 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setRotation(F32,F32,F32)", 0);
	}
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s) 
{ 
	setChanged(ROTATED);
	if (llfinite(x) && llfinite(y) && llfinite(z) && llfinite(s))
	{
		mRotation.mQ[VX] = x; mRotation.mQ[VY] = y; mRotation.mQ[VZ] = z; mRotation.mQ[VS] = s; 
	}
	else
	{
		llxformtrouble("Non Finite in LLXform::setRotation(F32,F32,F32,F32)", 0);
	}
}

#endif

#endif
