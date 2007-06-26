/** 
 * @file llvoinventorylistener.h
 * @brief Interface for classes that wish to receive updates about viewer object inventory
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Description of LLVOInventoryListener class, which is an interface
// for windows that are interested in updates to a ViewerObject's inventory.

#ifndef LL_LLVOINVENTORYLISTENER_H
#define LL_LLVOINVENTORYLISTENER_H

#include "llviewerobject.h"

class LLVOInventoryListener
{
public:
	virtual void inventoryChanged(LLViewerObject* object,
								 InventoryObjectList* inventory,
								 S32 serial_num,
								 void* user_data) = 0;

	// Remove the listener from the object and clear this listener
	void removeVOInventoryListener();

	// Just clear this listener, don't worry about the object.
	void clearVOInventoryListener();

protected:
	LLVOInventoryListener() : mListenerVObject(NULL) { }
	virtual ~LLVOInventoryListener() { removeVOInventoryListener(); }

	void registerVOInventoryListener(LLViewerObject* object, void* user_data);
	void requestVOInventory();

private:
	// LLViewerObject is normally wrapped by an LLPointer, but not in
	// this case, because it's already sure to be kept alive by
	// LLPointers held by other objects that have longer lifetimes
	// than this one.  Plumbing correct LLPointer usage all the way
	// down here has been deemed too much work for now.
	LLViewerObject *mListenerVObject;
};

#endif

