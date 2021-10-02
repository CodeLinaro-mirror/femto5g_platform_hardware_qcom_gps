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
#ifndef XTRA_SYSTEM_STATUS_OBS_H
#define XTRA_SYSTEM_STATUS_OBS_H

#include <cinttypes>
#include <MsgTask.h>
#include <LocIpc.h>
#include <LocTimer.h>

using namespace std;
using namespace loc_util;
using loc_core::IOsObserver;
using loc_core::IDataItemObserver;
using loc_core::IDataItemCore;

struct StartDgnssNtripParams {
    GnssNtripConnectionParams ntripParams;
    string                    nmea;

    void clear() {
        ntripParams.hostNameOrIp.clear();
        ntripParams.mountPoint.clear();
        ntripParams.username.clear();
        ntripParams.password.clear();
        ntripParams.port = 0;
        ntripParams.useSSL = false;
        ntripParams.requiresNmeaLocation = false;
        nmea.clear();
    }
};

#ifdef USE_GLIB
typedef uint32_t  NetConnectReqBitMask;
#define NO_CONNECT_REQ        0X00
#define XTRA_CONNECT_REQ      0X01
#define NTRP_CONNECT_REQ      0X02

class XtraSystemStatusObserver;

class NetConnectTimer : public LocTimer {
public:
    NetConnectTimer(XtraSystemStatusObserver&  XtraSSO,
                    const MsgTask& msgTask) :
        mXtraSSO(XtraSSO), mMsgTask(msgTask) {}

    ~NetConnectTimer() = default;

    virtual void timeOutCallback() override;

private:
    XtraSystemStatusObserver&    mXtraSSO;
    const MsgTask&               mMsgTask;
};
#endif

class GnssAdapter;

class XtraSystemStatusObserver : public IDataItemObserver {
public :
    // constructor & destructor
    XtraSystemStatusObserver(GnssAdapter* adapter, IOsObserver* sysStatObs, const MsgTask* msgTask);
    inline virtual ~XtraSystemStatusObserver() {
        subscribe(false);
        mIpc.stopNonBlockingListening();
    }

    // IDataItemObserver overrides
    inline virtual void getName(string& name);
    virtual void notify(const list<IDataItemCore*>& dlist);

    bool updateLockStatus(GnssConfigGpsLock lock);
    bool updateConnections(uint64_t allConnections);
    bool updateTac(const string& tac);
    bool updateMccMnc(const string& mccmnc);
    bool updateXtraThrottle(const bool enabled);
    bool updatePowerState(const PowerStateType powerState);
    inline const MsgTask* getMsgTask() { return mMsgTask; }
    void subscribe(bool yes);
    bool onStatusRequested(int32_t xtraStatusUpdated);
    void startDgnssSource(const StartDgnssNtripParams& params);
    void restartDgnssSource();
    void stopDgnssSource();
    void updateNmeaToDgnssServer(const string& nmea);
    bool updateXtraConfig(bool enabled, const XtraConfigParams& configParams);
    bool getXtraStatus(uint32_t sessionId);
    bool registerXtraStatusUpdate(uint32_t sessionId, bool registerUpdate);
    bool updateXtraDataDeletion();

#ifdef USE_GLIB
    void connectNetwork(NetConnectReqBitMask conMask);
    void disconnectNetwork(NetConnectReqBitMask conMask);
    BackhaulContext mBackhaulCtx;
#endif

private:
    GnssAdapter*   mAdapter;
    IOsObserver*   mSystemStatusObsrvr;
    const MsgTask* mMsgTask;
    GnssConfigGpsLock mGpsLock;
    LocIpc mIpc;
    uint64_t mConnections;
    string mTac;
    string mMccmnc;
    bool mXtraThrottle;
    PowerStateType mPowerState;
    bool mReqStatusReceived;
    bool mIsConnectivityStatusKnown;
    shared_ptr<LocIpcSender> mSender;
    string mNtripParamsString;
    bool mRegisterForXtraStatus;

#ifdef USE_GLIB
    NetConnectReqBitMask mNetReqMask;
    NetConnectTimer mNetConnectTimer;
    string mClientName;
#endif

    class DelayLocTimer : public LocTimer {
        LocIpcSender& mSender;
    public:
        DelayLocTimer(LocIpcSender& sender) : mSender(sender) {}
        virtual void timeOutCallback() override {
            LocIpc::send(mSender, (const uint8_t*)"halinit", sizeof("halinit"));
        }
    } mDelayLocTimer;

#ifdef USE_GLIB
friend class XtraIpcListener;
#endif
};

#endif
