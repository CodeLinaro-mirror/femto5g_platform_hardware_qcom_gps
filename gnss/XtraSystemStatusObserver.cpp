/* Copyright (c) 2017, 2020-2021 The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_XtraSystemStatusObs"

#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <loc_log.h>
#include <loc_nmea.h>
#include <SystemStatus.h>
#include <vector>
#include <sstream>
#include <XtraSystemStatusObserver.h>
#include <LocAdapterBase.h>
#include <DataItemId.h>
#include <DataItemsFactoryProxy.h>
#include <DataItemConcreteTypesBase.h>

using namespace loc_util;
using namespace loc_core;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_XSSO"

class XtraIpcListener : public ILocIpcListener {
    IOsObserver*    mSystemStatusObsrvr;
    const MsgTask* mMsgTask;
    XtraSystemStatusObserver& mXSSO;
public:
    inline XtraIpcListener(IOsObserver* observer, const MsgTask* msgTask,
                           XtraSystemStatusObserver& xsso) :
            mSystemStatusObsrvr(observer), mMsgTask(msgTask), mXSSO(xsso) {}
    virtual void onReceive(const char* data, uint32_t length,
                           const LocIpcRecver* recver) override {
#define STRNCMP(str, constStr) strncmp(str, constStr, sizeof(constStr)-1)
        if (!STRNCMP(data, "ping")) {
            LOC_LOGd("ping received");
#ifdef USE_GLIB
        } else if ((!STRNCMP(data, "connectBackhaul")) || (!STRNCMP(data, "disconnectBackhaul"))) {
            char clientName[30] = {0};
            uint16_t prefSub;
            char prefApnName[30] = {0};
            string prefApn;
            uint16_t prefIpType;
            int ret = sscanf(data, "%*s %29s %u %29s %u",
                             clientName, &prefSub, prefApnName, &prefIpType);
            if (0 == strcmp(prefApnName, "EMPTY")) {
                prefApn = "";
            } else {
                prefApn = string(prefApnName);
            }
            mXSSO.mBackhaulCtx.clientName.assign(clientName);
            mXSSO.mBackhaulCtx.prefSub = prefSub;
            mXSSO.mBackhaulCtx.prefApn.assign(prefApn);
            mXSSO.mBackhaulCtx.prefIpType = prefIpType;

            if (!STRNCMP(data, "connectBackhaul")) {
                mXSSO.connectNetwork(XTRA_CONNECT_REQ);
            } else {
                mXSSO.disconnectNetwork(XTRA_CONNECT_REQ);
            }
#endif
        } else if (!STRNCMP(data, "requestStatus")) {
            int32_t xtraStatusUpdated = 0;
            sscanf(data, "%*s %d", &xtraStatusUpdated);

            struct HandleStatusRequestMsg : public LocMsg {
                XtraSystemStatusObserver& mXSSO;
                int32_t mXtraStatusUpdated;
                inline HandleStatusRequestMsg(XtraSystemStatusObserver& xsso,
                                              int32_t xtraStatusUpdated) :
                        mXSSO(xsso), mXtraStatusUpdated(xtraStatusUpdated) {}
                inline void proc() const override {
                    mXSSO.onStatusRequested(mXtraStatusUpdated);
                    // SSR for DGnss Ntrip Source
                    mXSSO.restartDgnssSource();
                }
            };
            mMsgTask->sendMsg(new HandleStatusRequestMsg(mXSSO, xtraStatusUpdated));
        } else {
            LOC_LOGw("unknown event: %s", data);
        }
    }
};

#ifdef USE_GLIB
void NetConnectTimer::timeOutCallback() {

    struct TimerMsg : public LocMsg {
        XtraSystemStatusObserver&    mXtraSSO;

        inline TimerMsg(XtraSystemStatusObserver&    XtraSSO) :
            LocMsg(), mXtraSSO(XtraSSO) {}

        inline virtual void proc() const {
            mXtraSSO.connectNetwork(NO_CONNECT_REQ);
        }
    };

    mMsgTask.sendMsg(new TimerMsg(mXtraSSO));
}

void XtraSystemStatusObserver::connectNetwork(NetConnectReqBitMask conMask) {
    mNetReqMask |= conMask;
    uint32_t CHECK_NETWORK_CONNECT_TIMER  = (10000);    //10s
    LOC_LOGd("conMask 0x%x mNetReqMask 0x%x", conMask, mNetReqMask);
    if (mConnections != ~0) {
        LOC_LOGd("mConnections=%" PRIu64 " ", mConnections);
    } else {
        // mBackhaulCtx should be set by XtraClient
        if (!mBackhaulCtx.clientName.empty()) {
            mSystemStatusObsrvr->connectBackhaul(mBackhaulCtx);
            if (mNetReqMask & XTRA_CONNECT_REQ) {
                return;
            }
        }
        mNetConnectTimer.start(CHECK_NETWORK_CONNECT_TIMER, false);
    }
}

void XtraSystemStatusObserver::disconnectNetwork(NetConnectReqBitMask conMask) {
    mNetReqMask &= ~conMask;
    LOC_LOGd("conMask 0x%x mNetReqMask 0x%x", conMask, mNetReqMask);
    if (NO_CONNECT_REQ == mNetReqMask) {
        mSystemStatusObsrvr->disconnectBackhaul(mBackhaulCtx);
    }
}
#endif

XtraSystemStatusObserver::XtraSystemStatusObserver(IOsObserver* sysStatObs,
                                                   const MsgTask* msgTask) :
#ifdef USE_GLIB
        mNetConnectTimer(*this, *msgTask),
        mNetReqMask(NO_CONNECT_REQ),
#endif
        mSystemStatusObsrvr(sysStatObs), mMsgTask(msgTask),
        mGpsLock(-1), mConnections(~0), mXtraThrottle(true),
        mReqStatusReceived(false),
        mIsConnectivityStatusKnown(false),
        mSender(LocIpc::getLocIpcLocalSender(LOC_IPC_XTRA)),
        mDelayLocTimer(*mSender) {
#ifndef USE_FEATURE_TELSDK
    subscribe(true);
#endif
    auto recver = LocIpc::getLocIpcLocalRecver(
            make_shared<XtraIpcListener>(sysStatObs, msgTask, *this),
            LOC_IPC_HAL);
    mIpc.startNonBlockingListening(recver);
    mDelayLocTimer.start(100 /*.1 sec*/,  false);
}

bool XtraSystemStatusObserver::updateLockStatus(GnssConfigGpsLock lock) {
    mGpsLock = lock;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "gpslock";
    ss << " " << lock;
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateConnections(uint64_t allConnections) {
    mIsConnectivityStatusKnown = true;
    mConnections = allConnections;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "connection";
    ss << " " << mConnections;
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateTac(const string& tac) {
    mTac = tac;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "tac";
    ss << " " << tac.c_str();
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateMccMnc(const string& mccmnc) {
    mMccmnc = mccmnc;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "mncmcc";
    ss << " " << mccmnc.c_str();
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateXtraThrottle(const bool enabled) {
    mXtraThrottle = enabled;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "xtrathrottle";
    ss << " " << (enabled ? 1 : 0);
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updatePowerState(const PowerStateType powerState) {

    if (mPowerState == powerState) {
        return true;
    }

    mPowerState = powerState;

    if (!mReqStatusReceived) {
        return true;
    }

    int32_t pState;
    switch (mPowerState) {
        case POWER_STATE_UNKNOWN:
            pState = 0;
            break;
        case POWER_STATE_SUSPEND:
            pState = 1;
            break;
        case POWER_STATE_RESUME:
            pState = 2;
            break;
        case POWER_STATE_SHUTDOWN:
            pState = 3;
            break;
        default:
            LOC_LOGd("Invalid power state %d", mPowerState);
            break;
    };

    stringstream ss;
    ss <<  "powerstate";
    ss << " " << pState;
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

inline bool XtraSystemStatusObserver::onStatusRequested(int32_t xtraStatusUpdated) {
    mReqStatusReceived = true;

    if (xtraStatusUpdated) {
        return true;
    }

    stringstream ss;

    ss << "respondStatus" << endl;
    (mGpsLock == -1 ? ss : ss << mGpsLock) << endl;
    (mConnections == (uint64_t)~0 ? ss : ss << mConnections) << endl
            << mTac << endl << mMccmnc << endl << mIsConnectivityStatusKnown;

    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

void XtraSystemStatusObserver::startDgnssSource(const StartDgnssNtripParams& params) {
    stringstream ss;
    const GnssNtripConnectionParams* ntripParams = &(params.ntripParams);

    ss <<  "startDgnssSource" << endl;
    ss << ntripParams->useSSL << endl;
    ss << ntripParams->hostNameOrIp.data() << endl;
    ss << ntripParams->port << endl;
    ss << ntripParams->mountPoint.data() << endl;
    ss << ntripParams->username.data() << endl;
    ss << ntripParams->password.data() << endl;
    if (ntripParams->requiresNmeaLocation && !params.nmea.empty()) {
        ss << params.nmea.data() << endl;
    }
    string s = ss.str();

    LOC_LOGd("%s", s.data());
    LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size());
    // make a local copy of the string for SSR
    mNtripParamsString.assign(std::move(s));

#ifdef USE_GLIB
    connectNetwork(NTRP_CONNECT_REQ);
#endif
}

void XtraSystemStatusObserver::restartDgnssSource() {
    if (!mNtripParamsString.empty()) {
        LocIpc::send(*mSender,
            (const uint8_t*)mNtripParamsString.data(), mNtripParamsString.size());
        LOC_LOGv("Xtra SSR %s", mNtripParamsString.data());
    }
}

void XtraSystemStatusObserver::stopDgnssSource() {
    LOC_LOGv();
    mNtripParamsString.clear();

    const char s[] = "stopDgnssSource";
    LocIpc::send(*mSender, (const uint8_t*)s, strlen(s));
#ifdef USE_GLIB
    disconnectNetwork(NTRP_CONNECT_REQ);
#endif
}

void XtraSystemStatusObserver::updateNmeaToDgnssServer(const string& nmea)
{
    stringstream ss;
    ss <<  "updateDgnssServerNmea" << endl;
    ss << nmea.data() << endl;

    string s = ss.str();
    LOC_LOGd("%s", s.data());
    LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size());
}

void XtraSystemStatusObserver::subscribe(bool yes)
{
    // Subscription data list
    list<DataItemId> subItemIdList;
    subItemIdList.push_back(NETWORKINFO_DATA_ITEM_ID);
    subItemIdList.push_back(MCCMNC_DATA_ITEM_ID);

    if (yes) {
        mSystemStatusObsrvr->subscribe(subItemIdList, this);

        list<DataItemId> reqItemIdList;
        reqItemIdList.push_back(TAC_DATA_ITEM_ID);

        mSystemStatusObsrvr->requestData(reqItemIdList, this);

    } else {
        mSystemStatusObsrvr->unsubscribe(subItemIdList, this);
    }
}

// IDataItemObserver overrides
void XtraSystemStatusObserver::getName(string& name)
{
    name = "XtraSystemStatusObserver";
}

void XtraSystemStatusObserver::notify(const list<IDataItemCore*>& dlist)
{
    struct HandleOsObserverUpdateMsg : public LocMsg {
        XtraSystemStatusObserver* mXtraSysStatObj;
        list <IDataItemCore*> mDataItemList;

        inline HandleOsObserverUpdateMsg(XtraSystemStatusObserver* xtraSysStatObs,
                const list<IDataItemCore*>& dataItemList) :
                mXtraSysStatObj(xtraSysStatObs) {
            for (auto eachItem : dataItemList) {
                IDataItemCore* dataitem = DataItemsFactoryProxy::createNewDataItem(
                        eachItem->getId());
                if (NULL == dataitem) {
                    break;
                }
                // Copy the contents of the data item
                dataitem->copy(eachItem);

                mDataItemList.push_back(dataitem);
            }
        }

        inline ~HandleOsObserverUpdateMsg() {
            for (auto itor = mDataItemList.begin(); itor != mDataItemList.end(); ++itor) {
                if (*itor != nullptr) {
                    delete *itor;
                    *itor = nullptr;
                }
            }
        }

        inline void proc() const {
            for (auto each : mDataItemList) {
                switch (each->getId())
                {
                    case NETWORKINFO_DATA_ITEM_ID:
                    {
                        NetworkInfoDataItemBase* networkInfo =
                                static_cast<NetworkInfoDataItemBase*>(each);
                        mXtraSysStatObj->updateConnections(networkInfo->getAllTypes());
                    }
                    break;

                    case TAC_DATA_ITEM_ID:
                    {
                        TacDataItemBase* tac =
                                 static_cast<TacDataItemBase*>(each);
                        mXtraSysStatObj->updateTac(tac->mValue);
                    }
                    break;

                    case MCCMNC_DATA_ITEM_ID:
                    {
                        MccmncDataItemBase* mccmnc =
                                static_cast<MccmncDataItemBase*>(each);
                        mXtraSysStatObj->updateMccMnc(mccmnc->mValue);
                    }
                    break;

                    default:
                    break;
                }
            }
        }
    };
    mMsgTask->sendMsg(new (nothrow) HandleOsObserverUpdateMsg(this, dlist));
}
