/** 
 * @file llassettype.h
 * @brief Declaration of LLAssetType.
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

#ifndef LL_LLASSETTYPE_H
#define LL_LLASSETTYPE_H

#include <string>

#include "stdenums.h" 	// for EDragAndDropType

class LLAssetType
{
public:
	enum EType
	{
		AT_TEXTURE = 0,
			// Used for painting the faces of geometry.
			// Stored in typical j2c stream format.

		AT_SOUND = 1, 
			// Used to fill the aural spectrum.

		AT_CALLINGCARD = 2,
		    // Links instant message access to the user on the card.
			// : E.G. A card for yourself, for linden support, for
			// : the guy you were talking to in the coliseum.

		AT_LANDMARK = 3,
			// Links to places in the world with location and a screen shot or image saved.
			// : E.G. Home, linden headquarters, the coliseum, destinations where 
			// : we want to increase traffic.

		AT_SCRIPT = 4,
			// Valid scripts that can be attached to an object.
			// : E.G. Open a door, jump into the air.

		AT_CLOTHING = 5,
			// A collection of textures and parameters that can be worn by an avatar.

		AT_OBJECT = 6,
			// Any combination of textures, sounds, and scripts that are
			// associated with a fixed piece of geometry.
			// : E.G. A hot tub, a house with working door.

		AT_NOTECARD = 7,
			// Just text.

		AT_CATEGORY = 8,
			// Holds a collection of inventory items.
			// It's treated as an item in the inventory and therefore needs a type.

		AT_ROOT_CATEGORY = 9,
			// A user's root inventory category.
			// We decided to expose it visually, so it seems logical to fold
			// it into the asset types.

		AT_LSL_TEXT = 10,
		AT_LSL_BYTECODE = 11,
			// The LSL is the scripting language. 
			// We've split it into a text and bytecode representation.
		
		AT_TEXTURE_TGA = 12,
			// Uncompressed TGA texture.

		AT_BODYPART = 13,
			// A collection of textures and parameters that can be worn by an avatar.

		AT_TRASH = 14,
			// Only to be used as a marker for a category preferred type. 
			// Using this, we can throw things in the trash before completely deleting.

		AT_SNAPSHOT_CATEGORY = 15,
			// A marker for a folder meant for snapshots. 
			// No actual assets will be snapshots, though if there were, you
			// could interpret them as textures.

		AT_LOST_AND_FOUND = 16,
			// Used to stuff lost&found items into.

		AT_SOUND_WAV = 17,
			// Uncompressed sound.

		AT_IMAGE_TGA = 18,
			// Uncompressed image, non-square.
			// Not appropriate for use as a texture.

		AT_IMAGE_JPEG = 19,
			// Compressed image, non-square.
			// Not appropriate for use as a texture.

		AT_ANIMATION = 20,
			// Animation.

		AT_GESTURE = 21,
			// Gesture, sequence of animations, sounds, chat, wait steps.

		AT_SIMSTATE = 22,
			// Simstate file.

		AT_FAVORITE = 23,
			// favorite items

		AT_LINK = 24,
			// Inventory symbolic link

		AT_LINK_FOLDER = 25,
			// Inventory folder link

		AT_FOLDER_ENSEMBLE_START = 26,
		AT_FOLDER_ENSEMBLE_END = 45,
			// This range is reserved for special clothing folder types.

		AT_CURRENT_OUTFIT = 46,
			// Current outfit

		AT_OUTFIT = 47,
			// Predefined outfit ("look")

		AT_MY_OUTFITS = 48,
			// Folder that holds your outfits.
			
		AT_MESH = 49,
		    // Mesh data in our proprietary SLM format

		AT_COUNT = 50,
			// +*********************************************************+
			// |  TO ADD AN ELEMENT TO THIS ENUM:                        |
			// +*********************************************************+
			// | 1. INSERT BEFORE AT_COUNT                               |
			// | 2. INCREMENT AT_COUNT BY 1                              |
			// | 3. ADD TO LLAssetDictionary in LLAssetType.cpp          |
			// | 3. ADD TO DEFAULT_ASSET_FOR_INV in LLInventoryType.cpp  |
			// +*********************************************************+

		AT_NONE = -1
	};

	// machine transation between type and strings
	static EType 				lookup(const char* name); // safe conversion to std::string, *TODO: deprecate
	static EType 				lookup(const std::string& type_name);
	static const char*			lookup(EType asset_type);

	// translation from a type to a human readable form.
	static EType 				lookupHumanReadable(const char* desc_name); // safe conversion to std::string, *TODO: deprecate
	static EType 				lookupHumanReadable(const std::string& readable_name);
	static const char*			lookupHumanReadable(EType asset_type);

	// Generate a good default description. You may want to add a verb
	// or agent name after this depending on your application.
	static void 				generateDescriptionFor(LLAssetType::EType asset_type,
													   std::string& description);

	static EType 				getType(const std::string& desc_name);
	static const std::string&	getDesc(EType asset_type);
	static EDragAndDropType   	lookupDragAndDropType(EType asset_type);

	static bool 				lookupCanLink(EType asset_type);
	static bool 				lookupIsLinkType(EType asset_type);

	static const char*  		lookupCategoryName(EType asset_type);
	static bool 				lookupIsProtectedCategoryType(EType asset_type);
	static bool 				lookupIsEnsembleCategoryType(EType asset_type);

	/* TODO: Change return types from "const char *" to "const std::string &".
	This is fairly straightforward, but requires changing some calls to use .c_str().
	e.g.:
	-	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType));
	+	fprintf(fp, "\t\ttype\t%s\n", LLAssetType::lookup(mType).c_str());
	*/
	
private:
	// don't instantiate or derive one of these objects
	LLAssetType( void ) {}
	~LLAssetType( void ) {}
};

#endif // LL_LLASSETTYPE_H
