/*
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

/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/chrono_utils.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>

#include <utils/SystemClock.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Timers.h>

#include <iostream>
#include <algorithm>
#include <memory>

#include <log_util.h>
#include "GnssCarPowerHandler.h"

#define LOG_TAG "LocSvc_GnssCARPowerPolicy"

namespace aafap = ::aidl::android::frameworks::automotive::powerpolicy;

using aafap::CarPowerPolicy;
using aafap::CarPowerPolicyFilter;
using aafap::ICarPowerPolicyChangeCallback;
using aafap::ICarPowerPolicyServer;
using aafap::PowerComponent;
using android::uptimeMillis;
using android::base::Error;
using android::base::Result;
using ::ndk::ScopedAStatus;
using ::ndk::SpAIBinder;
using ::ndk::SharedRefBase;


namespace gnssCPM {

static bool isInitPowerPolicy = false;
// Car power policy daemon interface name
const char* kPowerPolicyDaemon =
        "android.frameworks.automotive.powerpolicy.ICarPowerPolicyServer/default";


bool GnssCARPowerHandler::hasLocationComponent(
        const std::vector<::aidl::android::frameworks::automotive::powerpolicy::PowerComponent>&
                components) {

     std::vector<PowerComponent>::const_iterator it =
             std::find(components.cbegin(), components.cend(), PowerComponent::LOCATION);
     return (it != components.cend());

}


GnssCARPowerHandler::GnssCARPowerHandler() :
      mLocationControlApi(LocationControlAPI::getInstance()) {
      LOC_LOGv("Entry");
}

::ndk::ScopedAStatus GnssCARPowerHandler::onPolicyChanged(const CarPowerPolicy& powerPolicy) {

    LOC_LOGv("Entry");

    if (hasLocationComponent(powerPolicy.enabledComponents)) {
        // Resume GNSS Session.
         if (NULL == mLocationControlApi) {
             mLocationControlApi = LocationControlAPI::getInstance();
         }
         if (NULL != mLocationControlApi) {
            LOC_LOGv("Resume");
            mLocationControlApi->powerStateEvent(POWER_STATE_RESUME);
         }
         return ::ndk::ScopedAStatus::ok();
    } else if (hasLocationComponent(powerPolicy.disabledComponents)) {
        // Stop GNSS Session.
         if (NULL == mLocationControlApi) {
             mLocationControlApi = LocationControlAPI::getInstance();
         }

         if (NULL != mLocationControlApi) {
            LOC_LOGv("Suspend");
            mLocationControlApi->powerStateEvent(POWER_STATE_SUSPEND);
         }
         return ::ndk::ScopedAStatus::ok();
    }
    return ::ndk::ScopedAStatus::ok();
}


bool GnssCARPowerHandler::initialize() {
    LOC_LOGv("Entry");
    // Get power policy daemon binder.
    ndk::SpAIBinder binder(AServiceManager_getService(kPowerPolicyDaemon));
    if (binder.get() == nullptr) {
        LOC_LOGe("Failed to get car power policy daemon");
        return false;
    }
    std::shared_ptr<ICarPowerPolicyServer> server = ICarPowerPolicyServer::fromBinder(binder);
    if (server == nullptr) {
        LOC_LOGe("Failed to connect to car power policy daemon");
        return false;
    }

    // This class is implementing ICarPowerPolicyChangeCallback and used as client.
    binder = this->asBinder();
    if (binder.get() == nullptr) {
        LOC_LOGe("Failed to get car power policy client binder object");
        return false;
    }
    std::shared_ptr<ICarPowerPolicyChangeCallback> client =
           ICarPowerPolicyChangeCallback::fromBinder(binder);
    if (client == nullptr) {
        LOC_LOGe("Failed to get ICarPowerPolicyChangeCallback from binder");
        return false;
    }
    // Specify components of interest.
    CarPowerPolicyFilter filter;
    filter.components.push_back(PowerComponent::LOCATION);

    // Register the power policy callback to the daemon
    server->registerPowerPolicyChangeCallback(client, filter);
    LOC_LOGi("Successfully registered the client to car power policy daemon");
    return true;
}

};


extern "C" void initGnssAutoPowerHandler(void) {
    LOC_LOGv("Entry");

    if (false == gnssCPM::isInitPowerPolicy) {

        // Setup a binder thread pool for HIDL
       ::android::hardware::configureRpcThreadpool(1, true);

       auto serviceRegistrar = ::android::hardware::LazyServiceRegistrar::getInstance();

         // Setup a binder thread pool to be a power policy client.
       ABinderProcess_setThreadPoolMaxThreadCount(1);
       ABinderProcess_startThreadPool();

        // Create and initialize a power policy client.
       std::shared_ptr<gnssCPM::GnssCARPowerHandler> powerPolicyClient =
            ndk::SharedRefBase::make<gnssCPM::GnssCARPowerHandler>();
       if (!powerPolicyClient->initialize()) {
           LOC_LOGw("Failed to initialize power policy client");
           sleep(2);
           return;
       }

       ::android::hardware::joinRpcThreadpool();
       gnssCPM::isInitPowerPolicy = true;
    }

    LOC_LOGv("Exit");
}
