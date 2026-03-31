/* Copyright (c) 2011-2014, 2016-2021 The Linux Foundation. All rights reserved.
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

#define LOG_NDEBUG 0 //Define to enable LOGV
#define LOG_TAG "LocSvc_LocApiBase"

#include <dlfcn.h>
#include <inttypes.h>
#include <gps_extended_c.h>
#include <LocApiBase.h>
#include <LocAdapterBase.h>
#include <log_util.h>
#include <LocContext.h>
#include <loc_misc_utils.h>

#ifdef PTP_SUPPORTED
#include <gptp_helper.h>
#endif

namespace loc_core {

#define MSEC_IN_ONE_WEEK 604800000LL

#define TO_ALL_LOCADAPTERS(call) TO_ALL_ADAPTERS(mLocAdapters, (call))
#define TO_1ST_HANDLING_LOCADAPTERS(call) TO_1ST_HANDLING_ADAPTER(mLocAdapters, (call))

struct LocSsrMsg : public LocMsg {
    LocApiBase* mLocApi;
    inline LocSsrMsg(LocApiBase* locApi) :
        LocMsg(), mLocApi(locApi)
    {
        locallog();
    }
    inline virtual void proc() const {
        mLocApi->close();
        // Reset all QWES feature flags to false during SSR
        std::unordered_map<LocationQwesFeatureType, bool> featureMap;
        featureMap[LOCATION_QWES_FEATURE_TYPE_CARRIER_PHASE] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_POLYNOMIAL] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_SV_EPH] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_SINGLE_FREQUENCY] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_GNSS_MULTI_FREQUENCY] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_FREQUENCY] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_TIME_UNCERTAINTY] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_CLOCK_ESTIMATE] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_DGNSS] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_PPE] = false;
        featureMap[LOCATION_QWES_FEATURE_NLOS_ML20] = false;
        featureMap[LOCATION_QWES_FEATURE_STATUS_GNSS_NHZ] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_SBAS] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_ROBUST_LOCATION] = false;
        featureMap[LOCATION_QWES_FEATURE_TYPE_WOCS] = false;
        mLocApi->reportQwesCapabilities(featureMap);
        if (LOC_API_ADAPTER_ERR_SUCCESS == mLocApi->open(mLocApi->getEvtMask())) {
            // Notify adapters that engine up after SSR
            mLocApi->handleEngineUpEvent();
        }
    }
    inline void locallog() const {
        LOC_LOGV("LocSsrMsg");
    }
    inline virtual void log() const {
        locallog();
    }
};

struct LocOpenMsg : public LocMsg {
    LocApiBase* mLocApi;
    LocAdapterBase* mAdapter;
    inline LocOpenMsg(LocApiBase* locApi, LocAdapterBase* adapter = nullptr) :
            LocMsg(), mLocApi(locApi), mAdapter(adapter)
    {
        locallog();
    }
    inline virtual void proc() const {
        if (LOC_API_ADAPTER_ERR_SUCCESS == mLocApi->open(mLocApi->getEvtMask()) &&
            nullptr != mAdapter) {
            mAdapter->handleEngineUpEvent();
        }
    }
    inline void locallog() const {
        LOC_LOGv("LocOpen Mask: %" PRIx64 "\n", mLocApi->getEvtMask());
    }
    inline virtual void log() const {
        locallog();
    }
};

struct LocCloseMsg : public LocMsg {
    LocApiBase* mLocApi;
    inline LocCloseMsg(LocApiBase* locApi) :
        LocMsg(), mLocApi(locApi)
    {
        locallog();
    }
    inline virtual void proc() const {
        mLocApi->close();
    }
    inline void locallog() const {
    }
    inline virtual void log() const {
        locallog();
    }
};

MsgTask* LocApiBase::mMsgTask = nullptr;

LocApiBase::LocApiBase(LOC_API_ADAPTER_EVENT_MASK_T excludedMask,
                       ContextBase* context) :
    mContext(context),
    mMask(0), mExcludedMask(excludedMask), mEngineLockState(ENGINE_LOCK_STATE_DISABLED) {
    memset(mLocAdapters, 0, sizeof(mLocAdapters));

    if (nullptr == mMsgTask) {
        mMsgTask = new MsgTask("LocApiMsgTask");
    }
}

LOC_API_ADAPTER_EVENT_MASK_T LocApiBase::getEvtMask()
{
    LOC_API_ADAPTER_EVENT_MASK_T mask = 0;

    TO_ALL_LOCADAPTERS(mask |= mLocAdapters[i]->getEvtMask());

    return mask & ~mExcludedMask;
}

bool LocApiBase::isMaster()
{
    bool isMaster = false;

    for (int i = 0;
            !isMaster && i < MAX_ADAPTERS && NULL != mLocAdapters[i];
            i++) {
        isMaster |= mLocAdapters[i]->isAdapterMaster();
    }
    return isMaster;
}

bool LocApiBase::isInSession()
{
    bool inSession = false;

    for (int i = 0;
         !inSession && i < MAX_ADAPTERS && NULL != mLocAdapters[i];
         i++) {
        inSession = mLocAdapters[i]->isInSession();
    }

    return inSession;
}

void LocApiBase::addAdapter(LocAdapterBase* adapter)
{
    for (int i = 0; i < MAX_ADAPTERS && mLocAdapters[i] != adapter; i++) {
        if (mLocAdapters[i] == NULL) {
            mLocAdapters[i] = adapter;
            sendMsg(new LocOpenMsg(this,  adapter));
            break;
        }
    }
}

void LocApiBase::removeAdapter(LocAdapterBase* adapter)
{
    for (int i = 0;
         i < MAX_ADAPTERS && NULL != mLocAdapters[i];
         i++) {
        if (mLocAdapters[i] == adapter) {
            mLocAdapters[i] = NULL;

            // shift the rest of the adapters up so that the pointers
            // in the array do not have holes.  This should be more
            // performant, because the array maintenance is much much
            // less frequent than event handlings, which need to linear
            // search all the adapters
            int j = i;
            while (++i < MAX_ADAPTERS && mLocAdapters[i] != NULL);

            // i would be MAX_ADAPTERS or point to a NULL
            i--;
            // i now should point to a none NULL adapter within valid
            // range although i could be equal to j, but it won't hurt.
            // No need to check it, as it gains nothing.
            mLocAdapters[j] = mLocAdapters[i];
            // this makes sure that we exit the for loop
            mLocAdapters[i] = NULL;

            // if we have an empty list of adapters
            if (0 == i) {
                sendMsg(new LocCloseMsg(this));
            } else {
                // else we need to remove the bit
                sendMsg(new LocOpenMsg(this));
            }
        }
    }
}

void LocApiBase::updateEvtMask()
{
    sendMsg(new LocOpenMsg(this));
}

void LocApiBase::handleEngineUpEvent()
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->handleEngineUpEvent());
}

void LocApiBase::handleEngineDownEvent()
{    // This will take care of renegotiating the loc handle
    sendMsg(new LocSsrMsg(this));

    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->handleEngineDownEvent());
}

void LocApiBase::reportPosition(UlpLocation& location,
                                GpsLocationExtended& locationExtended,
                                enum loc_sess_status status,
                                LocPosTechMask loc_technology_mask)
{
    // print the location info before delivering
    LOC_LOGd("\n  flags: 0x%x\n  source: %d\n  latitude: %f\n  longitude: %f\n  "
           "altitude: %f\n  speed: %f\n  bearing: %f\n  accuracy: %f\n  "
           "timestamp: %" PRId64 "\n  "
           "session status: %d\n  technology mask: 0x%x\n  time bias unc %f msec\n  "
           "SV used in fix (gps/glo/bds/gal/qzss/navic) : \n"
           "(0x%" PRIx64 "/0x%" PRIx64 "/0x%" PRIx64 "/0x%" PRIx64 "/0x%" PRIx64 "/0x%" PRIx64 ")",
             location.gpsLocation.flags, location.position_source,
             location.gpsLocation.latitude, location.gpsLocation.longitude,
             location.gpsLocation.altitude, location.gpsLocation.speed,
             location.gpsLocation.bearing, location.gpsLocation.accuracy,
             location.gpsLocation.timestamp, status, loc_technology_mask,
             locationExtended.gnssSystemTime.u.gpsSystemTime.systemClkTimeUncMs,
             locationExtended.gnss_sv_used_ids.gpsSvUsedIdsMask,
             locationExtended.gnss_sv_used_ids.gloSvUsedIdsMask,
             locationExtended.gnss_sv_used_ids.bdsSvUsedIdsMask,
             locationExtended.gnss_sv_used_ids.galSvUsedIdsMask,
             locationExtended.gnss_sv_used_ids.qzssSvUsedIdsMask,
             locationExtended.gnss_sv_used_ids.navicSvUsedIdsMask);
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportPositionEvent(location, locationExtended,
                                             status, loc_technology_mask)
    );
}

void LocApiBase::requestOdcpi(OdcpiRequestInfo& request)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestOdcpiEvent(request));
}

void LocApiBase::reportGnssEngEnergyConsumedEvent(uint64_t energyConsumedSinceFirstBoot)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportGnssEngEnergyConsumedEvent(
            energyConsumedSinceFirstBoot));
}

void LocApiBase::reportDeleteAidingDataEvent(GnssAidingData& aidingData) {
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportDeleteAidingDataEvent(aidingData));
}

void LocApiBase::reportKlobucharIonoModel(GnssKlobucharIonoModel & ionoModel) {
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportKlobucharIonoModelEvent(ionoModel));
}

void LocApiBase::reportGnssAdditionalSystemInfo(GnssAdditionalSystemInfo& additionalSystemInfo) {
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportGnssAdditionalSystemInfoEvent(
            additionalSystemInfo));
}

void LocApiBase::sendNfwNotification(GnssNfwNotification& notification)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportNfwNotificationEvent(notification));

}

void LocApiBase::reportSv(GnssSvNotification& svNotify)
{
    const char* constellationString[] = { "Unknown", "GPS", "SBAS", "GLONASS",
        "QZSS", "BEIDOU", "GALILEO", "NAVIC" };

    // print the SV info before delivering
    LOC_LOGv("num sv: %u\n"
        "      sv: constellation svid  cN0  bbCN0"
        "  elevation  azimuth  carrierFreq  gloFreq flags signalType",
        svNotify.count);
    for (size_t i = 0; i < svNotify.count && i < GNSS_SV_MAX; i++) {
        if (svNotify.gnssSvs[i].type >
            sizeof(constellationString) / sizeof(constellationString[0]) - 1) {
            svNotify.gnssSvs[i].type = GNSS_SV_TYPE_UNKNOWN;
        }
        // Display what we report to clients
        LOC_LOGV(" %03zu: %*s %02d  %2.2f  %2.2f  %3.2f  %3.2f %10.2f %u 0x%02X 0x%2X",
            i,
            13,
            constellationString[svNotify.gnssSvs[i].type],
            svNotify.gnssSvs[i].svId,
            svNotify.gnssSvs[i].cN0Dbhz,
            svNotify.gnssSvs[i].basebandCarrierToNoiseDbHz,
            svNotify.gnssSvs[i].elevation,
            svNotify.gnssSvs[i].azimuth,
            svNotify.gnssSvs[i].carrierFrequencyHz,
            svNotify.gnssSvs[i].gloFrequency,
            svNotify.gnssSvs[i].gnssSvOptionsMask,
            svNotify.gnssSvs[i].gnssSignalTypeMask);
    }
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvEvent(svNotify)
        );
}

void LocApiBase::reportSvPolynomial(GnssSvPolynomial &svPolynomial)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvPolynomialEvent(svPolynomial)
    );
}

void LocApiBase::reportSvEphemeris(GnssSvEphemerisReport & svEphemeris)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvEphemerisEvent(svEphemeris)
    );
}

void LocApiBase::reportData(GnssDataNotification& dataNotify)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportDataEvent(dataNotify));
}

void LocApiBase::reportNmea(const char* nmea, int length)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportNmeaEvent(nmea, length));
}

void LocApiBase::reportLocationSystemInfo(const LocationSystemInfo& locationSystemInfo)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportLocationSystemInfoEvent(locationSystemInfo));
}

void LocApiBase::reportDcMessage(const GnssDcReportInfo& dcReport) {
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportDcMessage(dcReport));
}

void LocApiBase::reportSignalTypeCapabilities(const GnssCapabNotification& gnssCapabNotification) {
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportSignalTypeCapabilities(gnssCapabNotification));
}

void LocApiBase::reportNtnStatusEvent(LocationError status,
        const GnssSignalTypeMask& gpsSignalTypeConfigMask, bool isSetResponse) {
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportNtnStatusEvent(
                status, gpsSignalTypeConfigMask, isSetResponse));
}

void LocApiBase::reportNtnConfigUpdateEvent(const GnssSignalTypeMask& gpsSignalTypeConfigMask) {
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportNtnConfigUpdateEvent(gpsSignalTypeConfigMask));
}

void LocApiBase::reportQwesCapabilities
(
    const std::unordered_map<LocationQwesFeatureType, bool> &featureMap
)
{
    //Set Qwes feature status map in ContextBase
    ContextBase::setQwesFeatureStatus(featureMap);
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportQwesCapabilities(featureMap));
}

void LocApiBase::requestTime() {
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestTime());
}

void LocApiBase::requestATL(int connHandle, LocAGpsType agps_type,
                            LocApnTypeMask apn_type_mask, LocSubId sub_id,
                            uint32_t timeout)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(
            mLocAdapters[i]->requestATL(connHandle, agps_type, apn_type_mask, sub_id, timeout));
}

void LocApiBase::releaseATL(int connHandle, uint32_t timeout)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->releaseATL(connHandle, timeout));
}

void* LocApiBase :: getSibling()
    DEFAULT_IMPL(NULL)

LocApiProxyBase* LocApiBase :: getLocApiProxy()
    DEFAULT_IMPL(NULL)

void LocApiBase::reportGnssMeasurements(GnssMeasurements& gnssMeasurements)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportGnssMeasurementsEvent(gnssMeasurements));
}

void LocApiBase::reportGnssSvIdConfig(const GnssSvIdConfig& config)
{
    // Print the config
    LOC_LOGv("gloBlacklistSvMask: %" PRIu64 ", bdsBlacklistSvMask: %" PRIu64 ",\n"
             "qzssBlacklistSvMask: %" PRIu64 ", galBlacklistSvMask: %" PRIu64 ",\n"
              "navicBlacklistSvMask: %" PRIu64,
             config.gloBlacklistSvMask, config.bdsBlacklistSvMask,
             config.qzssBlacklistSvMask, config.galBlacklistSvMask, config.navicBlacklistSvMask);

    // Loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportGnssSvIdConfigEvent(config));
}

void LocApiBase::geofenceBreach(size_t count, uint32_t* hwIds, Location& location,
                                GeofenceBreachType breachType, uint64_t timestamp)
{
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->geofenceBreachEvent(count, hwIds, location, breachType,
                                                            timestamp));
}

void LocApiBase::geofenceStatus(GeofenceStatusAvailable available)
{
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->geofenceStatusEvent(available));
}

void LocApiBase::reportLocations(Location* locations, size_t count)
{
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportLocationsEvent(locations, count));
}

void LocApiBase::handleBatchStatusEvent(BatchingStatus batchStatus)
{
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportBatchStatusChangeEvent(batchStatus));
}

void LocApiBase::reportGnssConfig(uint32_t sessionId, const GnssConfig& gnssConfig)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportGnssConfigEvent(sessionId, gnssConfig));
}

void LocApiBase::reportEngineLockStatus(EngineLockState engineLockState) {
    // loop through adapters, and deliver to the All handling adapter.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->handleEngineLockStatusEvent(engineLockState));
}

void LocApiBase::reportEngDebugDataInfo(GnssEngineDebugDataInfo& gnssEngineDebugDataInfo) {
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportEngDebugDataInfoEvent(gnssEngineDebugDataInfo));
}

enum loc_api_adapter_err LocApiBase::
   open(LOC_API_ADAPTER_EVENT_MASK_T /*mask*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    close()
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

void LocApiBase::
    deleteAidingData(const GnssAidingData& /*data*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::
    injectPosition(const Location& /*location*/, bool /*onDemandCpi*/)
DEFAULT_IMPL()

void LocApiBase::
    injectPosition(const GnssLocationInfoNotification & /*locationInfo*/, bool /*onDemandCpi*/)
DEFAULT_IMPL()

void LocApiBase::
    injectPositionAndCivicAddress(const Location& location, const GnssCivicAddress& addr)
DEFAULT_IMPL()

void LocApiBase::
    setTime(LocGpsUtcTime /*time*/, int64_t /*timeReference*/, int /*uncertainty*/)
DEFAULT_IMPL()

void LocApiBase::
   atlOpenStatus(int /*handle*/, int /*is_succ*/, char* /*apn*/, uint32_t /*apnLen*/,
                 AGpsBearerType /*bear*/, LocAGpsType /*agpsType*/,
                 LocApnTypeMask /*mask*/)
DEFAULT_IMPL()

void LocApiBase::
    atlCloseStatus(int /*handle*/, int /*is_succ*/)
DEFAULT_IMPL()

LocationError LocApiBase::
    setServerSync(const char* /*url*/, int /*len*/, LocServerType /*type*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setServerSync(unsigned int /*ip*/, int /*port*/, LocServerType /*type*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setSUPLVersionSync(GnssConfigSuplVersion /*version*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setNMEATypesSync (uint32_t /*typesMask*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    setLPPConfigSync(GnssConfigLppProfileMask /*profileMask*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)


enum loc_api_adapter_err LocApiBase::
    setSensorPropertiesSync(bool /*gyroBiasVarianceRandomWalk_valid*/,
                        float /*gyroBiasVarianceRandomWalk*/,
                        bool /*accelBiasVarianceRandomWalk_valid*/,
                        float /*accelBiasVarianceRandomWalk*/,
                        bool /*angleBiasVarianceRandomWalk_valid*/,
                        float /*angleBiasVarianceRandomWalk*/,
                        bool /*rateBiasVarianceRandomWalk_valid*/,
                        float /*rateBiasVarianceRandomWalk*/,
                        bool /*velocityBiasVarianceRandomWalk_valid*/,
                        float /*velocityBiasVarianceRandomWalk*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setSensorPerfControlConfigSync(int /*controlMode*/,
                               int /*accelSamplesPerBatch*/,
                               int /*accelBatchesPerSec*/,
                               int /*gyroSamplesPerBatch*/,
                               int /*gyroBatchesPerSec*/,
                               int /*accelSamplesPerBatchHigh*/,
                               int /*accelBatchesPerSecHigh*/,
                               int /*gyroSamplesPerBatchHigh*/,
                               int /*gyroBatchesPerSecHigh*/,
                               int /*algorithmConfig*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    setAGLONASSProtocolSync(GnssConfigAGlonassPositionProtocolMask /*aGlonassProtocol*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask /*lppeCP*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask /*lppeUP*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

GnssConfigSuplVersion LocApiBase::convertSuplVersion(const uint32_t /*suplVersion*/)
DEFAULT_IMPL(GNSS_CONFIG_SUPL_VERSION_1_0_0)

GnssConfigLppeControlPlaneMask LocApiBase::convertLppeCp(const uint32_t /*lppeControlPlaneMask*/)
DEFAULT_IMPL(0)

GnssConfigLppeUserPlaneMask LocApiBase::convertLppeUp(const uint32_t /*lppeUserPlaneMask*/)
DEFAULT_IMPL(0)

LocationError LocApiBase::setEmergencyExtensionWindowSync(
        const uint32_t /*emergencyExtensionSeconds*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

#ifdef _ANDROID_
void LocApiBase::setMeasurementCorrections(
        const GnssMeasurementCorrections& /*gnssMeasurementCorrections*/)
DEFAULT_IMPL()
#endif

bool LocApiBase::
   getBestAvailableZppFixSync(LocGpsLocation &zppLoc, LocPosTechMask &tech_mask)
DEFAULT_IMPL(false)

LocationError LocApiBase::
    setGpsLockSync(GnssConfigGpsLock /*lock*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

void LocApiBase::
    requestForAidingData(GnssAidingDataSvMask /*svDataMask*/)
DEFAULT_IMPL()

LocationError LocApiBase::setBlacklistSvSync(const GnssSvIdConfig& /*config*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

void LocApiBase::setBlacklistSv(const GnssSvIdConfig& /*config*/,
                                LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::getBlacklistSv()
DEFAULT_IMPL()

void LocApiBase::setConstellationControl(const GnssSvTypeConfig& /*config*/,
                                         LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::resetConstellationControl(LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::
    setConstrainedTuncMode(bool /*enabled*/,
                           float /*tuncConstraint*/,
                           uint32_t /*energyBudget*/,
                           LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::
    setPositionAssistedClockEstimatorMode(bool /*enabled*/,
                                          LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::getGnssEnergyConsumed()
DEFAULT_IMPL()


void LocApiBase::addGeofence(uint32_t /*clientId*/, const GeofenceOption& /*options*/,
        const GeofenceInfo& /*info*/,
        LocApiResponseData<LocApiGeofenceData>* /*adapterResponseData*/)
DEFAULT_IMPL()

void LocApiBase::removeGeofence(uint32_t /*hwId*/, uint32_t /*clientId*/,
        LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::pauseGeofence(uint32_t /*hwId*/, uint32_t /*clientId*/,
        LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::resumeGeofence(uint32_t /*hwId*/, uint32_t /*clientId*/,
        LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::modifyGeofence(uint32_t /*hwId*/, uint32_t /*clientId*/,
         const GeofenceOption& /*options*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::startTimeBasedTracking(const TrackingOptions& /*options*/,
        LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::stopTimeBasedTracking(LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::startBatching(uint32_t /*sessionId*/, const LocationOptions& /*options*/,
        uint32_t /*accuracy*/, uint32_t /*timeout*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::stopBatching(uint32_t /*sessionId*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

LocationError LocApiBase::getBatchedLocationsSync(size_t /*count*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

void LocApiBase::getBatchedLocations(size_t /*count*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::setBatchSize(size_t /*size*/)
DEFAULT_IMPL()

void LocApiBase::addToCallQueue(LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::updateSystemPowerState(PowerStateType /*powerState*/)
DEFAULT_IMPL()

void LocApiBase::updatePowerConnectState(bool /*connected*/)
DEFAULT_IMPL()

void LocApiBase::
    configRobustLocation(bool /*enabled*/,
                         bool /*enableForE911*/,
                         LocApiResponse* /*adapterResponse*/,
                         bool /*enableForE911Valid*/)
DEFAULT_IMPL()

void LocApiBase::
    getRobustLocationConfig(uint32_t /*sessionId*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

#ifdef USE_GLIB
void LocApiBase::
    configMinGpsWeek(uint16_t /*minGpsWeek*/,
                     LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()
#endif

void LocApiBase::
    getMinGpsWeek(uint32_t /*sessionId*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

LocationError LocApiBase::
    setParameterSync(const GnssConfig& /*gnssConfig*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

void LocApiBase::
    getParameter(uint32_t /*sessionId*/, GnssConfigFlagsMask /*flags*/,
                 LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::
    configConstellationMultiBand(const GnssSvTypeConfig& /*secondaryBandConfig*/,
                                 LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::
    getConstellationMultiBandConfig(uint32_t /*sessionId*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::setTribandState(bool /*enabled*/)
DEFAULT_IMPL()

void LocApiBase::
    configPrecisePositioning(PreciseType preciseType, bool enable,
            LocApiResponse* /*adpterResponse*/)
DEFAULT_IMPL()

void LocApiBase::configMerkleTree(mgpOsnmaPublicKeyAndMerkleTreeStruct* /*merkleTree*/,
            LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::configOsnmaEnablement(bool /*enable*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

#ifdef _ANDROID_
void LocApiBase::getNtnConfigSignalMask(LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()
#endif

void LocApiBase::setNtnConfigSignalMask(GnssSignalTypeMask /*gpsSignalTypeConfigMask*/,
            LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

#ifdef _ANDROID_
void LocApiBase::injectSuplCert(int32_t /*suplCertId*/,
        const std::vector<uint8_t>& /*suplCertData*/, LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()

void LocApiBase::setPreferredConstellation(GnssSvType /*type*/,
        LocApiResponse* /*adapterResponse*/)
DEFAULT_IMPL()
#endif

void RealtimeEstimator::reset() {
    memset(&mTimePairPVTReport, 0, sizeof(mTimePairPVTReport));
    memset(&mTimePairMeasReport, 0, sizeof(mTimePairMeasReport));
}

void RealtimeEstimator::saveGpsTimeAndQtimerPairInPvtReport(
        const GpsLocationExtended& locationExtended,
        enum loc_sess_status status) {

    // Use GPS timestamp and qtimer tick for 1Hz PVT report or Final fixes for association
    if (locationExtended.isReportTimeAccurate() &&
            ((locationExtended.gnssSystemTime.u.gpsSystemTime.systemMsec % 1000 == 0) ||
             (LOC_SESS_SUCCESS == status))) {
        LOC_LOGv("save time association from PVT report with gps time %u %u, "
                 "qtimer %" PRIi64 " %f ",
                 locationExtended.gnssSystemTime.u.gpsSystemTime.systemWeek,
                 locationExtended.gnssSystemTime.u.gpsSystemTime.systemMsec,
                 locationExtended.systemTick, locationExtended.systemTickUnc);
        mTimePairPVTReport.gpsTime.gpsWeek =
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemWeek;
        mTimePairPVTReport.gpsTime.gpsTimeOfWeekMs =
                locationExtended.gnssSystemTime.u.gpsSystemTime.systemMsec;
        mTimePairPVTReport.qtimerTick = locationExtended.systemTick;
        mTimePairPVTReport.timeUncMsec = locationExtended.systemTickUnc;
    }
}


bool RealtimeEstimator::fillAdditionalTimestamps(
        const GPSTimeStruct& gpsTimeAtOrigin,
        int64_t &bootTimeNsAtOrigin, float &bootTimeUnc,
        uint64_t &gptpTime, bool &gPTPValidity) {
    struct timespec curBootTime = {};
    int64_t curBootTimeNs = 0;
    int64_t curQTimerNSec = 0;
    int64_t qtimerNsecAtOrigin = 0;
    int64_t gpsTimeDiffMsec = 0;
    GpsTimeQtimerTickPair timePair;

    // We have valid association
   if (mTimePairPVTReport.gpsTime.gpsWeek != 0) {
        LOC_LOGv("use PVT time association");
        timePair = mTimePairPVTReport;
    } else {
        return false;
    }

    int64_t timePairQtimerNsec = (timePair.qtimerTick / 192) * 10000;
    int64_t originMsec = (int64_t)gpsTimeAtOrigin.gpsWeek * (int64_t)MSEC_IN_ONE_WEEK +
                         (int64_t)gpsTimeAtOrigin.gpsTimeOfWeekMs;
    int64_t timePairMsec = (int64_t)timePair.gpsTime.gpsWeek * (int64_t)MSEC_IN_ONE_WEEK +
                            (int64_t)timePair.gpsTime.gpsTimeOfWeekMs;

    gpsTimeDiffMsec = originMsec - timePairMsec;

    qtimerNsecAtOrigin = timePairQtimerNsec + gpsTimeDiffMsec * 1000000;

    clock_gettime(CLOCK_BOOTTIME, &curBootTime);
    curBootTimeNs = ((int64_t)curBootTime.tv_sec) * 1000000000 + (int64_t)curBootTime.tv_nsec;
    // qtimer freq: 19200000, so
    // so 1 tick equals 1000,000,000/19,200,000 ns = 10000/192
    curQTimerNSec = (getQTimerTickCount() / 192) * 10000;
    bootTimeNsAtOrigin = curBootTimeNs - (curQTimerNSec - qtimerNsecAtOrigin);

    bootTimeUnc = timePair.timeUncMsec;
#ifdef PTP_SUPPORTED
    if (gptpGetPtpTimeFromQTimeNs(&gptpTime, qtimerNsecAtOrigin)) {
        gPTPValidity = true;
    }
#endif

    LOC_LOGv("gpsTimeAtOrigin (%d, %d), timepair: gps (%d, %d), "
             "qtimer nsec =%" PRIi64 ", curQTimerNSec=%" PRIi64 " qtimerNsecAtOrigin=%" PRIi64 ""
             " curBoottimeNSec=%" PRIi64 " bootimeNsecAtOrigin=%" PRIi64 ", boottime unc =%f"
             " gptp Time =%" PRIu64 " gPTPValidity = %d",
             gpsTimeAtOrigin.gpsWeek, gpsTimeAtOrigin.gpsTimeOfWeekMs,
             timePair.gpsTime.gpsWeek, timePair.gpsTime.gpsTimeOfWeekMs,
             timePairQtimerNsec, curQTimerNSec, qtimerNsecAtOrigin,
             curBootTimeNs, bootTimeNsAtOrigin, bootTimeUnc, gptpTime, gPTPValidity);

    if (bootTimeNsAtOrigin > 0) {
        return true;
    } else {
        return false;
    }
}

} // namespace loc_core
