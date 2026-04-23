/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * GnssAutoStartSession.cpp
 *
 * Purpose:
 *   The main class for the Gnss Auto Start session, this object
 *   owns a GnssAutoStartSessionClient sentinel instance and passes
 *   its address into startTrackingCommand / stopTrackingCommand.
 *
 *   All virtual callback methods are overridden as safe no-ops so that
 *   GnssAdapter's internal reportPosition() / reportResponse() loops
 *   never dereference a null or invalid pointer.
 *
 * AUTORFI: AUTORFI-54961
 */
#ifdef FEATURE_AUTO_SESSION
#include "GnssAutoStartSession.h"
#include <log_util.h>
#include <loc_cfg.h>
#include <thread>
#include <chrono>

#ifdef __ANDROID__
#include <utils/SystemClock.h>
#include <utils/Timers.h>
#endif

namespace loc_core {

// -----------------------------------------------------------------------
// gps.conf parameters
// -----------------------------------------------------------------------
static uint32_t sAutoStartGnss    = 0;
static uint32_t sGnssSessionTbfMs = 1000;
static uint32_t sAutoSessionTimeout = 60000;

static const loc_param_s_type gAutoStartParams[] = {
    { "AUTO_START_GNSS",      &sAutoStartGnss,       nullptr, 'n' },
    { "GNSS_SESSION_TBF_MS",  &sGnssSessionTbfMs,    nullptr, 'n' },
    { "AUTO_SESSION_TIMEOUT", &sAutoSessionTimeout,  nullptr, 'n' },
};

// -----------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------

GnssAutoStartSession::GnssAutoStartSession(GnssAdapter& gnssAdapter)
    : mGnssAdapter(gnssAdapter),
      mSentinelClient(this)       // pass owner so sentinel can call onSessionResponse()
{
    LOC_LOGd("[AutoStart] GnssAutoStartSession constructed. "
             "Sentinel client @ %p", &mSentinelClient);
}

GnssAutoStartSession::~GnssAutoStartSession() {
    stopSession();
}

// -----------------------------------------------------------------------
// init() — read gps.conf, build TrackingOptions
// -----------------------------------------------------------------------

bool GnssAutoStartSession::init() {
    UTIL_READ_CONF(
        LOC_PATH_GPS_CONF,
        gAutoStartParams
    );

    mEnabled  = (sAutoStartGnss == 1);
    mTbfMs    = sGnssSessionTbfMs;
    mTimeoutMs = sAutoSessionTimeout;

    if (!mEnabled) {
        LOC_LOGi("[AutoStart] AUTO_START_GNSS=0 — auto-start disabled.");
        return false;
    }

    buildTrackingOptions();

    LOC_LOGi("[AutoStart] init() OK. TBF=%u ms, Timeout=%u ms, Sentinel=%p",
             mTbfMs, mTimeoutMs, &mSentinelClient);

    return true;
}

// -----------------------------------------------------------------------
// buildTrackingOptions
// -----------------------------------------------------------------------

void GnssAutoStartSession::buildTrackingOptions() {
    memset(&mTrackingOptions, 0, sizeof(mTrackingOptions));
    mTrackingOptions.size        = sizeof(TrackingOptions);
    mTrackingOptions.minInterval = mTbfMs;
    mTrackingOptions.mode        = GNSS_SUPL_MODE_STANDALONE;
}

// -----------------------------------------------------------------------
// unlockModemForAutoStart()
//   Reads the current GPS lock from gps.conf, saves it, then sends
//   GNSS_CONFIG_GPS_LOCK_NONE to the modem via setGpsLockSync() so that
//   the engine is free to start a tracking session.
//
//   Must be called from the GnssAdapter worker thread (inside sendMsg)
//   because setGpsLockSync() is a synchronous QMI call.
// -----------------------------------------------------------------------

void GnssAutoStartSession::unlockModemForAutoStart() {
    if (mModemUnlocked) {
        LOC_LOGd("[AutoStart] unlockModemForAutoStart: already unlocked, skipping.");
        return;
    }

    // Save the current configured lock so we can restore it later
    mSavedGpsLock = ContextBase::mGps_conf.GPS_LOCK;

    LOC_LOGi("[AutoStart] unlockModemForAutoStart: "
             "savedLock=0x%x — sending GNSS_CONFIG_GPS_LOCK_NONE to modem.",
             static_cast<uint32_t>(mSavedGpsLock));

    // Send unlock synchronously on the QMI thread via LocApiBase
    LocationError err = mGnssAdapter.getLocApi()->setGpsLockSync(
                            GNSS_CONFIG_GPS_LOCK_NONE);

    if (err == LOCATION_ERROR_SUCCESS) {
        mModemUnlocked = true;
        LOC_LOGi("[AutoStart] unlockModemForAutoStart: modem unlocked OK.");
    } else {
        LOC_LOGe("[AutoStart] unlockModemForAutoStart: "
                 "setGpsLockSync failed err=%u — proceeding anyway.",
                 static_cast<uint32_t>(err));
        // Still attempt startSession even if unlock failed;
        // the modem may already be unlocked.
    }
}

// -----------------------------------------------------------------------
// restoreModemLock()
//   Restores the GPS lock to the value saved before auto-start unlocked
//   the modem. Called from stopSession() and onLpmStateChanged().
// -----------------------------------------------------------------------

void GnssAutoStartSession::restoreModemLock() {
    if (!mModemUnlocked) {
        LOC_LOGd("[AutoStart] restoreModemLock: modem was not unlocked by us, skipping.");
        return;
    }

    LOC_LOGi("[AutoStart] restoreModemLock: "
             "restoring GPS lock to 0x%x.",
             static_cast<uint32_t>(mSavedGpsLock));

    LocationError err = mGnssAdapter.getLocApi()->setGpsLockSync(mSavedGpsLock);

    if (err == LOCATION_ERROR_SUCCESS) {
        mModemUnlocked = false;
        LOC_LOGi("[AutoStart] restoreModemLock: GPS lock restored OK.");
    } else {
        LOC_LOGe("[AutoStart] restoreModemLock: "
                 "setGpsLockSync failed err=%u.",
                 static_cast<uint32_t>(err));
    }
}

// -----------------------------------------------------------------------
// startSession() — passes &mSentinelClient as the LocationAPI* client
// -----------------------------------------------------------------------

void GnssAutoStartSession::startSession() {
    std::lock_guard<std::mutex> lock(mSessionMutex);

    if (!mEnabled) {
        LOC_LOGd("[AutoStart] startSession skipped — not enabled.");
        return;
    }
    if (mSessionActive) {
        LOC_LOGd("[AutoStart] startSession skipped — already active (id=%u).",
                 mSessionId);
        return;
    }
    if (mInLpm) {
        LOC_LOGw("[AutoStart] startSession skipped — device in LPM.");
        return;
    }

    LOC_LOGi("[AutoStart] Starting session via mGnssAdapter.startTrackingCommand, "
             "sentinel=%p TBF=%u ms", &mSentinelClient, mTbfMs);

    // -----------------------------------------------------------------
    // Step 1: Unlock the modem before sending the start request.
    //   The GNSS engine may be locked (GPS_LOCK != NONE) at boot time
    //   which causes startTimeBasedTracking to fail with ENGINE_BUSY.
    //   We save the current lock, set it to NONE, then start the session.
    //   The lock is restored in stopSession() / onLpmStateChanged().
    // -----------------------------------------------------------------
    unlockModemForAutoStart();

    // Add internal client to GnssAdapter client map
    mGnssAdapter.addClientCommand((LocationAPI*) &mSentinelClient, mSentinelClient.getLocationCallbacks());

    // -----------------------------------------------------------------
    // SAFE: &mSentinelClient is a valid non-null LocationAPI* pointer.
    //    GnssAdapter will:
    //      1. Store &mSentinelClient as the map key in mClientData.
    //      2. Call mSentinelClient.reportResponse() on the worker thread
    //         → safe no-op.
    //      3. Call mSentinelClient.reportLocationEvent() on each fix
    //         → safe no-op (modem warm-up is the goal, not consuming fixes here).
    // -----------------------------------------------------------------
    mSessionId = mGnssAdapter.startTrackingCommand(
        (LocationAPI*) &mSentinelClient,   // Valid sentinel client — shall NOT be nullptr
        mTrackingOptions
    );

    // Log Boot KPI
    int retMarker = loc_boot_kpi_marker("L - GNSS Auto-Start Session - Start");
    if (retMarker < 0) {
       ALOGI("%s]: KPI marker failed with error %d", __FUNCTION__, retMarker);
    }

    mSessionActive  = true;
    // Arm the timeout deadline from the moment the session becomes active.
    // mTimeoutMs is guaranteed valid here because init() has already run.
    mSessionTimeOut = timeTickfromBootup() + mTimeoutMs;

    LOC_LOGi("[AutoStart] startTrackingCommand dispatched to worker thread. "
             "Session now active. Timeout deadline in %u ms.", mTimeoutMs);
}

// -----------------------------------------------------------------------
// isSessionActive() — returns whether a tracking session is currently
//   active. Thread-safe: acquires mSessionMutex before reading
//   mSessionActive so the result is consistent with concurrent calls to
//   startSession() and stopSession() on other threads.
// -----------------------------------------------------------------------

bool GnssAutoStartSession::isSessionActive() const {
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return mSessionActive;
}

// -----------------------------------------------------------------------
// stopSession() — symmetric use of &mSentinelClient
// -----------------------------------------------------------------------

void GnssAutoStartSession::stopSession() {
    std::lock_guard<std::mutex> lock(mSessionMutex);

    if (!mSessionActive) {
        LOC_LOGd("[AutoStart] stopSession skipped — no active session.");
        return;
    }

    LOC_LOGi("[AutoStart] Stopping session. sentinel=%p id=%u",
             &mSentinelClient, mSessionId);

    // -----------------------------------------------------------------
    // ✅ SAFE: Same &mSentinelClient pointer used at start.
    //    GnssAdapter will:
    //      1. Look up mClientData[&mSentinelClient] → finds the entry.
    //      2. Call mSentinelClient.reportResponse() → safe no-op.
    //      3. Erase the entry cleanly from the map.
    // -----------------------------------------------------------------
    mGnssAdapter.stopTrackingCommand(
        ((LocationAPI*) &mSentinelClient),   // ✅ Same valid sentinel pointer — shall NOT be nullptr
        mSessionId
    );

    mSessionActive = false;
    mSessionId     = 0;
    mSessionTimeOut = 0;

    // -----------------------------------------------------------------
    // Restore the GPS lock that was saved before we unlocked the modem.
    // This ensures the modem lock state is not permanently altered by
    // the auto-start session.
    // -----------------------------------------------------------------
    restoreModemLock();

    LOC_LOGi("[AutoStart] stopTrackingCommand dispatched. Session cleared.");
}

// -----------------------------------------------------------------------
// onFinalFixReceived() — called by sentinel's trackingCb when a location
//   report carries a valid lat/long GNSS fix. Stops the session.
// -----------------------------------------------------------------------

void GnssAutoStartSession::onFinalFixReceived(const Location& location) {
    LOC_LOGi("[AutoStart] onFinalFixReceived: "
             "lat=%.6f lon=%.6f acc=%.1fm tech=0x%x — stopping session.",
             location.latitude, location.longitude,
             location.accuracy,
             static_cast<uint32_t>(location.techMask));

    // Log Boot KPI
    int retMarker = loc_boot_kpi_marker("L - GNSS Auto-Start Session - Stop by VALID_FIX");
    if (retMarker < 0) {
        ALOGI("%s]: KPI marker failed with error %d", __FUNCTION__, retMarker);
    }

    stopSession();
}

// -----------------------------------------------------------------------
// onBlankSvInfoReceived() — timeout watchdog driven by SV events.
//
//   Called by GnssAutoStartSessionClient::reportGnssSvEvent() whenever
//   the GNSS engine reports fewer than 4 satellites in view, which
//   indicates the engine has not yet acquired enough signal to compute
//   a position fix.
//
//   Purpose:
//     Since the timer module has been removed, this function serves as
//     the timeout enforcement point. Each low-SV-count event is used as
//     a periodic heartbeat to check whether the session has been running
//     longer than AUTO_SESSION_TIMEOUT (mTimeoutMs) milliseconds without
//     producing a valid fix.
//
//   Timeout check:
//     mSessionTimeOut holds the absolute boot-time deadline (ms) set in
//     startSession(). If the current boot-time clock has passed that
//     deadline, the session is stopped.
//
//     The mSessionTimeOut != 0 guard prevents a spurious stop before the
//     first session is started (mSessionTimeOut initialises to 0) or
//     after stopSession() resets it to 0.
//
//   Normal exit path:
//     If a valid fix arrives before the deadline, onFinalFixReceived()
//     calls stopSession() first. stopSession() sets mSessionTimeOut = 0,
//     so any subsequent SV event that reaches this function will see
//     mSessionTimeOut == 0 and return immediately without double-stopping.
// -----------------------------------------------------------------------
void GnssAutoStartSession::onBlankSvInfoReceived(const GnssSvNotification& svNotify) {
    if (mSessionTimeOut != 0 && mSessionTimeOut < timeTickfromBootup()) {
        LOC_LOGw("[AutoStart] onBlankSvInfoReceived: AUTO_SESSION_TIMEOUT expired "
                 "with no valid fix (svCount=%d) — stopping session.",
                 svNotify.count);
        // Log Boot KPI
        int retMarker = loc_boot_kpi_marker("L - GNSS Auto-Start Session - Stop by TIMEOUT");
        if (retMarker < 0) {
            ALOGI("%s]: KPI marker failed with error %d", __FUNCTION__, retMarker);
        }
        stopSession();
    }
}
// -----------------------------------------------------------------------
// onSessionResponse() — called by sentinel's reportResponse() callback
//   err == LOCATION_ERROR_SUCCESS  → session started OK, nothing to do
//   err != LOCATION_ERROR_SUCCESS  → engine was busy; schedule a retry
// -----------------------------------------------------------------------

void GnssAutoStartSession::onSessionResponse(LocationError err,
                                              uint32_t sessionId) {
    if (err == LOCATION_ERROR_SUCCESS) {
        LOC_LOGi("[AutoStart] onSessionResponse: id=%u SUCCESS — session running.",
                 sessionId);
        // Session started successfully — reset the retry counter so that
        // any future failure starts a fresh retry sequence from attempt 1.
        std::lock_guard<std::mutex> lock(mSessionMutex);
        mRetryCount = 0;
        return;
    }

    // Session failed — reset active flag so startSession() can run again
    {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        mSessionActive = false;
        mSessionId     = 0;
    }

    LOC_LOGw("[AutoStart] onSessionResponse: id=%u err=%u — scheduling retry.",
             sessionId, static_cast<uint32_t>(err));

    scheduleRetry();
}

// -----------------------------------------------------------------------
// scheduleRetry() — spawns a detached thread that waits kRetryDelayMs
//   then calls startSession() again, up to kMaxRetries times.
//
//   Why detached thread?
//     startSession() must NOT be called from the GnssAdapter worker
//     thread (it would deadlock on mSessionMutex held by the caller).
//     A short-lived detached thread is the simplest safe approach here.
// -----------------------------------------------------------------------

void GnssAutoStartSession::scheduleRetry() {
    std::lock_guard<std::mutex> lock(mSessionMutex);

    if (!mEnabled || mInLpm) {
        LOC_LOGw("[AutoStart] scheduleRetry: skipped — enabled=%d inLpm=%d",
                 mEnabled, mInLpm);
        return;
    }

    if (mRetryCount >= kMaxRetries) {
        LOC_LOGe("[AutoStart] scheduleRetry: max retries (%u) reached — giving up.",
                 kMaxRetries);
        return;
    }

    mRetryCount++;
    uint32_t attempt = mRetryCount;

    LOC_LOGi("[AutoStart] scheduleRetry: attempt %u/%u in %u ms.",
             attempt, kMaxRetries, kRetryDelayMs);

    // Capture 'this' — safe because GnssAutoStartSession lifetime is
    // tied to GnssAdapter which outlives this thread.
    std::thread([this, attempt]() {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(kRetryDelayMs));

        LOC_LOGi("[AutoStart] retry thread: attempt %u — calling startSession().",
                 attempt);
        startSession();
    }).detach();
}

// -----------------------------------------------------------------------
// LPM state change handler
// -----------------------------------------------------------------------

void GnssAutoStartSession::onLpmStateChanged(bool isLpmEntry) {
    LOC_LOGi("[AutoStart] onLpmStateChanged: isLpmEntry=%d", isLpmEntry);
    mInLpm = isLpmEntry;
    if (isLpmEntry) {
        // Stop session before platform suspend to avoid blocking wakelock
        stopSession();
    }
}

// -----------------------------------------------------------------------
// Modem SSR restart handler
// -----------------------------------------------------------------------

void GnssAutoStartSession::onModemRestart() {
    LOC_LOGi("[AutoStart] onModemRestart: resetting session state for re-trigger.");
    std::lock_guard<std::mutex> lock(mSessionMutex);
    // Reset state — GnssAdapter will call startSession() again after
    // modem comes back up via onModemUpEvent.
    mSessionActive = false;
    mSessionId     = 0;
}


// -----------------------------------------------------------------------
// timeTickfromBootup() — returns current boot-time clock in milliseconds.
//   Used to compute and check the session timeout deadline.
// -----------------------------------------------------------------------
uint64_t GnssAutoStartSession::timeTickfromBootup(void) {
  #ifdef __ANDROID__
    return nanoseconds_to_milliseconds(android::elapsedRealtimeNano());
  #else
    struct timespec ts;
    uint64_t time_ms = 0;
    clock_gettime(CLOCK_BOOTTIME, &ts);

    time_ms += (ts.tv_sec * 1000LL);     // Seconds to milliseconds
    time_ms += ts.tv_nsec / 1000000LL;   // Nanoseconds to milliseconds

    return time_ms;
  #endif
}

} // namespace loc_core
#endif /* FEATURE_AUTO_SESSION */
