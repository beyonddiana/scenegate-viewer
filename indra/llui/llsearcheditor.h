/** 
 * @file llsearcheditor.h
 * @brief Text editor widget that represents a search operation
 *
 * Features: 
 *		Text entry of a single line (text, delete, left and right arrow, insert, return).
 *		Callbacks either on every keystroke or just on the return key.
 *		Focus (allow multiple text entry widgets)
 *		Clipboard (cut, copy, and paste)
 *		Horizontal scrolling to allow strings longer than widget size allows 
 *		Pre-validation (limit which keys can be used)
 *		Optional line history so previous entries can be recalled by CTRL UP/DOWN
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

#ifndef LL_SEARCHEDITOR_H
#define LL_SEARCHEDITOR_H

#include "lllineeditor.h"
#include "llbutton.h"

class LLSearchEditor : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLLineEditor::Params>
	{
		Optional<LLButton::Params> search_button;

		Params()
		: search_button("search_button")
		{
			name = "search_editor";
		}
	};

protected:
	LLSearchEditor(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSearchEditor() {}

	void setText(const LLStringExplicit &new_text) { mSearchEditor->setText(new_text); }
	const std::string& getText() const		{ return mSearchEditor->getText(); }


	// LLUICtrl interface
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual void	clear();

	void	setKeystrokeCallback(LLLineEditor::callback_t callback, void* user_data)
	{
		if(mSearchEditor)
			mSearchEditor->setKeystrokeCallback(callback,user_data);
	}

private:
	LLLineEditor* mSearchEditor;
	LLButton* mSearchButton;
};

#endif  // LL_SEARCHEDITOR_H
