/* Copyright (c) 2015-2017,2021 The Linux Foundation. All rights reserved.
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

#ifndef __DATAITEMID_H__
#define __DATAITEMID_H__

/**
 * Enumeration of Data Item types
 * When add/remove/update changes are made to Data Items, this file needs to be updated
 * accordingly
 */
enum DataItemId {
    INVALID_DATA_ITEM_ID = 0,
    ENH_DATA_ITEM_ID,
    GPSSTATE_DATA_ITEM_ID,
    WIFIHARDWARESTATE_DATA_ITEM_ID,
    NETWORKINFO_DATA_ITEM_ID,
    RILCELLINFO_DATA_ITEM_ID,
    MODEL_DATA_ITEM_ID,
    MANUFACTURER_DATA_ITEM_ID,
    ASSISTED_GPS_DATA_ITEM_ID,
    TIMEZONE_CHANGE_DATA_ITEM_ID,
    TIME_CHANGE_DATA_ITEM_ID,
    WIFI_SUPPLICANT_STATUS_DATA_ITEM_ID,
    MCCMNC_DATA_ITEM_ID,
    IN_EMERGENCY_CALL_DATA_ITEM_ID,
    PRECISE_LOCATION_ENABLED_DATA_ITEM_ID,
    TRACKING_STARTED_DATA_ITEM_ID,
    NTRIP_STARTED_DATA_ITEM_ID,
    LOC_FEATURE_STATUS_DATA_ITEM_ID,
    NETWORK_POSITIONING_STARTED_DATA_ITEM_ID,
    WWAN_APP_INFO_DATA_ITEM_ID,
};

/*** Data item post card name definitions***/
#define ENH_CARD                        "ENH"
#define GPSSTATE_CARD                   "GPS_STATE"
#define WIFIHARDWARESTATE_CARD          "WIFI_HARDWARE"
#define NETWORKINFO_CARD                "ACTIVE_NETWORK_INFO"
#define RILCELLINFO_CARD                "RILCELL_INFO"
#define MODEL_CARD                      "MODEL_INFO"
#define MANUFACTURER_CARD               "MANUFACTURER_INFO"
#define TIMEZONECHANGE_CARD             "TIMEZONE_CHANGE"
#define TIMECHANGE_CARD                 "TIME_CHANGE"
#define WIFI_SUPPLICANT_STATUS_CARD     "WIFI_SUPPLICANT_STATUS"
#define MCCMNC_CARD                     "MCCMNC_INFO"
#define IN_EMERGENCY_CALL_CARD          "IN_EMERGENCY_CALL"
#define PRECISE_LOCATION_ENABLED_CARD   "PRECISE_LOCATION_ENABLED"
#define TRACKING_STARTED_CARD           "TRACKING_STARTED"
#define NTRIP_STARTED_CARD              "NTRIP_STARTED"
#define LOC_FEATURE_STATUS_CARD         "LOC_FEATURE_STATUS"
#define NLP_SESSION_STARTED_CARD        "NLP_SESSION_STARTED"
#define WWAN_APP_INFO_CARD              "WWAN_APP_INFO"

#endif // #ifndef __DATAITEMID_H__
