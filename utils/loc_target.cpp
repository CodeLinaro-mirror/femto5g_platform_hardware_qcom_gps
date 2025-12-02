/* Copyright (c) 2012-2015, 2021 The Linux Foundation. All rights reserved.
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
 *
 */
/*
Changes from Qualcomm Technologies, Inc. are provided under the following license:
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_utils_target"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <log_util.h>
#include "loc_target.h"
#include "loc_log.h"
#include <loc_pla.h>

/* SOC_ID for various targets can be obtained from:
 * https://ipcatalog.qualcomm.com/socinfo/chipinfo */
#define APQ8064_ID_1 (109)
#define APQ8064_ID_2 (153)
#define MPQ8064_ID_1 (130)
#define MSM8930_ID_1 (142)
#define MSM8930_ID_2 (116)
#define APQ8030_ID_1 (157)
#define APQ8074_ID_1 (184)
#define SG8350P_ID_1 (682)
#define SM8650Q_ID_1 (696)

#define DEFAULT_SOC_ID (-1)

#define LINE_LEN 100
#define STR_LIQUID      "Liquid"
#define STR_SURF        "Surf"
#define STR_MTP         "MTP"
#define STR_APQ         "apq"
#define STR_SDC         "sdc"  // alternative string for APQ targets
#define STR_QCS         "qcs"  // string for Gen9 APQ targets
#define STR_MSM         "msm"
#define STR_SDM         "sdm"  // alternative string for MSM targets
#define STR_APQ_NO_WGR  "baseband_apq_nowgr"
#define STR_AUTO        "auto"
#define IS_STR_END(c) ((c) == '\0' || (c) == '\n' || (c) == '\r')
#define LENGTH(s) (sizeof(s) - 1)
#define GPS_CHECK_NO_ERROR 0
#define GPS_CHECK_NO_GPS_HW 1

#ifdef USE_GLIB

#define SM7750_CQ7790S_SOC_ID (732)

static const int gLEAPQTargetList[] = {SM7750_CQ7790S_SOC_ID};
#endif
static unsigned int gTarget = (unsigned int)-1;

static int read_a_line(const char * file_path, char * line, int line_size)
{
    FILE *fp;
    int result = 0;

    * line = '\0';
    fp = fopen(file_path, "r" );
    if( fp == NULL ) {
        LOC_LOGE("open failed: %s: %s\n", file_path, strerror(errno));
        result = -1;
    } else {
        int len;
        fgets(line, line_size, fp);
        len = strlen(line);
        while ('\n' == line[len-1]) {
            // If there is a new line at end of string, replace it with NULL
            line[len-1] = '\0';
            len--;
        }
        len = len < line_size - 1? len : line_size - 1;
        line[len] = '\0';
        LOC_LOGD("cat %s: %s", file_path, line);
        fclose(fp);
    }
    return result;
}

/*The character array passed to this function should have length
  of atleast PROPERTY_VALUE_MAX*/
void loc_get_target_baseband(char *baseband, int array_length)
{
    if(baseband && (array_length >= PROPERTY_VALUE_MAX)) {
        property_get("ro.baseband", baseband, "");
        LOC_LOGD("%s:%d]: Baseband: %s\n", __func__, __LINE__, baseband);
    }
    else {
        LOC_LOGE("%s:%d]: NULL parameter or array length less than PROPERTY_VALUE_MAX\n",
                 __func__, __LINE__);
    }
}

/*The character array passed to this function should have length
  of atleast PROPERTY_VALUE_MAX*/
void loc_get_auto_platform_name(char *platform_name, int array_length)
{
    if(platform_name && (array_length >= PROPERTY_VALUE_MAX)) {
        property_get("ro.hardware.type", platform_name, "");
        LOC_LOGD("%s:%d]: Autoplatform name: %s\n", __func__, __LINE__, platform_name);
    }
    else {
        LOC_LOGE("%s:%d]: Null parameter or array length less than PROPERTY_VALUE_MAX\n",
                 __func__, __LINE__);
    }
}

/* Reads the soc_id node and return the soc_id value, returns DEFAULT_SOC_ID if soc id cannot be
 * fetched */
int loc_get_device_soc_id()
{
    const char *soc_id_paths[] = {"/sys/devices/soc0/soc_id", "/sys/devices/system/soc/soc0/id"};
    int         file_read_res  = -1;
    int         soc_id         = DEFAULT_SOC_ID;
    char        soc_id_str[LINE_LEN] = {};

    for (size_t i = 0; i < sizeof(soc_id_paths) / sizeof(char *); i++) {
        LOC_LOGd("Trying to read soc_id from path: %s", soc_id_paths[i]);

        //Path should be accessible to other users also, but sepolicy rule would be needed.
        if (!access(soc_id_paths[i], F_OK | R_OK)) {
            file_read_res = read_a_line(soc_id_paths[i], soc_id_str, sizeof(soc_id_str));
            if (0 == file_read_res) {
                LOC_LOGd("soc_id read successful from path: %s", soc_id_paths[i]);
                break;
            }
        } else {
            LOC_LOGe("access failed for path: %s, err: %s\n", soc_id_paths[i], strerror(errno));
        }
    }

    if (0 == file_read_res && soc_id_str[0] != '\0') {
        LOC_LOGd("SOC Id str value: %s\n", soc_id_str);

        char *endptr      = nullptr;
        long  soc_id_long = strtol(soc_id_str, &endptr, 10);

        // Check for conversion errors
        if (endptr == soc_id_str || *endptr != '\0' || soc_id_long <= 0 ||
            soc_id_long > INT32_MAX) {
            LOC_LOGe("Invalid soc_id: %s", soc_id_str);
        } else {
            soc_id = (int)soc_id_long;
            LOC_LOGd("soc_id: %d", soc_id);
        }
    } else {
        LOC_LOGe("Unable to read the soc_id value\n");
    }
    return soc_id;
}

unsigned int loc_get_target(void)
{
    if (gTarget != (unsigned int)-1)
        return gTarget;

    static const char hw_platform[]      = "/sys/devices/soc0/hw_platform";
    static const char hw_platform_dep[]  =
        "/sys/devices/system/soc/soc0/hw_platform";
    static const char mdm[]              = "/target"; // mdm target we are using

    char rd_hw_platform[LINE_LEN];
    char rd_mdm[LINE_LEN];
    char baseband[LINE_LEN];
    char rd_auto_platform[LINE_LEN];

    int soc_id = DEFAULT_SOC_ID;

    loc_get_target_baseband(baseband, sizeof(baseband));

    if (!access(hw_platform, F_OK)) {
        read_a_line(hw_platform, rd_hw_platform, LINE_LEN);
    } else {
        read_a_line(hw_platform_dep, rd_hw_platform, LINE_LEN);
    }
    // Get the soc-id for this device.
    soc_id = loc_get_device_soc_id();

    /*check automotive platform*/
    loc_get_auto_platform_name(rd_auto_platform, sizeof(rd_auto_platform));
    if( !memcmp(rd_auto_platform, STR_AUTO, LENGTH(STR_AUTO)) )
    {
          gTarget = TARGET_AUTO;
          goto detected;
    }

    if( !memcmp(baseband, STR_APQ_NO_WGR, LENGTH(STR_APQ_NO_WGR)) ){

        gTarget = TARGET_NO_GNSS;
        goto detected;
    }

    if( !memcmp(baseband, STR_APQ, LENGTH(STR_APQ)) ||
        !memcmp(baseband, STR_SDC, LENGTH(STR_SDC)) ||
        !memcmp(baseband, STR_QCS, LENGTH(STR_QCS)) ) {

        if (MPQ8064_ID_1 == soc_id)
            gTarget = TARGET_NO_GNSS;
        else
            gTarget = TARGET_APQ_SA;
    } else if (((!memcmp(rd_hw_platform, STR_LIQUID, LENGTH(STR_LIQUID))
                 && IS_STR_END(rd_hw_platform[LENGTH(STR_LIQUID)])) ||
                (!memcmp(rd_hw_platform, STR_SURF,   LENGTH(STR_SURF))
                 && IS_STR_END(rd_hw_platform[LENGTH(STR_SURF)])) ||
                (!memcmp(rd_hw_platform, STR_MTP,   LENGTH(STR_MTP))
                 && IS_STR_END(rd_hw_platform[LENGTH(STR_MTP)]))) &&
               !read_a_line( mdm, rd_mdm, LINE_LEN)) {
        gTarget = TARGET_MDM;
    } else if (MSM8930_ID_1 == soc_id || MSM8930_ID_2 == soc_id) {
        gTarget = TARGET_MSM_NO_SSC;
    } else if ( !memcmp(baseband, STR_MSM, LENGTH(STR_MSM)) ||
                !memcmp(baseband, STR_SDM, LENGTH(STR_SDM)) ) {
        gTarget = TARGET_DEFAULT;
    } else if (SG8350P_ID_1 == soc_id) {
        gTarget = TARGET_NO_GNSS;
    } else if (SM8650Q_ID_1 == soc_id) {
        gTarget = TARGET_NO_GNSS;
    } else {
        gTarget = TARGET_UNKNOWN;
    }

detected:
    LOC_LOGW("HAL: %s returned %d", __FUNCTION__, gTarget);
    return gTarget;
}

#ifdef USE_GLIB
bool isLEAPQTarget(int soc_id) {
    if (soc_id <= 0) {
        return false;
    }
    const size_t targetListSize = sizeof(gLEAPQTargetList) / sizeof(gLEAPQTargetList[0]);
    for (size_t i = 0; i < targetListSize; i++) {
        if (gLEAPQTargetList[i] == soc_id)
            return true;
    }

    //no match found
    return false;
}
#endif
