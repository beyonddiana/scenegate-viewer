// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llthreadwatchdog.cpp
 * @brief The LLThreadWatchdog class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h"
#include "llwatchdog.h"
#include "llthread.h"
#include "llwin32headerslean.h"

const U32 WATCHDOG_SLEEP_TIME_USEC = 1000000;

void default_killer_callback()
{
#ifdef LL_WINDOWS
	RaiseException(0,0,0,nullptr);
#else
	raise(SIGQUIT);
#endif
}

// This class runs the watchdog timing thread.
class LLWatchdogTimerThread : public LLThread
{
public:
	LLWatchdogTimerThread() : 
		LLThread("Watchdog"),
		mSleepMsecs(0),
		mStopping(false)
	{
	}
            
	~LLWatchdogTimerThread() {}
    
	void setSleepTime(long ms) { mSleepMsecs = ms; }
	void stop() 
	{
		mStopping = true; 
		mSleepMsecs = 1;
	}
    
	/* virtual */ void run() override
	{
		while(!mStopping)
		{
			LLWatchdog::getInstance()->run();
			ms_sleep(mSleepMsecs);
		}
	}

private:
	long mSleepMsecs;
	bool mStopping;
};

// LLWatchdogEntry
LLWatchdogEntry::LLWatchdogEntry()
{
}

LLWatchdogEntry::~LLWatchdogEntry()
{
	stop();
}

void LLWatchdogEntry::start()
{
	LLWatchdog::getInstance()->add(this);
}

void LLWatchdogEntry::stop()
{
	LLWatchdog::getInstance()->remove(this);
}

// LLWatchdogTimeout
const std::string UNINIT_STRING = "uninitialized";

LLWatchdogTimeout::LLWatchdogTimeout() : 
	mTimeout(0.0f),
	mPingState(UNINIT_STRING)
{
}

LLWatchdogTimeout::~LLWatchdogTimeout() 
{
}

bool LLWatchdogTimeout::isAlive() const 
{ 
	return (mTimer.getStarted() && !mTimer.hasExpired()); 
}

void LLWatchdogTimeout::reset()
{
	mTimer.setTimerExpirySec(mTimeout); 
}

void LLWatchdogTimeout::setTimeout(F32 d) 
{
	mTimeout = d;
}

void LLWatchdogTimeout::start(const std::string& state) 
{
    if (mTimeout == 0)
    {
        LL_WARNS() << "Cant' start watchdog entry - no timeout set" << LL_ENDL;
        return;
    }
	// Order of operation is very important here.
	// After LLWatchdogEntry::start() is called
	// LLWatchdogTimeout::isAlive() will be called asynchronously. 
	ping(state);
	mTimer.start();
    mTimer.setTimerExpirySec(mTimeout); // timer expiration set to 0 by start()
	LLWatchdogEntry::start();
}

void LLWatchdogTimeout::stop() 
{
	LLWatchdogEntry::stop();
	mTimer.stop();
}

void LLWatchdogTimeout::ping(const std::string& state) 
{ 
	if(!state.empty())
	{
		mPingState = state;
	}
	reset();
}

// LLWatchdog
LLWatchdog::LLWatchdog() :
	mSuspectsAccessMutex(nullptr),
	mTimer(nullptr),
	mLastClockCount(0),
	mKillerCallback(&default_killer_callback)
{
}

LLWatchdog::~LLWatchdog()
{
}

void LLWatchdog::add(LLWatchdogEntry* e)
{
	lockThread();
	mSuspects.insert(e);
	unlockThread();
}

void LLWatchdog::remove(LLWatchdogEntry* e)
{
	lockThread();
    mSuspects.erase(e);
	unlockThread();
}

void LLWatchdog::init(killer_event_callback func)
{
	mKillerCallback = func;
	if(!mSuspectsAccessMutex && !mTimer)
	{
		mSuspectsAccessMutex = new LLMutex();
		mTimer = new LLWatchdogTimerThread();
		mTimer->setSleepTime(WATCHDOG_SLEEP_TIME_USEC / 1000);
		mLastClockCount = LLTimer::getTotalTime();

		// mTimer->start() kicks off the thread, any code after
		// start needs to use the mSuspectsAccessMutex
		mTimer->start();
	}
}

void LLWatchdog::cleanup()
{
	if(mTimer)
	{
		mTimer->stop();
		delete mTimer;
		mTimer = nullptr;
	}

	if(mSuspectsAccessMutex)
	{
		delete mSuspectsAccessMutex;
		mSuspectsAccessMutex = nullptr;
	}

	mLastClockCount = 0;
}

void LLWatchdog::run()
{
	lockThread();

	// Check the time since the last call to run...
	// If the time elapsed is two times greater than the regualr sleep time
	// reset the active timeouts.
	const U32 TIME_ELAPSED_MULTIPLIER = 2;
	U64 current_time = LLTimer::getTotalTime();
	U64 current_run_delta = current_time - mLastClockCount;
	mLastClockCount = current_time;
	
	if(current_run_delta > (WATCHDOG_SLEEP_TIME_USEC * TIME_ELAPSED_MULTIPLIER))
	{
		LL_INFOS() << "Watchdog thread delayed: resetting entries." << LL_ENDL;
		std::for_each(mSuspects.begin(), 
			mSuspects.end(), 
			std::mem_fn(&LLWatchdogEntry::reset)
			);
	}
	else
	{
		SuspectsRegistry::iterator result = 
			std::find_if(mSuspects.begin(), 
				mSuspects.end(), 
				std::not1(std::mem_fn(&LLWatchdogEntry::isAlive))
				);
		if(result != mSuspects.end())
		{
			// error!!!
			if(mTimer)
			{
				mTimer->stop();
			}

			LL_INFOS() << "Watchdog detected error:" << LL_ENDL;
			mKillerCallback();
		}
	}


	unlockThread();
}

void LLWatchdog::lockThread()
{
	if(mSuspectsAccessMutex != nullptr)
	{
		mSuspectsAccessMutex->lock();
	}
}

void LLWatchdog::unlockThread()
{
	if(mSuspectsAccessMutex != nullptr)
	{
		mSuspectsAccessMutex->unlock();
	}
}
