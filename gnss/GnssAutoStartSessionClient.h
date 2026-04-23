/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * GnssAutoStartSessionClient.h
 *
 * Purpose:
 *   Lightweight sentinel LocationAPI subclass used exclusively by
 *   GnssAutoStartSession to satisfy the LocationAPI* client parameter
 *   required by GnssAdapter::startTrackingCommand() and
 *   GnssAdapter::stopTrackingCommand().
 *
 *   All virtual callback methods are overridden as safe no-ops so that
 *   GnssAdapter's internal reportPosition() / reportResponse() loops
 *   never dereference a null or invalid pointer.
 *
 * AUTORFI: AUTORFI-54961
 */

#ifdef FEATURE_AUTO_SESSION
#ifndef GNSS_AUTO_START_SESSION_CLIENT_H
#define GNSS_AUTO_START_SESSION_CLIENT_H

#include <LocationAPI.h>     // Base class: LocationAPI
#include <ILocationAPI.h>
#include <LocationDataTypes.h>
#include <log_util.h>        // LOC_LOGd / LOC_LOGe / LOC_LOGi

namespace loc_core {

class GnssAutoStartSession;   // forward declaration

/**
 * @class GnssAutoStartSessionClient
 *
 * @brief Sentinel LocationAPI client for the internal GNSS auto-start session.
 *
 * Why this exists
 * ---------------
 * GnssAdapter::startTrackingCommand(LocationAPI* client, ...) stores the
 * client pointer as a KEY in its internal mClientData map and later
 * dereferences it during:
 *   - reportPosition()  → client->reportLocationEvent(...)
 *   - reportResponse()  → client->reportResponse(...)
 *   - stopClientSessions() → iterates map and calls per-client callbacks
 *
 * Passing nullptr causes:
 *   1. Silent map lookup failure  (session never stored/found)
 *   2. Segfault on callback dispatch  (nullptr dereference)
 *
 * This sentinel provides a real, stable object address that is safe to
 * store as a map key and safe to dereference — all callbacks are no-ops
 * that simply log at DEBUG level.
 *
 * Ownership
 * ---------
 *   Owned as a member of GnssAutoStartSession (not heap-allocated
 *   externally). Lifetime is tied to GnssAdapter.
 */
class GnssAutoStartSessionClient : public LocationAPI {

private:
    LocationCallbacks  mLocationCallbacks;
    GnssAutoStartSession* mOwner;   // back-pointer to call onSessionResponse()

public:
    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------
    explicit GnssAutoStartSessionClient(GnssAutoStartSession* owner);
    ~GnssAutoStartSessionClient();

    // Non-copyable, non-movable — address stability is critical because
    // it is used as a map key inside GnssAdapter.
    GnssAutoStartSessionClient(const GnssAutoStartSessionClient&)            = delete;
    GnssAutoStartSessionClient& operator=(const GnssAutoStartSessionClient&) = delete;
    GnssAutoStartSessionClient(GnssAutoStartSessionClient&&)                 = delete;
    GnssAutoStartSessionClient& operator=(GnssAutoStartSessionClient&&)      = delete;

    // ------------------------------------------------------------------
    // LocationAPI virtual callback overrides — all safe no-ops
    // ------------------------------------------------------------------

    /**
     * Called by GnssAdapter::reportResponse() after startTracking /
     * stopTracking commands are processed on the worker thread.
     * Forwards the result to GnssAutoStartSession::onSessionResponse()
     * so that a retry can be scheduled when err != SUCCESS.
     */
    void reportResponse(LocationError err, uint32_t id) const;

    /**
     * Called by GnssAdapter during reportPosition() for each registered
     * client. Inspects the location fix quality and on the first valid
     * final GNSS fix, notifies the owner to stop the session.
     */
    void reportLocationEvent(const Location& location) const;

    /**
     * Called when GNSS capabilities are reported. No-op for sentinel.
     */
    void reportCapabilitiesEvent(LocationCapabilitiesMask mask) const;

    /**
     * Called on GNSS SV status update.
     * Forwards to owner's onBlankSvInfoReceived() when svNotify.count < 4
     * (fewer than 4 satellites in view means no meaningful signal yet),
     * so the owner can check whether the session timeout has expired.
     */
    void reportGnssSvEvent(const GnssSvNotification& svNotify) const;


    /**
     * Called on NMEA sentence availability. No-op for sentinel.
     */
    void reportNmeaEvent(GnssNmeaNotification& nmeaNotify) const {
        (void)nmeaNotify;
        // Deliberate no-op.
    }

    /**
     * Called when a geofence breach occurs. No-op for sentinel.
     * Auto-start session does not use geofencing.
     */
    void reportGeofenceBreachEvent(GeofenceBreachNotification& breachNotify) const {
        (void)breachNotify;
        // Deliberate no-op.
    }

    /**
     * Called when geofence status changes. No-op for sentinel.
     */
    void reportGeofenceStatusEvent(GeofenceStatusNotification& statusNotify) const {
        (void)statusNotify;
        // Deliberate no-op.
    }

    /**
     * Called on batching. No-op for sentinel (not using batching).
     */
    void reportBatchingEvent(BatchingStatusInfo& batchNotify) const {
        (void)batchNotify;
        // Deliberate no-op.
    }

    /**
     * Called for collective geofence responses. No-op for sentinel.
     */
    void reportCollectiveResponseEvent(
            size_t count,
            LocationError* errs,
            uint32_t* ids) const ;

    LocationCallbacks& getLocationCallbacks();

}; //GnssAutoStartSessionClient

} // namespace loc_core

#endif /* GNSS_AUTO_START_SESSION_CLIENT_H */
#endif /* FEATURE_AUTO_SESSION */
