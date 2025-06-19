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
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <loc_cfg.h>
#include <loc_pla.h>
#include <loc_misc_utils.h>
#ifdef USE_GLIB
#include <glib.h>
#endif
#include "log_util.h"
#include <vector>

/*=============================================================================
 *
 *                          GLOBAL DATA DECLARATION
 *
 *============================================================================*/
/* Parameter data */
static uint32_t DEBUG_LEVEL = UINT32_MAX;
static uint32_t TIMESTAMP = 0;
static uint32_t sLogBufferEnabled = 0;
static uint32_t sQxdmLogEnabled = 0;

// Each conf item is a name, value pair
struct ConfPair
{
  const char * name;
  char * value;
};

struct ConfFile
{
   const char * filename;
   std::vector<ConfPair> confPairs;
   bool  cached; // indicates conf file is cached or not
};

static std::vector<ConfFile*> confFiles;

/* Parameter spec table */
static loc_param_s_type gps_default_param_table[] =
{
    {"DEBUG_LEVEL",             &DEBUG_LEVEL,        NULL, 'n'},
    {"TIMESTAMP",               &TIMESTAMP,          NULL, 'n'},
    {"LOG_BUFFER_ENABLED",      &sLogBufferEnabled,  NULL, 'n'},
    {"QXDM_LOG",                &sQxdmLogEnabled,    NULL, 'n'},
};

typedef bool(*LogGnssF3Init)(void);

// Reference below arrays wherever needed to avoid duplicating
// same conf path string over and again in location code.
const char LOC_PATH_GPS_CONF[]          = LOC_PATH_GPS_CONF_STR;
const char LOC_PATH_IZAT_PROCESS_CONF[] = LOC_PATH_IZAT_PROCESS_CONF_STR;
const char LOC_PATH_IZAT_CONF[]         = LOC_PATH_IZAT_CONF_STR;
const char LOC_PATH_LOWI_CONF[]         = LOC_PATH_LOWI_CONF_STR;
const char LOC_PATH_SAP_CONF[]          = LOC_PATH_SAP_CONF_STR;
const char LOC_PATH_APDR_CONF[]         = LOC_PATH_APDR_CONF_STR;
const char LOC_PATH_XTWIFI_CONF[]       = LOC_PATH_XTWIFI_CONF_STR;
const char LOC_PATH_ANT_CORR_CONF[]     = LOC_PATH_ANT_CORR_CONF_STR;
const char LOC_PATH_SLIM_CONF[]         = LOC_PATH_SLIM_CONF_STR;
const char LOC_PATH_QPPE_CONF[]         = LOC_PATH_QPPE_CONF_STR;

// trim the line between cursor_begin and cursor_end
void trim(size_t & cursor_begin, size_t & cursor_end, const char * const line)
{
  // check arguments
  if (cursor_end <= cursor_begin) {
    // empty line already
    return;
  }

  // left trim of white space or quote
  for (; cursor_begin < cursor_end; ++cursor_begin) {
    char c = line[cursor_begin];
    if (!isspace(c) && (c != '"')) {
      break;
    }
  }

  // right trim of white space or quote
  for (; cursor_begin < cursor_end; --cursor_end) {
    char c = line[cursor_end - 1];
    if (!isspace(c) && (c != '"')) {
      break;
    }
  }
}

// the copied string include null terminiating byte
char * sub_string_dup(const char * src, const size_t begin, const size_t end)
{
  char * dest = 0;

  size_t length = end - begin;
  if (length) {
     dest = (char*)malloc(length+1);
     if (dest) {
        memcpy(dest, src + begin, length);
        dest[length] = '\0';
     }
  }
  return dest;
}

// return the index of '=' in the line from cursor_begin to cursor_end
int find_equal(size_t & cursor_begin, size_t & cursor_end, const char * const line)
{
  int index = -1;

  if (0 == line) {
    return -1;
  }

  for (size_t i = cursor_begin; i < cursor_end; i++) {
     if ('=' == line[i]) {
        index = i;
        break;
     }
  }
  return index;
}

// this util function will free the memory used for name and value pair
// of each config item in the conf file
void free_conf_pair(ConfPair& pair) {
   if (pair.name) {
      free((void*)pair.name);
      pair.name = 0;
   }
   if (pair.value) {
      free((void*)pair.value);
      pair.value = 0;
   }
}

// this util function will free the memory used to parse the conf file,
// including name and value pair for all config items and file name
// for the conf file
void free_conf_file(ConfFile* confFile) {
   LOC_LOGd("%s, cached %d", confFile->filename, confFile->cached);
   if (confFile && !confFile->cached) {
      for (ConfPair pair : confFile->confPairs) {
         free_conf_pair(pair);
      }
      if (confFile->filename) {
         free((void*) confFile->filename);
         confFile->filename = NULL;
      }
      delete confFile;
   }
}

// return conf file it is cached
ConfFile* get_cache_conf_file(const char* filename) {
   // first check if file is already cached, if so, return the cached copy
   for (ConfFile* file : confFiles) {
      if (strcmp (file->filename, filename) == 0) {
         return file;
      }
   }
   return NULL;
}

// parse config line and save the name, value pair into confPair
bool get_conf_pair_from_line(char* line, ConfPair & pair) {
   size_t cursor_begin = 0;
   size_t cursor_end = strlen(line);
   trim(cursor_begin, cursor_end, line);

   // continue to parse for non-empty and non comment line (starts with #)
   if ((cursor_begin < cursor_end) && ('#' != line[cursor_begin])) {
       int equal_pos = find_equal(cursor_begin, cursor_end, line);
       if (equal_pos > 1) {
          // we shall see name = value pattern
          size_t name_begin = cursor_begin;
          size_t name_end = equal_pos;
          size_t value_begin = equal_pos+1;
          size_t value_end = cursor_end;

          trim(name_begin, name_end, line);
          trim(value_begin, value_end, line);

          pair.name = sub_string_dup(line, name_begin, name_end);
          pair.value = sub_string_dup(line, value_begin, value_end);
      }
   }

   // we allow value pair to be empty for string type, used in process parsing
   if (!pair.name) {
      free_conf_pair(pair);
      return false;
   } else{
      return true;
   }
}

/* read conf file and cache the content if needToCache is true.
   the cached content will be used for subsequent reading from the conf file */
ConfFile* loc_load_conf_file(const char* filename, uint16_t max_line_len,
                             bool needToCache) {
   int      result = 1;
   char     line[max_line_len];
   FILE *   file = 0;
   ConfPair pair = {};
   ConfFile* confFile = NULL;

   do {
      BREAK_IF_ZERO(2, filename);
      LOC_LOGd("load conf file %s, cache %d, line len %d",
               filename, needToCache, max_line_len);

      confFile = get_cache_conf_file(filename);
      BREAK_IF_NON_ZERO(0, confFile);

      file = fopen(filename, "r");
      BREAK_IF_ZERO(3, file);

      confFile = new ConfFile();
      BREAK_IF_ZERO(4, confFile);
      confFile->filename = sub_string_dup(filename, 0, strlen(filename));
      BREAK_IF_ZERO(5, confFile->filename);

      line[0] = '\0';
      while (NULL != fgets(line, sizeof(line), file)) {
         if (strlen(line) == (max_line_len-1)) {
            LOC_LOGe("input file line too long %s, exceed %d", line,
                     max_line_len);
            break;
         }
         pair = {}; // zero initialize
         if (get_conf_pair_from_line(line, pair)) {
            confFile->confPairs.push_back(pair);
            pair.name = 0;
            pair.value = 0;
         }
         // reset the string
         line[0] = '\0';
      }

      // print out conf pairs
      for (ConfPair pair : confFile->confPairs) {
         LOC_LOGa("config pair %s : %s", pair.name, pair.value ? pair.value : "null");
      }

      // cache the conf file if needed
      if (needToCache) {
         confFile->cached = true;
         confFiles.push_back(confFile);
      } else {
         confFile->cached = false;
      }
      result = 0;
   } while (0);

   if (0 != file) {
      (void) fclose(file);
      file = 0;
   }

   if (0 != result) {
      LOC_LOGe("config file %s parse failed, result = %d", filename, result);
      // free the resource on failure
      free_conf_file(confFile);
      confFile = NULL;
   }

   return confFile;
}

// fill up config table entry based on one confiure value/name pair from
// the config file
bool fill_conf_entry(const ConfPair& pair, const loc_param_s_type config_table[],
                     uint32_t table_length, uint16_t string_len) {

   bool entry_filled = false;

   for (uint32_t i = 0; NULL != config_table && i < table_length; i++) {
      const loc_param_s_type* config_entry = &(config_table[i]);
      if (!config_entry->param_name || !config_entry->param_ptr ||
          strcmp(config_entry->param_name, pair.name)) {
         continue;
      }

      switch (config_entry->param_type)
      {
         case 's':
            if (pair.value) {
               strlcpy((char*) config_entry->param_ptr, pair.value, string_len);
            }
            // we allow value to be empty for a string field, e.g.: for process info
            entry_filled = true;
            break;
         case 'n':
            if ((strlen(pair.value) >= 3) && (pair.value[0] == '0') &&
                   (pair.value[1] == 'x' || pair.value[1] == 'X')) {
               // hex
               *((int *)config_entry->param_ptr) = (int) strtol(&pair.value[2], (char**) NULL, 16);
            } else {
                *((int *)config_entry->param_ptr) = atoi(pair.value); /* dec */
            }
            entry_filled = true;
            break;
         case 'f':
             *((double *)config_entry->param_ptr) = (double) atof(pair.value);
             entry_filled = true;
             break;
          default:
              LOC_LOGe("PARAM %s parameter type must be n, f, or s",
                       config_entry->param_name);
             break;
      }
      LOC_LOGe("param %s %s type: %d",
               config_entry->param_name, pair.value, config_entry->param_type);
   }
   return entry_filled;
}

void loc_read_gps_conf_default() {

   LOC_LOGd("reading conf %s, DEBUG LEVEL is %d", LOC_PATH_GPS_CONF, DEBUG_LEVEL);

   // if DEBUG_LEVEL has already been read, then nothing else need to
   // done
   if (DEBUG_LEVEL != UINT32_MAX){
      return;
   }

   uint32_t table_size = sizeof(gps_default_param_table) / sizeof(loc_param_s_type);
   loc_read_conf(LOC_PATH_GPS_CONF, gps_default_param_table, table_size, LOC_MAX_PARAM_LINE);

   QxdmF3 qxdmF3 = nullptr;
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

void loc_read_conf(const char* conf_file_name,
                   const loc_param_s_type config_table[],
                   uint32_t table_length,
                   uint16_t string_len)
{
   ConfFile* confFile = NULL;

   // get the cached conf file or read the conf file and
   // save the name/value pair in confFile
   // false means conf file will not be cached if it has not yet been cached
   LOC_LOGd("reading conf %s, line len %d", conf_file_name, string_len);
   confFile = loc_load_conf_file(conf_file_name, string_len, false);
   if (!confFile) {
      return;
   }

   for (ConfPair pair : confFile->confPairs) {
      fill_conf_entry(pair, config_table, table_length, string_len);
   }

   // if conf file was not already cached, then free the memory
   free_conf_file(confFile);
}

void loc_cache_conf_file(const char* filename, uint16_t max_line_len) {
   (void) loc_load_conf_file(filename, max_line_len, true);
}

int loc_read_conf_r_long(FILE *conf_fp, const loc_param_s_type config_table[],
                         uint32_t table_length, uint16_t string_len)
{
   int result = 0;
   char input_buf[string_len];  /* declare a char array */
   unsigned int filled_param_cnt = 0;

   do {
      BREAK_IF_ZERO(2, conf_fp);
      input_buf[0] = '\0';
      while(filled_param_cnt < table_length) {
         BREAK_IF_ZERO(3, fgets(input_buf, string_len, conf_fp));

         ConfPair pair = {}; // zero initialize
         if (get_conf_pair_from_line(input_buf, pair)) {
            if (fill_conf_entry(pair, config_table, table_length, LOC_MAX_PARAM_STRING)) {
               filled_param_cnt++;
            }
            free_conf_pair(pair);
         }
         input_buf[0] = '\0';
      }
   } while (0);

   LOC_LOGe("loc_read_conf_r_long, table length %d, filled param num %d, result = %d",
            table_length, filled_param_cnt, result);
   if (filled_param_cnt == table_length) {
      result = 0;
   } else {
      result = 1;
   }

   return result;
}
