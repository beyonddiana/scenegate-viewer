/** 
 * @file llcolorswatch.cpp
 * @brief LLColorSwatch class implementation
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

#include "llviewerprecompiledheaders.h"

// File include
#include "llcolorswatch.h"

// Linden library includes
#include "v4color.h"
#include "llwindow.h"	// setCursor()

// Project includes
#include "llui.h"
#include "llrender.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llfloatercolorpicker.h"
#include "llviewborder.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"

static LLDefaultChildRegistry::Register<LLColorSwatchCtrl> r("color_swatch");

LLColorSwatchCtrl::Params::Params()
:	color("color", LLColor4::white),
	can_apply_immediately("can_apply_immediately", false),
	alpha_background_image("alpha_background_image"),
	border_color("border_color"),
    label_width("label_width", -1),
	caption_text("caption_text"),
	border("border")
{
	name = "colorswatch";
}

LLColorSwatchCtrl::LLColorSwatchCtrl(const Params& p)
:	LLUICtrl(p),
	mValid( TRUE ),
	mColor(p.color),
	mCanApplyImmediately(p.can_apply_immediately),
	mAlphaGradientImage(p.alpha_background_image),
	mOnCancelCallback(p.cancel_callback()),
	mOnSelectCallback(p.select_callback()),
	mBorderColor(p.border_color()),
	mLabelWidth(p.label_width)
{	
	LLTextBox::Params tp = p.caption_text;
	// label_width is specified, not -1
	if(mLabelWidth!= -1)
	{
		tp.rect(LLRect( 0, BTN_HEIGHT_SMALL, mLabelWidth, 0 ));
	}
	else
	{
		tp.rect(LLRect( 0, BTN_HEIGHT_SMALL, getRect().getWidth(), 0 ));
	}
	
	tp.text(p.label);
	mCaption = LLUICtrlFactory::create<LLTextBox>(tp);
	addChild( mCaption );

	LLRect border_rect = getLocalRect();
	border_rect.mTop -= 1;
	border_rect.mRight -=1;
	border_rect.mBottom += BTN_HEIGHT_SMALL;

	LLViewBorder::Params params = p.border;
	params.rect(border_rect);
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	addChild(mBorder);
}

LLColorSwatchCtrl::~LLColorSwatchCtrl ()
{
	// parent dialog is destroyed so we are too and we need to cancel selection
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->cancelSelection();
		pickerp->closeFloater();
	}
}

BOOL LLColorSwatchCtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return handleMouseDown(x, y, mask);
}

BOOL LLColorSwatchCtrl::handleHover(S32 x, S32 y, MASK mask)
{
	getWindow()->setCursor(UI_CURSOR_HAND);
	return TRUE;
}

BOOL LLColorSwatchCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	if( ' ' == uni_char )
	{
		showPicker(TRUE);
	}
	return LLUICtrl::handleUnicodeCharHere(uni_char);
}

// forces color of this swatch and any associated floater to the input value, if currently invalid
void LLColorSwatchCtrl::setOriginal(const LLColor4& color)
{
	mColor = color;
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->setOrigRgb(mColor.mV[VRED], mColor.mV[VGREEN], mColor.mV[VBLUE]);
	}
}

void LLColorSwatchCtrl::set(const LLColor4& color, BOOL update_picker, BOOL from_event)
{
	mColor = color; 
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp && update_picker)
	{
		pickerp->setCurRgb(mColor.mV[VRED], mColor.mV[VGREEN], mColor.mV[VBLUE]);
	}
	if (!from_event)
	{
		setControlValue(mColor.getValue());
	}
}

void LLColorSwatchCtrl::setLabel(const std::string& label)
{
	mCaption->setText(label);
}

BOOL LLColorSwatchCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	// No handler is needed for capture lost since this object has no state that depends on it.
	gFocusMgr.setMouseCapture( this );

	return TRUE;
}


BOOL LLColorSwatchCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		// If mouseup in the widget, it's been clicked
		if ( pointInView(x, y) )
		{
			llassert(getEnabled());
			llassert(getVisible());

			showPicker(FALSE);
		}
	}

	return TRUE;
}

// assumes GL state is set for 2D
void LLColorSwatchCtrl::draw()
{
	F32 alpha = getDrawContext().mAlpha;
	mBorder->setKeyboardFocusHighlight(hasFocus());
	// Draw border
	LLRect border( 0, getRect().getHeight(), getRect().getWidth(), BTN_HEIGHT_SMALL );
	gl_rect_2d( border, mBorderColor.get(), FALSE );

	LLRect interior = border;
	interior.stretch( -1 );

	// Check state
	if ( mValid )
	{
		// Draw the color swatch
		gl_rect_2d_checkerboard( interior );
		gl_rect_2d(interior, mColor, TRUE);
		LLColor4 opaque_color = mColor;
		opaque_color.mV[VALPHA] = 1.f;
		gGL.color4fv(opaque_color.mV);
		if (mAlphaGradientImage.notNull())
		{
			gGL.pushMatrix();
			{
				mAlphaGradientImage->draw(interior, mColor);
			}
			gGL.popMatrix();
		}
	}
	else
	{
		if (!mFallbackImageName.empty())
		{
			LLPointer<LLViewerTexture> fallback_image = LLViewerTextureManager::getFetchedTextureFromFile(mFallbackImageName, TRUE, FALSE, LLViewerTexture::LOD_TEXTURE);
			if( fallback_image->getComponents() == 4 )
			{	
				gl_rect_2d_checkerboard( interior );
			}	
			gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), fallback_image, LLColor4::white % alpha);
			fallback_image->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
		}
		else
		{
			// Draw grey and an X
			gl_rect_2d(interior, LLColor4::grey % alpha, TRUE);
			
			gl_draw_x(interior, LLColor4::black % alpha);
		}
	}

	LLUICtrl::draw();
}

void LLColorSwatchCtrl::setEnabled( BOOL enabled )
{
	mCaption->setEnabled( enabled );
	LLView::setEnabled( enabled );

	if (!enabled)
	{
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
		if (pickerp)
		{
			pickerp->cancelSelection();
			pickerp->closeFloater();
		}
	}
}


void LLColorSwatchCtrl::setValue(const LLSD& value)
{
	set(LLColor4(value), TRUE, TRUE);
}

//////////////////////////////////////////////////////////////////////////////
// called (infrequently) when the color changes so the subject of the swatch can be updated.
void LLColorSwatchCtrl::onColorChanged ( void* data, EColorPickOp pick_op )
{
	LLColorSwatchCtrl* subject = ( LLColorSwatchCtrl* )data;
	if ( subject )
	{
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)subject->mPickerHandle.get();
		if (pickerp)
		{
			// move color across from selector to internal widget storage
			LLColor4 updatedColor ( pickerp->getCurR (), 
									pickerp->getCurG (), 
									pickerp->getCurB (), 
									subject->mColor.mV[VALPHA] ); // keep current alpha
			subject->mColor = updatedColor;
			subject->setControlValue(updatedColor.getValue());

			if (pick_op == COLOR_CANCEL && subject->mOnCancelCallback)
			{
				subject->mOnCancelCallback( subject, LLSD());
			}
			else if (pick_op == COLOR_SELECT && subject->mOnSelectCallback)
			{
				subject->mOnSelectCallback( subject, LLSD() );
			}
			else
			{
				// just commit change
				subject->onCommit ();
			}
		}
	}
}

void LLColorSwatchCtrl::setValid(BOOL valid )
{
	mValid = valid;

	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->setActive(valid);
	}
}

void LLColorSwatchCtrl::showPicker(BOOL take_focus)
{
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (!pickerp)
	{
		pickerp = new LLFloaterColorPicker(this, mCanApplyImmediately);
		gFloaterView->getParentFloater(this)->addDependentFloater(pickerp);
		mPickerHandle = pickerp->getHandle();
	}

	// initialize picker with current color
	pickerp->initUI ( mColor.mV [ VRED ], mColor.mV [ VGREEN ], mColor.mV [ VBLUE ] );

	// display it
	pickerp->showUI ();

	if (take_focus)
	{
		pickerp->setFocus(TRUE);
	}
}

