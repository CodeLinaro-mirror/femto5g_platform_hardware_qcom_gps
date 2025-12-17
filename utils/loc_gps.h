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

/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */

/** Requested recurrence mode for GPS operation. */
typedef uint32_t LocGpsPositionRecurrence;
/* IMPORTANT: Note that the following values must match
 * constants in GpsLocationProvider.java. */
/** Receive GPS fixes on a recurring basis at a specified period. */
#define LOC_GPS_POSITION_RECURRENCE_PERIODIC    0
/** Request a single shot GPS fix. */
#define LOC_GPS_POSITION_RECURRENCE_SINGLE      1

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

typedef uint16_t LocSubId;
enum {
    LOC_DEFAULT_SUB   = 0,
    LOC_PRIMARY_SUB   = 1,
    LOC_SECONDARY_SUB = 2,
    LOC_TERTIARY_SUB  = 3
};

typedef uint16_t LocApnIpType;
#define LOC_APN_IP_INVALID          0
#define LOC_APN_IP_IPV4             4
#define LOC_APN_IP_IPV6             6
#define LOC_APN_IP_IPV4V6           10

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

#ifdef __cplusplus
}
#endif

#endif /* LOC_GPS_H */

