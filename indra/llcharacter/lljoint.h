/** 
 * @file lljoint.h
 * @brief Implementation of LLJoint class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLJOINT_H
#define LL_LLJOINT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include <string>

#include "linked_lists.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "xform.h"
#include "lldarray.h"

const S32 LL_CHARACTER_MAX_JOINTS_PER_MESH = 15;
const U32 LL_CHARACTER_MAX_JOINTS = 32; // must be divisible by 4!
const U32 LL_HAND_JOINT_NUM = 31;
const U32 LL_FACE_JOINT_NUM = 30;
const S32 LL_CHARACTER_MAX_PRIORITY = 7;
const F32 LL_MAX_PELVIS_OFFSET = 5.f;

//-----------------------------------------------------------------------------
// class LLJoint
//-----------------------------------------------------------------------------
class LLJoint
{
public:
	// priority levels, from highest to lowest
	enum JointPriority
	{
		USE_MOTION_PRIORITY = -1,
		LOW_PRIORITY = 0,
		MEDIUM_PRIORITY,
		HIGH_PRIORITY,
		HIGHER_PRIORITY,
		HIGHEST_PRIORITY,
		ADDITIVE_PRIORITY = LL_CHARACTER_MAX_PRIORITY
	};

	enum DirtyFlags
	{
		MATRIX_DIRTY = 0x1 << 0,
		ROTATION_DIRTY = 0x1 << 1,
		POSITION_DIRTY = 0x1 << 2,
		ALL_DIRTY = 0x7
	};
protected:
	std::string	mName;

	// parent joint
	LLJoint	*mParent;

	// explicit transformation members
	LLXformMatrix		mXform;

public:
	U32				mDirtyFlags;
	BOOL			mWorldRotationDirty;
	BOOL			mUpdateXform;

	// describes the skin binding pose
	LLVector3		mSkinOffset;

	S32				mJointNum;

	LLDynamicArray<LLVector3> mConstraintSilhouette;

	// child joints
	typedef std::list<LLJoint*> child_list_t;
	child_list_t mChildren;

	// debug statics
	static S32		sNumTouches;
	static S32		sNumUpdates;

public:
	LLJoint();
	LLJoint( const std::string &name, LLJoint *parent=NULL );

	virtual ~LLJoint();

	// set name and parent
	void setup( const std::string &name, LLJoint *parent=NULL );

	void touch(U32 flags = ALL_DIRTY);

	// get/set name
	const std::string &getName() { return mName; }
	void setName( const std::string &name ) { mName = name; }

	// getParent
	LLJoint *getParent() { return mParent; }

	// getRoot
	LLJoint *getRoot();

	// search for child joints by name
	LLJoint *findJoint( const std::string &name );

	// add/remove children
	void addChild( LLJoint *joint );
	void removeChild( LLJoint *joint );
	void removeAllChildren();

	// get/set local position
	const LLVector3& getPosition();
	void setPosition( const LLVector3& pos );

	// get/set world position
	LLVector3 getWorldPosition();
	LLVector3 getLastWorldPosition();
	void setWorldPosition( const LLVector3& pos );

	// get/set local rotation
	const LLQuaternion& getRotation();
	void setRotation( const LLQuaternion& rot );

	// get/set world rotation
	LLQuaternion getWorldRotation();
	LLQuaternion getLastWorldRotation();
	void setWorldRotation( const LLQuaternion& rot );

	// get/set local scale
	const LLVector3& getScale();
	void setScale( const LLVector3& scale );

	// get/set world matrix
	const LLMatrix4 &getWorldMatrix();
	void setWorldMatrix( const LLMatrix4& mat );

	void updateWorldMatrixChildren();
	void updateWorldMatrixParent();

	void updateWorldPRSParent();

	void updateWorldMatrix();

	// get/set skin offset
	const LLVector3 &getSkinOffset();
	void setSkinOffset( const LLVector3 &offset);

	LLXformMatrix	*getXform() { return &mXform; }

	void setConstraintSilhouette(LLDynamicArray<LLVector3>& silhouette);

	void clampRotation(LLQuaternion old_rot, LLQuaternion new_rot);

	virtual BOOL isAnimatable() { return TRUE; }

	S32 getJointNum() { return mJointNum; }
	void setJointNum(S32 joint_num) { mJointNum = joint_num; }
};
#endif // LL_LLJOINT_H

