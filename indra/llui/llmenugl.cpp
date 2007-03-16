/** 
 * @file llmenugl.cpp
 * @brief LLMenuItemGL base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//*****************************************************************************
//
// This file contains the opengl based menu implementation.
//
// NOTES: A menu label is split into 4 columns. The left column, the
// label colum, the accelerator column, and the right column. The left
// column is used for displaying boolean values for toggle and check
// controls. The right column is used for submenus.
//
//*****************************************************************************

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llmenugl.h"

#include "llmath.h"
#include "llgl.h"
#include "llfocusmgr.h"
#include "llfont.h"
#include "llcoord.h"
#include "llwindow.h"
#include "llcriticaldamp.h"
#include "lluictrlfactory.h"

#include "llfontgl.h"
#include "llresmgr.h"
#include "llui.h"

#include "llglheaders.h"
#include "llstl.h"

#include "v2math.h"
#include <set>
#include <boost/tokenizer.hpp>

// static
LLMenuHolderGL *LLMenuGL::sMenuContainer = NULL;

S32 MENU_BAR_HEIGHT = 0;
S32 MENU_BAR_WIDTH = 0;

///============================================================================
/// Local function declarations, constants, enums, and typedefs
///============================================================================

const LLString SEPARATOR_NAME("separator");
const LLString TEAROFF_SEPARATOR_LABEL( "~~~~~~~~~~~" );
const LLString SEPARATOR_LABEL( "-----------" );
const LLString VERTICAL_SEPARATOR_LABEL( "|" );

const S32 LABEL_BOTTOM_PAD_PIXELS = 2;

const U32 LEFT_PAD_PIXELS = 3;
const U32 LEFT_WIDTH_PIXELS = 15;
const U32 LEFT_PLAIN_PIXELS = LEFT_PAD_PIXELS + LEFT_WIDTH_PIXELS;

const U32 RIGHT_PAD_PIXELS = 2;
const U32 RIGHT_WIDTH_PIXELS = 15;
const U32 RIGHT_PLAIN_PIXELS = RIGHT_PAD_PIXELS + RIGHT_WIDTH_PIXELS;

const U32 ACCEL_PAD_PIXELS = 10;
const U32 PLAIN_PAD_PIXELS = LEFT_PAD_PIXELS + LEFT_WIDTH_PIXELS + RIGHT_PAD_PIXELS + RIGHT_WIDTH_PIXELS;

const U32 BRIEF_PAD_PIXELS = 2;

const U32 SEPARATOR_HEIGHT_PIXELS = 8;
const S32 TEAROFF_SEPARATOR_HEIGHT_PIXELS = 10;
const S32 MENU_ITEM_PADDING = 4;

const LLString BOOLEAN_TRUE_PREFIX( "X" );
const LLString BRANCH_SUFFIX( ">" );
const LLString ARROW_UP  ("^^^^^^^");
const LLString ARROW_DOWN("vvvvvvv");

const F32 MAX_MOUSE_SLOPE_SUB_MENU = 0.9f;

const S32 PIE_GESTURE_ACTIVATE_DISTANCE = 10;

LLColor4 LLMenuItemGL::sEnabledColor( 0.0f, 0.0f, 0.0f, 1.0f );
LLColor4 LLMenuItemGL::sDisabledColor( 0.5f, 0.5f, 0.5f, 1.0f );
LLColor4 LLMenuItemGL::sHighlightBackground( 0.0f, 0.0f, 0.7f, 1.0f );
LLColor4 LLMenuItemGL::sHighlightForeground( 1.0f, 1.0f, 1.0f, 1.0f );
BOOL LLMenuItemGL::sDropShadowText = TRUE;

LLColor4 LLMenuGL::sDefaultBackgroundColor( 0.25f, 0.25f, 0.25f, 0.75f );
BOOL LLMenuGL::sKeyboardMode = FALSE;

LLViewHandle LLMenuHolderGL::sItemLastSelectedHandle;
LLFrameTimer LLMenuHolderGL::sItemActivationTimer;
//LLColor4 LLMenuGL::sBackgroundColor( 0.8f, 0.8f, 0.0f, 1.0f );

const S32 PIE_CENTER_SIZE = 20;		// pixels, radius of center hole
const F32 PIE_SCALE_FACTOR = 1.7f; // scale factor for pie menu when mouse is initially down
const F32 PIE_SHRINK_TIME = 0.2f; // time of transition between unbounded and bounded display of pie menu

const F32 ACTIVATE_HIGHLIGHT_TIME = 0.3f;

///============================================================================
/// Class LLMenuItemGL
///============================================================================

// Default constructor
LLMenuItemGL::LLMenuItemGL( const LLString& name, const LLString& label, KEY key, MASK mask ) :
	LLView( name, TRUE ),
	mJumpKey(KEY_NONE),
	mAcceleratorKey( key ),
	mAcceleratorMask( mask ),
	mAllowKeyRepeat(FALSE),
	mHighlight( FALSE ),
	mGotHover( FALSE ),
	mBriefItem( FALSE ),
	mFont( LLFontGL::sSansSerif ),
	mStyle(LLFontGL::NORMAL),
	mDrawTextDisabled( FALSE )
{
	setLabel( label );
}

// virtual
LLXMLNodePtr LLMenuItemGL::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();

	node->createChild("type", TRUE)->setStringValue(getType());

	node->createChild("label", TRUE)->setStringValue(mLabel);

	if (mAcceleratorKey != KEY_NONE)
	{
		std::stringstream out;
		if (mAcceleratorMask & MASK_CONTROL)
		{
			out << "control|";
		}
		if (mAcceleratorMask & MASK_ALT)
		{
			out << "alt|";
		}
		if (mAcceleratorMask & MASK_SHIFT)
		{
			out << "shift|";
		}
		out << LLKeyboard::stringFromKey(mAcceleratorKey);

		node->createChild("shortcut", TRUE)->setStringValue(out.str());
	}

	return node;
}

BOOL LLMenuItemGL::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	// modified from LLView::handleKey
	// ignore visibility, as keyboard accelerators should still trigger menu items
	// even when they are not visible
	// also, ignore enabled flag for self, as that can change based on menu callbacks
	BOOL handled = FALSE;

	if( called_from_parent )  
	{
		// Downward traversal
		if (mEnabled)
		{
			handled = childrenHandleKey( key, mask ) != NULL;
		}
	}
	
	if( !handled )
	{
		handled = handleKeyHere( key, mask, called_from_parent );
	}

	return handled;
}

BOOL LLMenuItemGL::handleAcceleratorKey(KEY key, MASK mask)
{
	if( mEnabled && (!gKeyboard->getKeyRepeated(key) || mAllowKeyRepeat) && (key == mAcceleratorKey) && (mask == mAcceleratorMask) )
	{
		doIt();
		return TRUE;
	}
	return FALSE;
}

BOOL LLMenuItemGL::handleHover(S32 x, S32 y, MASK mask)
{
	mGotHover = TRUE;
	getWindow()->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}
	
void LLMenuItemGL::setBriefItem(BOOL b)
{
	mBriefItem = b;
}

// This function checks to see if the accelerator key is already in use;
// if not, it will be added to the list
BOOL LLMenuItemGL::addToAcceleratorList(std::list <LLKeyBinding*> *listp)
{
	LLKeyBinding *accelerator = NULL;

	if (mAcceleratorKey != KEY_NONE)
	{
		std::list<LLKeyBinding*>::iterator list_it;
		for (list_it = listp->begin(); list_it != listp->end(); ++list_it)
		{
			accelerator = *list_it;
			if ((accelerator->mKey == mAcceleratorKey) && (accelerator->mMask == mAcceleratorMask))
			{

			// *NOTE: get calling code to throw up warning or route
			// warning messages back to app-provided output
			//	LLString warning;
			//	warning.append("Duplicate key binding <");
			//	appendAcceleratorString( warning );
			//	warning.append("> for menu items:\n    ");
			//	warning.append(accelerator->mName);
			//	warning.append("\n    ");
			//	warning.append(mLabel);

			//	llwarns << warning << llendl;
			//	LLAlertDialog::modalAlert(warning);
				return FALSE;
			}
		}
		if (!accelerator)
		{				
			accelerator = new LLKeyBinding;
			if (accelerator)
			{
				accelerator->mKey = mAcceleratorKey;
				accelerator->mMask = mAcceleratorMask;
// 				accelerator->mName = mLabel;
			}
			listp->push_back(accelerator);//addData(accelerator);
		}
	}
	return TRUE;
}

// This function appends the character string representation of
// the current accelerator key and mask to the provided string.
void LLMenuItemGL::appendAcceleratorString( LLString& st )
{
	// break early if this is a silly thing to do.
	if( KEY_NONE == mAcceleratorKey )
	{
		return;
	}

	// Append any masks
#ifdef LL_DARWIN
	// Standard Mac names for modifier keys in menu equivalents
	// We could use the symbol characters, but they only exist in certain fonts.
	if( mAcceleratorMask & MASK_CONTROL )
		st.append( "Cmd-" );		// Symbol would be "\xE2\x8C\x98"
	if( mAcceleratorMask & MASK_ALT )
		st.append( "Opt-" );		// Symbol would be "\xE2\x8C\xA5"
	if( mAcceleratorMask & MASK_SHIFT )
		st.append( "Shift-" );		// Symbol would be "\xE2\x8C\xA7"
#else
	if( mAcceleratorMask & MASK_CONTROL )
		st.append( "Ctrl-" );
	if( mAcceleratorMask & MASK_ALT )
		st.append( "Alt-" );
	if( mAcceleratorMask & MASK_SHIFT )
		st.append( "Shift-" );
#endif

	LLString keystr = LLKeyboard::stringFromKey( mAcceleratorKey );
	if ((mAcceleratorMask & (MASK_CONTROL|MASK_ALT|MASK_SHIFT)) &&
		(keystr[0] == '-' || keystr[0] == '='))
	{
		st.append( " " );
	}
	st.append( keystr );
}

void LLMenuItemGL::setJumpKey(KEY key)
{
	mJumpKey = LLStringOps::toUpper((char)key);
}

KEY LLMenuItemGL::getJumpKey()
{
	return mJumpKey;
}


// set the font used by all of the menu objects
void LLMenuItemGL::setFont(LLFontGL* font)
{
	mFont = font;
}

// returns the height in pixels for the current font.
U32 LLMenuItemGL::getNominalHeight( void )
{
	return llround(mFont->getLineHeight()) + MENU_ITEM_PADDING;
}

// functions to control the color scheme
void LLMenuItemGL::setEnabledColor( const LLColor4& color )
{
	sEnabledColor = color;
}

void LLMenuItemGL::setDisabledColor( const LLColor4& color )
{
	sDisabledColor = color;
}

void LLMenuItemGL::setHighlightBGColor( const LLColor4& color )
{
	sHighlightBackground = color;
}

void LLMenuItemGL::setHighlightFGColor( const LLColor4& color )
{
	sHighlightForeground = color;
}


// change the label
void LLMenuItemGL::setLabel( const LLString& label )
{
	mLabel = label;
}

// Get the parent menu for this item
LLMenuGL* LLMenuItemGL::getMenu()
{
	return (LLMenuGL*) getParent();
}


// getNominalWidth() - returns the normal width of this control in
// pixels - this is used for calculating the widest item, as well as
// for horizontal arrangement.
U32 LLMenuItemGL::getNominalWidth( void )
{
	U32 width;
	
	if (mBriefItem)
	{
		width = BRIEF_PAD_PIXELS;
	}
	else
	{
		width = PLAIN_PAD_PIXELS;
	}

	if( KEY_NONE != mAcceleratorKey )
	{
		width += ACCEL_PAD_PIXELS;
		LLString temp;
		appendAcceleratorString( temp );
		width += mFont->getWidth( temp );
	}
	width += mFont->getWidth( mLabel.getWString().c_str() );
	return width;
}

// called to rebuild the draw label
void LLMenuItemGL::buildDrawLabel( void )
{
	mDrawAccelLabel.clear();
	LLString st = mDrawAccelLabel.getString();
	appendAcceleratorString( st );
	mDrawAccelLabel = st;
}

void LLMenuItemGL::doIt( void )
{
	// close all open menus by default
	// if parent menu is actually visible (and we are not triggering menu item via accelerator)
	if (!getMenu()->getTornOff() 
		&& getMenu()->getVisible())
	{
		LLMenuGL::sMenuContainer->hideMenus();
	}
}

// set the hover status (called by it's menu)
 void LLMenuItemGL::setHighlight( BOOL highlight )
{
	if (highlight)
	{
		getMenu()->clearHoverItem();
	}
	mHighlight = highlight;
}

// determine if this object represents an active sub-menu
BOOL LLMenuItemGL::isActive( void ) const
{
	return FALSE;
}

// determine if this object represents an open sub-menu
BOOL LLMenuItemGL::isOpen( void ) const
{
	return FALSE;
}

BOOL LLMenuItemGL::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	if (getHighlight() && 
		getMenu()->isOpen())
	{
		if (key == KEY_UP)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			getMenu()->highlightPrevItem(this);
			return TRUE;
		}
		else if (key == KEY_DOWN)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			getMenu()->highlightNextItem(this);
			return TRUE;
		}
		else if (key == KEY_RETURN && mask == MASK_NONE)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			doIt();
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMenuItemGL::handleMouseUp( S32 x, S32 y, MASK )
{
	if (mEnabled)
	{
		// switch to mouse navigation mode
		LLMenuGL::setKeyboardMode(FALSE);

		doIt();
		make_ui_sound("UISndClickRelease");
		return TRUE;
	}
	return FALSE;
}

BOOL LLMenuItemGL::handleMouseDown( S32 x, S32 y, MASK )
{
	if (mEnabled)
	{
		// switch to mouse navigation mode
		LLMenuGL::setKeyboardMode(FALSE);

		setHighlight(TRUE);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void LLMenuItemGL::draw( void )
{
	// *FIX: This can be optimized by using switches. Want to avoid
	// that until the functionality is finalized.

	// HACK: Brief items don't highlight.  Pie menu takes care of it.  JC
	// let disabled items be highlighted, just don't draw them as such
	if( getEnabled() && getHighlight() && !mBriefItem)
	{
		glColor4fv( sHighlightBackground.mV );
		gl_rect_2d( 0, mRect.getHeight(), mRect.getWidth(), 0 );
	}

	LLColor4 color;

	U8 font_style = mStyle;
	if (LLMenuItemGL::sDropShadowText && getEnabled() && !mDrawTextDisabled )
	{
		font_style |= LLFontGL::DROP_SHADOW;
	}

	if ( getEnabled() && getHighlight() )
	{
		color = sHighlightForeground;
	}
	else if( getEnabled() && !mDrawTextDisabled )
	{
		color = sEnabledColor;
	}
	else
	{
		color = sDisabledColor;
	}

	// Draw the text on top.
	if (mBriefItem)
	{
		mFont->render( mLabel, 0, BRIEF_PAD_PIXELS / 2, 0, color,
					   LLFontGL::LEFT, LLFontGL::BOTTOM, font_style );
	}
	else
	{
		if( !mDrawBoolLabel.empty() )
		{
			mFont->render( mDrawBoolLabel.getWString(), 0, (F32)LEFT_PAD_PIXELS, ((F32)MENU_ITEM_PADDING / 2.f) + 1.f, color,
						   LLFontGL::LEFT, LLFontGL::BOTTOM, font_style, S32_MAX, S32_MAX, NULL, FALSE );
		}
		mFont->render( mLabel.getWString(), 0, (F32)LEFT_PLAIN_PIXELS, ((F32)MENU_ITEM_PADDING / 2.f) + 1.f, color,
					   LLFontGL::LEFT, LLFontGL::BOTTOM, font_style, S32_MAX, S32_MAX, NULL, FALSE );
		if( !mDrawAccelLabel.empty() )
		{
			mFont->render( mDrawAccelLabel.getWString(), 0, (F32)mRect.mRight - (F32)RIGHT_PLAIN_PIXELS, ((F32)MENU_ITEM_PADDING / 2.f) + 1.f, color,
						   LLFontGL::RIGHT, LLFontGL::BOTTOM, font_style, S32_MAX, S32_MAX, NULL, FALSE );
		}
		if( !mDrawBranchLabel.empty() )
		{
			mFont->render( mDrawBranchLabel.getWString(), 0, (F32)mRect.mRight - (F32)RIGHT_PAD_PIXELS, ((F32)MENU_ITEM_PADDING / 2.f) + 1.f, color,
						   LLFontGL::RIGHT, LLFontGL::BOTTOM, font_style, S32_MAX, S32_MAX, NULL, FALSE );
		}
	}

	// underline "jump" key only when keyboard navigation has been initiated
	if (getMenu()->jumpKeysActive() && LLMenuGL::getKeyboardMode())
	{
		LLString upper_case_label = mLabel.getString();
		LLString::toUpper(upper_case_label);
		std::string::size_type offset = upper_case_label.find(mJumpKey);
		if (offset != std::string::npos)
		{
			S32 x_begin = LEFT_PLAIN_PIXELS + mFont->getWidth(mLabel, 0, offset);
			S32 x_end = LEFT_PLAIN_PIXELS + mFont->getWidth(mLabel, 0, offset + 1);
			gl_line_2d(x_begin, (MENU_ITEM_PADDING / 2) + 1, x_end, (MENU_ITEM_PADDING / 2) + 1);
		}
	}

	// clear got hover every frame
	mGotHover = FALSE;
}

BOOL LLMenuItemGL::setLabelArg( const LLString& key, const LLString& text )
{
	mLabel.setArg(key, text);
	return TRUE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemSeparatorGL
//
// This class represents a separator.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemSeparatorGL : public LLMenuItemGL
{
public:
	LLMenuItemSeparatorGL( const LLString &name = SEPARATOR_NAME );

	virtual LLString getType() const	{ return "separator"; }

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_MENU_ITEM_SEPARATOR; }
	virtual LLString getWidgetTag() const { return LL_MENU_ITEM_SEPARATOR_GL_TAG; }

	// doIt() - do the primary funcationality of the menu item.
	virtual void doIt( void ) {}

	virtual void draw( void );
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);

	virtual U32 getNominalHeight( void ) { return SEPARATOR_HEIGHT_PIXELS; }
};

LLMenuItemSeparatorGL::LLMenuItemSeparatorGL( const LLString &name ) :
	LLMenuItemGL( SEPARATOR_NAME, SEPARATOR_LABEL )
{
}

void LLMenuItemSeparatorGL::draw( void )
{
	glColor4fv( sDisabledColor.mV );
	const S32 y = mRect.getHeight() / 2;
	const S32 PAD = 6;
	gl_line_2d( PAD, y, mRect.getWidth() - PAD, y );
}

BOOL LLMenuItemSeparatorGL::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLMenuGL* parent_menu = getMenu();
	if (y > mRect.getHeight() / 2)
	{
		return parent_menu->handleMouseDown(x + mRect.mLeft, mRect.mTop + 1, mask);
	}
	else
	{
		return parent_menu->handleMouseDown(x + mRect.mLeft, mRect.mBottom - 1, mask);
	}
}

BOOL LLMenuItemSeparatorGL::handleMouseUp(S32 x, S32 y, MASK mask) 
{
	LLMenuGL* parent_menu = getMenu();
	if (y > mRect.getHeight() / 2)
	{
		return parent_menu->handleMouseUp(x + mRect.mLeft, mRect.mTop + 1, mask);
	}
	else
	{
		return parent_menu->handleMouseUp(x + mRect.mLeft, mRect.mBottom - 1, mask);
	}
}

BOOL LLMenuItemSeparatorGL::handleHover(S32 x, S32 y, MASK mask) 
{
	LLMenuGL* parent_menu = getMenu();
	if (y > mRect.getHeight() / 2)
	{
		parent_menu->highlightPrevItem(this, FALSE);
		return FALSE;
	}
	else
	{
		parent_menu->highlightNextItem(this, FALSE);
		return FALSE;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemVerticalSeparatorGL
//
// This class represents a vertical separator.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemVerticalSeparatorGL
:	public LLMenuItemSeparatorGL
{
public:
	LLMenuItemVerticalSeparatorGL( void );

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_MENU_SEPARATOR_VERTICAL; }
	virtual LLString getWidgetTag() const { return LL_MENU_ITEM_VERTICAL_SEPARATOR_GL_TAG; }

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; }
};

LLMenuItemVerticalSeparatorGL::LLMenuItemVerticalSeparatorGL( void )
{
	setLabel( VERTICAL_SEPARATOR_LABEL );
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemTearOffGL
//
// This class represents a separator.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLMenuItemTearOffGL::LLMenuItemTearOffGL(LLViewHandle parent_floater_handle) : 
	LLMenuItemGL("tear off", TEAROFF_SEPARATOR_LABEL), 
	mParentHandle(parent_floater_handle)
{
}

EWidgetType LLMenuItemTearOffGL::getWidgetType() const
{
	return WIDGET_TYPE_TEAROFF_MENU;
}

LLString LLMenuItemTearOffGL::getWidgetTag() const
{
	return LL_MENU_ITEM_TEAR_OFF_GL_TAG;
}

void LLMenuItemTearOffGL::doIt()
{
	if (getMenu()->getTornOff())
	{
		LLTearOffMenu* torn_off_menu = (LLTearOffMenu*)(getMenu()->getParent());
		torn_off_menu->close();
	}
	else
	{
		// transfer keyboard focus and highlight to first real item in list
		if (getHighlight())
		{
			getMenu()->highlightNextItem(this);
		}

		getMenu()->arrange();

		LLFloater* parent_floater = LLFloater::getFloaterByHandle(mParentHandle);
		LLFloater* tear_off_menu = LLTearOffMenu::create(getMenu());
		if (parent_floater && tear_off_menu)
		{
			parent_floater->addDependentFloater(tear_off_menu, FALSE);
		}

		// give focus to torn off menu because it will have been taken away
		// when parent menu closes
		tear_off_menu->setFocus(TRUE);
	}
	LLMenuItemGL::doIt();
}

void LLMenuItemTearOffGL::draw()
{
	// disabled items can be highlighted, but shouldn't render as such
	if( getEnabled() && getHighlight() && !mBriefItem)
	{
		glColor4fv( sHighlightBackground.mV );
		gl_rect_2d( 0, mRect.getHeight(), mRect.getWidth(), 0 );
	}

	if (mEnabled)
	{
		glColor4fv( sEnabledColor.mV );
	}
	else
	{
		glColor4fv( sDisabledColor.mV );
	}
	const S32 y = mRect.getHeight() / 3;
	const S32 PAD = 6;
	gl_line_2d( PAD, y, mRect.getWidth() - PAD, y );
	gl_line_2d( PAD, y * 2, mRect.getWidth() - PAD, y * 2 );
}

U32 LLMenuItemTearOffGL::getNominalHeight( void ) { return TEAROFF_SEPARATOR_HEIGHT_PIXELS; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemBlankGL
//
// This class represents a blank, non-functioning item.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemBlankGL : public LLMenuItemGL
{
public:
	LLMenuItemBlankGL( void );

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_MENU_ITEM_BLANK; }
	virtual LLString getWidgetTag() const { return LL_MENU_ITEM_BLANK_GL_TAG; }

	// doIt() - do the primary funcationality of the menu item.
	virtual void doIt( void ) {}

	virtual void draw( void ) {}
};

LLMenuItemBlankGL::LLMenuItemBlankGL( void )
:	LLMenuItemGL( "", "" )
{
	mEnabled = FALSE;
}

///============================================================================
/// Class LLMenuItemCallGL
///============================================================================

LLMenuItemCallGL::LLMenuItemCallGL( const LLString& name, 
									const LLString& label, 
									menu_callback clicked_cb, 
								    enabled_callback enabled_cb,
									void* user_data,
									KEY key, MASK mask,
									BOOL enabled,
									on_disabled_callback on_disabled_cb) :
	LLMenuItemGL( name, label, key, mask ),
	mCallback( clicked_cb ),
	mEnabledCallback( enabled_cb ),
	mLabelCallback(NULL),
	mUserData( user_data ),
	mOnDisabledCallback(on_disabled_cb)
{
	if(!enabled) setEnabled(FALSE);
}

LLMenuItemCallGL::LLMenuItemCallGL( const LLString& name, 
									menu_callback clicked_cb, 
								    enabled_callback enabled_cb,
									void* user_data,
									KEY key, MASK mask,
									BOOL enabled,
									on_disabled_callback on_disabled_cb) :
	LLMenuItemGL( name, name, key, mask ),
	mCallback( clicked_cb ),
	mEnabledCallback( enabled_cb ),
	mLabelCallback(NULL),
	mUserData( user_data ),
	mOnDisabledCallback(on_disabled_cb)
{
	if(!enabled) setEnabled(FALSE);
}

LLMenuItemCallGL::LLMenuItemCallGL(const LLString& name,
								   const LLString& label,
								   menu_callback clicked_cb,
								   enabled_callback enabled_cb,
								   label_callback label_cb,
								   void* user_data,
								   KEY key, MASK mask,
								   BOOL enabled,
								   on_disabled_callback on_disabled_cb) :
	LLMenuItemGL(name, label, key, mask),
	mCallback(clicked_cb),
	mEnabledCallback(enabled_cb),
	mLabelCallback(label_cb),
	mUserData(user_data),
	mOnDisabledCallback(on_disabled_cb)
{
	if(!enabled) setEnabled(FALSE);
}

LLMenuItemCallGL::LLMenuItemCallGL(const LLString& name,
								   menu_callback clicked_cb,
								   enabled_callback enabled_cb,
								   label_callback label_cb,
								   void* user_data,
								   KEY key, MASK mask,
								   BOOL enabled,
								   on_disabled_callback on_disabled_cb) :
	LLMenuItemGL(name, name, key, mask),
	mCallback(clicked_cb),
	mEnabledCallback(enabled_cb),
	mLabelCallback(label_cb),
	mUserData(user_data),
	mOnDisabledCallback(on_disabled_cb)
{
	if(!enabled) setEnabled(FALSE);
}

void LLMenuItemCallGL::setEnabledControl(LLString enabled_control, LLView *context)
{
	// Register new listener
	if (!enabled_control.empty())
	{
		LLControlBase *control = context->findControl(enabled_control);
		if (control)
		{
			LLSD state = control->registerListener(this, "ENABLED");
			setEnabled(state);
		}
		else
		{
			context->addBoolControl(enabled_control, mEnabled);
			control = context->findControl(enabled_control);
			control->registerListener(this, "ENABLED");
		}
	}
}

void LLMenuItemCallGL::setVisibleControl(LLString enabled_control, LLView *context)
{
	// Register new listener
	if (!enabled_control.empty())
	{
		LLControlBase *control = context->findControl(enabled_control);
		if (control)
		{
			LLSD state = control->registerListener(this, "VISIBLE");
			setVisible(state);
		}
		else
		{
			context->addBoolControl(enabled_control, mEnabled);
			control = context->findControl(enabled_control);
			control->registerListener(this, "VISIBLE");
		}
	}
}

// virtual
bool LLMenuItemCallGL::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (userdata.asString() == "ENABLED" && event->desc() == "value_changed")
	{
		LLSD state = event->getValue();
		setEnabled(state);
		return TRUE;
	}
	if (userdata.asString() == "VISIBLE" && event->desc() == "value_changed")
	{
		LLSD state = event->getValue();
		setVisible(state);
		return TRUE;
	}
	return LLMenuItemGL::handleEvent(event, userdata);
}

// virtual
LLXMLNodePtr LLMenuItemCallGL::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLMenuItemGL::getXML();

	// Contents

	std::vector<LLListenerEntry> listeners = mDispatcher->getListeners();
	std::vector<LLListenerEntry>::iterator itor;
	for (itor = listeners.begin(); itor != listeners.end(); ++itor)
	{
		LLString listener_name = findEventListener((LLSimpleListener*)itor->listener);
		if (!listener_name.empty())
		{
			LLXMLNodePtr child_node = node->createChild("on_click", FALSE);
			child_node->createChild("function", TRUE)->setStringValue(listener_name);
			child_node->createChild("filter", TRUE)->setStringValue(itor->filter.asString());
			child_node->createChild("userdata", TRUE)->setStringValue(itor->userdata.asString());
		}
	}

	return node;
}

// doIt() - Call the callback provided
void LLMenuItemCallGL::doIt( void )
{
	// RN: menu item can be deleted in callback, so beware
	getMenu()->setItemLastSelected( this );

	if( mCallback )
	{
		mCallback( mUserData );
	}
	LLPointer<LLEvent> fired_event = new LLEvent(this);
	fireEvent(fired_event, "on_click");
	LLMenuItemGL::doIt();
}

EWidgetType LLMenuItemCallGL::getWidgetType() const 
{
	return WIDGET_TYPE_MENU_ITEM_CALL;
}

LLString LLMenuItemCallGL::getWidgetTag() const
{
	return LL_MENU_ITEM_CALL_GL_TAG;
}

void LLMenuItemCallGL::buildDrawLabel( void )
{
	LLPointer<LLEvent> fired_event = new LLEvent(this);
	fireEvent(fired_event, "on_build");
	if( mEnabledCallback )
	{
		setEnabled( mEnabledCallback( mUserData ) );
	}
	if(mLabelCallback)
	{
		LLString label;
		mLabelCallback(label, mUserData);
		mLabel = label;
	}
	LLMenuItemGL::buildDrawLabel();
}

BOOL LLMenuItemCallGL::handleAcceleratorKey( KEY key, MASK mask )
{
 	if( (!gKeyboard->getKeyRepeated(key) || mAllowKeyRepeat) && (key == mAcceleratorKey) && (mask == mAcceleratorMask) )
	{
		LLPointer<LLEvent> fired_event = new LLEvent(this);
		fireEvent(fired_event, "on_build");
		if( mEnabledCallback )
		{
			setEnabled( mEnabledCallback( mUserData ) );
		}
		if( !mEnabled )
		{
			if( mOnDisabledCallback )
			{
				mOnDisabledCallback( mUserData );
			}
		}
	}
	return LLMenuItemGL::handleAcceleratorKey(key, mask);
}

///============================================================================
/// Class LLMenuItemCheckGL
///============================================================================

LLMenuItemCheckGL::LLMenuItemCheckGL ( const LLString& name, 
									   const LLString& label,
									   menu_callback clicked_cb,
									   enabled_callback enabled_cb,
									   check_callback check_cb,
									   void* user_data,
									   KEY key, MASK mask ) :
	LLMenuItemCallGL( name, label, clicked_cb, enabled_cb, user_data, key, mask ),
	mCheckCallback( check_cb ), 
	mChecked(FALSE)
{
}

LLMenuItemCheckGL::LLMenuItemCheckGL ( const LLString& name, 
									   menu_callback clicked_cb,
									   enabled_callback enabled_cb,
									   check_callback check_cb,
									   void* user_data,
									   KEY key, MASK mask ) :
	LLMenuItemCallGL( name, name, clicked_cb, enabled_cb, user_data, key, mask ),
	mCheckCallback( check_cb ), 
	mChecked(FALSE)
{
}

LLMenuItemCheckGL::LLMenuItemCheckGL ( const LLString& name, 
									   const LLString& label,
									   menu_callback clicked_cb,
									   enabled_callback enabled_cb,
									   LLString control_name,
									   LLView *context,
									   void* user_data,
									   KEY key, MASK mask ) :
	LLMenuItemCallGL( name, label, clicked_cb, enabled_cb, user_data, key, mask ),
	mCheckCallback( NULL )
{
	setControlName(control_name, context);
}

void LLMenuItemCheckGL::setCheckedControl(LLString checked_control, LLView *context)
{
	// Register new listener
	if (!checked_control.empty())
	{
		LLControlBase *control = context->findControl(checked_control);
		if (control)
		{
			LLSD state = control->registerListener(this, "CHECKED");
			mChecked = state;
		}
		else
		{
			context->addBoolControl(checked_control, mChecked);
			control = context->findControl(checked_control);
			control->registerListener(this, "CHECKED");
		}
	}
}

// virtual
bool LLMenuItemCheckGL::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (userdata.asString() == "CHECKED" && event->desc() == "value_changed")
	{
		LLSD state = event->getValue();
		mChecked = state;
		if(mChecked)
		{
			mDrawBoolLabel = BOOLEAN_TRUE_PREFIX;
		}
		else
		{
			mDrawBoolLabel.clear();
		}
		return TRUE;
	}
	return LLMenuItemCallGL::handleEvent(event, userdata);
}

// virtual
LLXMLNodePtr LLMenuItemCheckGL::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLMenuItemCallGL::getXML();
	return node;
}

EWidgetType LLMenuItemCheckGL::getWidgetType() const 
{ 
	return WIDGET_TYPE_MENU_ITEM_CHECK;
}

LLString LLMenuItemCheckGL::getWidgetTag() const
{
	return LL_MENU_ITEM_CHECK_GL_TAG;
}

// called to rebuild the draw label
void LLMenuItemCheckGL::buildDrawLabel( void )
{
	if(mChecked || (mCheckCallback && mCheckCallback( mUserData ) ) )
	{
		mDrawBoolLabel = BOOLEAN_TRUE_PREFIX;
	}
	else
	{
		mDrawBoolLabel.clear();
	}
	LLMenuItemCallGL::buildDrawLabel();
}


///============================================================================
/// Class LLMenuItemToggleGL
///============================================================================

LLMenuItemToggleGL::LLMenuItemToggleGL( const LLString& name, const LLString& label, BOOL* toggle,
										KEY key, MASK mask ) :
	LLMenuItemGL( name, label, key, mask ),
	mToggle( toggle )
{
}

LLMenuItemToggleGL::LLMenuItemToggleGL( const LLString& name, BOOL* toggle,
										KEY key, MASK mask ) :
	LLMenuItemGL( name, name, key, mask ),
	mToggle( toggle )
{
}


// called to rebuild the draw label
void LLMenuItemToggleGL::buildDrawLabel( void )
{
	if( *mToggle )
	{
		mDrawBoolLabel = BOOLEAN_TRUE_PREFIX;
	}
	else
	{
		mDrawBoolLabel.clear();
	}
	mDrawAccelLabel.clear();
	LLString st = mDrawAccelLabel;
	appendAcceleratorString( st );
	mDrawAccelLabel = st;
}

// doIt() - do the primary funcationality of the menu item.
void LLMenuItemToggleGL::doIt( void )
{
	getMenu()->setItemLastSelected( this );
	//llinfos << "LLMenuItemToggleGL::doIt " << mLabel.c_str() << llendl;
	*mToggle = !(*mToggle);
	buildDrawLabel();
	LLMenuItemGL::doIt();
}


LLMenuItemBranchGL::LLMenuItemBranchGL( const LLString& name, const LLString& label, LLMenuGL* branch,
										KEY key, MASK mask ) :
	LLMenuItemGL( name, label, key, mask ),
	mBranch( branch )
{
	mBranch->setVisible( FALSE );
	mBranch->setParentMenuItem(this);
}

// virtual
LLView* LLMenuItemBranchGL::getChildByName(const LLString& name, BOOL recurse) const
{
	if (mBranch->getName() == name)
	{
		return mBranch;
	}
	// Always recurse on branches
	return mBranch->getChildByName(name, recurse);
}

EWidgetType LLMenuItemBranchGL::getWidgetType() const
{
	return WIDGET_TYPE_MENU_ITEM_BRANCH;
}

LLString LLMenuItemBranchGL::getWidgetTag() const
{
	return LL_MENU_ITEM_BRANCH_GL_TAG;
}

// virtual
BOOL LLMenuItemBranchGL::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (mEnabled)
	{
		// switch to mouse navigation mode
		LLMenuGL::setKeyboardMode(FALSE);

		doIt();
		make_ui_sound("UISndClickRelease");
	}
	return FALSE;
}

BOOL LLMenuItemBranchGL::handleAcceleratorKey(KEY key, MASK mask)
{
	return mBranch->handleAcceleratorKey(key, mask);
}

// virtual
LLXMLNodePtr LLMenuItemBranchGL::getXML(bool save_children) const
{
	if (mBranch)
	{
		return mBranch->getXML();
	}

	return LLMenuItemGL::getXML();
}


// This function checks to see if the accelerator key is already in use;
// if not, it will be added to the list
BOOL LLMenuItemBranchGL::addToAcceleratorList(std::list<LLKeyBinding*> *listp)
{
	U32 item_count = mBranch->getItemCount();
	LLMenuItemGL *item;

	while (item_count--)
	{
		if ((item = mBranch->getItem(item_count)))
		{
			return item->addToAcceleratorList(listp);
		}
	}
	return FALSE;
}


// called to rebuild the draw label
void LLMenuItemBranchGL::buildDrawLabel( void )
{
	mDrawAccelLabel.clear();
	LLString st = mDrawAccelLabel;
	appendAcceleratorString( st );
	mDrawAccelLabel = st;
	mDrawBranchLabel = BRANCH_SUFFIX;
}

// doIt() - do the primary functionality of the menu item.
void LLMenuItemBranchGL::doIt( void )
{
	openMenu();

	// keyboard navigation automatically propagates highlight to sub-menu
	// to facilitate fast menu control via jump keys
	if (LLMenuGL::getKeyboardMode() && !mBranch->getHighlightedItem())
	{
		mBranch->highlightNextItem(NULL);
	}
}

BOOL LLMenuItemBranchGL::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (called_from_parent)
	{
		handled = mBranch->handleKey(key, mask, called_from_parent);
	}

	if (!handled)
	{
		handled = LLMenuItemGL::handleKey(key, mask, called_from_parent);
	}

	return handled;
}

BOOL LLMenuItemBranchGL::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (called_from_parent)
	{
		handled = mBranch->handleUnicodeChar(uni_char, TRUE);
	}

	if (!handled)
	{
		handled = LLMenuItemGL::handleUnicodeChar(uni_char, called_from_parent);
	}

	return handled;
}


// set the hover status (called by it's menu)
void LLMenuItemBranchGL::setHighlight( BOOL highlight )
{
	if (highlight == getHighlight()) return;

	// make sure only yourself is highlighted
	if (highlight)
	{
		getMenu()->clearHoverItem();
	}

	BOOL auto_open = mEnabled && (!mBranch->getVisible() || mBranch->getTornOff());
	// torn off menus don't open sub menus on hover unless they have focus
	if (getMenu()->getTornOff() && !((LLFloater*)getMenu()->getParent())->hasFocus())
	{
		auto_open = FALSE;
	}
	// don't auto open torn off sub-menus (need to explicitly active menu item to give them focus)
	if (mBranch->getTornOff())
	{
		auto_open = FALSE;
	}

	mHighlight = highlight;
	if( highlight )
	{
		if(auto_open)
		{
			openMenu();
		}
	}
	else
	{
		if (mBranch->getTornOff())
		{
			((LLFloater*)mBranch->getParent())->setFocus(FALSE);
			mBranch->clearHoverItem();
		}
		else
		{
			mBranch->setVisible( FALSE );
		}
	}
}

void LLMenuItemBranchGL::setEnabledSubMenus(BOOL enabled)
{
	mBranch->setEnabledSubMenus(enabled);
}

void LLMenuItemBranchGL::draw()
{
	LLMenuItemGL::draw();
	if (mBranch->getVisible() && !mBranch->getTornOff())
	{
		setHighlight(TRUE);
	}
}

// determine if this object is active
// which, for branching menus, means the branch is open and has "focus"
BOOL LLMenuItemBranchGL::isActive( void ) const
{
	return isOpen() && mBranch->getHighlightedItem();
}

BOOL LLMenuItemBranchGL::isOpen( void ) const
{
	return mBranch->isOpen();
}

void LLMenuItemBranchGL::updateBranchParent(LLView* parentp)
{
	if (mBranch->getParent() == NULL)
	{
		// make the branch menu a sibling of my parent menu
		mBranch->updateParent(parentp);
	}
}

void LLMenuItemBranchGL::onVisibilityChange( BOOL curVisibilityIn )
{
	if (curVisibilityIn == FALSE && mBranch->getVisible() && !mBranch->getTornOff())
	{
		mBranch->setVisible(FALSE);
	}
}

BOOL LLMenuItemBranchGL::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	if (getMenu()->getVisible() && mBranch->getVisible() && key == KEY_LEFT)
	{
		// switch to keyboard navigation mode
		LLMenuGL::setKeyboardMode(TRUE);

		BOOL handled = mBranch->clearHoverItem();
		if (mBranch->getTornOff())
		{
			((LLFloater*)mBranch->getParent())->setFocus(FALSE);
		}
		if (handled && getMenu()->getTornOff())
		{
			((LLFloater*)getMenu()->getParent())->setFocus(TRUE);
		}
		return handled;
	}

	if (getEnabled() &&
		getHighlight() && 
		getMenu()->isOpen() && 
		key == KEY_RIGHT && !mBranch->getHighlightedItem())
	{
		// switch to keyboard navigation mode
		LLMenuGL::setKeyboardMode(TRUE);

		LLMenuItemGL* itemp = mBranch->highlightNextItem(NULL);
		if (itemp)
		{
			return TRUE;
		}
	}

	return LLMenuItemGL::handleKeyHere(key, mask, called_from_parent);
}

void LLMenuItemBranchGL::openMenu()
{
	if (mBranch->getTornOff())
	{
		gFloaterView->bringToFront((LLFloater*)mBranch->getParent());
		// this might not be necessary, as torn off branches don't get focus and hence no highligth
		mBranch->highlightNextItem(NULL);
	}
	else if( !mBranch->getVisible() )
	{
		// get valid rectangle for menus
		const LLRect menu_region_rect = LLMenuGL::sMenuContainer->getMenuRect();

		mBranch->arrange();

		LLRect rect = mBranch->getRect();
		// calculate root-view relative position for branch menu
		S32 left = mRect.mRight;
		S32 top = mRect.mTop - mRect.mBottom;

		localPointToOtherView(left, top, &left, &top, mBranch->getParent());

		rect.setLeftTopAndSize( left, top,
								rect.getWidth(), rect.getHeight() );

		if (mBranch->getCanTearOff())
		{
			rect.translate(0, TEAROFF_SEPARATOR_HEIGHT_PIXELS);
		}
		mBranch->setRect( rect );
		S32 x = 0;
		S32 y = 0;
		mBranch->localPointToOtherView( 0, 0, &x, &y, mBranch->getParent() ); 
		S32 delta_x = 0;
		S32 delta_y = 0;
		if( y < menu_region_rect.mBottom )
		{
			delta_y = menu_region_rect.mBottom - y;
		}

		S32 menu_region_width = menu_region_rect.getWidth();
		if( x - menu_region_rect.mLeft > menu_region_width - rect.getWidth() )
		{
			// move sub-menu over to left side
			delta_x = llmax(-x, (-1 * (rect.getWidth() + mRect.getWidth())));
		}
		mBranch->translate( delta_x, delta_y );
		mBranch->setVisible( TRUE );
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemBranchDownGL
//
// The LLMenuItemBranchDownGL represents a menu item that has a
// sub-menu. This is used to make menu bar menus.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemBranchDownGL : public LLMenuItemBranchGL
{
protected:

public:
	LLMenuItemBranchDownGL( const LLString& name, const LLString& label, LLMenuGL* branch,
							KEY key = KEY_NONE, MASK mask = MASK_NONE );

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_MENU_ITEM_BRANCH_DOWN; }
	virtual LLString getWidgetTag() const { return LL_MENU_ITEM_BRANCH_DOWN_GL_TAG; }

	virtual LLString getType() const	{ return "menu"; }

	// returns the normal width of this control in pixels - this is
	// used for calculating the widest item, as well as for horizontal
	// arrangement.
	virtual U32 getNominalWidth( void );

	// called to rebuild the draw label
	virtual void buildDrawLabel( void );

	// handles opening, positioning, and arranging the menu branch associated with this item
	virtual void openMenu( void );

	// set the hover status (called by it's menu) and if the object is
	// active. This is used for behavior transfer.
	virtual void setHighlight( BOOL highlight );

	virtual BOOL isActive( void ) const;

	// LLView functionality
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask ) {return FALSE; }
	virtual void draw( void );
	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	
	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
};

LLMenuItemBranchDownGL::LLMenuItemBranchDownGL( const LLString& name,
												const LLString& label,
												LLMenuGL* branch, 
												KEY key, MASK mask ) :
	LLMenuItemBranchGL( name, label, branch, key, mask )
{
}

// returns the normal width of this control in pixels - this is used
// for calculating the widest item, as well as for horizontal
// arrangement.
U32 LLMenuItemBranchDownGL::getNominalWidth( void )
{
	U32 width = LEFT_PAD_PIXELS + LEFT_WIDTH_PIXELS + RIGHT_PAD_PIXELS;
	width += mFont->getWidth( mLabel.getWString().c_str() ); 
	return width;
}

// called to rebuild the draw label
void LLMenuItemBranchDownGL::buildDrawLabel( void )
{
	mDrawAccelLabel.clear();
	LLString st = mDrawAccelLabel;
	appendAcceleratorString( st );
	mDrawAccelLabel = st;
}

void LLMenuItemBranchDownGL::openMenu( void )
{
	if( mBranch->getVisible() && !mBranch->getTornOff() )
	{
		mBranch->setVisible( FALSE );
	}
	else
	{
		if (mBranch->getTornOff())
		{
			gFloaterView->bringToFront((LLFloater*)mBranch->getParent());
		}
		else
		{
			// We're showing the drop-down menu, so patch up its labels/rects
			mBranch->arrange();

			LLRect rect = mBranch->getRect();
			S32 left = 0;
			S32 top = mRect.mBottom;
			localPointToOtherView(left, top, &left, &top, mBranch->getParent());

			rect.setLeftTopAndSize( left, top,
									rect.getWidth(), rect.getHeight() );
			mBranch->setRect( rect );
			S32 x = 0;
			S32 y = 0;
			mBranch->localPointToScreen( 0, 0, &x, &y ); 
			S32 delta_x = 0;

			LLCoordScreen window_size;
			LLWindow* windowp = getWindow();
			windowp->getSize(&window_size);

			S32 window_width = window_size.mX;
			if( x > window_width - rect.getWidth() )
			{
				delta_x = (window_width - rect.getWidth()) - x;
			}
			mBranch->translate( delta_x, 0 );

			setHighlight(TRUE);
			mBranch->setVisible( TRUE );
		}


	}
}

// set the hover status (called by it's menu)
void LLMenuItemBranchDownGL::setHighlight( BOOL highlight )
{
	if (highlight == getHighlight()) return;

	if (highlight)
	{
		getMenu()->clearHoverItem();
	}
	mHighlight = highlight;
	if( !highlight)
	{
		if (mBranch->getTornOff())
		{
			((LLFloater*)mBranch->getParent())->setFocus(FALSE);
			mBranch->clearHoverItem();
		}
		else
		{
			mBranch->setVisible( FALSE );
		}
	}
}

BOOL LLMenuItemBranchDownGL::isActive() const
{
	// for top level menus, being open is sufficient to be considered 
	// active, because clicking on them with the mouse will open
	// them, without moving keyboard focus to them
	return isOpen();
}

BOOL LLMenuItemBranchDownGL::handleMouseDown( S32 x, S32 y, MASK mask )
{
	// switch to mouse control mode
	LLMenuGL::setKeyboardMode(FALSE);
	doIt();
	make_ui_sound("UISndClick");
	return TRUE;
}


BOOL LLMenuItemBranchDownGL::handleAcceleratorKey(KEY key, MASK mask)
{
	BOOL branch_visible = mBranch->getVisible();
	BOOL handled = mBranch->handleAcceleratorKey(key, mask);
	if (handled && !branch_visible && getVisible())
	{
		// flash this menu entry because we triggered an invisible menu item
		LLMenuHolderGL::setActivatedItem(this);
	}

	return handled;
}

BOOL LLMenuItemBranchDownGL::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL menu_open = mBranch->getVisible();
	// don't do keyboard navigation of top-level menus unless in keyboard mode, or menu expanded
	if (getHighlight() && getMenu()->getVisible() && (isActive() || LLMenuGL::getKeyboardMode()))
	{
		if (key == KEY_LEFT)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			LLMenuItemGL* itemp = getMenu()->highlightPrevItem(this);
			// open new menu only if previous menu was open
			if (itemp && itemp->getEnabled() && menu_open)
			{
				itemp->doIt();
			}

			return TRUE;
		}
		else if (key == KEY_RIGHT)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			LLMenuItemGL* itemp = getMenu()->highlightNextItem(this);
			// open new menu only if previous menu was open
			if (itemp && itemp->getEnabled() && menu_open)
			{
				itemp->doIt();
			}

			return TRUE;
		}
		else if (key == KEY_DOWN)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			if (getEnabled() && !isActive())
			{
				doIt();
			}
			mBranch->highlightNextItem(NULL);
			return TRUE;
		}
		else if (key == KEY_UP)
		{
			// switch to keyboard navigation mode
			LLMenuGL::setKeyboardMode(TRUE);

			if (getEnabled() && !isActive())
			{
				doIt();
			}
			mBranch->highlightPrevItem(NULL);
			return TRUE;
		}
	}

	return FALSE;
}

void LLMenuItemBranchDownGL::draw( void )
{
	//FIXME: try removing this
	if (mBranch->getVisible() && !mBranch->getTornOff())
	{
		setHighlight(TRUE);
	}

	if( getHighlight() )
	{
		glColor4fv( sHighlightBackground.mV );
		gl_rect_2d( 0, mRect.getHeight(), mRect.getWidth(), 0 );
	}

	U8 font_style = mStyle;
	if (LLMenuItemGL::sDropShadowText && getEnabled() && !mDrawTextDisabled )
	{
		font_style |= LLFontGL::DROP_SHADOW;
	}

	LLColor4 color;
	if (getHighlight())
	{
		color = sHighlightForeground;
	}
	else if( mEnabled )
	{
		color = sEnabledColor;
	}
	else
	{
		color = sDisabledColor;
	}
	mFont->render( mLabel.getWString(), 0, (F32)mRect.getWidth() / 2.f, (F32)LABEL_BOTTOM_PAD_PIXELS, color,
				   LLFontGL::HCENTER, LLFontGL::BOTTOM, font_style );


	// underline navigation key only when keyboard navigation has been initiated
	if (getMenu()->jumpKeysActive() && LLMenuGL::getKeyboardMode())
	{
		LLString upper_case_label = mLabel.getString();
		LLString::toUpper(upper_case_label);
		std::string::size_type offset = upper_case_label.find(mJumpKey);
		if (offset != std::string::npos)
		{
			S32 x_offset = llround((F32)mRect.getWidth() / 2.f - mFont->getWidthF32(mLabel.getString(), 0, S32_MAX) / 2.f);
			S32 x_begin = x_offset + mFont->getWidth(mLabel, 0, offset);
			S32 x_end = x_offset + mFont->getWidth(mLabel, 0, offset + 1);
			gl_line_2d(x_begin, LABEL_BOTTOM_PAD_PIXELS, x_end, LABEL_BOTTOM_PAD_PIXELS);
		}
	}

	// reset every frame so that we only show highlight 
	// when we get hover events on that frame
	mGotHover = FALSE;
}

///============================================================================
/// Class LLMenuGL
///============================================================================

// Default constructor
LLMenuGL::LLMenuGL( const LLString& name, const LLString& label, LLViewHandle parent_floater_handle )
:	LLUICtrl( name, LLRect(), FALSE, NULL, NULL ),
	mBackgroundColor( sDefaultBackgroundColor ),
	mBgVisible( TRUE ),
	mParentMenuItem( NULL ),
	mLabel( label ),
	mDropShadowed( TRUE ),
	mHorizontalLayout( FALSE ),
	mKeepFixedSize( FALSE ),
	mLastMouseX(0),
	mLastMouseY(0),
	mMouseVelX(0),
	mMouseVelY(0),
	mTornOff(FALSE),
	mTearOffItem(NULL),
	mSpilloverBranch(NULL),
	mSpilloverMenu(NULL),
	mParentFloaterHandle(parent_floater_handle),
	mJumpKey(KEY_NONE)
{
	mFadeTimer.stop();
	setCanTearOff(TRUE, parent_floater_handle);
	setTabStop(FALSE);
}

LLMenuGL::LLMenuGL( const LLString& label, LLViewHandle parent_floater_handle )
:	LLUICtrl( label, LLRect(), FALSE, NULL, NULL ),
	mBackgroundColor( sDefaultBackgroundColor ),
	mBgVisible( TRUE ),
	mParentMenuItem( NULL ),
	mLabel( label ),
	mDropShadowed( TRUE ),
	mHorizontalLayout( FALSE ),
	mKeepFixedSize( FALSE ),
	mLastMouseX(0),
	mLastMouseY(0),
	mMouseVelX(0),
	mMouseVelY(0),
	mTornOff(FALSE),
	mTearOffItem(NULL),
	mSpilloverBranch(NULL),
	mSpilloverMenu(NULL),
	mParentFloaterHandle(parent_floater_handle),
	mJumpKey(KEY_NONE)
{
	mFadeTimer.stop();
	setCanTearOff(TRUE, parent_floater_handle);
	setTabStop(FALSE);
}

// Destroys the object
LLMenuGL::~LLMenuGL( void )
{
	// delete the branch, as it might not be in view hierarchy
	// leave the menu, because it is always in view hierarchy
	delete mSpilloverBranch;
	mJumpKeys.clear();
}

void LLMenuGL::setCanTearOff(BOOL tear_off, LLViewHandle parent_floater_handle )
{
	if (tear_off && mTearOffItem == NULL)
	{
		mTearOffItem = new LLMenuItemTearOffGL(parent_floater_handle);
		mItems.insert(mItems.begin(), mTearOffItem);
		addChildAtEnd(mTearOffItem);
		arrange();
	}
	else if (!tear_off && mTearOffItem != NULL)
	{
		mItems.remove(mTearOffItem);
		removeChild(mTearOffItem);
		delete mTearOffItem;
		mTearOffItem = NULL;
		arrange();
	}
}

// virtual
LLXMLNodePtr LLMenuGL::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();

	// Attributes

	node->createChild("opaque", TRUE)->setBoolValue(mBgVisible);

	node->createChild("drop_shadow", TRUE)->setBoolValue(mDropShadowed);

	node->createChild("tear_off", TRUE)->setBoolValue((mTearOffItem != NULL));

	if (mBgVisible)
	{
		// TomY TODO: this should save out the color control name
		node->createChild("color", TRUE)->setFloatValue(4, mBackgroundColor.mV);
	}

	// Contents
	item_list_t::const_iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLView* child = (*item_iter);
		LLMenuItemGL* item = (LLMenuItemGL*)child;

		LLXMLNodePtr child_node = item->getXML();

		node->addChild(child_node);
	}

	return node;
}

void LLMenuGL::parseChildXML(LLXMLNodePtr child, LLView *parent, LLUICtrlFactory *factory)
{
	if (child->hasName(LL_MENU_GL_TAG))
	{
		// SUBMENU
		LLMenuGL *submenu = (LLMenuGL*)LLMenuGL::fromXML(child, parent, factory);
		appendMenu(submenu);
		if (LLMenuGL::sMenuContainer != NULL)
		{
			submenu->updateParent(LLMenuGL::sMenuContainer);
		}
		else
		{
			submenu->updateParent(parent);
		}
	}
	else if (child->hasName(LL_MENU_ITEM_CALL_GL_TAG) || 
			child->hasName(LL_MENU_ITEM_CHECK_GL_TAG) || 
			child->hasName(LL_MENU_ITEM_SEPARATOR_GL_TAG))
	{
		LLMenuItemGL *item = NULL;

		LLString type;
		LLString item_name;
		LLString source_label;
		LLString item_label;
		KEY		 jump_key = KEY_NONE;

		child->getAttributeString("type", type);
		child->getAttributeString("name", item_name);
		child->getAttributeString("label", source_label);

		// parse jump key out of label
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("_");
		tokenizer tokens(source_label, sep);
		tokenizer::iterator token_iter;
		S32 token_count = 0;
		for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
		{
			item_label += (*token_iter);
			if (token_count > 0)
			{
				jump_key = (*token_iter).c_str()[0];
			}
			++token_count;
		}


		if (child->hasName(LL_MENU_ITEM_SEPARATOR_GL_TAG))
		{
			appendSeparator(item_name);
		}
		else
		{
			// ITEM
			if (child->hasName(LL_MENU_ITEM_CALL_GL_TAG) || 
				child->hasName(LL_MENU_ITEM_CHECK_GL_TAG))
			{
				MASK mask = 0;
				LLString shortcut;
				child->getAttributeString("shortcut", shortcut);
				if (shortcut.find("control") != shortcut.npos)
				{
					mask |= MASK_CONTROL;
				}
				if (shortcut.find("alt") != shortcut.npos)
				{
					mask |= MASK_ALT;
				}
				if (shortcut.find("shift") != shortcut.npos)
				{
					mask |= MASK_SHIFT;
				}
				S32 pipe_pos = shortcut.rfind("|");
				LLString key_str = shortcut.substr(pipe_pos+1);

				KEY key = KEY_NONE;
				LLKeyboard::keyFromString(key_str, &key);

				LLMenuItemCallGL *new_item;
				LLXMLNodePtr call_child;

				if (child->hasName(LL_MENU_ITEM_CHECK_GL_TAG))
				{
					LLString control_name;
					child->getAttributeString("control_name", control_name);

					new_item = new LLMenuItemCheckGL(item_name, item_label, 0, 0, control_name, parent, 0, key, mask);

					for (call_child = child->getFirstChild(); call_child.notNull(); call_child = call_child->getNextSibling())
					{
						if (call_child->hasName("on_check"))
						{
							LLString callback_name;
							LLString control_name = "";
							if (call_child->hasAttribute("function"))
							{
								call_child->getAttributeString("function", callback_name);

								control_name = callback_name;

								LLString callback_data = item_name;
								if (call_child->hasAttribute("userdata"))
								{
									call_child->getAttributeString("userdata", callback_data);
									if (!callback_data.empty())
									{
										control_name = llformat("%s(%s)", callback_name.c_str(), callback_data.c_str());
									}
								}

								LLSD userdata;
								userdata["control"] = control_name;
								userdata["data"] = callback_data;

								LLSimpleListener* callback = parent->getListenerByName(callback_name);

								if (!callback) continue;

								new_item->addListener(callback, "on_build", userdata);
							}
							else if (call_child->hasAttribute("control"))
							{
								call_child->getAttributeString("control", control_name);
							}
							else
							{
								continue;
							}
							LLControlBase *control = parent->findControl(control_name);
							if (!control)
							{
								parent->addBoolControl(control_name, FALSE);
							}
							((LLMenuItemCheckGL*)new_item)->setCheckedControl(control_name, parent);
						}
					}
				}
				else
				{
					new_item = new LLMenuItemCallGL(item_name, item_label, 0, 0, 0, 0, key, mask);
				}

				for (call_child = child->getFirstChild(); call_child.notNull(); call_child = call_child->getNextSibling())
				{
					if (call_child->hasName("on_click"))
					{
						LLString callback_name;
						call_child->getAttributeString("function", callback_name);

						LLString callback_data = item_name;
						if (call_child->hasAttribute("userdata"))
						{
							call_child->getAttributeString("userdata", callback_data);
						}

						LLSimpleListener* callback = parent->getListenerByName(callback_name);

						if (!callback) continue;

						new_item->addListener(callback, "on_click", callback_data);
					}
					if (call_child->hasName("on_enable"))
					{
						LLString callback_name;
						LLString control_name = "";
						if (call_child->hasAttribute("function"))
						{
							call_child->getAttributeString("function", callback_name);

							control_name = callback_name;

							LLString callback_data = "";
							if (call_child->hasAttribute("userdata"))
							{
								call_child->getAttributeString("userdata", callback_data);
								if (!callback_data.empty())
								{
									control_name = llformat("%s(%s)", callback_name.c_str(), callback_data.c_str());
								}
							}

							LLSD userdata;
							userdata["control"] = control_name;
							userdata["data"] = callback_data;

							LLSimpleListener* callback = parent->getListenerByName(callback_name);

							if (!callback) continue;

							new_item->addListener(callback, "on_build", userdata);
						}
						else if (call_child->hasAttribute("control"))
						{
							call_child->getAttributeString("control", control_name);
						}
						else
						{
							continue;
						}
						new_item->setEnabledControl(control_name, parent);
					}
					if (call_child->hasName("on_visible"))
					{
						LLString callback_name;
						LLString control_name = "";
						if (call_child->hasAttribute("function"))
						{
							call_child->getAttributeString("function", callback_name);

							control_name = callback_name;

							LLString callback_data = "";
							if (call_child->hasAttribute("userdata"))
							{
								call_child->getAttributeString("userdata", callback_data);
								if (!callback_data.empty())
								{
									control_name = llformat("%s(%s)", callback_name.c_str(), callback_data.c_str());
								}
							}

							LLSD userdata;
							userdata["control"] = control_name;
							userdata["data"] = callback_data;

							LLSimpleListener* callback = parent->getListenerByName(callback_name);

							if (!callback) continue;

							new_item->addListener(callback, "on_build", userdata);
						}
						else if (call_child->hasAttribute("control"))
						{
							call_child->getAttributeString("control", control_name);
						}
						else
						{
							continue;
						}
						new_item->setVisibleControl(control_name, parent);
					}
				}
				item = new_item;
				item->setLabel(item_label);
				if (jump_key != KEY_NONE)
					item->setJumpKey(jump_key);
			}

			if (item != NULL)
			{
				append(item);
			}
		}
	}
}

// are we the childmost active menu and hence our jump keys should be enabled?
// or are we a free-standing torn-off menu (which uses jump keys too)
BOOL LLMenuGL::jumpKeysActive()
{
	LLMenuItemGL* highlighted_item = getHighlightedItem();
	BOOL active = getVisible() && getEnabled();
	if (getTornOff())
	{
		// activation of jump keys on torn off menus controlled by keyboard focus
		active = active && ((LLFloater*)getParent())->hasFocus();

	}
	else
	{
		// Are we the terminal active menu?
		// Yes, if parent menu item deems us to be active (just being visible is sufficient for top-level menus)
		// and we don't have a highlighted menu item pointing to an active sub-menu
		active = active && (!getParentMenuItem() || getParentMenuItem()->isActive()) // I have a parent that is active...
		                && (!highlighted_item || !highlighted_item->isActive()); //... but no child that is active
	}
	return active;
}

BOOL LLMenuGL::isOpen()
{
	if (getTornOff())
	{
		LLMenuItemGL* itemp = getHighlightedItem();
		// if we have an open sub-menu, then we are considered part of 
		// the open menu chain even if we don't have focus
		if (itemp && itemp->isOpen())
		{
			return TRUE;
		}
		// otherwise we are only active if we have keyboard focus
		return ((LLFloater*)getParent())->hasFocus();
	}
	else
	{
		// normally, menus are hidden as soon as the user focuses
		// on another menu, so just use the visibility criterion
		return getVisible();
	}
}
// static
LLView* LLMenuGL::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("menu");
	node->getAttributeString("name", name);

	LLString label = name;
	node->getAttributeString("label", label);

	// parse jump key out of label
	LLString new_menu_label;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("_");
	tokenizer tokens(label, sep);
	tokenizer::iterator token_iter;

	KEY jump_key = KEY_NONE;
	S32 token_count = 0;
	for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		new_menu_label += (*token_iter);
		if (token_count > 0)
		{
			jump_key = (*token_iter).c_str()[0];
		}
		++token_count;
	}

	BOOL opaque = FALSE;
	node->getAttributeBOOL("opaque", opaque);

	LLMenuGL *menu = new LLMenuGL(name, new_menu_label);

	menu->setJumpKey(jump_key);

	BOOL tear_off = FALSE;
	node->getAttributeBOOL("tear_off", tear_off);
	menu->setCanTearOff(tear_off);

	if (node->hasAttribute("drop_shadow"))
	{
		BOOL drop_shadow = FALSE;
		node->getAttributeBOOL("drop_shadow", drop_shadow);
		menu->setDropShadowed(drop_shadow);
	}

	menu->setBackgroundVisible(opaque);
	LLColor4 color(0,0,0,1);
	if (opaque && LLUICtrlFactory::getAttributeColor(node,"color", color))
	{
		menu->setBackgroundColor(color);
	}

	BOOL create_jump_keys = FALSE;
	node->getAttributeBOOL("create_jump_keys", create_jump_keys);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		menu->parseChildXML(child, parent, factory);
	}

	if (create_jump_keys)
	{
		menu->createJumpKeys();
	}
	return menu;
}

// control the color scheme
void LLMenuGL::setDefaultBackgroundColor( const LLColor4& color )
{
	sDefaultBackgroundColor = color;
}

void LLMenuGL::setBackgroundColor( const LLColor4& color )
{
	mBackgroundColor = color;
}

LLColor4 LLMenuGL::getBackgroundColor()
{
	return mBackgroundColor;
}


// rearrange the child rects so they fit the shape of the menu.
void LLMenuGL::arrange( void )
{
	// calculate the height & width, and set our rect based on that
	// information.
	LLRect initial_rect = mRect;

	U32 width = 0, height = MENU_ITEM_PADDING;

	cleanupSpilloverBranch();

	if( mItems.size() ) 
	{
		const LLRect menu_region_rect = LLMenuGL::sMenuContainer ? LLMenuGL::sMenuContainer->getMenuRect() : LLRect(0, S32_MAX, S32_MAX, 0);

		U32 max_width = menu_region_rect.getWidth();
		U32 max_height = menu_region_rect.getHeight();
		// *FIX: create the item first and then ask for its dimensions?
		S32 spillover_item_width = PLAIN_PAD_PIXELS + LLFontGL::sSansSerif->getWidth( "More" );
		S32 spillover_item_height = llround(LLFontGL::sSansSerif->getLineHeight()) + MENU_ITEM_PADDING;

		if (mHorizontalLayout)
		{
			item_list_t::iterator item_iter;
			for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
			{
				if ((*item_iter)->getVisible())
				{
					if (!getTornOff() && width + (*item_iter)->getNominalWidth() > max_width - spillover_item_width)
					{
						// no room for any more items
						createSpilloverBranch();

						item_list_t::iterator spillover_iter;
						for (spillover_iter = item_iter; spillover_iter != mItems.end(); ++spillover_iter)
						{
							LLMenuItemGL* itemp = (*spillover_iter);
							removeChild(itemp);
							mSpilloverMenu->append(itemp);
						}
						mItems.erase(item_iter, mItems.end());
						
						mItems.push_back(mSpilloverBranch);
						addChild(mSpilloverBranch);
						height = llmax(height, mSpilloverBranch->getNominalHeight());
						width += mSpilloverBranch->getNominalWidth();

						break;
					}
					else
					{
						// track our rect
						height = llmax(height, (*item_iter)->getNominalHeight());
						width += (*item_iter)->getNominalWidth();
					}
				}
			}
		}
		else
		{
			item_list_t::iterator item_iter;
			for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
			{
				if ((*item_iter)->getVisible())
				{
					if (!getTornOff() && height + (*item_iter)->getNominalHeight() > max_height - spillover_item_height)
					{
						// no room for any more items
						createSpilloverBranch();

						item_list_t::iterator spillover_iter;
						for (spillover_iter= item_iter; spillover_iter != mItems.end(); ++spillover_iter)
						{
							LLMenuItemGL* itemp = (*spillover_iter);
							removeChild(itemp);
							mSpilloverMenu->append(itemp);
						}
						mItems.erase(item_iter, mItems.end());
						mItems.push_back(mSpilloverBranch);
						addChild(mSpilloverBranch);
						height += mSpilloverBranch->getNominalHeight();
						width = llmax( width, mSpilloverBranch->getNominalWidth() );

						break;
					}
					else
					{
						// track our rect
						height += (*item_iter)->getNominalHeight();
						width = llmax( width, (*item_iter)->getNominalWidth() );
					}
				}
			}
		}

		mRect.mRight = mRect.mLeft + width;
		mRect.mTop = mRect.mBottom + height;

		S32 cur_height = (S32)llmin(max_height, height);
		S32 cur_width = 0;
		item_list_t::iterator item_iter;
		for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
		{
			if ((*item_iter)->getVisible())
			{
				// setup item rect to hold label
				LLRect rect;
				if (mHorizontalLayout)
				{
					rect.setLeftTopAndSize( cur_width, height, (*item_iter)->getNominalWidth(), height);
					cur_width += (*item_iter)->getNominalWidth();
				}
				else
				{
					rect.setLeftTopAndSize( 0, cur_height, width, (*item_iter)->getNominalHeight());
					cur_height -= (*item_iter)->getNominalHeight();
				}
				(*item_iter)->setRect( rect );
				(*item_iter)->buildDrawLabel();
			}
		}
	}
	if (mKeepFixedSize)
	{
		reshape(initial_rect.getWidth(), initial_rect.getHeight());
	}
}

void LLMenuGL::createSpilloverBranch()
{
	if (!mSpilloverBranch)
	{
		// should be NULL but delete anyway
		delete mSpilloverMenu;
		// technically, you can't tear off spillover menus, but we're passing the handle
		// along just to be safe
		mSpilloverMenu = new LLMenuGL("More", "More", mParentFloaterHandle);
		mSpilloverMenu->updateParent(LLMenuGL::sMenuContainer);
		// Inherit colors
		mSpilloverMenu->setBackgroundColor( mBackgroundColor );
		mSpilloverMenu->setCanTearOff(FALSE);

		mSpilloverBranch = new LLMenuItemBranchGL("More", "More", mSpilloverMenu);
		mSpilloverBranch->setFontStyle(LLFontGL::ITALIC);
	}
}

void LLMenuGL::cleanupSpilloverBranch()
{
	if (mSpilloverBranch && mSpilloverBranch->getParent() == this)
	{
		// head-recursion to propagate items back up to root menu
		mSpilloverMenu->cleanupSpilloverBranch();

		removeChild(mSpilloverBranch);

		item_list_t::iterator found_iter = std::find(mItems.begin(), mItems.end(), mSpilloverBranch);
		if (found_iter != mItems.end())
		{
			mItems.erase(found_iter);
		}

		// pop off spillover items
		while (mSpilloverMenu->getItemCount())
		{
			LLMenuItemGL* itemp = mSpilloverMenu->getItem(0);
			mSpilloverMenu->removeChild(itemp);
			mSpilloverMenu->mItems.erase(mSpilloverMenu->mItems.begin());
			// put them at the end of our own list
			mItems.push_back(itemp);
			addChild(itemp);
		}
	}
}

void LLMenuGL::createJumpKeys()
{
	mJumpKeys.clear();

	std::set<LLString> unique_words;
	std::set<LLString> shared_words;

	item_list_t::iterator item_it;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");

	for(item_it = mItems.begin(); item_it != mItems.end(); ++item_it)
	{
		LLString uppercase_label = (*item_it)->getLabel();
		LLString::toUpper(uppercase_label);

		tokenizer tokens(uppercase_label, sep);
		tokenizer::iterator token_iter;
		for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
		{
			if (unique_words.find(*token_iter) != unique_words.end())
			{
				// this word exists in more than one menu instance
				shared_words.insert(*token_iter);
			}
			else
			{
				// we have a new word, keep track of it
				unique_words.insert(*token_iter);
			}
		}
	}

	// pre-assign specified jump keys
	for(item_it = mItems.begin(); item_it != mItems.end(); ++item_it)
	{
		KEY jump_key = (*item_it)->getJumpKey();
		if(jump_key != KEY_NONE)
		{
			if (mJumpKeys.find(jump_key) == mJumpKeys.end())
			{
				mJumpKeys.insert(std::pair<KEY, LLMenuItemGL*>(jump_key, (*item_it)));
			}
			else
			{
				// this key is already spoken for, 
				// so we need to reassign it below
				(*item_it)->setJumpKey(KEY_NONE);
			}
		}
	}

	for(item_it = mItems.begin(); item_it != mItems.end(); ++item_it)
	{
		// skip over items that already have assigned jump keys
		if ((*item_it)->getJumpKey() != KEY_NONE)
		{
			continue;
		}
		LLString uppercase_label = (*item_it)->getLabel();
		LLString::toUpper(uppercase_label);

		tokenizer tokens(uppercase_label, sep);
		tokenizer::iterator token_iter;

		BOOL found_key = FALSE;
		for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
		{
			LLString uppercase_word = *token_iter;

			// this word is not shared with other menu entries...
			if (shared_words.find(*token_iter) == shared_words.end())
			{
				S32 i;
				for(i = 0; i < (S32)uppercase_word.size(); i++)
				{
					char jump_key = uppercase_word[i];
					
					if (LLStringOps::isDigit(jump_key) || LLStringOps::isUpper(jump_key) &&
						mJumpKeys.find(jump_key) == mJumpKeys.end())
					{
						mJumpKeys.insert(std::pair<KEY, LLMenuItemGL*>(jump_key, (*item_it)));
						(*item_it)->setJumpKey(jump_key);
						found_key = TRUE;
						break;
					}
				}
			}
			if (found_key)
			{
				break;
			}
		}
	}
}

// remove all items on the menu
void LLMenuGL::empty( void )
{
	mItems.clear();

	deleteAllChildren();
	
}

// Adjust rectangle of the menu
void LLMenuGL::setLeftAndBottom(S32 left, S32 bottom)
{
	mRect.mLeft = left;
	mRect.mBottom = bottom;
	arrange();
}

BOOL LLMenuGL::handleJumpKey(KEY key)
{
	// must perform case-insensitive comparison, so just switch to uppercase input key
	key = toupper(key);
	navigation_key_map_t::iterator found_it = mJumpKeys.find(key);
	if(found_it != mJumpKeys.end() && found_it->second->getEnabled())
	{
		// switch to keyboard navigation mode
		LLMenuGL::setKeyboardMode(TRUE);

		// force highlight to close old menus and any open sub-menus

		//clearHoverItem();
		// force highlight to close old menus and open and sub-menus
		found_it->second->setHighlight(TRUE);
		found_it->second->doIt();

	}
	// if we are navigating the menus, we need to eat the keystroke
	// so rest of UI doesn't handle it
	return TRUE;
}


// Add the menu item to this menu.
BOOL LLMenuGL::append( LLMenuItemGL* item )
{
	mItems.push_back( item );
	addChild( item );
	arrange();
	return TRUE;
}

// add a separator to this menu
BOOL LLMenuGL::appendSeparator( const LLString &separator_name )
{
	LLMenuItemGL* separator = new LLMenuItemSeparatorGL(separator_name);
	return append( separator );
}

// add a menu - this will create a cascading menu
BOOL LLMenuGL::appendMenu( LLMenuGL* menu )
{
	if( menu == this )
	{
		llerrs << "** Attempt to attach menu to itself. This is certainly "
			   << "a logic error." << llendl;
	}
	BOOL success = TRUE;

	LLMenuItemBranchGL* branch = NULL;
	branch = new LLMenuItemBranchGL( menu->getName(), menu->getLabel(), menu );
	branch->setJumpKey(menu->getJumpKey());
	success &= append( branch );

	// Inherit colors
	menu->setBackgroundColor( mBackgroundColor );

	return success;
}

void LLMenuGL::setEnabledSubMenus(BOOL enable)
{
	setEnabled(enable);
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		(*item_iter)->setEnabledSubMenus( enable );
	}
}

// setItemEnabled() - pass the label and the enable flag for a menu
// item. TRUE will make sure it's enabled, FALSE will disable it.
void LLMenuGL::setItemEnabled( const LLString& name, BOOL enable )
{
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		if( (*item_iter)->getName() == name )
		{
			(*item_iter)->setEnabled( enable );
			(*item_iter)->setEnabledSubMenus( enable );
			break;
		}
	}
}

void LLMenuGL::setItemVisible( const LLString& name, BOOL visible )
{
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		if( (*item_iter)->getName() == name )
		{
			(*item_iter)->setVisible( visible );
			break;
		}
	}
}

void LLMenuGL::setItemLastSelected(LLMenuItemGL* item)
{
	if (getVisible())
	{
		LLMenuHolderGL::setActivatedItem(item);
	}

	// fix the checkmarks
	item->buildDrawLabel();
}

//  Set whether drop shadowed 
void LLMenuGL::setDropShadowed( const BOOL shadowed )
{
	mDropShadowed = shadowed;
}

void LLMenuGL::setTornOff(BOOL torn_off)
{ 
	mTornOff = torn_off;
}

U32 LLMenuGL::getItemCount()
{
	return mItems.size();
}

LLMenuItemGL* LLMenuGL::getItem(S32 number)
{
	if (number >= 0 && number < (S32)mItems.size())
	{
		item_list_t::iterator item_iter;
		for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
		{
			if (number == 0)
			{
				return (*item_iter);
			}
			number--;
		}
	}
	return NULL;
}

LLMenuItemGL* LLMenuGL::getHighlightedItem()
{
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		if ((*item_iter)->getHighlight())
		{
			return (*item_iter);
		}
	}
	return NULL;
}

LLMenuItemGL* LLMenuGL::highlightNextItem(LLMenuItemGL* cur_item, BOOL skip_disabled)
{
	// highlighting first item on a torn off menu is the
	// same as giving focus to it
	if (!cur_item && getTornOff())
	{
		((LLFloater*)getParent())->setFocus(TRUE);
	}

	item_list_t::iterator cur_item_iter;
	for (cur_item_iter = mItems.begin(); cur_item_iter != mItems.end(); ++cur_item_iter)
	{
		if( (*cur_item_iter) == cur_item)
		{
			break;
		}
	}

	item_list_t::iterator next_item_iter;
	if (cur_item_iter == mItems.end())
	{
		next_item_iter = mItems.begin();
	}
	else
	{
		next_item_iter = cur_item_iter;
		next_item_iter++;
		if (next_item_iter == mItems.end())
		{
			next_item_iter = mItems.begin();
		}
	}

	// when first highlighting a menu, skip over tear off menu item
	if (mTearOffItem && !cur_item)
	{
		// we know the first item is the tear off menu item
		cur_item_iter = mItems.begin();
		next_item_iter++;
		if (next_item_iter == mItems.end())
		{
			next_item_iter = mItems.begin();
		}
	}

	while(1)
	{
		// skip separators and disabled items
		if ((*next_item_iter)->getEnabled() && (*next_item_iter)->getName() != SEPARATOR_NAME)
		{
			if (cur_item)
			{
				cur_item->setHighlight(FALSE);
			}
			(*next_item_iter)->setHighlight(TRUE);
			return (*next_item_iter);
		}


		if (!skip_disabled || next_item_iter == cur_item_iter)
		{
			break;
		}

		next_item_iter++;
		if (next_item_iter == mItems.end())
		{
			if (cur_item_iter == mItems.end())
			{
				break;
			}
			next_item_iter = mItems.begin();
		}
	}

	return NULL;
}

LLMenuItemGL* LLMenuGL::highlightPrevItem(LLMenuItemGL* cur_item, BOOL skip_disabled)
{
	// highlighting first item on a torn off menu is the
	// same as giving focus to it
	if (!cur_item && getTornOff())
	{
		((LLFloater*)getParent())->setFocus(TRUE);
	}

	item_list_t::reverse_iterator cur_item_iter;
	for (cur_item_iter = mItems.rbegin(); cur_item_iter != mItems.rend(); ++cur_item_iter)
	{
		if( (*cur_item_iter) == cur_item)
		{
			break;
		}
	}

	item_list_t::reverse_iterator prev_item_iter;
	if (cur_item_iter == mItems.rend())
	{
		prev_item_iter = mItems.rbegin();
	}
	else
	{
		prev_item_iter = cur_item_iter;
		prev_item_iter++;
		if (prev_item_iter == mItems.rend())
		{
			prev_item_iter = mItems.rbegin();
		}
	}

	while(1)
	{
		// skip separators and disabled items
		if ((*prev_item_iter)->getEnabled() && (*prev_item_iter)->getName() != SEPARATOR_NAME)
		{
			(*prev_item_iter)->setHighlight(TRUE);
			return (*prev_item_iter);
		}

		if (!skip_disabled || prev_item_iter == cur_item_iter)
		{
			break;
		}

		prev_item_iter++;
		if (prev_item_iter == mItems.rend())
		{
			if (cur_item_iter == mItems.rend())
			{
				break;
			}

			prev_item_iter = mItems.rbegin();
		}
	}

	return NULL;
}

void LLMenuGL::buildDrawLabels()
{
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		(*item_iter)->buildDrawLabel();
	}
}

void LLMenuGL::updateParent(LLView* parentp)
{
	if (getParent())
	{
		getParent()->removeChild(this);
	}
	parentp->addChild(this);
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		(*item_iter)->updateBranchParent(parentp);
	}
}

// LLView functionality
BOOL LLMenuGL::handleKey( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	// Pass down even if not visible
	if( mEnabled && called_from_parent )
	{
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (viewp->handleKey(key, mask, TRUE))
			{
				handled = TRUE;
				break;
			}
		}
	}

	if( !handled )
	{
		handled = handleKeyHere( key, mask, called_from_parent );
		if (handled && LLView::sDebugKeys)
		{
			llinfos << "Key handled by " << getName() << llendl;
		}
	}

	return handled;
}

BOOL LLMenuGL::handleAcceleratorKey(KEY key, MASK mask)
{
	// Pass down even if not visible
	item_list_t::iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLMenuItemGL* itemp = *item_iter;
		if (itemp->handleAcceleratorKey(key, mask))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMenuGL::handleUnicodeCharHere( llwchar uni_char, BOOL called_from_parent )
{
	if (jumpKeysActive())
	{
		return handleJumpKey((KEY)uni_char);
	}
	return FALSE;
}

BOOL LLMenuGL::handleHover( S32 x, S32 y, MASK mask )
{
	// leave submenu in place if slope of mouse < MAX_MOUSE_SLOPE_SUB_MENU
	BOOL no_mouse_data = mLastMouseX == 0 && mLastMouseY == 0;
	S32 mouse_delta_x = no_mouse_data ? 0 : x - mLastMouseX;
	S32 mouse_delta_y = no_mouse_data ? 0 : y - mLastMouseY;
	LLVector2 mouse_dir((F32)mouse_delta_x, (F32)mouse_delta_y);
	mouse_dir.normVec();
	LLVector2 mouse_avg_dir((F32)mMouseVelX, (F32)mMouseVelY);
	mouse_avg_dir.normVec();
	F32 interp = 0.5f * (llclamp(mouse_dir * mouse_avg_dir, 0.f, 1.f));
	mMouseVelX = llround(lerp((F32)mouse_delta_x, (F32)mMouseVelX, interp));
	mMouseVelY = llround(lerp((F32)mouse_delta_y, (F32)mMouseVelY, interp));
	mLastMouseX = x;
	mLastMouseY = y;

	// don't change menu focus unless mouse is moving or alt key is not held down
	if ((llabs(mMouseVelX) > 0 || 
			llabs(mMouseVelY) > 0) &&
		(!mHasSelection ||
		//(mouse_delta_x == 0 && mouse_delta_y == 0) ||
		(mMouseVelX < 0) ||
		llabs((F32)mMouseVelY) / llabs((F32)mMouseVelX) > MAX_MOUSE_SLOPE_SUB_MENU))
	{
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->getRect().mLeft;
			S32 local_y = y - viewp->getRect().mBottom;
			if (!viewp->pointInView(local_x, local_y) && ((LLMenuItemGL*)viewp)->getHighlight())
			{
				// moving mouse always highlights new item
				if (mouse_delta_x != 0 || mouse_delta_y != 0)
				{
					((LLMenuItemGL*)viewp)->setHighlight(FALSE);
				}
			}
		}

		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->getRect().mLeft;
			S32 local_y = y - viewp->getRect().mBottom;
			//RN: always call handleHover to track mGotHover status
			// but only set highlight when mouse is moving
			if( viewp->getVisible() && 
				//RN: allow disabled items to be highlighted to preserve "active" menus when
				// moving mouse through them
				//viewp->getEnabled() && 
				viewp->pointInView(local_x, local_y) && 
				viewp->handleHover(local_x, local_y, mask))
			{
				// moving mouse always highlights new item
				if (mouse_delta_x != 0 || mouse_delta_y != 0)
				{
					((LLMenuItemGL*)viewp)->setHighlight(TRUE);
					LLMenuGL::setKeyboardMode(FALSE);
				}
				mHasSelection = TRUE;
			}
		}
	}
	getWindow()->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}

void LLMenuGL::draw( void )
{
	if (mDropShadowed && !mTornOff)
	{
		gl_drop_shadow(0, mRect.getHeight(), mRect.getWidth(), 0, 
			LLUI::sColorsGroup->getColor("ColorDropShadow"), 
			LLUI::sConfigGroup->getS32("DropShadowFloater") );
	}

	LLColor4 bg_color = mBackgroundColor;

	if( mBgVisible )
	{
		gl_rect_2d( 0, mRect.getHeight(), mRect.getWidth(), 0, mBackgroundColor );
	}
	LLView::draw();
}

void LLMenuGL::drawBackground(LLMenuItemGL* itemp, LLColor4& color)
{
	glColor4fv( color.mV );
	LLRect item_rect = itemp->getRect();
	gl_rect_2d( 0, item_rect.getHeight(), item_rect.getWidth(), 0);
}

void LLMenuGL::setVisible(BOOL visible)
{
	if (visible != getVisible())
	{
		if (!visible)
		{
			mFadeTimer.start();
			clearHoverItem();
			// reset last known mouse coordinates so
			// we don't spoof a mouse move next time we're opened
			mLastMouseX = 0;
			mLastMouseY = 0;
		}
		else
		{
			mHasSelection = FALSE;
			mFadeTimer.stop();
		}

		LLView::setVisible(visible);
	}
}

LLMenuGL* LLMenuGL::getChildMenuByName(const LLString& name, BOOL recurse) const
{
	LLView* view = getChildByName(name, recurse);
	if (view)
	{
		if (view->getWidgetType() == WIDGET_TYPE_MENU_ITEM_BRANCH)
		{
			LLMenuItemBranchGL *branch = (LLMenuItemBranchGL *)view;
			return branch->getBranch();
		}
		if (view->getWidgetType() == WIDGET_TYPE_MENU || view->getWidgetType() == WIDGET_TYPE_PIE_MENU)
		{
			return (LLMenuGL*)view;
		}
	}
	llwarns << "Child Menu " << name << " not found in menu " << mName << llendl;
	return NULL;
}

BOOL LLMenuGL::clearHoverItem()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLMenuItemGL* itemp = (LLMenuItemGL*)*child_it;
		if (itemp->getHighlight())
		{
			itemp->setHighlight(FALSE);
			return TRUE;
		}
	}		
	return FALSE;
}

void hide_top_view( LLView* view )
{
	if( view ) view->setVisible( FALSE );
}


// static
void LLMenuGL::showPopup(LLView* spawning_view, LLMenuGL* menu, S32 x, S32 y)
{
	const LLRect menu_region_rect = LLMenuGL::sMenuContainer->getMenuRect();

	const S32 HPAD = 2;
	LLRect rect = menu->getRect();
	//LLView* cur_view = spawning_view;
	S32 left = x + HPAD;
	S32 top = y;
	spawning_view->localPointToOtherView(left, top, &left, &top, menu->getParent());
	rect.setLeftTopAndSize( left, top,
							rect.getWidth(), rect.getHeight() );


	//rect.setLeftTopAndSize(x + HPAD, y, rect.getWidth(), rect.getHeight());
	menu->setRect( rect );

	S32 bottom;
	left = rect.mLeft;
	bottom = rect.mBottom;
	//menu->getParent()->localPointToScreen( rect.mLeft, rect.mBottom, 
	//									&left, &bottom ); 
	S32 delta_x = 0;
	S32 delta_y = 0;
	if( bottom < menu_region_rect.mBottom )
	{
		// At this point, we need to move the context menu to the
		// other side of the mouse.
		//delta_y = menu_region_rect.mBottom - bottom;
		delta_y = (rect.getHeight() + 2 * HPAD);
	}

	if( left > menu_region_rect.mRight - rect.getWidth() )
	{
		// At this point, we need to move the context menu to the
		// other side of the mouse.
		//delta_x = (window_width - rect.getWidth()) - x;
		delta_x = -(rect.getWidth() + 2 * HPAD);
	}
	menu->translate( delta_x, delta_y );
	menu->setVisible( TRUE );
}

//-----------------------------------------------------------------------------
// class LLPieMenuBranch
// A branch to another pie menu
//-----------------------------------------------------------------------------
class LLPieMenuBranch : public LLMenuItemGL
{
public:
	LLPieMenuBranch(const LLString& name, const LLString& label, LLPieMenu* branch,
					enabled_callback ecb, void* user_data);

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_PIE_MENU_BRANCH; }
	virtual LLString getWidgetTag() const { return LL_PIE_MENU_BRANCH_TAG; }

	// called to rebuild the draw label
	virtual void buildDrawLabel( void );

	// doIt() - do the primary funcationality of the menu item.
	virtual void doIt( void );

	LLPieMenu* getBranch() { return mBranch; }

protected:
	LLPieMenu* mBranch;
	enabled_callback mEnabledCallback;
	void* mUserData;
};

LLPieMenuBranch::LLPieMenuBranch(const LLString& name,
								 const LLString& label,
								 LLPieMenu* branch,
								 enabled_callback ecb,
								 void* user_data) 
:	LLMenuItemGL( name, label, KEY_NONE, MASK_NONE ),
	mBranch( branch ),
	mEnabledCallback( ecb ),
	mUserData(user_data)
{
	mBranch->hide(FALSE);
	mBranch->setParentMenuItem(this);
}

// called to rebuild the draw label
void LLPieMenuBranch::buildDrawLabel( void )
{
	if(mEnabledCallback)
	{
		setEnabled(mEnabledCallback(mUserData));
		mDrawTextDisabled = FALSE;
	}
	else
	{
		// default enablement is this -- if any of the subitems are
		// enabled, this item is enabled. JC
		U32 sub_count = mBranch->getItemCount();
		U32 i;
		BOOL any_enabled = FALSE;
		for (i = 0; i < sub_count; i++)
		{
			LLMenuItemGL* item = mBranch->getItem(i);
			item->buildDrawLabel();
			if (item->getEnabled() && !item->getDrawTextDisabled() )
			{
				any_enabled = TRUE;
				break;
			}
		}
		mDrawTextDisabled = !any_enabled;
		setEnabled(TRUE);
	}

	mDrawAccelLabel.clear();
	LLString st = mDrawAccelLabel;
	appendAcceleratorString( st );
	mDrawAccelLabel = st;
	
	// No special branch suffix
	mDrawBranchLabel.clear();
}

// doIt() - do the primary funcationality of the menu item.
void LLPieMenuBranch::doIt( void )
{
	LLPieMenu *parent = (LLPieMenu *)getParent();

	LLRect rect = parent->getRect();
	S32 center_x;
	S32 center_y;
	parent->localPointToScreen(rect.getWidth() / 2, rect.getHeight() / 2, &center_x, &center_y);

	parent->hide(FALSE);
	mBranch->show(	center_x, center_y, FALSE );
}

//-----------------------------------------------------------------------------
// class LLPieMenu
// A circular menu of items, icons, etc.
//-----------------------------------------------------------------------------
LLPieMenu::LLPieMenu(const LLString& name, const LLString& label)
:	LLMenuGL(name, label),
	mFirstMouseDown(FALSE),
	mUseInfiniteRadius(FALSE),
	mHoverItem(NULL),
	mHoverThisFrame(FALSE),
	mOuterRingAlpha(1.f),
	mCurRadius(0.f),
	mRightMouseDown(FALSE)
{ 
	LLMenuGL::setVisible(FALSE);
	setCanTearOff(FALSE);
}

LLPieMenu::LLPieMenu(const LLString& name)
:	LLMenuGL(name, name),
	mFirstMouseDown(FALSE),
	mUseInfiniteRadius(FALSE),
	mHoverItem(NULL),
	mHoverThisFrame(FALSE),
	mOuterRingAlpha(1.f),
	mCurRadius(0.f),
	mRightMouseDown(FALSE)
{ 
	LLMenuGL::setVisible(FALSE);
	setCanTearOff(FALSE);
}

// virtual
LLPieMenu::~LLPieMenu()
{ }


EWidgetType LLPieMenu::getWidgetType() const
{
	return WIDGET_TYPE_PIE_MENU;
}

LLString LLPieMenu::getWidgetTag() const
{
	return LL_PIE_MENU_TAG;
}

void LLPieMenu::initXML(LLXMLNodePtr node, LLView *context, LLUICtrlFactory *factory)
{
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName(LL_PIE_MENU_TAG))
		{
			// SUBMENU
			LLString name("menu");
			child->getAttributeString("name", name);
			LLString label(name);
			child->getAttributeString("label", label);

			LLPieMenu *submenu = new LLPieMenu(name, label);
			appendMenu(submenu);
			submenu->initXML(child, context, factory);
		}
		else
		{
			parseChildXML(child, context, factory);
		}
	}
}

// virtual
void LLPieMenu::setVisible(BOOL visible)
{
	if (!visible)
	{
		hide(FALSE);
	}
}

BOOL LLPieMenu::handleHover( S32 x, S32 y, MASK mask )
{
	// This is mostly copied from the llview class, but it continues
	// the hover handle code after a hover handler has been found.
	BOOL handled = FALSE;

	// If we got a hover event, we've already moved the cursor
	// for any menu shifts, so subsequent mouseup messages will be in the
	// correct position.  No need to correct them.
	//mShiftHoriz = 0;
	//mShiftVert = 0;

	// release mouse capture after short period of visibility if we're using a finite boundary
	// so that right click outside of boundary will trigger new pie menu
	if (gFocusMgr.getMouseCapture() == this && 
		!mRightMouseDown && 
		mShrinkBorderTimer.getStarted() && 
		mShrinkBorderTimer.getElapsedTimeF32() >= PIE_SHRINK_TIME)
	{
		gFocusMgr.setMouseCapture(NULL, NULL);
		mUseInfiniteRadius = FALSE;
	}

	LLMenuItemGL *item = pieItemFromXY( x, y );

	if (item && item->getEnabled())
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
		handled = TRUE;

		if (item != mHoverItem)
		{
			if (mHoverItem)
			{
				mHoverItem->setHighlight( FALSE );
			}
			mHoverItem = item;
			mHoverItem->setHighlight( TRUE );

			switch(pieItemIndexFromXY(x, y))
			{
			case 0:
				make_ui_sound("UISndPieMenuSliceHighlight0");
				break;
			case 1:
				make_ui_sound("UISndPieMenuSliceHighlight1");
				break;
			case 2:
				make_ui_sound("UISndPieMenuSliceHighlight2");
				break;
			case 3:
				make_ui_sound("UISndPieMenuSliceHighlight3");
				break;
			case 4:
				make_ui_sound("UISndPieMenuSliceHighlight4");
				break;
			case 5:
				make_ui_sound("UISndPieMenuSliceHighlight5");
				break;
			case 6:
				make_ui_sound("UISndPieMenuSliceHighlight6");
				break;
			case 7:
				make_ui_sound("UISndPieMenuSliceHighlight7");
				break;
			default:
				make_ui_sound("UISndPieMenuSliceHighlight0");
				break;
			}
		}
	}
	else
	{
		// clear out our selection
		if (mHoverItem)
		{
			mHoverItem->setHighlight(FALSE);
			mHoverItem = NULL;
		}
	}

	if( !handled && pointInView( x, y ) )
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
		handled = TRUE;
	}

	mHoverThisFrame = TRUE;

	return handled;
}

BOOL LLPieMenu::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	// The click was somewhere within our rectangle
	LLMenuItemGL *item = pieItemFromXY( x, y );

	if (item)
	{
		// lie to the item about where the click happened
		// to make sure it's within the item's rectangle
		handled = item->handleMouseDown( 0, 0, mask );
	}
	else if (!mRightMouseDown)
	{
		// call hidemenus to make sure transient selections get cleared
		((LLMenuHolderGL*)getParent())->hideMenus();
	}

	// always handle mouse down as mouse up will close open menus
	return handled;
}

BOOL LLPieMenu::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	mRightMouseDown = TRUE;

	// The click was somewhere within our rectangle
	LLMenuItemGL *item = pieItemFromXY( x, y );
	S32 delta_x = x /*+ mShiftHoriz*/ - getLocalRect().getCenterX();
	S32 delta_y = y /*+ mShiftVert*/ - getLocalRect().getCenterY();
	BOOL clicked_in_pie = ((delta_x * delta_x) + (delta_y * delta_y) < mCurRadius*mCurRadius) || mUseInfiniteRadius;

	// grab mouse if right clicking anywhere within pie (even deadzone in middle), to detect drag outside of pie
	if (clicked_in_pie)
	{
		// capture mouse cursor as if on initial menu show
		gFocusMgr.setMouseCapture(this, NULL);
		mShrinkBorderTimer.stop();
		mUseInfiniteRadius = TRUE;
		handled = TRUE;
	}
	
	if (item)
	{
		// lie to the item about where the click happened
		// to make sure it's within the item's rectangle
		if (item->handleMouseDown( 0, 0, mask ))
		{
			handled = TRUE;
		}
	}

	return handled;
}

BOOL LLPieMenu::handleRightMouseUp( S32 x, S32 y, MASK mask )
{
	// release mouse capture when right mouse button released, and we're past the shrink time
	if (mShrinkBorderTimer.getStarted() && 
		mShrinkBorderTimer.getElapsedTimeF32() > PIE_SHRINK_TIME)
	{
		mUseInfiniteRadius = FALSE;
		gFocusMgr.setMouseCapture(NULL, NULL);
	}

	BOOL result = handleMouseUp( x, y, mask );
	mRightMouseDown = FALSE;

	return result;
}

BOOL LLPieMenu::handleMouseUp( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;

	// The click was somewhere within our rectangle
	LLMenuItemGL *item = pieItemFromXY( x, y );

	if (item)
	{
		// lie to the item about where the click happened
		// to make sure it's within the item's rectangle
		if (item->getEnabled())
		{
			handled = item->handleMouseUp( 0, 0, mask );
			hide(TRUE);
		}
	}
	else if (!mRightMouseDown)
	{
		// call hidemenus to make sure transient selections get cleared
		((LLMenuHolderGL*)getParent())->hideMenus();
	}

	if (handled)
	{
		make_ui_sound("UISndClickRelease");
	}

	if (!handled && !mUseInfiniteRadius)
	{
		// call hidemenus to make sure transient selections get cleared
		sMenuContainer->hideMenus();
	}

	if (mFirstMouseDown)
	{
		make_ui_sound("UISndPieMenuAppear");
		mFirstMouseDown = FALSE;
	}
	
	// *FIX: is this necessary?
	if (!mShrinkBorderTimer.getStarted())
	{
		mShrinkBorderTimer.start();
	}

	return handled;
}


// virtual
void LLPieMenu::draw()
{
	// clear hover if mouse moved away
	if (!mHoverThisFrame && mHoverItem)
	{
		mHoverItem->setHighlight(FALSE);
		mHoverItem = NULL;
	}

	F32 width = (F32) mRect.getWidth();
	F32 height = (F32) mRect.getHeight();
	mCurRadius = PIE_SCALE_FACTOR * llmax( width/2, height/2 );

	mOuterRingAlpha = mUseInfiniteRadius ? 0.f : 1.f;
	if (mShrinkBorderTimer.getStarted())
	{
		mOuterRingAlpha = clamp_rescale(mShrinkBorderTimer.getElapsedTimeF32(), 0.f, PIE_SHRINK_TIME, 0.f, 1.f);
		mCurRadius *= clamp_rescale(mShrinkBorderTimer.getElapsedTimeF32(), 0.f, PIE_SHRINK_TIME, 1.f, 1.f / PIE_SCALE_FACTOR);
	}

	// correct for non-square pixels
	F32 center_x = width/2;
	F32 center_y = height/2;
	S32 steps = 100;

	glPushMatrix();
	{
		glTranslatef(center_x, center_y, 0.f);

		F32 line_width = LLUI::sConfigGroup->getF32("PieMenuLineWidth");
		LLColor4 line_color = LLUI::sColorsGroup->getColor("PieMenuLineColor");
		LLColor4 bg_color = LLUI::sColorsGroup->getColor("PieMenuBgColor");
		LLColor4 selected_color = LLUI::sColorsGroup->getColor("PieMenuSelectedColor");

		// main body
		LLColor4 outer_color = bg_color;
		outer_color.mV[VALPHA] *= mOuterRingAlpha;
		gl_washer_2d( mCurRadius, (F32) PIE_CENTER_SIZE, steps, bg_color, outer_color );

		// selected wedge
		item_list_t::iterator item_iter;
		S32 i = 0;
		for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
		{
			if ((*item_iter)->getHighlight())
			{
				F32 arc_size = F_PI * 0.25f;

				F32 start_radians = (i * arc_size) - (arc_size * 0.5f);
				F32 end_radians = start_radians + arc_size;

				LLColor4 outer_color = selected_color;
				outer_color.mV[VALPHA] *= mOuterRingAlpha;
				gl_washer_segment_2d( mCurRadius, (F32)PIE_CENTER_SIZE, start_radians, end_radians, steps / 8, selected_color, outer_color );
			}
			i++;
		}

		LLUI::setLineWidth( line_width );

		// inner lines
		outer_color = line_color;
		outer_color.mV[VALPHA] *= mOuterRingAlpha;
		gl_washer_spokes_2d( mCurRadius, (F32)PIE_CENTER_SIZE, 8, line_color, outer_color );

		// inner circle
		glColor4fv( line_color.mV );
		gl_circle_2d( 0, 0, (F32)PIE_CENTER_SIZE, steps, FALSE );

		// outer circle
		glColor4fv( outer_color.mV );
		gl_circle_2d( 0, 0, mCurRadius, steps, FALSE );

		LLUI::setLineWidth(1.0f);
	}
	glPopMatrix();

	mHoverThisFrame = FALSE;

	LLView::draw();
}

void LLPieMenu::drawBackground(LLMenuItemGL* itemp, LLColor4& color)
{
	F32 width = (F32) mRect.getWidth();
	F32 height = (F32) mRect.getHeight();
	F32 center_x = width/2;
	F32 center_y = height/2;
	S32 steps = 100;

	glColor4fv( color.mV );
	glPushMatrix();
	{
		glTranslatef(center_x - itemp->getRect().mLeft, center_y - itemp->getRect().mBottom, 0.f);

		item_list_t::iterator item_iter;
		S32 i = 0;
		for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
		{
			if ((*item_iter) == itemp)
			{
				F32 arc_size = F_PI * 0.25f;

				F32 start_radians = (i * arc_size) - (arc_size * 0.5f);
				F32 end_radians = start_radians + arc_size;

				LLColor4 outer_color = color;
				outer_color.mV[VALPHA] *= mOuterRingAlpha;
				gl_washer_segment_2d( mCurRadius, (F32)PIE_CENTER_SIZE, start_radians, end_radians, steps / 8, color, outer_color );
			}
			i++;
		}
	}
	glPopMatrix();
}

// virtual
BOOL LLPieMenu::append(LLMenuItemGL *item)
{
	item->setBriefItem(TRUE);
	item->setFont( LLFontGL::sSansSerifSmall );
	return LLMenuGL::append(item);
}

// virtual
BOOL LLPieMenu::appendSeparator(const LLString &separator_name)
{
	LLMenuItemGL* separator = new LLMenuItemBlankGL();
	separator->setFont( LLFontGL::sSansSerifSmall );
	return append( separator );
}


// virtual
BOOL LLPieMenu::appendMenu(LLPieMenu *menu,
						   enabled_callback enabled_cb,
						   void* user_data)
{
	if (menu == this)
	{
		llerrs << "Can't attach a pie menu to itself" << llendl;
	}
	LLPieMenuBranch *item;
	item = new LLPieMenuBranch(menu->getName(), menu->getLabel(), menu, enabled_cb, user_data);
	getParent()->addChild(item->getBranch());
	item->setFont( LLFontGL::sSansSerifSmall );
	return append( item );
}

// virtual
void LLPieMenu::arrange()
{
	const S32 rect_height = 180;
	const S32 rect_width = 180;

	// all divide by 6
	const S32 CARD_X = 60;
	const S32 DIAG_X = 48;
	const S32 CARD_Y = 76;
	const S32 DIAG_Y = 42;

	const S32 ITEM_CENTER_X[] = { CARD_X, DIAG_X,      0, -DIAG_X, -CARD_X, -DIAG_X,       0,  DIAG_X };
	const S32 ITEM_CENTER_Y[] = {      0, DIAG_Y, CARD_Y,  DIAG_Y,       0, -DIAG_Y, -CARD_Y, -DIAG_Y };

	LLRect rect;
	
	S32 font_height = 0;
	if( mItems.size() )
	{
		font_height = (*mItems.begin())->getNominalHeight();
	}
	S32 item_width = 0;

//	F32 sin_delta = OO_SQRT2;	// sin(45 deg)
//	F32 cos_delta = OO_SQRT2;	// cos(45 deg)

	// TODO: Compute actual bounding rect for menu

	mRect.setOriginAndSize(mRect.mLeft, mRect.mBottom, rect_width, rect_height );

	// place items around a circle, with item 0 at positive X,
	// rotating counter-clockwise
	item_list_t::iterator item_iter;
	S32 i = 0;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLMenuItemGL *item = *item_iter;

		item_width = item->getNominalWidth();

		// Put in the right place around a circle centered at 0,0
		rect.setCenterAndSize(ITEM_CENTER_X[i],
							  ITEM_CENTER_Y[i], 
							  item_width, font_height );

		// Correct for the actual rectangle size
		rect.translate( rect_width/2, rect_height/2 );

		item->setRect( rect );

		// Make sure enablement is correct
		item->buildDrawLabel();
		i++;
	}
}

LLMenuItemGL *LLPieMenu::pieItemFromXY(S32 x, S32 y)
{
	// We might have shifted this menu on draw.  If so, we need
	// to shift over mouseup events until we get a hover event.
	//x += mShiftHoriz;
	//y += mShiftVert; 

	// An arc of the pie menu is 45 degrees
	const F32 ARC_DEG = 45.f;
	S32 delta_x = x - mRect.getWidth() / 2;
	S32 delta_y = y - mRect.getHeight() / 2;

	// circle safe zone in the center
	S32 dist_squared = delta_x*delta_x + delta_y*delta_y;
	if (dist_squared < PIE_CENTER_SIZE*PIE_CENTER_SIZE)
	{
		return NULL;
	}

	// infinite radius is only used with right clicks
	S32 radius = llmax( mRect.getWidth()/2, mRect.getHeight()/2 );
	if (!(mUseInfiniteRadius && mRightMouseDown) && dist_squared > radius * radius)
	{
		return NULL;
	}

	F32 angle = RAD_TO_DEG * (F32) atan2((F32)delta_y, (F32)delta_x);
	
	// rotate marks CCW so that east = [0, ARC_DEG) instead of
	// [-ARC_DEG/2, ARC_DEG/2)
	angle += ARC_DEG / 2.f;

	// make sure we're only using positive angles
	if (angle < 0.f) angle += 360.f;

	S32 which = S32( angle / ARC_DEG );

	if (0 <= which && which < (S32)mItems.size() )
	{
		item_list_t::iterator item_iter;
		for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
		{
			if (which == 0)
			{
				return (*item_iter);
			}
			which--;
		}
	}

	return NULL;
}

S32 LLPieMenu::pieItemIndexFromXY(S32 x, S32 y)
{
	// An arc of the pie menu is 45 degrees
	const F32 ARC_DEG = 45.f;
	// correct for non-square pixels
	S32 delta_x = x - mRect.getWidth() / 2;
	S32 delta_y = y - mRect.getHeight() / 2;

	// circle safe zone in the center
	if (delta_x*delta_x + delta_y*delta_y < PIE_CENTER_SIZE*PIE_CENTER_SIZE)
	{
		return -1;
	}

	F32 angle = RAD_TO_DEG * (F32) atan2((F32)delta_y, (F32)delta_x);
	
	// rotate marks CCW so that east = [0, ARC_DEG) instead of
	// [-ARC_DEG/2, ARC_DEG/2)
	angle += ARC_DEG / 2.f;

	// make sure we're only using positive angles
	if (angle < 0.f) angle += 360.f;

	S32 which = S32( angle / ARC_DEG );
	return which;
}

void LLPieMenu::show(S32 x, S32 y, BOOL mouse_down)
{
	S32 width = mRect.getWidth();
	S32 height = mRect.getHeight();

	const LLRect menu_region_rect = LLMenuGL::sMenuContainer->getMenuRect();

	LLView* parent_view = getParent();
	BOOL moved = FALSE;

	S32 local_x, local_y;
	parent_view->screenPointToLocal(x, y, &local_x, &local_y);

	mRect.setCenterAndSize(local_x, local_y, width, height);
	arrange();

	// Adjust the pie rectangle to keep it on screen
	if (mRect.mLeft < menu_region_rect.mLeft) 
	{
		//mShiftHoriz = menu_region_rect.mLeft - mRect.mLeft;
		//mRect.translate( mShiftHoriz, 0 );
		mRect.translate( menu_region_rect.mLeft - mRect.mLeft, 0 );
		moved = TRUE;
	}

	if (mRect.mRight > menu_region_rect.mRight) 
	{
		//mShiftHoriz = menu_region_rect.mRight - mRect.mRight;
		//mRect.translate( mShiftHoriz, 0);
		mRect.translate( menu_region_rect.mRight - mRect.mRight, 0 );
		moved = TRUE;
	}

	if (mRect.mBottom < menu_region_rect.mBottom)
	{
		//mShiftVert = menu_region_rect.mBottom - mRect.mBottom;
		//mRect.translate( 0, mShiftVert );
		mRect.translate( 0, menu_region_rect.mBottom - mRect.mBottom );
		moved = TRUE;
	}


	if (mRect.mTop > menu_region_rect.mTop)
	{
		//mShiftVert = menu_region_rect.mTop - mRect.mTop;
		//mRect.translate( 0, mShiftVert );
		mRect.translate( 0, menu_region_rect.mTop - mRect.mTop );
		moved = TRUE;
	}

	// If we had to relocate the pie menu, put the cursor in the
	// center of its rectangle
	if (moved)
	{
		LLCoordGL center;
		center.mX = (mRect.mLeft + mRect.mRight) / 2;
		center.mY = (mRect.mTop + mRect.mBottom) / 2;

		LLUI::setCursorPositionLocal(getParent(), center.mX, center.mY);
	}

	// *FIX: what happens when mouse buttons reversed?
	mRightMouseDown = mouse_down;
	mFirstMouseDown = mouse_down;
	mUseInfiniteRadius = TRUE;
	if (!mFirstMouseDown)
	{
		make_ui_sound("UISndPieMenuAppear");
	}

	LLView::setVisible(TRUE);

	// we want all mouse events in case user does quick right click again off of pie menu
	// rectangle, to support gestural menu traversal
	gFocusMgr.setMouseCapture(this, NULL);

	if (mouse_down)
	{
		mShrinkBorderTimer.stop();
	}
	else 
	{
		mShrinkBorderTimer.start();
	}
}

void LLPieMenu::hide(BOOL item_selected)
{
	if (!getVisible()) return;

	if (mHoverItem)
	{
		mHoverItem->setHighlight( FALSE );
		mHoverItem = NULL;
	}

	make_ui_sound("UISndPieMenuHide");

	mFirstMouseDown = FALSE;
	mRightMouseDown = FALSE;
	mUseInfiniteRadius = FALSE;

	LLView::setVisible(FALSE);

	gFocusMgr.setMouseCapture(NULL, NULL);
}

///============================================================================
/// Class LLMenuBarGL
///============================================================================

// Default constructor
LLMenuBarGL::LLMenuBarGL( const LLString& name ) : LLMenuGL ( name, name )
{
	mHorizontalLayout = TRUE;
	setCanTearOff(FALSE);
	mKeepFixedSize = TRUE;
	mAltKeyTrigger = FALSE;
}

// Default destructor
LLMenuBarGL::~LLMenuBarGL()
{
	std::for_each(mAccelerators.begin(), mAccelerators.end(), DeletePointer());
	mAccelerators.clear();
}

// virtual
LLXMLNodePtr LLMenuBarGL::getXML(bool save_children) const
{
	// Sorty of hacky: reparent items to this and then back at the end of the export
	LLView *orig_parent = NULL;
	item_list_t::const_iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLMenuItemGL* child = *item_iter;
		LLMenuItemBranchGL* branch = (LLMenuItemBranchGL*)child;
		LLMenuGL *menu = branch->getBranch();
		orig_parent = menu->getParent();
		menu->updateParent((LLView *)this);
	}

	LLXMLNodePtr node = LLMenuGL::getXML();

	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLMenuItemGL* child = *item_iter;
		LLMenuItemBranchGL* branch = (LLMenuItemBranchGL*)child;
		LLMenuGL *menu = branch->getBranch();
		menu->updateParent(orig_parent);
	}

	return node;
}

LLView* LLMenuBarGL::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("menu");
	node->getAttributeString("name", name);

	BOOL opaque = FALSE;
	node->getAttributeBOOL("opaque", opaque);

	LLMenuBarGL *menubar = new LLMenuBarGL(name);

	LLViewHandle parent_handle = LLViewHandle::sDeadHandle;
	if (parent->getWidgetType() == WIDGET_TYPE_FLOATER)
	{
		parent_handle = ((LLFloater*)parent)->getHandle();
	}

	// We need to have the rect early so that it's around when building
	// the menu items
	LLRect view_rect;
	createRect(node, view_rect, parent, menubar->getRequiredRect());
	menubar->setRect(view_rect);

	if (node->hasAttribute("drop_shadow"))
	{
		BOOL drop_shadow = FALSE;
		node->getAttributeBOOL("drop_shadow", drop_shadow);
		menubar->setDropShadowed(drop_shadow);
	}

	menubar->setBackgroundVisible(opaque);
	LLColor4 color(0,0,0,0);
	if (opaque && LLUICtrlFactory::getAttributeColor(node,"color", color))
	{
		menubar->setBackgroundColor(color);
	}

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("menu"))
		{
			LLMenuGL *menu = (LLMenuGL*)LLMenuGL::fromXML(child, parent, factory);
			// because of lazy initialization, have to disable tear off functionality
			// and then re-enable with proper parent handle
			if (menu->getCanTearOff())
			{
				menu->setCanTearOff(FALSE);
				menu->setCanTearOff(TRUE, parent_handle);
			}
			menubar->appendMenu(menu);
			if (LLMenuGL::sMenuContainer != NULL)
			{
				menu->updateParent(LLMenuGL::sMenuContainer);
			}
			else
			{
				menu->updateParent(parent);
			}
		}
	}

	menubar->initFromXML(node, parent);

	BOOL create_jump_keys = FALSE;
	node->getAttributeBOOL("create_jump_keys", create_jump_keys);
	if (create_jump_keys)
	{
		menubar->createJumpKeys();
	}

	return menubar;
}

BOOL LLMenuBarGL::handleAcceleratorKey(KEY key, MASK mask)
{
	if (getHighlightedItem() && mask == MASK_NONE)
	{
		// unmodified key accelerators are ignored when navigating menu
		// (but are used as jump keys so will still work when appropriate menu is up)
		return FALSE;
	}
	BOOL result = LLMenuGL::handleAcceleratorKey(key, mask);
	if (result && mask & MASK_ALT)
	{
		// ALT key used to trigger hotkey, don't use as shortcut to open menu
		mAltKeyTrigger = FALSE;
	}

	if(!result && (key == KEY_F10 && mask == MASK_CONTROL) && !gKeyboard->getKeyRepeated(key))
	{
		if (getHighlightedItem())
		{
			clearHoverItem();
		}
		else
		{
			highlightNextItem(NULL);
			LLMenuGL::setKeyboardMode(TRUE);
		}
		return TRUE;
	}

	return result;
}

BOOL LLMenuBarGL::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if(key == KEY_ALT && !gKeyboard->getKeyRepeated(key) && LLUI::sConfigGroup->getBOOL("UseAltKeyForMenus"))
	{
		mAltKeyTrigger = TRUE;
	}
	else // if any key other than ALT hit, clear out waiting for Alt key mode
	{
		mAltKeyTrigger = FALSE;
	}

	// before processing any other key, check to see if ALT key has triggered menu access
	checkMenuTrigger();

	return LLMenuGL::handleKeyHere(key, mask, called_from_parent);
}

BOOL LLMenuBarGL::handleJumpKey(KEY key)
{
	// perform case-insensitive comparison
	key = toupper(key);
	navigation_key_map_t::iterator found_it = mJumpKeys.find(key);
	if(found_it != mJumpKeys.end() && found_it->second->getEnabled())
	{
		// switch to keyboard navigation mode
		LLMenuGL::setKeyboardMode(TRUE);

		found_it->second->setHighlight(TRUE);
		found_it->second->doIt();
	}
	return TRUE;
}

void LLMenuBarGL::draw()
{
	LLMenuItemGL* itemp = getHighlightedItem();
	// If we are in mouse-control mode and the mouse cursor is not hovering over
	// the current highlighted menu item and it isn't open, then remove the highlight.
	// This is done via a polling mechanism here, as we don't receive notifications when
	// the mouse cursor moves off of us
	if (itemp && !itemp->isOpen() && !itemp->getHover() && !LLMenuGL::getKeyboardMode())
	{
		clearHoverItem();
	}

	checkMenuTrigger();

	LLMenuGL::draw();
}

void LLMenuBarGL::checkMenuTrigger()
{
	// has the ALT key been pressed and subsequently released?
	if (mAltKeyTrigger && !gKeyboard->getKeyDown(KEY_ALT))
	{
		// if alt key was released quickly, treat it as a menu access key
		// otherwise it was probably an Alt-zoom or similar action
		if (gKeyboard->getKeyElapsedTime(KEY_ALT) <= LLUI::sConfigGroup->getF32("MenuAccessKeyTime") ||
			gKeyboard->getKeyElapsedFrameCount(KEY_ALT) < 2)
		{
			if (getHighlightedItem())
			{
				clearHoverItem();
			}
			else
			{
				highlightNextItem(NULL);
				LLMenuGL::setKeyboardMode(TRUE);
			}
		}
		mAltKeyTrigger = FALSE;
	}
}

BOOL LLMenuBarGL::jumpKeysActive()
{
	// require user to be in keyboard navigation mode to activate key triggers
	// as menu bars are always visible and it is easy to leave the mouse cursor over them
	return LLMenuGL::getKeyboardMode() && getHighlightedItem() && LLMenuGL::jumpKeysActive();
}

// rearrange the child rects so they fit the shape of the menu bar.
void LLMenuBarGL::arrange( void )
{
	U32 pos = 0;
	LLRect rect( 0, mRect.getHeight(), 0, 0 );
	item_list_t::const_iterator item_iter;
	for (item_iter = mItems.begin(); item_iter != mItems.end(); ++item_iter)
	{
		LLMenuItemGL* item = *item_iter;
		rect.mLeft = pos;
		pos += item->getNominalWidth();
		rect.mRight = pos;
		item->setRect( rect );
		item->buildDrawLabel();
	}
	reshape(rect.mRight, rect.getHeight());
}


S32 LLMenuBarGL::getRightmostMenuEdge()
{
	// Find the last visible menu
	item_list_t::reverse_iterator item_iter;
	for (item_iter = mItems.rbegin(); item_iter != mItems.rend(); ++item_iter)
	{
		if ((*item_iter)->getVisible())
		{
			break;
		}
	}

	if (item_iter == mItems.rend())
	{
		return 0;
	}
	return (*item_iter)->getRect().mRight;
}

// add a vertical separator to this menu
BOOL LLMenuBarGL::appendSeparator( const LLString &separator_name )
{
	LLMenuItemGL* separator = new LLMenuItemVerticalSeparatorGL();
	return append( separator );
}

// add a menu - this will create a drop down menu.
BOOL LLMenuBarGL::appendMenu( LLMenuGL* menu )
{
	if( menu == this )
	{
		llerrs << "** Attempt to attach menu to itself. This is certainly "
			   << "a logic error." << llendl;
	}

	BOOL success = TRUE;

	LLMenuItemBranchGL* branch = NULL;
	branch = new LLMenuItemBranchDownGL( menu->getName(), menu->getLabel(), menu );
	success &= branch->addToAcceleratorList(&mAccelerators);
	success &= append( branch );
	branch->setJumpKey(branch->getJumpKey());
	return success;
}

BOOL LLMenuBarGL::handleHover( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	LLView* active_menu = NULL;

	BOOL no_mouse_data = mLastMouseX == 0 && mLastMouseY == 0;
	S32 mouse_delta_x = no_mouse_data ? 0 : x - mLastMouseX;
	S32 mouse_delta_y = no_mouse_data ? 0 : y - mLastMouseY;
	mMouseVelX = (mMouseVelX / 2) + (mouse_delta_x / 2);
	mMouseVelY = (mMouseVelY / 2) + (mouse_delta_y / 2);
	mLastMouseX = x;
	mLastMouseY = y;

	// if nothing currently selected or mouse has moved since last call, pick menu item via mouse
	// otherwise let keyboard control it
	if (!getHighlightedItem() || !LLMenuGL::getKeyboardMode() || llabs(mMouseVelX) > 0 || llabs(mMouseVelY) > 0)
	{
		// find current active menu
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (((LLMenuItemGL*)viewp)->isOpen())
			{
				active_menu = viewp;
			}
		}

		// check for new active menu
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->getRect().mLeft;
			S32 local_y = y - viewp->getRect().mBottom;
			if( viewp->getVisible() && 
				viewp->getEnabled() &&
				viewp->pointInView(local_x, local_y) && 
				viewp->handleHover(local_x, local_y, mask))
			{
				((LLMenuItemGL*)viewp)->setHighlight(TRUE);
				handled = TRUE;
				if (active_menu && active_menu != viewp)
				{
					((LLMenuItemGL*)viewp)->doIt();
					LLMenuGL::setKeyboardMode(FALSE);
				}
				LLMenuGL::setKeyboardMode(FALSE);
			}
		}

		if (handled)
		{
			// set hover false on inactive menus
			for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
			{
				LLView* viewp = *child_it;
				S32 local_x = x - viewp->getRect().mLeft;
				S32 local_y = y - viewp->getRect().mBottom;
				if (!viewp->pointInView(local_x, local_y) && ((LLMenuItemGL*)viewp)->getHighlight())
				{
					((LLMenuItemGL*)viewp)->setHighlight(FALSE);
				}
			}
		}
	}

	getWindow()->setCursor(UI_CURSOR_ARROW);
	
	return TRUE;
}

///============================================================================
/// Class LLMenuHolderGL
///============================================================================
LLMenuHolderGL::LLMenuHolderGL()
:	LLPanel("Menu Holder")
{
	setMouseOpaque(FALSE);
	sItemActivationTimer.stop();
	mCanHide = TRUE;
}

LLMenuHolderGL::LLMenuHolderGL(const LLString& name, const LLRect& rect, BOOL mouse_opaque, U32 follows) 
:	LLPanel(name, rect, FALSE)
{
	setMouseOpaque(mouse_opaque);
	sItemActivationTimer.stop();
	mCanHide = TRUE;
}

LLMenuHolderGL::~LLMenuHolderGL()
{
}

EWidgetType LLMenuHolderGL::getWidgetType() const
{
	return WIDGET_TYPE_MENU_HOLDER;
}

LLString LLMenuHolderGL::getWidgetTag() const
{
	return LL_MENU_HOLDER_GL_TAG;
}

void LLMenuHolderGL::draw()
{
	LLView::draw();
	// now draw last selected item as overlay
	LLMenuItemGL* selecteditem = (LLMenuItemGL*)LLView::getViewByHandle(sItemLastSelectedHandle);
	if (selecteditem && sItemActivationTimer.getStarted() && sItemActivationTimer.getElapsedTimeF32() < ACTIVATE_HIGHLIGHT_TIME)
	{
		// make sure toggle items, for example, show the proper state when fading out
		selecteditem->buildDrawLabel();

		LLRect item_rect;
		selecteditem->localRectToOtherView(selecteditem->getLocalRect(), &item_rect, this);

		F32 interpolant = sItemActivationTimer.getElapsedTimeF32() / ACTIVATE_HIGHLIGHT_TIME;
		F32 alpha = lerp(LLMenuItemGL::sHighlightBackground.mV[VALPHA], 0.f, interpolant);
		LLColor4 bg_color(LLMenuItemGL::sHighlightBackground.mV[VRED], 
			LLMenuItemGL::sHighlightBackground.mV[VGREEN], 
			LLMenuItemGL::sHighlightBackground.mV[VBLUE], 
			alpha);
		
		LLUI::pushMatrix();
		{
			LLUI::translate((F32)item_rect.mLeft, (F32)item_rect.mBottom, 0.f);
			selecteditem->getMenu()->drawBackground(selecteditem, bg_color);
			selecteditem->draw();
		}
		LLUI::popMatrix();
	}
}

BOOL LLMenuHolderGL::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;
	if (!handled)
	{
		// clicked off of menu, hide them all
		hideMenus();
	}
	return handled;
}

BOOL LLMenuHolderGL::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = LLView::childrenHandleRightMouseDown(x, y, mask) != NULL;
	if (!handled)
	{
		// clicked off of menu, hide them all
		hideMenus();
	}
	return handled;
}

void LLMenuHolderGL::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (width != mRect.getWidth() || height != mRect.getHeight())
	{
		hideMenus();
	}
	LLView::reshape(width, height, called_from_parent);
}

BOOL LLMenuHolderGL::hasVisibleMenu() const
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if (viewp->getVisible() && viewp->getWidgetType() != WIDGET_TYPE_MENU_BAR)
		{
			return TRUE;
		}
	}
	return FALSE;
}

const LLRect LLMenuHolderGL::getMenuRect() const
{
	return getLocalRect();
}

BOOL LLMenuHolderGL::hideMenus()
{
	if (!mCanHide)
	{
		return FALSE;
	}
	BOOL menu_visible = hasVisibleMenu();
	if (menu_visible)
	{
		LLMenuGL::setKeyboardMode(FALSE);
		// clicked off of menu, hide them all
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			// clicks off of menu do not hide menu bar
			if (viewp->getWidgetType() != WIDGET_TYPE_MENU_BAR && viewp->getVisible())
			{
				viewp->setVisible(FALSE);
			}
		}
	}
	//if (gFocusMgr.childHasKeyboardFocus(this))
	//{
	//	gFocusMgr.setKeyboardFocus(NULL, NULL);
	//}

	return menu_visible;
}

void LLMenuHolderGL::setActivatedItem(LLMenuItemGL* item)
{
	sItemLastSelectedHandle = item->mViewHandle;
	sItemActivationTimer.start();
}

///============================================================================
/// Class LLTearOffMenu
///============================================================================
LLTearOffMenu::LLTearOffMenu(LLMenuGL* menup) : 
	LLFloater(menup->getName(), LLRect(0, 100, 100, 0), menup->getLabel(), FALSE, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT, FALSE, FALSE)
{
	LLRect rect;
	menup->localRectToOtherView(LLRect(-1, menup->getRect().getHeight(), menup->getRect().getWidth() + 3, 0), &rect, gFloaterView);
	mTargetHeight = (F32)(rect.getHeight() + LLFLOATER_HEADER_SIZE + 5);
	reshape(rect.getWidth(), rect.getHeight());
	setRect(rect);
	mOldParent = menup->getParent();
	mOldParent->removeChild(menup);

	menup->setFollowsAll();
	addChild(menup);
	menup->setVisible(TRUE);
	menup->translate(-menup->getRect().mLeft + 1, -menup->getRect().mBottom + 1);

	menup->setTornOff(TRUE);
	menup->setDropShadowed(FALSE);

	mMenu = menup;

	// highlight first item (tear off item will be disabled)
	mMenu->highlightNextItem(NULL);
}

LLTearOffMenu::~LLTearOffMenu()
{
}

void LLTearOffMenu::draw()
{
	mMenu->setBackgroundVisible(mBgOpaque);
	mMenu->arrange();

	if (mRect.getHeight() != mTargetHeight)
	{
		// animate towards target height
		reshape(mRect.getWidth(), llceil(lerp((F32)mRect.getHeight(), mTargetHeight, LLCriticalDamp::getInterpolant(0.05f))));
	}
	else
	{
		// when in stasis, remain big enough to hold menu contents
		mTargetHeight = (F32)(mMenu->getRect().getHeight() + LLFLOATER_HEADER_SIZE + 4);
		reshape(mMenu->getRect().getWidth() + 3, mMenu->getRect().getHeight() + LLFLOATER_HEADER_SIZE + 5);
	}
	LLFloater::draw();
}

void LLTearOffMenu::onFocusReceived()
{
	// if nothing is highlighted, just highlight first item
	if (!mMenu->getHighlightedItem())
	{
		mMenu->highlightNextItem(NULL);
	}

	// parent menu items get highlights so navigation logic keeps working
	LLMenuItemGL* parent_menu_item = mMenu->getParentMenuItem();
	while(parent_menu_item)
	{
		if (parent_menu_item->getMenu()->getVisible())
		{
			parent_menu_item->setHighlight(TRUE);
			parent_menu_item = parent_menu_item->getMenu()->getParentMenuItem();
		}
		else
		{
			break;
		}
	}
}

void LLTearOffMenu::onFocusLost()
{
	// remove highlight from parent item and our own menu
	mMenu->clearHoverItem();
}

BOOL LLTearOffMenu::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	// pass keystrokes down to menu
	return mMenu->handleUnicodeChar(uni_char, TRUE);
}

BOOL LLTearOffMenu::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if (!mMenu->getHighlightedItem())
	{
		if (key == KEY_UP)
		{
			mMenu->highlightPrevItem(NULL);		
			return TRUE;
		}
		else if (key == KEY_DOWN)
		{
			mMenu->highlightNextItem(NULL);
			return TRUE;
		}
	}
	// pass keystrokes down to menu
	return mMenu->handleKey(key, mask, TRUE);
}

void LLTearOffMenu::translate(S32 x, S32 y)
{
	if (x != 0 && y != 0)
	{
		// hide open sub-menus by clearing current hover item
		mMenu->clearHoverItem();
	}
	LLFloater::translate(x, y);
}

//static
LLTearOffMenu* LLTearOffMenu::create(LLMenuGL* menup)
{
	LLTearOffMenu* tearoffp = new LLTearOffMenu(menup);
	// keep onscreen
	gFloaterView->adjustToFitScreen(tearoffp, FALSE);
	tearoffp->open();	/* Flawfinder: ignore */
	return tearoffp;
}

void LLTearOffMenu::onClose(bool app_quitting)
{
	removeChild(mMenu);
	mOldParent->addChild(mMenu);
	mMenu->clearHoverItem();
	mMenu->setFollowsNone();
	mMenu->setBackgroundVisible(TRUE);
	mMenu->setVisible(FALSE);
	mMenu->setTornOff(FALSE);
	mMenu->setDropShadowed(TRUE);
	destroy();
}

///============================================================================
/// Class LLEditMenuHandlerMgr
///============================================================================
LLEditMenuHandlerMgr& LLEditMenuHandlerMgr::getInstance()
{
	static LLEditMenuHandlerMgr instance;
	return instance;
}

LLEditMenuHandlerMgr::LLEditMenuHandlerMgr()
{
}

LLEditMenuHandlerMgr::~LLEditMenuHandlerMgr()
{
}

///============================================================================
/// Local function definitions
///============================================================================
