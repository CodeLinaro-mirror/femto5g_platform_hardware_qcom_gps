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

#define LOG_TAG "LocSvc_SystemStatus"

#include <inttypes.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <loc_pla.h>
#include <log_util.h>
#include <loc_nmea.h>
#include <SystemStatus.h>
#include <SystemStatusOsObserver.h>
#include <DataItemConcreteTypes.h>

namespace loc_core
{
/******************************************************************************
 SystemStatusTimeAndClock
******************************************************************************/
SystemStatusTimeAndClock::SystemStatusTimeAndClock(const GnssEngineDebugDataInfo& info) :
    mGpsWeek(info.gpsWeek),
    mGpsTowMs(info.gpsTowMs),
    mTimeValid(info.timeValid),
    mTimeSource(info.sourceOfTime),
    mTimeUnc(info.clkTimeUnc),
    mClockFreqBias(info.clkFreqBias),
    mClockFreqBiasUnc(info.clkFreqUnc),
    mLeapSeconds(info.leapSecondInfo.leapSec),
    mLeapSecUnc(info.leapSecondInfo.leapSecUnc),
    mTimeUncNs(info.clkTimeUnc * 1000000LL)
{
}

bool SystemStatusTimeAndClock::equals(const SystemStatusItemBase& peer) {
    if ((mGpsWeek != ((const SystemStatusTimeAndClock&)peer).mGpsWeek) ||
        (mGpsTowMs != ((const SystemStatusTimeAndClock&)peer).mGpsTowMs) ||
        (mTimeValid != ((const SystemStatusTimeAndClock&)peer).mTimeValid) ||
        (mTimeSource != ((const SystemStatusTimeAndClock&)peer).mTimeSource) ||
        (mTimeUnc != ((const SystemStatusTimeAndClock&)peer).mTimeUnc) ||
        (mClockFreqBias != ((const SystemStatusTimeAndClock&)peer).mClockFreqBias) ||
        (mClockFreqBiasUnc != ((const SystemStatusTimeAndClock&)peer).mClockFreqBiasUnc) ||
        (mLeapSeconds != ((const SystemStatusTimeAndClock&)peer).mLeapSeconds) ||
        (mLeapSecUnc != ((const SystemStatusTimeAndClock&)peer).mLeapSecUnc) ||
        (mTimeUncNs != ((const SystemStatusTimeAndClock&)peer).mTimeUncNs)) {
        return false;
    }
    return true;
}

void SystemStatusTimeAndClock::dump()
{
    LOC_LOGV("TimeAndClock: u=%ld:%ld g=%d:%d v=%d ts=%d tu=%d b=%d bu=%d ls=%d lu=%d un=%" PRIu64,
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mGpsWeek,
             mGpsTowMs,
             mTimeValid,
             mTimeSource,
             mTimeUnc,
             mClockFreqBias,
             mClockFreqBiasUnc,
             mLeapSeconds,
             mLeapSecUnc,
             mTimeUncNs);
}

/******************************************************************************
 SystemStatusXoState
******************************************************************************/
SystemStatusXoState::SystemStatusXoState(const GnssEngineDebugDataInfo& info) :
    mXoState(info.xoState),
    mXoTemp(info.xoTemp),
    mXoTempSlope(info.xoTempSlope),
    mXoTempAccel(info.xoTempAccel),
    mXoCalResetCount(info.xoCalResetCount),
    mXoRotatorQuality(info.xoRotatorQuality),
    mTimeInconsistencyStatus(info.timeInconsistencyStatus)
{
}

bool SystemStatusXoState::equals(const SystemStatusItemBase& peer) {
    if (mXoState != ((const SystemStatusXoState&)peer).mXoState) {
        return false;
    }
    return true;
}

void SystemStatusXoState::dump()
{
    LOC_LOGV("XoState: u=%ld:%ld x=%d",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mXoState);
}

/******************************************************************************
 SystemStatusRfAndParams
******************************************************************************/
SystemStatusRfAndParams::SystemStatusRfAndParams(const GnssEngineDebugDataInfo& info) :
    mJammedSignalsMask(info.jammedSignalsMask),
    mJammerGps(0),
    mJammerGlo(0),
    mJammerBds(0),
    mJammerGal(0) {

    if (info.jammerInd.size() > 0) {
         mJammerGps = info.jammerInd[GNSS_LOC_SIGNAL_TYPE_GPS_L1CA];
         mJammerGlo = info.jammerInd[GNSS_LOC_SIGNAL_TYPE_GLONASS_G1];
         mJammerBds = info.jammerInd[GNSS_LOC_SIGNAL_TYPE_BEIDOU_B1_I];
         mJammerGal = info.jammerInd[GNSS_LOC_SIGNAL_TYPE_GALILEO_E1_C];
    }

    mJammerInd = std::move(info.jammerInd);
}

bool SystemStatusRfAndParams::equals(const SystemStatusItemBase& peer) {
    if ((mJammerGps != ((const SystemStatusRfAndParams&)peer).mJammerGps) ||
        (mJammerGlo != ((const SystemStatusRfAndParams&)peer).mJammerGlo) ||
        (mJammerBds != ((const SystemStatusRfAndParams&)peer).mJammerBds) ||
        (mJammerGal != ((const SystemStatusRfAndParams&)peer).mJammerGal) ||
        (mJammedSignalsMask != ((const SystemStatusRfAndParams&)peer).mJammedSignalsMask)) {
        return false;
    }
    return true;
}

void SystemStatusRfAndParams::dump()
{
    LOC_LOGV("RfAndParams: u=%ld:%ld jgp=%d jgl=%d jbd=%d jga=%d ",
             mUtcTime.tv_sec, mUtcTime.tv_nsec, mJammerGps, mJammerGlo,
             mJammerBds, mJammerGal);
}

/******************************************************************************
 SystemStatusErrRecovery
******************************************************************************/
SystemStatusErrRecovery::SystemStatusErrRecovery(const GnssEngineDebugDataInfo& info) :
    mRecErrorRecovery(info.rcvrErrRecovery)
{
}

bool SystemStatusErrRecovery::equals(const SystemStatusItemBase& peer) {
    if (mRecErrorRecovery != ((const SystemStatusErrRecovery&)peer).mRecErrorRecovery) {
        return false;
    }
    return true;
}

void SystemStatusErrRecovery::dump()
{
    LOC_LOGV("ErrRecovery: u=%ld:%ld e=%d",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mRecErrorRecovery);
}

/******************************************************************************
 SystemStatusInjectedPosition
******************************************************************************/
SystemStatusInjectedPosition::SystemStatusInjectedPosition(const GnssEngineDebugDataInfo& info) :
    mEpiValidity(info.epiValidity),
    mEpiLat(info.epiLat),
    mEpiLon(info.epiLon),
    mEpiAlt(info.epiAlt),
    mEpiHepe(info.epiHepe),
    mEpiAltUnc(info.epiAltUnc),
    mEpiSrc(info.epiSrc)
{
}

bool SystemStatusInjectedPosition::equals(const SystemStatusItemBase& peer) {
    if ((mEpiValidity != ((const SystemStatusInjectedPosition&)peer).mEpiValidity) ||
        (mEpiLat != ((const SystemStatusInjectedPosition&)peer).mEpiLat) ||
        (mEpiLon != ((const SystemStatusInjectedPosition&)peer).mEpiLon) ||
        (mEpiAlt != ((const SystemStatusInjectedPosition&)peer).mEpiAlt) ||
        (mEpiHepe != ((const SystemStatusInjectedPosition&)peer).mEpiHepe) ||
        (mEpiAltUnc != ((const SystemStatusInjectedPosition&)peer).mEpiAltUnc) ||
        (mEpiSrc != ((const SystemStatusInjectedPosition&)peer).mEpiSrc)) {
        return false;
    }
    return true;
}

void SystemStatusInjectedPosition::dump()
{
    LOC_LOGV("InjectedPosition: u=%ld:%ld v=%x la=%f lo=%f al=%f he=%f au=%f es=%d",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mEpiValidity,
             mEpiLat,
             mEpiLon,
             mEpiAlt,
             mEpiHepe,
             mEpiAltUnc,
             mEpiSrc);
}

/******************************************************************************
 SystemStatusBestPosition
******************************************************************************/
SystemStatusBestPosition::SystemStatusBestPosition(const GnssEngineDebugDataInfo& info) :
    mValid(true),
    mBestLat(info.bestPosLat),
    mBestLon(info.bestPosLon),
    mBestAlt(info.bestPosAlt),
    mBestHepe(info.bestPosHepe),
    mBestAltUnc(info.bestPosAltUnc)
{
}

bool SystemStatusBestPosition::equals(const SystemStatusItemBase& peer) {
    if ((mBestLat != ((const SystemStatusBestPosition&)peer).mBestLat) ||
        (mBestLon != ((const SystemStatusBestPosition&)peer).mBestLon) ||
        (mBestAlt != ((const SystemStatusBestPosition&)peer).mBestAlt) ||
        (mBestHepe != ((const SystemStatusBestPosition&)peer).mBestHepe) ||
        (mBestAltUnc != ((const SystemStatusBestPosition&)peer).mBestAltUnc)) {
        return false;
    }
    return true;
}

void SystemStatusBestPosition::dump()
{
    LOC_LOGV("BestPosition: u=%ld:%ld la=%f lo=%f al=%f he=%f au=%f",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mBestLat,
             mBestLon,
             mBestAlt,
             mBestHepe,
             mBestAltUnc);
}

/******************************************************************************
 SystemStatusXtra
******************************************************************************/
SystemStatusXtra::SystemStatusXtra(const GnssEngineDebugDataInfo& info) :
    mXtraValidMask(info.xtraValidMask),
    mGpsXtraAge(info.gpsXtraAge),
    mGloXtraAge(info.gloXtraAge),
    mBdsXtraAge(info.bdsXtraAge),
    mGalXtraAge(info.galXtraAge),
    mQzssXtraAge(info.qzssXtraAge),
    mNavicXtraAge(info.navicXtraAge),
    mGpsXtraValid(info.gpsXtraMask),
    mGloXtraValid(info.gloXtraMask),
    mBdsXtraValid(info.bdsXtraMask),
    mGalXtraValid(info.galXtraMask),
    mQzssXtraValid(info.qzssXtraMask),
    mNavicXtraValid(info.navicXtraMask)
{
}

bool SystemStatusXtra::equals(const SystemStatusItemBase& peer) {
    if ((mXtraValidMask != ((const SystemStatusXtra&)peer).mXtraValidMask) ||
        (mGpsXtraAge != ((const SystemStatusXtra&)peer).mGpsXtraAge) ||
        (mGloXtraAge != ((const SystemStatusXtra&)peer).mGloXtraAge) ||
        (mBdsXtraAge != ((const SystemStatusXtra&)peer).mBdsXtraAge) ||
        (mGalXtraAge != ((const SystemStatusXtra&)peer).mGalXtraAge) ||
        (mQzssXtraAge != ((const SystemStatusXtra&)peer).mQzssXtraAge) ||
        (mNavicXtraAge != ((const SystemStatusXtra&)peer).mNavicXtraAge) ||
        (mGpsXtraValid != ((const SystemStatusXtra&)peer).mGpsXtraValid) ||
        (mGloXtraValid != ((const SystemStatusXtra&)peer).mGloXtraValid) ||
        (mBdsXtraValid != ((const SystemStatusXtra&)peer).mBdsXtraValid) ||
        (mGalXtraValid != ((const SystemStatusXtra&)peer).mGalXtraValid) ||
        (mQzssXtraValid != ((const SystemStatusXtra&)peer).mQzssXtraValid) ||
        (mNavicXtraValid != ((const SystemStatusXtra&)peer).mNavicXtraValid)) {
        return false;
    }
    return true;
}

void SystemStatusXtra::dump()
{
    LOC_LOGV("SystemStatusXtra: u=%ld:%ld m=%x a=%d:%d:%d:%d:%d v=%x:%x:%" PRIx64 ":%" PRIx64":%x",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mXtraValidMask,
             mGpsXtraAge,
             mGloXtraAge,
             mBdsXtraAge,
             mGalXtraAge,
             mQzssXtraAge,
             mGpsXtraValid,
             mGloXtraValid,
             mBdsXtraValid,
             mGalXtraValid,
             mQzssXtraValid);
}

/******************************************************************************
 SystemStatusEphemeris
******************************************************************************/
SystemStatusEphemeris::SystemStatusEphemeris(const GnssEngineDebugDataInfo& info) :
    mGpsEpheValid(info.gpsEphMask),
    mGloEpheValid(info.gloEphMask),
    mBdsEpheValid(info.bdsEphMask),
    mGalEpheValid(info.galEphMask),
    mQzssEpheValid(info.qzssEphMask),
    mNavicEpheValid(info.navicEphMask)
{
}

bool SystemStatusEphemeris::equals(const SystemStatusItemBase& peer) {
    if ((mGpsEpheValid != ((const SystemStatusEphemeris&)peer).mGpsEpheValid) ||
        (mGloEpheValid != ((const SystemStatusEphemeris&)peer).mGloEpheValid) ||
        (mBdsEpheValid != ((const SystemStatusEphemeris&)peer).mBdsEpheValid) ||
        (mGalEpheValid != ((const SystemStatusEphemeris&)peer).mGalEpheValid) ||
        (mQzssEpheValid != ((const SystemStatusEphemeris&)peer).mQzssEpheValid) ||
        (mNavicEpheValid != ((const SystemStatusEphemeris&)peer).mNavicEpheValid)) {
        return false;
    }
    return true;
}

void SystemStatusEphemeris::dump()
{
    LOC_LOGV("Ephemeris: u=%ld:%ld ev=%x:%x:%" PRIx64 ":%" PRIx64 ":%x%x",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mGpsEpheValid,
             mGloEpheValid,
             mBdsEpheValid,
             mGalEpheValid,
             mQzssEpheValid,
             mNavicEpheValid);
}

/******************************************************************************
 SystemStatusSvHealth
******************************************************************************/
SystemStatusSvHealth::SystemStatusSvHealth(const GnssEngineDebugDataInfo& info) :
    mGpsUnknownMask(info.gpsHealthUnknownMask),
    mGloUnknownMask(info.gloHealthUnknownMask),
    mBdsUnknownMask(info.bdsHealthUnknownMask),
    mGalUnknownMask(info.galHealthUnknownMask),
    mQzssUnknownMask(info.qzssHealthUnknownMask),
    mNavicUnknownMask(info.navicHealthUnknownMask),
    mGpsGoodMask(info.gpsHealthGoodMask),
    mGloGoodMask(info.gloHealthGoodMask),
    mBdsGoodMask(info.bdsHealthGoodMask),
    mGalGoodMask(info.galHealthGoodMask),
    mQzssGoodMask(info.qzssHealthGoodMask),
    mNavicGoodMask(info.navicHealthGoodMask),
    mGpsBadMask(info.gpsHealthBadMask),
    mGloBadMask(info.gloHealthBadMask),
    mBdsBadMask(info.bdsHealthBadMask),
    mGalBadMask(info.galHealthBadMask),
    mQzssBadMask(info.qzssHealthBadMask),
    mNavicBadMask(info.navicHealthBadMask)
{
}

bool SystemStatusSvHealth::equals(const SystemStatusItemBase& peer) {
    if ((mGpsUnknownMask != ((const SystemStatusSvHealth&)peer).mGpsUnknownMask) ||
        (mGloUnknownMask != ((const SystemStatusSvHealth&)peer).mGloUnknownMask) ||
        (mBdsUnknownMask != ((const SystemStatusSvHealth&)peer).mBdsUnknownMask) ||
        (mGalUnknownMask != ((const SystemStatusSvHealth&)peer).mGalUnknownMask) ||
        (mQzssUnknownMask != ((const SystemStatusSvHealth&)peer).mQzssUnknownMask) ||
        (mNavicUnknownMask != ((const SystemStatusSvHealth&)peer).mNavicUnknownMask) ||
        (mGpsGoodMask != ((const SystemStatusSvHealth&)peer).mGpsGoodMask) ||
        (mGloGoodMask != ((const SystemStatusSvHealth&)peer).mGloGoodMask) ||
        (mBdsGoodMask != ((const SystemStatusSvHealth&)peer).mBdsGoodMask) ||
        (mGalGoodMask != ((const SystemStatusSvHealth&)peer).mGalGoodMask) ||
        (mQzssGoodMask != ((const SystemStatusSvHealth&)peer).mQzssGoodMask) ||
        (mNavicGoodMask != ((const SystemStatusSvHealth&)peer).mNavicGoodMask) ||
        (mGpsBadMask != ((const SystemStatusSvHealth&)peer).mGpsBadMask) ||
        (mGloBadMask != ((const SystemStatusSvHealth&)peer).mGloBadMask) ||
        (mBdsBadMask != ((const SystemStatusSvHealth&)peer).mBdsBadMask) ||
        (mGalBadMask != ((const SystemStatusSvHealth&)peer).mGalBadMask) ||
        (mQzssBadMask != ((const SystemStatusSvHealth&)peer).mQzssBadMask) ||
        (mNavicBadMask != ((const SystemStatusSvHealth&)peer).mNavicBadMask)) {
        return false;
    }
    return true;
}

void SystemStatusSvHealth::dump()
{
    LOC_LOGV("SvHealth: u=%ld:%ld \
             u=%x:%x:%" PRIx64 ":%" PRIx64 ":%x:%x\
             g=%x:%x:%" PRIx64 ":%" PRIx64 ":%x:%x \
             b=%x:%x:%" PRIx64 ":%" PRIx64 ":%x:%x",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mGpsUnknownMask,
             mGloUnknownMask,
             mBdsUnknownMask,
             mGalUnknownMask,
             mQzssUnknownMask,
             mNavicUnknownMask,
             mGpsGoodMask,
             mGloGoodMask,
             mBdsGoodMask,
             mGalGoodMask,
             mQzssGoodMask,
             mNavicGoodMask,
             mGpsBadMask,
             mGloBadMask,
             mBdsBadMask,
             mGalBadMask,
             mQzssBadMask,
             mNavicBadMask);
}

/******************************************************************************
 SystemStatusPdr
******************************************************************************/
SystemStatusPdr::SystemStatusPdr(const GnssEngineDebugDataInfo& info) :
    mFixInfoMask(info.fixInfoMask)
{
}

bool SystemStatusPdr::equals(const SystemStatusItemBase& peer) {
    if (mFixInfoMask != ((const SystemStatusPdr&)peer).mFixInfoMask) {
        return false;
    }
    return true;
}

void SystemStatusPdr::dump()
{
    LOC_LOGV("Pdr: u=%ld:%ld m=%x",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mFixInfoMask);
}

/******************************************************************************
 SystemStatusNavData
******************************************************************************/
SystemStatusNavData::SystemStatusNavData(const GnssEngineDebugDataInfo& info)
{
   memset(mNav, 0, sizeof(mNav));
   for (int i = 0; i < info.navDataLen; i++) {
        int offset = 0;
        GnssNavDataInfo navInfo  = info.navData[i];

        if (0 == navInfo.gnssSvId) continue;
        if (navInfo.gnssSvType == GNSS_SV_TYPE_GPS) {
            offset = GPS_SV_INDEX_OFFSET + navInfo.gnssSvId - GPS_SV_ID_MIN;
        } else if (navInfo.gnssSvType == GNSS_SV_TYPE_GLONASS) {
            offset = GLO_SV_INDEX_OFFSET + navInfo.gnssSvId - GLO_SV_ID_MIN;
        } else if (navInfo.gnssSvType == GNSS_SV_TYPE_BEIDOU) {
            offset = BDS_SV_INDEX_OFFSET + navInfo.gnssSvId - BDS_SV_ID_MIN;
        } else if (navInfo.gnssSvType == GNSS_SV_TYPE_GALILEO) {
            offset = GAL_SV_INDEX_OFFSET + navInfo.gnssSvId - GAL_SV_ID_MIN;
        } else if (navInfo.gnssSvType == GNSS_SV_TYPE_QZSS) {
            offset = QZSS_SV_INDEX_OFFSET + navInfo.gnssSvId - QZSS_SV_ID_MIN;
        } else if (navInfo.gnssSvType ==GNSS_SV_TYPE_NAVIC) {
            offset = NAVIC_SV_INDEX_OFFSET + navInfo.gnssSvId - NAVIC_SV_ID_MIN;
        }
        mNav[offset].mSvType = navInfo.gnssSvType;
        mNav[offset].mSvId   = navInfo.gnssSvId;
        mNav[offset].mType   = GnssEphemerisType(navInfo.type);
        mNav[offset].mSource = GnssEphemerisSource(navInfo.src);
        mNav[offset].mAgeSec = navInfo.age;
   }
}

bool SystemStatusNavData::equals(const SystemStatusItemBase& peer) {
    return !memcmp (&mNav, &((const SystemStatusNavData&)peer).mNav, sizeof(mNav));
}

void SystemStatusNavData::dump()
{
    LOC_LOGV("NavData: u=%ld:%ld",
            mUtcTime.tv_sec, mUtcTime.tv_nsec);
    for (uint32_t i=0; i<SV_ALL_NUM; i++) {
        LOC_LOGV("i=%d system=%d id=%d type=%d src=%d age=%d",
            i, mNav[i].mSvType, mNav[i].mSvId, mNav[i].mType,
            mNav[i].mSource, mNav[i].mAgeSec);
    }
}

/******************************************************************************
 SystemStatusPositionFailure
******************************************************************************/
SystemStatusPositionFailure::SystemStatusPositionFailure(const GnssEngineDebugDataInfo& info) :
    mFixInfoMask(info.fixStatusMask),
    mHepeLimit(info.fixHepeLimit)
{
}

bool SystemStatusPositionFailure::equals(const SystemStatusItemBase& peer) {
    if ((mFixInfoMask != ((const SystemStatusPositionFailure&)peer).mFixInfoMask) ||
        (mHepeLimit != ((const SystemStatusPositionFailure&)peer).mHepeLimit)) {
        return false;
    }
    return true;
}

void SystemStatusPositionFailure::dump()
{
    LOC_LOGV("PositionFailure: u=%ld:%ld m=%d h=%d",
             mUtcTime.tv_sec, mUtcTime.tv_nsec,
             mFixInfoMask,
             mHepeLimit);
}

/******************************************************************************
 SystemStatusLocation
******************************************************************************/
bool SystemStatusLocation::equals(const SystemStatusItemBase& peer) {
    if ((mLocation.gpsLocation.latitude !=
                ((const SystemStatusLocation&)peer).mLocation.gpsLocation.latitude) ||
        (mLocation.gpsLocation.longitude !=
                ((const SystemStatusLocation&)peer).mLocation.gpsLocation.longitude) ||
        (mLocation.gpsLocation.altitude !=
                ((const SystemStatusLocation&)peer).mLocation.gpsLocation.altitude)) {
        return false;
    }
    return true;
}

void SystemStatusLocation::dump()
{
    LOC_LOGV("Location: lat=%f lon=%f alt=%f spd=%f",
             mLocation.gpsLocation.latitude,
             mLocation.gpsLocation.longitude,
             mLocation.gpsLocation.altitude,
             mLocation.gpsLocation.speed);
}

/******************************************************************************
 SystemStatus
******************************************************************************/
pthread_mutex_t   SystemStatus::mMutexSystemStatus = PTHREAD_MUTEX_INITIALIZER;
SystemStatus*     SystemStatus::mInstance = NULL;

SystemStatus* SystemStatus::getInstance(const MsgTask* msgTask)
{
    pthread_mutex_lock(&mMutexSystemStatus);

    if (!mInstance) {
        // Instantiating for the first time. msgTask should not be NULL
        if (msgTask == NULL) {
            LOC_LOGE("SystemStatus: msgTask is NULL!!");
            pthread_mutex_unlock(&mMutexSystemStatus);
            return NULL;
        }
        mInstance = new (nothrow) SystemStatus(msgTask);
        LOC_LOGD("SystemStatus::getInstance:%p. Msgtask:%p", mInstance, msgTask);
    }

    pthread_mutex_unlock(&mMutexSystemStatus);
    return mInstance;
}

void SystemStatus::destroyInstance()
{
    delete mInstance;
    mInstance = NULL;
}

SystemStatusOsObserver* SystemStatus::getOsObserver()
{
    return mSysStatusObsvr;
}

SystemStatus::SystemStatus(const MsgTask* msgTask) :
    mSysStatusObsvr(SystemStatusOsObserver::getInstance(msgTask)), mTracking(false) {
    int result = 0;
    ENTRY_LOG ();
    mCache.mLocation.clear();

    mCache.mTimeAndClock.clear();
    mCache.mXoState.clear();
    mCache.mRfAndParams.clear();
    mCache.mErrRecovery.clear();

    mCache.mInjectedPosition.clear();
    mCache.mBestPosition.clear();
    mCache.mXtra.clear();
    mCache.mEphemeris.clear();
    mCache.mSvHealth.clear();
    mCache.mPdr.clear();
    mCache.mNavData.clear();

    mCache.mPositionFailure.clear();

    EXIT_LOG_WITH_ERROR ("%d",result);
}

/******************************************************************************
 SystemStatus - storing dataitems
******************************************************************************/
template <typename TYPE_REPORT, typename TYPE_ITEM>
bool SystemStatus::setIteminReport(TYPE_REPORT& report, TYPE_ITEM&& s)
{
    if (s.ignore()) {
        return false;
    }
    if (!report.empty() && report.back().equals(static_cast<TYPE_ITEM&>(s.collate(report.back())))) {
        // there is no change - just update reported timestamp
        report.back().mUtcReported = s.mUtcReported;
        return false;
    }

    // first event or updated
    report.push_back(s);
    if (report.size() > s.maxItem) {
        report.erase(report.begin());
    }
    return true;
}

template <typename TYPE_REPORT, typename TYPE_ITEM>
void SystemStatus::setDefaultIteminReport(TYPE_REPORT& report, const TYPE_ITEM& s)
{
    report.push_back(s);
    if (report.size() > s.maxItem) {
        report.erase(report.begin());
    }
}

template <typename TYPE_REPORT, typename TYPE_ITEM>
void SystemStatus::getIteminReport(TYPE_REPORT& reportout, const TYPE_ITEM& c) const
{
    reportout.clear();
    if (c.size() >= 1) {
        reportout.push_back(c.back());
        reportout.back().dump();
    }
}

void SystemStatus::setEngineDebugDataInfo(const GnssEngineDebugDataInfo& gnssEngineDebugDataInfo) {
    pthread_mutex_lock(&mMutexSystemStatus);
    LOC_LOGd("setEngine data");
    setIteminReport(mCache.mTimeAndClock, SystemStatusTimeAndClock(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mXoState, SystemStatusXoState(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mRfAndParams, SystemStatusRfAndParams(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mErrRecovery, SystemStatusErrRecovery(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mInjectedPosition,
                    SystemStatusInjectedPosition(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mBestPosition, SystemStatusBestPosition(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mXtra, SystemStatusXtra(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mEphemeris, SystemStatusEphemeris(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mSvHealth, SystemStatusSvHealth(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mPdr, SystemStatusPdr(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mNavData, SystemStatusNavData(gnssEngineDebugDataInfo));
    setIteminReport(mCache.mPositionFailure, SystemStatusPositionFailure(gnssEngineDebugDataInfo));
    pthread_mutex_unlock(&mMutexSystemStatus);
}


/******************************************************************************
@brief      API to set report position data into internal buffer

@param[In]  UlpLocation

@return     true when successfully done
******************************************************************************/
bool SystemStatus::eventPosition(const UlpLocation& location,
                                 const GpsLocationExtended& locationEx)
{
    bool ret = false;
    pthread_mutex_lock(&mMutexSystemStatus);

    ret = setIteminReport(mCache.mLocation, SystemStatusLocation(location, locationEx));
    LOC_LOGV("eventPosition - lat=%f lon=%f alt=%f speed=%f",
             location.gpsLocation.latitude,
             location.gpsLocation.longitude,
             location.gpsLocation.altitude,
             location.gpsLocation.speed);

    pthread_mutex_unlock(&mMutexSystemStatus);
    return ret;
}

/******************************************************************************
@brief      API to get report data into a given buffer

@param[In]  reference to report buffer
@param[In]  bool flag to identify latest only or entire buffer

@return     true when successfully done
******************************************************************************/
bool SystemStatus::getReport(SystemStatusReports& report, bool isLatestOnly,
        bool inSessionOnly) const {
    pthread_mutex_lock(&mMutexSystemStatus);
    if (inSessionOnly && !mTracking) {
        pthread_mutex_unlock(&mMutexSystemStatus);
        return true;
    }

    if (isLatestOnly) {
        // push back only the latest report and return it
        getIteminReport(report.mLocation, mCache.mLocation);

        getIteminReport(report.mTimeAndClock, mCache.mTimeAndClock);
        getIteminReport(report.mXoState, mCache.mXoState);
        getIteminReport(report.mRfAndParams, mCache.mRfAndParams);
        getIteminReport(report.mErrRecovery, mCache.mErrRecovery);

        getIteminReport(report.mInjectedPosition, mCache.mInjectedPosition);
        getIteminReport(report.mBestPosition, mCache.mBestPosition);
        getIteminReport(report.mXtra, mCache.mXtra);
        getIteminReport(report.mEphemeris, mCache.mEphemeris);
        getIteminReport(report.mSvHealth, mCache.mSvHealth);
        getIteminReport(report.mPdr, mCache.mPdr);
        getIteminReport(report.mNavData, mCache.mNavData);

        getIteminReport(report.mPositionFailure, mCache.mPositionFailure);

    }
    else {
        // copy entire reports and return them
        report.mLocation.clear();

        report.mTimeAndClock.clear();
        report.mXoState.clear();
        report.mRfAndParams.clear();
        report.mErrRecovery.clear();

        report.mInjectedPosition.clear();
        report.mBestPosition.clear();
        report.mXtra.clear();
        report.mEphemeris.clear();
        report.mSvHealth.clear();
        report.mPdr.clear();
        report.mNavData.clear();

        report.mPositionFailure.clear();
        report = mCache;
    }

    pthread_mutex_unlock(&mMutexSystemStatus);
    return true;
}

/******************************************************************************
@brief      API to set default report data

@param[In]  none

@return     true when successfully done
******************************************************************************/
bool SystemStatus::setDefaultGnssEngineStates(void)
{
    pthread_mutex_lock(&mMutexSystemStatus);

    setDefaultIteminReport(mCache.mLocation, SystemStatusLocation());

    setDefaultIteminReport(mCache.mTimeAndClock, SystemStatusTimeAndClock());
    setDefaultIteminReport(mCache.mXoState, SystemStatusXoState());
    setDefaultIteminReport(mCache.mRfAndParams, SystemStatusRfAndParams());
    setDefaultIteminReport(mCache.mErrRecovery, SystemStatusErrRecovery());

    setDefaultIteminReport(mCache.mInjectedPosition, SystemStatusInjectedPosition());
    setDefaultIteminReport(mCache.mBestPosition, SystemStatusBestPosition());
    setDefaultIteminReport(mCache.mXtra, SystemStatusXtra());
    setDefaultIteminReport(mCache.mEphemeris, SystemStatusEphemeris());
    setDefaultIteminReport(mCache.mSvHealth, SystemStatusSvHealth());
    setDefaultIteminReport(mCache.mPdr, SystemStatusPdr());
    setDefaultIteminReport(mCache.mNavData, SystemStatusNavData());

    setDefaultIteminReport(mCache.mPositionFailure, SystemStatusPositionFailure());

    pthread_mutex_unlock(&mMutexSystemStatus);
    return true;
}
} // namespace loc_core

