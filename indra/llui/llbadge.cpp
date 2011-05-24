/** 
 * @file llbadge.cpp
 * @brief Implementation for badges
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#define LLBADGE_CPP
#include "llbadge.h"

#include "lluictrlfactory.h"


static LLDefaultChildRegistry::Register<LLBadge> r("badge");

// Compiler optimization, generate extern template
template class LLBadge* LLView::getChild<class LLBadge>(const std::string& name, BOOL recurse) const;


LLBadge::Params::Params()
	: image("image")
	, image_color("image_color")
	, label("label")
	, label_color("label_color")
	, location("location", LLRelPos::TOP_LEFT)
	, location_percent_hcenter("location_percent_hcenter")
	, location_percent_vcenter("location_percent_vcenter")
	, padding_horiz("padding_horiz")
	, padding_vert("padding_vert")
{
	// We set a name here so the name isn't necessary in any xml files that use badges
	name = "badge";
}

bool LLBadge::Params::equals(const Params& a) const
{
	bool comp = true;
	
	// skip owner in comparison on purpose
	
	comp &= (image() == a.image());
	comp &= (image_color() == a.image_color());
	comp &= (label() == a.label());
	comp &= (label_color() == a.label_color());
	comp &= (location() == a.location());
	comp &= (location_percent_hcenter() == a.location_percent_hcenter());
	comp &= (location_percent_vcenter() == a.location_percent_vcenter());
	comp &= (padding_horiz() == a.padding_horiz());
	comp &= (padding_vert() == a.padding_vert());
	
	return comp;
}

LLBadge::LLBadge(const LLBadge::Params& p)
	: LLUICtrl(p)
	, mOwner(p.owner)
	, mGLFont(p.font)
	, mImage(p.image)
	, mImageColor(p.image_color)
	, mLabel(p.label)
	, mLabelColor(p.label_color)
	, mLocation(p.location)
	, mLocationPercentHCenter(0.5f)
	, mLocationPercentVCenter(0.5f)
	, mPaddingHoriz(p.padding_horiz)
	, mPaddingVert(p.padding_vert)
{
	if (mImage.isNull())
	{
		llwarns << "Badge: " << getName() << " with no image!" << llendl;
	}

	//
	// The following logic is to set the mLocationPercentHCenter and mLocationPercentVCenter
	// based on the Location enum and our horizontal and vertical location percentages.  The
	// draw code then uses this on the owner rectangle to compute the screen location for
	// the badge.
	//

	if (!LLRelPos::IsCenter(mLocation))
	{
		F32 h_center = p.location_percent_hcenter * 0.01f;
		F32 v_center = p.location_percent_vcenter * 0.01f;

		if (LLRelPos::IsRight(mLocation))
		{
			mLocationPercentHCenter = 0.5f * (1.0f + h_center);
		}
		else if (LLRelPos::IsLeft(mLocation))
		{
			mLocationPercentHCenter = 0.5f * (1.0f - h_center);
		}
			
		if (LLRelPos::IsTop(mLocation))
		{
			mLocationPercentVCenter = 0.5f * (1.0f + v_center);
		}
		else if (LLRelPos::IsBottom(mLocation))
		{
			mLocationPercentVCenter = 0.5f * (1.0f - v_center);
		}
	}
}

LLBadge::~LLBadge()
{
}

void LLBadge::setLabel(const LLStringExplicit& label)
{
	mLabel = label;
}

//
// This is a fallback function to render a rectangle for badges without a valid image
//
void renderBadgeBackground(F32 centerX, F32 centerY, F32 width, F32 height, const LLColor4U &color)
{
	gGL.pushUIMatrix();
	gGL.loadUIIdentity();
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	gGL.color4ubv(color.mV);
	gGL.texCoord2i(0, 0);
	
	F32 x = LLFontGL::sCurOrigin.mX + centerX - width * 0.5f;
	F32 y = LLFontGL::sCurOrigin.mY + centerY - height * 0.5f;
	
	LLRectf screen_rect(llround(x),
						llround(y),
						llround(x) + width,
						llround(y) + height);
	
	LLVector3 vertices[4];
	vertices[0] = LLVector3(screen_rect.mRight, screen_rect.mTop,    1.0f);
	vertices[1] = LLVector3(screen_rect.mLeft,  screen_rect.mTop,    1.0f);
	vertices[2] = LLVector3(screen_rect.mLeft,  screen_rect.mBottom, 1.0f);
	vertices[3] = LLVector3(screen_rect.mRight, screen_rect.mBottom, 1.0f);
	
	gGL.begin(LLRender::QUADS);
	{
		gGL.vertexBatchPreTransformed(vertices, 4);
	}
	gGL.end();
	
	gGL.popUIMatrix();
}


// virtual
void LLBadge::draw()
{
	if (!mLabel.empty())
	{
		LLUICtrl* owner_ctrl = mOwner.get();

		if (owner_ctrl)
		{
			//
			// Calculate badge position based on owner
			//
			
			LLRect owner_rect;
			owner_ctrl->localRectToOtherView(owner_ctrl->getLocalRect(), & owner_rect, this);
			
			F32 badge_center_x = owner_rect.mLeft + owner_rect.getWidth() * mLocationPercentHCenter;
			F32 badge_center_y = owner_rect.mBottom + owner_rect.getHeight() * mLocationPercentVCenter;

			//
			// Calculate badge size based on label text
			//

			LLWString badge_label_wstring = mLabel;
			
			S32 badge_label_begin_offset = 0;
			S32 badge_char_length = S32_MAX;
			S32 badge_pixel_length = S32_MAX;
			F32 *right_position_out = NULL;
			BOOL do_not_use_ellipses = false;

			F32 badge_width = (2.0f * mPaddingHoriz) +
				mGLFont->getWidthF32(badge_label_wstring.c_str(), badge_label_begin_offset, badge_char_length);

			F32 badge_height = (2.0f * mPaddingVert) + mGLFont->getLineHeight();

			//
			// Draw button image, if available.
			// Otherwise draw basic rectangular button.
			//

			F32 alpha = getDrawContext().mAlpha;

			if (!mImage.isNull())
			{
				F32 badge_x = badge_center_x - badge_width * 0.5f;
				F32 badge_y = badge_center_y - badge_height * 0.5f;
			
				mImage->drawSolid(badge_x, badge_y, badge_width, badge_height, mImageColor % alpha);
			}
			else
			{
				lldebugs << "No image for badge " << getName() << " on owner " << owner_ctrl->getName() << llendl;
				
				renderBadgeBackground(badge_center_x, badge_center_y,
									  badge_width, badge_height,
									  mImageColor % alpha);
			}

			//
			// Draw the label
			//

			mGLFont->render(badge_label_wstring, badge_label_begin_offset,
							badge_center_x, badge_center_y,
							mLabelColor % alpha,
							LLFontGL::HCENTER, LLFontGL::VCENTER, // centered around the position
							LLFontGL::NORMAL, // normal text (not bold, italics, etc.)
							LLFontGL::DROP_SHADOW_SOFT,
							badge_char_length, badge_pixel_length,
							right_position_out, do_not_use_ellipses);
		}
	}
}


namespace LLInitParam
{
	void TypeValues<LLRelPos::Location>::declareValues()
	{
		declare("bottom",		LLRelPos::BOTTOM);
		declare("bottom_left",	LLRelPos::BOTTOM_LEFT);
		declare("bottom_right", LLRelPos::BOTTOM_RIGHT);
		declare("center",		LLRelPos::CENTER);
		declare("left",			LLRelPos::LEFT);
		declare("right",		LLRelPos::RIGHT);
		declare("top",			LLRelPos::TOP);
		declare("top_left",		LLRelPos::TOP_LEFT);
		declare("top_right",	LLRelPos::TOP_RIGHT);
	}
}


// eof
