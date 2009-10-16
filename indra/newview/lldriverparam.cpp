/** 
 * @file lldriverparam.cpp
 * @brief A visual parameter that drives (controls) other visual parameters.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "lldriverparam.h"

#include "llfasttimer.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llagent.h"
#include "llwearable.h"

//-----------------------------------------------------------------------------
// LLDriverParamInfo
//-----------------------------------------------------------------------------

LLDriverParamInfo::LLDriverParamInfo()
{
}

BOOL LLDriverParamInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "param" ) && node->getChildByName( "param_driver" ) );

	if( !LLViewerVisualParamInfo::parseXml( node ))
		return FALSE;

	LLXmlTreeNode* param_driver_node = node->getChildByName( "param_driver" );
	if( !param_driver_node )
		return FALSE;

	for (LLXmlTreeNode* child = param_driver_node->getChildByName( "driven" );
		 child;
		 child = param_driver_node->getNextNamedChild())
	{
		S32 driven_id;
		static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
		if( child->getFastAttributeS32( id_string, driven_id ) )
		{
			F32 min1 = mMinWeight;
			F32 max1 = mMaxWeight;
			F32 max2 = max1;
			F32 min2 = max1;

			//	driven    ________							//
			//	^        /|       |\						//
			//	|       / |       | \						//
			//	|      /  |       |  \						//
			//	|     /   |       |   \						//
			//	|    /    |       |    \					//
			//-------|----|-------|----|-------> driver		//
			//  | min1   max1    max2  min2

			static LLStdStringHandle min1_string = LLXmlTree::addAttributeString("min1");
			child->getFastAttributeF32( min1_string, min1 ); // optional
			static LLStdStringHandle max1_string = LLXmlTree::addAttributeString("max1");
			child->getFastAttributeF32( max1_string, max1 ); // optional
			static LLStdStringHandle max2_string = LLXmlTree::addAttributeString("max2");
			child->getFastAttributeF32( max2_string, max2 ); // optional
			static LLStdStringHandle min2_string = LLXmlTree::addAttributeString("min2");
			child->getFastAttributeF32( min2_string, min2 ); // optional

			// Push these on the front of the deque, so that we can construct
			// them in order later (faster)
			mDrivenInfoList.push_front( LLDrivenEntryInfo( driven_id, min1, max1, max2, min2 ) );
		}
		else
		{
			llerrs << "<driven> Unable to resolve driven parameter: " << driven_id << llendl;
			return FALSE;
		}
	}
	return TRUE;
}

//virtual 
void LLDriverParamInfo::toStream(std::ostream &out)
{
	LLViewerVisualParamInfo::toStream(out);
	out << "driver" << "\t";
	out << mDrivenInfoList.size() << "\t";
	for (entry_info_list_t::iterator iter = mDrivenInfoList.begin(); iter != mDrivenInfoList.end(); iter++)
	{
		LLDrivenEntryInfo driven = *iter;
		out << driven.mDrivenID << "\t";
	}

	out << std::endl;

	LLVOAvatarSelf *avatar = gAgent.getAvatarObject();
	if(avatar)
	{
		for (entry_info_list_t::iterator iter = mDrivenInfoList.begin(); iter != mDrivenInfoList.end(); iter++)
		{
			LLDrivenEntryInfo driven = *iter;
			LLViewerVisualParam *param = (LLViewerVisualParam*)avatar->getVisualParam(driven.mDrivenID);
			if (param)
			{
				param->getInfo()->toStream(out);
				if (param->getWearableType() != mWearableType)
				{
					if(param->getCrossWearable())
					{
						out << "cross-wearable" << "\t";
					}
					else
					{
						out << "ERROR!" << "\t";
					}
				}
				else
				{
					out << "valid" << "\t";
				}
			}
			else
			{
				llwarns << "could not get parameter " << driven.mDrivenID << " from avatar " << avatar << " for driver parameter " << getID() << llendl;
			}
			out << std::endl;
		}
	}
}

//-----------------------------------------------------------------------------
// LLDriverParam
//-----------------------------------------------------------------------------

LLDriverParam::LLDriverParam(LLVOAvatar *avatarp)
	: mCurrentDistortionParam( NULL ), mAvatarp(avatarp), mWearablep(NULL)
{
}

LLDriverParam::LLDriverParam(LLWearable *wearablep)
	: mCurrentDistortionParam( NULL ), mAvatarp(NULL), mWearablep(wearablep)
{
}

LLDriverParam::~LLDriverParam()
{
}

BOOL LLDriverParam::setInfo(LLDriverParamInfo *info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
		return FALSE;
	mInfo = info;
	mID = info->mID;

	setWeight(getDefaultWeight(), FALSE );

	BOOL success;
	if (mWearablep)
	{
		LLVisualParam*(LLWearable::*function)(S32)const = &LLWearable::getVisualParam; // need this line to disambiguate between versions of LLCharacter::getVisualParam()
		success = linkDrivenParams(boost::bind(function,(LLWearable*)mWearablep, _1), false);
	}
	else
	{
		LLVisualParam*(LLCharacter::*function)(S32)const = &LLCharacter::getVisualParam; // need this line to disambiguate between versions of LLCharacter::getVisualParam()
		success = linkDrivenParams(boost::bind(function,(LLCharacter*)mAvatarp, _1), false);
	}
	if(!success)
	{
		mInfo = NULL;
		return FALSE;
	}
	
	return TRUE;
}

void LLDriverParam::setWearable(LLWearable *wearablep)
{
	if (wearablep)
	{
		mWearablep = wearablep;
		mAvatarp = NULL;
	}
}

void LLDriverParam::setAvatar(LLVOAvatar *avatarp)
{
	if (avatarp)
	{
		mWearablep = NULL;
		mAvatarp = avatarp;
	}
}

/*virtual*/ LLViewerVisualParam * 	LLDriverParam::cloneParam(LLWearable* wearable) const
{
	LLDriverParam *new_param;
	if (wearable)
	{
		new_param = new LLDriverParam(wearable);
	}
	else
	{
		if (mWearablep)
		{
			new_param = new LLDriverParam(mWearablep);
		}
		else
		{
			new_param = new LLDriverParam(mAvatarp);
		}
	}
	*new_param = *this;
	return new_param;
}

#if 0 // obsolete
BOOL LLDriverParam::parseData(LLXmlTreeNode* node)
{
	LLDriverParamInfo* info = new LLDriverParamInfo;

	info->parseXml(node);
	if (!setInfo(info))
	{
		delete info;
		return FALSE;
	}
	return TRUE;
}
#endif

void LLDriverParam::setWeight(F32 weight, BOOL set_by_user)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	if (mIsAnimating)
	{
		// allow overshoot when animating
		mCurWeight = weight;
	}
	else
	{
		mCurWeight = llclamp(weight, min_weight, max_weight);
	}

	//	driven    ________
	//	^        /|       |\       ^
	//	|       / |       | \      |
	//	|      /  |       |  \     |
	//	|     /   |       |   \    |
	//	|    /    |       |    \   |
	//-------|----|-------|----|-------> driver
	//  | min1   max1    max2  min2

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		LLDrivenEntryInfo* info = driven->mInfo;
		
		F32 driven_weight = 0.f;
		F32 driven_min = driven->mParam->getMinWeight();
		F32 driven_max = driven->mParam->getMaxWeight();

		if (mIsAnimating)
		{
			// driven param doesn't interpolate (textures, for example)
			if (!driven->mParam->getAnimating())
			{
				continue;
			}
			if( mCurWeight < info->mMin1 )
			{
				if (info->mMin1 == min_weight)
				{
					if (info->mMin1 == info->mMax1)
					{
						driven_weight = driven_max;
					}
					else
					{
						//up slope extrapolation
						F32 t = (mCurWeight - info->mMin1) / (info->mMax1 - info->mMin1 );
						driven_weight = driven_min + t * (driven_max - driven_min);
					}
				}
				else
				{
					driven_weight = driven_min;
				}
				
				setDrivenWeight(driven,driven_weight,set_by_user);
				continue;
			}
			else 
			if ( mCurWeight > info->mMin2 )
			{
				if (info->mMin2 == max_weight)
				{
					if (info->mMin2 == info->mMax2)
					{
						driven_weight = driven_max;
					}
					else
					{
						//down slope extrapolation					
						F32 t = (mCurWeight - info->mMax2) / (info->mMin2 - info->mMax2 );
						driven_weight = driven_max + t * (driven_min - driven_max);
					}
				}
				else
				{
					driven_weight = driven_min;
				}

				setDrivenWeight(driven,driven_weight,set_by_user);
				continue;
			}
		}

		driven_weight = getDrivenWeight(driven, mCurWeight);
		setDrivenWeight(driven,driven_weight,set_by_user);
	}
}

F32	LLDriverParam::getTotalDistortion()
{
	F32 sum = 0.f;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getTotalDistortion();
	}

	return sum; 
}

const LLVector3	&LLDriverParam::getAvgDistortion()	
{
	// It's not actually correct to take the average of averages, but it good enough here.
	LLVector3 sum;
	S32 count = 0;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getAvgDistortion();
		count++;
	}
	sum /= (F32)count;

	mDefaultVec = sum;
	return mDefaultVec; 
}

F32	LLDriverParam::getMaxDistortion() 
{
	F32 max = 0.f;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		F32 param_max = driven->mParam->getMaxDistortion();
		if( param_max > max )
		{
			max = param_max;
		}
	}

	return max; 
}


LLVector3	LLDriverParam::getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)
{
	LLVector3 sum;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getVertexDistortion( index, poly_mesh );
	}
	return sum;
}

const LLVector3*	LLDriverParam::getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	mCurrentDistortionParam = NULL;
	const LLVector3* v = NULL;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		v = driven->mParam->getFirstDistortion( index, poly_mesh );
		if( v )
		{
			mCurrentDistortionParam = driven->mParam;
			break;
		}
	}

	return v;
};

const LLVector3*	LLDriverParam::getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	llassert( mCurrentDistortionParam );
	if( !mCurrentDistortionParam )
	{
		return NULL;
	}

	LLDrivenEntry* driven = NULL;
	entry_list_t::iterator iter;
	
	// Set mDriven iteration to the right point
	for( iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		driven = &(*iter);
		if( driven->mParam == mCurrentDistortionParam )
		{
			break;
		}
	}

	// We're already in the middle of a param's distortions, so get the next one.
	const LLVector3* v = driven->mParam->getNextDistortion( index, poly_mesh );
	if( (!v) && (iter != mDriven.end()) )
	{
		// This param is finished, so start the next param.  It might not have any
		// distortions, though, so we have to loop to find the next param that does.
		for( iter++; iter != mDriven.end(); iter++ )
		{
			driven = &(*iter);
			v = driven->mParam->getFirstDistortion( index, poly_mesh );
			if( v )
			{
				mCurrentDistortionParam = driven->mParam;
				break;
			}
		}
	}

	return v;
};

//-----------------------------------------------------------------------------
// setAnimationTarget()
//-----------------------------------------------------------------------------
void LLDriverParam::setAnimationTarget( F32 target_value, BOOL set_by_user )
{
	LLVisualParam::setAnimationTarget(target_value, set_by_user);

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		F32 driven_weight = getDrivenWeight(driven, mTargetWeight);

		// this isn't normally necessary, as driver params handle interpolation of their driven params
		// but texture params need to know to assume their final value at beginning of interpolation
		driven->mParam->setAnimationTarget(driven_weight, set_by_user);
	}
}

//-----------------------------------------------------------------------------
// stopAnimating()
//-----------------------------------------------------------------------------
void LLDriverParam::stopAnimating(BOOL set_by_user)
{
	LLVisualParam::stopAnimating(set_by_user);

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		driven->mParam->setAnimating(FALSE);
	}
}

/*virtual*/ 
BOOL LLDriverParam::linkDrivenParams(visual_param_mapper mapper, bool only_cross_params)
{
	BOOL success = TRUE;
	LLDriverParamInfo::entry_info_list_t::iterator iter;
	mDriven.clear();
	mDriven.reserve(getInfo()->mDrivenInfoList.size());
	for (iter = getInfo()->mDrivenInfoList.begin(); iter != getInfo()->mDrivenInfoList.end(); ++iter)
	{
		LLDrivenEntryInfo *driven_info = &(*iter);
		S32 driven_id = driven_info->mDrivenID;

		// check for already existing links. Do not overwrite.
		BOOL found = FALSE;
		for (entry_list_t::iterator driven_iter = mDriven.begin(); driven_iter != mDriven.end() && !found; ++driven_iter)
		{
			if (driven_iter->mInfo->mDrivenID == driven_id)
			{
				found = TRUE;
			}
		}

		if (!found)
		{
			LLViewerVisualParam* param = (LLViewerVisualParam*)mapper(driven_id);
			bool push = param && (!only_cross_params || param->getCrossWearable());
			if (push)
			{
				mDriven.push_back(LLDrivenEntry( param, driven_info ));
			}
			else
			{
				success = FALSE;
			}
		}
	}
	
	return success;	
}

//-----------------------------------------------------------------------------
// getDrivenWeight()
//-----------------------------------------------------------------------------
F32 LLDriverParam::getDrivenWeight(const LLDrivenEntry* driven, F32 input_weight)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	const LLDrivenEntryInfo* info = driven->mInfo;

	F32 driven_weight = 0.f;
	F32 driven_min = driven->mParam->getMinWeight();
	F32 driven_max = driven->mParam->getMaxWeight();

	if( input_weight <= info->mMin1 )
	{
		if( info->mMin1 == info->mMax1 && 
			info->mMin1 <= min_weight)
		{
			driven_weight = driven_max;
		}
		else
		{
			driven_weight = driven_min;
		}
	}
	else
	if( input_weight <= info->mMax1 )
	{
		F32 t = (input_weight - info->mMin1) / (info->mMax1 - info->mMin1 );
		driven_weight = driven_min + t * (driven_max - driven_min);
	}
	else
	if( input_weight <= info->mMax2 )
	{
		driven_weight = driven_max;
	}
	else
	if( input_weight <= info->mMin2 )
	{
		F32 t = (input_weight - info->mMax2) / (info->mMin2 - info->mMax2 );
		driven_weight = driven_max + t * (driven_min - driven_max);
	}
	else
	{
		if (info->mMax2 >= max_weight)
		{
			driven_weight = driven_max;
		}
		else
		{
			driven_weight = driven_min;
		}
	}

	return driven_weight;
}

void LLDriverParam::setDrivenWeight(LLDrivenEntry *driven, F32 driven_weight, bool set_by_user)
{
	LLVOAvatarSelf *avatar_self = gAgent.getAvatarObject();
	if(mWearablep && driven->mParam->getCrossWearable() &&
	   mWearablep->isOnTop())
	{
		// call setWeight through LLVOAvatarSelf so other wearables can be updated with the correct values
		avatar_self->setVisualParamWeight( (LLVisualParam*)driven->mParam, driven_weight, set_by_user );
	}
	else
	{
		driven->mParam->setWeight( driven_weight, set_by_user );
	}
}
