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

#define LOG_TAG "DataItemConcreteTypes"

#include "DataItemConcreteTypes.h"
#include <inttypes.h>


namespace loc_core
{
using namespace std;
bool copyStringToCharArray(const std::string& src, char* dest, size_t destSize) {
    memset(dest, 0, destSize);
    if (dest == nullptr || destSize == 0) {
        return false;
    }
    size_t copyLength = src.size() >= (destSize - 1) ? (destSize - 1) : src.size();
    memcpy(dest, src.c_str(), copyLength);
    dest[copyLength] = '\0';
    return true;
}

void NetworkInfoDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": mAllTypes: " + std::to_string(mNetInfo.mAllTypes);
    valueStr += " mType: " + std::to_string(mNetInfo.mType);
    valueStr += " mAvailable: " + std::to_string(mNetInfo.mAvailable);
    valueStr += " mConnected: " + std::to_string(mNetInfo.mConnected);
    valueStr += " mRoaming: " + std::to_string(mNetInfo.mRoaming);
    valueStr += " networkHandle: " + std::to_string(mNetInfo.networkHandle);
    string apnStr(mNetInfo.mApn);
    valueStr += " mApn: " + apnStr;
}

uint8_t ENHDataItem::currentEnhStatusMask = 0;
void ENHDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": user consent: ";
    valueStr += mEnhStatusMask & ENH_USER_CONSENT_ALLOWED_MASK ? "true" : "false";
    valueStr += ", is region allowed: ";
    valueStr += mEnhStatusMask & ENH_EMBARGO_REGION_ALLOWED_MASK ? "true" : "false";
}

void GPSStateDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void WifiHardwareStateDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void TimeZoneChangeDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": currentTimeMillis: " + std::to_string(mTimezoneInfo.mCurrTimeMillis);
    valueStr += " rawOffset: " + std::to_string(mTimezoneInfo.mRawOffsetTZ);
    valueStr += " dstOffset: " + std::to_string(mTimezoneInfo.mDstOffsetTZ);
}

void TimeChangeDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": currentTimeMillis: " + std::to_string(mTimeInfo.mCurrTimeMillis);
    valueStr += " rawOffset: " + std::to_string(mTimeInfo.mRawOffsetTZ);
    valueStr += " dstOffset: " + std::to_string(mTimeInfo.mDstOffsetTZ);
}

void ModelDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": " + mModel;
}

void ManufacturerDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": " + mManufacturer;
}

void RilCellInfoDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": cellType: " + std::to_string((int)mRilInfo.cellType);
    valueStr += " regionId1: " + std::to_string(mRilInfo.regionId1);
    valueStr += " regionId2: " + std::to_string(mRilInfo.regionId2);
    valueStr += " regionId3: " + std::to_string(mRilInfo.regionId3);
    valueStr += " regionId4: " + std::to_string(mRilInfo.regionId4);
    valueStr += " frequency: " + std::to_string(mRilInfo.frequency);
    valueStr += " physicalId: " + std::to_string(mRilInfo.physicalId);
}

void WifiSupplicantStatusDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": mState:" + std::to_string((int)mWifiSupplicantInfo.mState);
    valueStr += " mApMacAddressValid: " + std::to_string(mWifiSupplicantInfo.mApMacAddressValid);
    valueStr += " mApMacAddress: ";
    string marAddrStr(mWifiSupplicantInfo.mApMacAddress,
            mWifiSupplicantInfo.mApMacAddress + sizeof(mWifiSupplicantInfo.mApMacAddress));
    valueStr += marAddrStr;
    valueStr += " mWifiApSsidValid: " + std::to_string(mWifiSupplicantInfo.mWifiApSsidValid);
    valueStr += " mWifiApSsid: ";
    string ssidStr(mWifiSupplicantInfo.mWifiApSsid,
            mWifiSupplicantInfo.mWifiApSsid + sizeof(mWifiSupplicantInfo.mWifiApSsid));
    valueStr += ssidStr;
}

void MccmncDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": " + mMccmnc;
}

void InEmergencyCallDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void PreciseLocationEnabledDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void TrackingStartedDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void NtripStartedDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void LocFeatureStatusDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": ";
    for (int i = 0; i < mInfo.len; i++) {
        valueStr += std::to_string(mInfo.mFids[i]);
        valueStr += " ";
    }
}

void NlpSessionStartedDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

void WwanAppInfoDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += ": mPid: " + std::to_string(mWwanAppInfo.mPid);
    valueStr += " mUid: " + std::to_string(mWwanAppInfo.mUid);
    valueStr += " mAppHasFinePermission: " + std::to_string(mWwanAppInfo.mAppHasFinePermission);
    valueStr += " mAppHasBackgroundPermission: " +
        std::to_string(mWwanAppInfo.mAppHasBackgroundPermission);
    string appHashStr(mWwanAppInfo.mAppHash, mWwanAppInfo.mAppHash + sizeof(mWwanAppInfo.mAppHash));
    valueStr += " mAppHash: " + appHashStr;
    string appPkgStr(mWwanAppInfo.mAppPackageName,
            mWwanAppInfo.mAppPackageName + sizeof(mWwanAppInfo.mAppPackageName));
    valueStr += " mAppPackageName: " + appPkgStr;
    string appCookieStr(mWwanAppInfo.mAppCookie,
            mWwanAppInfo.mAppCookie + sizeof(mWwanAppInfo.mAppCookie));
    valueStr += " mAppCookie: " + appCookieStr;
}

void AssistedGpsDataItem::stringify(string& valueStr) {
    valueStr = mName;
    valueStr += mIsEnabled ? ": true" : ": false";
}

IDataItemCore* DataItemsFactory::createNewDataItem(const DataItemId& id) {
    IDataItemCore *mydi = nullptr;
    switch (id) {
        case NETWORKINFO_DATA_ITEM_ID:
            mydi = new NetworkInfoDataItem();
            break;
        case ENH_DATA_ITEM_ID:
            mydi = new ENHDataItem();
            break;
        case GPSSTATE_DATA_ITEM_ID:
            mydi = new GPSStateDataItem();
            break;
        case WIFIHARDWARESTATE_DATA_ITEM_ID:
            mydi = new WifiHardwareStateDataItem();
            break;
        case TIMEZONE_CHANGE_DATA_ITEM_ID:
            mydi = new TimeZoneChangeDataItem();
            break;
        case TIME_CHANGE_DATA_ITEM_ID:
            mydi = new TimeChangeDataItem();
            break;
        case MODEL_DATA_ITEM_ID:
            mydi = new ModelDataItem();
            break;
        case MANUFACTURER_DATA_ITEM_ID:
            mydi = new ManufacturerDataItem();
            break;
        case RILCELLINFO_DATA_ITEM_ID:
            mydi = new RilCellInfoDataItem();
            break;
        case WIFI_SUPPLICANT_STATUS_DATA_ITEM_ID:
            mydi = new WifiSupplicantStatusDataItem();
            break;
        case MCCMNC_DATA_ITEM_ID:
            mydi = new MccmncDataItem();
            break;
        case IN_EMERGENCY_CALL_DATA_ITEM_ID:
            mydi = new InEmergencyCallDataItem();
            break;
        case PRECISE_LOCATION_ENABLED_DATA_ITEM_ID:
            mydi = new PreciseLocationEnabledDataItem();
            break;
        case TRACKING_STARTED_DATA_ITEM_ID:
            mydi = new TrackingStartedDataItem();
            break;
        case NTRIP_STARTED_DATA_ITEM_ID:
            mydi = new NtripStartedDataItem();
            break;
        case LOC_FEATURE_STATUS_DATA_ITEM_ID:
            mydi = new LocFeatureStatusDataItem();
            break;
        case NETWORK_POSITIONING_STARTED_DATA_ITEM_ID:
            mydi = new NlpSessionStartedDataItem();
            break;
        case WWAN_APP_INFO_DATA_ITEM_ID:
            mydi = new WwanAppInfoDataItem();
            break;
        case ASSISTED_GPS_DATA_ITEM_ID:
            mydi = new AssistedGpsDataItem();
            break;
        default:
            LOC_LOGd("unsupported data item");
            break;
    };
    return mydi;
}

} //namespace loc_core
