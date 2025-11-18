/* Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_SystemStatusOsObserver"

#include <algorithm>
#include <SystemStatus.h>
#include <SystemStatusOsObserver.h>
#include <IDataItemCore.h>

namespace loc_core
{

SystemStatusOsObserver* SystemStatusOsObserver::sInstance = nullptr;
std::once_flag SystemStatusOsObserver::sFlag;

SystemStatusOsObserver::~SystemStatusOsObserver() {
    // Destroy cache
    for (auto each : mCachedDataItemMap) {
        if (nullptr != each.second) {
            delete each.second;
        }
    }
    mCachedDataItemMap.clear();
}

void SystemStatusOsObserver::setSubscriptionObj(updateDataitemIdListFunc& fun,
        const unordered_set<DataItemId>& li) {
    struct setSubscriptionMsg : public LocMsg {
        inline setSubscriptionMsg(SystemStatusOsObserver* parent,
                const unordered_set<DataItemId>& li, updateDataitemIdListFunc& fun) :
            mParent(parent), mDataItemSet(std::move(li)), mFunc(fun) {}

        void proc() const {
            mParent->mSubscriptionVec.push_back({mFunc, mDataItemSet});
            std::unordered_set<DataItemId> idsTobeSubscribe;
            for (const std::pair<DataItemId, IDataItemCore*> item : mParent->mCachedDataItemMap) {
                if (mDataItemSet.find(item.first) != mDataItemSet.end()) {
                    idsTobeSubscribe.insert(item.first);
                }
            }
            if (!idsTobeSubscribe.empty()) {
                mFunc(idsTobeSubscribe, SUBSCRIBE_DATA_ITEM_ID);
            }
        }
        mutable SystemStatusOsObserver* mParent;
        updateDataitemIdListFunc mFunc;
        const unordered_set<DataItemId> mDataItemSet;
    };

    if (nullptr != fun) {
        LOC_LOGd("set subscriptionObj");
        mMsgTask->sendMsg(new setSubscriptionMsg(this, li, fun));
    } else {
        LOC_LOGe("set subscriptionObj nullptr")
    }
}

void SystemStatusOsObserver::setFrameworkActionReqObj(IFrameworkActionReq* frameworkActionReqObj) {
#ifdef USE_GLIB
    struct setFrameworkActionReqMsg : public LocMsg {
        inline setFrameworkActionReqMsg(SystemStatusOsObserver* parent,
                IFrameworkActionReq* req) :
            mParent(parent), mFWReq(req) {}

        void proc() const {
            mParent->mFrameworkActionReqObj = mFWReq;
            uint32_t numBackHaulClients = mParent->mBackHaulConnReqCache.size();
            if (numBackHaulClients > 0) {
                // For each client, invoke connectbackhaul.
                for (auto clientContext : mParent->mBackHaulConnReqCache) {
                    LOC_LOGd("Invoke connectBackhaul for client: %s Sub: %d Apn: %s IpType: %d",
                            clientContext.second.clientName.c_str(), clientContext.second.prefSub,
                            clientContext.second.prefApn.c_str(), clientContext.second.prefIpType);
                    BackhaulContext ctx = { clientContext.second.clientName,
                        clientContext.second.prefSub,
                        clientContext.second.prefApn,
                        clientContext.second.prefIpType };
                    mParent->connectBackhaul(ctx);
                }
                // Clear the set
                mParent->mBackHaulConnReqCache.clear();
            }
        }
        mutable SystemStatusOsObserver* mParent;
        IFrameworkActionReq* mFWReq;
    };

    if (nullptr != frameworkActionReqObj) {
        LOC_LOGd("setFrameworkActionReqObj");
        mMsgTask->sendMsg(new setFrameworkActionReqMsg(this, frameworkActionReqObj));
    }
#endif
}

/******************************************************************************
 IDataItemSubscription Overrides
******************************************************************************/
void SystemStatusOsObserver::subscribe(const unordered_set<DataItemId>& li,
                                       IDataItemObserver* client,
                                       bool toRequestData) {
    struct HandleSubscribeReq : public LocMsg {
        inline HandleSubscribeReq(SystemStatusOsObserver* parent,
                const unordered_set<DataItemId>& li, IDataItemObserver* client, bool requestData) :
                mParent(parent), mClient(client),
                mDataItemSet(std::move(li)),
                mToRequestData(requestData) {}

        void proc() const {
            string info = "";
            mClient->getName(info);
            unordered_set<const IDataItemCore*> diList;
            for (DataItemId item : mDataItemSet) {
                // Logging
                info += " ";
                info += std::to_string((int8_t)item);
                //Update mClientMap
                if (mParent->mClientMap.find(item) != mParent->mClientMap.end()) {
                    mParent->mClientMap[item].insert(mClient);
                } else {
                    mParent->mClientMap[item] = {mClient};
                }
                //Add data item to cache if not exists
                if (mParent->mCachedDataItemMap.find(item) == mParent->mCachedDataItemMap.end()) {
                    IDataItemCore* di = DataItemsFactory::createNewDataItem(item);
                    if (di != nullptr) {
                        mParent->mCachedDataItemMap[item] =di;
                    }
                }
                diList.insert(static_cast<const IDataItemCore*>(mParent->mCachedDataItemMap[item]));
            }
            LOC_LOGd("subscribe: %s", info.c_str());
            //Notify local data items to clients
            mClient->notify(diList);
            //Subscribe data items to source
            for (std::pair<updateDataitemIdListFunc, unordered_set<DataItemId>> item :
                    mParent->mSubscriptionVec) {
                unordered_set<DataItemId> idsTobeSubscribe;
                for (DataItemId di : mDataItemSet) {
                    if (item.second.find(di) != item.second.end()) {
                        idsTobeSubscribe.insert(di);
                    }
                }
                if (idsTobeSubscribe.size() > 0) {
                    if (mToRequestData) {
                        item.first(idsTobeSubscribe, REQUEST_DATA_ITEM_ID);
                    } else {
                        item.first(idsTobeSubscribe, SUBSCRIBE_DATA_ITEM_ID);
                    }
                }
            }
        }
        mutable SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        unordered_set<DataItemId> mDataItemSet;
        bool mToRequestData;
    };

    if (li.empty() || nullptr == client) {
        LOC_LOGw("Data item set is empty or client is nullptr");
    } else {
        mMsgTask->sendMsg(new HandleSubscribeReq(this, li, client, toRequestData));
    }
}

void SystemStatusOsObserver::unsubscribe(
        const unordered_set<DataItemId>& li, IDataItemObserver* client) {
    struct HandleUnsubscribeReq : public LocMsg {
        HandleUnsubscribeReq(SystemStatusOsObserver* parent,
                const unordered_set<DataItemId>& li, IDataItemObserver* client) :
                mParent(parent), mClient(client),
                mDataItemSet(std::move(li)) {}

        void proc() const {
            string info = "";
            mClient->getName(info);
            unordered_set<DataItemId> idsToUnsubscribe;
            for (std::pair<DataItemId, unordered_set<IDataItemObserver*>> iter :
                    mParent->mClientMap) {
                if (mDataItemSet.find(iter.first) != mDataItemSet.end()) {
                    iter.second.erase(mClient);
                    if (iter.second.empty()) {
                        mParent->mClientMap.erase(iter.first);
                        idsToUnsubscribe.insert(iter.first);
                        // Logging
                        info += " ";
                        info += std::to_string((int8_t)(iter.first));
                    }
                }
            }
            LOC_LOGd("unsubscribe: %s", info.c_str());
            if (!idsToUnsubscribe.empty()) {
                for (const auto& subscription : mParent->mSubscriptionVec) {
                    subscription.first(idsToUnsubscribe, UNSUBSCRIBE_DATA_ITEM_ID);
                }
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        unordered_set<DataItemId> mDataItemSet;
    };

    if (li.empty() || nullptr == client) {
        LOC_LOGw("Data item set is empty or client is nullptr");
    } else {
        mMsgTask->sendMsg(new HandleUnsubscribeReq(this, li, client));
    }
}

void SystemStatusOsObserver::unsubscribeAll(IDataItemObserver* client) {
    struct HandleUnsubscribeAllReq : public LocMsg {
        HandleUnsubscribeAllReq(SystemStatusOsObserver* parent,
                IDataItemObserver* client) :
                mParent(parent), mClient(client) {}

        void proc() const {
            string info = "";
            mClient->getName(info);
            unordered_set<DataItemId> idsToUnsubscribe;
            for (std::pair<DataItemId, unordered_set<IDataItemObserver*>> iter :
                    mParent->mClientMap) {
                iter.second.erase(mClient);
                if (iter.second.empty()) {
                    mParent->mClientMap.erase(iter.first);
                    idsToUnsubscribe.insert(iter.first);
                    // Logging
                    info += " ";
                    info += std::to_string((int8_t)(iter.first));
                }
            }
            LOC_LOGd("unsubscribe: %s", info.c_str());
            if (!idsToUnsubscribe.empty()) {
                for (const auto& subscription : mParent->mSubscriptionVec) {
                    subscription.first(idsToUnsubscribe, UNSUBSCRIBE_DATA_ITEM_ID);
                }
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
    };

    if (nullptr == client) {
        LOC_LOGw("Data item set is empty or client is nullptr");
    } else {
        mMsgTask->sendMsg(new HandleUnsubscribeAllReq(this, client));
    }
}

void SystemStatusOsObserver::notify(IDataItemCore* di) {
    struct notifyDataItem : public LocMsg {
        notifyDataItem(SystemStatusOsObserver* parent, IDataItemCore* di) :
                mParent(parent), mDataItem(di) {}

        void proc() const {
            std::string dataItemStr;
            mDataItem->stringify(dataItemStr);
            LOC_LOGd("[SystemStatusOsObserver] notify: %s", dataItemStr.c_str());
            auto iter = mParent->mCachedDataItemMap.find(mDataItem->getId());
            if (iter != mParent->mCachedDataItemMap.end() &&
                    mParent->mCachedDataItemMap[mDataItem->getId()]->equal(mDataItem)) {
                LOC_LOGd("data item found in cache and remain the same");
                delete mDataItem;
            } else {
                //Update cache
                LOC_LOGd("data item notify to clients");
                //Release previous cached data item
                if (iter != mParent->mCachedDataItemMap.end() &&
                        mParent->mCachedDataItemMap[mDataItem->getId()] != nullptr) {
                    LOC_LOGd("release previous data item, %d", mDataItem->getId());
                    delete mParent->mCachedDataItemMap[mDataItem->getId()];
                }
                //Save new data item in cache
                mParent->mCachedDataItemMap[mDataItem->getId()] = mDataItem;
                //Notify to clients
                for(IDataItemObserver* obPtr : mParent->mClientMap[mDataItem->getId()]) {
                    obPtr->notify({mDataItem});
                }
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemCore* mDataItem;
    };

    if (di == nullptr) {
        LOC_LOGw("Data item set is empty or client is nullptr");
    } else {
        mMsgTask->sendMsg(new notifyDataItem(this, di));
    }
}

void SystemStatusOsObserver::eventConnectionStatus(bool connected, int8_t type,
                           bool roaming, NetworkHandle networkHandle, const string& apn) {
    IDataItemCore* di = new NetworkInfoDataItem(type, connected && (!roaming),
            connected, roaming, networkHandle, apn);
    notify(di);
}

void SystemStatusOsObserver::eventOptInStatus(bool userConsent) {
    IDataItemCore* di = new ENHDataItem(userConsent, ENHDataItem::ENH_USER_CONSENT_ALLOWED_MASK);
    notify(di);
}

void SystemStatusOsObserver::eventRegionAllowedStatus(bool regionAllowed) {
    IDataItemCore* di =
        new ENHDataItem(regionAllowed, ENHDataItem::ENH_EMBARGO_REGION_ALLOWED_MASK);
    notify(di);
}

void SystemStatusOsObserver::eventWifiHardwareStatus(bool isEnabled) {
    IDataItemCore* di = new WifiHardwareStateDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventTimeZoneChange(int64_t currentTimeMillis,
        int32_t rawOffsetTZ, int32_t dstOffsetTZ) {
    IDataItemCore* di = new TimeZoneChangeDataItem(currentTimeMillis, rawOffsetTZ, dstOffsetTZ);
    notify(di);
}

void SystemStatusOsObserver::eventTimeChange(int64_t currentTimeMillis,
        int32_t rawOffsetTZ, int32_t dstOffsetTZ) {
    IDataItemCore* di = new TimeChangeDataItem(currentTimeMillis, rawOffsetTZ, dstOffsetTZ);
    notify(di);
}

void SystemStatusOsObserver::eventModelData(const std::string& data) {
    IDataItemCore* di = new ModelDataItem(data);
    notify(di);
}

void SystemStatusOsObserver::eventManufacturerData(const std::string& data) {
    IDataItemCore* di = new ManufacturerDataItem(data);
    notify(di);
}

void SystemStatusOsObserver::eventLocRilCellInfo(const LocRilCellInfo& info) {
    IDataItemCore* di = new RilCellInfoDataItem(info);
    notify(di);
}

void SystemStatusOsObserver::eventWifiSupplicantInfo(const LocWifiSupplicantInfo& info) {
    IDataItemCore* di = new WifiSupplicantStatusDataItem(info);
    notify(di);
}

void SystemStatusOsObserver::eventMccmnc(const std::string& mccmnc) {
    IDataItemCore* di = new MccmncDataItem(mccmnc);
    notify(di);
}

void SystemStatusOsObserver::eventInEmergencyCall(bool isEnabled) {
    IDataItemCore* di = new InEmergencyCallDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventSetTracking(bool isEnabled) {
    IDataItemCore* di = new TrackingStartedDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventNtripStarted(bool isEnabled) {
    IDataItemCore* di = new NtripStartedDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventPreciseLocation(bool isEnabled) {
    IDataItemCore* di = new PreciseLocationEnabledDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventLocFeatureStatus(std::unordered_set<int> fids) {
    IDataItemCore* di = new LocFeatureStatusDataItem(fids);
    notify(di);
}

void SystemStatusOsObserver::eventNlpSessionStatus(bool isEnabled) {
    IDataItemCore* di = new NlpSessionStartedDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventGpsEnabled(bool isEnabled) {
    IDataItemCore* di = new GPSStateDataItem(isEnabled);
    notify(di);
}

void SystemStatusOsObserver::eventWwanAppInfo(
        int32_t uid,
        bool appHasFinePermission,
        bool appHasBackgroundPermission,
        string appPackageName,
        string appCookie) {
    IDataItemCore* di = new WwanAppInfoDataItem(uid, appHasFinePermission,
            appHasBackgroundPermission, appPackageName, appCookie);
    notify(di);
}

void SystemStatusOsObserver::eventAssistedGpsEnabled(bool aGpsEnabled) {
    IDataItemCore* di = new AssistedGpsDataItem(aGpsEnabled);
    notify(di);
}

/*****************  None Android specific start ***************************/
#ifdef USE_GLIB
bool SystemStatusOsObserver::connectBackhaul(const BackhaulContext& ctx)
{
    struct HandleConnectBackhaul : public LocMsg {
        HandleConnectBackhaul(SystemStatusOsObserver* parent, const BackhaulContext& ctx) :
                mParent(parent), mCtx(ctx) {}
        virtual ~HandleConnectBackhaul() {}
        void proc() const {
            LOC_LOGi("HandleConnectBackhaul::enter");
            if (mParent->mFrameworkActionReqObj != NULL) {
                mParent->mFrameworkActionReqObj->connectBackhaul(mCtx);
            } else {
                LOC_LOGe("Framework action request object is NULL.Caching connect request: %s",
                         mCtx.clientName.c_str());
                LOC_LOGd("Adding client context to BackHaulConnReqCache list");
                mParent->mBackHaulConnReqCache[mCtx.clientName] = mCtx;
            }
            LOC_LOGi("HandleConnectBackhaul::exit");
        }
        SystemStatusOsObserver* mParent;
        BackhaulContext mCtx;
    };
    mMsgTask->sendMsg(
            new (nothrow) HandleConnectBackhaul(this, ctx));
    return (mFrameworkActionReqObj != NULL);
}

bool SystemStatusOsObserver::disconnectBackhaul(const BackhaulContext& ctx)
{
    struct HandleDisconnectBackhaul : public LocMsg {
        HandleDisconnectBackhaul(SystemStatusOsObserver* parent, const BackhaulContext& ctx) :
                mParent(parent), mCtx(ctx) {}
        virtual ~HandleDisconnectBackhaul() {}
        void proc() const {
            LOC_LOGi("HandleDisconnectBackhaul::enter");
            if (mParent->mFrameworkActionReqObj != NULL) {
                mParent->mFrameworkActionReqObj->disconnectBackhaul(mCtx);
            } else {
                LOC_LOGe("Framework action request object is NULL.Caching connect request: %s",
                         mCtx.clientName.c_str());
                LOC_LOGd("Removing client from BackHaulConnReqCache list");
                mParent->mBackHaulConnReqCache.erase(mCtx.clientName);
            }
            LOC_LOGi("HandleConnectBackhaul::exit");
        }
        SystemStatusOsObserver* mParent;
        BackhaulContext mCtx;
    };
    mMsgTask->sendMsg(
            new (nothrow) HandleDisconnectBackhaul(this, ctx));
    return (mFrameworkActionReqObj != NULL);
}
#endif
/*****************  None Android specific end ***************************/

} // namespace loc_core

