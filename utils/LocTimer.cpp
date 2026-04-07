/* Copyright (c) 2015, 2020-2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
Changes from Qualcomm Technologies, Inc. are provided under the following license:
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#define LOG_NDEBUG 0
#define LOG_TAG "LocUtil_Timer"

#include <map>
#include <thread>
#include <mutex>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <LocTimer.h>
#include <log_util.h>
#include <errno.h>

namespace loc_util {

class TimerEngine {
public:
    // This gurantees that it will be called only once
    // Timer engine will create a thread when first timer registers with timer engine,
    // and the thread will not be freed once created on the assumption that timer
    // will always be used for location daemon if it is used once.
    static TimerEngine& instance() {
        static TimerEngine sTimerEngine;
        return sTimerEngine;
    }

    // Each loc timer will use one timerfd
    bool registerTimer(LocTimer* timer, uint32_t timeoutMs) {
        std::lock_guard<std::recursive_mutex> lock(mMutex);

        if (timer->mFd == -1) {
            // CLOCK_BOOTTIME will include time device in suspend and wil not
            // wake up device
            timer->mFd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
            if (timer->mFd == -1) {
                LOC_LOGe("timerfd_create failed: %s", strerror(errno));
                return false;
            }

            epoll_event ev{};
            ev.events = EPOLLIN;
            ev.data.ptr = timer;

            if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, timer->mFd, &ev) == -1) {
                LOC_LOGe("epoll_ctl ADD failed: %s, close timer", strerror(errno));
                close(timer->mFd);
                timer->mFd = -1;
                return false;
            }
        }

        itimerspec spec{};
        spec.it_value.tv_sec = timeoutMs / 1000;
        spec.it_value.tv_nsec = (timeoutMs % 1000) * 1000000;

        if (timerfd_settime(timer->mFd, 0, &spec, nullptr) == -1) {
            LOC_LOGe("timerfd_settime failed: %s", strerror(errno));
            return false;
        }

        timer->mIsRunning.store(true);
        mTimers[timer->mFd] = timer;
        LOC_LOGd("timer name %s, timer-fd %d,timeout %dmsec, total number of timers %d",
                 timer->mName, timer->mFd, timeoutMs, mTimers.size());
        return true;
    }

    bool unregisterTimer(LocTimer* timer) {
        std::lock_guard<std::recursive_mutex> lock(mMutex);

        LOC_LOGd("timer name %s, timer-fd %d, running %d, total number of timers %d",
                 timer->mName, timer->mFd, timer->mIsRunning.load(), mTimers.size());
        if (timer->mFd == -1 || !timer->mIsRunning.load()) return false;

        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, timer->mFd, nullptr);
        close(timer->mFd);
        timer->mFd = -1;
        timer->mIsRunning.store(false);
        mTimers.erase(timer->mFd);

        return true;
    }

private:
    TimerEngine() {
        LOC_LOGd("timer engine constructor enter");
        mEpollFd = epoll_create1(0);
        if (mEpollFd == -1) {
            LOC_LOGe("epoll_create1 failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        mWorkerThread = std::thread([this]() { this->run(); });
        mWorkerThread.detach();
    }

    ~TimerEngine() {
        close(mEpollFd);
    }

    void run() {
        constexpr int MAX_EVENTS = 1;
        epoll_event events[MAX_EVENTS];

        pthread_setname_np(pthread_self(), "LocTimerThread");
        while (true) {
            int nfds = epoll_wait(mEpollFd, events, MAX_EVENTS, -1);
            if (nfds == -1) {
                LOC_LOGe("epoll_wait failed %s", strerror(errno));
                continue;
            }

            {
               std::lock_guard<std::recursive_mutex> lock(mMutex);
               for (int i = 0; i < nfds; ++i) {
                  LocTimer* timer = static_cast<LocTimer*>(events[i].data.ptr);
                  if (!timer) continue;

                  LOC_LOGd("timeoutcallback, timer name %s, timer-fd %d, nfds %d, i = %d",
                           timer->mName, timer->mFd, nfds, i);
                  uint64_t expirations;
                  read(timer->mFd, &expirations, sizeof(expirations)); // Clear event

                  if (timer->mIsRunning.load()) {
                     timer->timeOutCallback();
                  }
               }
            }
        }
    }

    int mEpollFd;
    std::map<int, LocTimer*> mTimers;
    std::recursive_mutex mMutex;
    std::thread mWorkerThread;
};

LocTimer::LocTimer() : mName("unnamed timer"), mFd(-1), mIsRunning(false) {
}

LocTimer::LocTimer(const char* name) : mName(name), mFd(-1), mIsRunning(false) {
   if (mName == nullptr) {
      mName = "unnamed timer";
   }
   LOC_LOGd("timer %s consturctor called", mName);
}

LocTimer::~LocTimer() {
    LOC_LOGd("timer %s desturctor called", mName);
    stop();
}

bool LocTimer::start(uint32_t timeoutMs, bool wakeOnExpire) {
    if (wakeOnExpire == true) {
       LOC_LOGd("alarm based timer %s not supported, will use soft timer",
                mName);
    }
    return TimerEngine::instance().registerTimer(this, timeoutMs);
}

bool LocTimer::stop() {
    return TimerEngine::instance().unregisterTimer(this);
}

bool LocTimer::isRunning() {
    return mIsRunning.load();
}

} // namespace loc_util
