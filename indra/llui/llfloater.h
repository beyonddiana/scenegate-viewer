/** 
 * @file llfloater.h
 * @brief LLFloater base class
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

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.


#ifndef LL_FLOATER_H
#define LL_FLOATER_H

#include "llpanel.h"
#include "lluuid.h"
#include "llnotifications.h"
#include <set>

class LLDragHandle;
class LLResizeHandle;
class LLResizeBar;
class LLButton;
class LLMultiFloater;
class LLFloater;


const BOOL RESIZE_YES = TRUE;
const BOOL RESIZE_NO = FALSE;

const BOOL DRAG_ON_TOP = FALSE;
const BOOL DRAG_ON_LEFT = TRUE;

const BOOL MINIMIZE_YES = TRUE;
const BOOL MINIMIZE_NO = FALSE;

const BOOL CLOSE_YES = TRUE;
const BOOL CLOSE_NO = FALSE;

const BOOL ADJUST_VERTICAL_YES = TRUE;
const BOOL ADJUST_VERTICAL_NO = FALSE;

// associates a given notification instance with a particular floater
class LLFloaterNotificationContext : 
	public LLNotificationContext
{
public:
	LLFloaterNotificationContext(LLHandle<LLFloater> floater_handle) :
		mFloaterHandle(floater_handle)
	{}

	LLFloater* getFloater() { return mFloaterHandle.get(); }
private:
	LLHandle<LLFloater> mFloaterHandle;
};

class LLFloater : public LLPanel
{
friend class LLFloaterView;
friend class LLFloaterReg;
friend class LLMultiFloater;
public:
	struct KeyCompare
	{
		static bool compare(const LLSD& a, const LLSD& b);
		static bool equate(const LLSD& a, const LLSD& b);
		bool operator()(const LLSD& a, const LLSD& b) const
		{
			return compare(a, b);
		}
	};
	
	enum EFloaterButtons
	{
		BUTTON_CLOSE,
		BUTTON_RESTORE,
		BUTTON_MINIMIZE,
		BUTTON_TEAR_OFF,
		BUTTON_EDIT,
		BUTTON_DOCK,
		BUTTON_UNDOCK,
		BUTTON_COUNT
	};
	
	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> open_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> open_signal_t;
	
	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param, bool app_quitting)> close_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param, bool app_quitting)> close_signal_t;

	struct OpenCallbackParam : public LLInitParam::Block<OpenCallbackParam, CallbackParam >
	{
		Optional<open_callback_t> function;
	};

	struct CloseCallbackParam : public LLInitParam::Block<CloseCallbackParam, CallbackParam >
	{
		Optional<close_callback_t> function;
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<std::string>	title,
								short_title;
		
		Optional<bool>			single_instance,
								auto_tile,
								can_resize,
								can_minimize,
								can_close,
								can_drag_on_left,
								can_tear_off,
								save_rect,
								save_visibility,
								can_dock;
		
		Optional<OpenCallbackParam> open_callback;
		Optional<CloseCallbackParam> close_callback;
		
		Params();
	};
	
	// use this to avoid creating your own default LLFloater::Param instance
	static const Params& getDefaultParams();

	LLFloater(const LLSD& key = LLSD(), const Params& params = getDefaultParams());

	virtual ~LLFloater();

	// Don't export top/left for rect, only height/width
	static void setupParamsForExport(Params& p, LLView* parent);

	void initFromParams(const LLFloater::Params& p);
	void initFloaterXML(LLXMLNodePtr node, LLView *parent, BOOL open_floater = TRUE, LLXMLNodePtr output_node = NULL);

	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);
	/*virtual*/ BOOL canSnapTo(const LLView* other_view);
	/*virtual*/ void setSnappedTo(const LLView* snap_view);
	/*virtual*/ void setFocus( BOOL b );
	/*virtual*/ void setIsChrome(BOOL is_chrome);
	/*virtual*/ void setRect(const LLRect &rect);

	void 			initFloater();

	void			openFloater(const LLSD& key = LLSD());

	// If allowed, close the floater cleanly, releasing focus.
	// app_quitting is passed to onClose() below.
	void			closeFloater(bool app_quitting = false);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	// Release keyboard and mouse focus
	void			releaseFocus();

	// moves to center of gFloaterView
	void			center();

	LLMultiFloater* getHost();

	void			applyTitle();
	const std::string&	getCurrentTitle() const;
	void			setTitle( const std::string& title);
	std::string		getTitle();
	void			setShortTitle( const std::string& short_title );
	std::string		getShortTitle();
	void			setTitleVisible(bool visible);
	virtual void	setMinimized(BOOL b);
	void			moveResizeHandlesToFront();
	void			addDependentFloater(LLFloater* dependent, BOOL reposition = TRUE);
	void			addDependentFloater(LLHandle<LLFloater> dependent_handle, BOOL reposition = TRUE);
	LLFloater*		getDependee() { return (LLFloater*)mDependeeHandle.get(); }
	void		removeDependentFloater(LLFloater* dependent);
	BOOL			isMinimized()					{ return mMinimized; }
	BOOL			isFrontmost();
	BOOL			isDependent()					{ return !mDependeeHandle.isDead(); }
	void			setCanMinimize(BOOL can_minimize);
	void			setCanClose(BOOL can_close);
	void			setCanTearOff(BOOL can_tear_off);
	virtual void	setCanResize(BOOL can_resize);
	void			setCanDrag(BOOL can_drag);
	void			setHost(LLMultiFloater* host);
	BOOL			isResizable() const				{ return mResizable; }
	void			setResizeLimits( S32 min_width, S32 min_height );
	void			getResizeLimits( S32* min_width, S32* min_height ) { *min_width = mMinWidth; *min_height = mMinHeight; }

	bool			isMinimizeable() const{ return mCanMinimize; }
	bool			isCloseable() const{ return mCanClose; }
	bool			isDragOnLeft() const{ return mDragOnLeft; }
	S32				getMinWidth() const{ return mMinWidth; }
	S32				getMinHeight() const{ return mMinHeight; }

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	virtual void	draw();

	virtual void	onOpen(const LLSD& key) {}

	// Call destroy() to free memory, or setVisible(FALSE) to keep it
	// If app_quitting, you might not want to save your visibility.
	// Defaults to destroy().
	virtual void	onClose(bool app_quitting) { destroy(); }

	// This cannot be "const" until all derived floater canClose()
	// methods are const as well.  JC
	virtual BOOL	canClose() { return TRUE; }

	virtual void	setVisible(BOOL visible);
	virtual void	onVisibilityChange ( BOOL curVisibilityIn );
	
	void			setFrontmost(BOOL take_focus = TRUE);
	
	// Defaults to false.
	virtual BOOL	canSaveAs() const { return FALSE; }

	virtual void	saveAs() {}

	void			setSnapTarget(LLHandle<LLFloater> handle) { mSnappedTo = handle; }
	void			clearSnapTarget() { mSnappedTo.markDead(); }
	LLHandle<LLFloater>	getSnapTarget() const { return mSnappedTo; }

	LLHandle<LLFloater> getHandle() const { return mHandle; }
	const LLSD& 	getKey() { return mKey; }
	BOOL		 	matchesKey(const LLSD& key) { return mSingleInstance || KeyCompare::equate(key, mKey); }

	bool            isDockable() const { return mCanDock; }
	void            setCanDock(bool b);

	bool            isDocked() const { return mDocked; }
	virtual void    setDocked(bool docked, bool pop_on_undock = true);

	// Return a closeable floater, if any, given the current focus.
	static LLFloater* getClosableFloaterFromFocus(); 

	// Close the floater returned by getClosableFloaterFromFocus() and 
	// handle refocusing.
	static void		closeFocusedFloater();

	LLNotification::Params contextualNotification(const std::string& name) 
	{ 
	    return LLNotification::Params(name).context(mNotificationContext); 
	}

	static void		onClickClose(LLFloater* floater);
	static void		onClickMinimize(LLFloater* floater);
	static void		onClickTearOff(LLFloater* floater);
	static void		onClickEdit(LLFloater* floater);
	static void     onClickDock(LLFloater* floater);

	static void		setFloaterHost(LLMultiFloater* hostp) {sHostp = hostp; }
	static void		setEditModeEnabled(BOOL enable);
	static BOOL		getEditModeEnabled() { return sEditModeEnabled; }
	static LLMultiFloater* getFloaterHost() {return sHostp; }
		
protected:

	void			setRectControl(const std::string& rectname) { mRectControl = rectname; };
	void			applyRectControl();
	void			storeRectControl();
	void			storeVisibilityControl();

	void		 	setKey(const LLSD& key);
	void		 	setInstanceName(const std::string& name);
	
	virtual void	bringToFront(S32 x, S32 y);
	virtual void	setVisibleAndFrontmost(BOOL take_focus=TRUE);    
	
	void			setExpandedRect(const LLRect& rect) { mExpandedRect = rect; } // size when not minimized
	const LLRect&	getExpandedRect() const { return mExpandedRect; }

	void			setAutoFocus(BOOL focus) { mAutoFocus = focus; } // whether to automatically take focus when opened
	LLDragHandle*	getDragHandle() const { return mDragHandle; }

	void			destroy() { die(); } // Don't call this directly.  You probably want to call close(). JC

	void			initOpenCallback(const OpenCallbackParam& cb, open_signal_t& sig);
	void			initCloseCallback(const CloseCallbackParam& cb, close_signal_t& sig);

private:
	void			setForeground(BOOL b);	// called only by floaterview
	void			cleanupHandles(); // remove handles to dead floaters
	void			createMinimizeButton();
	void			updateButtons();
	void			buildButtons();
	BOOL			offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButtons index);
	void			addResizeCtrls();
	void 			addDragHandle();

public:
	class OpenCallbackRegistry : public CallbackRegistry<open_callback_t, OpenCallbackRegistry> {};
	class CloseCallbackRegistry : public CallbackRegistry<close_callback_t, CloseCallbackRegistry> {};

protected:
	std::string		mRectControl;
	std::string		mVisibilityControl;
	open_signal_t   mOpenSignal;
	close_signal_t  mCloseSignal;
	LLSD			mKey;				// Key used for retrieving instances; set (for now) by LLFLoaterReg

	LLDragHandle*	mDragHandle;
	LLResizeBar*	mResizeBar[4];
	LLResizeHandle*	mResizeHandle[4];

private:
	LLRect			mExpandedRect;
	
	LLUIString		mTitle;
	LLUIString		mShortTitle;
	
	BOOL			mSingleInstance;	// TRUE if there is only ever one instance of the floater
	std::string		mInstanceName;		// Store the instance name so we can remove ourselves from the list
	BOOL			mAutoTile;			// TRUE if placement of new instances tiles
	
	BOOL			mCanTearOff;
	BOOL			mCanMinimize;
	BOOL			mCanClose;
	BOOL			mDragOnLeft;
	BOOL			mResizable;
	
	S32				mMinWidth;
	S32				mMinHeight;
	
	BOOL			mMinimized;
	BOOL			mForeground;
	LLHandle<LLFloater>	mDependeeHandle;
	

	BOOL			mFirstLook;			// TRUE if the _next_ time this floater is visible will be the first time in the session that it is visible.
	BOOL			mEditing;
	
	typedef std::set<LLHandle<LLFloater> > handle_set_t;
	typedef std::set<LLHandle<LLFloater> >::iterator handle_set_iter_t;
	handle_set_t	mDependents;

	BOOL			mButtonsEnabled[BUTTON_COUNT];
	LLButton*		mButtons[BUTTON_COUNT];
	F32				mButtonScale;
	BOOL			mAutoFocus;
	LLHandle<LLFloater> mSnappedTo;
	
	LLHandle<LLFloater> mHostHandle;
	LLHandle<LLFloater> mLastHostHandle;

	bool            mCanDock;
	bool            mDocked;

	static LLMultiFloater* sHostp;
	static BOOL		sEditModeEnabled;
	static BOOL		sQuitting;
	static std::string	sButtonActiveImageNames[BUTTON_COUNT];
	static std::string	sButtonPressedImageNames[BUTTON_COUNT];
	static std::string	sButtonNames[BUTTON_COUNT];
	static std::string	sButtonToolTips[BUTTON_COUNT];
	static std::string  sButtonToolTipsIndex[BUTTON_COUNT];
	
	typedef void(*click_callback)(LLFloater*);
	static click_callback sButtonCallbacks[BUTTON_COUNT];

	typedef std::map<LLHandle<LLFloater>, LLFloater*> handle_map_t;
	typedef std::map<LLHandle<LLFloater>, LLFloater*>::iterator handle_map_iter_t;
	static handle_map_t	sFloaterMap;

	std::vector<LLHandle<LLView> > mMinimizedHiddenChildren;

	BOOL			mHasBeenDraggedWhileMinimized;
	S32				mPreviousMinimizedBottom;
	S32				mPreviousMinimizedLeft;

	LLColor4		mBgColorAlpha;
	LLColor4		mBgColorOpaque;

	LLFloaterNotificationContext* mNotificationContext;
	LLRootHandle<LLFloater>		mHandle;	
};


/////////////////////////////////////////////////////////////
// LLFloaterView
// Parent of all floating panels

class LLFloaterView : public LLUICtrl
{
protected:
	LLFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void reshapeFloater(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical);

	/*virtual*/ void draw();
	/*virtual*/ LLRect getSnapRect() const;
	/*virtual*/ void refresh();

	LLRect			findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor );

	// Given a child of gFloaterView, make sure this view can fit entirely onscreen.
	void			adjustToFitScreen(LLFloater* floater, BOOL allow_partial_outside);

	void			getMinimizePosition( S32 *left, S32 *bottom);
	void			restoreAll();		// un-minimize all floaters
	typedef std::set<LLView*> skip_list_t;
	void pushVisibleAll(BOOL visible, const skip_list_t& skip_list = skip_list_t());
	void popVisibleAll(const skip_list_t& skip_list = skip_list_t());

	void			setCycleMode(BOOL mode) { mFocusCycleMode = mode; }
	BOOL			getCycleMode() const { return mFocusCycleMode; }
	void			bringToFront( LLFloater* child, BOOL give_focus = TRUE );
	void			highlightFocusedFloater();
	void			unhighlightFocusedFloater();
	void			focusFrontFloater();
	void			destroyAllChildren();
	// attempt to close all floaters
	void			closeAllChildren(bool app_quitting);
	BOOL			allChildrenClosed();

	LLFloater* getFrontmost() const;
	LLFloater* getBackmost() const;
	LLFloater* getParentFloater(LLView* viewp) const;
	LLFloater* getFocusedFloater() const;
	void		syncFloaterTabOrder();

	// Returns z order of child provided. 0 is closest, larger numbers
	// are deeper in the screen. If there is no such child, the return
	// value is not defined.
	S32 getZOrder(LLFloater* child);

	void setSnapOffsetBottom(S32 offset) { mSnapOffsetBottom = offset; }
	void setSnapOffsetRight(S32 offset) { mSnapOffsetRight = offset; }

private:
	S32				mColumn;
	S32				mNextLeft;
	S32				mNextTop;
	BOOL			mFocusCycleMode;
	S32				mSnapOffsetBottom;
	S32				mSnapOffsetRight;
};

//*******************************************************
//* TO BE DEPRECATED
//*******************************************************
// visibility policy specialized for floaters
template<>
class VisibilityPolicy<LLFloater>
{
public:
	// visibility methods
	static bool visible(LLFloater* instance, const LLSD& key);

	static void show(LLFloater* instance, const LLSD& key);

	static void hide(LLFloater* instance, const LLSD& key);
};

//
// Globals
//

extern LLFloaterView* gFloaterView;

namespace LLInitParam
{   
    template<> 
	bool ParamCompare<LLFloater::close_callback_t>::equals(
		const LLFloater::close_callback_t &a, 
		const LLFloater::close_callback_t &b); 
}


#endif  // LL_FLOATER_H



