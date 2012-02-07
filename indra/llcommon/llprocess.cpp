/** 
 * @file llprocess.cpp
 * @brief Utility class for launching, terminating, and tracking the state of processes.
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

#include "linden_common.h"
#include "llprocess.h"
#include "llsdserialize.h"
#include "llsingleton.h"
#include "llstring.h"
#include "stringize.h"
#include "llapr.h"

#include <boost/foreach.hpp>
#include <iostream>
#include <stdexcept>

static std::string empty;
static LLProcess::Status interpret_status(int status);

/// Need an exception to avoid constructing an invalid LLProcess object, but
/// internal use only
struct LLProcessError: public std::runtime_error
{
	LLProcessError(const std::string& msg): std::runtime_error(msg) {}
};

LLProcessPtr LLProcess::create(const LLSDOrParams& params)
{
	try
	{
		return LLProcessPtr(new LLProcess(params));
	}
	catch (const LLProcessError& e)
	{
		LL_WARNS("LLProcess") << e.what() << LL_ENDL;
		return LLProcessPtr();
	}
}

/// Call an apr function returning apr_status_t. On failure, log warning and
/// throw LLProcessError mentioning the function call that produced that
/// result.
#define chkapr(func)                            \
    if (ll_apr_warn_status(func))               \
        throw LLProcessError(#func " failed")

LLProcess::LLProcess(const LLSDOrParams& params):
	mAutokill(params.autokill)
{
	if (! params.validateBlock(true))
	{
		throw LLProcessError(STRINGIZE("not launched: failed parameter validation\n"
									   << LLSDNotationStreamer(params)));
	}

	apr_procattr_t *procattr = NULL;
	chkapr(apr_procattr_create(&procattr, gAPRPoolp));

	// For which of stdin, stdout, stderr should we create a pipe to the
	// child? In the viewer, there are only a couple viable
	// apr_procattr_io_set() alternatives: inherit the viewer's own stdxxx
	// handle (APR_NO_PIPE, e.g. for stdout, stderr), or create a pipe that's
	// blocking on the child end but nonblocking at the viewer end
	// (APR_CHILD_BLOCK). The viewer can't block for anything: the parent end
	// MUST be nonblocking. As the APR documentation itself points out, it
	// makes very little sense to set nonblocking I/O for the child end of a
	// pipe: only a specially-written child could deal with that.
	// Other major options could include explicitly creating a single APR pipe
	// and passing it as both stdout and stderr (apr_procattr_child_out_set(),
	// apr_procattr_child_err_set()), or accepting a filename, opening it and
	// passing that apr_file_t (simple <, >, 2> redirect emulation).
//	chkapr(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
	chkapr(apr_procattr_io_set(procattr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE));

	// Thumbs down on implicitly invoking the shell to invoke the child. From
	// our point of view, the other major alternative to APR_PROGRAM_PATH
	// would be APR_PROGRAM_ENV: still copy environment, but require full
	// executable pathname. I don't see a downside to searching the PATH,
	// though: if our caller wants (e.g.) a specific Python interpreter, s/he
	// can still pass the full pathname.
	chkapr(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));
	// YES, do extra work if necessary to report child exec() failures back to
	// parent process.
	chkapr(apr_procattr_error_check_set(procattr, 1));
	// Do not start a non-autokill child in detached state. On Posix
	// platforms, this setting attempts to daemonize the new child, closing
	// std handles and the like, and that's a bit more detachment than we
	// want. autokill=false just means not to implicitly kill the child when
	// the parent terminates!
//	chkapr(apr_procattr_detach_set(procattr, params.autokill? 0 : 1));

	if (params.autokill)
	{
#if defined(APR_HAS_PROCATTR_AUTOKILL_SET)
		apr_status_t ok = apr_procattr_autokill_set(procattr, 1);
# if LL_WINDOWS
		// As of 2012-02-02, we only expect this to be implemented on Windows.
		// Avoid spamming the log with warnings we fully expect.
		ll_apr_warn_status(ok);
# endif // LL_WINDOWS
#else
		LL_WARNS("LLProcess") << "This version of APR lacks Linden apr_procattr_autokill_set() extension" << LL_ENDL;
#endif
	}

	// Have to instantiate named std::strings for string params items so their
	// c_str() values persist.
	std::string cwd(params.cwd);
	if (! cwd.empty())
	{
		chkapr(apr_procattr_dir_set(procattr, cwd.c_str()));
	}

	// create an argv vector for the child process
	std::vector<const char*> argv;

	// add the executable path
	std::string executable(params.executable);
	argv.push_back(executable.c_str());

	// and any arguments
	std::vector<std::string> args(params.args.begin(), params.args.end());
	BOOST_FOREACH(const std::string& arg, args)
	{
		argv.push_back(arg.c_str());
	}

	// terminate with a null pointer
	argv.push_back(NULL);

	// Launch! The NULL would be the environment block, if we were passing one.
	chkapr(apr_proc_create(&mProcess, argv[0], &argv[0], NULL, procattr, gAPRPoolp));    

	// arrange to call status_callback()
	apr_proc_other_child_register(&mProcess, &LLProcess::status_callback, this, mProcess.in,
								  gAPRPoolp);
	mStatus.mState = RUNNING;

	mDesc = STRINGIZE(LLStringUtil::quote(params.executable) << " (" << mProcess.pid << ')');
	LL_INFOS("LLProcess") << "Launched " << params << " (" << mProcess.pid << ")" << LL_ENDL;

	// Unless caller explicitly turned off autokill (child should persist),
	// take steps to terminate the child. This is all suspenders-and-belt: in
	// theory our destructor should kill an autokill child, but in practice
	// that doesn't always work (e.g. VWR-21538).
	if (params.autokill)
	{
		// Tie the lifespan of this child process to the lifespan of our APR
		// pool: on destruction of the pool, forcibly kill the process. Tell
		// APR to try SIGTERM and wait 3 seconds. If that didn't work, use
		// SIGKILL.
		apr_pool_note_subprocess(gAPRPoolp, &mProcess, APR_KILL_AFTER_TIMEOUT);

		// On Windows, associate the new child process with our Job Object.
		autokill();
	}
}

LLProcess::~LLProcess()
{
	// Only in state RUNNING are we registered for callback. In UNSTARTED we
	// haven't yet registered. And since receiving the callback is the only
	// way we detect child termination, we only change from state RUNNING at
	// the same time we unregister.
	if (mStatus.mState == RUNNING)
	{
		// We're still registered for a callback: unregister. Do it before
		// we even issue the kill(): even if kill() somehow prompted an
		// instantaneous callback (unlikely), this object is going away! Any
		// information updated in this object by such a callback is no longer
		// available to any consumer anyway.
		apr_proc_other_child_unregister(this);
	}

	if (mAutokill)
	{
		kill("destructor");
	}
}

bool LLProcess::kill(const std::string& who)
{
	if (isRunning())
	{
		LL_INFOS("LLProcess") << who << " killing " << mDesc << LL_ENDL;

#if LL_WINDOWS
		int sig = -1;
#else  // Posix
		int sig = SIGTERM;
#endif

		ll_apr_warn_status(apr_proc_kill(&mProcess, sig));
	}

	return ! isRunning();
}

bool LLProcess::isRunning(void)
{
	return getStatus().mState == RUNNING;
}

LLProcess::Status LLProcess::getStatus()
{
	// Only when mState is RUNNING might the status change dynamically. For
	// any other value, pointless to attempt to update status: it won't
	// change.
	if (mStatus.mState == RUNNING)
	{
		// Tell APR to sense whether the child is still running and call
		// handle_status() appropriately. We should be able to get the same
		// info from an apr_proc_wait(APR_NOWAIT) call; but at least in APR
		// 1.4.2, testing suggests that even with APR_NOWAIT, apr_proc_wait()
		// blocks the caller. We can't have that in the viewer. Hence the
		// callback rigmarole. Once we update APR, it's probably worth testing
		// again. Also -- although there's an apr_proc_other_child_refresh()
		// call, i.e. get that information for one specific child, it accepts
		// an 'apr_other_child_rec_t*' that's mentioned NOWHERE else in the
		// documentation or header files! I would use the specific call if I
		// knew how. As it is, each call to this method will call callbacks
		// for ALL still-running child processes. Sigh...
		apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
	}

	return mStatus;
}

std::string LLProcess::getStatusString()
{
	return getStatusString(getStatus());
}

std::string LLProcess::getStatusString(const Status& status)
{
	return getStatusString(mDesc, status);
}

//static
std::string LLProcess::getStatusString(const std::string& desc, const Status& status)
{
	if (status.mState == UNSTARTED)
		return desc + " was never launched";

	if (status.mState == RUNNING)
		return desc + " running";

	if (status.mState == EXITED)
		return STRINGIZE(desc << " exited with code " << status.mData);

	if (status.mState == KILLED)
#if LL_WINDOWS
		return STRINGIZE(desc << " killed with exception " << std::hex << status.mData);
#else
		return STRINGIZE(desc << " killed by signal " << status.mData);
#endif


	return STRINGIZE(desc << " in unknown state " << status.mState << " (" << status.mData << ")");
}

// Classic-C-style APR callback
void LLProcess::status_callback(int reason, void* data, int status)
{
	// Our only role is to bounce this static method call back into object
	// space.
	static_cast<LLProcess*>(data)->handle_status(reason, status);
}

#define tabent(symbol) { symbol, #symbol }
static struct ReasonCode
{
	int code;
	const char* name;
} reasons[] =
{
	tabent(APR_OC_REASON_DEATH),
	tabent(APR_OC_REASON_UNWRITABLE),
	tabent(APR_OC_REASON_RESTART),
	tabent(APR_OC_REASON_UNREGISTER),
	tabent(APR_OC_REASON_LOST),
	tabent(APR_OC_REASON_RUNNING)
};
#undef tabent

// Object-oriented callback
void LLProcess::handle_status(int reason, int status)
{
	{
		// This odd appearance of LL_DEBUGS is just to bracket a lookup that will
		// only be performed if in fact we're going to produce the log message.
		LL_DEBUGS("LLProcess") << empty;
		std::string reason_str;
		BOOST_FOREACH(const ReasonCode& rcp, reasons)
		{
			if (reason == rcp.code)
			{
				reason_str = rcp.name;
				break;
			}
		}
		if (reason_str.empty())
		{
			reason_str = STRINGIZE("unknown reason " << reason);
		}
		LL_CONT << mDesc << ": handle_status(" << reason_str << ", " << status << ")" << LL_ENDL;
	}

	if (! (reason == APR_OC_REASON_DEATH || reason == APR_OC_REASON_LOST))
	{
		// We're only interested in the call when the child terminates.
		return;
	}

	// Somewhat oddly, APR requires that you explicitly unregister even when
	// it already knows the child has terminated. We must pass the same 'data'
	// pointer as for the register() call, which was our 'this'.
	apr_proc_other_child_unregister(this);
	// We overload mStatus.mState to indicate whether the child is registered
	// for APR callback: only RUNNING means registered. Track that we've
	// unregistered. We know the child has terminated; might be EXITED or
	// KILLED; refine below.
	mStatus.mState = EXITED;

//	wi->rv = apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT);
	// It's just wrong to call apr_proc_wait() here. The only way APR knows to
	// call us with APR_OC_REASON_DEATH is that it's already reaped this child
	// process, so calling wait() will only produce "huh?" from the OS. We
	// must rely on the status param passed in, which unfortunately comes
	// straight from the OS wait() call, which means we have to decode it by
	// hand.
	mStatus = interpret_status(status);
	LL_INFOS("LLProcess") << getStatusString() << LL_ENDL;
}

LLProcess::id LLProcess::getProcessID() const
{
	return mProcess.pid;
}

LLProcess::handle LLProcess::getProcessHandle() const
{
#if LL_WINDOWS
	return mProcess.hproc;
#else
	return mProcess.pid;
#endif
}

std::ostream& operator<<(std::ostream& out, const LLProcess::Params& params)
{
	std::string cwd(params.cwd);
	if (! cwd.empty())
	{
		out << "cd " << LLStringUtil::quote(cwd) << ": ";
	}
	out << LLStringUtil::quote(params.executable);
	BOOST_FOREACH(const std::string& arg, params.args)
	{
		out << ' ' << LLStringUtil::quote(arg);
	}
	return out;
}

/*****************************************************************************
*   Windows specific
*****************************************************************************/
#if LL_WINDOWS

static std::string WindowsErrorString(const std::string& operation);

/**
 * Wrap a Windows Job Object for use in managing child-process lifespan.
 *
 * On Windows, we use a Job Object to constrain the lifespan of any
 * autokill=true child process to the viewer's own lifespan:
 * http://stackoverflow.com/questions/53208/how-do-i-automatically-destroy-child-processes-in-windows
 * (thanks Richard!).
 *
 * We manage it using an LLSingleton for a couple of reasons:
 *
 * # Lazy initialization: if some viewer session never launches a child
 *   process, we should never have to create a Job Object.
 * # Cross-DLL support: be wary of C++ statics when multiple DLLs are
 *   involved.
 */
class LLJob: public LLSingleton<LLJob>
{
public:
	void assignProcess(const std::string& prog, LLProcess::handle hProcess)
	{
		// If we never managed to initialize this Job Object, can't use it --
		// but don't keep spamming the log, we already emitted warnings when
		// we first tried to create.
		if (! mJob)
			return;

		if (! AssignProcessToJobObject(mJob, hProcess))
		{
			LL_WARNS("LLProcess") << WindowsErrorString(STRINGIZE("AssignProcessToJobObject("
																  << prog << ")")) << LL_ENDL;
		}
	}

private:
	friend class LLSingleton<LLJob>;
	LLJob():
		mJob(0)
	{
		mJob = CreateJobObject(NULL, NULL);
		if (! mJob)
		{
			LL_WARNS("LLProcess") << WindowsErrorString("CreateJobObject()") << LL_ENDL;
			return;
		}

		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };

		// Configure all child processes associated with this new job object
		// to terminate when the calling process (us!) terminates.
		jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		if (! SetInformationJobObject(mJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
		{
			LL_WARNS("LLProcess") << WindowsErrorString("SetInformationJobObject()") << LL_ENDL;
			// This Job Object is useless to us
			CloseHandle(mJob);
			// prevent assignProcess() from trying to use it
			mJob = 0;
		}
	}

	LLProcess::handle mJob;
};

void LLProcess::autokill()
{
	LLJob::instance().assignProcess(mDesc, mProcess.hproc);
}

LLProcess::handle LLProcess::isRunning(handle h, const std::string& desc)
{
	// This direct Windows implementation is because we have no access to the
	// apr_proc_t struct: we expect it's been destroyed.
	if (! h)
		return 0;

	DWORD waitresult = WaitForSingleObject(h, 0);
	if(waitresult == WAIT_OBJECT_0)
	{
		// the process has completed.
		if (! desc.empty())
		{
			DWORD status = 0;
			if (! GetExitCodeProcess(h, &status))
			{
				LL_WARNS("LLProcess") << desc << " terminated, but "
									  << WindowsErrorString("GetExitCodeProcess()") << LL_ENDL;
			}
			{
				LL_INFOS("LLProcess") << getStatusString(desc, interpret_status(status))
									  << LL_ENDL;
			}
		}
		CloseHandle(h);
		return 0;
	}

	return h;
}

static LLProcess::Status interpret_status(int status)
{
	LLProcess::Status result;

	// This bit of code is cribbed from apr/threadproc/win32/proc.c, a
	// function (unfortunately static) called why_from_exit_code():
	/* See WinNT.h STATUS_ACCESS_VIOLATION and family for how
	 * this class of failures was determined
	 */
	if ((status & 0xFFFF0000) == 0xC0000000)
	{
		result.mState = KILLED;
	}
	else
	{
		result.mState = EXITED;
	}
	result.mData = status;

	return result;
}

/// GetLastError()/FormatMessage() boilerplate
static std::string WindowsErrorString(const std::string& operation)
{
	int result = GetLastError();

	LPTSTR error_str = 0;
	if (FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL,
					   result,
					   0,
					   (LPTSTR)&error_str,
					   0,
					   NULL)
		!= 0) 
	{
        // convert from wide-char string to multi-byte string
		char message[256];
		wcstombs(message, error_str, sizeof(message));
		message[sizeof(message)-1] = 0;
		LocalFree(error_str);
		// convert to std::string to trim trailing whitespace
		std::string mbsstr(message);
		mbsstr.erase(mbsstr.find_last_not_of(" \t\r\n"));
		return STRINGIZE(operation << " failed (" << result << "): " << mbsstr);
	}
	return STRINGIZE(operation << " failed (" << result
					 << "), but FormatMessage() did not explain");
}

/*****************************************************************************
*   Posix specific
*****************************************************************************/
#else // Mac and linux

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

void LLProcess::autokill()
{
	// What we ought to do here is to:
	// 1. create a unique process group and run all autokill children in that
	//    group (see https://jira.secondlife.com/browse/SWAT-563);
	// 2. figure out a way to intercept control when the viewer exits --
	//    gracefully or not; 
	// 3. when the viewer exits, kill off the aforementioned process group.

	// It's point 2 that's troublesome. Although I've seen some signal-
	// handling logic in the Posix viewer code, I haven't yet found any bit of
	// code that's run no matter how the viewer exits (a try/finally for the
	// whole process, as it were).
}

// Attempt to reap a process ID -- returns true if the process has exited and been reaped, false otherwise.
static bool reap_pid(pid_t pid, LLProcess::Status* pstatus=NULL)
{
	LLProcess::Status dummy;
	if (! pstatus)
	{
		// If caller doesn't want to see Status, give us a target anyway so we
		// don't have to have a bunch of conditionals.
		pstatus = &dummy;
	}

	int status = 0;
	pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
	if (wait_result == pid)
	{
		*pstatus = interpret_status(status);
		return true;
	}
	if (wait_result == 0)
	{
		pstatus->mState = LLProcess::RUNNING;
		pstatus->mData	= 0;
		return false;
	}

	// Clear caller's Status block; caller must interpret UNSTARTED to mean
	// "if this PID was ever valid, it no longer is."
	*pstatus = LLProcess::Status();

	// We've dealt with the success cases: we were able to reap the child
	// (wait_result == pid) or it's still running (wait_result == 0). It may
	// be that the child terminated but didn't hang around long enough for us
	// to reap. In that case we still have no Status to report, but we can at
	// least state that it's not running.
	if (wait_result == -1 && errno == ECHILD)
	{
		// No such process -- this may mean we're ignoring SIGCHILD.
		return true;
	}

	// Uh, should never happen?!
	LL_WARNS("LLProcess") << "LLProcess::reap_pid(): waitpid(" << pid << ") returned "
						  << wait_result << "; not meaningful?" << LL_ENDL;
	// If caller is looping until this pid terminates, and if we can't find
	// out, better to break the loop than to claim it's still running.
	return true;
}

LLProcess::id LLProcess::isRunning(id pid, const std::string& desc)
{
	// This direct Posix implementation is because we have no access to the
	// apr_proc_t struct: we expect it's been destroyed.
	if (! pid)
		return 0;

	// Check whether the process has exited, and reap it if it has.
	LLProcess::Status status;
	if(reap_pid(pid, &status))
	{
		// the process has exited.
		if (! desc.empty())
		{
			std::string statstr(desc + " apparently terminated: no status available");
			// We don't just pass UNSTARTED to getStatusString() because, in
			// the context of reap_pid(), that state has special meaning.
			if (status.mState != UNSTARTED)
			{
				statstr = getStatusString(desc, status);
			}
			LL_INFOS("LLProcess") << statstr << LL_ENDL;
		}
		return 0;
	}

	return pid;
}

static LLProcess::Status interpret_status(int status)
{
	LLProcess::Status result;

	if (WIFEXITED(status))
	{
		result.mState = LLProcess::EXITED;
		result.mData  = WEXITSTATUS(status);
	}
	else if (WIFSIGNALED(status))
	{
		result.mState = LLProcess::KILLED;
		result.mData  = WTERMSIG(status);
	}
	else                            // uh, shouldn't happen?
	{
		result.mState = LLProcess::EXITED;
		result.mData  = status;     // someone else will have to decode
	}

	return result;
}

/*==========================================================================*|
static std::list<pid_t> sZombies;

void LLProcess::orphan(void)
{
	// Disassociate the process from this object
	if(mProcessID != 0)
	{	
		// We may still need to reap the process's zombie eventually
		sZombies.push_back(mProcessID);
	
		mProcessID = 0;
	}
}

// static 
void LLProcess::reap(void)
{
	// Attempt to real all saved process ID's.
	
	std::list<pid_t>::iterator iter = sZombies.begin();
	while(iter != sZombies.end())
	{
		if(reap_pid(*iter))
		{
			iter = sZombies.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}
|*==========================================================================*/

#endif  // Posix
