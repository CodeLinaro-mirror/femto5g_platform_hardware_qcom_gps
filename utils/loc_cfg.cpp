/* Copyright (c) 2011-2015, 2018-2021 The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_utils_cfg"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <grp.h>
#include <errno.h>
#include <loc_cfg.h>
#include <loc_pla.h>
#include <loc_target.h>
#include <loc_misc_utils.h>
#ifdef USE_GLIB
#include <glib.h>
#endif
#include "log_util.h"

/*=============================================================================
 *
 *                          GLOBAL DATA DECLARATION
 *
 *============================================================================*/

/* Parameter data */
static uint32_t DEBUG_LEVEL = UINT32_MAX;
static uint32_t TIMESTAMP = 0;
static bool sVendorEnhanced = true;
static uint32_t sLogBufferEnabled = 0;
static uint32_t sQxdmLogEnabled = 0;

/* Parameter spec table */
static const loc_param_s_type loc_param_table[] =
{
    {"DEBUG_LEVEL",             &DEBUG_LEVEL,        NULL, 'n'},
    {"TIMESTAMP",               &TIMESTAMP,          NULL, 'n'},
    {"LOG_BUFFER_ENABLED",      &sLogBufferEnabled,  NULL, 'n'},
    {"QXDM_LOG",                &sQxdmLogEnabled,    NULL, 'n'},
};
static const int loc_param_num = sizeof(loc_param_table) / sizeof(loc_param_s_type);

typedef struct loc_param_v_type
{
    char* param_name;
    char* param_str_value;
    int param_int_value;
    double param_double_value;
}loc_param_v_type;

typedef bool(*LogGnssF3Init)(void);

// Reference below arrays wherever needed to avoid duplicating
// same conf path string over and again in location code.
const char LOC_PATH_GPS_CONF[] = LOC_PATH_GPS_CONF_STR;
const char LOC_PATH_IZAT_CONF[] = LOC_PATH_IZAT_CONF_STR;
const char LOC_PATH_LOWI_CONF[] = LOC_PATH_LOWI_CONF_STR;
const char LOC_PATH_SAP_CONF[] = LOC_PATH_SAP_CONF_STR;
const char LOC_PATH_APDR_CONF[] = LOC_PATH_APDR_CONF_STR;
const char LOC_PATH_XTWIFI_CONF[] = LOC_PATH_XTWIFI_CONF_STR;
const char LOC_PATH_QUIPC_CONF[] = LOC_PATH_QUIPC_CONF_STR;
const char LOC_PATH_ANT_CORR[] = LOC_PATH_ANT_CORR_STR;
const char LOC_PATH_SLIM_CONF[] = LOC_PATH_SLIM_CONF_STR;
const char LOC_PATH_VPE_CONF[] = LOC_PATH_VPE_CONF_STR;
const char LOC_PATH_QPPE_CONF[] = LOC_PATH_QPPE_CONF_STR;

/*===========================================================================
FUNCTION loc_set_config_entry

DESCRIPTION
   Potentially sets a given configuration table entry based on the passed in
   configuration value. This is done by using a string comparison of the
   parameter names and those found in the configuration file.

PARAMETERS:
   config_entry: configuration entry in the table to possibly set
   config_value: value to store in the entry if the parameter names match

DEPENDENCIES
   N/A

RETURN VALUE
   None

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_set_config_entry(const loc_param_s_type* config_entry,
                        loc_param_v_type* config_value,
                        uint16_t string_len = LOC_MAX_PARAM_STRING)
{
    int ret=-1;
    if(NULL == config_entry || NULL == config_value)
    {
        LOC_LOGE("%s: INVALID config entry or parameter", __FUNCTION__);
        return ret;
    }

    if (strcmp(config_entry->param_name, config_value->param_name) == 0 &&
        config_entry->param_ptr)
    {
        switch (config_entry->param_type)
        {
        case 's':
            if (strcmp(config_value->param_str_value, "NULL") == 0)
            {
                *((char*)config_entry->param_ptr) = '\0';
            }
            else {
                strlcpy((char*) config_entry->param_ptr,
                        config_value->param_str_value,
                        string_len);
            }
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %s", __FUNCTION__,
                     config_entry->param_name, (char*)config_entry->param_ptr);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        case 'n':
            *((int *)config_entry->param_ptr) = config_value->param_int_value;
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %d", __FUNCTION__,
                     config_entry->param_name, config_value->param_int_value);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        case 'f':
            *((double *)config_entry->param_ptr) = config_value->param_double_value;
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %f", __FUNCTION__,
                     config_entry->param_name, config_value->param_double_value);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        default:
            LOC_LOGE("%s: PARAM %s parameter type must be n, f, or s",
                     __FUNCTION__, config_entry->param_name);
        }
    }
    return ret;
}

/*===========================================================================
FUNCTION loc_fill_conf_item

DESCRIPTION
   Takes a line of configuration item and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   input_buf : buffer contanis config item
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   0: Number of records in the config_table filled with input_buf

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_fill_conf_item(char* input_buf,
                       const loc_param_s_type config_table[],
                       uint32_t table_length, uint16_t string_len = LOC_MAX_PARAM_STRING)
{
    int ret = 0;

    if (input_buf && config_table) {
        char *lasts;
        loc_param_v_type config_value;
        memset(&config_value, 0, sizeof(config_value));

        /* Separate variable and value */
        config_value.param_name = strtok_r(input_buf, "=", &lasts);
        /* skip lines that do not contain "=" */
        if (config_value.param_name) {
            config_value.param_str_value = strtok_r(NULL, "\0", &lasts);

            /* skip lines that do not contain two operands */
            if (config_value.param_str_value) {
                /* Trim leading and trailing spaces */
                loc_util_trim_space(config_value.param_name);
                loc_util_trim_space(config_value.param_str_value);

                /* Parse numerical value */
                if ((strlen(config_value.param_str_value) >=3) &&
                    (config_value.param_str_value[0] == '0') &&
                    (tolower(config_value.param_str_value[1]) == 'x'))
                {
                    /* hex */
                    config_value.param_int_value = (int) strtol(&config_value.param_str_value[2],
                                                                (char**) NULL, 16);
                }
                else {
                    config_value.param_double_value = (double) atof(config_value.param_str_value); /* float */
                    config_value.param_int_value = atoi(config_value.param_str_value); /* dec */
                }

                for(uint32_t i = 0; NULL != config_table && i < table_length; i++)
                {
                    if(!loc_set_config_entry(&config_table[i], &config_value, string_len)) {
                        ret += 1;
                    }
                }
            }
        }
    }

    return ret;
}

/*===========================================================================
FUNCTION loc_read_conf_r_long (repetitive)

DESCRIPTION
   Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.
   The difference between this and loc_read_conf is that this function returns
   the file pointer position at the end of filling a config table. Also, it
   reads a fixed number of parameters at a time which is equal to the length
   of the configuration table. This functionality enables the caller to
   repeatedly call the function to read data from the same file.

PARAMETERS:
   conf_fp : file pointer
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   0: Table filled successfully
   1: No more parameters to read
  -1: Error filling table

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_read_conf_r_long(FILE *conf_fp, const loc_param_s_type config_table[],
                         uint32_t table_length, uint16_t string_len)
{
    int ret=0;
    char input_buf[string_len];  /* declare a char array */
    unsigned int num_params=table_length;

    if(conf_fp == NULL) {
        LOC_LOGE("%s:%d]: ERROR: File pointer is NULL\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }

    /* Clear all validity bits */
    for(uint32_t i = 0; NULL != config_table && i < table_length; i++)
    {
        if(NULL != config_table[i].param_set)
        {
            *(config_table[i].param_set) = 0;
        }
    }

    LOC_LOGD("%s:%d]: num_params: %d\n", __func__, __LINE__, num_params);
    while(num_params)
    {
        if(!fgets(input_buf, string_len, conf_fp)) {
            LOC_LOGD("%s:%d]: fgets returned NULL\n", __func__, __LINE__);
            break;
        }

        num_params -= loc_fill_conf_item(input_buf, config_table, table_length, string_len);
    }

err:
    return ret;
}

/*===========================================================================
FUNCTION loc_udpate_conf_long

DESCRIPTION
   Parses the passed in buffer for configuration items, and update the table
   that is also passed in.

Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   conf_data: configuration items in bufferas a string
   length: strlen(conf_data)
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   number of the records in the table that is updated at time of return.

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_update_conf_long(const char* conf_data, int32_t length,
                         const loc_param_s_type config_table[],
                         uint32_t table_length, uint16_t string_len)
{
    int ret = -1;

    if (conf_data && length && config_table && table_length) {
        // make a copy, so we do not tokenize the original data
        char* conf_copy = (char*)malloc(length+1);

        if (conf_copy != NULL)
        {
            memcpy(conf_copy, conf_data, length);
            // we hard NULL the end of string to be safe
            conf_copy[length] = 0;

            // start with one record off
            uint32_t num_params = table_length - 1;
            char* saveptr = NULL;
            char* input_buf = strtok_r(conf_copy, "\n", &saveptr);
            ret = 0;

            LOC_LOGD("%s:%d]: num_params: %d\n", __func__, __LINE__, num_params);
            while(num_params && input_buf) {
                ret++;
                num_params -=
                        loc_fill_conf_item(input_buf, config_table, table_length, string_len);
                input_buf = strtok_r(NULL, "\n", &saveptr);
            }
            free(conf_copy);
        }
    }

    return ret;
}

/*===========================================================================
FUNCTION loc_read_conf_long

DESCRIPTION
   Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   conf_file_name: configuration file to read
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   None

SIDE EFFECTS
   N/A
===========================================================================*/
void loc_read_conf_long(const char* conf_file_name, const loc_param_s_type config_table[],
                        uint32_t table_length, uint16_t string_len)
{
    FILE *conf_fp = NULL;
    QxdmF3 qxdmF3 = NULL;

    if ((conf_fp = fopen(conf_file_name, "r")) != NULL)
    {
        LOC_LOGd("using %s", conf_file_name);
        if(table_length && config_table) {
            loc_read_conf_r_long(conf_fp, config_table, table_length, string_len);
            rewind(conf_fp);
        }
        if (DEBUG_LEVEL == UINT32_MAX) {
            /* Read default config entries*/
            loc_read_conf_r(conf_fp, loc_param_table, loc_param_num);
            if (sQxdmLogEnabled) {
                LogGnssF3Init logGnssF3Init;
                const char* libname = "liblocdiagiface.so";
                void* libHandle = nullptr;
                logGnssF3Init = (LogGnssF3Init)dlGetSymFromLib(libHandle, libname, "LogGnssF3Init");
                qxdmF3 = (QxdmF3)dlGetSymFromLib(libHandle, libname, "LogGnssF3");
                if (nullptr == logGnssF3Init || nullptr == qxdmF3) {
                    ALOGE("DiagIface logGnssF3Init or qxdmF3 is nullptr !!\n");
                } else {
                    if (true != logGnssF3Init()) {
                        ALOGE("logF3Init failed !!\n");
                    }
                }
            }
            /* Initialize logging mechanism with parsed data */
            loc_logger_init(DEBUG_LEVEL, TIMESTAMP, qxdmF3);
            log_buffer_init(sLogBufferEnabled);
            log_tag_level_map_init();
        }
        fclose(conf_fp);
    }
}

/*=============================================================================
 *
 *   Define and Structures for Parsing Location Process Configuration File
 *
 *============================================================================*/
#define MAX_NUM_STRINGS   20

typedef struct {
    char proc_name[LOC_MAX_PARAM_STRING];
    char proc_argument[LOC_MAX_PARAM_STRING];
    char proc_status[LOC_MAX_PARAM_STRING];
    char group_list[LOC_MAX_PARAM_STRING];
    unsigned int launch_trigger_mask;
} loc_launcher_conf;

/* process configuration parameters */
static loc_launcher_conf conf;

/* location feature conf, e.g.: izat.conf feature mode table*/

/* location process conf, e.g.: izat.conf Parameter spec table */
static const loc_param_s_type loc_process_conf_parameter_table[] = {
    {"PROCESS_NAME",               &conf.proc_name,                NULL, 's'},
    {"PROCESS_ARGUMENT",           &conf.proc_argument,            NULL, 's'},
    {"PROCESS_STATE",              &conf.proc_status,              NULL, 's'},
    {"PROCESS_GROUPS",             &conf.group_list,               NULL, 's'},
    {"LAUNCH_TRIGGER_MASK",       &conf.launch_trigger_mask,     NULL, 'n'},
};

/*===========================================================================
FUNCTION loc_read_process_conf

DESCRIPTION
   Parse the specified conf file and return info for the processes defined.
   The format of the file should conform with izat.conf.

PARAMETERS:
   conf_file_name: configuration file to read
   process_count_ptr: pointer to store number of processes defined in the conf file.
   process_info_table_ptr: pointer to store the process info table.

DEPENDENCIES
   The file must be in izat.conf format.

RETURN VALUE
   0: success
   none-zero: failure

SIDE EFFECTS
   N/A

NOTES:
   On success, memory pointed by (*process_info_table_ptr) must be freed.
===========================================================================*/
int loc_read_process_conf(const char* conf_file_name, uint32_t * process_count_ptr,
                          loc_process_info_s_type** process_info_table_ptr) {
    loc_process_info_s_type *child_proc = nullptr;
    volatile int i=0;
    unsigned int j=0;
    char *split_strings[MAX_NUM_STRINGS];
    int name_length=0, group_list_length=0, ngroups=0, ret=0;
    int nstrings=0, status_length=0;
    FILE* conf_fp = nullptr;
    unsigned char proc_list_length=0;
    char arg_launch_trigger_mask[LOC_PROCESS_MAX_ARG_STR_LENGTH] = "--";

    if (process_count_ptr == NULL || process_info_table_ptr == NULL) {
        return -1;
    }

    strlcat(arg_launch_trigger_mask, LOC_FEATURE_LAUNCH_TRIGGER_MASK,
            LOC_PROCESS_MAX_ARG_STR_LENGTH-3);

    if((conf_fp = fopen(conf_file_name, "r")) == NULL) {
        LOC_LOGE("%s:%d]: Error opening %s %s\n", __func__,
                 __LINE__, conf_file_name, strerror(errno));
        ret = -1;
        goto err;
    }

    //Parse through the file to find out how many processes are to be launched
    proc_list_length = 0;
    do {
        conf.proc_name[0] = 0;
        //Here note that the 3rd parameter is passed as 1.
        //This is so that only the first parameter in the table which is "PROCESS_NAME"
        //is read. We do not want to read the entire block of parameters at this time
        //since we are only counting the number of processes to launch.
        //Therefore, only counting the occurrences of PROCESS_NAME parameter
        //should suffice
        if(loc_read_conf_r(conf_fp, loc_process_conf_parameter_table, 1)) {
            LOC_LOGE("%s:%d]: Unable to read conf file. Failing\n", __func__, __LINE__);
            ret = -1;
            goto err;
        }
        name_length=(int)strlen(conf.proc_name);
        if(name_length) {
            proc_list_length++;
            LOC_LOGD("Process name:%s", conf.proc_name);
        }
    } while(name_length);
    LOC_LOGD("Process cnt = %d", proc_list_length);

    child_proc = (loc_process_info_s_type *)calloc(proc_list_length, sizeof(loc_process_info_s_type));
    if(child_proc == NULL) {
        LOC_LOGE("%s:%d]: ERROR: Malloc returned NULL\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }

    //Move file descriptor to the beginning of the file
    //so that the parameters can be read
    rewind(conf_fp);

    for(j=0; j<proc_list_length; j++) {
        //Set defaults for all the child process structs
        child_proc[j].proc_status = DISABLED;
        memset(child_proc[j].group_list, 0, sizeof(child_proc[j].group_list));
        if(loc_read_conf_r(conf_fp, loc_process_conf_parameter_table,
                           sizeof(loc_process_conf_parameter_table)/sizeof(loc_process_conf_parameter_table[0]))) {
            LOC_LOGE("%s:%d]: Unable to read conf file. Failing\n", __func__, __LINE__);
            ret = -1;
            goto err;
        }

        name_length=(int)strlen(conf.proc_name);
        group_list_length=(int)strlen(conf.group_list);
        status_length = (int)strlen(conf.proc_status);

        if(!name_length || !group_list_length) {
            LOC_LOGE("%s:%d]: Error: i: %d; One of the parameters not specified in conf file",
                     __func__, __LINE__, i);
            continue;
        }

        if (strcmp(conf.proc_status, "DISABLED") == 0) {
            LOC_LOGD("%s:%d]: Process %s is disabled in conf file",
                     __func__, __LINE__, conf.proc_name);
            child_proc[j].proc_status = DISABLED;
            continue;
        }
        else if (strcmp(conf.proc_status, "ENABLED") == 0) {
            LOC_LOGD("%s:%d]: Process %s is enabled in conf file",
                     __func__, __LINE__, conf.proc_name);
            child_proc[j].proc_status = ENABLED;
        }

        //Since strlcpy copies length-1 characters, we add 1 to name_length
        if((name_length+1) > LOC_MAX_PARAM_STRING) {
            LOC_LOGE("%s:%d]: i: %d; Length of name parameter too long. Max length: %d",
                     __func__, __LINE__, i, LOC_MAX_PARAM_STRING);
            continue;
        }
        strlcpy(child_proc[j].name[0], conf.proc_name, sizeof (child_proc[j].name[0]));

        child_proc[j].num_groups = 0;
        ngroups = loc_util_split_string(conf.group_list, split_strings, MAX_NUM_STRINGS, ' ');
        for(i=0; i<ngroups; i++) {
            struct group* grp = getgrnam(split_strings[i]);
            if (grp) {
                child_proc[j].group_list[child_proc[j].num_groups] = grp->gr_gid;
                child_proc[j].num_groups++;
                LOC_LOGd("Group %s = %d", split_strings[i], grp->gr_gid);
            }
        }

        if (child_proc[j].proc_status != DISABLED) {

            //Set args
            //The first argument passed through argv is usually the name of the
            //binary when started from commandline.
            //getopt() seems to ignore this first argument and hence we assign it
            //to the process name for consistency with command line args
            i = 0;
            char* temp_arg = ('/' == child_proc[j].name[0][0]) ?
                (strrchr(child_proc[j].name[0], '/') + 1) : child_proc[j].name[0];
            strlcpy (child_proc[j].args[i++], temp_arg, sizeof (child_proc[j].args[0]));

            /*Fill up the remaining arguments from configuration file*/
            LOC_LOGD("%s] Parsing Process_Arguments from Configuration: %s \n",
                      __func__, conf.proc_argument);
            if ('\0' != conf.proc_argument[0])
            {
                /**************************************
                ** conf_proc_argument is shared by all the programs getting launched,
                ** hence copy to process specific argument string and parse the same.
                ***************************************/
                strlcpy(child_proc[j].argumentString, conf.proc_argument,
                        sizeof(child_proc[j].argumentString));
                char *temp_args[LOC_PROCESS_MAX_NUM_ARGS];
                memset (temp_args, 0, sizeof (temp_args));
                loc_util_split_string(child_proc[j].argumentString, &temp_args[i],
                                      (LOC_PROCESS_MAX_NUM_ARGS - i), ' ');
                // copy argument from the pointer to the memory
                for (unsigned int index = i; index < LOC_PROCESS_MAX_NUM_ARGS; index++, i++) {
                    if (temp_args[index] == NULL) {
                        break;
                    }
                    strlcpy(child_proc[j].args[index], temp_args[index],
                            sizeof(child_proc[j].args[index]));
                }
            }
            // disable dynamic launch for AUTO SP
#if defined (USE_GLIB) && !defined (OPENWRT_BUILD)
            conf.launch_trigger_mask = 0;
#endif
            // Send auto shutdown feature status, to mute shutdown timer if auto shutdown
            // feature is disable
            if (conf.launch_trigger_mask) {
                LOC_LOGd("Process %s launch will be delayed.", conf.proc_name);
                child_proc[j].launch_trigger_mask = conf.launch_trigger_mask;
                char launchTriggerMaskBuff[LOC_PROCESS_MAX_ARG_STR_LENGTH];
                snprintf(launchTriggerMaskBuff, LOC_PROCESS_MAX_ARG_STR_LENGTH,
                    "0x%x", conf.launch_trigger_mask);
                strlcpy(child_proc[j].args[i++], arg_launch_trigger_mask,
                        LOC_PROCESS_MAX_ARG_STR_LENGTH);
                strlcpy(child_proc[j].args[i++], launchTriggerMaskBuff,
                        LOC_PROCESS_MAX_ARG_STR_LENGTH);
            }
        }
        else {
            LOC_LOGD("%s:%d]: Process %s is disabled\n",
                     __func__, __LINE__, child_proc[j].name[0]);
        }
    }

err:
    if (conf_fp) {
        fclose(conf_fp);
    }
    if (ret != 0) {
        LOC_LOGE("%s:%d]: ret: %d", __func__, __LINE__, ret);
        if (child_proc) {
            free (child_proc);
            child_proc = nullptr;
        }
        *process_count_ptr = 0;
        *process_info_table_ptr = nullptr;

    }
    else {
        *process_count_ptr = proc_list_length;
        *process_info_table_ptr = child_proc;
    }

    return ret;
}
