/** 
 * @file llpreeditor.h
 * @brief abstract class that defines interface for components to feedback preedits to users.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_PREEDITOR
#define LL_PREEDITOR

class LLPreeditor
{
public:

	typedef std::vector<S32> segment_lengths_t;
	typedef std::vector<BOOL> standouts_t;
	
	// We don't delete against LLPreeditor, but compilers complain without this...
	
	virtual ~LLPreeditor() {};

	// Discard any preedit info. on this preeditor.
	
	virtual void resetPreedit() = 0;

	// Update the preedit feedback using specified details.
	// Existing preedit is discarded and replaced with the new one.  (I.e., updatePreedit is not cumulative.) 
	// All arguments are IN.
	// preedit_count is the number of elements in arrays preedit_list and preedit_standouts.
	// preedit list is an array of preedit texts (clauses.)
	// preedit_standouts indicates whether each preedit text should be shown as standout clause.
	// caret_position is the preedit-local position of text editing caret, in # of llwchar.
	
	virtual void updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position) = 0;

	// Turn the specified sub-contents into an active preedit.
	// Both position and length are IN and count with UTF-32 (llwchar) characters.
	// This method primarily facilitates reconversion.

	virtual void markAsPreedit(S32 position, S32 length) = 0;

	// Get the position and the length of the active preedit in the contents.
	// Both position and length are OUT and count with UTF-32 (llwchar) characters.
	// When this preeditor has no active preedit, position receives
	// the caret position, and length receives 0.

	virtual void getPreeditRange(S32 *position, S32 *length) const = 0;

	// Get the position and the length of the current selection in the contents.
	// Both position and length are OUT and count with UTF-32 (llwchar) characters.
	// When this preeditor has no selection, position receives
	// the caret position, and length receives 0.

	virtual void getSelectionRange(S32 *position, S32 *length) const = 0;

	// Get the locations where the preedit and related UI elements are displayed.
	// Locations are relative to the app window and measured in GL coordinate space (before scaling.)
	// query_position is IN argument, and other three are OUT.

	virtual BOOL getPreeditLocation(S32 query_position, LLCoordGL *coord, LLRect *bounds, LLRect *control) const = 0;

	// Get the size (height) of the current font used in this preeditor.

	virtual S32 getPreeditFontSize() const = 0;

	// Get the contents of this preeditor as a LLWString.  If there is an active preedit,
	// the returned LLWString contains it.

	virtual const LLWString & getWText() const = 0;

	// Handle a UTF-32 char on this preeditor, i.e., add the character
	// to the contents.
	// This is a back door of the method of same name of LLWindowCallback.
	// called_from_parent should be set to FALSE if calling through LLPreeditor.

	virtual BOOL handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent) = 0;
};

#endif
