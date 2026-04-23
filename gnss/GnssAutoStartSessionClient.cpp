/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * GnssAutoStartSessionClient.cpp
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
#include <LocationAPI.h>     // Base class: LocationAPI
#include <ILocationAPI.h>
#include <LocationDataTypes.h>
#include <log_util.h>        // LOC_LOGd / LOC_LOGe / LOC_LOGi
#include "GnssAutoStartSessionClient.h"
#include "GnssAutoStartSession.h"  // for onSessionResponse()

namespace loc_core {

GnssAutoStartSessionClient::GnssAutoStartSessionClient(GnssAutoStartSession* owner)
    : LocationAPI(), mOwner(owner) {
    memset(&mLocationCallbacks, 0, sizeof(LocationCallbacks));
};

GnssAutoStartSessionClient::~GnssAutoStartSessionClient() {
};

// -----------------------------------------------------------------------
// reportResponse() — forwards result to owner for retry logic
// -----------------------------------------------------------------------
void GnssAutoStartSessionClient::reportResponse(LocationError err,
                                                 uint32_t id) const {
    if ((mOwner != nullptr) && (mOwner->isSessionActive())) {
        LOC_LOGd("[AutoStartSentinel] reportResponse: id=%u err=%u",
             id, static_cast<uint32_t>(err));

        if (mOwner != nullptr) {
            mOwner->onSessionResponse(err, id);
        }
    }
}

// -----------------------------------------------------------------------
// reportLocationEvent() — inspect fix quality; stop session on 1st final fix
// -----------------------------------------------------------------------
void GnssAutoStartSessionClient::reportLocationEvent(
        const Location& location) const {

    if ((mOwner != nullptr) && (mOwner->isSessionActive())) {
        LOC_LOGd("[AutoStartSentinel] reportLocationEvent: "
             "lat=%.6f lon=%.6f flags=0x%x tech=0x%x",
             location.latitude, location.longitude,
             static_cast<uint32_t>(location.flags),
             static_cast<uint32_t>(location.techMask));

        // A final fix must have:
        //   1. Valid lat/long  — LOCATION_HAS_LAT_LONG_BIT
        //   2. GNSS technology — LOCATION_TECHNOLOGY_GNSS_BIT
        const bool hasLatLong = (location.flags & LOCATION_HAS_LAT_LONG_BIT) != 0;
        const bool isGnssFix  = (location.techMask & LOCATION_TECHNOLOGY_GNSS_BIT) != 0;

        if (hasLatLong && isGnssFix) {
            LOC_LOGi("[AutoStartSentinel] reportLocationEvent: "
                 "final GNSS fix detected — notifying owner.");
            if (mOwner != nullptr) {
                 mOwner->onFinalFixReceived(location);
            }
        } else {
            LOC_LOGd("[AutoStartSentinel] reportLocationEvent: "
                 "intermediate/invalid fix — hasLatLong=%d isGnssFix=%d, waiting.",
                 hasLatLong, isGnssFix);
        }
    }
}

// -----------------------------------------------------------------------
// reportGnssSvEvent() — check timeout on low SV count
// -----------------------------------------------------------------------
void GnssAutoStartSessionClient::reportGnssSvEvent(
        const GnssSvNotification& svNotify) const {

    if ((mOwner != nullptr) && (mOwner->isSessionActive())) {
        LOC_LOGd("[AutoStartSentinel] reportGnssSvEvent: %d", svNotify.count);

        // heartbeat to check whether the session timeout has expired.
        mOwner->onBlankSvInfoReceived(svNotify);
    }
}

// -----------------------------------------------------------------------
// reportCapabilitiesEvent()
// -----------------------------------------------------------------------
void GnssAutoStartSessionClient::reportCapabilitiesEvent(LocationCapabilitiesMask mask) const {
    if ((mOwner != nullptr) && (mOwner->isSessionActive())) {
        LOC_LOGd("[AutoStartSentinel] reportCapabilitiesEvent: mask=0x%x",
            static_cast<uint32_t>(mask));
    }
    (void)mask;
} //reportCapabilitiesEvent

// -----------------------------------------------------------------------
// reportCollectiveResponseEvent()
// -----------------------------------------------------------------------
void GnssAutoStartSessionClient::reportCollectiveResponseEvent(
    size_t count, LocationError* errs, uint32_t* ids) const {
            (void)count;
            (void)errs;
            (void)ids;
  // Deliberate no-op.
} //reportCollectiveResponseEvent

// -----------------------------------------------------------------------
// getLocationCallbacks()
// -----------------------------------------------------------------------
LocationCallbacks& GnssAutoStartSessionClient::getLocationCallbacks() {

    mLocationCallbacks.capabilitiesCb = [this](LocationCapabilitiesMask mask) {
        reportCapabilitiesEvent(mask);
    };

    mLocationCallbacks.responseCb = [this] (LocationError err, uint32_t id) {
        reportResponse(err, id);
    };

    mLocationCallbacks.collectiveResponseCb = [this](size_t count, LocationError* errs, uint32_t* ids) {
        reportCollectiveResponseEvent(count, errs, ids);
    };

    //tracking callback
    mLocationCallbacks.trackingCb = [this](Location location) {
        reportLocationEvent(location);
    };

    mLocationCallbacks.gnssSvCb = [this] (const GnssSvNotification& svNotify) {
        reportGnssSvEvent(svNotify);
    };

    return mLocationCallbacks;

} //getLocationCallbacks


} // namespace loc_core
#endif /* FEATURE_AUTO_SESSION */
