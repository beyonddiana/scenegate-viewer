/** 
 * @file lscript_readlso.h
 * @brief classes to read lso file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LSCRIPT_READLSO_H
#define LL_LSCRIPT_READLSO_H

#include "lscript_byteconvert.h"

// list of op code print functions
void print_noop(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pop(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pops(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_poparg(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popip(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popbp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popsp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popslr(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_dup(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dups(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_store(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stores(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storel(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storev(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storeq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storeg(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegs(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadsp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadlp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadvp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadqp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgsp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadglp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgvp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgqp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_push(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushs(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushg(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgs(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_puship(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushbp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushsp(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargb(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargi(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargf(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargs(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargv(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushe(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushev(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pusheq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pusharge(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_add(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_sub(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_mul(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_div(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_mod(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_eq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_neq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_leq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_geq(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_less(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_greater(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_bitand(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitor(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitxor(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_booland(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_boolor(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_shl(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_shr(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_neg(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitnot(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_boolnot(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_jump(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_jumpif(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_jumpnif(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_state(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_call(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_return(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_cast(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stacktos(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stacktol(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_print(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_calllib(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_calllib_two_byte(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);

class LLScriptLSOParse
{
public:
	LLScriptLSOParse(LLFILE *fp);
	LLScriptLSOParse(U8 *buffer);
	~LLScriptLSOParse();

	void initOpCodePrinting();

	void printData(LLFILE *fp);
	void printNameDesc(LLFILE *fp);
	void printRegisters(LLFILE *fp);
	void printGlobals(LLFILE *fp);
	void printGlobalFunctions(LLFILE *fp);
	void printStates(LLFILE *fp);
	void printHeap(LLFILE *fp);
	void printOpCodes(LLFILE *fp, S32 &offset, S32 tabs);
	void printOpCodeRange(LLFILE *fp, S32 start, S32 end, S32 tabs);

	U8	*mRawData;
	void (*mPrintOpCodes[0x100])(LLFILE *fp, U8 *buffer, S32 &offset, S32 tabs);
};


void lso_print_tabs(LLFILE *fp, S32 tabs);

#endif
