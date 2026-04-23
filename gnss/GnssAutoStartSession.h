/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 *
 * GnssAutoStartSession.h
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
#ifndef GNSS_AUTO_START_SESSION_H
#define GNSS_AUTO_START_SESSION_H

#include "GnssAdapter.h"
#include "GnssAutoStartSessionClient.h"
#include <LocationDataTypes.h>
#include <log_util.h>
#include <loc_cfg.h>
#include <atomic>
#include <mutex>

namespace loc_core {

class GnssAutoStartSession {

public:
    GnssAutoStartSession(GnssAdapter& gnssAdapter);
    ~GnssAutoStartSession();

    bool init();
    void startSession();
    void onLpmStateChanged(bool isLpmEntry);
    void onModemRestart();
    void onSessionResponse(LocationError err, uint32_t sessionId);
    void onFinalFixReceived(const Location& location);
    void onBlankSvInfoReceived(const GnssSvNotification& svNotify);

    /**
     * Returns true if a tracking session is currently active.
     *
     * Thread-safe: acquires mSessionMutex before reading mSessionActive
     * so the returned value is consistent with startSession() /
     * stopSession() running concurrently on other threads.
     *
     * Typical callers: GnssAdapter diagnostic paths, unit tests.
     */
    bool isSessionActive() const;

private:
    // ----------------------------------------------------------------
    // Reference to owning GnssAdapter — used for direct HAL calls
    // ----------------------------------------------------------------
    GnssAdapter& mGnssAdapter;

    // ----------------------------------------------------------------
    // Sentinel client — provides a valid, stable LocationAPI* address
    // to pass as the 'client' parameter in startTrackingCommand /
    // stopTrackingCommand. All its callbacks are safe no-ops.
    // ----------------------------------------------------------------
    GnssAutoStartSessionClient mSentinelClient;

    // ----------------------------------------------------------------
    // Session state
    // ----------------------------------------------------------------
    TrackingOptions  mTrackingOptions;
    uint32_t         mSessionId     = 0;
    bool             mEnabled       = false;
    bool             mSessionActive = false;
    bool             mInLpm         = false;
    mutable std::mutex mSessionMutex;

    // ----------------------------------------------------------------
    // Retry state
    // ----------------------------------------------------------------
    uint32_t         mRetryCount    = 0;
    static constexpr uint32_t kMaxRetries      = 5;
    static constexpr uint32_t kRetryDelayMs    = 1000;  // 1 second between retries

    // ----------------------------------------------------------------
    // Modem unlock state
    // ----------------------------------------------------------------
    // Saved GPS lock value before we override it to NONE for auto-start.
    // Restored in stopSession() / onLpmStateChanged() so the modem lock
    // is left exactly as it was before auto-start touched it.
    GnssConfigGpsLock mSavedGpsLock  = GNSS_CONFIG_GPS_LOCK_NONE;
    bool              mModemUnlocked = false;  // true while we hold the unlock

    // ----------------------------------------------------------------
    // Config (read from gps.conf)
    // ----------------------------------------------------------------
    uint32_t         mTbfMs         = 1000;    // GNSS_SESSION_TBF_MS
    uint32_t         mTimeoutMs     = 60000;   // AUTO_SESSION_TIMEOUT

    // ----------------------------------------------------------------
    // Timeout state
    // ----------------------------------------------------------------
    // Absolute boot-time deadline (ms) by which a valid fix must arrive.
    // Set in startSession() after init() has read mTimeoutMs from gps.conf.
    // Reset to 0 in stopSession(). Checked in onBlankSvInfoReceived() on
    // every SV event with svNotify.count < 4 (no meaningful signal yet).
    uint64_t         mSessionTimeOut = 0;

    void buildTrackingOptions();
    void unlockModemForAutoStart();  // called only from startSession()
    void restoreModemLock();         // called only from stopSession()
    uint64_t timeTickfromBootup();

    // scheduleRetry() acquires mSessionMutex internally.
    // MUST NOT be called while mSessionMutex is already held — doing so
    // will deadlock. Current callers: onSessionResponse() only, which
    // releases mSessionMutex before calling here.
    void scheduleRetry();

    void stopSession();
};

} // namespace loc_core

#endif /* GNSS_AUTO_START_SESSION_H */
#endif /* FEATURE_AUTO_SESSION */
