/**
 * @file lleasymessagereader.cpp
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
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
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "lleasymessagereader.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "llsdserialize.h"

#include <pugixml.hpp>
#include <libxml2/libxml/parser.h>
#include <boost/algorithm/string/predicate.hpp>

//I doubt any of this is thread safe!
LLEasyMessageLogEntry::LLEasyMessageLogEntry(LogPayload entry, LLEasyMessageReader* message_reader)
:	LLMessageLogEntry((*entry))
,	mEasyMessageReader(message_reader)
,	mResponseMsg(NULL)
{
	mID.generate();
	mSequenceID = 0;

	if(mType == TEMPLATE)
	{
		mFlags = mData[0];

		LLMessageTemplate* temp = NULL;

		if (mEasyMessageReader)
			temp = mEasyMessageReader->decodeTemplateMessage(
							&(mData[0]), mDataSize, mFromHost, mSequenceID);

		if (temp)
			mNames.insert(temp->mName);
		else
			mNames.insert("Invalid");

		mRegionHosts.insert(isOutgoing() ? mToHost : mFromHost);
	}
	else if(mType == HTTP_REQUEST)// not template
	{
		std::string base_url = get_base_cap_url(mURL);

		if(LLWorld::getInstance()->isCapURLMapped(base_url))
		{
			CapUrlMatches matches = LLWorld::getInstance()->getCapURLMatches(base_url);
			mNames = matches.mCapNames;
			for(std::set<LLViewerRegion*>::iterator iter = matches.mRegions.begin(); iter != matches.mRegions.end(); ++iter)
			{
				mRegionHosts.insert((*iter)->getHost());
			}
		}
		else
			mNames.insert(mURL);
	}
	else // not template
	{
		mNames.insert("SOMETHING ELSE");
	}
}

LLEasyMessageLogEntry::~LLEasyMessageLogEntry()
{
	if(mResponseMsg)
		delete mResponseMsg;
}
BOOL LLEasyMessageLogEntry::isOutgoing()
{
#define LOCALHOST_ADDR 16777343
	return mFromHost == LLHost(LOCALHOST_ADDR, gMessageSystem->getListenPort());
#undef LOCALHOST_ADDR
}
std::string LLEasyMessageLogEntry::getName()
{
	std::string message_names;
	std::set<std::string>::iterator iter = mNames.begin();
	std::set<std::string>::const_iterator begin = mNames.begin();
	std::set<std::string>::const_iterator end = mNames.end();

	while(iter != end)
	{
		if(iter != begin)
			message_names += ", ";

		message_names += (*iter);
		++iter;
	}

	return message_names;
}

void LLEasyMessageLogEntry::setResponseMessage(LogPayload entry)
{
	// we already had a response set, somehow. just get rid of it
	if(mResponseMsg)
		delete mResponseMsg;

	mResponseMsg = new LLEasyMessageLogEntry(entry);
	delete entry;
}
std::string LLEasyMessageLogEntry::getFull(BOOL beautify, BOOL show_header)
{
	std::ostringstream full;
	if(mType == TEMPLATE)
	{
		LLMessageTemplate* temp = NULL;

		if(mEasyMessageReader)
			temp = mEasyMessageReader->decodeTemplateMessage(&(mData[0]), mDataSize, mFromHost);

		if(temp)
		{
			full << (isOutgoing() ? "out " : "in ");
			full << llformat("%s\n\n", temp->mName);
			if(show_header)
			{
				full << "[Header]\n";
				full << llformat("SequenceID = %u\n", mSequenceID);
				full << llformat("LL_ZERO_CODE_FLAG = %s\n", (mFlags & LL_ZERO_CODE_FLAG) ? "True" : "False");
				full << llformat("LL_RELIABLE_FLAG = %s\n", (mFlags & LL_RELIABLE_FLAG) ? "True" : "False");
				full << llformat("LL_RESENT_FLAG = %s\n", (mFlags & LL_RESENT_FLAG) ? "True" : "False");
				full << llformat("LL_ACK_FLAG = %s\n\n", (mFlags & LL_ACK_FLAG) ? "True" : "False");
			}
			LLMessageTemplate::message_block_map_t::iterator blocks_end = temp->mMemberBlocks.end();
			for (LLMessageTemplate::message_block_map_t::iterator blocks_iter = temp->mMemberBlocks.begin();
				 blocks_iter != blocks_end; ++blocks_iter)
			{
				LLMessageBlock* block = (*blocks_iter);
				const char* block_name = block->mName;
				S32 num_blocks = mEasyMessageReader->getNumberOfBlocks(block_name);
				for(S32 block_num = 0; block_num < num_blocks; block_num++)
				{
					full << llformat("[%s]\n", block->mName);
					LLMessageBlock::message_variable_map_t::iterator var_end = block->mMemberVariables.end();
					for (LLMessageBlock::message_variable_map_t::iterator var_iter = block->mMemberVariables.begin();
						 var_iter != var_end; ++var_iter)
					{
						LLMessageVariable* variable = (*var_iter);
						const char* var_name = variable->getName();
						BOOL returned_hex;
						std::string value = mEasyMessageReader->var2Str(block_name, block_num, variable, returned_hex);
						if(returned_hex)
							full << llformat("    %s =| ", var_name);
						else
							full << llformat("    %s = ", var_name);

						full << value << "\n";
					}
				}
			} // blocks_iter
		}
		else
		{
			full << (isOutgoing() ? "out" : "in") << "\n";
			for(S32 i = 0; i < mDataSize; i++)
				full << llformat("%02X ", mData[i]);
		}
	}
	else if(mType == HTTP_REQUEST || HTTP_RESPONSE)
	{
		if(mType == HTTP_REQUEST)
			full << llformat("%s %s\n", httpMethodAsVerb(mMethod).c_str(), mURL.c_str());
		if(mType == HTTP_RESPONSE)
			full << llformat("%d\n", mStatusCode);

		if (mHeaders.isMap())
		{
	        LLSD::map_const_iterator iter = mHeaders.beginMap();
	        LLSD::map_const_iterator end  = mHeaders.endMap();

	        for (; iter != end; ++iter)
	        {
	            full << iter->first << ": " << iter->second.asString() << "\n";
	        }
	    }
		full << "\n";

		if(mDataSize)
		{
			bool can_beautify = false;
			if(beautify)
			{
				std::string content_type;
				for(LLSD::map_iterator iter = mHeaders.beginMap(); iter != mHeaders.endMap(); ++iter)
				{
					if(boost::iequals(iter->first, "content-type"))
					{
						content_type = iter->second.asString();
						break;
					}
				}

				if(!content_type.empty())
				{
					if(content_type == "application/llsd+xml" || content_type == "application/xml")
					{
						// Use PugiXML instead of LLXMLNode since Expat can change the semantics of
						// input by dropping xml decls and expanding entities, as well as DoS the client.
						// LLSDSerialize can't be used either since it uses Expat internally.
						pugi::xml_document doc;
						U32 parse_opts = (pugi::parse_default | pugi::parse_comments | pugi::parse_doctype
										  | pugi::parse_declaration | pugi::parse_pi) & ~(pugi::parse_escapes);
						pugi::xml_parse_result res = doc.load_buffer(mData, mDataSize, parse_opts);
						if(res)
						{
							U32 format_opts = pugi::format_default | pugi::format_no_escapes | pugi::format_no_declaration;
							if(doc.child("llsd"))
								format_opts |= pugi::format_pretty_llsd;
							doc.save(full, "    ", format_opts);
							can_beautify = true;
						}
						else
						{
							LL_WARNS("EasyMessageReader") << "PugiXML failed with: " << res.description() << LL_ENDL;
						}
					}
				}
			}
			if(!can_beautify)
				full << mData;
		}
	}
	//unsupported message type
	else
	{
		full << "FIXME";
	}
	return full.str();
}

std::string LLEasyMessageLogEntry::getResponseFull(BOOL beautify, BOOL show_header)
{
	if(!mResponseMsg)
		return "";

	return mResponseMsg->getFull(beautify, show_header);
}

LLEasyMessageReader::LLEasyMessageReader()
    : mTemplateMessageReader(gMessageSystem->mMessageNumbers)
{
}

LLEasyMessageReader::~LLEasyMessageReader()
{
}

//we might want the sequenceid of the packet, which we can't get from
//a messagetemplate pointer, allow for passing in a U32 to be replaced
//with the sequenceid
LLMessageTemplate* LLEasyMessageReader::decodeTemplateMessage(U8 *data, S32 data_len, LLHost from_host)
{
	U32 fake_id = 0;
	return decodeTemplateMessage(data, data_len, from_host, fake_id);
}

LLMessageTemplate* LLEasyMessageReader::decodeTemplateMessage(U8 *data, S32 data_len, LLHost from_host, U32& sequence_id)
{
	if(data_len > NET_BUFFER_SIZE)
	{
		LL_ERRS("") << "Tried to decode a template message of size " << data_len << ", greater than NET_BUFFER_SIZE!" << LL_ENDL;
		return NULL;
	}
	U8 decode_buf[NET_BUFFER_SIZE];
	memcpy(&(decode_buf[0]), data, data_len);
	U8* decodep = &(decode_buf[0]);

	LLMessageTemplate* message_template = NULL;

	gMessageSystem->zeroCodeExpand(&decodep, &data_len);

	if(data_len >= LL_MINIMUM_VALID_PACKET_SIZE)
	{
		sequence_id = ntohl(*((U32*)(&decodep[1])));
		mTemplateMessageReader.clearMessage();
		if(mTemplateMessageReader.validateMessage(decodep, data_len, from_host, TRUE))
		{
			if(mTemplateMessageReader.decodeData(decodep, from_host, TRUE))
			{
				message_template = mTemplateMessageReader.getTemplate();
			}
		}
	}
	return message_template;
}

S32 LLEasyMessageReader::getNumberOfBlocks(const char *blockname)
{
	return mTemplateMessageReader.getNumberOfBlocks(blockname);
}

std::string LLEasyMessageReader::var2Str(const char* block_name, S32 block_num, LLMessageVariable* variable, BOOL &returned_hex, BOOL summary_mode)
{
	const char* var_name = variable->getName();
	e_message_variable_type var_type = variable->getType();

	returned_hex = FALSE;
	std::stringstream stream;

	char* value;
	U32 valueU32;
	U16 valueU16;
	LLVector3 valueVector3;
	LLVector3d valueVector3d;
	LLVector4 valueVector4;
	LLQuaternion valueQuaternion;
	LLUUID valueLLUUID;

	switch(var_type)
	{
	case MVT_U8:
		U8 valueU8;
		mTemplateMessageReader.getU8(block_name, var_name, valueU8, block_num);
		stream << U32(valueU8);
		break;
	case MVT_U16:
		mTemplateMessageReader.getU16(block_name, var_name, valueU16, block_num);
		stream << valueU16;
		break;
	case MVT_U32:
		mTemplateMessageReader.getU32(block_name, var_name, valueU32, block_num);
		stream << valueU32;
		break;
	case MVT_U64:
		U64 valueU64;
		mTemplateMessageReader.getU64(block_name, var_name, valueU64, block_num);
		stream << valueU64;
		break;
	case MVT_S8:
		S8 valueS8;
		mTemplateMessageReader.getS8(block_name, var_name, valueS8, block_num);
		stream << S32(valueS8);
		break;
	case MVT_S16:
		S16 valueS16;
		mTemplateMessageReader.getS16(block_name, var_name, valueS16, block_num);
		stream << valueS16;
		break;
	case MVT_S32:
		S32 valueS32;
		mTemplateMessageReader.getS32(block_name, var_name, valueS32, block_num);
		stream << valueS32;
		break;
	/*case MVT_S64:
		S64 valueS64;
		mTemplateMessageReader.getS64(block_name, var_name, valueS64, block_num);
		stream << valueS64;
		break;*/
	case MVT_F32:
		F32 valueF32;
		mTemplateMessageReader.getF32(block_name, var_name, valueF32, block_num);
		stream << valueF32;
		break;
	case MVT_F64:
		F64 valueF64;
		mTemplateMessageReader.getF64(block_name, var_name, valueF64, block_num);
		stream << valueF64;
		break;
	case MVT_LLVector3:
		mTemplateMessageReader.getVector3(block_name, var_name, valueVector3, block_num);
		//stream << valueVector3;
		stream << "<" << valueVector3.mV[0] << ", " << valueVector3.mV[1] << ", " << valueVector3.mV[2] << ">";
		break;
	case MVT_LLVector3d:
		mTemplateMessageReader.getVector3d(block_name, var_name, valueVector3d, block_num);
		//stream << valueVector3d;
		stream << "<" << valueVector3d.mdV[0] << ", " << valueVector3d.mdV[1] << ", " << valueVector3d.mdV[2] << ">";
		break;
	case MVT_LLVector4:
		mTemplateMessageReader.getVector4(block_name, var_name, valueVector4, block_num);
		//stream << valueVector4;
		stream << "<" << valueVector4.mV[0] << ", " << valueVector4.mV[1] << ", " << valueVector4.mV[2] << ", " << valueVector4.mV[3] << ">";
		break;
	case MVT_LLQuaternion:
		mTemplateMessageReader.getQuat(block_name, var_name, valueQuaternion, block_num);
		//stream << valueQuaternion;
		stream << "<" << valueQuaternion.mQ[0] << ", " << valueQuaternion.mQ[1] << ", " << valueQuaternion.mQ[2] << ", " << valueQuaternion.mQ[3] << ">";
		break;
	case MVT_LLUUID:
		mTemplateMessageReader.getUUID(block_name, var_name, valueLLUUID, block_num);
		stream << valueLLUUID;
		break;
	case MVT_BOOL:
		BOOL valueBOOL;
		mTemplateMessageReader.getBOOL(block_name, var_name, valueBOOL, block_num);
		stream << valueBOOL;
		break;
	case MVT_IP_ADDR:
		mTemplateMessageReader.getIPAddr(block_name, var_name, valueU32, block_num);
		stream << LLHost(valueU32, 0).getIPString();
		break;
	case MVT_IP_PORT:
		mTemplateMessageReader.getIPPort(block_name, var_name, valueU16, block_num);
		stream << valueU16;
	case MVT_VARIABLE:
	case MVT_FIXED:
	default:
		S32 size = mTemplateMessageReader.getSize(block_name, block_num, var_name);
		if(size)
		{
			value = new char[size + 1];
			mTemplateMessageReader.getBinaryData(block_name, var_name, value, size, block_num);
			value[size] = '\0';
			S32 readable = 0;
			S32 unreadable = 0;
			S32 end = (summary_mode && (size > 64)) ? 64 : size;
			for(S32 i = 0; i < end; i++)
			{
				if(!value[i])
				{
					if(i != (end - 1))
					{ // don't want null terminator hiding data
						unreadable = S32_MAX;
						break;
					}
				}
				else if(value[i] < 0x20 || value[i] >= 0x7F)
				{
					if(summary_mode)
						unreadable++;
					else
					{ // never want any wrong characters outside of summary mode
						unreadable = S32_MAX;
						break;
					}
				}
				else readable++;
			}
			if(readable >= unreadable)
			{
				if(summary_mode && (size > 64))
				{
					for(S32 i = 60; i < 63; i++)
						value[i] = '.';
					value[63] = '\0';
				}
				stream << value;
			}
			else
			{
				returned_hex = TRUE;
				S32 end = (summary_mode && (size > 8)) ? 8 : size;
				for(S32 i = 0; i < end; i++)
					//stream << std::uppercase << std::hex << U32(value[i]) << " ";
					stream << llformat("%02X ", (U8)value[i]);
				if(summary_mode && (size > 8))
					stream << " ... ";
			}

			delete[] value;
		}
		break;
	}

	return stream.str();
}
