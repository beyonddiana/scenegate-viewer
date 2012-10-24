/** 
 * @file llpredicate.h
 * @brief abstraction for filtering objects by predicates, with arbitrary boolean expressions
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLPREDICATE_H
#define LL_LLPREDICATE_H

#include "llerror.h"

namespace LLPredicate
{
	template<typename ENUM> class Rule;

	template<typename ENUM>
	struct Value
	{
		friend Rule<ENUM>;
	public:
		Value(ENUM e)
		:	mPredicateFlags(0x1),
			mPredicateCombinationFlags(0x1)
		{
			add(e);
		}

		Value()
		:	mPredicateFlags(0x1),
			mPredicateCombinationFlags(0x1)
		{}

		void add(ENUM predicate)
		{
			llassert(predicate < 5);
			if (!has(predicate))
			{
				int predicate_flag = 0x1 << (0x1 << (int)predicate);
				mPredicateCombinationFlags *= predicate_flag;
				mPredicateFlags |= predicate_flag;
			}
		}

		void remove(ENUM predicate)
		{
			llassert(predicate < 5);
			int predicate_flag = 0x1 << (0x1 << (int)predicate);
			if (mPredicateFlags & predicate_flag)
			{
				mPredicateCombinationFlags /= predicate_flag;
				mPredicateFlags &= ~predicate_flag;
			}
		}

		void unknown(ENUM predicate)
		{
			add(predicate);
			int predicate_shift = 0x1 << (int)predicate;
			mPredicateCombinationFlags |= mPredicateCombinationFlags << predicate_shift;
		}

		bool has(ENUM predicate)
		{
			int predicate_flag = 0x1 << (0x1 << (int)predicate);
			return (mPredicateFlags & predicate_flag) != 0;
		}

	private:
		int mPredicateCombinationFlags;
		int mPredicateFlags;
	};

	struct EmptyRule {};

	template<typename ENUM>
	class Rule
	{
	public:
		Rule(ENUM value)
		:	mPredicateRequirements(predicateFromValue(value))
		{}

		Rule()
		:	mPredicateRequirements(0x1)
		{}

		Rule operator~()
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = ~mPredicateRequirements;
			return new_rule;
		}

		Rule operator &&(const Rule& other)
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = mPredicateRequirements & other.mPredicateRequirements;
			return new_rule;
		}

		Rule operator ||(const Rule& other)
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = mPredicateRequirements | other.mPredicateRequirements;
			return new_rule;
		}

		bool check(const Value<ENUM>& value) const
		{
			return ((value.mPredicateCombinationFlags | 0x1) & mPredicateRequirements) != 0;
		}

		static int predicateFromValue(ENUM value)
		{
			llassert(value < 5);
			static const int predicates[5] = 
			{
				0xAAAAaaaa, //  10101010101010101010101010101010
				0xCCCCcccc, //  11001100110011001100110011001100
				0xF0F0F0F0, //  11110000111100001111000011110000
				0xFF00FF00, //  11111111000000001111111100000000
				0xFFFF0000  //  11111111111111110000000000000000 
			};
			return predicates[value];
		}

		bool isTriviallyTrue() const
		{
			return mPredicateRequirements & 0x1;
		}

		bool isTriviallyFalse() const
		{
			return mPredicateRequirements == 0;
		}

	private:
		int mPredicateRequirements;
	};

	template<typename ENUM>
	Rule<ENUM> make_rule(ENUM e) { return Rule<ENUM>(e);}

	// return generic empty rule class to avoid requiring template argument to create an empty rule
	EmptyRule make_rule();

}
#endif // LL_LLPREDICATE_H
