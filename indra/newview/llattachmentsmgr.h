/** 
 * @file llattachmentsmgr.h
 * @brief Batches up attachment requests and sends them all
 * in one message.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLATTACHMENTSMGR_H
#define LL_LLATTACHMENTSMGR_H

#include "llsingleton.h"

class LLViewerInventoryItem;

//--------------------------------------------------------------------------------
// LLAttachmentsMgr
// 
// This class manages batching up of requests at two stages of
// attachment rezzing.
//
// First, attachments requested to rez get saved in
// mPendingAttachments and sent as a single
// RezMultipleAttachmentsFromInv request. This batching is needed
// mainly because of weaknessing the UI element->inventory item
// handling, such that we don't always know when we are requesting
// multiple items. Now they just pile up and get swept into a single
// request during the idle loop.
//
// Second, after attachments arrive, we need to generate COF links for
// them. There are both efficiency and UI correctness reasons why it
// is better to request all the COF links at once and run a single
// callback after they all complete. Given the vagaries of the
// attachment system, there is no guarantee that we will get all the
// attachments we ask for, but we frequently do. So in the common case
// that all the desired attachments arrive fairly quickly, we generate
// a single batched request for COF links. If attachments arrive late
// or not at all, we will still issue COF link requests once a timeout
// value has been exceeded.
//
// To handle attachments that never arrive, we forget about requests
// that exceed a timeout value.
//--------------------------------------------------------------------------------
class LLAttachmentsMgr: public LLSingleton<LLAttachmentsMgr>
{
public:
    // Stores info for attachments that will be requested during idle.
	struct AttachmentsInfo
	{
		LLUUID mItemID;
		U8 mAttachmentPt;
		BOOL mAdd;
	};
	typedef std::vector<AttachmentsInfo> attachments_vec_t;

	LLAttachmentsMgr();
	virtual ~LLAttachmentsMgr();

	void addAttachmentRequest(const LLUUID& item_id,
                              const U8 attachment_pt,
                              const BOOL add);
	void requestAttachments(const attachments_vec_t& attachment_requests);
	static void onIdle(void *);

	BOOL attachmentWasRequestedRecently(const LLUUID& inv_item_id) const;
	void addAttachmentRequestTime(const LLUUID& inv_item_id);
    void onAttachmentArrived(const LLUUID& inv_item_id);

private:
	void removeAttachmentRequestTime(const LLUUID& inv_item_id);
	void onIdle();
	void requestPendingAttachments();
	void linkRecentlyArrivedAttachments();
    void expireOldAttachmentRequests();

    // Attachments that we are planning to rez but haven't requested from the server yet.
	attachments_vec_t mPendingAttachments;

	// Attachments that have been requested from server but have not arrived yet.
	std::map<LLUUID,LLTimer> mAttachmentRequests;

    // Attachments that have arrived but have not been linked in the COF yet.
    std::set<LLUUID> mRecentlyArrivedAttachments;
    LLTimer mCOFLinkBatchTimer;
};

#endif
