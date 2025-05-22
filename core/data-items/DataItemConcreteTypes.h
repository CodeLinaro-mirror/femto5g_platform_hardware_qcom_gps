/* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#ifndef DATAITEM_CONCRETETYPES_H
#define DATAITEM_CONCRETETYPES_H

#include <string>
#include <cstring>
#include <sstream>
#include <DataItemId.h>
#include <IDataItemCore.h>
#include <gps_extended_c.h>
#include <inttypes.h>
#include <unordered_set>
#include <log_util.h>

namespace loc_core {
/************** C style data item related data structure definitions ******************/
//Network connection data structure
#define NETWORKINFO_DEFAULT_TYPE 300
enum NetworkType {
    TYPE_MOBILE = 0,
    TYPE_WIFI,
    TYPE_ETHERNET,
    TYPE_BLUETOOTH,
    TYPE_MMS,
    TYPE_SUPL,
    TYPE_DUN,
    TYPE_HIPRI,
    TYPE_WIMAX,
    TYPE_PROXY,
    TYPE_UNKNOWN,
};

#define MAX_NETWORK_INFO_STR_LEN 20
struct LocNetworkInfo {
    uint64_t mAllTypes;
    int32_t mType;
    bool mAvailable;
    bool mConnected;
    bool mRoaming;
    // Unique network handle ID
    uint64_t networkHandle;
    // Type of network for corresponding network handle
    NetworkType networkType;
    char mApn[MAX_NETWORK_INFO_STR_LEN];
};

//LocRil data structure
enum LocRilCellType {
    UNKNOWN = 0,
    GSM     = 1,
    WCDMA   = 2,
    CDMA    = 3,
    LTE     = 4,
    NR      = 5,
};

#define LOC_RIL_CELL_INFO_UNAVAILABLE UINT32_MAX
struct LocRilCellInfo {
    LocRilCellType cellType;
    uint16_t regionId1;  // MCC
    uint16_t regionId2;  // MNC
    uint32_t regionId3;  // TAC (24-bit) or LAC (16-bit) or UNAVAILABLE
    uint64_t regionId4;  // Global Cell ID (64-bit for NR, and 32-bit for WCDMA and LTE)
    uint32_t frequency;  // ARFCN, or UNAVAILABLE
    uint32_t physicalId; // physical id: PSC (UMTS), PCI (LTE/NR), or UNAVAILABLE
};

//Time zone data structure
struct LocTimezoneInfo {
    int64_t mCurrTimeMillis;
    int32_t mRawOffsetTZ;
    int32_t mDstOffsetTZ;
};

// WiFi supplicant info data structure
enum WifiSupplicantState {
    DISCONNECTED,
    INTERFACE_DISABLED,
    INACTIVE,
    SCANNING,
    AUTHENTICATING,
    ASSOCIATING,
    ASSOCIATED,
    FOUR_WAY_HANDSHAKE,
    GROUP_HANDSHAKE,
    COMPLETED,
    DORMANT,
    UNINITIALIZED,
    INVALID
};

#define MAC_ADDRESS_LENGTH    6
#define WIFI_AP_SSID_LENGTH 33
struct LocWifiSupplicantInfo {
    /* Represents whether access point attach state*/
    WifiSupplicantState mState;
    /* Represents info on whether ap mac address is valid */
    bool mApMacAddressValid;
    /* Represents mac address of the wifi access point*/
    uint8_t mApMacAddress[MAC_ADDRESS_LENGTH];
    /* Represents info on whether ap SSID is valid */
    bool mWifiApSsidValid;
    /* Represents Wifi SSID string*/
    char mWifiApSsid[WIFI_AP_SSID_LENGTH];
};

//Feature status data structure
#define MAX_LOC_FID_NUM 20
struct LocFeatureStatusInfo {
    uint8_t len;
    uint32_t mFids[MAX_LOC_FID_NUM];
};

//WWAN app info data structure
#define APP_COOKIE_MAX_SIZE 40
#define APP_HASH_KEY_MAX_SIZE 64
#define PACKAGE_NAME_MAX_SIZE 256
struct LocWwanAppInfo {
    uint32_t mPid;
    uint32_t mUid;
    bool mAppHasFinePermission;
    bool mAppHasBackgroundPermission;
    char mAppHash[APP_HASH_KEY_MAX_SIZE];
    char mAppPackageName[PACKAGE_NAME_MAX_SIZE];
    char mAppCookie[APP_COOKIE_MAX_SIZE];
};

bool copyStringToCharArray(const std::string& src, char* dest, size_t destSize);

/************** Data item classes definitions ******************/
class NetworkInfoDataItem: public IDataItemCore {
public:
    NetworkInfoDataItem(int32_t type = 0,
                        bool available = false,
                        bool connected = false,
                        bool roaming = false,
                        uint64_t networkHandle = 0,
                        std::string apn = "") {
        mId = NETWORKINFO_DATA_ITEM_ID;
        mName = NETWORKINFO_CARD;
        mNetInfo.mType = type;
        mNetInfo.networkType = (NetworkType)type;
        mNetInfo.mAllTypes = typeToAllTypes(mNetInfo.networkType);
        mNetInfo.networkHandle = networkHandle;
        mNetInfo.mConnected = connected;
        mNetInfo.mAvailable = available;
        mNetInfo.mRoaming = roaming;
        copyStringToCharArray(apn, mNetInfo.mApn, sizeof(mNetInfo.mApn));
        setBlobPtr((void*)(&mNetInfo), sizeof(LocNetworkInfo));
    }
    virtual void stringify(std::string& valueStr) override;
    inline uint64_t getAllTypes() { return mNetInfo.mAllTypes; }
    inline static NetworkType getNormalizedType(int32_t type) {
        NetworkType typeout = TYPE_UNKNOWN;
        switch (type) {
            case 100:
                typeout = TYPE_WIFI;
                break;
            case 101:
                typeout = TYPE_ETHERNET;
                break;
            case 102:
                typeout = TYPE_BLUETOOTH;
                break;
            case 201:
                typeout = TYPE_MOBILE;
                break;
            case 202:
                typeout = TYPE_DUN;
                break;
            case 203:
                typeout = TYPE_HIPRI;
                break;
            case 204:
                typeout = TYPE_MMS;
                break;
            case 205:
                typeout = TYPE_SUPL;
                break;
            case 220:
                typeout = TYPE_WIMAX;
                break;
            case 300:
            default:
                typeout = TYPE_UNKNOWN;
                break;
        }
        return typeout;
   }
   inline uint64_t typeToAllTypes(NetworkType type) {
       return (type >= TYPE_UNKNOWN || type < TYPE_MOBILE) ?  0 : (1<<type);
   }

//Data members
   LocNetworkInfo mNetInfo;
};

class ENHDataItem: public IDataItemCore {
public:
    enum ENHDataItemStatusMasks {
        ENH_USER_CONSENT_ALLOWED_MASK = 0x01,
        ENH_EMBARGO_REGION_ALLOWED_MASK = 0x02,
    };

    ENHDataItem(bool isEnabled = false,
            uint8_t mask = (ENH_USER_CONSENT_ALLOWED_MASK|ENH_EMBARGO_REGION_ALLOWED_MASK)) {
         mId = ENH_DATA_ITEM_ID;
         mName = ENH_CARD;
         mEnhStatusMask = currentEnhStatusMask;
         if (isEnabled) {
             mEnhStatusMask |= mask;
         } else {
             mEnhStatusMask &= ~mask;
         }
         setBlobPtr((void*)(&mEnhStatusMask), sizeof(uint8_t));
         currentEnhStatusMask = mEnhStatusMask;
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    //Mask for ENH status,
    //bit 0 - true if user consent is true
    //bit 1 - true if device is not in embrago region
    //Consolidated user consent is true if both bits are set to true
    uint8_t mEnhStatusMask;
    //currentEnhStatusMask is used to cache latest ENH status
    //then when eventOptInStatus or eventRegionAllowedStatus is called,
    //we can update consolidated ENH combined with latest status and
    //current status.
    static uint8_t currentEnhStatusMask;
};

class GPSStateDataItem: public IDataItemCore {
public:
    GPSStateDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = GPSSTATE_DATA_ITEM_ID;
        mName = GPSSTATE_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class WifiHardwareStateDataItem: public IDataItemCore {
public:
    WifiHardwareStateDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = WIFIHARDWARESTATE_DATA_ITEM_ID;
        mName = WIFIHARDWARESTATE_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class TimeZoneChangeDataItem: public IDataItemCore {
public:
    TimeZoneChangeDataItem(int64_t currentTimeMillis = 0,
                           int32_t rawOffsetTZ = 0,
                           int32_t dstOffsetTZ = 0) {
        mId = TIMEZONE_CHANGE_DATA_ITEM_ID;
        mName = TIMEZONECHANGE_CARD;
        mTimezoneInfo.mCurrTimeMillis = currentTimeMillis;
        mTimezoneInfo.mRawOffsetTZ = rawOffsetTZ;
        mTimezoneInfo.mDstOffsetTZ = dstOffsetTZ;
        setBlobPtr((void*)(&mTimezoneInfo), sizeof(LocTimezoneInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocTimezoneInfo mTimezoneInfo;
};

class TimeChangeDataItem: public IDataItemCore {
public:
    TimeChangeDataItem(int64_t currTimeMillis = 0,
                       int32_t rawOffset = 0,
                       int32_t dstOffset = 0) {
        mId = TIME_CHANGE_DATA_ITEM_ID;
        mName = TIMECHANGE_CARD;
        mTimeInfo.mCurrTimeMillis = currTimeMillis;
        mTimeInfo.mRawOffsetTZ = rawOffset;
        mTimeInfo.mDstOffsetTZ = dstOffset;
        setBlobPtr((void*)(&mTimeInfo), sizeof(LocTimezoneInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocTimezoneInfo mTimeInfo;
};

class ModelDataItem: public IDataItemCore {
public:
    ModelDataItem(const std::string& name = "0") :
            mModel (name) {
        mId = MODEL_DATA_ITEM_ID;
        mName = MODEL_CARD;
        setBlobPtr((void*)(mModel.c_str()), mModel.size() + 1);
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    std::string mModel;
};

class ManufacturerDataItem: public IDataItemCore {
public:
    ManufacturerDataItem(const std::string& name = "0") :
            mManufacturer(name) {
        mId = MANUFACTURER_DATA_ITEM_ID;
        mName = MANUFACTURER_CARD;
        setBlobPtr((void*)(mManufacturer.c_str()), mManufacturer.size() + 1);
    }
    virtual void stringify(std::string& valueStr) override;
//Data members
    std::string mManufacturer;
};

class RilCellInfoDataItem: public IDataItemCore {
public:
    RilCellInfoDataItem(const LocRilCellInfo& cellInfo = {UNKNOWN, 0, 0, 0, 0, 0, 0}) :
            mRilInfo(cellInfo) {
        mId = RILCELLINFO_DATA_ITEM_ID;
        mName = RILCELLINFO_CARD;
        setBlobPtr((void*)(&mRilInfo), sizeof(LocRilCellInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocRilCellInfo mRilInfo;
};

class WifiSupplicantStatusDataItem: public IDataItemCore {
public:
    WifiSupplicantStatusDataItem(
        const LocWifiSupplicantInfo& info = {INVALID, false, "", false, ""}) :
            mWifiSupplicantInfo(info) {
        mId = WIFI_SUPPLICANT_STATUS_DATA_ITEM_ID;
        mName = WIFI_SUPPLICANT_STATUS_CARD;
        setBlobPtr((void*)(&mWifiSupplicantInfo), sizeof(LocWifiSupplicantInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocWifiSupplicantInfo mWifiSupplicantInfo;
};

class MccmncDataItem: public IDataItemCore {
public:
    MccmncDataItem(const std::string& name = "0") :
            mMccmnc(name) {
        mId = MCCMNC_DATA_ITEM_ID;
        mName = MCCMNC_CARD;
        setBlobPtr((void*)(mMccmnc.c_str()), mMccmnc.size() + 1);
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    std::string mMccmnc;
};

class InEmergencyCallDataItem: public IDataItemCore {
public:
    InEmergencyCallDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = IN_EMERGENCY_CALL_DATA_ITEM_ID;
        mName = IN_EMERGENCY_CALL_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class PreciseLocationEnabledDataItem: public IDataItemCore {
public:
    PreciseLocationEnabledDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = PRECISE_LOCATION_ENABLED_DATA_ITEM_ID;
        mName = PRECISE_LOCATION_ENABLED_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class TrackingStartedDataItem: public IDataItemCore {
public:
    TrackingStartedDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = TRACKING_STARTED_DATA_ITEM_ID;
        mName = TRACKING_STARTED_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class NtripStartedDataItem: public IDataItemCore {
public:
    NtripStartedDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = NTRIP_STARTED_DATA_ITEM_ID;
        mName = NTRIP_STARTED_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class LocFeatureStatusDataItem: public IDataItemCore {
public:
    LocFeatureStatusDataItem(std::unordered_set<int> fids = {}) {
        mId = LOC_FEATURE_STATUS_DATA_ITEM_ID;
        mName = LOC_FEATURE_STATUS_CARD;
        mInfo.len = fids.size();
        int i = 0;
        for (const auto& elem : fids) {
            if (i < MAX_LOC_FID_NUM) {
                mInfo.mFids[i] = elem;
            }
            i++;
        }
        setBlobPtr((void*)(&mInfo), sizeof(LocFeatureStatusInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocFeatureStatusInfo mInfo;
};

class NlpSessionStartedDataItem: public IDataItemCore {
public:
    NlpSessionStartedDataItem(bool enabled = false) :
            mIsEnabled(enabled) {
        mId = NETWORK_POSITIONING_STARTED_DATA_ITEM_ID;
        mName = NLP_SESSION_STARTED_CARD;
        setBlobPtr((void*)(&mIsEnabled), sizeof(bool));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    bool mIsEnabled;
};

class WwanAppInfoDataItem: public IDataItemCore {
public:
    WwanAppInfoDataItem(uint32_t pid = 0,
                uint32_t uid = 0,
                bool appHasFinePermission = false,
                bool appHasBackgroundPermission = false,
                std::string appHash = "",
                std::string appPackageName = "",
                std::string appCookie = "") {
        mId = WWAN_APP_INFO_DATA_ITEM_ID;
        mName = WWAN_APP_INFO_CARD;
        mWwanAppInfo.mPid = pid;
        mWwanAppInfo.mUid = uid;
        mWwanAppInfo.mAppHasFinePermission = appHasFinePermission;
        mWwanAppInfo.mAppHasBackgroundPermission = appHasBackgroundPermission;
        copyStringToCharArray(appHash, mWwanAppInfo.mAppHash, sizeof(mWwanAppInfo.mAppHash));
        copyStringToCharArray(appPackageName, mWwanAppInfo.mAppPackageName,
                sizeof(mWwanAppInfo.mAppPackageName));
        copyStringToCharArray(appCookie, mWwanAppInfo.mAppCookie, sizeof(mWwanAppInfo.mAppCookie));
        setBlobPtr((void*)(&mWwanAppInfo), sizeof(LocWwanAppInfo));
    }
    virtual void stringify(std::string& valueStr) override;

//Data members
    LocWwanAppInfo mWwanAppInfo;
};

class DataItemsFactory {
public:
    static IDataItemCore* createNewDataItem(const DataItemId& id);
};

} // namespace loc_core

#endif //DATAITEM_CONCRETETYPES_H
