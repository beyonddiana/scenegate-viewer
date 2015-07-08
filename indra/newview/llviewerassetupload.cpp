/**
* @file llviewerassetupload.cpp
* @author optional
* @brief brief description of the file
*
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "linden_common.h"
#include "llviewertexturelist.h"
#include "llimage.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llvorbisencode.h"
#include "lluploaddialog.h"
#include "llpreviewscript.h"
#include "llnotificationsutil.h"
#include "lleconomy.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llstatusbar.h"
#include "llinventorypanel.h"
#include "llsdutil.h"
#include "llviewerassetupload.h"
#include "llappviewer.h"
#include "llviewerstats.h"
#include "llvfile.h"

LLSD NewResourceUploadInfo::prepareUpload()
{
    if (mAssetId.isNull())
        generateNewAssetId();

    incrementUploadStats();
    assignDefaults();

    return LLSD().with("success", LLSD::Boolean(true));
}

std::string NewResourceUploadInfo::getAssetTypeString() const
{
    return LLAssetType::lookup(mAssetType);
}

std::string NewResourceUploadInfo::getInventoryTypeString() const
{
    return LLInventoryType::lookup(mInventoryType);
}

LLSD NewResourceUploadInfo::generatePostBody()
{
    LLSD body;

    body["folder_id"] = mFolderId;
    body["asset_type"] = getAssetTypeString();
    body["inventory_type"] = getInventoryTypeString();
    body["name"] = mName;
    body["description"] = mDescription;
    body["next_owner_mask"] = LLSD::Integer(mNextOwnerPerms);
    body["group_mask"] = LLSD::Integer(mGroupPerms);
    body["everyone_mask"] = LLSD::Integer(mEveryonePerms);

    return body;

}

void NewResourceUploadInfo::logPreparedUpload()
{
    LL_INFOS() << "*** Uploading: " << std::endl <<
        "Type: " << LLAssetType::lookup(mAssetType) << std::endl <<
        "UUID: " << mAssetId.asString() << std::endl <<
        "Name: " << mName << std::endl <<
        "Desc: " << mDescription << std::endl <<
        "Expected Upload Cost: " << mExpectedUploadCost << std::endl <<
        "Folder: " << mFolderId << std::endl <<
        "Asset Type: " << LLAssetType::lookup(mAssetType) << LL_ENDL;
}

LLUUID NewResourceUploadInfo::finishUpload(LLSD &result)
{
    if (getFolderId().isNull())
    {
        return LLUUID::null;
    }

    U32 permsEveryone = PERM_NONE;
    U32 permsGroup = PERM_NONE;
    U32 permsNextOwner = PERM_ALL;

    if (result.has("new_next_owner_mask"))
    {
        // The server provided creation perms so use them.
        // Do not assume we got the perms we asked for in
        // since the server may not have granted them all.
        permsEveryone = result["new_everyone_mask"].asInteger();
        permsGroup = result["new_group_mask"].asInteger();
        permsNextOwner = result["new_next_owner_mask"].asInteger();
    }
    else
    {
        // The server doesn't provide creation perms
        // so use old assumption-based perms.
        if (getAssetTypeString() != "snapshot")
        {
            permsNextOwner = PERM_MOVE | PERM_TRANSFER;
        }
    }

    LLPermissions new_perms;
    new_perms.init(
        gAgent.getID(),
        gAgent.getID(),
        LLUUID::null,
        LLUUID::null);

    new_perms.initMasks(
        PERM_ALL,
        PERM_ALL,
        permsEveryone,
        permsGroup,
        permsNextOwner);

    U32 flagsInventoryItem = 0;
    if (result.has("inventory_flags"))
    {
        flagsInventoryItem = static_cast<U32>(result["inventory_flags"].asInteger());
        if (flagsInventoryItem != 0)
        {
            LL_INFOS() << "inventory_item_flags " << flagsInventoryItem << LL_ENDL;
        }
    }
    S32 creationDate = time_corrected();

    LLUUID serverInventoryItem = result["new_inventory_item"].asUUID();
    LLUUID serverAssetId = result["new_asset"].asUUID();

    LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(
        serverInventoryItem,
        getFolderId(),
        new_perms,
        serverAssetId,
        getAssetType(),
        getInventoryType(),
        getName(),
        getDescription(),
        LLSaleInfo::DEFAULT,
        flagsInventoryItem,
        creationDate);

    gInventory.updateItem(item);
    gInventory.notifyObservers();

    return serverInventoryItem;
}


LLAssetID NewResourceUploadInfo::generateNewAssetId()
{
    if (gDisconnected)
    {
        LLAssetID rv;

        rv.setNull();
        return rv;
    }
    mAssetId = mTransactionId.makeAssetID(gAgent.getSecureSessionID());

    return mAssetId;
}

void NewResourceUploadInfo::incrementUploadStats() const
{
    if (LLAssetType::AT_SOUND == mAssetType)
    {
        add(LLStatViewer::UPLOAD_SOUND, 1);
    }
    else if (LLAssetType::AT_TEXTURE == mAssetType)
    {
        add(LLStatViewer::UPLOAD_TEXTURE, 1);
    }
    else if (LLAssetType::AT_ANIMATION == mAssetType)
    {
        add(LLStatViewer::ANIMATION_UPLOADS, 1);
    }
}

void NewResourceUploadInfo::assignDefaults()
{
    if (LLInventoryType::IT_NONE == mInventoryType)
    {
        mInventoryType = LLInventoryType::defaultForAssetType(mAssetType);
    }
    LLStringUtil::stripNonprintable(mName);
    LLStringUtil::stripNonprintable(mDescription);

    if (mName.empty())
    {
        mName = "(No Name)";
    }
    if (mDescription.empty())
    {
        mDescription = "(No Description)";
    }

    mFolderId = gInventory.findCategoryUUIDForType(
        (mDestinationFolderType == LLFolderType::FT_NONE) ?
        (LLFolderType::EType)mAssetType : mDestinationFolderType);

}

std::string NewResourceUploadInfo::getDisplayName() const
{
    return (mName.empty()) ? mAssetId.asString() : mName;
};

//=========================================================================
NewFileResourceUploadInfo::NewFileResourceUploadInfo(
    std::string fileName,
    std::string name,
    std::string description,
    S32 compressionInfo,
    LLFolderType::EType destinationType,
    LLInventoryType::EType inventoryType,
    U32 nextOWnerPerms,
    U32 groupPerms,
    U32 everyonePerms,
    S32 expectedCost) :
    NewResourceUploadInfo(name, description, compressionInfo,
    destinationType, inventoryType,
    nextOWnerPerms, groupPerms, everyonePerms, expectedCost),
    mFileName(fileName)
{
    LLTransactionID tid;
    tid.generate();
    setTransactionId(tid);
}



LLSD NewFileResourceUploadInfo::prepareUpload()
{
    generateNewAssetId();

    LLSD result = exportTempFile();
    if (result.has("error"))
        return result;

    return NewResourceUploadInfo::prepareUpload();
}

LLSD NewFileResourceUploadInfo::exportTempFile()
{
    std::string filename = gDirUtilp->getTempFilename();

    std::string exten = gDirUtilp->getExtension(getFileName());
    U32 codec = LLImageBase::getCodecFromExtension(exten);

    LLAssetType::EType assetType = LLAssetType::AT_NONE;
    std::string errorMessage;
    std::string errorLabel;

    bool error = false;

    if (exten.empty())
    {
        std::string shortName = gDirUtilp->getBaseFileName(filename);

        // No extension
        errorMessage = llformat(
            "No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
            shortName.c_str());
        errorLabel = "NoFileExtension";
        error = true;
    }
    else if (codec != IMG_CODEC_INVALID)
    {
        // It's an image file, the upload procedure is the same for all
        assetType = LLAssetType::AT_TEXTURE;
        if (!LLViewerTextureList::createUploadFile(getFileName(), filename, codec))
        {
            errorMessage = llformat("Problem with file %s:\n\n%s\n",
                getFileName().c_str(), LLImage::getLastError().c_str());
            errorLabel = "ProblemWithFile";
            error = true;
        }
    }
    else if (exten == "wav")
    {
        assetType = LLAssetType::AT_SOUND;  // tag it as audio
        S32 encodeResult = 0;

        LL_INFOS() << "Attempting to encode wav as an ogg file" << LL_ENDL;

        encodeResult = encode_vorbis_file(getFileName(), filename);

        if (LLVORBISENC_NOERR != encodeResult)
        {
            switch (encodeResult)
            {
            case LLVORBISENC_DEST_OPEN_ERR:
                errorMessage = llformat("Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());
                errorLabel = "CannotOpenTemporarySoundFile";
                break;

            default:
                errorMessage = llformat("Unknown vorbis encode failure on: %s\n", getFileName().c_str());
                errorLabel = "UnknownVorbisEncodeFailure";
                break;
            }
            error = true;
        }
    }
    else if (exten == "bvh")
    {
        errorMessage = llformat("We do not currently support bulk upload of animation files\n");
        errorLabel = "DoNotSupportBulkAnimationUpload";
        error = true;
    }
    else if (exten == "anim")
    {
        assetType = LLAssetType::AT_ANIMATION;
        filename = getFileName();
    }
    else
    {
        // Unknown extension
        errorMessage = llformat(LLTrans::getString("UnknownFileExtension").c_str(), exten.c_str());
        errorLabel = "ErrorMessage";
        error = TRUE;;
    }

    if (error)
    {
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        errorResult["label"] = errorLabel;
        return errorResult;
    }

    setAssetType(assetType);

    // copy this file into the vfs for upload
    S32 file_size;
    LLAPRFile infile;
    infile.open(filename, LL_APR_RB, NULL, &file_size);
    if (infile.getFileHandle())
    {
        LLVFile file(gVFS, getAssetId(), assetType, LLVFile::WRITE);

        file.setMaxSize(file_size);

        const S32 buf_size = 65536;
        U8 copy_buf[buf_size];
        while ((file_size = infile.read(copy_buf, buf_size)))
        {
            file.write(copy_buf, file_size);
        }
    }
    else
    {
        errorMessage = llformat("Unable to access output file: %s", filename.c_str());
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        return errorResult;
    }

    return LLSD();

}

//=========================================================================


//=========================================================================
/*static*/
void LLViewerAssetUpload::AssetInventoryUploadCoproc(LLCoros::self &self, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, const LLUUID &id,
    std::string url, NewResourceUploadInfo::ptr_t uploadInfo)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = uploadInfo->prepareUpload();
    uploadInfo->logPreparedUpload();

    if (result.has("error"))
    {
        HandleUploadError(LLCore::HttpStatus(499), result, uploadInfo);
        return;
    }

    //self.yield();

    std::string uploadMessage = "Uploading...\n\n";
    uploadMessage.append(uploadInfo->getDisplayName());
    LLUploadDialog::modalUploadDialog(uploadMessage);

    LLSD body = uploadInfo->generatePostBody();

    result = httpAdapter->postAndYield(self, httpRequest, url, body);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if ((!status) || (result.has("error")))
    {
        HandleUploadError(status, result, uploadInfo);
        LLUploadDialog::modalUploadFinished();
        return;
    }

    std::string uploader = result["uploader"].asString();

    result = httpAdapter->postFileAndYield(self, httpRequest, uploader, uploadInfo->getAssetId(), uploadInfo->getAssetType());
    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        HandleUploadError(status, result, uploadInfo);
        LLUploadDialog::modalUploadFinished();
        return;
    }

    S32 uploadPrice = 0;

    // Update L$ and ownership credit information
    // since it probably changed on the server
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE ||
        uploadInfo->getAssetType() == LLAssetType::AT_SOUND ||
        uploadInfo->getAssetType() == LLAssetType::AT_ANIMATION ||
        uploadInfo->getAssetType() == LLAssetType::AT_MESH)
    {
        uploadPrice = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
    }

    bool success = false;

    if (uploadPrice > 0)
    {
        // this upload costed us L$, update our balance
        // and display something saying that it cost L$
        LLStatusBar::sendMoneyBalanceRequest();

        LLSD args;
        args["AMOUNT"] = llformat("%d", uploadPrice);
        LLNotificationsUtil::add("UploadPayment", args);
    }

    LLUUID serverInventoryItem = uploadInfo->finishUpload(result);

    if (serverInventoryItem.notNull())
    {
        success = true;

        // Show the preview panel for textures and sounds to let
        // user know that the image (or snapshot) arrived intact.
        LLInventoryPanel* panel = LLInventoryPanel::getActiveInventoryPanel();
        if (panel)
        {
            LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();
            panel->setSelection(serverInventoryItem, TAKE_FOCUS_NO);

            // restore keyboard focus
            gFocusMgr.setKeyboardFocus(focus);
        }
    }
    else
    {
        LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
    }

    // remove the "Uploading..." message
    LLUploadDialog::modalUploadFinished();

    // Let the Snapshot floater know we have finished uploading a snapshot to inventory.
    LLFloater* floater_snapshot = LLFloaterReg::findInstance("snapshot");
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE && floater_snapshot)
    {
        floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", success).with("msg", "inventory")));
    }
}

//=========================================================================
/*static*/
void LLViewerAssetUpload::HandleUploadError(LLCore::HttpStatus status, LLSD &result, NewResourceUploadInfo::ptr_t &uploadInfo)
{
    std::string reason;
    std::string label("CannotUploadReason");

    LL_WARNS() << ll_pretty_print_sd(result) << LL_ENDL;

    if (result.has("label"))
    {
        label = result["label"].asString();
    }

    if (result.has("message"))
    {
        reason = result["message"].asString();
    }
    else
    {
        if (status.getType() == 499)
        {
            reason = "The server is experiencing unexpected difficulties.";
        }
        else
        {
            reason = "Error in upload request.  Please visit "
                "http://secondlife.com/support for help fixing this problem.";
        }
    }

    LLSD args;
    args["FILE"] = uploadInfo->getDisplayName();
    args["REASON"] = reason;

    LLNotificationsUtil::add(label, args);

    // unfreeze script preview
    if (uploadInfo->getAssetType() == LLAssetType::AT_LSL_TEXT)
    {
        LLPreviewLSL* preview = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", 
            uploadInfo->getItemId());
        if (preview)
        {
            LLSD errors;
            errors.append(LLTrans::getString("UploadFailed") + reason);
            preview->callbackLSLCompileFailed(errors);
        }
    }

}

