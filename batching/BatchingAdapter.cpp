/* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_BatchingAdapter"

#include <loc_pla.h>
#include <log_util.h>
#include <LocContext.h>
#include <BatchingAdapter.h>

using namespace loc_core;

BatchingAdapter::BatchingAdapter() :
    LocAdapterBase(0,
                   LocContext::getLocContext(LocContext::mLocationHalName),
                   false, nullptr, true),
    mBatchingTimeout(0),
    mBatchingAccuracy(1),
    mBatchSize(20),
    mSystemPowerState(POWER_STATE_UNKNOWN) {
    LOC_LOGD("%s]: Constructor", __func__);
    const loc_param_s_type batching_conf_param_table[] =
    {
        {"BATCH_SIZE", &mBatchSize, NULL, 'n'},
        {"BATCH_SESSION_TIMEOUT", &mBatchingTimeout, NULL, 'n'},
        {"ACCURACY", &mBatchingAccuracy, NULL, 'n'},
    };
    UTIL_READ_CONF(LOC_PATH_IZAT_CONF, batching_conf_param_table);

    LOC_LOGd("batchSize %u batchingAccuracy %u batchingTimeout %u ",
            mBatchSize, mBatchingAccuracy, mBatchingTimeout);

    // at last step, let us inform adapater base that we are done
    // with initialization, e.g.: ready to process handleEngineUpEvent
    doneInit();
}

void
BatchingAdapter::stopClientSessions(LocationAPI* client, bool eraseSession) {
    LOC_LOGD("%s]: client %p", __func__, client);

    typedef struct pairKeyBatchMode {
        LocationAPI* client;
        uint32_t id;
        inline pairKeyBatchMode(LocationAPI* _client, uint32_t _id) :
            client(_client), id(_id) {}
    } pairKeyBatchMode;
    std::vector<pairKeyBatchMode> vBatchingClient;
    for (auto it : mBatchingSessions) {
        if (client == it.first.client) {
            vBatchingClient.emplace_back(it.first.client, it.first.id);
        }
    }
    for (auto keyBatchingMode : vBatchingClient) {
        stopBatching(keyBatchingMode.client, keyBatchingMode.id, eraseSession);
    }
}

void
BatchingAdapter::updateClientsEventMask() {
    LOC_API_ADAPTER_EVENT_MASK_T mask = 0;
    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        // we don't register LOC_API_ADAPTER_BIT_BATCH_FULL until we
        // start batching with ROUTINE option
        if (it->second.batchingCb != nullptr) {
            mask |= LOC_API_ADAPTER_BIT_BATCH_STATUS;
        }
    }
    if (autoReportBatchingSessionsCount() > 0) {
        mask |= LOC_API_ADAPTER_BIT_BATCH_FULL;
    }
    updateEvtMask(mask, LOC_REGISTRATION_MASK_SET);
}

void
BatchingAdapter::handleEngineLockStatusEvent(EngineLockState engineLockState) {

    LOC_LOGd("Engine state : %d", engineLockState);

    struct MsgEngineLockStateEvent : public LocMsg {
        BatchingAdapter& mAdapter;
        EngineLockState mEngineLockState;

        inline MsgEngineLockStateEvent(BatchingAdapter& adapter, EngineLockState engineLockState) :
            LocMsg(),
            mAdapter(adapter),
            mEngineLockState(engineLockState){}

        virtual void proc() const {
            mAdapter.handleEngineLockStatus(mEngineLockState);
        }
    };

    sendMsg(new MsgEngineLockStateEvent(*this, engineLockState));
}

void
BatchingAdapter::handleEngineLockStatus(EngineLockState engineLockState) {

    if (ENGINE_LOCK_STATE_DISABLED != engineLockState) {
        for (auto msg: mPendingMsgs) {
            sendMsg(msg);
        }
        mPendingMsgs.clear();

        if ((POWER_STATE_SUSPEND != mSystemPowerState) &&
            (POWER_STATE_DEEP_SLEEP_ENTRY != mSystemPowerState) &&
            POWER_STATE_SHUTDOWN != mSystemPowerState) {
            restartSessions();
        }
    }
}

void
BatchingAdapter::handleEngineUpEvent() {
    struct MsgSSREvent : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        inline MsgSSREvent(BatchingAdapter& adapter,
                           LocApiBase& api) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api) {}
        virtual void proc() const {
            mAdapter.setEngineCapabilitiesKnown(true);
            mAdapter.broadcastCapabilities(mAdapter.getCapabilities());
            mApi.setBatchSize(mAdapter.getBatchSize());
            if (ENGINE_LOCK_STATE_DISABLED != mApi.getEngineLockState()) {
                for (auto msg: mAdapter.mPendingMsgs) {
                    mAdapter.sendMsg(msg);
                }
                mAdapter.mPendingMsgs.clear();

                if ((POWER_STATE_SUSPEND != mAdapter.mSystemPowerState) &&
                    (POWER_STATE_DEEP_SLEEP_ENTRY != mAdapter.mSystemPowerState) &&
                    POWER_STATE_SHUTDOWN != mAdapter.mSystemPowerState) {
                    mAdapter.restartSessions();
                }
            }
        }
    };

    sendMsg(new MsgSSREvent(*this, *mLocApi));
}

void
BatchingAdapter::restartSessions() {
    LOC_LOGD("%s]: ", __func__);

    if (autoReportBatchingSessionsCount() > 0) {
        updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                      LOC_REGISTRATION_MASK_ENABLED);
    }
    for (auto it = mBatchingSessions.begin();
              it != mBatchingSessions.end(); ++it) {
        mLocApi->startBatching(it->first.id, it->second,
                getBatchingAccuracy(), getBatchingTimeout(),
                new LocApiResponse(*getContext(),
                [] (LocationError /*err*/) {}));
    }
}

bool
BatchingAdapter::hasBatchingCallback(LocationAPI* client) {
    auto it = mClientData.find(client);
    return (it != mClientData.end() && it->second.batchingCb);
}

bool
BatchingAdapter::isBatchingSession(LocationAPI* client, uint32_t sessionId) {
    LocationSessionKey key(client, sessionId);
    return (mBatchingSessions.find(key) != mBatchingSessions.end());
}

void
BatchingAdapter::saveBatchingSession(LocationAPI* client, uint32_t sessionId,
        const BatchingOptions& batchingOptions) {
    LocationSessionKey key(client, sessionId);
    mBatchingSessions[key] = batchingOptions;
}

void
BatchingAdapter::eraseBatchingSession(LocationAPI* client, uint32_t sessionId) {
    LocationSessionKey key(client, sessionId);
    auto it = mBatchingSessions.find(key);
    if (it != mBatchingSessions.end()) {
        mBatchingSessions.erase(it);
    }
}

void
BatchingAdapter::reportResponse(LocationAPI* client, LocationError err, uint32_t sessionId) {
    LOC_LOGD("%s]: client %p id %u err %u", __func__, client, sessionId, err);

    auto it = mClientData.find(client);
    if (it != mClientData.end() &&
        it->second.responseCb != nullptr) {
        it->second.responseCb(err, sessionId);
    } else {
        LOC_LOGE("%s]: client %p id %u not found in data", __func__, client, sessionId);
    }
}

uint32_t
BatchingAdapter::autoReportBatchingSessionsCount() {
    uint32_t count = 0;
    for (auto batchingSession: mBatchingSessions) {
        if (batchingSession.second.batchingMode != BATCHING_MODE_NO_AUTO_REPORT) {
            count++;
        }
    }
    return count;
}

uint32_t
BatchingAdapter::startBatchingCommand(
        LocationAPI* client, const BatchingOptions& batchOptions) {
    uint32_t sessionId = generateSessionId();
    LOC_LOGD("%s]: client %p id %u minInterval %u mode %u Batching Mode %d",
             __func__, client, sessionId, batchOptions.minInterval,
             batchOptions.mode,batchOptions.batchingMode);

    struct MsgStartBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        BatchingOptions mBatchingOptions;
        inline MsgStartBatching(BatchingAdapter& adapter,
                               LocApiBase& api,
                               LocationAPI* client,
                               uint32_t sessionId,
                               const BatchingOptions& batchOptions) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mBatchingOptions(batchOptions) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgStartBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;

            if (!mAdapter.hasBatchingCallback(mClient)) {
                err = LOCATION_ERROR_CALLBACK_MISSING;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                mAdapter.startBatching(mClient, mSessionId, mBatchingOptions);
            } else {
                mAdapter.reportResponse(mClient, err, mSessionId);
            }
        }
    };

    sendMsg(new MsgStartBatching(*this, *mLocApi, client, sessionId, batchOptions));

    return sessionId;
}

void
BatchingAdapter::startBatching(LocationAPI* client, uint32_t sessionId,
        const BatchingOptions& batchingOptions) {
    saveBatchingSession(client, sessionId, batchingOptions);
    if (ENGINE_LOCK_STATE_DISABLED == mLocApi->getEngineLockState()) {
        LOC_LOGe("engine lock disabled, return!");
        reportResponse(client, LOCATION_ERROR_NOT_SUPPORTED, sessionId);
        return;
    }
    if (batchingOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT &&
        1 == autoReportBatchingSessionsCount()) {
        // if there is no other batching session interested in batch full event, then this
        // new session will need to register for batch full event
        updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                      LOC_REGISTRATION_MASK_ENABLED);
    }

    // Assume start will be OK, remove session if not
    mLocApi->startBatching(sessionId, batchingOptions, getBatchingAccuracy(),
            getBatchingTimeout(), new LocApiResponse(*getContext(),
            [this, client, sessionId, batchingOptions] (LocationError err) {
        if (LOCATION_ERROR_SUCCESS != err) {
            eraseBatchingSession(client, sessionId);
        }

        if (LOCATION_ERROR_SUCCESS != err &&
            batchingOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT &&
            0 == autoReportBatchingSessionsCount()) {
            // if we fail to start batching and we have already registered batch full event
            // we need to undo that since no sessions are now interested in batch full event
            updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                          LOC_REGISTRATION_MASK_DISABLED);
        }

        reportResponse(client, err, sessionId);
    }));
}

void
BatchingAdapter::updateBatchingOptionsCommand(LocationAPI* client, uint32_t id,
        const BatchingOptions& batchOptions) {
    LOC_LOGD("%s]: client %p id %u minInterval %u mode %u batchMode %u",
             __func__, client, id, batchOptions.minInterval,
             batchOptions.mode,
             batchOptions.batchingMode);

    struct MsgUpdateBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        BatchingOptions mBatchOptions;
        inline MsgUpdateBatching(BatchingAdapter& adapter,
                                LocApiBase& api,
                                LocationAPI* client,
                                uint32_t sessionId,
                                const BatchingOptions& batchOptions) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mBatchOptions(batchOptions) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgUpdateBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            } else if (mBatchOptions.batchingMode > BATCHING_MODE_NO_AUTO_REPORT) {
                err = LOCATION_ERROR_INVALID_PARAMETER;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                mAdapter.stopBatching(mClient, mSessionId, true, mBatchOptions);
            }
        }
    };

    sendMsg(new MsgUpdateBatching(*this, *mLocApi, client, id, batchOptions));
}

void
BatchingAdapter::stopBatchingCommand(LocationAPI* client, uint32_t id) {
    LOC_LOGD("%s]: client %p id %u", __func__, client, id);

    struct MsgStopBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        inline MsgStopBatching(BatchingAdapter& adapter,
                               LocApiBase& api,
                               LocationAPI* client,
                               uint32_t sessionId) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgStopBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                mAdapter.stopBatching(mClient, mSessionId);
            }
        }
    };

    sendMsg(new MsgStopBatching(*this, *mLocApi, client, id));
}

void
BatchingAdapter::stopBatching(LocationAPI* client, uint32_t sessionId, bool restartNeeded,
        const BatchingOptions& batchOptions, bool eraseSession) {
    LocationSessionKey key(client, sessionId);
    auto it = mBatchingSessions.find(key);
    if (it != mBatchingSessions.end()) {
        auto flpOptions = it->second;
        // Assume stop will be OK, restore session if not
        if (eraseSession)
            eraseBatchingSession(client, sessionId);
        if (ENGINE_LOCK_STATE_DISABLED != mLocApi->getEngineLockState()) {
            mLocApi->stopBatching(sessionId,
                new LocApiResponse(*getContext(),
                [this, client, sessionId, flpOptions, restartNeeded,
                batchOptions, eraseSession]
                (LocationError err) {
                if (LOCATION_ERROR_SUCCESS != err) {
                    if (eraseSession)
                        saveBatchingSession(client, sessionId, batchOptions);
                } else {
                    // if stopBatching is success,
                    // unregister for batch full event if this was the last
                    // batching session that is interested in batch full event
                    if (0 == autoReportBatchingSessionsCount() &&
                        flpOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT) {
                        updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                                      LOC_REGISTRATION_MASK_DISABLED);
                    }

                    if (restartNeeded) {
                        if (batchOptions.batchingMode == BATCHING_MODE_ROUTINE ||
                                batchOptions.batchingMode == BATCHING_MODE_NO_AUTO_REPORT) {
                            startBatching(client, sessionId, batchOptions);
                        }
                    }
                }
                reportResponse(client, err, sessionId);
            }));
        }
    }
}

void
BatchingAdapter::getBatchedLocationsCommand(LocationAPI* client, uint32_t id, size_t count) {
    LOC_LOGD("%s]: client %p id %u count %zu", __func__, client, id, count);

    struct MsgGetBatchedLocations : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        size_t mCount;
        inline MsgGetBatchedLocations(BatchingAdapter& adapter,
                                     LocApiBase& api,
                                     LocationAPI* client,
                                     uint32_t sessionId,
                                     size_t count) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mCount(count) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgGetBatchedLocations(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.hasBatchingCallback(mClient)) {
                err = LOCATION_ERROR_CALLBACK_MISSING;
            } else if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                mApi.getBatchedLocations(mCount, new LocApiResponse(*mAdapter.getContext(),
                            [&mAdapter = mAdapter, mSessionId = mSessionId,
                            mClient = mClient] (LocationError err) {
                            mAdapter.reportResponse(mClient, err, mSessionId);
                            }));
            } else {
                mAdapter.reportResponse(mClient, err, mSessionId);
            }
        }
    };

    sendMsg(new MsgGetBatchedLocations(*this, *mLocApi, client, id, count));
}

void
BatchingAdapter::reportLocationsEvent(const Location* locations, size_t count) {
    LOC_LOGD("%s]: count %zu ", __func__, count);

    struct MsgReportLocations : public LocMsg {
        BatchingAdapter& mAdapter;
        Location* mLocations;
        size_t mCount;
        inline MsgReportLocations(BatchingAdapter& adapter,
                                  const Location* locations,
                                  size_t count) :
            LocMsg(),
            mAdapter(adapter),
            mLocations(new Location[count]),
            mCount(count)
        {
            if (nullptr == mLocations) {
                LOC_LOGE("%s]: new failed to allocate mLocations", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                mLocations[i] = locations[i];
            }
        }
        inline virtual ~MsgReportLocations() {
            if (nullptr != mLocations)
                delete[] mLocations;
        }
        inline virtual void proc() const {
            mAdapter.reportLocations(mLocations, mCount);
        }
    };

    sendMsg(new MsgReportLocations(*this, locations, count));
}

void
BatchingAdapter::reportLocations(Location* locations, size_t count) {
    BatchingOptions batchOptions(BATCHING_MODE_ROUTINE);

    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        if (nullptr != it->second.batchingCb) {
            it->second.batchingCb(count, locations, batchOptions);
        }
    }
}

void
BatchingAdapter::reportBatchStatusChange(BatchingStatus batchStatus,
        std::list<uint32_t> & completedTripsList) {
    BatchingStatusInfo batchStatusInfo = {batchStatus};

    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        if (nullptr != it->second.batchingStatusCb) {
            it->second.batchingStatusCb(batchStatusInfo, completedTripsList);
        }
    }
}

void
BatchingAdapter::reportBatchStatusChangeEvent(BatchingStatus batchStatus) {
    struct MsgReportBatchStatus : public LocMsg {
        BatchingAdapter& mAdapter;
        BatchingStatus mBatchStatus;
        inline MsgReportBatchStatus(BatchingAdapter& adapter,
                BatchingStatus batchStatus) :
            LocMsg(),
            mAdapter(adapter),
            mBatchStatus(batchStatus)
        {
        }
        inline virtual ~MsgReportBatchStatus() {
        }
        inline virtual void proc() const {
            std::list<uint32_t> tempList;
            tempList.clear();
            mAdapter.reportBatchStatusChange(mBatchStatus, tempList);
        }
    };

    sendMsg(new MsgReportBatchStatus(*this, batchStatus));
}

void
BatchingAdapter::updateSystemPowerStateCommand(PowerStateType powerState) {
    LOC_LOGD("%s]: powerState: %d", __func__, powerState);

    struct MsgUpdateSystemPowerState : public LocMsg {
        BatchingAdapter& mAdapter;
        PowerStateType mPowerState;
        inline MsgUpdateSystemPowerState(BatchingAdapter& adapter,
                PowerStateType powerState) :
            mAdapter(adapter),
            mPowerState(powerState) {}
        inline virtual void proc() const {
            mAdapter.updateSystemPowerState(mPowerState);
        }
    };

    sendMsg(new MsgUpdateSystemPowerState(*this, powerState));
}

void
BatchingAdapter::suspendBatchingSessions() {
    for (auto it = mBatchingSessions.begin(); it != mBatchingSessions.end(); ++it) {
        LocationSessionKey key(it->first);
        stopClientSessions(key.client, false);
    }
}

void
BatchingAdapter::updateSystemPowerState(PowerStateType systemPowerState) {

    if (POWER_STATE_UNKNOWN != systemPowerState) {
        mSystemPowerState = systemPowerState;

        /*Manage active GNSS sessions based on power event*/
        switch (systemPowerState){

            case POWER_STATE_SUSPEND:
            case POWER_STATE_SHUTDOWN:
            case POWER_STATE_DEEP_SLEEP_ENTRY:
                suspendBatchingSessions();
                LOC_LOGd("Suspending all Batching session -- powerState: %d", systemPowerState);
                break;
            case POWER_STATE_RESUME:
            case POWER_STATE_DEEP_SLEEP_EXIT:
                restartSessions();
                LOC_LOGd("Re-starting all Batching session -- powerState: %d", systemPowerState);
                break;
            default:
                break;
        } // switch
    }
}

