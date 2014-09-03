/*
 * @file llaescipher.h
 * @brief AES wrapper
 *
 * Copyright (c) 2014, Cinder Roxley <cinder@sdf.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef LL_AESCIPHER_H
#define LL_AESCIPHER_H

#include "llcipher.h"

class LLAESCipher : public LLCipher
{
public:
	LLAESCipher(const U8* secret, size_t secret_size, const U8* iv, size_t iv_size);
	virtual ~LLAESCipher();
	
	/*virtual*/ U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	/*virtual*/ U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len);
	/*virtual*/ U32 requiredEncryptionSpace(U32 src_len) const;
	
private:
	U8* mSecret;
	size_t mSecretSize;
	U8* mInitialVector;
	size_t mInitialVectorSize;
};

#endif // LL_AESCIPHER_H
