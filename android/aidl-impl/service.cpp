/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2_0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2_0
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

#include <unistd.h>
#include <aidl/android/hardware/gnss/IGnss.h>
#include "loc_cfg.h"
#include "loc_misc_utils.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include "Gnss.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "android.hardware.gnss-aidl-impl-qti"

#ifdef ARCH_ARM_32
#define DEFAULT_HW_BINDER_MEM_SIZE 65536
#endif

using ::android::sp;

typedef int vendorEnhancedServiceMain(int /* argc */, char* /* argv */ []);
typedef void createQesdkHandle();

using GnssAidl = ::android::hardware::gnss::aidl::implementation::Gnss;

static void sleepIfInShutdown() {
    char shutdownProp[PROPERTY_VALUE_MAX] = {};
    const char propName[] = "sys.shutdown.requested";
    const char propDefault[] = "N/A";
    property_get(propName, shutdownProp, propDefault);
    if (strncmp(shutdownProp, propDefault, sizeof(propDefault)-1) != 0) {
        ALOGW("%s, %s was set %s, SLEEP!!!", __FUNCTION__, propName, shutdownProp);
        sleep(UINT_MAX);
    }
}

#define GNSS_AUTO_POWER_LIBNAME   "libgnssauto_power.so"
#define GNSS_WEAR_POWER_LIBNAME   "libgnsswear_power.so"
#define GNSS_WEAR_SMC_HAL_LIBNAME "libgnsswear_smc.so"

typedef void (*gnssPowerHandler)(void);
typedef void (*gnssSmcHandler)(void);

int initializeGnssAutoPowerHandler() {

    void * handle = nullptr;
    gnssPowerHandler getter = (gnssPowerHandler) dlGetSymFromLib(handle, GNSS_AUTO_POWER_LIBNAME,
                                                                 "initGnssAutoPowerHandler");
    if (nullptr != getter) {
        getter();
        ALOGI("GnssAutoPowerHandler Initialized!");
        return 0;
    }
    return -1;
}

int initializeGnssWearPowerHandler() {

    void * handle = nullptr;
    gnssPowerHandler getter = (gnssPowerHandler) dlGetSymFromLib(handle, GNSS_WEAR_POWER_LIBNAME,
                                                                 "initGnssWearPowerHandler");
    if (nullptr != getter) {
        getter();
        ALOGI("GnssWearPowerHandler Initialized!");
        return 0;
    }
    return -1;
}

void initializeGnssWearSmcHalHandler() {

    void * handle = nullptr;
    gnssSmcHandler getter = (gnssSmcHandler) dlGetSymFromLib(handle, GNSS_WEAR_SMC_HAL_LIBNAME,
                                                                 "initGnssSmcHalHandler");
    if (nullptr != getter) {
        getter();
        ALOGI("initializeGnssWearSmcHalHandler Initialized!");
    } else {
        ALOGW("Gnss Wear Smc Hal Handler unavailable.");
    }
}

void initializeGnssPowerHandler() {

    if (0 != initializeGnssAutoPowerHandler()) {
        ALOGW("Gnss Auto Power Handler unavailable.");

        if (0 != initializeGnssWearPowerHandler()) {
            ALOGW("Gnss Wear Power Handler unavailable.");
        }
    }
}

int main() {
    // read a copy of gps conf and izat.conf and cache it for future use
    UTIL_CACHE_CONF_FILE(LOC_PATH_GPS_CONF);
    UTIL_CACHE_CONF_FILE(LOC_PATH_IZAT_CONF);
    UTIL_READ_CONF_DEFAULT(LOC_PATH_GPS_CONF);

    ALOGI("%s, start Gnss HAL process", __FUNCTION__);
    sleepIfInShutdown();
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<GnssAidl> gnssAidl = ndk::SharedRefBase::make<GnssAidl>();
    const std::string instance = std::string() + GnssAidl::descriptor + "/default";
    if (gnssAidl != nullptr) {
        binder_status_t status =
            AServiceManager_addService(gnssAidl->asBinder().get(), instance.c_str());
        if (STATUS_OK == status) {
            ALOGD("register IGnss AIDL service success");
        } else {
            ALOGE("Error while register IGnss AIDL service, status: %d", status);
        }
    }

    // Loc AIDL service
#define VENDOR_AIDL_LIB "vendor.qti.gnss-service.so"
#define QESDK_SERVICE_LIB "liblocation_qesdk.so"
    void* libQesdkHandle = NULL;
    createQesdkHandle* qesdkMainMethod = (createQesdkHandle*)
        dlGetSymFromLib(libQesdkHandle, QESDK_SERVICE_LIB, "createLocationQesdk");
    if (NULL != qesdkMainMethod) {
        ALOGI("start Location QESDK service");
        (*qesdkMainMethod)();
    }

    void* libAidlHandle = NULL;
    vendorEnhancedServiceMain* aidlMainMethod = (vendorEnhancedServiceMain*)
        dlGetSymFromLib(libAidlHandle, VENDOR_AIDL_LIB, "main");
    if (NULL != aidlMainMethod) {
        ALOGI("start LocAidl service");
        (*aidlMainMethod)(0, NULL);
    }
    // Load gnss power handler
    initializeGnssPowerHandler();
    //Load gnss wear smc handler
    initializeGnssWearSmcHalHandler();
    // Loc AIDL service end
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE;  // should not reach
}
