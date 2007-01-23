/** 
 * @file llpumpio.cpp
 * @author Phoenix
 * @date 2004-11-21
 * @brief Implementation of the i/o pump and related functions.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llpumpio.h"

#include <set>
#include "apr-1/apr_poll.h"

#include "llapr.h"
#include "llmemtype.h"
#include "llstl.h"

// This should not be in production, but it is intensely useful during
// development.
#if LL_LINUX
#define LL_DEBUG_PIPE_TYPE_IN_PUMP 0
#endif

#if LL_DEBUG_PIPE_TYPE_IN_PUMP
#include <typeinfo>
#endif

// constants for poll timeout. if we are threading, we want to have a
// longer poll timeout.
#if LL_THREADS_APR
static const S32 DEFAULT_POLL_TIMEOUT = 1000;
#else
static const S32 DEFAULT_POLL_TIMEOUT = 0;
#endif

// The default (and fallback) expiration time for chains
const F32 DEFAULT_CHAIN_EXPIRY_SECS = 30.0f;
extern const F32 SHORT_CHAIN_EXPIRY_SECS = 1.0f;
extern const F32 NEVER_CHAIN_EXPIRY_SECS = 0.0f;

// sorta spammy debug modes.
//#define LL_DEBUG_SPEW_BUFFER_CHANNEL_IN_ON_ERROR 1
//#define LL_DEBUG_PROCESS_LINK 1
//#define LL_DEBUG_PROCESS_RETURN_VALUE 1

// Super spammy debug mode.
//#define LL_DEBUG_SPEW_BUFFER_CHANNEL_IN 1
//#define LL_DEBUG_SPEW_BUFFER_CHANNEL_OUT 1

/**
 * @class
 */
class LLChainSleeper : public LLRunnable
{
public:
	static LLRunner::run_ptr_t build(LLPumpIO* pump, S32 key)
	{
		return LLRunner::run_ptr_t(new LLChainSleeper(pump, key));
	}
	
	virtual void run(LLRunner* runner, S64 handle)
	{
		mPump->clearLock(mKey);
	}

protected:
	LLChainSleeper(LLPumpIO* pump, S32 key) : mPump(pump), mKey(key) {}
	LLPumpIO* mPump;
	S32 mKey;
};


/**
 * @struct ll_delete_apr_pollset_fd_client_data
 * @brief This is a simple helper class to clean up our client data.
 */
struct ll_delete_apr_pollset_fd_client_data
{
	typedef std::pair<LLIOPipe::ptr_t, apr_pollfd_t> pipe_conditional_t;
	void operator()(const pipe_conditional_t& conditional)
	{
		LLMemType m1(LLMemType::MTYPE_IO_PUMP);
		S32* client_id = (S32*)conditional.second.client_data;
		delete client_id;
	}
};

/**
 * LLPumpIO
 */
LLPumpIO::LLPumpIO(apr_pool_t* pool) :
	mState(LLPumpIO::NORMAL),
	mRebuildPollset(false),
	mPollset(NULL),
	mPollsetClientID(0),
	mNextLock(0),
	mPool(NULL),
	mCurrentPool(NULL),
	mCurrentPoolReallocCount(0),
	mChainsMutex(NULL),
	mCallbackMutex(NULL)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	initialize(pool);
}

LLPumpIO::~LLPumpIO()
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	cleanup();
}

bool LLPumpIO::prime(apr_pool_t* pool)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	cleanup();
	initialize(pool);
	return ((pool == NULL) ? false : true);
}

bool LLPumpIO::addChain(const chain_t& chain, F32 timeout)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	if(chain.empty()) return false;

#if LL_THREADS_APR
	LLScopedLock lock(mChainsMutex);
#endif
	LLChainInfo info;
	info.setTimeoutSeconds(timeout);
	info.mData = LLIOPipe::buffer_ptr_t(new LLBufferArray);
	LLLinkInfo link;
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
	lldebugs << "LLPumpIO::addChain() " << chain[0] << " '"
		<< typeid(*(chain[0])).name() << "'" << llendl;
#else
	lldebugs << "LLPumpIO::addChain() " << chain[0] <<llendl;
#endif
	chain_t::const_iterator it = chain.begin();
	chain_t::const_iterator end = chain.end();
	for(; it != end; ++it)
	{
		link.mPipe = (*it);
		link.mChannels = info.mData->nextChannel();
		info.mChainLinks.push_back(link);
	}
	mPendingChains.push_back(info);
	return true;
}

bool LLPumpIO::addChain(
	const LLPumpIO::links_t& links,
	LLIOPipe::buffer_ptr_t data,
	LLSD context,
	F32 timeout)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);

	// remember that if the caller is providing a full link
	// description, we need to have that description matched to a
	// particular buffer.
	if(!data) return false;
	if(links.empty()) return false;

#if LL_THREADS_APR
	LLScopedLock lock(mChainsMutex);
#endif
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
	lldebugs << "LLPumpIO::addChain() " << links[0].mPipe << " '"
		<< typeid(*(links[0].mPipe)).name() << "'" << llendl;
#else
	lldebugs << "LLPumpIO::addChain() " << links[0].mPipe << llendl;
#endif
	LLChainInfo info;
	info.setTimeoutSeconds(timeout);
	info.mChainLinks = links;
	info.mData = data;
	info.mContext = context;
	mPendingChains.push_back(info);
	return true;
}

bool LLPumpIO::setTimeoutSeconds(F32 timeout)
{
	// If no chain is running, return failure.
	if(current_chain_t() == mCurrentChain)
	{
		return false;
	}
	(*mCurrentChain).setTimeoutSeconds(timeout);
	return true;
}

bool LLPumpIO::setConditional(LLIOPipe* pipe, const apr_pollfd_t* poll)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	//lldebugs << "LLPumpIO::setConditional" << llendl;
	if(pipe)
	{
		// remove any matching poll file descriptors for this pipe.
		LLIOPipe::ptr_t pipe_ptr(pipe);

		LLChainInfo::conditionals_t::iterator it = (*mCurrentChain).mDescriptors.begin();
		LLChainInfo::conditionals_t::iterator end = (*mCurrentChain).mDescriptors.end();
		while (it != end)
		{
			LLChainInfo::pipe_conditional_t& value = (*it);
			if ( pipe_ptr == value.first )
			{
				ll_delete_apr_pollset_fd_client_data()(value);
				(*mCurrentChain).mDescriptors.erase(it++);
				mRebuildPollset = true;
			}
			else
			{
				++it;
			}
		}

		if(poll)
		{
			LLChainInfo::pipe_conditional_t value;
			value.first = pipe_ptr;
			value.second = *poll;
			if(!poll->p)
			{
				// each fd needs a pool to work with, so if one was
				// not specified, use this pool.
				// *FIX: Should it always be this pool?
				value.second.p = mPool;
			}
			value.second.client_data = new S32(++mPollsetClientID);
			(*mCurrentChain).mDescriptors.push_back(value);
			mRebuildPollset = true;
		}
		return true;
	}
	return false;
}

S32 LLPumpIO::setLock()
{
	// *NOTE: I do not think it is necessary to acquire a mutex here
	// since this should only be called during the pump(), and should
	// only change the running chain. Any other use of this method is
	// incorrect usage. If it becomes necessary to acquire a lock
	// here, be sure to lock here and call a protected method to get
	// the lock, and sleepChain() should probably acquire the same
	// lock while and calling the same protected implementation to
	// lock the runner at the same time.

	// If no chain is running, return failure.
	if(current_chain_t() == mCurrentChain)
	{
		return 0;
	}

	// deal with wrap.
	if(++mNextLock <= 0)
	{
		mNextLock = 1;
	}

	// set the lock
	(*mCurrentChain).mLock = mNextLock;
	return mNextLock;
}

void LLPumpIO::clearLock(S32 key)
{
	// We need to lock it here since we do not want to be iterating
	// over the chains twice. We can safely call process() while this
	// is happening since we should not be erasing a locked pipe, and
	// therefore won't be treading into deleted memory. I think we can
	// also clear the lock on the chain safely since the pump only
	// reads that value.
#if LL_THREADS_APR
	LLScopedLock lock(mChainsMutex);
#endif
	mClearLocks.insert(key);
}

bool LLPumpIO::sleepChain(F64 seconds)
{
	// Much like the call to setLock(), this should only be called
	// from one chain during processing, so there is no need to
	// acquire a mutex.
	if(seconds <= 0.0) return false;
	S32 key = setLock();
	if(!key) return false;
	LLRunner::run_handle_t handle = mRunner.addRunnable(
		LLChainSleeper::build(this, key),
		LLRunner::RUN_IN,
		seconds);
	if(0 == handle) return false;
	return true;
}

bool LLPumpIO::copyCurrentLinkInfo(links_t& links) const
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	if(current_chain_t() == mCurrentChain)
	{
		return false;
	}
	std::copy(
		(*mCurrentChain).mChainLinks.begin(),
		(*mCurrentChain).mChainLinks.end(),
		std::back_insert_iterator<links_t>(links));
	return true;
}

void LLPumpIO::pump()
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	LLFastTimer t1(LLFastTimer::FTM_PUMP);
	//llinfos << "LLPumpIO::pump()" << llendl;

	// Run any pending runners.
	mRunner.run();

	// We need to move all of the pending heads over to the running
	// chains.
	PUMP_DEBUG;
	if(true)
	{
#if LL_THREADS_APR
		LLScopedLock lock(mChainsMutex);
#endif
		// bail if this pump is paused.
		if(PAUSING == mState)
		{
			mState = PAUSED;
		}
		if(PAUSED == mState)
		{
			return;
		}

		PUMP_DEBUG;
		// Move the pending chains over to the running chaings
		if(!mPendingChains.empty())
		{
			PUMP_DEBUG;
			//lldebugs << "Pushing " << mPendingChains.size() << "." << llendl;
			std::copy(
				mPendingChains.begin(),
				mPendingChains.end(),
				std::back_insert_iterator<running_chains_t>(mRunningChains));
			mPendingChains.clear();
			PUMP_DEBUG;
		}

		// Clear any locks. This needs to be done here so that we do
		// not clash during a call to clearLock().
		if(!mClearLocks.empty())
		{
			PUMP_DEBUG;
			running_chains_t::iterator it = mRunningChains.begin();
			running_chains_t::iterator end = mRunningChains.end();
			std::set<S32>::iterator not_cleared = mClearLocks.end();
			for(; it != end; ++it)
			{
				if((*it).mLock && mClearLocks.find((*it).mLock) != not_cleared)
				{
					(*it).mLock = 0;
				}
			}
			PUMP_DEBUG;
			mClearLocks.clear();
		}
	}

	PUMP_DEBUG;
	// rebuild the pollset if necessary
	if(mRebuildPollset)
	{
		PUMP_DEBUG;
		rebuildPollset();
		mRebuildPollset = false;
	}

	// Poll based on the last known pollset
	// *FIX: may want to pass in a poll timeout so it works correctly
	// in single and multi threaded processes.
	PUMP_DEBUG;
	typedef std::set<S32> signal_client_t;
	signal_client_t signalled_client;
	if(mPollset)
	{
		PUMP_DEBUG;
		//llinfos << "polling" << llendl;
		S32 count = 0;
		S32 client_id = 0;
		const apr_pollfd_t* poll_fd = NULL;
		apr_pollset_poll(mPollset, DEFAULT_POLL_TIMEOUT, &count, &poll_fd);
		PUMP_DEBUG;
		for(S32 i = 0; i < count; ++i)
		{
			client_id = *((S32*)poll_fd[i].client_data);
			signalled_client.insert(client_id);
		}
		PUMP_DEBUG;
	}

	PUMP_DEBUG;
	// set up for a check to see if each one was signalled
	signal_client_t::iterator not_signalled = signalled_client.end();

	// Process everything as appropriate
	//lldebugs << "Running chain count: " << mRunningChains.size() << llendl;
	running_chains_t::iterator run_chain = mRunningChains.begin();
	bool process_this_chain = false;
	for(; run_chain != mRunningChains.end(); )
	{
		PUMP_DEBUG;
		if((*run_chain).mInit
		   && (*run_chain).mTimer.getStarted()
		   && (*run_chain).mTimer.hasExpired())
		{
			PUMP_DEBUG;
			if(handleChainError(*run_chain, LLIOPipe::STATUS_EXPIRED))
			{
				// the pipe probably handled the error. If the handler
				// forgot to reset the expiration then we need to do
				// that here.
				if((*run_chain).mTimer.getStarted()
				   && (*run_chain).mTimer.hasExpired())
				{
					PUMP_DEBUG;
					llinfos << "Error handler forgot to reset timeout. "
							<< "Resetting to " << DEFAULT_CHAIN_EXPIRY_SECS
							<< " seconds." << llendl;
					(*run_chain).setTimeoutSeconds(DEFAULT_CHAIN_EXPIRY_SECS);
				}
			}
			else
			{
				PUMP_DEBUG;
				// it timed out and no one handled it, so we need to
				// retire the chain
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
				lldebugs << "Removing chain "
						<< (*run_chain).mChainLinks[0].mPipe
						<< " '"
						<< typeid(*((*run_chain).mChainLinks[0].mPipe)).name()
						<< "' because it timed out." << llendl;
#else
//				lldebugs << "Removing chain "
//						<< (*run_chain).mChainLinks[0].mPipe
//						<< " because we reached the end." << llendl;
#endif
				mRunningChains.erase(run_chain++);
				continue;
			}
		}
		PUMP_DEBUG;
		if((*run_chain).mLock)
		{
			++run_chain;
			continue;
		}
		PUMP_DEBUG;
		mCurrentChain = run_chain;
		if((*run_chain).mDescriptors.empty())
		{
			// if there are no conditionals, just process this chain.
			process_this_chain = true;
			//lldebugs << "no conditionals - processing" << llendl;
		}
		else
		{
			PUMP_DEBUG;
			//lldebugs << "checking conditionals" << llendl;
			// Check if this run chain was signalled. If any file
			// descriptor is ready for something, then go ahead and
			// process this chian.
			process_this_chain = false;
			if(!signalled_client.empty())
			{
				PUMP_DEBUG;
				LLChainInfo::conditionals_t::iterator it;
				it = (*run_chain).mDescriptors.begin();
				LLChainInfo::conditionals_t::iterator end;
				end = (*run_chain).mDescriptors.end();
				S32 client_id = 0;
				for(; it != end; ++it)
				{
					PUMP_DEBUG;
					client_id = *((S32*)((*it).second.client_data));
					if(signalled_client.find(client_id) != not_signalled)
					{
						process_this_chain = true;
						break;
					}
					//llinfos << "no fd ready for this one." << llendl;
				}
			}
		}
		if(process_this_chain)
		{
			PUMP_DEBUG;
			if(!((*run_chain).mInit))
			{
				(*run_chain).mHead = (*run_chain).mChainLinks.begin();
				(*run_chain).mInit = true;
			}
			PUMP_DEBUG;
			processChain(*run_chain);
		}

		PUMP_DEBUG;
		if((*run_chain).mHead == (*run_chain).mChainLinks.end())
		{
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
			lldebugs << "Removing chain " << (*run_chain).mChainLinks[0].mPipe
					<< " '"
					<< typeid(*((*run_chain).mChainLinks[0].mPipe)).name()
					<< "' because we reached the end." << llendl;
#else
//			lldebugs << "Removing chain " << (*run_chain).mChainLinks[0].mPipe
//					<< " because we reached the end." << llendl;
#endif

			PUMP_DEBUG;
			// This chain is done. Clean up any allocated memory and
			// erase the chain info.
			std::for_each(
				(*run_chain).mDescriptors.begin(),
				(*run_chain).mDescriptors.end(),
				ll_delete_apr_pollset_fd_client_data());
			mRunningChains.erase(run_chain++);

			// *NOTE: may not always need to rebuild the pollset.
			mRebuildPollset = true;
		}
		else
		{
			PUMP_DEBUG;
			// this chain needs more processing - just go to the next
			// chain.
			++run_chain;
		}
	}

	PUMP_DEBUG;
	// null out the chain
	mCurrentChain = current_chain_t();
	END_PUMP_DEBUG;
}

//bool LLPumpIO::respond(const chain_t& pipes)
//{
//#if LL_THREADS_APR
//	LLScopedLock lock(mCallbackMutex);
//#endif
//	LLChainInfo info;
//	links_t links;
//	
//	mPendingCallbacks.push_back(info);
//	return true;
//}

bool LLPumpIO::respond(LLIOPipe* pipe)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	if(NULL == pipe) return false;

#if LL_THREADS_APR
	LLScopedLock lock(mCallbackMutex);
#endif
	LLChainInfo info;
	LLLinkInfo link;
	link.mPipe = pipe;
	info.mChainLinks.push_back(link);
	mPendingCallbacks.push_back(info);
	return true;
}

bool LLPumpIO::respond(
	const links_t& links,
	LLIOPipe::buffer_ptr_t data,
	LLSD context)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	// if the caller is providing a full link description, we need to
	// have that description matched to a particular buffer.
	if(!data) return false;
	if(links.empty()) return false;

#if LL_THREADS_APR
	LLScopedLock lock(mCallbackMutex);
#endif

	// Add the callback response
	LLChainInfo info;
	info.mChainLinks = links;
	info.mData = data;
	info.mContext = context;
	mPendingCallbacks.push_back(info);
	return true;
}

void LLPumpIO::callback()
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	//llinfos << "LLPumpIO::callback()" << llendl;
	if(true)
	{
#if LL_THREADS_APR
		LLScopedLock lock(mCallbackMutex);
#endif
		std::copy(
			mPendingCallbacks.begin(),
			mPendingCallbacks.end(),
			std::back_insert_iterator<callbacks_t>(mCallbacks));
		mPendingCallbacks.clear();
	}
	if(!mCallbacks.empty())
	{
		callbacks_t::iterator it = mCallbacks.begin();
		callbacks_t::iterator end = mCallbacks.end();
		for(; it != end; ++it)
		{
			// it's always the first and last time for respone chains
			(*it).mHead = (*it).mChainLinks.begin();
			(*it).mInit = true;
			(*it).mEOS = true;
			processChain(*it);
		}
		mCallbacks.clear();
	}
}

void LLPumpIO::control(LLPumpIO::EControl op)
{
#if LL_THREADS_APR
	LLScopedLock lock(mChainsMutex);
#endif
	switch(op)
	{
	case PAUSE:
		mState = PAUSING;
		break;
	case RESUME:
		mState = NORMAL;
		break;
	default:
		// no-op
		break;
	}
}

void LLPumpIO::initialize(apr_pool_t* pool)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	if(!pool) return;
#if LL_THREADS_APR
	apr_thread_mutex_create(&mChainsMutex, APR_THREAD_MUTEX_DEFAULT, pool);
	apr_thread_mutex_create(&mCallbackMutex, APR_THREAD_MUTEX_DEFAULT, pool);
#endif
	mPool = pool;
}

void LLPumpIO::cleanup()
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
#if LL_THREADS_APR
	if(mChainsMutex) apr_thread_mutex_destroy(mChainsMutex);
	if(mCallbackMutex) apr_thread_mutex_destroy(mCallbackMutex);
#endif
	mChainsMutex = NULL;
	mCallbackMutex = NULL;
	if(mPollset)
	{
//		lldebugs << "cleaning up pollset" << llendl;
		apr_pollset_destroy(mPollset);
		mPollset = NULL;
	}
	if(mCurrentPool)
	{
		apr_pool_destroy(mCurrentPool);
		mCurrentPool = NULL;
	}
	mPool = NULL;
}

void LLPumpIO::rebuildPollset()
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
//	lldebugs << "LLPumpIO::rebuildPollset()" << llendl;
	if(mPollset)
	{
		//lldebugs << "destroying pollset" << llendl;
		apr_pollset_destroy(mPollset);
		mPollset = NULL;
	}
	U32 size = 0;
	running_chains_t::iterator run_it = mRunningChains.begin();
	running_chains_t::iterator run_end = mRunningChains.end();
	for(; run_it != run_end; ++run_it)
	{
		size += (*run_it).mDescriptors.size();
	}
	//lldebugs << "found " << size << " descriptors." << llendl;
	if(size)
	{
		// Recycle the memory pool
		const S32 POLLSET_POOL_RECYCLE_COUNT = 100;
		if(mCurrentPool
		   && (0 == (++mCurrentPoolReallocCount % POLLSET_POOL_RECYCLE_COUNT)))
		{
			apr_pool_destroy(mCurrentPool);
			mCurrentPool = NULL;
			mCurrentPoolReallocCount = 0;
		}
		if(!mCurrentPool)
		{
			apr_status_t status = apr_pool_create(&mCurrentPool, mPool);
			(void)ll_apr_warn_status(status);
		}

		// add all of the file descriptors
		run_it = mRunningChains.begin();
		LLChainInfo::conditionals_t::iterator fd_it;
		LLChainInfo::conditionals_t::iterator fd_end;
		apr_pollset_create(&mPollset, size, mCurrentPool, 0);
		for(; run_it != run_end; ++run_it)
		{
			fd_it = (*run_it).mDescriptors.begin();
			fd_end = (*run_it).mDescriptors.end();
			for(; fd_it != fd_end; ++fd_it)
			{
				apr_pollset_add(mPollset, &((*fd_it).second));
			}
		}
	}
}

void LLPumpIO::processChain(LLChainInfo& chain)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	LLIOPipe::EStatus status = LLIOPipe::STATUS_OK;
	links_t::iterator it = chain.mHead;
	links_t::iterator end = chain.mChainLinks.end();
	bool need_process_signaled = false;
	bool keep_going = true;
	do
	{
#if LL_DEBUG_PROCESS_LINK
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
		llinfos << "Processing " << typeid(*((*it).mPipe)).name() << "."
				<< llendl;
#else
		llinfos << "Processing link " << (*it).mPipe << "." << llendl;
#endif
#endif
#if LL_DEBUG_SPEW_BUFFER_CHANNEL_IN
		if(chain.mData)
		{
			char* buf = NULL;
			S32 bytes = chain.mData->countAfter((*it).mChannels.in(), NULL);
			if(bytes)
			{
				buf = new char[bytes + 1];
				chain.mData->readAfter(
					(*it).mChannels.in(),
					NULL,
					(U8*)buf,
					bytes);
				buf[bytes] = '\0';
				llinfos << "CHANNEL IN(" << (*it).mChannels.in() << "): "
						<< buf << llendl;
				delete[] buf;
				buf = NULL;
			}
			else
			{
				llinfos << "CHANNEL IN(" << (*it).mChannels.in()<< "): (null)"
						<< llendl;
			}
		}
#endif
		PUMP_DEBUG;
		status = (*it).mPipe->process(
			(*it).mChannels,
			chain.mData,
			chain.mEOS,
			chain.mContext,
			this);
#if LL_DEBUG_SPEW_BUFFER_CHANNEL_OUT
		if(chain.mData)
		{
			char* buf = NULL;
			S32 bytes = chain.mData->countAfter((*it).mChannels.out(), NULL);
			if(bytes)
			{
				buf = new char[bytes + 1];
				chain.mData->readAfter(
					(*it).mChannels.out(),
					NULL,
					(U8*)buf,
					bytes);
				buf[bytes] = '\0';
				llinfos << "CHANNEL OUT(" << (*it).mChannels.out()<< "): "
						<< buf << llendl;
				delete[] buf;
				buf = NULL;
			}
			else
			{
				llinfos << "CHANNEL OUT(" << (*it).mChannels.out()<< "): (null)"
						<< llendl;
			}
		}
#endif

#if LL_DEBUG_PROCESS_RETURN_VALUE
		// Only bother with the success codes - error codes are logged
		// below.
		if(LLIOPipe::isSuccess(status))
		{
			llinfos << "Pipe returned: '"
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
					<< typeid(*((*it).mPipe)).name() << "':'"
#endif
					<< LLIOPipe::lookupStatusString(status) << "'" << llendl;
		}
#endif

		PUMP_DEBUG;
		switch(status)
		{
		case LLIOPipe::STATUS_OK:
			// no-op
			break;
		case LLIOPipe::STATUS_STOP:
			PUMP_DEBUG;
			status = LLIOPipe::STATUS_OK;
			chain.mHead = end;
			keep_going = false;
			break;
		case LLIOPipe::STATUS_DONE:
			PUMP_DEBUG;
			status = LLIOPipe::STATUS_OK;
			chain.mHead = (it + 1);
			chain.mEOS = true;
			break;
		case LLIOPipe::STATUS_BREAK:
			PUMP_DEBUG;
			status = LLIOPipe::STATUS_OK;
			keep_going = false;
			break;
		case LLIOPipe::STATUS_NEED_PROCESS:
			PUMP_DEBUG;
			status = LLIOPipe::STATUS_OK;
			if(!need_process_signaled)
			{
				need_process_signaled = true;
				chain.mHead = it;
			}
			break;
		default:
			PUMP_DEBUG;
			if(LLIOPipe::isError(status))
			{
				llinfos << "Pump generated pipe error: '"
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
						<< typeid(*((*it).mPipe)).name() << "':'"
#endif
						<< LLIOPipe::lookupStatusString(status)
						<< "'" << llendl;
#if LL_DEBUG_SPEW_BUFFER_CHANNEL_IN_ON_ERROR
				if(chain.mData)
				{
					char* buf = NULL;
					S32 bytes = chain.mData->countAfter(
						(*it).mChannels.in(),
						NULL);
					if(bytes)
					{
						buf = new char[bytes + 1];
						chain.mData->readAfter(
							(*it).mChannels.in(),
							NULL,
							(U8*)buf,
							bytes);
						buf[bytes] = '\0';
						llinfos << "Input After Error: " << buf << llendl;
						delete[] buf;
						buf = NULL;
					}
					else
					{
						llinfos << "Input After Error: (null)" << llendl;
					}
				}
				else
				{
					llinfos << "Input After Error: (null)" << llendl;
				}
#endif
				keep_going = false;
				chain.mHead  = it;
				if(!handleChainError(chain, status))
				{
					chain.mHead = end;
				}
			}
			else
			{
				llinfos << "Unhandled status code: " << status << ":"
						<< LLIOPipe::lookupStatusString(status) << llendl;
			}
			break;
		}
		PUMP_DEBUG;
	} while(keep_going && (++it != end));
	PUMP_DEBUG;
}

bool LLPumpIO::handleChainError(
	LLChainInfo& chain,
	LLIOPipe::EStatus error)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	links_t::reverse_iterator rit;
	if(chain.mHead == chain.mChainLinks.end())
	{
		rit = links_t::reverse_iterator(chain.mHead);
	}
	else
	{
		rit = links_t::reverse_iterator(chain.mHead + 1);
	}
	
	links_t::reverse_iterator rend = chain.mChainLinks.rend();
	bool handled = false;
	bool keep_going = true;
	do
	{
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
		lldebugs << "Passing error to " << typeid(*((*rit).mPipe)).name()
				 << "." << llendl;
#endif
		error = (*rit).mPipe->handleError(error, this);
		switch(error)
		{
		case LLIOPipe::STATUS_OK:
			handled = true;
			chain.mHead = rit.base();
			break;
		case LLIOPipe::STATUS_STOP:
		case LLIOPipe::STATUS_DONE:
		case LLIOPipe::STATUS_BREAK:
		case LLIOPipe::STATUS_NEED_PROCESS:
#if LL_DEBUG_PIPE_TYPE_IN_PUMP
			lldebugs << "Pipe " << typeid(*((*rit).mPipe)).name()
					 << " returned code to stop error handler." << llendl;
#endif
			keep_going = false;
			break;
		default:
			if(LLIOPipe::isSuccess(error))
			{
				llinfos << "Unhandled status code: " << error << ":"
						<< LLIOPipe::lookupStatusString(error) << llendl;
				error = LLIOPipe::STATUS_ERROR;
				keep_going = false;
			}
			break;
		}
	} while(keep_going && !handled && (++rit != rend));
	return handled;
}

/**
 * LLPumpIO::LLChainInfo
 */

LLPumpIO::LLChainInfo::LLChainInfo() :
	mInit(false),
	mLock(0),
	mEOS(false)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	mTimer.setTimerExpirySec(DEFAULT_CHAIN_EXPIRY_SECS);
}

void LLPumpIO::LLChainInfo::setTimeoutSeconds(F32 timeout)
{
	LLMemType m1(LLMemType::MTYPE_IO_PUMP);
	if(timeout > 0.0f)
	{
		mTimer.start();
		mTimer.reset();
		mTimer.setTimerExpirySec(timeout);
	}
	else
	{
		mTimer.stop();
	}
}
