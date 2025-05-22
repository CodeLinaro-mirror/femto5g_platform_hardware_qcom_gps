/* Copyright (c) 2015-2017, 2020 The Linux Foundation. All rights reserved.
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
#ifndef __SYSTEM_STATUS_OSOBSERVER__
#define __SYSTEM_STATUS_OSOBSERVER__

#include <cinttypes>
#include <string>
#include <map>
#include <new>
#include <vector>
#include <MsgTask.h>
#include <DataItemId.h>
#include <loc_pla.h>
#include <log_util.h>
#include <gps_extended.h>
#include <unordered_set>
#include <unordered_map>
#include <DataItemConcreteTypes.h>
#include <IDataItemObserver.h>
#include <IFrameworkActionReq.h>

namespace loc_core
{
/******************************************************************************
 SystemStatusOsObserver
******************************************************************************/
using namespace std;
using namespace loc_util;

enum SubscribeAction {
    SUBSCRIBE_DATA_ITEM_ID    = (1<<0),
    UNSUBSCRIBE_DATA_ITEM_ID  = (1<<1),
    REQUEST_DATA_ITEM_ID      = (1<<2),
};

typedef std::function<void(const unordered_set<DataItemId>& idList, SubscribeAction toReqData)>
    updateDataitemIdListFunc;

// Forward Declarations
class IDataItemCore;
class SystemStatusOsObserver;
#ifdef USE_GLIB
// Cache details of backhaul client requests
typedef std::map<string, BackhaulContext> ClientBackhaulReqCache;
#endif

class SystemStatusOsObserver : public IFrameworkActionReq {

public:
    static SystemStatusOsObserver* getInstance(const MsgTask* msgTask) {
        std::call_once(sFlag, [&](){ sInstance = new SystemStatusOsObserver(msgTask);});
        return sInstance;
    }
    // ctor
    inline SystemStatusOsObserver(const MsgTask* msgTask) :
            mMsgTask(msgTask), mFrameworkActionReqObj(NULL) {}

    // dtor
    ~SystemStatusOsObserver();

    // Public functions used by IDataItemObserver
    inline void subscribe(const unordered_set<DataItemId>& li, IDataItemObserver* client) {
        subscribe(li, client, false);
    }
    void updateSubscription(const unordered_set<DataItemId>& li, IDataItemObserver* client) {
        subscribe(li, client, false);
    }
    inline void requestData(const unordered_set<DataItemId>& li, IDataItemObserver* client) {
        subscribe(li, client, true);
    }
    void unsubscribe(const unordered_set<DataItemId>& li, IDataItemObserver* client);
    void unsubscribeAll(IDataItemObserver* client);

    // To set the subscription callback function from LocAidl
    void setSubscriptionObj(updateDataitemIdListFunc& fun,
            const unordered_set<DataItemId>& li);

    //Event system status events, each data item should map to one function below
    void eventConnectionStatus(bool connected, int8_t type,
                           bool roaming, NetworkHandle networkHandle, const string& apn);
    void eventOptInStatus(bool userConsent);
    void eventRegionAllowedStatus(bool regionAllowed);
    void eventWifiHardwareStatus(bool isWifiEnabled);
    void eventTimeZoneChange(int64_t currentTimeMillis,
        int32_t rawOffsetTZ, int32_t dstOffsetTZ);
    void eventTimeChange(int64_t currentTimeMillis,
        int32_t rawOffsetTZ, int32_t dstOffsetTZ);
    void eventModelData(const std::string& data);
    void eventManufacturerData(const std::string& data);
    void eventLocRilCellInfo(const LocRilCellInfo& info);
    void eventWifiSupplicantInfo(const LocWifiSupplicantInfo& info);
    void eventMccmnc(const std::string& mccmnc);
    void eventInEmergencyCall(bool isEmergency);
    void eventSetTracking(bool tracking);
    void eventNtripStarted(bool ntripStarted);
    void eventPreciseLocation(bool preciseLocation);
    void eventLocFeatureStatus(std::unordered_set<int> fids);
    void eventNlpSessionStatus(bool nlpStarted);
    void eventGpsEnabled(bool gpsEnabled);
    void eventWwanAppInfo(int32_t pid = 0,
            int32_t uid = 0,
            bool appHasFinePermission = false,
            bool appHasBackgroundPermission = false,
            string appHash = "",
            string appPackageName = "",
            string appCookie = "",
            string appQwesLicenseId = "");

/*****************  None Android specific start ***************************/
#ifdef USE_GLIB
    virtual bool connectBackhaul(const BackhaulContext& ctx) override;
    virtual bool disconnectBackhaul(const BackhaulContext& ctx) override;
#endif

    // To set the framework action request object
    void setFrameworkActionReqObj(IFrameworkActionReq* frameworkActionReqObj);
/*****************  None Android specific end ***************************/

private:
    static SystemStatusOsObserver* sInstance;
    static std::once_flag sFlag;
    const MsgTask* mMsgTask;
    IFrameworkActionReq* mFrameworkActionReqObj;

    unordered_map<DataItemId, unordered_set<IDataItemObserver*>> mClientMap;
    unordered_map<DataItemId, IDataItemCore*> mCachedDataItemMap;
    std::vector<std::pair<updateDataitemIdListFunc, unordered_set<DataItemId>>> mSubscriptionVec;
#ifdef USE_GLIB
    // Cache the framework action request for connect/disconnect
    ClientBackhaulReqCache  mBackHaulConnReqCache;
#endif
    void notify(IDataItemCore* di);
    void subscribe(const unordered_set<DataItemId>& li, IDataItemObserver* client,
            bool toRequestData);
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

