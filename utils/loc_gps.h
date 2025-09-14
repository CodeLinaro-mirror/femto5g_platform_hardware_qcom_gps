/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 *
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
Changes from Qualcomm Technologies, Inc. are provided under the following license:
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef LOC_GPS_H
#define LOC_GPS_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Milliseconds since January 1, 1970 */
typedef int64_t LocGpsUtcTime;

/** Maximum number of SVs for loc_gps_sv_status_callback(). */
#define LOC_GNSS_MAX_SVS 64

/** Requested operational mode for GPS operation. */
typedef uint32_t LocGpsPositionMode;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** Mode for running GPS standalone (no assistance). */
#define LOC_GPS_POSITION_MODE_STANDALONE    0
/** AGPS MS-Based mode. */
#define LOC_GPS_POSITION_MODE_MS_BASED      1
/**
 * AGPS MS-Assisted mode. This mode is not maintained by the platform anymore.
 * It is strongly recommended to use LOC_GPS_POSITION_MODE_MS_BASED instead.
 */
#define LOC_GPS_POSITION_MODE_MS_ASSISTED   2

/** Requested recurrence mode for GPS operation. */
typedef uint32_t LocGpsPositionRecurrence;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** Receive GPS fixes on a recurring basis at a specified period. */
#define LOC_GPS_POSITION_RECURRENCE_PERIODIC    0
/** Request a single shot GPS fix. */
#define LOC_GPS_POSITION_RECURRENCE_SINGLE      1

/** GPS status event values. */
typedef uint16_t LocGpsStatusValue;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** GPS status unknown. */
#define LOC_GPS_STATUS_NONE             0
/** GPS has begun navigating. */
#define LOC_GPS_STATUS_SESSION_BEGIN    1
/** GPS has stopped navigating. */
#define LOC_GPS_STATUS_SESSION_END      2
/** GPS has powered on but is not navigating. */
#define LOC_GPS_STATUS_ENGINE_ON        3
/** GPS is powered off. */
#define LOC_GPS_STATUS_ENGINE_OFF       4

/** Flags to indicate which values are valid in a LocGpsLocation. */
typedef uint16_t LocGpsLocationFlags;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** LocGpsLocation has valid latitude and longitude. */
#define LOC_GPS_LOCATION_HAS_LAT_LONG   0x0001
/** LocGpsLocation has valid altitude. */
#define LOC_GPS_LOCATION_HAS_ALTITUDE   0x0002
/** LocGpsLocation has valid speed. */
#define LOC_GPS_LOCATION_HAS_SPEED      0x0004
/** LocGpsLocation has valid bearing. */
#define LOC_GPS_LOCATION_HAS_BEARING    0x0008
/** LocGpsLocation has valid accuracy. */
#define LOC_GPS_LOCATION_HAS_ACCURACY   0x0010
/** LocGpsLocation has valid vertical uncertainity */
#define LOC_GPS_LOCATION_HAS_VERT_UNCERTAINITY   0x0020
/** LocGpsLocation has valid speed accuracy */
#define LOC_GPS_LOCATION_HAS_SPEED_ACCURACY   0x0040
/** LocGpsLocation has valid bearing accuracy */
#define LOC_GPS_LOCATION_HAS_BEARING_ACCURACY 0x0080
/** Location has valid source information. */
#define LOC_GPS_LOCATION_HAS_SOURCE_INFO   0x0200

/** Flags for the loc_gps_set_capabilities callback. */

/**
 * GPS HAL schedules fixes for LOC_GPS_POSITION_RECURRENCE_PERIODIC mode. If this is
 * not set, then the framework will use 1000ms for min_interval and will start
 * and call start() and stop() to schedule the GPS.
 */
/** GPS supports MS-Based AGPS mode */
#define LOC_GPS_CAPABILITY_MSB              (1 << 1)
/** GPS supports MS-Assisted AGPS mode */
#define LOC_GPS_CAPABILITY_MSA              (1 << 2)
/** GPS supports on demand time injection */
#define LOC_GPS_CAPABILITY_ON_DEMAND_TIME   (1 << 4)

/**
 * Flags used to specify which aiding data to delete when calling
 * delete_aiding_data().
 */
typedef uint16_t LocGpsAidingData;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
#define LOC_GPS_DELETE_EPHEMERIS        0x0001
#define LOC_GPS_DELETE_ALMANAC          0x0002
#define LOC_GPS_DELETE_POSITION         0x0004
#define LOC_GPS_DELETE_TIME             0x0008
#define LOC_GPS_DELETE_IONO             0x0010
#define LOC_GPS_DELETE_UTC              0x0020
#define LOC_GPS_DELETE_HEALTH           0x0040
#define LOC_GPS_DELETE_SVDIR            0x0080
#define LOC_GPS_DELETE_SVSTEER          0x0100
#define LOC_GPS_DELETE_SADATA           0x0200
#define LOC_GPS_DELETE_RTI              0x0400
#define LOC_GPS_DELETE_MB_DATA          0x0800
#define LOC_GPS_DELETE_CELLDB_INFO      0x8000
#define LOC_GPS_DELETE_ALL              0xFFFF

/** AGPS type */
typedef uint16_t LocAGpsType;
#define LOC_AGPS_TYPE_ANY           0
#define LOC_AGPS_TYPE_SUPL          1
#define LOC_AGPS_TYPE_C2K           2
#define LOC_AGPS_TYPE_WWAN_ANY      3
#define LOC_AGPS_TYPE_WIFI          4
#define LOC_AGPS_TYPE_SUPL_ES       5

typedef uint16_t LocSubId;
#define LOC_DEFAULT_SUB    0
#define LOC_PRIMARY_SUB    1
#define LOC_SECONDARY_SUB  2
#define LOC_TERTIARY_SUB   3

typedef uint16_t LocAGpsSetIDType;
#define LOC_AGPS_SETID_TYPE_NONE    0
#define LOC_AGPS_SETID_TYPE_IMSI    1
#define LOC_AGPS_SETID_TYPE_MSISDN  2

typedef uint16_t LocApnIpType;
#define LOC_APN_IP_INVALID          0
#define LOC_APN_IP_IPV4             4
#define LOC_APN_IP_IPV6             6
#define LOC_APN_IP_IPV4V6           10

/**
 * String length constants
 */
#define LOC_GPS_NI_SHORT_STRING_MAXLEN      256
#define LOC_GPS_NI_LONG_STRING_MAXLEN       2048

/**
 * LocGpsNiType constants
 */
typedef uint32_t LocGpsNiType;
#define LOC_GPS_NI_TYPE_VOICE              1
#define LOC_GPS_NI_TYPE_UMTS_SUPL          2
#define LOC_GPS_NI_TYPE_UMTS_CTRL_PLANE    3
/*Emergency SUPL*/
#define LOC_GPS_NI_TYPE_EMERGENCY_SUPL     4

/**
 * LocGpsNiNotifyFlags constants
 */
typedef uint32_t LocGpsNiNotifyFlags;
/** NI requires notification */
#define LOC_GPS_NI_NEED_NOTIFY          0x0001
/** NI requires verification */
#define LOC_GPS_NI_NEED_VERIFY          0x0002
/** NI requires privacy override, no notification/minimal trace */
#define LOC_GPS_NI_PRIVACY_OVERRIDE     0x0004

/**
 * GPS NI responses, used to define the response in
 * NI structures
 */
typedef int LocGpsUserResponseType;
#define LOC_GPS_NI_RESPONSE_ACCEPT         1
#define LOC_GPS_NI_RESPONSE_DENY           2
#define LOC_GPS_NI_RESPONSE_NORESP         3

/**
 * NI data encoding scheme
 */
typedef int LocGpsNiEncodingType;
#define LOC_GPS_ENC_NONE                   0
#define LOC_GPS_ENC_SUPL_GSM_DEFAULT       1
#define LOC_GPS_ENC_SUPL_UTF8              2
#define LOC_GPS_ENC_SUPL_UCS2              3
#define LOC_GPS_ENC_UNKNOWN                -1

/** AGPS status event values. */
typedef uint8_t LocAGpsStatusValue;
/** GPS requests data connection for AGPS. */
#define LOC_GPS_REQUEST_AGPS_DATA_CONN  1
/** GPS releases the AGPS data connection. */
#define LOC_GPS_RELEASE_AGPS_DATA_CONN  2
/** AGPS data connection initiated */
#define LOC_GPS_AGPS_DATA_CONNECTED     3
/** AGPS data connection completed */
#define LOC_GPS_AGPS_DATA_CONN_DONE     4
/** AGPS data connection failed */
#define LOC_GPS_AGPS_DATA_CONN_FAILED   5

typedef uint16_t LocAGpsRefLocationType;
#define LOC_AGPS_REF_LOCATION_TYPE_GSM_CELLID   1
#define LOC_AGPS_REF_LOCATION_TYPE_UMTS_CELLID  2
#define LOC_AGPS_REF_LOCATION_TYPE_MAC          3
#define LOC_AGPS_REF_LOCATION_TYPE_LTE_CELLID   4

/** Represents a location. */
typedef struct {
    /** set to sizeof(LocGpsLocation) */
    uint32_t        size;
    /** Contains LocGpsLocationFlags bits. */
    uint16_t        flags;
    /** Represents latitude in degrees. */
    double          latitude;
    /** Represents longitude in degrees. */
    double          longitude;
    /**
     * Represents altitude in meters above the WGS 84 reference ellipsoid.
     */
    double          altitude;
    /** Represents horizontal speed in meters per second. */
    float           speed;
    /** Represents heading in degrees. */
    float           bearing;
    /** Represents expected accuracy in meters. */
    float           accuracy;
    /** Represents the expected vertical uncertainity in meters*/
    float           vertUncertainity;
    /** Timestamp for the location fix. */
    LocGpsUtcTime   timestamp;
    /** Elapsed RealTime in nanosends */
    uint64_t        elapsedRealTime;
    /** Elapsed Real Time Uncertainty in nanosends */
    uint64_t        elapsedRealTimeUnc;
} LocGpsLocation;

/** Represents the status. */
typedef struct {
    /** set to sizeof(LocGpsStatus) */
    size_t          size;
    LocGpsStatusValue status;
} LocGpsStatus;

/**
 * Callback with status information. Can only be called from a thread created by
 * create_thread_cb.
 */
typedef void (* loc_gps_status_callback)(LocGpsStatus* status);

/**
 * Callback to inform framework of the GPS engine's capabilities. Capability
 * parameter is a bit field of LOC_GPS_CAPABILITY_* flags.
 */
typedef void (* loc_gps_set_capabilities)(uint32_t capabilities);

/**
 * Callback utility for acquiring the GPS wakelock. This can be used to prevent
 * the CPU from suspending while handling GPS events.
 */
typedef void (* loc_gps_acquire_wakelock)();

/** Callback utility for releasing the GPS wakelock. */
typedef void (* loc_gps_release_wakelock)();

/** Callback for requesting NTP time */
typedef void (* loc_gps_request_utc_time)();

/**
 * Callback for creating a thread that can call into the Java framework code.
 * This must be used to create any threads that report events up to the
 * framework.
 */
typedef pthread_t (* loc_gps_create_thread)(const char* name, void (*start)(void *), void* arg);


/*
 * Represents the status of AGPS augmented to support IPv4 and IPv6.
 */
typedef struct {
    /** set to sizeof(LocAGpsStatus) */
    size_t                  size;

    LocAGpsType                type;
    LocAGpsStatusValue         status;

    /**
     * Must be set to a valid IPv4 address if the field 'addr' contains an IPv4
     * address, or set to INADDR_NONE otherwise.
     */
    uint32_t                ipaddr;

    /**
     * Must contain the IPv4 (AF_INET) or IPv6 (AF_INET6) address to report.
     * Any other value of addr.ss_family will be rejected.
     */
    struct sockaddr_storage addr;
} LocAGpsStatus;

typedef struct {
    size_t  length;
    u_char* data;
} LocDerEncodedCertificate;

/** Represents an NI request */
typedef struct {
    /** set to sizeof(LocGpsNiNotification) */
    size_t          size;

    /**
     * An ID generated by HAL to associate NI notifications and UI
     * responses
     */
    int             notification_id;

    /**
     * An NI type used to distinguish different categories of NI
     * events, such as LOC_GPS_NI_TYPE_VOICE, LOC_GPS_NI_TYPE_UMTS_SUPL, ...
     */
    LocGpsNiType       ni_type;

    /**
     * Notification/verification options, combinations of LocGpsNiNotifyFlags constants
     */
    LocGpsNiNotifyFlags notify_flags;

    /**
     * Timeout period to wait for user response.
     * Set to 0 for no time out limit.
     */
    int             timeout;

    /**
     * Default response when time out.
     */
    LocGpsUserResponseType default_response;

    /**
     * Requestor ID
     */
    char            requestor_id[LOC_GPS_NI_SHORT_STRING_MAXLEN];

    /**
     * Notification message. It can also be used to store client_id in some cases
     */
    char            text[LOC_GPS_NI_LONG_STRING_MAXLEN];

    /**
     * Client name decoding scheme
     */
    LocGpsNiEncodingType requestor_id_encoding;

    /**
     * Client name decoding scheme
     */
    LocGpsNiEncodingType text_encoding;

    /**
     * A pointer to extra data. Format:
     * key_1 = value_1
     * key_2 = value_2
     */
    char           extras[LOC_GPS_NI_LONG_STRING_MAXLEN];

} LocGpsNiNotification;


#define LOC_GPS_GEOFENCE_ENTERED     (1<<0L)
#define LOC_GPS_GEOFENCE_EXITED      (1<<1L)

#define LOC_GPS_GEOFENCE_DWELL_INSIDE  (1<<0L)
#define LOC_GPS_GEOFENCE_DWELL_OUTSIDE (1<<1L)

#define LOC_GPS_GEOFENCE_CONFIDENCE_LOW    1
#define LOC_GPS_GEOFENCE_CONFIDENCE_MEDIUM 2
#define LOC_GPS_GEOFENCE_CONFIDENCE_HIGH   3

#ifdef __cplusplus
}
#endif

#endif /* LOC_GPS_H */

