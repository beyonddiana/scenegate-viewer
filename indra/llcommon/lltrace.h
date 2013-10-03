/** 
 * @file lltrace.h
 * @brief Runtime statistics accumulation.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLTRACE_H
#define LL_LLTRACE_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include "llmemory.h"
#include "llrefcount.h"
#include "lltraceaccumulators.h"
#include "llthreadlocalstorage.h"
#include "lltimer.h"
#include "llpointer.h"
#include "llunits.h"

#include <list>

namespace LLTrace
{
class Recording;

template<typename T>
T storage_value(T val) { return val; }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE storage_value(LLUnit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE storage_value(LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

class TraceBase
{
public:
	TraceBase(const char* name, const char* description);
	virtual ~TraceBase() {};
	virtual const char* getUnitLabel() const;

	const std::string& getName() const { return mName; }
	const std::string& getDescription() const { return mDescription; }

protected:
	std::string	mName;
	std::string	mDescription;
};

template<typename ACCUMULATOR>
class TraceType 
:	public TraceBase,
	public LLInstanceTracker<TraceType<ACCUMULATOR>, std::string>
{
public:
	TraceType(const char* name, const char* description = NULL)
	:	LLInstanceTracker<TraceType<ACCUMULATOR>, std::string>(name),
		TraceBase(name, description),
		mAccumulatorIndex(AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer()->reserveSlot())
	{}

	LL_FORCE_INLINE ACCUMULATOR& getCurrentAccumulator() const
	{
		ACCUMULATOR* accumulator_storage = LLThreadLocalSingletonPointer<ACCUMULATOR>::getInstance();
		return accumulator_storage ? accumulator_storage[mAccumulatorIndex] : (*AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer())[mAccumulatorIndex];
	}

	size_t getIndex() const { return mAccumulatorIndex; }
	static size_t getNumIndices() { return AccumulatorBuffer<ACCUMULATOR>::getNumIndices(); }

protected:
	const size_t		mAccumulatorIndex;
};


template<>
class TraceType<TimeBlockAccumulator::CallCountFacet>
:	public TraceType<TimeBlockAccumulator>
{
public:

	TraceType(const char* name, const char* description = "")
	:	TraceType<TimeBlockAccumulator>(name, description)
	{}
};

template<>
class TraceType<TimeBlockAccumulator::SelfTimeFacet>
	:	public TraceType<TimeBlockAccumulator>
{
public:

	TraceType(const char* name, const char* description = "")
		:	TraceType<TimeBlockAccumulator>(name, description)
	{}
};

template <typename T = F64>
class EventStatHandle
:	public TraceType<EventAccumulator>
{
public:
	typedef F64 storage_t;
	typedef TraceType<EventAccumulator> trace_t;
	typedef EventStatHandle<T> self_t;

	EventStatHandle(const char* name, const char* description = NULL)
	:	trace_t(name, description)
	{}

	/*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }

};

template<typename T, typename VALUE_T>
void record(EventStatHandle<T>& measurement, VALUE_T value)
{
	T converted_value(value);
	measurement.getCurrentAccumulator().record(storage_value(converted_value));
}

template <typename T = F64>
class SampleStatHandle
:	public TraceType<SampleAccumulator>
{
public:
	typedef F64 storage_t;
	typedef TraceType<SampleAccumulator> trace_t;
	typedef SampleStatHandle<T> self_t;

	SampleStatHandle(const char* name, const char* description = NULL)
	:	trace_t(name, description)
	{}

	/*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }
};

template<typename T, typename VALUE_T>
void sample(SampleStatHandle<T>& measurement, VALUE_T value)
{
	T converted_value(value);
	measurement.getCurrentAccumulator().sample(storage_value(converted_value));
}

template <typename T = F64>
class CountStatHandle
:	public TraceType<CountAccumulator>
{
public:
	typedef F64 storage_t;
	typedef TraceType<CountAccumulator> trace_t;
	typedef CountStatHandle<T> self_t;

	CountStatHandle(const char* name, const char* description = NULL) 
	:	trace_t(name, description)
	{}

	/*virtual*/ const char* getUnitLabel() const { return LLGetUnitLabel<T>::getUnitLabel(); }
};

template<typename T, typename VALUE_T>
void add(CountStatHandle<T>& count, VALUE_T value)
{
	T converted_value(value);
	count.getCurrentAccumulator().add(storage_value(converted_value));
}

template<>
class TraceType<MemStatAccumulator::AllocationFacet>
:	public TraceType<MemStatAccumulator>
{
public:

	TraceType(const char* name, const char* description = "")
	:	TraceType<MemStatAccumulator>(name, description)
	{}
};

template<>
class TraceType<MemStatAccumulator::DeallocationFacet>
:	public TraceType<MemStatAccumulator>
{
public:

	TraceType(const char* name, const char* description = "")
	:	TraceType<MemStatAccumulator>(name, description)
	{}
};

class MemStatHandle : public TraceType<MemStatAccumulator>
{
public:
	typedef TraceType<MemStatAccumulator> trace_t;
	MemStatHandle(const char* name)
	:	trace_t(name)
	{}

	void setName(const char* name)
	{
		mName = name;
		setKey(name);
	}

	/*virtual*/ const char* getUnitLabel() const { return "KB"; }

	TraceType<MemStatAccumulator::AllocationFacet>& allocations() 
	{ 
		return static_cast<TraceType<MemStatAccumulator::AllocationFacet>&>(*(TraceType<MemStatAccumulator>*)this);
	}

	TraceType<MemStatAccumulator::DeallocationFacet>& deallocations() 
	{ 
		return static_cast<TraceType<MemStatAccumulator::DeallocationFacet>&>(*(TraceType<MemStatAccumulator>*)this);
	}
};


// measures effective memory footprint of specified type
// specialize to cover different types
template<typename T, typename IS_MEM_TRACKABLE = void, typename IS_UNITS = void>
struct MeasureMem
{
	static size_t measureFootprint(const T& value)
	{
		return sizeof(T);
	}
};

template<typename T, typename IS_BYTES>
struct MeasureMem<T, typename T::mem_trackable_tag_t, IS_BYTES>
{
	static size_t measureFootprint(const T& value)
	{
		return sizeof(T) + value.getMemFootprint();
	}
};

template<typename T, typename IS_MEM_TRACKABLE>
struct MeasureMem<T, IS_MEM_TRACKABLE, typename T::is_unit_t>
{
	static size_t measureFootprint(const T& value)
	{
		return U32Bytes(value).value();
	}
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<T*, IS_MEM_TRACKABLE, IS_BYTES>
{
	static size_t measureFootprint(const T* value)
	{
		if (!value)
		{
			return 0;
		}
		return MeasureMem<T>::measureFootprint(*value);
	}
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<LLPointer<T>, IS_MEM_TRACKABLE, IS_BYTES>
{
	static size_t measureFootprint(const LLPointer<T> value)
	{
		if (value.isNull())
		{
			return 0;
		}
		return MeasureMem<T>::measureFootprint(*value);
	}
};

template<typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<S32, IS_MEM_TRACKABLE, IS_BYTES>
{
	static size_t measureFootprint(S32 value)
	{
		return value;
	}
};

template<typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<U32, IS_MEM_TRACKABLE, IS_BYTES>
{
	static size_t measureFootprint(U32 value)
	{
		return value;
	}
};

template<typename T, typename IS_MEM_TRACKABLE, typename IS_BYTES>
struct MeasureMem<std::basic_string<T>, IS_MEM_TRACKABLE, IS_BYTES>
{
	static size_t measureFootprint(const std::basic_string<T>& value)
	{
		return value.capacity() * sizeof(T);
	}
};


template<typename T>
inline void claim_alloc(MemStatHandle& measurement, const T& value)
{
	S32 size = MeasureMem<T>::measureFootprint(value);
	if(size == 0) return;
	MemStatAccumulator& accumulator = measurement.getCurrentAccumulator();
	accumulator.mSize.sample(accumulator.mSize.hasValue() ? accumulator.mSize.getLastValue() + (F64)size : (F64)size);
	accumulator.mFootprintAllocations.record(size);
}

template<typename T>
inline void disclaim_alloc(MemStatHandle& measurement, const T& value)
{
	S32 size = MeasureMem<T>::measureFootprint(value);
	if(size == 0) return;
	MemStatAccumulator& accumulator = measurement.getCurrentAccumulator();
	accumulator.mSize.sample(accumulator.mSize.hasValue() ? accumulator.mSize.getLastValue() - (F64)size : -(F64)size);
	accumulator.mFootprintDeallocations.add(size);
}

template<typename DERIVED, size_t ALIGNMENT = LL_DEFAULT_HEAP_ALIGN>
class MemTrackable
{
public:
	typedef void mem_trackable_tag_t;

	MemTrackable(const char* name)
	:	mMemFootprint(0)
	{
		static bool name_initialized = false;
		if (!name_initialized)
		{
			name_initialized = true;
			sMemStat.setName(name);
		}
	}

	virtual ~MemTrackable()
	{
		disclaimMem(mMemFootprint);
	}

	static MemStatHandle& getMemStatHandle()
	{
		return sMemStat;
	}

	S32 getMemFootprint() const	{ return mMemFootprint; }

	void* operator new(size_t size) 
	{
		claim_alloc(sMemStat, size);
		return ll_aligned_malloc(ALIGNMENT, size);
	}

	void operator delete(void* ptr, size_t size)
	{
		disclaim_alloc(sMemStat, size);
		ll_aligned_free(ALIGNMENT, ptr);
	}

	void* operator new [](size_t size)
	{
		claim_alloc(sMemStat, size);
		return ll_aligned_malloc(ALIGNMENT, size);
	}

	void operator delete[](void* ptr, size_t size)
	{
		disclaim_alloc(sMemStat, size);
		ll_aligned_free(ALIGNMENT, ptr);
	}

	// claim memory associated with other objects/data as our own, adding to our calculated footprint
	template<typename CLAIM_T>
	void claimMem(const CLAIM_T& value) const
	{
		S32 size = MeasureMem<CLAIM_T>::measureFootprint(value);
		claim_alloc(sMemStat, size);
		mMemFootprint += size;
	}

	// remove memory we had claimed from our calculated footprint
	template<typename CLAIM_T>
	void disclaimMem(const CLAIM_T& value) const
	{
		S32 size = MeasureMem<CLAIM_T>::measureFootprint(value);
		disclaim_alloc(sMemStat, size);
		mMemFootprint -= size;
	}

private:
	// use signed values so that we can temporarily go negative
	// and reconcile in destructor
	// NB: this assumes that no single class is responsible for > 2GB of allocations
	mutable S32 mMemFootprint;
	
	static	MemStatHandle	sMemStat;
};

template<typename DERIVED, size_t ALIGNMENT>
MemStatHandle MemTrackable<DERIVED, ALIGNMENT>::sMemStat("");

}
#endif // LL_LLTRACE_H
