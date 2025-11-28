/*
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_BatchingAPIClient"

#include <inttypes.h>
#include <log_util.h>
#include <loc_cfg.h>
#include <thread>
#include "LocationUtil.h"
#include "BatchingAPIClient.h"
#include "GnssAPIClient.h"

#include "limits.h"

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

using ::aidl::android::hardware::gnss::IGnssBatching;
using ::aidl::android::hardware::gnss::IGnssBatchingCallback;

static void convertBatchOption(const IGnssBatching::Options& in, LocationOptions& out,
        LocationCapabilitiesMask mask);

BatchingAPIClient::BatchingAPIClient(const shared_ptr<IGnssBatchingCallback>& callback) :
    LocationAPIClientBase(),
    mGnssBatchingCbIface(nullptr),
    mDefaultId(UINT_MAX),
    mLocationCapabilitiesMask(0) {
    LOC_LOGd("]: (%p)", &callback);
    gnssUpdateCallbacks(callback);
}

BatchingAPIClient::~BatchingAPIClient() {
    LOC_LOGd("]: ()");
}

int BatchingAPIClient::getBatchSize() {
    int batchSize = locAPIGetBatchSize();
    LOC_LOGd("batchSize: %d", batchSize);
    return batchSize;
}

void BatchingAPIClient::setCallbacks() {
    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);
    locationCallbacks.batchingCb = [this](size_t count, Location* location,
        const BatchingOptions& batchOptions) {
        onBatchingCb(count, location, batchOptions);
    };

    locAPISetCallbacks(locationCallbacks);
}

void BatchingAPIClient::gnssUpdateCallbacks(const shared_ptr<IGnssBatchingCallback>& callback) {
    gSharedMtx.lock();
    bool cbWasNull = (mGnssBatchingCbIface == nullptr);
    mGnssBatchingCbIface = callback;
    gSharedMtx.unlock();

    if (cbWasNull) {
        setCallbacks();
    }
}

int BatchingAPIClient::startSession(const IGnssBatching::Options& opts) {
    gSharedMtx.lock();
    mState = STARTED;
    gSharedMtx.unlock();
    LOC_LOGd("]: (%lld %d)",
            static_cast<long long>(opts.periodNanos), static_cast<uint8_t>(opts.flags));
    int retVal = -1;
    LocationOptions options;
    convertBatchOption(opts, options, mLocationCapabilitiesMask);
    uint32_t mode = 0;
    if (opts.flags == static_cast<uint8_t>(IGnssBatching::WAKEUP_ON_FIFO_FULL)) {
        mode = SESSION_MODE_ON_FULL;
    }
    if (locAPIStartSession(mDefaultId, mode, options) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

int BatchingAPIClient::updateSessionOptions(const IGnssBatching::Options& opts) {
    LOC_LOGd("]: (%lld %d)",
            static_cast<long long>(opts.periodNanos), static_cast<uint8_t>(opts.flags));
    int retVal = -1;
    LocationOptions options;
    convertBatchOption(opts, options, mLocationCapabilitiesMask);

    uint32_t mode = 0;
    if (opts.flags == static_cast<uint8_t>(IGnssBatching::WAKEUP_ON_FIFO_FULL)) {
        mode = SESSION_MODE_ON_FULL;
    }
    if (locAPIUpdateSessionOptions(mDefaultId, mode, options) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

int BatchingAPIClient::stopSession() {
    gSharedMtx.lock();
    if (mState != STARTED) {
        LOC_LOGe("] Error Stop called without start");
        gSharedMtx.unlock();
        return -1;
    }
    mState = STOPPING;
    gSharedMtx.unlock();
    LOC_LOGd("]: ");
    int retVal = -1;
    locAPIGetBatchedLocations(mDefaultId, SIZE_MAX);
    if (locAPIStopSession(mDefaultId) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

void BatchingAPIClient::getBatchedLocation(int last_n_locations) {
    LOC_LOGd("]: (%d)", last_n_locations);
    locAPIGetBatchedLocations(mDefaultId, last_n_locations);
}

void BatchingAPIClient::flushBatchedLocations() {
    LOC_LOGd("]: ()");
    uint32_t retVal = locAPIGetBatchedLocations(mDefaultId, SIZE_MAX);
    // when flush a stopped session or one doesn't exist, just report an empty batch.
    if (LOCATION_ERROR_ID_UNKNOWN == retVal) {
        BatchingOptions opt = {};
        ::std::thread thd(&BatchingAPIClient::onBatchingCb, this, 0, nullptr, opt);
        thd.detach();
    }
}

void BatchingAPIClient::onCapabilitiesCb(LocationCapabilitiesMask capabilitiesMask) {
    LOC_LOGd("]: (%" PRIu64 ")", capabilitiesMask);
    mLocationCapabilitiesMask = capabilitiesMask;
}

void BatchingAPIClient::onBatchingCb(size_t count, Location* location,
        const BatchingOptions& /*batchOptions*/) {
    bool processReport = false;
    LOC_LOGd("(count: %zu)", count);
    gSharedMtx.lock();
    // back to back stop() and flush() could bring twice onBatchingCb(). Each one might come first.
    // Combine them both (the first goes to cache, the second in location*) before report to FW
    switch (mState) {
        case STOPPING:
            mState = STOPPED;
            for (size_t i = 0; i < count; i++) {
                mBatchedLocationInCache.push_back(location[i]);
            }
            break;
        case STARTED:
        case STOPPED: // flush() always trigger report, even on a stopped session
            processReport = true;
            break;
        default:
            break;
    }
    // report location batch when in STARTED state or flush(), combined with cache in last stop()
    if (processReport) {
        auto gnssBatchingCbIface(mGnssBatchingCbIface);
        size_t batchCacheCnt = mBatchedLocationInCache.size();
        LOC_LOGd("(batchCacheCnt: %zu)", batchCacheCnt);
        if (gnssBatchingCbIface != nullptr) {
            std::vector<GnssLocation> locationVec;
            if (count+batchCacheCnt > 0) {
                locationVec.resize(count+batchCacheCnt);
                for (size_t i = 0; i < batchCacheCnt; ++i) {
                    convertGnssLocation(mBatchedLocationInCache[i], locationVec[i]);
                }
                for (size_t i = 0; i < count; i++) {
                    convertGnssLocation(location[i], locationVec[i+batchCacheCnt]);
                }
            }
            gSharedMtx.unlock();
            auto r = gnssBatchingCbIface->gnssLocationBatchCb(locationVec);
            if (!r.isOk()) {
                LOC_LOGe("] Error from gnssLocationBatchCb");
            }
        } else {
            gSharedMtx.unlock();
        }
        gSharedMtx.lock();
        mBatchedLocationInCache.clear();
        gSharedMtx.unlock();
    } else {
        gSharedMtx.unlock();
    }
}

static void convertBatchOption(const IGnssBatching::Options& in, LocationOptions& out,
        LocationCapabilitiesMask mask) {
    memset(&out, 0, sizeof(LocationOptions));
    out.size = sizeof(LocationOptions);
    out.minInterval = (uint32_t)(in.periodNanos / 1000000L);
    out.mode = GNSS_SUPL_MODE_STANDALONE;
    if (mask & LOCATION_CAPABILITIES_GNSS_MSA_BIT)
        out.mode = GNSS_SUPL_MODE_MSA;
    if (mask & LOCATION_CAPABILITIES_GNSS_MSB_BIT)
        out.mode = GNSS_SUPL_MODE_MSB;
}

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
