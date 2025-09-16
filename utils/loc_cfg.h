/* Copyright (c) 2011-2015, 2018 - 2021 The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LOC_CFG_H
#define LOC_CFG_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

#define LOC_MAX_PARAM_NAME     60
#define LOC_MAX_PARAM_STRING   100
#define LOC_MAX_PARAM_LINE    (LOC_MAX_PARAM_NAME + LOC_MAX_PARAM_STRING)

#define UTIL_CACHE_CONF_FILE(filename) \
   loc_cache_conf_file((filename), LOC_MAX_PARAM_LINE)
// Fill default conf entries, e.g.: debug level etc
#define UTIL_READ_CONF_DEFAULT(filename) \
    loc_read_gps_conf_default();

#define UTIL_READ_CONF(filename, config_table) \
    loc_read_conf_long((filename), config_table, \
                        sizeof(config_table) / sizeof(config_table[0]),\
                        LOC_MAX_PARAM_LINE)

#define UTIL_READ_CONF_LONG(filename, config_table, rec_len) \
    loc_read_conf_long((filename), config_table, \
                        sizeof(config_table) / sizeof(config_table[0]), rec_len)

/*=============================================================================
 *
 *                        MODULE TYPE DECLARATION
 *
 *============================================================================*/
typedef struct
{
  const char *param_name;
  void       *param_ptr;   /* for string type, buf size need to be LOC_MAX_PARAM_STRING */
  uint8_t    *param_set;   /* indicate value set by config file */
  char        param_type;  /* 'n' for number,
                              's' for string, NOTE: buf size need to be LOC_MAX_PARAM_STRING
                              'f' for double */
} loc_param_s_type;

#define LOC_PROCESS_MAX_NUM_GROUPS        20
#define LOC_FEATURE_LAUNCH_TRIGGER_MASK   "launch-trigger-mask"

/*=============================================================================
 *
 *                          MODULE EXTERNAL DATA
 *
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *
 *                       MODULE EXPORTED FUNCTIONS
 *
 *============================================================================*/
// Below are the location conf file paths
extern const char LOC_PATH_GPS_CONF[];
extern const char LOC_PATH_IZAT_CONF[];
extern const char LOC_PATH_IZAT_PROCESS_CONF[];
extern const char LOC_PATH_LOWI_CONF[];
extern const char LOC_PATH_SAP_CONF[];
extern const char LOC_PATH_APDR_CONF[];
extern const char LOC_PATH_XTWIFI_CONF[];
extern const char LOC_PATH_ANT_CORR_CONF[];
extern const char LOC_PATH_SLIM_CONF[];
extern const char LOC_PATH_QPPE_CONF[];

// cache the conf file to facilitate subsequent read
void loc_cache_conf_file(const char* file_name, uint16_t max_line_len);
// fill in the default conf entries from gps.conf regarding DEBUG level
void loc_read_gps_conf_default();
// fill up the conf entries from the conf file
void loc_read_conf_long(const char* file_name, const loc_param_s_type config_table[],
                        uint32_t table_length, uint16_t max_line_len);
// used to fill up entries recursively
int loc_read_conf_r_long(FILE *conf_fp, const loc_param_s_type config_table[],
                         uint32_t table_length, uint16_t string_len);

#ifdef __cplusplus
}
#endif

#endif /* LOC_CFG_H */
