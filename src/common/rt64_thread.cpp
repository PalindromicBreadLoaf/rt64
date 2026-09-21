//
// RT64
//

#include "rt64_thread.h"

#include <cassert>
#include <thread>

#if defined(_WIN64)
#   include <Windows.h>
#   include "utf8conv/utf8conv.h"
#elif defined(__linux__)
#   include <pthread.h>
#elif defined(__SWITCH__)
#   include <switch.h>
#endif

namespace RT64 {
#   if defined(_WIN32)
    static int toWindowsPriority(Thread::Priority priority) {
        switch (priority) {
        case Thread::Priority::Idle:
            return THREAD_PRIORITY_IDLE;
        case Thread::Priority::Lowest:
            return THREAD_PRIORITY_LOWEST;
        case Thread::Priority::Low:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case Thread::Priority::Normal:
            return THREAD_PRIORITY_NORMAL;
        case Thread::Priority::High:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        case Thread::Priority::Highest:
            return THREAD_PRIORITY_HIGHEST;
        default:
            assert(false && "Unknown thread priority.");
            return THREAD_PRIORITY_NORMAL;
        }
    }
#   endif

#   if defined(__SWITCH__)
    extern "C" __attribute__((weak)) void rt64_switch_place_thread(const char *name);

    static u32 toHorizonPriority(Thread::Priority priority) {
        switch (priority) {
        case Thread::Priority::Idle:
            return 0x3B;
        case Thread::Priority::Lowest:
            return 0x32;
        case Thread::Priority::Low:
            return 0x30;
        case Thread::Priority::Normal:
            return 0x2C;
        case Thread::Priority::High:
            return 0x2A;
        case Thread::Priority::Highest:
            return 0x28;
        default:
            assert(false && "Unknown thread priority.");
            return 0x2C;
        }
    }
#   endif

    // Thread

    void Thread::setCurrentThreadName(const std::string &str) {
#   if defined(_WIN32)
        std::wstring nameWide = win32::Utf8ToUtf16(str);
        SetThreadDescription(GetCurrentThread(), nameWide.c_str());
#   elif defined(__linux__)
        pthread_setname_np(pthread_self(), str.c_str());
#   elif defined(__APPLE__)
        pthread_setname_np(str.c_str());
#   elif defined(__SWITCH__)
        if (rt64_switch_place_thread != nullptr) {
            rt64_switch_place_thread(str.c_str());
        }
#   else
        static_assert(false, "Unimplemented");
#   endif
    }

    void Thread::setCurrentThreadPriority(Priority priority) {
#   if defined(_WIN32)
        SetThreadPriority(GetCurrentThread(), toWindowsPriority(priority));
#   elif defined(__linux__) || defined(__APPLE__)
        // On Linux, thread priorities can't be changed under the default scheduler policy (SCHED_OTHER) and the other policies
        // that are available without root privileges are lower priority. Instead you can set the thread's "nice" value, which ranges
        // from -20 to 19 (lower being higher priority). However, by strict POSIX spec "nice" is meant to be per-process instead of
        // per-thread. Therefore to avoid issues in case Linux is modified to match the spec in the future, this function does nothing.
        (void)priority;
#   elif defined(__SWITCH__)
        svcSetThreadPriority(CUR_THREAD_HANDLE, toHorizonPriority(priority));
#   else
        static_assert(false, "Unimplemented");
#   endif
    }

    void Thread::sleepMilliseconds(uint32_t millis) {
#   if defined(_WIN32)
        // The implementations of std::chrono::sleep_until and sleep_for were affected by changing the system clock backwards in older versions
        // of Microsoft's STL. This was fixed as of Visual Studio 2022 17.9, but to be safe RT64 uses Win32 Sleep directly.
        Sleep(millis);
#   else
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
#   endif
    }
};
