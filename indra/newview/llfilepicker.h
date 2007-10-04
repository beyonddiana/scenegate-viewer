/** 
 * @file llfilepicker.h
 * @brief OS-specific file picker
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

// OS specific file selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. When you call getMultipleOpenFile(), it locks the picker
// until you iterate to the end of the list of selected files with
// getNextFile() or call reset().

#ifndef LL_LLFILEPICKER_H
#define LL_LLFILEPICKER_H

#include "stdtypes.h"

#if LL_DARWIN
#include <Carbon/Carbon.h>

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require

#include <vector>
#include "llstring.h"

#endif

// Need commdlg.h for OPENFILENAMEA
#ifdef LL_WINDOWS
#include <commdlg.h>
#endif

// mostly for Linux, possible on others
#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK

// also mostly for Linux, for some X11-specific filepicker usability tweaks
#if LL_X11
#include "SDL/SDL_syswm.h"
#endif

#if LL_GTK
// we use an aggregate structure so we can pass its pointer through a C callback
typedef struct {
	GtkWidget *win;
	std::vector<LLString> fileVector;
	std::string contextName;
} StoreFilenamesStruct;
#endif // LL_GTK

class LLFilePicker
{
#ifdef LL_GTK
	friend class LLDirPicker;
	friend void chooser_responder(GtkWidget *, gint, gpointer);
#endif // LL_GTK
public:
	// calling this before main() is undefined
	static LLFilePicker& instance( void ) { return sInstance; }

	enum ELoadFilter
	{
		FFLOAD_ALL = 1,
		FFLOAD_WAV = 2,
		FFLOAD_IMAGE = 3,
		FFLOAD_ANIM = 4,
#ifdef _CORY_TESTING
		FFLOAD_GEOMETRY = 5,
#endif
		FFLOAD_XML = 6,
		FFLOAD_SLOBJECT = 7,
		FFLOAD_RAW = 8,
	};

	enum ESaveFilter
	{
		FFSAVE_ALL = 1,
		FFSAVE_WAV = 3,
		FFSAVE_TGA = 4,
		FFSAVE_BMP = 5,
		FFSAVE_AVI = 6,
		FFSAVE_ANIM = 7,
#ifdef _CORY_TESTING
		FFSAVE_GEOMETRY = 8,
#endif
		FFSAVE_XML = 9,
		FFSAVE_COLLADA = 10,
		FFSAVE_RAW = 11,
		FFSAVE_J2C = 12,
	};

	// open the dialog. This is a modal operation
	BOOL getSaveFile( ESaveFilter filter = FFSAVE_ALL, const char* filename = NULL );
	BOOL getOpenFile( ELoadFilter filter = FFLOAD_ALL );
	BOOL getMultipleOpenFiles( ELoadFilter filter = FFLOAD_ALL );

	// Get the filename(s) found. getFirstFile() sets the pointer to
	// the start of the structure and allows the start of iteration.
	const char* getFirstFile();

	// getNextFile() increments the internal representation and
	// returns the next file specified by the user. Returns NULL when
	// no more files are left. Further calls to getNextFile() are
	// undefined.
	const char* getNextFile();

	// This utility function extracts the directory name but doesn't
	// do any incrementing. This is currently only supported when
	// you're opening multiple files.
	const char* getDirname();

	// clear any lists of buffers or whatever, and make sure the file
	// picker isn't locked.
	void reset();

private:
	enum
	{
		SINGLE_FILENAME_BUFFER_SIZE = 1024,
		//FILENAME_BUFFER_SIZE = 65536
		FILENAME_BUFFER_SIZE = 65000
	};
	
	void buildFilename( void );

#if LL_WINDOWS
	OPENFILENAMEW mOFN;				// for open and save dialogs
	char *mOpenFilter;
	WCHAR mFilesW[FILENAME_BUFFER_SIZE];

	BOOL setupFilter(ELoadFilter filter);
#endif

#if LL_DARWIN
	NavDialogCreationOptions mNavOptions;
	std::vector<LLString> mFileVector;
	UInt32 mFileIndex;
	
	OSStatus doNavChooseDialog(ELoadFilter filter);
	OSStatus doNavSaveDialog(ESaveFilter filter, const char* filename);
	void getFilePath(SInt32 index);
	void getFileName(SInt32 index);
	static Boolean navOpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode);
#endif

#if LL_GTK
	StoreFilenamesStruct mStoreFilenames;
	U32 mNextFileIndex;
	// we remember the last path that was accessed for a particular usage
	static std::map <std::string, std::string> sContextToPathMap;
#endif

	char mFiles[FILENAME_BUFFER_SIZE];	/*Flawfinder: ignore*/
	char mFilename[LL_MAX_PATH];	/*Flawfinder: ignore*/
	char* mCurrentFile;
	BOOL mLocked;
	BOOL mMultiFile;

	static LLFilePicker sInstance;
	
protected:
#if LL_GTK
        GtkWindow* buildFilePicker(bool is_save, bool is_folder,
				   std::string context = "generic");
#endif

public:
	// don't call these directly please.
	LLFilePicker();
	~LLFilePicker();
};

#endif
