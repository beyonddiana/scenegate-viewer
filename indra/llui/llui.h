/** 
 * @file llui.h
 * @brief GL function declarations and other general static UI services.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// All immediate-mode gl drawing should happen here.

#ifndef LL_LLUI_H
#define LL_LLUI_H

#include "llpointer.h"		// LLPointer<>
#include "llrect.h"
#include "llcontrol.h"
#include "llcoord.h"
#include "lluiimage.h"		// *TODO: break this dependency, need to add #include "lluiimage.h" to all widgets that hold an Optional<LLUIImage*> in their paramblocks
#include "llinitparam.h"
#include "llregistry.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include <boost/signals2.hpp>
#include "lllazyvalue.h"
#include "llhandle.h"		// *TODO: remove this dependency, added as a 
							// convenience when LLHandle moved to llhandle.h
#include "llframetimer.h"

// LLUIFactory
#include "llsd.h"

// for initparam specialization
#include "llfontgl.h"

class LLColor4; 
class LLHtmlHelp;
class LLVector3;
class LLVector2;
class LLUIImage;
class LLUUID;
class LLWindow;
class LLView;

// UI colors
extern const LLColor4 UI_VERTEX_COLOR;
void make_ui_sound(const char* name);

BOOL ui_point_in_rect(S32 x, S32 y, S32 left, S32 top, S32 right, S32 bottom);
void gl_state_for_2d(S32 width, S32 height);

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2);
void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2, const LLColor4 &color );
void gl_triangle_2d(S32 x1, S32 y1, S32 x2, S32 y2, S32 x3, S32 y3, const LLColor4& color, BOOL filled);
void gl_rect_2d_simple( S32 width, S32 height );

void gl_draw_x(const LLRect& rect, const LLColor4& color);

void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, BOOL filled = TRUE );
void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, const LLColor4& color, BOOL filled = TRUE );
void gl_rect_2d_checkerboard(const LLRect& rect);

void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines);

void gl_circle_2d(F32 x, F32 y, F32 radius, S32 steps, BOOL filled);
void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle);
void gl_deep_circle( F32 radius, F32 depth );
void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center );
void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac);
void gl_washer_2d(F32 outer_radius, F32 inner_radius, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_segment_2d(F32 outer_radius, F32 inner_radius, F32 start_radians, F32 end_radians, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_spokes_2d(F32 outer_radius, F32 inner_radius, S32 count, const LLColor4& inner_color, const LLColor4& outer_color);

void gl_draw_image(S32 x, S32 y, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees,LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f), const LLRectf& scale_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
// Flip vertical, used for LLFloaterHTML
void gl_draw_scaled_image_inverted(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));

void gl_rect_2d_xor(S32 left, S32 top, S32 right, S32 bottom);
void gl_stippled_line_3d( const LLVector3& start, const LLVector3& end, const LLColor4& color, F32 phase = 0.f ); 

void gl_rect_2d_simple_tex( S32 width, S32 height );

// segmented rectangles

/*
   TL |______TOP_________| TR 
     /|                  |\  
   _/_|__________________|_\_
   L| |    MIDDLE        | |R
   _|_|__________________|_|_
    \ |    BOTTOM        | /  
   BL\|__________________|/ BR
      |                  |    
*/

typedef enum e_rounded_edge
{
	ROUNDED_RECT_LEFT	= 0x1, 
	ROUNDED_RECT_TOP	= 0x2, 
	ROUNDED_RECT_RIGHT	= 0x4, 
	ROUNDED_RECT_BOTTOM	= 0x8,
	ROUNDED_RECT_ALL	= 0xf
}ERoundedEdge;


void gl_segmented_rect_2d_tex(const S32 left, const S32 top, const S32 right, const S32 bottom, const S32 texture_width, const S32 texture_height, const S32 border_size, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_2d_fragment_tex(const S32 left, const S32 top, const S32 right, const S32 bottom, const S32 texture_width, const S32 texture_height, const S32 border_size, const F32 start_fragment, const F32 end_fragment, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_3d_tex(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec, U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_3d_tex_top(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec);

inline void gl_rect_2d( const LLRect& rect, BOOL filled )
{
	gl_rect_2d( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, filled );
}

inline void gl_rect_2d_offset_local( const LLRect& rect, S32 pixel_offset, BOOL filled)
{
	gl_rect_2d_offset_local( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, pixel_offset, filled );
}

class LLImageProviderInterface;

typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);

class LLUI
{
	LOG_CLASS(LLUI);
public:
	//
	// Methods
	//
	typedef std::map<std::string, LLControlGroup*> settings_map_t;
	static void initClass(const settings_map_t& settings,
						  LLImageProviderInterface* image_provider,
						  LLUIAudioCallback audio_callback = NULL,
						  const LLVector2 *scale_factor = NULL,
						  const std::string& language = LLStringUtil::null);
	static void cleanupClass();

	static void pushMatrix();
	static void popMatrix();
	static void loadIdentity();
	static void translate(F32 x, F32 y, F32 z = 0.0f);

	static LLRect	sDirtyRect;
	static BOOL		sDirty;
	static void		dirtyRect(LLRect rect);

	// Return the ISO639 language name ("en", "ko", etc.) for the viewer UI.
	// http://www.loc.gov/standards/iso639-2/php/code_list.php
	static std::string getLanguage();
	
	static void setupPaths();
	static const std::vector<std::string>& getXUIPaths() { return sXUIPaths; }
	static std::string getSkinPath() { return sXUIPaths.front(); }
	static std::string getLocalizedSkinPath() { return sXUIPaths.back(); }  //all files may not exist at the localized path

	//helper functions (should probably move free standing rendering helper functions here)
	static LLView* getRootView() { return sRootView; }
	static void setRootView(LLView* view) { sRootView = view; }
	static std::string locateSkin(const std::string& filename);
	static void setMousePositionScreen(S32 x, S32 y);
	static void setMousePositionLocal(const LLView* viewp, S32 x, S32 y);
	static void getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y);
	static void setScaleFactor(const LLVector2& scale_factor);
	static void setLineWidth(F32 width);
	static LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id);
	static LLPointer<LLUIImage> getUIImage(const std::string& name);
	static LLVector2 getWindowSize();
	static void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	static void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	static void screenRectToGL(const LLRect& screen, LLRect *gl);
	static void glRectToScreen(const LLRect& gl, LLRect *screen);
	static void setHtmlHelp(LLHtmlHelp* html_help);
	// Returns the control group containing the control name, or the default group
	static LLControlGroup& getControlControlGroup (const std::string& controlname);
	static F32 getMouseIdleTime() { return sMouseIdleTimer.getElapsedTimeF32(); }
	static void resetMouseIdleTimer() { sMouseIdleTimer.reset(); }
	static LLWindow* getWindow() { return sWindow; }

	// Ensures view does not overlap mouse cursor, but is inside
	// the view's parent rectangle.  Used for tooltips, inspectors.
	// Optionally override the view's default X/Y, which are relative to the
	// view's parent.
	static void positionViewNearMouse(LLView* view,	S32 spawn_x = S32_MAX, S32 spawn_y = S32_MAX);

	//
	// Data
	//
	static settings_map_t sSettingGroups;
	static LLUIAudioCallback sAudioCallback;
	static LLVector2		sGLScaleFactor;
	static LLWindow*		sWindow;
	static LLHtmlHelp*		sHtmlHelp;
	static LLView*			sRootView;
private:
	static LLImageProviderInterface* sImageProvider;
	static std::vector<std::string> sXUIPaths;
	static LLFrameTimer		sMouseIdleTimer;
};


// Moved LLLocalClipRect to lllocalcliprect.h

// Moved all LLHandle-related code to llhandle.h

//RN: maybe this needs to moved elsewhere?
class LLImageProviderInterface
{
protected:
	LLImageProviderInterface() {};
	virtual ~LLImageProviderInterface() {};
public:
	virtual LLPointer<LLUIImage> getUIImage(const std::string& name) = 0;
	virtual LLPointer<LLUIImage> getUIImageByID(const LLUUID& id) = 0;
	virtual void cleanUp() = 0;
};

class LLCallbackRegistry
{
public:
	typedef boost::signals2::signal<void()> callback_signal_t;
	
	void registerCallback(const callback_signal_t::slot_type& slot)
	{
		mCallbacks.connect(slot);
	}

	void fireCallbacks()
	{
		mCallbacks();
	}

private:
	callback_signal_t mCallbacks;
};

class LLInitClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLInitClassList>
{
	friend class LLSingleton<LLInitClassList>;
private:
	LLInitClassList() {}
};

class LLDestroyClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLDestroyClassList>
{
	friend class LLSingleton<LLDestroyClassList>;
private:
	LLDestroyClassList() {}
};

template<typename T>
class LLRegisterWith
{
public:
	LLRegisterWith(boost::function<void ()> func)
	{
		T::instance().registerCallback(func);
	}

	// this avoids a MSVC bug where non-referenced static members are "optimized" away
	// even if their constructors have side effects
	void reference()
	{
		S32 dummy;
		dummy = 0;
	}
};

template<typename T>
class LLInitClass
{
public:
	LLInitClass() { sRegister.reference(); }

	static LLRegisterWith<LLInitClassList> sRegister;
private:

	static void initClass()
	{
		llerrs << "No static initClass() method defined for " << typeid(T).name() << llendl;
	}
};

template<typename T>
class LLDestroyClass
{
public:
	LLDestroyClass() { sRegister.reference(); }

	static LLRegisterWith<LLDestroyClassList> sRegister;
private:

	static void destroyClass()
	{
		llerrs << "No static destroyClass() method defined for " << typeid(T).name() << llendl;
	}
};

template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);

// useful parameter blocks
struct TimeIntervalParam : public LLInitParam::Choice<TimeIntervalParam>
{
	Alternative<F32>		seconds;
	Alternative<S32>		frames;
	TimeIntervalParam()
	:	seconds("seconds"),
		frames("frames")
	{}
};

template <class T>
class LLUICachedControl : public LLCachedControl<T>
{
public:
	// This constructor will declare a control if it doesn't exist in the contol group
	LLUICachedControl(const std::string& name,
					  const T& default_value,
					  const std::string& comment = "Declared In Code")
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name, default_value, comment)
	{}

	// This constructor will signal an error if the control doesn't exist in the control group
	LLUICachedControl(const std::string& name)
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name)
	{}
};

namespace LLInitParam
{
	template<>
	class TypedParam<LLRect> 
	:	public BlockValue<LLRect>
	{
        typedef BlockValue<LLRect> super_t;
	public:
		Optional<S32>	left,
						top,
						right,
						bottom,
						width,
						height;

		TypedParam(BlockDescriptor& descriptor, const char* name, const LLRect& value, ParamDescriptor::validation_func_t func, S32 min_count, S32 max_count);

		LLRect getValueFromBlock() const;
	};

	template<>
	struct TypeValues<LLUIColor> : public TypeValuesHelper<LLUIColor>
	{
		static void declareValues();
	};

	template<>
	class TypedParam<LLUIColor> 
	:	public BlockValue<LLUIColor>
	{
        typedef BlockValue<LLUIColor> super_t;
	public:
		Optional<F32>	red,
						green,
						blue,
						alpha;
		Optional<std::string> control;

		TypedParam(BlockDescriptor& descriptor, const char* name, const LLUIColor& value, ParamDescriptor::validation_func_t func, S32 min_count, S32 max_count);
		LLUIColor getValueFromBlock() const;
	};

	template<>
	class TypedParam<const LLFontGL*> 
	:	public BlockValue<const LLFontGL*>
	{
        typedef BlockValue<const LLFontGL*> super_t;
	public:
		Optional<std::string>	name,
								size,
								style;

		TypedParam(BlockDescriptor& descriptor, const char* name, const LLFontGL* const value, ParamDescriptor::validation_func_t func, S32 min_count, S32 max_count);
		const LLFontGL* getValueFromBlock() const;
	};

	template<>
	struct TypeValues<LLFontGL::HAlign> : public TypeValuesHelper<LLFontGL::HAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::VAlign> : public TypeValuesHelper<LLFontGL::VAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::ShadowType> : public TypeValuesHelper<LLFontGL::ShadowType>
	{
		static void declareValues();
	};

	template<>
	class TypedParam<LLCoordGL>
	:	public BlockValue<LLCoordGL>
	{
		typedef BlockValue<LLCoordGL> super_t;
	public:
		Optional<S32>	x,
						y;

		TypedParam(BlockDescriptor& descriptor, const char* name, LLCoordGL value, ParamDescriptor::validation_func_t func, S32 min_count, S32 max_count);
		LLCoordGL getValueFromBlock() const;
	};
}

#endif
