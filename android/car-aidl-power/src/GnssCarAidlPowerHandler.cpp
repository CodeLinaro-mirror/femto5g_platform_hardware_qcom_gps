/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * Copyright (C) 2023 The Android Open Source Project
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

#include <android/binder_process.h>
#include <android/binder_manager.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/chrono_utils.h>
#include <android-base/logging.h>
#include <android-base/logging.h>
#include <android-base/result.h>
#include <PowerPolicyClientBase.h>

#include <utils/StrongPointer.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <log_util.h>
#include "GnssCarAidlPowerHandler.h"


#include <iostream>
#include <algorithm>
#include <memory>

#include <shared_mutex>
#include <thread>
#include <vector>
#include <LocationAPI.h>

#define LOG_TAG "LocSvc_GnssCARAidlPowerPolicy"

using aidl::android::frameworks::automotive::powerpolicy::CarPowerPolicy;
using aidl::android::frameworks::automotive::powerpolicy::PowerComponent;
using ::android::frameworks::automotive::powerpolicy::hasComponent;
using ::ndk::ScopedAStatus;



namespace gnssAidlCPM {

static bool isInitPowerPolicy = false;

constexpr PowerComponent kLocationComponent = PowerComponent::LOCATION;


 GnssCARAidlPowerHandler::GnssCARAidlPowerHandler() :
       mLocationControlApi(LocationControlAPI::getInstance()) {
   LOC_LOGd("Entry");
}

GnssCARAidlPowerHandler::~GnssCARAidlPowerHandler() {
   LOC_LOGd("Exit");
}

void
GnssCARAidlPowerHandler::onInitFailed() {

   LOC_LOGe("Initializing power policy client failed");
}


std::vector<PowerComponent>
GnssCARAidlPowerHandler::getComponentsOfInterest() {

   std::vector<PowerComponent> components{kLocationComponent};
   return components;
}


ScopedAStatus
GnssCARAidlPowerHandler::onPolicyChanged(const CarPowerPolicy& powerPolicy) {
   LOC_LOGd("onPower Policy");

   if (hasComponent(powerPolicy.enabledComponents, kLocationComponent) ) {
       LOC_LOGd("Resume");
       if (nullptr != mLocationControlApi) {
           mLocationControlApi->powerStateEvent(POWER_STATE_RESUME);
       }
   } else if (hasComponent(powerPolicy.disabledComponents, kLocationComponent)) {
       LOC_LOGd("Suspend");
       if (nullptr != mLocationControlApi) {
           mLocationControlApi->powerStateEvent(POWER_STATE_SUSPEND);
      }
   }
   return ScopedAStatus::ok();
}

};

extern "C" void initGnssAutoPowerHandler(void) {
    LOC_LOGd("Entry");
    // Create and initialize a power policy client.
    if (false == gnssAidlCPM::isInitPowerPolicy) {
        ABinderProcess_setThreadPoolMaxThreadCount(1);
        std::shared_ptr<gnssAidlCPM::GnssCARAidlPowerHandler> powerPolicyClient =
            ndk::SharedRefBase::make<gnssAidlCPM::GnssCARAidlPowerHandler>();
        if(nullptr != powerPolicyClient) {
            powerPolicyClient->init();
        }
        ABinderProcess_joinThreadPool();
        gnssAidlCPM::isInitPowerPolicy = true;
    }
    LOC_LOGd("Exit");
}
