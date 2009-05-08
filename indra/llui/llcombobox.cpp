/** 
 * @file llcombobox.cpp
 * @brief LLComboBox base class
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

// A control that displays the name of the chosen item, which when
// clicked shows a scrolling box of options.

#define INSTANTIATE_GETCHILD_COMBOBOX

#include "linden_common.h"

// file includes
#include "llcombobox.h"

// common includes
#include "llstring.h"

// newview includes
#include "llbutton.h"
#include "llkeyboard.h"
#include "llscrolllistctrl.h"
#include "llwindow.h"
#include "llfloater.h"
#include "llscrollbar.h"
#include "llscrolllistcell.h"
#include "llscrolllistitem.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "v2math.h"
#include "lluictrlfactory.h"

// Globals
S32 LLCOMBOBOX_HEIGHT = 0;
S32 LLCOMBOBOX_WIDTH = 0;
S32 MAX_COMBO_WIDTH = 500;

template LLComboBox* LLView::getChild<LLComboBox>( const std::string& name, BOOL recurse, BOOL create_if_missing ) const;

static LLRegisterWidget<LLComboBox> register_combo_box("combo_box");

void LLComboBox::PreferredPositionValues::declareValues()
{
	declare("above", ABOVE);
	declare("below", BELOW);
}

LLComboBox::ItemParams::ItemParams()
:	label("label")
{
}


LLComboBox::Params::Params()
:	allow_text_entry("allow_text_entry", false),
	show_text_as_tentative("show_text_as_tentative", true),
	max_chars("max_chars", 20),
	arrow_image("arrow_image"),
	list_position("list_position", BELOW),
	items("item"),
	combo_button("combo_button"),
	combo_list("combo_list"),
	combo_editor("combo_editor")
{
	addSynonym(items, "combo_item");
}


LLComboBox::LLComboBox(const LLComboBox::Params& p)
:	LLUICtrl(p),
	mTextEntry(NULL),
	mTextEntryTentative(p.show_text_as_tentative),
	mAllowTextEntry(p.allow_text_entry),
	mMaxChars(p.max_chars),
	mPrearrangeCallback(p.prearrange_callback()),
	mTextEntryCallback(p.text_entry_callback()),
	mSelectionCallback(p.selection_callback()),
	mArrowImage(p.arrow_image),
	mListPosition(p.list_position)
{
	// Text label button

	LLButton::Params button_params = p.combo_button;
	button_params.mouse_down_callback.function(boost::bind(&LLComboBox::onButtonDown, this));
	button_params.follows.flags(FOLLOWS_LEFT|FOLLOWS_BOTTOM|FOLLOWS_RIGHT);
	button_params.rect(p.rect);
	button_params.pad_right(2);

	mButton = LLUICtrlFactory::create<LLButton>(button_params);
	mButton->setRightHPad(2);  //redo to compensate for button hack that leaves space for a character
	addChild(mButton);

	LLScrollListCtrl::Params params = p.combo_list;
	params.name("ComboBox");
	params.commit_callback.function(boost::bind(&LLComboBox::onItemSelected, this, _2));
	params.visible(false);
	params.commit_on_keyboard_movement(false);

	mList = LLUICtrlFactory::create<LLScrollListCtrl>(params);
	addChild(mList);

	for (LLInitParam::ParamIterator<ItemParams>::const_iterator it = p.items().begin();
		it != p.items().end();
		++it)
	{
		LLScrollListItem::Params item_params = *it;
		if (it->label.isProvided())
		{
			item_params.cells.add().value(it->label());
		}

		mList->addRow(item_params);
	}

	createLineEditor(p);

	setTopLostCallback(boost::bind(&LLComboBox::hideList, this));
}

void LLComboBox::initFromParams(const LLComboBox::Params& p)
{
	LLUICtrl::initFromParams(p);

	if (!acceptsTextInput() && mLabel.empty())
	{
		selectFirstItem();
	}
}

// virtual
BOOL LLComboBox::postBuild()
{
	if (mControlVariable)
	{
		setValue(mControlVariable->getValue()); // selects the appropriate item
	}
	return TRUE;
}


LLComboBox::~LLComboBox()
{
	// children automatically deleted, including mMenu, mButton
}


void LLComboBox::setEnabled(BOOL enabled)
{
	LLView::setEnabled(enabled);
	mButton->setEnabled(enabled);
}

void LLComboBox::clear()
{ 
	if (mTextEntry)
	{
		mTextEntry->setText(LLStringUtil::null);
	}
	mButton->setLabelSelected(LLStringUtil::null);
	mButton->setLabelUnselected(LLStringUtil::null);
	mButton->setDisabledLabel(LLStringUtil::null);
	mButton->setDisabledSelectedLabel(LLStringUtil::null);
	mList->deselectAllItems();
}

void LLComboBox::onCommit()
{
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		// we have selected an existing item, blitz the manual text entry with
		// the properly capitalized item
		mTextEntry->setValue(getSimple());
		mTextEntry->setTentative(FALSE);
	}
	setControlValue(getValue());
	LLUICtrl::onCommit();
}

// virtual
BOOL LLComboBox::isDirty() const
{
	BOOL grubby = FALSE;
	if ( mList )
	{
		grubby = mList->isDirty();
	}
	return grubby;
}

// virtual   Clear dirty state
void	LLComboBox::resetDirty()
{
	if ( mList )
	{
		mList->resetDirty();
	}
}


// add item "name" to menu
LLScrollListItem* LLComboBox::add(const std::string& name, EAddPosition pos, BOOL enabled)
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with a unique id to menu
LLScrollListItem* LLComboBox::add(const std::string& name, const LLUUID& id, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, id);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with attached userdata
LLScrollListItem* LLComboBox::add(const std::string& name, void* userdata, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	item->setUserdata( userdata );
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with attached generic data
LLScrollListItem* LLComboBox::add(const std::string& name, LLSD value, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, value);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

LLScrollListItem* LLComboBox::addSeparator(EAddPosition pos)
{
	return mList->addSeparator(pos);
}

void LLComboBox::sortByName(BOOL ascending)
{
	mList->sortOnce(0, ascending);
}


// Choose an item with a given name in the menu.
// Returns TRUE if the item was found.
BOOL LLComboBox::setSimple(const LLStringExplicit& name)
{
	BOOL found = mList->selectItemByLabel(name, FALSE);

	if (found)
	{
		setLabel(name);
	}

	return found;
}

// virtual
void LLComboBox::setValue(const LLSD& value)
{
	BOOL found = mList->selectByValue(value);
	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			setLabel( mList->getSelectedItemLabel() );
		}
	}
}

const std::string LLComboBox::getSimple() const
{
	const std::string res = mList->getSelectedItemLabel();
	if (res.empty() && mAllowTextEntry)
	{
		return mTextEntry->getText();
	}
	else
	{
		return res;
	}
}

const std::string LLComboBox::getSelectedItemLabel(S32 column) const
{
	return mList->getSelectedItemLabel(column);
}

// virtual
LLSD LLComboBox::getValue() const
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return item->getValue();
	}
	else if (mAllowTextEntry)
	{
		return mTextEntry->getValue();
	}
	else
	{
		return LLSD();
	}
}

void LLComboBox::setLabel(const LLStringExplicit& name)
{
	if ( mTextEntry )
	{
		mTextEntry->setText(name);
		if (mList->selectItemByLabel(name, FALSE))
		{
			mTextEntry->setTentative(FALSE);
		}
		else
		{
			mTextEntry->setTentative(mTextEntryTentative);
		}
	}
	
	if (!mAllowTextEntry)
	{
		mButton->setLabelUnselected(name);
		mButton->setLabelSelected(name);
		mButton->setDisabledLabel(name);
		mButton->setDisabledSelectedLabel(name);
	}
}


BOOL LLComboBox::remove(const std::string& name)
{
	BOOL found = mList->selectItemByLabel(name);

	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			mList->deleteSingleItem(mList->getItemIndex(item));
		}
	}

	return found;
}

BOOL LLComboBox::remove(S32 index)
{
	if (index < mList->getItemCount())
	{
		mList->deleteSingleItem(index);
		setLabel(mList->getSelectedItemLabel());
		return TRUE;
	}
	return FALSE;
}

// Keyboard focus lost.
void LLComboBox::onFocusLost()
{
	hideList();
	// if valid selection
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		mTextEntry->selectAll();
	}
	LLUICtrl::onFocusLost();
}

void LLComboBox::setButtonVisible(BOOL visible)
{
	static LLUICachedControl<S32> drop_shadow_button ("DropShadowButton", 0);

	mButton->setVisible(visible);
	if (mTextEntry)
	{
		LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
		if (visible)
		{
			text_entry_rect.mRight -= llmax(8,mArrowImage->getWidth()) + 2 * drop_shadow_button;
		}
		//mTextEntry->setRect(text_entry_rect);
		mTextEntry->reshape(text_entry_rect.getWidth(), text_entry_rect.getHeight(), TRUE);
	}
}

void LLComboBox::draw()
{
	mButton->setEnabled(getEnabled() /*&& !mList->isEmpty()*/);

	// Draw children normally
	LLUICtrl::draw();
}

BOOL LLComboBox::setCurrentByIndex( S32 index )
{
	BOOL found = mList->selectNthItem( index );
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}
	return found;
}

S32 LLComboBox::getCurrentIndex() const
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return mList->getItemIndex( item );
	}
	return -1;
}


void LLComboBox::createLineEditor(const LLComboBox::Params& p)
{
	static LLUICachedControl<S32> drop_shadow_button ("DropShadowButton", 0);
	LLRect rect = getLocalRect();
	if (mAllowTextEntry)
	{
		S32 shadow_size = drop_shadow_button;
		mButton->setRect(LLRect( getRect().getWidth() - llmax(8,mArrowImage->getWidth()) - 2 * shadow_size,
								rect.mTop, rect.mRight, rect.mBottom));
		mButton->setTabStop(FALSE);
		mButton->setHAlign(LLFontGL::HCENTER);

		LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
		text_entry_rect.mRight -= llmax(8,mArrowImage->getWidth()) + 2 * drop_shadow_button;
		// clear label on button
		std::string cur_label = mButton->getLabelSelected();
		LLLineEditor::Params params = p.combo_editor;
		params.rect(text_entry_rect);
		params.default_text(LLStringUtil::null);
		params.max_length_bytes(mMaxChars);
		params.commit_callback.function(boost::bind(&LLComboBox::onTextCommit, this, _2));
		params.keystroke_callback(boost::bind(&LLComboBox::onTextEntry, this, _1));
		params.focus_lost_callback(NULL);
		params.select_on_focus(true);
		params.handle_edit_keys_directly(true);
		params.commit_on_focus_lost(false);
		params.follows.flags(FOLLOWS_ALL);
		mTextEntry = LLUICtrlFactory::create<LLLineEditor> (params);
		mTextEntry->setText(cur_label);
		mTextEntry->setIgnoreTab(TRUE);
		addChild(mTextEntry);

		// clear label on button
		setLabel(LLStringUtil::null);

		mButton->setFollows(FOLLOWS_BOTTOM | FOLLOWS_TOP | FOLLOWS_RIGHT);
	}
	else
	{
		mButton->setRect(rect);
		mButton->setTabStop(TRUE);
		mButton->setHAlign(LLFontGL::LEFT);

		if (mTextEntry)
		{
			mTextEntry->setVisible(FALSE);
		}
		mButton->setFollowsAll();
	}
}

void* LLComboBox::getCurrentUserdata()
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return item->getUserdata();
	}
	return NULL;
}


void LLComboBox::showList()
{
	// Make sure we don't go off top of screen.
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	//HACK: shouldn't have to know about scale here
	mList->fitContents( 192, llfloor((F32)window_size.mY / LLUI::sGLScaleFactor.mV[VY]) - 50 );

	// Make sure that we can see the whole list
	LLRect root_view_local;
	LLView* root_view = getRootView();
	root_view->localRectToOtherView(root_view->getLocalRect(), &root_view_local, this);
	
	LLRect rect = mList->getRect();

	S32 min_width = getRect().getWidth();
	S32 max_width = llmax(min_width, MAX_COMBO_WIDTH);
	// make sure we have up to date content width metrics
	mList->calcColumnWidths();
	S32 list_width = llclamp(mList->getMaxContentWidth(), min_width, max_width);

	if (mListPosition == BELOW)
	{
		if (rect.getHeight() <= -root_view_local.mBottom)
		{
			// Move rect so it hangs off the bottom of this view
			rect.setLeftTopAndSize(0, 0, list_width, rect.getHeight() );
		}
		else
		{	
			// stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom > root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width, llmin(-root_view_local.mBottom, rect.getHeight()));
			}
			else
			{
				// move rect so it stacks on top of this view (clipped to size of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
			}
		}
	}
	else // ABOVE
	{
		if (rect.getHeight() <= root_view_local.mTop - getRect().getHeight())
		{
			// move rect so it stacks on top of this view (clipped to size of screen)
			rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
		}
		else
		{
			// stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom > root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width, llmin(-root_view_local.mBottom, rect.getHeight()));
			}
			else
			{
				// move rect so it stacks on top of this view (clipped to size of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
			}
		}

	}
	mList->setOrigin(rect.mLeft, rect.mBottom);
	mList->reshape(rect.getWidth(), rect.getHeight());
	mList->translateIntoRect(root_view_local, FALSE);

	// Make sure we didn't go off bottom of screen
	S32 x, y;
	mList->localPointToScreen(0, 0, &x, &y);

	if (y < 0)
	{
		mList->translate(0, -y);
	}

	// NB: this call will trigger the focuslost callback which will hide the list, so do it first
	// before finally showing the list

	mList->setFocus(TRUE);

	// register ourselves as a "top" control
	// effectively putting us into a special draw layer
	// and not affecting the bounding rectangle calculation
	gFocusMgr.setTopCtrl(this);

	// Show the list and push the button down
	mButton->setToggleState(TRUE);
	mList->setVisible(TRUE);
	
	setUseBoundingRect(TRUE);
}

void LLComboBox::hideList()
{
	//*HACK: store the original value explicitly somewhere, not just in label
	std::string orig_selection = mAllowTextEntry ? mTextEntry->getText() : mButton->getLabelSelected();

	// assert selection in list
	mList->selectItemByLabel(orig_selection, FALSE);

	mButton->setToggleState(FALSE);
	mList->setVisible(FALSE);
	mList->mouseOverHighlightNthItem(-1);

	setUseBoundingRect(FALSE);
	if( gFocusMgr.getTopCtrl() == this )
	{
		gFocusMgr.setTopCtrl(NULL);
	}
}

void LLComboBox::onButtonDown()
{
	if (!mList->getVisible())
	{
		LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
		if (last_selected_item)
		{
			// highlight the original selection before potentially selecting a new item
			mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
		}

		if( mPrearrangeCallback )
		{
			mPrearrangeCallback( this, LLSD() );
		}

		if (mList->getItemCount() != 0)
		{
			showList();
		}

		setFocus( TRUE );

		// pass mouse capture on to list if button is depressed
		if (mButton->hasMouseCapture())
		{
			gFocusMgr.setMouseCapture(mList);
		}
	}
	else
	{
		hideList();
	} 

}


//------------------------------------------------------------------
// static functions
//------------------------------------------------------------------

void LLComboBox::onItemSelected(const LLSD& data)
{
	const std::string name = mList->getSelectedItemLabel();

	S32 cur_id = getCurrentIndex();
	if (cur_id != -1)
	{
		setLabel(name);

		if (mAllowTextEntry)
		{
			gFocusMgr.setKeyboardFocus(mTextEntry);
			mTextEntry->selectAll();
		}
	}

	// hiding the list reasserts the old value stored in the text editor/dropdown button
	hideList();

	// commit does the reverse, asserting the value in the list
	onCommit();

	// call the callback if it exists
	if(mSelectionCallback)
	{
		mSelectionCallback(this, data);
	}
}

BOOL LLComboBox::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
    std::string tool_tip;

	if(LLUICtrl::handleToolTip(x, y, msg, sticky_rect_screen))
	{
		return TRUE;
	}
	
	if (LLUI::sShowXUINames)
	{
		tool_tip = getShowNamesToolTip();
	}
	else
	{
		tool_tip = getToolTip();
		if (tool_tip.empty())
		{
			tool_tip = getSelectedItemLabel();
		}
	}
	
	if( !tool_tip.empty() )
	{
		msg = tool_tip;

		// Convert rect local to screen coordinates
		localPointToScreen( 
			0, 0, 
			&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
		localPointToScreen(
			getRect().getWidth(), getRect().getHeight(),
			&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );
	}
	return TRUE;
}

BOOL LLComboBox::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = FALSE;
	if (hasFocus())
	{
		if (mList->getVisible() 
			&& key == KEY_ESCAPE && mask == MASK_NONE)
		{
			hideList();
			return TRUE;
		}
		//give list a chance to pop up and handle key
		LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
		if (last_selected_item)
		{
			// highlight the original selection before potentially selecting a new item
			mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
		}
		result = mList->handleKeyHere(key, mask);

		// will only see return key if it is originating from line editor
		// since the dropdown button eats the key
		if (key == KEY_RETURN)
		{
			// don't show list and don't eat key input when committing
			// free-form text entry with RETURN since user already knows
            // what they are trying to select
			return FALSE;
		}
		// if selection has changed, pop open list
		else if (mList->getLastSelectedItem() != last_selected_item)
		{
			showList();
		}
	}
	return result;
}

BOOL LLComboBox::handleUnicodeCharHere(llwchar uni_char)
{
	BOOL result = FALSE;
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		// space bar just shows the list
		if (' ' != uni_char )
		{
			LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
			if (last_selected_item)
			{
				// highlight the original selection before potentially selecting a new item
				mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
			}
			result = mList->handleUnicodeCharHere(uni_char);
			if (mList->getLastSelectedItem() != last_selected_item)
			{
				showList();
			}
		}
	}
	return result;
}

void LLComboBox::setTextEntry(const LLStringExplicit& text)
{
	if (mTextEntry)
	{
		mTextEntry->setText(text);
		updateSelection();
	}
}

void LLComboBox::onTextEntry(LLLineEditor* line_editor)
{
	if (mTextEntryCallback != NULL)
	{
		(mTextEntryCallback)(line_editor, LLSD());
	}

	KEY key = gKeyboard->currentKey();
	if (key == KEY_BACKSPACE || 
		key == KEY_DELETE)
	{
		if (mList->selectItemByLabel(line_editor->getText(), FALSE))
		{
			line_editor->setTentative(FALSE);
		}
		else
		{
			line_editor->setTentative(mTextEntryTentative);
			mList->deselectAllItems();
		}
		return;
	}

	if (key == KEY_LEFT || 
		key == KEY_RIGHT)
	{
		return;
	}

	if (key == KEY_DOWN)
	{
		setCurrentByIndex(llmin(getItemCount() - 1, getCurrentIndex() + 1));
		if (!mList->getVisible())
		{
			if( mPrearrangeCallback )
			{
				mPrearrangeCallback( this, LLSD() );
			}

			if (mList->getItemCount() != 0)
			{
				showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else if (key == KEY_UP)
	{
		setCurrentByIndex(llmax(0, getCurrentIndex() - 1));
		if (!mList->getVisible())
		{
			if( mPrearrangeCallback )
			{
				mPrearrangeCallback( this, LLSD() );
			}

			if (mList->getItemCount() != 0)
			{
				showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else
	{
		// RN: presumably text entry
		updateSelection();
	}
}

void LLComboBox::updateSelection()
{
	LLWString left_wstring = mTextEntry->getWText().substr(0, mTextEntry->getCursor());
	// user-entered portion of string, based on assumption that any selected
    // text was a result of auto-completion
	LLWString user_wstring = mTextEntry->hasSelection() ? left_wstring : mTextEntry->getWText();
	std::string full_string = mTextEntry->getText();

	// go ahead and arrange drop down list on first typed character, even
	// though we aren't showing it... some code relies on prearrange
	// callback to populate content
	if( mTextEntry->getWText().size() == 1 )
	{
		if (mPrearrangeCallback)
		{
			mPrearrangeCallback( this, LLSD() );
		}
	}

	if (mList->selectItemByLabel(full_string, FALSE))
	{
		mTextEntry->setTentative(FALSE);
	}
	else if (!mList->selectItemByPrefix(left_wstring, FALSE))
	{
		mList->deselectAllItems();
		mTextEntry->setText(wstring_to_utf8str(user_wstring));
		mTextEntry->setTentative(mTextEntryTentative);
	}
	else
	{
		LLWString selected_item = utf8str_to_wstring(mList->getSelectedItemLabel());
		LLWString wtext = left_wstring + selected_item.substr(left_wstring.size(), selected_item.size());
		mTextEntry->setText(wstring_to_utf8str(wtext));
		mTextEntry->setSelection(left_wstring.size(), mTextEntry->getWText().size());
		mTextEntry->endSelection();
		mTextEntry->setTentative(FALSE);
	}
}

void LLComboBox::onTextCommit(const LLSD& data)
{
	std::string text = mTextEntry->getText();
	setSimple(text);
	onCommit();
	mTextEntry->selectAll();
}

void LLComboBox::setFocus(BOOL b)
{
	LLUICtrl::setFocus(b);

	if (b)
	{
		mList->clearSearchString();
		if (mList->getVisible())
		{
			mList->setFocus(TRUE);
		}
	}
}

//============================================================================
// LLCtrlListInterface functions

S32 LLComboBox::getItemCount() const
{
	return mList->getItemCount();
}

void LLComboBox::addColumn(const LLSD& column, EAddPosition pos)
{
	mList->clearColumns();
	mList->addColumn(column, pos);
}

void LLComboBox::clearColumns()
{
	mList->clearColumns();
}

void LLComboBox::setColumnLabel(const std::string& column, const std::string& label)
{
	mList->setColumnLabel(column, label);
}

LLScrollListItem* LLComboBox::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	return mList->addElement(value, pos, userdata);
}

LLScrollListItem* LLComboBox::addSimpleElement(const std::string& value, EAddPosition pos, const LLSD& id)
{
	return mList->addSimpleElement(value, pos, id);
}

void LLComboBox::clearRows()
{
	mList->clearRows();
}

void LLComboBox::sortByColumn(const std::string& name, BOOL ascending)
{
	mList->sortByColumn(name, ascending);
}

//============================================================================
//LLCtrlSelectionInterface functions

BOOL LLComboBox::setCurrentByID(const LLUUID& id)
{
	BOOL found = mList->selectByID( id );

	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}

	return found;
}

LLUUID LLComboBox::getCurrentID() const
{
	return mList->getStringUUIDSelectedItem();
}
BOOL LLComboBox::setSelectedByValue(const LLSD& value, BOOL selected)
{
	BOOL found = mList->setSelectedByValue(value, selected);
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
	}
	return found;
}

LLSD LLComboBox::getSelectedValue()
{
	return mList->getSelectedValue();
}

BOOL LLComboBox::isSelected(const LLSD& value) const
{
	return mList->isSelected(value);
}

BOOL LLComboBox::operateOnSelection(EOperation op)
{
	if (op == OP_DELETE)
	{
		mList->deleteSelectedItems();
		return TRUE;
	}
	return FALSE;
}

BOOL LLComboBox::operateOnAll(EOperation op)
{
	if (op == OP_DELETE)
	{
		clearRows();
		return TRUE;
	}
	return FALSE;
}

BOOL LLComboBox::selectItemRange( S32 first, S32 last )
{
	return mList->selectItemRange(first, last);
}
