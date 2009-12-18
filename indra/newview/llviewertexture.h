/** 
 * @file llviewertexture.h
 * @brief Object for managing images and their textures
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LLVIEWERTEXTURE_H					
#define LL_LLVIEWERTEXTURE_H

#include "lltexture.h"
#include "lltimer.h"
#include "llframetimer.h"
#include "llhost.h"
#include "llgltypes.h"
#include "llrender.h"
#include "llmetricperformancetester.h"

#include <map>
#include <list>

#define MIN_VIDEO_RAM_IN_MEGA_BYTES    32
#define MAX_VIDEO_RAM_IN_MEGA_BYTES    512 // 512MB max for performance reasons.

class LLFace;
class LLImageGL ;
class LLImageRaw;
class LLViewerObject;
class LLViewerTexture;
class LLViewerFetchedTexture ;
class LLViewerMediaTexture ;
class LLTexturePipelineTester ;


typedef	void	(*loaded_callback_func)( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata );

class LLVFile;
class LLMessageSystem;
class LLViewerMediaImpl ;
class LLVOVolume ;

class LLLoadedCallbackEntry
{
public:
	LLLoadedCallbackEntry(loaded_callback_func cb,
						  S32 discard_level,
						  BOOL need_imageraw, // Needs image raw for the callback
						  void* userdata );

	loaded_callback_func	mCallback;
	S32						mLastUsedDiscard;
	S32						mDesiredDiscard;
	BOOL					mNeedsImageRaw;
	void*					mUserData;
};

class LLTextureBar;

class LLViewerTexture : public LLTexture
{
public:
	enum
	{
		MAX_IMAGE_SIZE_DEFAULT = 1024,
		INVALID_DISCARD_LEVEL = 0x7fff
	};
	enum
	{
		LOCAL_TEXTURE,		
		MEDIA_TEXTURE,
		DYNAMIC_TEXTURE,
		FETCHED_TEXTURE,
		LOD_TEXTURE,
		ATLAS_TEXTURE,
		INVALID_TEXTURE_TYPE
	};

	enum EBoostLevel
	{
		BOOST_NONE 			= 0,
		BOOST_AVATAR_BAKED	= 1,
		BOOST_AVATAR		= 2,
		BOOST_CLOUDS		= 3,
		BOOST_SCULPTED      = 4,
		
		BOOST_HIGH 			= 10,
		BOOST_TERRAIN		= 11, // has to be high priority for minimap / low detail
		BOOST_SELECTED		= 12,
		BOOST_HUD			= 13,
		BOOST_AVATAR_BAKED_SELF	= 14,
		BOOST_ICON			= 15,
		BOOST_UI			= 16,
		BOOST_PREVIEW		= 17,
		BOOST_MAP			= 18,
		BOOST_MAP_VISIBLE	= 19,
		BOOST_AVATAR_SELF	= 20, // needed for baking avatar
		BOOST_MAX_LEVEL,

		//other texture Categories
		LOCAL = BOOST_MAX_LEVEL,
		AVATAR_SCRATCH_TEX,
		DYNAMIC_TEX,
		MEDIA,
		ATLAS,
		OTHER,
		MAX_GL_IMAGE_CATEGORY
	};
	static S32 getTotalNumOfCategories() ;
	static S32 getIndexFromCategory(S32 category) ;
	static S32 getCategoryFromIndex(S32 index) ;

	typedef std::set<LLFace*> ll_face_list_t ;

protected:
	virtual ~LLViewerTexture();
	LOG_CLASS(LLViewerTexture);

public:	
	static void initClass();
	static void cleanupClass();
	static void updateClass(const F32 velocity, const F32 angular_velocity) ;
	
	LLViewerTexture(BOOL usemipmaps = TRUE);
	LLViewerTexture(const LLUUID& id, BOOL usemipmaps) ;
	LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) ;

	virtual S8 getType() const;
	virtual BOOL isMissingAsset()const ;
	virtual void dump();	// debug info to llinfos
	
	/*virtual*/ bool bindDefaultImage(const S32 stage = 0) ;
	/*virtual*/ void forceImmediateUpdate() ;
	
	const LLUUID& getID() const { return mID; }
	
	void setBoostLevel(S32 level);
	S32  getBoostLevel() { return mBoostLevel; }

	void addTextureStats(F32 virtual_size, BOOL needs_gltexture = TRUE) const;
	void resetTextureStats();	

	virtual F32  getMaxVirtualSize() ;

	LLFrameTimer* getLastReferencedTimer() {return &mLastReferencedTimer ;}
	
	S32 getFullWidth() const { return mFullWidth; }
	S32 getFullHeight() const { return mFullHeight; }	
	/*virtual*/ void setKnownDrawSize(S32 width, S32 height);

	virtual void addFace(LLFace* facep) ;
	virtual void removeFace(LLFace* facep) ; 
	const ll_face_list_t* getFaceList() const {return &mFaceList ;}

	void generateGLTexture() ;
	void destroyGLTexture() ;
	
	//---------------------------------------------------------------------------------------------
	//functions to access LLImageGL
	//---------------------------------------------------------------------------------------------
	/*virtual*/S32	       getWidth(S32 discard_level = -1) const;
	/*virtual*/S32	       getHeight(S32 discard_level = -1) const;
	
	BOOL       hasGLTexture() const ;
	LLGLuint   getTexName() const ;		
	BOOL       createGLTexture() ;
	BOOL       createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, BOOL to_create = TRUE, S32 category = LLViewerTexture::OTHER);

	void       setFilteringOption(LLTexUnit::eTextureFilterOptions option);
	void       setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE);
	void       setAddressMode(LLTexUnit::eTextureAddressMode mode);
	BOOL       setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height);
	BOOL       setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height);
	void       setGLTextureCreated (bool initialized);
	void       setCategory(S32 category) ;

	LLTexUnit::eTextureAddressMode getAddressMode(void) const ;
	S32        getMaxDiscardLevel() const;
	S32        getDiscardLevel() const;
	S8		   getComponents() const ;		
	BOOL       getBoundRecently() const;
	S32        getTextureMemory() const ;
	LLGLenum   getPrimaryFormat() const;
	BOOL       getIsAlphaMask() const ;
	LLTexUnit::eTextureType getTarget(void) const ;
	BOOL       getMask(const LLVector2 &tc);
	F32        getTimePassedSinceLastBound();
	BOOL       getMissed() const ;
	BOOL       isJustBound()const ;
	void       forceUpdateBindStats(void) const;

	U32        getTexelsInAtlas() const ;
	U32        getTexelsInGLTexture() const ;
	BOOL       isGLTextureCreated() const ;
	S32        getDiscardLevelInAtlas() const ;
	//---------------------------------------------------------------------------------------------
	//end of functions to access LLImageGL
	//---------------------------------------------------------------------------------------------

	//-----------------
	/*virtual*/ void setActive() ;
	void forceActive() ;
	void setNoDelete() ;
	void dontDiscard() { mDontDiscard = 1; mTextureState = NO_DELETE; }
	BOOL getDontDiscard() const { return mDontDiscard; }
	//-----------------	
	
	BOOL isLargeImage() ;	
	
	void setParcelMedia(LLViewerMediaTexture* media) {mParcelMedia = media;}
	BOOL hasParcelMedia() const { return mParcelMedia != NULL;}
	LLViewerMediaTexture* getParcelMedia() const { return mParcelMedia;}

	/*virtual*/ void updateBindStatsForTester() ;
protected:
	void cleanup() ;
	void init(bool firstinit) ;		

private:
	//note: do not make this function public.
	/*virtual*/ LLImageGL* getGLTexture() const ;
	virtual void switchToCachedImage();

protected:
	LLUUID mID;
	S32 mBoostLevel;				// enum describing priority level
	S32 mFullWidth;
	S32 mFullHeight;
	BOOL  mUseMipMaps ;
	S8  mComponents;
	mutable F32 mMaxVirtualSize;	// The largest virtual size of the image, in pixels - how much data to we need?
	mutable S8  mNeedsGLTexture;
	mutable BOOL mNeedsResetMaxVirtualSize ;
	mutable F32 mAdditionalDecodePriority;  // priority add to mDecodePriority.
	LLFrameTimer mLastReferencedTimer;

	ll_face_list_t mFaceList ; //reverse pointer pointing to the faces using this image as texture

	//GL texture
	LLPointer<LLImageGL> mGLTexturep ;
	S8 mDontDiscard;			// Keep full res version of this image (for UI, etc)

	//do not use LLPointer here.
	LLViewerMediaTexture* mParcelMedia ;

protected:
	typedef enum 
	{
		DELETED = 0,         //removed from memory
		DELETION_CANDIDATE,  //ready to be removed from memory
		INACTIVE,            //not be used for the last certain period (i.e., 30 seconds).
		ACTIVE,              //just being used, can become inactive if not being used for a certain time (10 seconds).
		NO_DELETE = 99       //stay in memory, can not be removed.
	} LLGLTexureState;
	LLGLTexureState  mTextureState ;

public:
	static const U32 sCurrentFileVersion;	
	static S32 sImageCount;
	static S32 sRawCount;
	static S32 sAuxCount;
	static LLTimer sEvaluationTimer;
	static F32 sDesiredDiscardBias;
	static F32 sDesiredDiscardScale;
	static S32 sBoundTextureMemoryInBytes;
	static S32 sTotalTextureMemoryInBytes;
	static S32 sMaxBoundTextureMemInMegaBytes;
	static S32 sMaxTotalTextureMemInMegaBytes;
	static S32 sMaxDesiredTextureMemInBytes ;
	static S8  sCameraMovingDiscardBias;
	static S32 sMaxSculptRez ;
	static S32 sMinLargeImageSize ;
	static S32 sMaxSmallImageSize ;
	static BOOL sFreezeImageScalingDown ;//do not scale down image res if set.
	static F32  sCurrentTime ;
	static BOOL sUseTextureAtlas ;

	static LLPointer<LLViewerTexture> sNullImagep; // Null texture for non-textured objects.
};


//
//textures are managed in gTextureList.
//raw image data is fetched from remote or local cache
//but the raw image this texture pointing to is fixed.
//
class LLViewerFetchedTexture : public LLViewerTexture
{
	friend class LLTextureBar; // debug info only
	friend class LLTextureView; // debug info only

protected:
	/*virtual*/ ~LLViewerFetchedTexture();
public:
	LLViewerFetchedTexture(const LLUUID& id, const LLHost& host = LLHost::invalid, BOOL usemipmaps = TRUE);
	LLViewerFetchedTexture(const LLImageRaw* raw, BOOL usemipmaps);
	LLViewerFetchedTexture(const std::string& url, const LLUUID& id, BOOL usemipmaps = TRUE);

public:
	static F32 maxDecodePriority();
	
	struct Compare
	{
		// lhs < rhs
		bool operator()(const LLPointer<LLViewerFetchedTexture> &lhs, const LLPointer<LLViewerFetchedTexture> &rhs) const
		{
			const LLViewerFetchedTexture* lhsp = (const LLViewerFetchedTexture*)lhs;
			const LLViewerFetchedTexture* rhsp = (const LLViewerFetchedTexture*)rhs;
			// greater priority is "less"
			const F32 lpriority = lhsp->getDecodePriority();
			const F32 rpriority = rhsp->getDecodePriority();
			if (lpriority > rpriority) // higher priority
				return true;
			if (lpriority < rpriority)
				return false;
			return lhsp < rhsp;
		}
	};

public:
	/*virtual*/ S8 getType() const ;
	/*virtual*/ void forceImmediateUpdate() ;
	/*virtual*/ void dump() ;

	// Set callbacks to get called when the image gets updated with higher 
	// resolution versions.
	void setLoadedCallback(loaded_callback_func cb,
						   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux,
						   void* userdata);
	bool hasCallbacks() { return mLoadedCallbackList.empty() ? false : true; }	
	bool doLoadedCallbacks();

	void addToCreateTexture();

	 // ONLY call from LLViewerTextureList
	BOOL createTexture(S32 usename = 0);
	void destroyTexture() ;	
	
	virtual void processTextureStats() ;
	F32  calcDecodePriority() ;

	BOOL needsAux() const { return mNeedsAux; }

	// Host we think might have this image, used for baked av textures.
	void setTargetHost(LLHost host)			{ mTargetHost = host; }
	LLHost getTargetHost() const			{ return mTargetHost; }
	
	// Set the decode priority for this image...
	// DON'T CALL THIS UNLESS YOU KNOW WHAT YOU'RE DOING, it can mess up
	// the priority list, and cause horrible things to happen.
	void setDecodePriority(F32 priority = -1.0f);
	F32 getDecodePriority() const { return mDecodePriority; };

	void setAdditionalDecodePriority(F32 priority) ;
	
	void updateVirtualSize() ;

	// setDesiredDiscardLevel is only used by LLViewerTextureList
	void setDesiredDiscardLevel(S32 discard) { mDesiredDiscardLevel = discard; }
	S32  getDesiredDiscardLevel()			 { return mDesiredDiscardLevel; }
	void setMinDiscardLevel(S32 discard) 	{ mMinDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel,(S8)discard); }

	bool updateFetch();
	
	// Override the computation of discard levels if we know the exact output
	// size of the image.  Used for UI textures to not decode, even if we have
	// more data.
	/*virtual*/ void setKnownDrawSize(S32 width, S32 height);

	void setIsMissingAsset();
	/*virtual*/ BOOL isMissingAsset()	const		{ return mIsMissingAsset; }

	// returns dimensions of original image for local files (before power of two scaling)
	// and returns 0 for all asset system images
	S32 getOriginalWidth() { return mOrigWidth; }
	S32 getOriginalHeight() { return mOrigHeight; }

	BOOL isInImageList() const {return mInImageList ;}
	void setInImageList(BOOL flag) {mInImageList = flag ;}

	LLFrameTimer* getLastPacketTimer() {return &mLastPacketTimer;}

	U32 getFetchPriority() const { return mFetchPriority ;}
	F32 getDownloadProgress() const {return mDownloadProgress ;}

	LLImageRaw* reloadRawImage(S8 discard_level) ;
	void destroyRawImage();

	const std::string& getUrl() const {return mUrl;}
	//---------------
	BOOL isDeleted() ;
	BOOL isInactive() ;
	BOOL isDeletionCandidate();
	void setDeletionCandidate() ;
	void setInactive() ;
	BOOL getUseDiscard() const { return mUseMipMaps && !mDontDiscard; }	
	//---------------

	void setForSculpt();
	BOOL forSculpt() const {return mForSculpt;}
	BOOL isForSculptOnly() const;

	//raw image management	
	void        checkCachedRawSculptImage() ;
	LLImageRaw* getRawImage()const { return mRawImage ;}
	S32         getRawImageLevel() const {return mRawDiscardLevel;}
	LLImageRaw* getCachedRawImage() const { return mCachedRawImage ;}
	S32         getCachedRawImageLevel() const {return mCachedRawDiscardLevel;}
	BOOL        isCachedRawImageReady() const {return mCachedRawImageReady ;}
	BOOL        isRawImageValid()const { return mIsRawImageValid ; }	
	void        forceToSaveRawImage(S32 desired_discard = 0) ;
	void        destroySavedRawImage() ;
	LLImageRaw* getSavedRawImage() ;
	BOOL        hasSavedRawImage() const ;
	F32         getElapsedLastReferencedSavedRawImageTime() const ;
	BOOL		isFullyLoaded() const;

protected:
	/*virtual*/ void switchToCachedImage();

private:
	void init(bool firstinit) ;
	void cleanup() ;

	void saveRawImage() ;
	BOOL forceFetch() ;
	void setCachedRawImage() ;
	BOOL keepReuestedDiscardLevel();

	//for atlas
	void resetFaceAtlas() ;
	void invalidateAtlas(BOOL rebuild_geom) ;
	BOOL insertToAtlas() ;

private:
	BOOL  mFullyLoaded;

protected:		
	std::string mLocalFileName;

	S32 mOrigWidth;
	S32 mOrigHeight;

	// Override the computation of discard levels if we know the exact output size of the image.
	// Used for UI textures to not decode, even if we have more data.
	S32 mKnownDrawWidth;
	S32	mKnownDrawHeight;
	BOOL mKnownDrawSizeChanged ;
	std::string mUrl;
	
	S32 mRequestedDiscardLevel;
	F32 mRequestedDownloadPriority;
	S32 mFetchState;
	U32 mFetchPriority;
	F32 mDownloadProgress;
	F32 mFetchDeltaTime;
	F32 mRequestDeltaTime;
	F32 mDecodePriority;			// The priority for decoding this image.
	S32	mMinDiscardLevel;
	S8  mDesiredDiscardLevel;			// The discard level we'd LIKE to have - if we have it and there's space	
	S8  mMinDesiredDiscardLevel;	// The minimum discard level we'd like to have

	S8  mNeedsAux;					// We need to decode the auxiliary channels
	S8  mDecodingAux;				// Are we decoding high components
	S8  mIsRawImageValid;
	S8  mHasFetcher;				// We've made a fecth request
	S8  mIsFetching;				// Fetch request is active
	
	mutable S8 mIsMissingAsset;		// True if we know that there is no image asset with this image id in the database.		

	typedef std::list<LLLoadedCallbackEntry*> callback_list_t;
	S8              mLoadedCallbackDesiredDiscardLevel;
	callback_list_t mLoadedCallbackList;

	LLPointer<LLImageRaw> mRawImage;
	S32 mRawDiscardLevel;

	// Used ONLY for cloth meshes right now.  Make SURE you know what you're 
	// doing if you use it for anything else! - djs
	LLPointer<LLImageRaw> mAuxRawImage;

	//keep a copy of mRawImage for some special purposes
	//when mForceToSaveRawImage is set.
	BOOL mForceToSaveRawImage ;
	LLPointer<LLImageRaw> mSavedRawImage;
	S32 mSavedRawDiscardLevel;
	S32 mDesiredSavedRawDiscardLevel;
	F32 mLastReferencedSavedRawImageTime ;

	//a small version of the copy of the raw image (<= 64 * 64)
	LLPointer<LLImageRaw> mCachedRawImage;
	S32 mCachedRawDiscardLevel;
	BOOL mCachedRawImageReady; //the rez of the mCachedRawImage reaches the upper limit.	

	LLHost mTargetHost;	// if LLHost::invalid, just request from agent's simulator

	// Timers
	LLFrameTimer mLastPacketTimer;		// Time since last packet.

	BOOL  mInImageList;				// TRUE if image is in list (in which case don't reset priority!)
	BOOL  mNeedsCreateTexture;	

	BOOL   mForSculpt ; //a flag if the texture is used as sculpt data.
	BOOL   mIsFetched ; //is loaded from remote or from cache, not generated locally.

public:
	static LLPointer<LLViewerFetchedTexture> sMissingAssetImagep;	// Texture to show for an image asset that is not in the database
	static LLPointer<LLViewerFetchedTexture> sWhiteImagep;	// Texture to show NOTHING (whiteness)
	static LLPointer<LLViewerFetchedTexture> sDefaultImagep; // "Default" texture for error cases, the only case of fetched texture which is generated in local.
	static LLPointer<LLViewerFetchedTexture> sSmokeImagep; // Old "Default" translucent texture
};

//
//the image data is fetched from remote or from local cache
//the resolution of the texture is adjustable: depends on the view-dependent parameters.
//
class LLViewerLODTexture : public LLViewerFetchedTexture
{
protected:
	/*virtual*/ ~LLViewerLODTexture(){}

public:
	LLViewerLODTexture(const LLUUID& id, const LLHost& host = LLHost::invalid, BOOL usemipmaps = TRUE);
	LLViewerLODTexture(const std::string& url, const LLUUID& id, BOOL usemipmaps = TRUE);

	/*virtual*/ S8 getType() const;
	// Process image stats to determine priority/quality requirements.
	/*virtual*/ void processTextureStats();
	BOOL isUpdateFrozen() ;

private:
	void init(bool firstinit) ;
	void scaleDown() ;		

private:
	
	F32 mTexelsPerImage;			// Texels per image.
	F32 mDiscardVirtualSize;		// Virtual size used to calculate desired discard	
	F32 mCalculatedDiscardLevel;    // Last calculated discard level
};

//
//the image data is fetched from the media pipeline periodically
//the resolution of the texture is also adjusted by the media pipeline
//
class LLViewerMediaTexture : public LLViewerTexture
{
protected:
	/*virtual*/ ~LLViewerMediaTexture() ;

public:
	LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;

	/*virtual*/ S8 getType() const;
	void reinit(BOOL usemipmaps = TRUE);	

	BOOL  getUseMipMaps() {return mUseMipMaps ; }
	void  setUseMipMaps(BOOL mipmap) ;	
	
	void setPlaying(BOOL playing) ;
	BOOL isPlaying() const {return mIsPlaying;}
	void setMediaImpl() ;

	void initVirtualSize() ;	
	void invalidateMediaImpl() ;

	void addMediaToFace(LLFace* facep) ;
	void removeMediaFromFace(LLFace* facep) ;

	/*virtual*/ void addFace(LLFace* facep) ;
	/*virtual*/ void removeFace(LLFace* facep) ; 

	/*virtual*/ F32  getMaxVirtualSize() ;
private:
	void switchTexture(LLFace* facep) ;
	BOOL findFaces() ;
	void stopPlaying() ;

private:
	//
	//an instant list, recording all faces referencing or can reference to this media texture.
	//NOTE: it is NOT thread safe. 
	//
	std::list< LLFace* > mMediaFaceList ; 

	//an instant list keeping all textures which are replaced by the current media texture,
	//is only used to avoid the removal of those textures from memory.
	std::list< LLPointer<LLViewerTexture> > mTextureList ;

	LLViewerMediaImpl* mMediaImplp ;	
	BOOL mIsPlaying ;
	U32  mUpdateVirtualSizeTime ;

public:
	static void updateClass() ;
	static void cleanup() ;	

	static LLViewerMediaTexture* findMediaTexture(const LLUUID& media_id) ;
	static void removeMediaImplFromTexture(const LLUUID& media_id) ;

private:
	typedef std::map< LLUUID, LLPointer<LLViewerMediaTexture> > media_map_t ;
	static media_map_t sMediaMap ;	
};

//just an interface class, do not create instance from this class.
class LLViewerTextureManager
{
private:
	//make the constructor private to preclude creating instances from this class.
	LLViewerTextureManager(){}

public:
    //texture pipeline tester
	static LLTexturePipelineTester* sTesterp ;

	//returns NULL if tex is not a LLViewerFetchedTexture nor derived from LLViewerFetchedTexture.
	static LLViewerFetchedTexture*    staticCastToFetchedTexture(LLTexture* tex, BOOL report_error = FALSE) ;

	//
	//"find-texture" just check if the texture exists, if yes, return it, otherwise return null.
	//
	static LLViewerTexture*           findTexture(const LLUUID& id) ;
	static LLViewerMediaTexture*      findMediaTexture(const LLUUID& id) ;
	
	static LLViewerMediaTexture*      createMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;

	//
	//"get-texture" will create a new texture if the texture does not exist.
	//
	static LLViewerMediaTexture*      getMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;
	
	static LLPointer<LLViewerTexture> getLocalTexture(BOOL usemipmaps = TRUE, BOOL generate_gl_tex = TRUE);
	static LLPointer<LLViewerTexture> getLocalTexture(const LLUUID& id, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;

	static LLViewerFetchedTexture* getFetchedTexture(const LLUUID &image_id,									 
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLViewerTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 LLHost request_from_host = LLHost()
									 );
	
	static LLViewerFetchedTexture* getFetchedTextureFromFile(const std::string& filename,									 
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLViewerTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );

	static LLViewerFetchedTexture* getFetchedTextureFromUrl(const std::string& url,									 
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLViewerTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );

	static LLViewerFetchedTexture* getFetchedTextureFromHost(const LLUUID& image_id, LLHost host) ;

	static void init() ;
	static void cleanup() ;
};
//
//this class is used for test/debug only
//it tracks the activities of the texture pipeline
//records them, and outputs them to log files
//
class LLTexturePipelineTester : public LLMetricPerformanceTester
{
	enum
	{
		MIN_LARGE_IMAGE_AREA = 262144  //512 * 512
	};
public:
	LLTexturePipelineTester() ;
	~LLTexturePipelineTester() ;

	void update();		
	void updateTextureBindingStats(const LLViewerTexture* imagep) ;
	void updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, BOOL from_cache) ;
	void updateGrayTextureBinding() ;
	void setStablizingTime() ;

	/*virtual*/ void analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) ;

private:
	void reset() ;
	void updateStablizingTime() ;

	/*virtual*/ void outputTestRecord(LLSD* sd) ;

private:
	BOOL mPause ;
private:
	BOOL mUsingDefaultTexture;            //if set, some textures are still gray.

	U32 mTotalBytesUsed ;                     //total bytes of textures bound/used for the current frame.
	U32 mTotalBytesUsedForLargeImage ;        //total bytes of textures bound/used for the current frame for images larger than 256 * 256.
	U32 mLastTotalBytesUsed ;                 //total bytes of textures bound/used for the previous frame.
	U32 mLastTotalBytesUsedForLargeImage ;    //total bytes of textures bound/used for the previous frame for images larger than 256 * 256.
		
	//
	//data size
	//
	U32 mTotalBytesLoaded ;               //total bytes fetched by texture pipeline
	U32 mTotalBytesLoadedFromCache ;      //total bytes fetched by texture pipeline from local cache	
	U32 mTotalBytesLoadedForLargeImage ;  //total bytes fetched by texture pipeline for images larger than 256 * 256. 
	U32 mTotalBytesLoadedForSculpties ;   //total bytes fetched by texture pipeline for sculpties

	//
	//time
	//NOTE: the error tolerances of the following timers is one frame time.
	//
	F32 mStartFetchingTime ;
	F32 mTotalGrayTime ;                  //total loading time when no gray textures.
	F32 mTotalStablizingTime ;            //total stablizing time when texture memory overflows
	F32 mStartTimeLoadingSculpties ;      //the start moment of loading sculpty images.
	F32 mEndTimeLoadingSculpties ;        //the end moment of loading sculpty images.
	F32 mStartStablizingTime ;
	F32 mEndStablizingTime ;

private:
	//
	//The following members are used for performance analyzing
	//
	class LLTextureTestSession : public LLTestSession
	{
	public:
		LLTextureTestSession() ;
		/*virtual*/ ~LLTextureTestSession() ;

		void reset() ;

		F32 mTotalFetchingTime ;
		F32 mTotalGrayTime ;
		F32 mTotalStablizingTime ;
		F32 mStartTimeLoadingSculpties ; 
		F32 mTotalTimeLoadingSculpties ;

		S32 mTotalBytesLoaded ; 
		S32 mTotalBytesLoadedFromCache ;
		S32 mTotalBytesLoadedForLargeImage ;
		S32 mTotalBytesLoadedForSculpties ; 

		typedef struct _texture_instant_preformance_t
		{
			S32 mAverageBytesUsedPerSecond ;         
			S32 mAverageBytesUsedForLargeImagePerSecond ;
			F32 mAveragePercentageBytesUsedPerSecond ;
			F32 mTime ;
		}texture_instant_preformance_t ;
		std::vector<texture_instant_preformance_t> mInstantPerformanceList ;
		S32 mInstantPerformanceListCounter ;
	};

	/*virtual*/ LLMetricPerformanceTester::LLTestSession* loadTestSession(LLSD* log) ;
	/*virtual*/ void compareTestSessions(std::ofstream* os) ;
};

#endif
