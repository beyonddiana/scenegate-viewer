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

#include "llmutex.h"
#include "llmemory.h"
#include "lltimer.h"

#include <list>

#define TOKEN_PASTE_ACTUAL(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_ACTUAL(x, y)
#define RECORD_BLOCK_TIME(block_timer) LLTrace::BlockTimer::Recorder TOKEN_PASTE(block_time_recorder, __COUNTER__)(block_timer);


namespace LLTrace
{
	class Sampler;

	void init();
	void cleanup();

	class LL_COMMON_API MasterThreadTrace& getMasterThreadTrace();

	// one per thread per type
	template<typename ACCUMULATOR>
	class LL_COMMON_API AccumulatorBuffer
	{
		static const U32 DEFAULT_ACCUMULATOR_BUFFER_SIZE = 64;
	private:
		enum StaticAllocationMarker { STATIC_ALLOC };

		AccumulatorBuffer(StaticAllocationMarker m)
		:	mStorageSize(64),
			mNextStorageSlot(0),
			mStorage(new ACCUMULATOR[DEFAULT_ACCUMULATOR_BUFFER_SIZE])
		{}

	public:

		// copying an accumulator buffer does not copy the actual contents, but simply initializes the buffer size
		// to be identical to the other buffer
		AccumulatorBuffer(const AccumulatorBuffer& other = getDefaultBuffer())
		:	mStorageSize(other.mStorageSize),
			mStorage(new ACCUMULATOR[other.mStorageSize]),
			mNextStorageSlot(other.mNextStorageSlot)
		{}

		~AccumulatorBuffer()
		{
			if (sPrimaryStorage == mStorage)
			{
				//TODO pick another primary?
				sPrimaryStorage = NULL;
			}
		}

		LL_FORCE_INLINE ACCUMULATOR& operator[](size_t index) 
		{ 
			return mStorage[index]; 
		}

		void mergeFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			llassert(mNextStorageSlot == other.mNextStorageSlot);

			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].mergeFrom(other.mStorage[i]);
			}
		}

		void copyFrom(const AccumulatorBuffer<ACCUMULATOR>& other)
		{
			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i] = other.mStorage[i];
			}
		}

		void reset()
		{
			for (size_t i = 0; i < mNextStorageSlot; i++)
			{
				mStorage[i].reset();
			}
		}

		void makePrimary()
		{
			sPrimaryStorage = mStorage;
		}

		LL_FORCE_INLINE static ACCUMULATOR* getPrimaryStorage() 
		{ 
			return sPrimaryStorage.get(); 
		}

		// NOTE: this is not thread-safe.  We assume that slots are reserved in the main thread before any child threads are spawned
		size_t reserveSlot()
		{
			size_t next_slot = mNextStorageSlot++;
			if (next_slot >= mStorageSize)
			{
				size_t new_size = mStorageSize + (mStorageSize >> 2);
				delete [] mStorage;
				mStorage = new ACCUMULATOR[new_size];
				mStorageSize = new_size;
			}
			llassert(next_slot < mStorageSize);
			return next_slot;
		}

		static AccumulatorBuffer<ACCUMULATOR>& getDefaultBuffer()
		{
			static AccumulatorBuffer sBuffer(STATIC_ALLOC);
			return sBuffer;
		}

	private:
		ACCUMULATOR*							mStorage;
		size_t									mStorageSize;
		size_t									mNextStorageSlot;
		static LLThreadLocalPtr<ACCUMULATOR>	sPrimaryStorage;
	};
	template<typename ACCUMULATOR> LLThreadLocalPtr<ACCUMULATOR> AccumulatorBuffer<ACCUMULATOR>::sPrimaryStorage;

	template<typename ACCUMULATOR>
	class LL_COMMON_API TraceType
	{
	public:
		TraceType(const std::string& name)
		:	mName(name)
		{
			mAccumulatorIndex = AccumulatorBuffer<ACCUMULATOR>::getDefaultBuffer().reserveSlot();
		}

		LL_FORCE_INLINE ACCUMULATOR& getPrimaryAccumulator()
		{
			return AccumulatorBuffer<ACCUMULATOR>::getPrimaryStorage()[mAccumulatorIndex];
		}

		ACCUMULATOR& getAccumulator(AccumulatorBuffer<ACCUMULATOR>& buffer) { return buffer[mAccumulatorIndex]; }

	protected:
		std::string	mName;
		size_t		mAccumulatorIndex;
	};


	template<typename T>
	class LL_COMMON_API StatAccumulator
	{
	public:
		StatAccumulator()
		:	mSum(0),
			mMin(0),
			mMax(0),
			mNumSamples(0)
		{}

		LL_FORCE_INLINE void sample(T value)
		{
			mNumSamples++;
			mSum += value;
			if (value < mMin)
			{
				mMin = value;
			}
			else if (value > mMax)
			{
				mMax = value;
			}
		}

		void mergeFrom(const StatAccumulator<T>& other)
		{
			mSum += other.mSum;
			if (other.mMin < mMin)
			{
				mMin = other.mMin;
			}
			if (other.mMax > mMax)
			{
				mMax = other.mMax;
			}
			mNumSamples += other.mNumSamples;
		}

		void reset()
		{
			mNumSamples = 0;
			mSum = 0;
			mMin = 0;
			mMax = 0;
		}

		T	getSum() { return mSum; }
		T	getMin() { return mMin; }
		T	getMax() { return mMax; }
		T	getMean() { return mSum / (T)mNumSamples; }

	private:
		T	mSum,
			mMin,
			mMax;

		U32	mNumSamples;
	};

	template <typename T>
	class LL_COMMON_API Stat 
	:	public TraceType<StatAccumulator<T> >, 
		public LLInstanceTracker<Stat<T>, std::string>
	{
	public:
		Stat(const std::string& name) 
		:	TraceType(name),
			LLInstanceTracker(name)
		{}

		void sample(T value)
		{
			getPrimaryAccumulator().sample(value);
		}
	};

	struct LL_COMMON_API TimerAccumulator
	{
		U32 							mTotalTimeCounter,
										mChildTimeCounter,
										mCalls;
		TimerAccumulator*				mParent;		// info for caller timer
		TimerAccumulator*				mLastCaller;	// used to bootstrap tree construction
		const class BlockTimer*			mTimer;			// points to block timer associated with this storage
		U8								mActiveCount;	// number of timers with this ID active on stack
		bool							mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		std::vector<TimerAccumulator*>	mChildren;		// currently assumed child timers

		void mergeFrom(const TimerAccumulator& other)
		{
			mTotalTimeCounter += other.mTotalTimeCounter;
			mChildTimeCounter += other.mChildTimeCounter;
			mCalls += other.mCalls;
		}

		void reset()
		{
			mTotalTimeCounter = 0;
			mChildTimeCounter = 0;
			mCalls = 0;
		}

	};

	class LL_COMMON_API BlockTimer : public TraceType<TimerAccumulator>
	{
	public:
		BlockTimer(const char* name)
		:	TraceType(name)
		{}

		struct Recorder
		{
			struct StackEntry
			{
				Recorder*			mRecorder;
				TimerAccumulator*	mAccumulator;
				U32					mChildTime;
			};

			LL_FORCE_INLINE Recorder(BlockTimer& block_timer)
			:	mLastRecorder(sCurRecorder)
			{
				mStartTime = getCPUClockCount32();
				TimerAccumulator* accumulator = &block_timer.getPrimaryAccumulator(); // get per-thread accumulator
				accumulator->mActiveCount++;
				accumulator->mCalls++;
				accumulator->mMoveUpTree |= (accumulator->mParent->mActiveCount == 0);

				// push new timer on stack
				sCurRecorder.mRecorder = this;
				sCurRecorder.mAccumulator = accumulator;
				sCurRecorder.mChildTime = 0;
			}

			LL_FORCE_INLINE ~Recorder()
			{
				U32 total_time = getCPUClockCount32() - mStartTime;

				TimerAccumulator* accumulator = sCurRecorder.mAccumulator;
				accumulator->mTotalTimeCounter += total_time;
				accumulator->mChildTimeCounter += sCurRecorder.mChildTime;
				accumulator->mActiveCount--;

				accumulator->mLastCaller = mLastRecorder.mAccumulator;
				mLastRecorder.mChildTime += total_time;

				// pop stack
				sCurRecorder = mLastRecorder;
			}

			StackEntry mLastRecorder;
			U32 mStartTime;
		};

	private:
		static U32 getCPUClockCount32()
		{
			U32 ret_val;
			__asm
			{
				_emit   0x0f
				_emit   0x31
				shr eax,8
				shl edx,24
				or eax, edx
				mov dword ptr [ret_val], eax
			}
			return ret_val;
		}

		// return full timer value, *not* shifted by 8 bits
		static U64 getCPUClockCount64()
		{
			U64 ret_val;
			__asm
			{
				_emit   0x0f
				_emit   0x31
				mov eax,eax
				mov edx,edx
				mov dword ptr [ret_val+4], edx
				mov dword ptr [ret_val], eax
			}
			return ret_val;
		}

		static Recorder::StackEntry sCurRecorder;
	};

	class LL_COMMON_API ThreadTrace
	{
	public:
		ThreadTrace();
		ThreadTrace(const ThreadTrace& other);

		virtual ~ThreadTrace();

		void activate(Sampler* sampler);
		void deactivate(Sampler* sampler);
		void flushPrimary();

		Sampler* createSampler();

		virtual void pushToMaster() = 0;

		Sampler* getPrimarySampler();
	protected:
		Sampler*				mPrimarySampler;
		std::list<Sampler*>		mActiveSamplers;
	};

	class LL_COMMON_API MasterThreadTrace : public ThreadTrace
	{
	public:
		MasterThreadTrace();

		void addSlaveThread(class SlaveThreadTrace* child);
		void removeSlaveThread(class SlaveThreadTrace* child);

		/*virtual */ void pushToMaster();

		// call this periodically to gather stats data from slave threads
		void pullFromSlaveThreads();

	private:
		struct SlaveThreadTraceProxy
		{
			SlaveThreadTraceProxy(class SlaveThreadTrace* trace, Sampler* storage);

			~SlaveThreadTraceProxy();
			class SlaveThreadTrace*	mSlaveTrace;
			Sampler*				mSamplerStorage;
		private:
			//no need to copy these and then have to duplicate the storage
			SlaveThreadTraceProxy(const SlaveThreadTraceProxy& other) {}
		};
		typedef std::list<SlaveThreadTraceProxy*> slave_thread_trace_list_t;

		slave_thread_trace_list_t		mSlaveThreadTraces;
		LLMutex							mSlaveListMutex;
	};

	class LL_COMMON_API SlaveThreadTrace : public ThreadTrace
	{
	public:
		SlaveThreadTrace();
		~SlaveThreadTrace();

		// call this periodically to gather stats data for master thread to consume
		/*virtual*/ void pushToMaster();

		MasterThreadTrace* 	mMaster;

		// this data is accessed by other threads, so give it a 64 byte alignment
		// to avoid false sharing on most x86 processors
		LL_ALIGNED(64) class SharedData
		{
		public:
			explicit 
			SharedData(Sampler* sampler);

			~SharedData();

			void copyFrom(Sampler* source);
			void copyTo(Sampler* sink);
		private:
			// add a cache line's worth of unused space to avoid any potential of false sharing
			LLMutex					mSamplerMutex;
			Sampler*				mSampler;
		};
		SharedData					mSharedData;
	};
}

#endif // LL_LLTRACE_H
