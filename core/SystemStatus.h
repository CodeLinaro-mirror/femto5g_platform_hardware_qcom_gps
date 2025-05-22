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
Changes from Qualcomm Technologies, Inc. are provided under the following license:
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __SYSTEM_STATUS__
#define __SYSTEM_STATUS__

#include <stdint.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <loc_pla.h>
#include <log_util.h>
#include <MsgTask.h>
#include <IDataItemCore.h>
#include <DataItemConcreteTypes.h>
#include <SystemStatusOsObserver.h>

#include <gps_extended_c.h>

//  Per QMI Loc API: qmiLocNavDataStructT_v02
#define GPS_SV_ID_MIN    (1)   //1-32
#define GLO_SV_ID_MIN    (65)  //65-96
#define QZSS_SV_ID_MIN   (193) //193-197
#define BDS_SV_ID_MIN    (201) //201-263
#define GAL_SV_ID_MIN    (301) //301-336
#define NAVIC_SV_ID_MIN  (401) //401-420

#define GPS_SV_ID_MAX    (32)   //1-32
#define GLO_SV_ID_MAX    (96)  //65-96
#define QZSS_SV_ID_MAX   (197) //193-197
#define BDS_SV_ID_MAX    (263) //201-263
#define GAL_SV_ID_MAX    (336) //301-336
#define NAVIC_SV_ID_MAX  (420) //401-420

#define GPS_SV_NUM    (GPS_SV_ID_MAX - GPS_SV_ID_MIN + 1)
#define GLO_SV_NUM    (GLO_SV_ID_MAX - GLO_SV_ID_MIN + 1)
#define QZSS_SV_NUM    (QZSS_SV_ID_MAX - QZSS_SV_ID_MIN + 1)
#define BDS_SV_NUM     (BDS_SV_ID_MAX - BDS_SV_ID_MIN + 1)
#define GAL_SV_NUM     (GAL_SV_ID_MAX - GAL_SV_ID_MIN + 1)
#define NAVIC_SV_NUM   (NAVIC_SV_ID_MAX - NAVIC_SV_ID_MIN + 1)

#define GPS_SV_INDEX_OFFSET   (0)
#define GLO_SV_INDEX_OFFSET   (GPS_SV_INDEX_OFFSET + GPS_SV_NUM)
#define QZSS_SV_INDEX_OFFSET  (GLO_SV_INDEX_OFFSET + GLO_SV_NUM)
#define BDS_SV_INDEX_OFFSET   (QZSS_SV_INDEX_OFFSET + QZSS_SV_NUM)
#define GAL_SV_INDEX_OFFSET   (BDS_SV_INDEX_OFFSET + BDS_SV_NUM)
#define NAVIC_SV_INDEX_OFFSET (GAL_SV_INDEX_OFFSET + GAL_SV_NUM)

#define SV_ALL_NUM  (GPS_SV_NUM + GLO_SV_NUM + QZSS_SV_NUM + \
                  BDS_SV_NUM + GAL_SV_NUM + NAVIC_SV_NUM )

#define GNSS_BUGREPORT_GPS_SV_ID_MIN    (1)
#define GNSS_BUGREPORT_GLO_SV_ID_MIN    (1)
#define GNSS_BUGREPORT_QZSS_SV_ID_MIN   (193)
#define GNSS_BUGREPORT_BDS_SV_ID_MIN    (1)
#define GNSS_BUGREPORT_GAL_SV_ID_MIN    (1)
#define GNSS_BUGREPORT_NAVIC_SV_ID_MIN  (1)

namespace loc_core
{

/******************************************************************************
 SystemStatus report data structure
******************************************************************************/
class SystemStatusItemBase
{
public:
    timespec  mUtcTime;
    timespec  mUtcReported;
    static const uint32_t maxItem = 5;

    SystemStatusItemBase() {
        timeval tv;
        gettimeofday(&tv, NULL);
        mUtcTime.tv_sec  = tv.tv_sec;
        mUtcTime.tv_nsec = tv.tv_usec*1000ULL;
        mUtcReported = mUtcTime;
    };
    virtual ~SystemStatusItemBase() {};
    inline virtual SystemStatusItemBase& collate(SystemStatusItemBase&) {
        return *this;
    }
    virtual void dump(void) {};
    inline virtual bool ignore() { return false; };
    virtual bool equals(const SystemStatusItemBase& peer) { return false; }
};

class SystemStatusLocation : public SystemStatusItemBase
{
public:
    bool mValid;
    UlpLocation mLocation;
    GpsLocationExtended mLocationEx;
    inline SystemStatusLocation() :
        mValid(false) {}
    inline SystemStatusLocation(const UlpLocation& location,
                         const GpsLocationExtended& locationEx) :
        mValid(true),
        mLocation(location),
        mLocationEx(locationEx) {}
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusTimeAndClock : public SystemStatusItemBase
{
public:
    uint16_t mGpsWeek;
    uint32_t mGpsTowMs;
    uint8_t  mTimeValid;
    uint8_t  mTimeSource;
    int32_t  mTimeUnc;
    int32_t  mClockFreqBias;
    int32_t  mClockFreqBiasUnc;
    int32_t  mLeapSeconds;
    int32_t  mLeapSecUnc;
    uint64_t mTimeUncNs;
    inline SystemStatusTimeAndClock() :
        mGpsWeek(0),
        mGpsTowMs(0),
        mTimeValid(0),
        mTimeSource(0),
        mTimeUnc(0),
        mClockFreqBias(0),
        mClockFreqBiasUnc(0),
        mLeapSeconds(0),
        mLeapSecUnc(0),
        mTimeUncNs(0ULL) {}
    inline SystemStatusTimeAndClock(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusXoState : public SystemStatusItemBase
{
public:
    uint8_t  mXoState;
    inline SystemStatusXoState() :
        mXoState(0) {}
    inline SystemStatusXoState(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusRfAndParams : public SystemStatusItemBase
{
public:
    uint32_t mJammedSignalsMask;
    uint32_t mJammerGps;
    uint32_t mJammerGlo;
    uint32_t mJammerBds;
    uint32_t mJammerGal;
    std::vector<int32_t> mJammerInd;

    inline SystemStatusRfAndParams() :
        mJammerGps(0),
        mJammerGlo(0),
        mJammerBds(0),
        mJammerGal(0),
        mJammedSignalsMask(0) {}
    inline SystemStatusRfAndParams(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusErrRecovery : public SystemStatusItemBase
{
public:
    uint32_t mRecErrorRecovery;
    inline SystemStatusErrRecovery() :
        mRecErrorRecovery(0) {};
    inline SystemStatusErrRecovery(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    inline bool ignore() override { return 0 == mRecErrorRecovery; };
    void dump(void) override;
};

class SystemStatusInjectedPosition : public SystemStatusItemBase
{
public:
    uint8_t  mEpiValidity;
    float    mEpiLat;
    float    mEpiLon;
    float    mEpiAlt;
    float    mEpiHepe;
    float    mEpiAltUnc;
    uint8_t  mEpiSrc;
    inline SystemStatusInjectedPosition() :
        mEpiValidity(0),
        mEpiLat(0),
        mEpiLon(0),
        mEpiAlt(0),
        mEpiHepe(0),
        mEpiAltUnc(0),
        mEpiSrc(0) {}
    inline SystemStatusInjectedPosition(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusBestPosition : public SystemStatusItemBase
{
public:
    bool     mValid;
    float    mBestLat;
    float    mBestLon;
    float    mBestAlt;
    float    mBestHepe;
    float    mBestAltUnc;
    inline SystemStatusBestPosition() :
        mValid(false),
        mBestLat(0),
        mBestLon(0),
        mBestAlt(0),
        mBestHepe(0),
        mBestAltUnc(0) {}
    inline SystemStatusBestPosition(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusXtra : public SystemStatusItemBase
{
public:
    uint8_t   mXtraValidMask;
    uint32_t  mGpsXtraAge;
    uint32_t  mGloXtraAge;
    uint32_t  mBdsXtraAge;
    uint32_t  mGalXtraAge;
    uint32_t  mQzssXtraAge;
    uint32_t  mNavicXtraAge;
    uint32_t  mGpsXtraValid;
    uint32_t  mGloXtraValid;
    uint64_t  mBdsXtraValid;
    uint64_t  mGalXtraValid;
    uint8_t   mQzssXtraValid;
    uint32_t  mNavicXtraValid;
    inline SystemStatusXtra() :
        mXtraValidMask(0),
        mGpsXtraAge(0),
        mGloXtraAge(0),
        mBdsXtraAge(0),
        mGalXtraAge(0),
        mQzssXtraAge(0),
        mNavicXtraAge(0),
        mGpsXtraValid(0),
        mGloXtraValid(0),
        mBdsXtraValid(0ULL),
        mGalXtraValid(0ULL),
        mQzssXtraValid(0),
        mNavicXtraValid(0) {}
    inline SystemStatusXtra(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusEphemeris : public SystemStatusItemBase
{
public:
    uint32_t  mGpsEpheValid;
    uint32_t  mGloEpheValid;
    uint64_t  mBdsEpheValid;
    uint64_t  mGalEpheValid;
    uint8_t   mQzssEpheValid;
    uint32_t  mNavicEpheValid;

    inline SystemStatusEphemeris() :
        mGpsEpheValid(0),
        mGloEpheValid(0),
        mBdsEpheValid(0ULL),
        mGalEpheValid(0ULL),
        mQzssEpheValid(0),
        mNavicEpheValid(0) {}

    inline SystemStatusEphemeris(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusSvHealth : public SystemStatusItemBase
{
public:
    uint32_t  mGpsUnknownMask;
    uint32_t  mGloUnknownMask;
    uint64_t  mBdsUnknownMask;
    uint64_t  mGalUnknownMask;
    uint8_t   mQzssUnknownMask;
    uint32_t  mNavicUnknownMask;
    uint32_t  mGpsGoodMask;
    uint32_t  mGloGoodMask;
    uint64_t  mBdsGoodMask;
    uint64_t  mGalGoodMask;
    uint8_t   mQzssGoodMask;
    uint32_t  mNavicGoodMask;
    uint32_t  mGpsBadMask;
    uint32_t  mGloBadMask;
    uint64_t  mBdsBadMask;
    uint64_t  mGalBadMask;
    uint8_t   mQzssBadMask;
    uint32_t  mNavicBadMask;
    inline SystemStatusSvHealth() :
        mGpsUnknownMask(0),
        mGloUnknownMask(0),
        mBdsUnknownMask(0ULL),
        mGalUnknownMask(0ULL),
        mQzssUnknownMask(0),
        mNavicUnknownMask(0),
        mGpsGoodMask(0),
        mGloGoodMask(0),
        mBdsGoodMask(0ULL),
        mGalGoodMask(0ULL),
        mQzssGoodMask(0),
        mNavicGoodMask(0),
        mGpsBadMask(0),
        mGloBadMask(0),
        mBdsBadMask(0ULL),
        mGalBadMask(0ULL),
        mQzssBadMask(0),
        mNavicBadMask(0) {}
    inline SystemStatusSvHealth(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusPdr : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    inline SystemStatusPdr() :
        mFixInfoMask(0) {}
    inline SystemStatusPdr(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;

};

struct SystemStatusNav
{
    GnssEphemerisType   mType;
    GnssEphemerisSource mSource;
    int32_t             mAgeSec;
};

class SystemStatusNavData : public SystemStatusItemBase
{
public:
    SystemStatusNav mNav[SV_ALL_NUM];
    inline SystemStatusNavData() {
        // GNSS_EPH_TYPE_UNKNOWN and GNSS_EPH_TYPE_UNKNOWN are 0
        memset(mNav, 0, sizeof (mNav));
    }

    inline SystemStatusNavData(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

class SystemStatusPositionFailure : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    uint32_t  mHepeLimit;
    inline SystemStatusPositionFailure() :
        mFixInfoMask(0),
        mHepeLimit(0) {}
    inline SystemStatusPositionFailure(const GnssEngineDebugDataInfo& info);
    bool equals(const SystemStatusItemBase& peer) override;
    void dump(void) override;
};

/******************************************************************************
 SystemStatusReports
******************************************************************************/
class SystemStatusReports
{
public:
    // from QMI_LOC indication
    std::vector<SystemStatusLocation>         mLocation;

    // from ME debug info
    std::vector<SystemStatusTimeAndClock>     mTimeAndClock;
    std::vector<SystemStatusXoState>          mXoState;
    std::vector<SystemStatusRfAndParams>      mRfAndParams;
    std::vector<SystemStatusErrRecovery>      mErrRecovery;

    // from PE debug info
    std::vector<SystemStatusInjectedPosition> mInjectedPosition;
    std::vector<SystemStatusBestPosition>     mBestPosition;
    std::vector<SystemStatusXtra>             mXtra;
    std::vector<SystemStatusEphemeris>        mEphemeris;
    std::vector<SystemStatusSvHealth>         mSvHealth;
    std::vector<SystemStatusPdr>              mPdr;
    std::vector<SystemStatusNavData>          mNavData;

    // from SM debug info
    std::vector<SystemStatusPositionFailure>  mPositionFailure;

};

/******************************************************************************
 SystemStatus
******************************************************************************/
class SystemStatus
{
private:
    static SystemStatus                       *mInstance;
    SystemStatusOsObserver*                    mSysStatusObsvr;
    // ctor
    SystemStatus(const MsgTask* msgTask);
    // dtor
    inline ~SystemStatus() {}

    // Data members
    static pthread_mutex_t                    mMutexSystemStatus;
    SystemStatusReports mCache;
    bool mTracking;

    template <typename TYPE_REPORT, typename TYPE_ITEM>
    bool setIteminReport(TYPE_REPORT& report, TYPE_ITEM&& s);

    // set default dataitem derived item in report cache
    template <typename TYPE_REPORT, typename TYPE_ITEM>
    void setDefaultIteminReport(TYPE_REPORT& report, const TYPE_ITEM& s);

    template <typename TYPE_REPORT, typename TYPE_ITEM>
    void getIteminReport(TYPE_REPORT& reportout, const TYPE_ITEM& c) const;

public:
    // Static methods
    static SystemStatus* getInstance(const MsgTask* msgTask);
    static void destroyInstance();
    SystemStatusOsObserver* getOsObserver();
    void resetNetworkInfo();

    // Helpers
    bool eventPosition(const UlpLocation& location,const GpsLocationExtended& locationEx);
    bool eventDataItemNotify(IDataItemCore* dataitem);
    void setEngineDebugDataInfo(const GnssEngineDebugDataInfo& gnssEngineDebugDataInfo);
    bool getReport(SystemStatusReports& reports, bool isLatestonly = false,
            bool inSessionOnly = true) const;
    bool setDefaultGnssEngineStates(void);
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

