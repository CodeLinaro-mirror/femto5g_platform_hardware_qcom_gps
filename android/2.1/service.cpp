/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#define LOG_TAG "android.hardware.gnss@2.1-service-qti"

#include <android/hardware/gnss/2.1/IGnss.h>
#include <hidl/LegacySupport.h>
#include "loc_cfg.h"
#include "loc_misc_utils.h"
#include <dlfcn.h>

#define CAR_POWER_DAEMON_SUPPORTED

extern "C" {
#include "vndfwk-detect.h"
}

#ifdef ARCH_ARM_32
#define DEFAULT_HW_BINDER_MEM_SIZE 65536
#endif

using android::hardware::gnss::V2_1::IGnss;

using android::hardware::configureRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::joinRpcThreadpool;

using android::status_t;
using android::OK;

typedef int vendorEnhancedServiceMain(int /* argc */, char* /* argv */ []);

#define GNSS_WEAR_POWER_LIBNAME  "libgnsswear_power.so"

#ifdef CAR_POWER_DAEMON_SUPPORTED
#define GNSS_POWER_LIBNAME  "/vendor/lib64/libgnss_car_powerpolicy.so"
#else
#define GNSS_POWER_LIBNAME  "/vendor/lib64/libgnssauto_power.so"
#endif

typedef const void* (*gnssPowerHandler)(void);

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

void initializeGnssPowerHandler() {

    if (0 != initializeGnssAutoPowerHandler()) {
        ALOGW("Gnss Auto Power Handler unavailable.");

        if (0 != initializeGnssWearPowerHandler()) {
            ALOGW("Gnss Wear Power Handler unavailable.");
        }
   }
}

int main() {

    ALOGI("%s", __FUNCTION__);

    int vendorInfo = getVendorEnhancedInfo();
    /* The magic number 2 points to
    #define VND_ENHANCED_SYS_STATUS_BIT 0x02 in vndfwk-detect.c */
    bool vendorEnhanced = ( vendorInfo & 2 );
    setVendorEnhanced(vendorEnhanced);

#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize((size_t)(DEFAULT_HW_BINDER_MEM_SIZE));
#endif
    configureRpcThreadpool(1, true);
    status_t status;

    status = registerPassthroughServiceImplementation<IGnss>();
    if (status == OK) {
        initializeGnssPowerHandler();
        joinRpcThreadpool();
    } else {
        ALOGE("Error while registering IGnss 2.1 service: %d", status);
    }

    return 0;
}
