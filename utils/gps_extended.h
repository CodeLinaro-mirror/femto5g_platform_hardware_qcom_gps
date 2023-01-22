/* Copyright (c) 2013-2017, 2020-2021 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:
Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef GPS_EXTENDED_H
#define GPS_EXTENDED_H

/**
 * @file
 * @brief C++ declarations for GPS types
 */

#include <gps_extended_c.h>
#if defined(USE_GLIB) || defined(OFF_TARGET)
#include <string.h>
#endif
#include <string>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



struct LocPosMode
{
    LocPositionMode mode;
    LocGpsPositionRecurrence recurrence;
    uint32_t min_interval;
    uint32_t preferred_accuracy;
    uint32_t preferred_time;
    bool share_position;
    char credentials[14];
    char provider[8];
    GnssPowerMode powerMode;
    uint32_t timeBetweenMeasurements;
    LocPosMode(LocPositionMode m, LocGpsPositionRecurrence recr,
               uint32_t gap, uint32_t accu, uint32_t time,
               bool sp, const char* cred, const char* prov,
               GnssPowerMode pMode = GNSS_POWER_MODE_INVALID,
               uint32_t tbm = 0) :
        mode(m), recurrence(recr),
        min_interval(gap < GPS_MIN_POSSIBLE_FIX_INTERVAL_MS ?
                     GPS_MIN_POSSIBLE_FIX_INTERVAL_MS : gap),
        preferred_accuracy(accu), preferred_time(time),
        share_position(sp), powerMode(pMode),
        timeBetweenMeasurements(tbm) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
        if (NULL != cred) {
            memcpy(credentials, cred, sizeof(credentials)-1);
        }
        if (NULL != prov) {
            memcpy(provider, prov, sizeof(provider)-1);
        }
    }

    inline LocPosMode() :
        mode(LOC_POSITION_MODE_MS_BASED),
        recurrence(LOC_GPS_POSITION_RECURRENCE_PERIODIC),
        min_interval(GPS_DEFAULT_FIX_INTERVAL_MS),
        preferred_accuracy(50), preferred_time(120000),
        share_position(true), powerMode(GNSS_POWER_MODE_INVALID),
        timeBetweenMeasurements(GPS_DEFAULT_FIX_INTERVAL_MS) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
    }

    inline bool equals(const LocPosMode &anotherMode) const
    {
        return anotherMode.mode == mode &&
            anotherMode.recurrence == recurrence &&
            anotherMode.min_interval == min_interval &&
            anotherMode.preferred_accuracy == preferred_accuracy &&
            anotherMode.preferred_time == preferred_time &&
            anotherMode.powerMode == powerMode &&
            anotherMode.timeBetweenMeasurements == timeBetweenMeasurements &&
            !strncmp(anotherMode.credentials, credentials, sizeof(credentials)-1) &&
            !strncmp(anotherMode.provider, provider, sizeof(provider)-1);
    }

    void logv() const;
};

/*
* Encapsulates the parameters (client name, preferred subscription ID and preferred APN
* for backhaul connect.
*/
struct BackhaulContext {
    std::string clientName;
    uint16_t prefSub;
    std::string prefApn;
    uint16_t prefIpType;

    inline bool operator ==(const BackhaulContext& i1) const {
        // we do not support multiple request from same client
        return i1.clientName == clientName;
    }

    // custom less than comparator for BackhaulContext set.
    struct less {
        inline bool operator()(BackhaulContext const& i, BackhaulContext const& j) const {
            // we support just one BackhaulRequest with the same client name.
            return (i.clientName.compare(j.clientName) < 0);
        }
    };

    // Custom hash function for BackhaulContext unordered_set.
    struct hash {
        inline size_t operator()(BackhaulContext const& i) const {
            // Index with client name base hash, as we support just one BackhaulRequest
            // with the same client name.
            return (std::hash<std::string>()(i.clientName));
        }
    };
};

/** Represents gps location extended. */
typedef struct {
    /** set to sizeof(GpsLocationExtended) */
    uint32_t          size;
    /** Contains GpsLocationExtendedFlags bits. */
    uint64_t        flags;
    /** Contains the Altitude wrt mean sea level */
    float           altitudeMeanSeaLevel;
    /** Contains Position Dilusion of Precision. */
    float           pdop;
    /** Contains Horizontal Dilusion of Precision. */
    float           hdop;
    /** Contains Vertical Dilusion of Precision. */
    float           vdop;
    /** Contains Magnetic Deviation. */
    float           magneticDeviation;
    /** vertical uncertainty in meters
     *  confidence level is at 68% */
    float           vert_unc;
    /** horizontal speed uncertainty in m/s
     *  confidence level is at 68% */
    float           speed_unc;
    /** heading uncertainty in degrees (0 to 359.999)
     *  confidence level is at 68% */
    float           bearing_unc;
    /** horizontal reliability. */
    LocReliability  horizontal_reliability;
    /** vertical reliability. */
    LocReliability  vertical_reliability;
    /**  Horizontal Elliptical Uncertainty (Semi-Major Axis)
     *   Confidence level is at 39% */
    float           horUncEllipseSemiMajor;
    /**  Horizontal Elliptical Uncertainty (Semi-Minor Axis)
     *   Confidence level is at 39% */
    float           horUncEllipseSemiMinor;
    /**  Elliptical Horizontal Uncertainty Azimuth */
    float           horUncEllipseOrientAzimuth;

    Gnss_ApTimeStampStructType               timeStamp;
    /** Gnss sv used in position data */
    GnssSvUsedInPosition gnss_sv_used_ids;
    /** Gnss sv used in position data for multiband */
    GnssSvMbUsedInPosition gnss_mb_sv_used_ids;
    /** Nav solution mask to indicate sbas corrections */
    LocNavSolutionMask  navSolutionMask;
    /** Position technology used in computing this fix */
    LocPosTechMask tech_mask;
    /** SV Info source used in computing this fix */
    LocSvInfoSource sv_source;
    /** Body Frame Dynamics: 4wayAcceleration and pitch set with validity */
    GnssLocationPositionDynamics bodyFrameData;
    /** GPS Time */
    GPSTimeStruct gpsTime;
    GnssSystemTime gnssSystemTime;
    /** Dilution of precision associated with this position*/
    LocExtDOP extDOP;
    /** North standard deviation.
        Unit: Meters */
    float northStdDeviation;
    /** East standard deviation.
        Unit: Meters */
    float eastStdDeviation;
    /** North Velocity.
        Unit: Meters/sec */
    float northVelocity;
    /** East Velocity.
        Unit: Meters/sec */
    float eastVelocity;
    /** Up Velocity.
        Unit: Meters/sec */
    float upVelocity;
    /** North Velocity standard deviation.
     *  Unit: Meters/sec.
     *  Confidence level is at 68% */
    float northVelocityStdDeviation;
    /** East Velocity standard deviation.
     *  Unit: Meters/sec
     *  Confidence level is at 68%   */
    float eastVelocityStdDeviation;
    /** Up Velocity standard deviation
     *  Unit: Meters/sec
     *  Confidence level is at 68% */
    float upVelocityStdDeviation;
    /** Estimated clock bias. Unit: Nano seconds */
    float clockbiasMeter;
    /** Estimated clock bias std deviation. Unit: Nano seconds */
    float clockBiasStdDeviationMeter;
    /** Estimated clock drift. Unit: Meters/sec */
    float clockDrift;
    /** Estimated clock drift std deviation. Unit: Meters/sec */
    float clockDriftStdDeviation;
    /** Number of valid reference stations. Range:[0-4] */
    uint8_t numValidRefStations;
    /** Reference station(s) number */
    uint16_t referenceStation[4];
    /** Number of measurements received for use in fix.
        Shall be used as maximum index in-to svUsageInfo[].
        Set to 0, if svUsageInfo reporting is not supported.
        Range: 0-EP_GNSS_MAX_MEAS */
    uint8_t numOfMeasReceived;
    /** Measurement Usage Information */
    GpsMeasUsageInfo measUsageInfo[GNSS_SV_MAX];
    /** Leap Seconds */
    uint8_t leapSeconds;
    /** Time uncertainty in milliseconds,
     *  SPE engine: confidence level is 99%
     *  all other engines: confidence level is not specified */
    float timeUncMs;
    /** Heading Rate is in NED frame.
        Range: 0 to 359.999. 946
        Unit: Degrees per Seconds */
    float headingRateDeg;
    /** Sensor calibration confidence percent. Range: 0 - 100 */
    uint8_t calibrationConfidence;
    DrCalibrationStatusMask calibrationStatus;
    /** location engine type. When the fix. when the type is set to
        LOC_ENGINE_SRC_FUSED, the fix is the propagated/aggregated
        reports from all engines running on the system (e.g.:
        DR/SPE/PPE). To check which location engine contributes to
        the fused output, check for locOutputEngMask. */
    LocOutputEngineType locOutputEngType;
    /** when loc output eng type is set to fused, this field
        indicates the set of engines contribute to the fix. */
    PositioningEngineMask locOutputEngMask;

    /**  DGNSS Correction Source for position report: RTCM, 3GPP
     *   etc. */
    LocDgnssCorrectionSourceType dgnssCorrectionSourceType;

    /**  If DGNSS is used, the SourceID is a 32bit number identifying
     *   the DGNSS source ID */
    uint32_t dgnssCorrectionSourceID;

    /** If DGNSS is used, which constellation was DGNSS used for to
     *  produce the pos report. */
    GnssConstellationTypeMask dgnssConstellationUsage;

    /** If DGNSS is used, DGNSS Reference station ID used for
     *  position report */
    uint16_t dgnssRefStationId;

    /**  If DGNSS is used, DGNSS data age in milli-seconds  */
    uint32_t dgnssDataAgeMsec;

    /** When robust location is enabled, this field
     * will how well the various input data considered for
     * navigation solution conform to expectations.
     * Range: 0 (least conforming) to 1 (most conforming) */
    float conformityIndex;
    GnssLocationPositionDynamicsExt bodyFrameDataExt;
    /** VRR-based latitude/longitude/altitude */
    LLAInfo llaVRPBased;
    /** VRR-based east, north, and up velocity */
    float enuVelocityVRPBased[3];
    DrSolutionStatusMask drSolutionStatusMask;
    /** When this field is valid, it will indicates whether altitude
     *  is assumed or calculated.
     *  false: Altitude is calculated.
     *  true:  Altitude is assumed; there may not be enough
     *         satellites to determine the precise altitude. */
    bool altitudeAssumed;

    /** Integrity risk used for protection level parameters.
     *  Unit of 2.5e-10. Valid range is [1 to (4e9-1)].
     *  Other values means integrity risk is disabled and
     *  GnssLocation::protectAlongTrack,
     *  GnssLocation::protectCrossTrack and
     *  GnssLocation::protectVertical will not be available.
     */
    uint32_t integrityRiskUsed;
    /** Along-track protection level at specified integrity risk, in
     *  unit of meter.
     */
    float    protectAlongTrack;
   /** Cross-track protection level at specified integrity risk, in
     *  unit of meter.
     */
    float    protectCrossTrack;
    /** Vertical component protection level at specified integrity
     *  risk, in unit of meter.
     */
    float    protectVertical;
    /** System Tick at GPS Time */
    uint64_t systemTick;
    /** Uncertainty for System Tick at GPS Time in milliseconds   */
    float systemTickUnc;

    // number of dgnss station id that is valid in dgnssStationId array
    uint32_t  numOfDgnssStationId;
    // List of DGNSS station IDs providing corrections.
    //   Range:
    //   - SBAS --  120 to 158 and 183 to 191.
    //   - Monitoring station -- 1000-2023 (Station ID biased by 1000).
    //   - Other values reserved.
    uint16_t dgnssStationId[DGNSS_STATION_ID_MAX];
    /** helper function to check sanity of accurate time */
    bool isReportTimeAccurate() const {
        return ((gnssSystemTime.hasAccurateGpsTime() == true) &&
            (flags & GPS_LOCATION_EXTENDED_HAS_SYSTEM_TICK) &&
            (systemTick != 0) &&
            (flags & GPS_LOCATION_EXTENDED_HAS_SYSTEM_TICK_UNC) &&
            (systemTickUnc != 0.0f));
    }

} GpsLocationExtended;

// struct that contains complete position info from engine
typedef struct {
    UlpLocation location;
    GpsLocationExtended locationExtended;
    enum loc_sess_status sessionStatus;
} EngineLocationInfo;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_H */
