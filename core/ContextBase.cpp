/* Copyright (c) 2011-2014,2016-2017,2020-2021 The Linux Foundation. All rights reserved.
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
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_CtxBase"

#include <dlfcn.h>
#include <unistd.h>
#include <ContextBase.h>
#include <msg_q.h>
#include <loc_target.h>
#include <loc_pla.h>
#include <loc_log.h>

namespace loc_core {

#define SLL_LOC_API_LIB_NAME "libsynergy_loc_api.so"
#define LOC_APIV2_0_LIB_NAME "libloc_api_v02.so"

loc_gps_cfg_s_type ContextBase::mGps_conf {};
loc_sap_cfg_s_type ContextBase::mSap_conf {};
loc_izat_cfg_s_type ContextBase::mIzat_conf {};
izat_process_info ContextBase:: mIzat_process_conf {};

bool ContextBase::sIsEngineCapabilitiesKnown = false;
bool ContextBase::sGnssMeasurementSupported = false;
uint8_t ContextBase::sFeaturesSupported[MAX_FEATURE_LENGTH];
GnssNMEARptRate ContextBase::sNmeaReportRate = GNSS_NMEA_REPORT_RATE_NHZ;
LocationCapabilitiesMask ContextBase::sQwesFeatureMask = 0;

uint32_t ContextBase::mAntennaInfoVectorSize = 0;

void ContextBase::readConfig()
{
    static bool confReadDone = false;
    LOC_LOGi("confReadDone %d", confReadDone);

    if (confReadDone) {
        return;
    }
    confReadDone = true;

    // default configuration QTI GNSS H/W
    mGps_conf.GNSS_DEPLOYMENT = 0;
    mGps_conf.CAPABILITIES = 0x7;
    // initialize config items from gps.conf
#ifdef FEATURE_AUTOMOTIVE
    mGps_conf.GPS_LOCK = GNSS_CONFIG_GPS_LOCK_MO_AND_NI & (~GNSS_CONFIG_GPS_LOCK_NFW_V2X);
#else
    mGps_conf.GPS_LOCK = GNSS_CONFIG_GPS_LOCK_MO_AND_NI;
#endif

    // By default NMEA Printing is disabled
    mGps_conf.ENABLE_NMEA_PRINT = 0;
    // default NMEA Tag Block Grouping is disabled
    mGps_conf.NMEA_TAG_BLOCK_GROUPING_ENABLED = 0;
    // external DR disabled by default
    mGps_conf.EXTERNAL_DR_ENABLED = 0;
    // Use emergency PDN by default
    mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL = 1;
    // inject supl config to modem with config values from config.xml or gps.conf, default 0
    mGps_conf.AGPS_CONFIG_INJECT = 0;
    // By default no LPPe CP technology is enabled
    mGps_conf.LPPE_CP_TECHNOLOGY = 0;
    // By default no LPPe UP technology is enabled
    mGps_conf.LPPE_UP_TECHNOLOGY = 0;
    mGps_conf.SUPL_VER = 0x00020004;
    mGps_conf.SUPL_MODE = 0x1;
    mGps_conf.SUPL_HOST[0] = 0;
    mGps_conf.SUPL_PORT = 0;
    mGps_conf.MO_SUPL_HOST[0] = 0;
    mGps_conf.MO_SUPL_PORT = 0;
    // LTE Positioning Profile configuration is disable by default
    mGps_conf.LPP_PROFILE = 0;
    // By default no positioning protocol is selected on A-GLONASS system
    mGps_conf.A_GLONASS_POS_PROTOCOL_SELECT = 0;

    char   NMEA_REPORT_RATE[LOC_MAX_PARAM_STRING]; // 1HZ or NHZ
    {
       const loc_param_s_type gps_conf_table[] =
       {
           {"GNSS_DEPLOYMENT",                &mGps_conf.GNSS_DEPLOYMENT,               NULL, 'n'},
           {"CAPABILITIES",                   &mGps_conf.CAPABILITIES,                  NULL, 'n'},
           {"GPS_LOCK",                       &mGps_conf.GPS_LOCK,                      NULL, 'n'},
           {"NMEA_REPORT_RATE",               &NMEA_REPORT_RATE,                        NULL, 's'},
           {"ENABLE_NMEA_PRINT",              &mGps_conf.ENABLE_NMEA_PRINT,             NULL, 'n'},
           {"NMEA_TAG_BLOCK_GROUPING_ENABLED",
                  &mGps_conf.NMEA_TAG_BLOCK_GROUPING_ENABLED,                           NULL, 'n'},
           {"EXTERNAL_DR_ENABLED",            &mGps_conf.EXTERNAL_DR_ENABLED,           NULL, 'n'},
           {"USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL",
                  &mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL,                      NULL, 'n'},
           {"AGPS_CONFIG_INJECT",             &mGps_conf.AGPS_CONFIG_INJECT,            NULL, 'n'},
           {"LPPE_CP_TECHNOLOGY",             &mGps_conf.LPPE_CP_TECHNOLOGY,            NULL, 'n'},
           {"LPPE_UP_TECHNOLOGY",             &mGps_conf.LPPE_UP_TECHNOLOGY,            NULL, 'n'},
           {"SUPL_VER",                       &mGps_conf.SUPL_VER,                      NULL, 'n'},
           {"SUPL_MODE",                      &mGps_conf.SUPL_MODE,                     NULL, 'n'},
           {"SUPL_HOST",                      &mGps_conf.SUPL_HOST,                     NULL, 's'},
           {"SUPL_PORT",                      &mGps_conf.SUPL_PORT,                     NULL, 'n'},
           {"MO_SUPL_HOST",                   &mGps_conf.MO_SUPL_HOST,                  NULL, 's' },
           {"MO_SUPL_PORT",                   &mGps_conf.MO_SUPL_PORT,                  NULL, 'n' },
           {"LPP_PROFILE",                    &mGps_conf.LPP_PROFILE,                   NULL, 'n'},
           {"A_GLONASS_POS_PROTOCOL_SELECT",  &mGps_conf.A_GLONASS_POS_PROTOCOL_SELECT, NULL, 'n'}
       };

       UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_table);
       if (strncmp(NMEA_REPORT_RATE, "1HZ", sizeof(NMEA_REPORT_RATE)) == 0) {
           /* NMEA reporting is configured at 1Hz*/
           sNmeaReportRate = GNSS_NMEA_REPORT_RATE_1HZ;
       } else {
           sNmeaReportRate = GNSS_NMEA_REPORT_RATE_NHZ;
       }

       LOC_LOGi("GNSS Deployment: %s",
                ((mGps_conf.GNSS_DEPLOYMENT == QCSR_SS5_ENABLED) ? "SS5" :
                ((mGps_conf.GNSS_DEPLOYMENT == PDS_API_ENABLED) ? "QFUSION" : "QGNSS")));

       switch (getTargetGnssType(loc_get_target())) {
           case GNSS_GSS:
           case GNSS_AUTO:
               // For APQ targets, MSA/MSB capabilities should be reset
               mGps_conf.CAPABILITIES &= ~(LOC_GPS_CAPABILITY_MSA | LOC_GPS_CAPABILITY_MSB);
               break;
           default:
               break;
       }
    }

    /* initialize config items from sap.conf*/
    mSap_conf.GYRO_BIAS_RANDOM_WALK = 0;
    mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC = 2;
    mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH = 5;
    mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC = 2;
    mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH = 5;
    mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC_HIGH = 4;
    mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH = 25;
    mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC_HIGH = 4;
    mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH = 25;
    mSap_conf.SENSOR_CONTROL_MODE = 0; /* AUTO */
    mSap_conf.SENSOR_ALGORITHM_CONFIG_MASK = 0; /* INS Disabled = FALSE*/
    /* Values MUST be set by OEMs in configuration for sensor-assisted
       navigation to work. There are NO default values */
    mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY = 0;
    mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY = 0;
    mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY = 0;
    mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY = 0;
    mSap_conf.GYRO_BIAS_RANDOM_WALK_VALID = 0;
    mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
    mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
    mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
    mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
    {
       const loc_param_s_type sap_conf_table[] =
       {
         {"GYRO_BIAS_RANDOM_WALK",
               &mSap_conf.GYRO_BIAS_RANDOM_WALK,
               &mSap_conf.GYRO_BIAS_RANDOM_WALK_VALID, 'f'},
         {"ACCEL_RANDOM_WALK_SPECTRAL_DENSITY",
               &mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY,
               &mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
         {"ANGLE_RANDOM_WALK_SPECTRAL_DENSITY",
               &mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY,
               &mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
         {"RATE_RANDOM_WALK_SPECTRAL_DENSITY",
               &mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY,
               &mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
         {"VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY",
               &mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY,
               &mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
         {"SENSOR_ACCEL_BATCHES_PER_SEC",
               &mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC,        NULL, 'n'},
         {"SENSOR_ACCEL_SAMPLES_PER_BATCH",
               &mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH,      NULL, 'n'},
         {"SENSOR_GYRO_BATCHES_PER_SEC",
               &mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC,         NULL, 'n'},
         {"SENSOR_GYRO_SAMPLES_PER_BATCH",
               &mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH,       NULL, 'n'},
         {"SENSOR_ACCEL_BATCHES_PER_SEC_HIGH",
               &mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC_HIGH,   NULL, 'n'},
         {"SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH",
               &mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH, NULL, 'n'},
         {"SENSOR_GYRO_BATCHES_PER_SEC_HIGH",
               &mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC_HIGH,    NULL, 'n'},
         {"SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH",
               &mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH,  NULL, 'n'},
         {"SENSOR_CONTROL_MODE",
               &mSap_conf.SENSOR_CONTROL_MODE,                 NULL, 'n'},
         {"SENSOR_ALGORITHM_CONFIG_MASK",
               &mSap_conf.SENSOR_ALGORITHM_CONFIG_MASK,        NULL, 'n'}
       };

       UTIL_READ_CONF(LOC_PATH_SAP_CONF, sap_conf_table);
    }

    /* initialize config items from izat.conf */
    /* default configuration value of constrained time uncertainty mode:
      feature disabled, time uncertainty threshold defined by modem,
      and unlimited power budget */
#ifdef FEATURE_AUTOMOTIVE
    mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_ENABLED = 1;
#else
    mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_ENABLED = 0;
#endif
    mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_THRESHOLD = 0.0;
    mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_ENERGY_BUDGET = 0;

    /* default configuration value of position assisted clock estimator mode */
    mIzat_conf.POSITION_ASSISTED_CLOCK_ESTIMATOR_ENABLED = 0;

    /* By default we use unknown modem type*/
    mIzat_conf.MODEM_TYPE = 1;
    {
        const loc_param_s_type izat_conf_table[] =
        {
           {"MODEM_TYPE",      &mIzat_conf.MODEM_TYPE,                      NULL, 'n' },
           {"CONSTRAINED_TIME_UNCERTAINTY_ENABLED",
                    &mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_ENABLED,       NULL, 'n'},
           {"CONSTRAINED_TIME_UNCERTAINTY_THRESHOLD",
                    &mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_THRESHOLD,     NULL, 'f'},
           {"CONSTRAINED_TIME_UNCERTAINTY_ENERGY_BUDGET",
                    &mIzat_conf.CONSTRAINED_TIME_UNCERTAINTY_ENERGY_BUDGET, NULL, 'n'},
           {"POSITION_ASSISTED_CLOCK_ESTIMATOR_ENABLED",
                    &mIzat_conf.POSITION_ASSISTED_CLOCK_ESTIMATOR_ENABLED,  NULL, 'n'},
        };
        UTIL_READ_CONF(LOC_PATH_IZAT_CONF, izat_conf_table);
    }

    loc_param_s_type ant_info_vector_table[] =
    {
       { "ANTENNA_INFO_VECTOR_SIZE", &mAntennaInfoVectorSize, NULL, 'n' }
    };
    UTIL_READ_CONF(LOC_PATH_ANT_CORR_CONF, ant_info_vector_table);

    readIZatConfForValueAddedProcess();
}

void ContextBase::readIZatConfForValueAddedProcess() {
   FILE* conf_fp = nullptr;
   /* location process conf, process name and enable state */
   char process_name[LOC_MAX_PARAM_STRING];
   char process_status[LOC_MAX_PARAM_STRING];
   char process_argument[LOC_MAX_PARAM_STRING];

   const loc_param_s_type loc_process_enable_table[] = {
    {"PROCESS_NAME",     &process_name,     NULL, 's'},
    {"PROCESS_STATE",    &process_status,   NULL, 's'},
    {"PROCESS_ARGUMENT", &process_argument, NULL, 's'}
   };

   conf_fp = fopen(LOC_PATH_IZAT_PROCESS_CONF, "r");
   if (!conf_fp) {
      LOC_LOGe("failed to open process conf file: %s", LOC_PATH_IZAT_PROCESS_CONF);
      return;
   }
   uint32_t paramCnt = sizeof (loc_process_enable_table)/ sizeof(loc_param_s_type);
   do {
      process_name[0] = 0;
      if (loc_read_conf_r_long(conf_fp, loc_process_enable_table,
                               paramCnt, LOC_MAX_PARAM_STRING)) {
         break;
      }

      if (strcmp(process_status, "ENABLED") == 0) {
         mIzat_process_conf.valueAddedProcessEnabled = true;

         if (strncmp(process_name, "engine-service",
                     strlen("engine-service")) == 0) {
            mIzat_process_conf.engineServiceEnabled = true;
            // check if this is DRE-INT engine
            if (strncmp(process_argument, "DRE-INT ",
                        sizeof("DRE-INT")) == 0) {
               mIzat_process_conf.engineServiceInfo.dreIntEnabled = true;
            } else if (strncmp(process_argument, "PPE ",
                               sizeof("PPE")) == 0) {
               mIzat_process_conf.engineServiceInfo.ppeEnabled = true;
            } else if (strncmp(process_argument, "PPE-INT ",
                               sizeof("PPE-INT")) == 0) {
               mIzat_process_conf.engineServiceInfo.ppeIntEnabled = true;
               mIzat_process_conf.engineServiceInfo.ppeEnabled = true;
            }
         } else if (strncmp(process_name, "xtwifi-client",
                            strlen("xtwifi-client")) == 0) {
            mIzat_process_conf.gtpDaemonEnabled = true;
         } else if (strncmp(process_name, "slim_daemon",
                            strlen("slim_daemon")) == 0) {
            mIzat_process_conf.slimDaemonEnabled = true;
         } else if (strncmp(process_name, "edgnss-daemon",
                            strlen("edgnss-daemon")) == 0) {
            mIzat_process_conf.eDgnssDaemonEnabled = true;
         }
      }
   } while (1);

   // close the file
   fclose(conf_fp);
   conf_fp = nullptr;

    LOC_LOGd ("value added process enabled %d, gtp enabled %d, slim daemon enabled %d, "
              "edgnss enabled %d, engine service enabled %d (ppe: %d, ppe-int:%d, dre: %d)",
              mIzat_process_conf.valueAddedProcessEnabled, mIzat_process_conf.gtpDaemonEnabled,
              mIzat_process_conf.slimDaemonEnabled, mIzat_process_conf.eDgnssDaemonEnabled,
              mIzat_process_conf.engineServiceEnabled,
              mIzat_process_conf.engineServiceInfo.ppeEnabled,
              mIzat_process_conf.engineServiceInfo.ppeIntEnabled,
              mIzat_process_conf.engineServiceInfo.dreIntEnabled);
}

uint32_t ContextBase::getCarrierCapabilities() {
    #define carrierMSA (uint32_t)0x2
    #define carrierMSB (uint32_t)0x1
    #define gpsConfMSA (uint32_t)0x4
    #define gpsConfMSB (uint32_t)0x2
    uint32_t capabilities = mGps_conf.CAPABILITIES;
    if ((mGps_conf.SUPL_MODE & carrierMSA) != carrierMSA) {
        capabilities &= ~gpsConfMSA;
    }
    if ((mGps_conf.SUPL_MODE & carrierMSB) != carrierMSB) {
        capabilities &= ~gpsConfMSB;
    }

    LOC_LOGV("getCarrierCapabilities: CAPABILITIES %x, SUPL_MODE %x, carrier capabilities %x",
             mGps_conf.CAPABILITIES, mGps_conf.SUPL_MODE, capabilities);
    return capabilities;
}

LBSProxyBase* ContextBase::getLBSProxy(const char* libName)
{
    LBSProxyBase* proxy = NULL;
    LOC_LOGD("%s:%d]: getLBSProxy libname: %s\n", __func__, __LINE__, libName);
    void* lib = dlopen(libName, RTLD_NOW);

    if ((void*)NULL != lib) {
        getLBSProxy_t* getter = (getLBSProxy_t*)dlsym(lib, "getLBSProxy");
        if (NULL != getter) {
            proxy = (*getter)();
        }
    }
    else
    {
        LOC_LOGW("%s:%d]: FAILED TO LOAD libname: %s\n", __func__, __LINE__, libName);
    }
    if (NULL == proxy) {
        proxy = new LBSProxyBase();
    }
    LOC_LOGD("%s:%d]: Exiting\n", __func__, __LINE__);
    return proxy;
}

LocApiBase* ContextBase::createLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask)
{
    LocApiBase* locApi = NULL;
    const char* libname = LOC_APIV2_0_LIB_NAME;

    // Check the target
    if (TARGET_NO_GNSS != loc_get_target()){

        if (NULL == (locApi = mLBSProxy->getLocApi(exMask, this))) {
            void *handle = NULL;

            if (QCSR_SS5_ENABLED == mGps_conf.GNSS_DEPLOYMENT) {
                libname = SLL_LOC_API_LIB_NAME;
            }

            if ((handle = dlopen(libname, RTLD_NOW)) != NULL) {
                LOC_LOGD("%s:%d]: %s is present", __func__, __LINE__, libname);
                getLocApi_t* getter = (getLocApi_t*) dlsym(handle, "getLocApi");
                if (getter != NULL) {
                    LOC_LOGD("%s:%d]: getter is not NULL of %s", __func__,
                            __LINE__, libname);
                    locApi = (*getter)(exMask, this);
                }
            }
        }
    }

    // locApi could still be NULL at this time
    // we would then create a dummy one
    if (NULL == locApi) {
        locApi = new LocApiBase(exMask, this);
    }

    return locApi;
}

ContextBase::ContextBase(const MsgTask* msgTask,
                         LOC_API_ADAPTER_EVENT_MASK_T exMask,
                         const char* libName) :
    mLBSProxy(getLBSProxy(libName)),
    mMsgTask(msgTask),
    mLocApi(createLocApi(exMask)),
    mLocApiProxy(mLocApi->getLocApiProxy())
{
}

ContextBase::~ContextBase() {
    if (nullptr != mLocApi) {
        mLocApi->destroy();
        mLocApi = nullptr;
    }
    if (nullptr != mLBSProxy) {
        delete mLBSProxy;
        mLBSProxy = nullptr;
    }
}

void ContextBase::setEngineCapabilities(uint8_t *featureList, bool gnssMeasurementSupported) {

    if (ContextBase::sIsEngineCapabilitiesKnown == false) {
        ContextBase::sGnssMeasurementSupported = gnssMeasurementSupported;
        if (featureList != NULL) {
            memcpy((void *)ContextBase::sFeaturesSupported,
                    (void *)featureList, sizeof(ContextBase::sFeaturesSupported));
        }
        mGps_conf.AGPS_CONFIG_INJECT &=
                !(isFeatureSupported(LOC_SUPPORTED_FEATURE_DSDA_CONFIGURATION));

        /* */
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_MEASUREMENTS_CORRECTION)) {
            static uint8_t isSapModeKnown = 0;

            if (!isSapModeKnown) {
                /* Check if SAP is PREMIUM_ENV_AIDING in izat.conf */
                char conf_feature_sap[LOC_MAX_PARAM_STRING];
                loc_param_s_type izat_conf_feature_table[] =
                {
                    { "SAP",           &conf_feature_sap,           &isSapModeKnown, 's' }
                };
                UTIL_READ_CONF(LOC_PATH_IZAT_CONF, izat_conf_feature_table);

                /* Disable this feature if SAP is not PREMIUM_ENV_AIDING in izat.conf */
                if (strcmp(conf_feature_sap, "PREMIUM_ENV_AIDING") != 0) {
                    uint8_t arrayIndex = LOC_SUPPORTED_FEATURE_MEASUREMENTS_CORRECTION >> 3;
                    uint8_t bitPos = LOC_SUPPORTED_FEATURE_MEASUREMENTS_CORRECTION & 7;

                    if (arrayIndex < MAX_FEATURE_LENGTH) {
                        /* To disable the feature we need to reset the bit on the "bitPos"
                           position, so shift a "1" to the left by "bitPos" */
                        ContextBase::sFeaturesSupported[arrayIndex] &= ~(1 << bitPos);
                    }
                }
            }
        }
        ContextBase::sIsEngineCapabilitiesKnown = true;
    }
}


bool ContextBase::isFeatureSupported(uint8_t featureVal)
{
    uint8_t arrayIndex = featureVal >> 3;
    uint8_t bitPos = featureVal & 7;

    if (arrayIndex >= MAX_FEATURE_LENGTH) return false;
    return ((ContextBase::sFeaturesSupported[arrayIndex] >> bitPos ) & 0x1);
}

bool ContextBase::gnssConstellationConfig() {
    return sGnssMeasurementSupported;
}

void ContextBase::setQwesFeatureStatus(
        const std::unordered_map<LocationQwesFeatureType, bool> &featureMap) {
    std::unordered_map<LocationQwesFeatureType, bool>::const_iterator itr;
    static LocationQwesFeatureType locQwesFeatType[LOCATION_QWES_FEATURE_TYPE_MAX];
    for (itr = featureMap.begin(); itr != featureMap.end(); ++itr) {
        LOC_LOGi("Feature : %d isValid: %d", itr->first, itr->second);
        locQwesFeatType[itr->first] = itr->second;
        switch (itr->first) {
            case LOCATION_QWES_FEATURE_TYPE_CARRIER_PHASE:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_CARRIER_PHASE;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_CARRIER_PHASE;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_SV_POLYNOMIAL:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_GNSS_SINGLE_FREQUENCY:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_GNSS_SINGLE_FREQUENCY;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_GNSS_SINGLE_FREQUENCY;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_SV_EPH:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_SV_EPHEMERIS;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_GNSS_MULTI_FREQUENCY:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_GNSS_MULTI_FREQUENCY;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_GNSS_MULTI_FREQUENCY;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_PPE:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_PPE;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_PPE;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_QDR2:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_QDR2;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_QDR2;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_QDR3:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_QDR3;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_QDR3;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_VPE:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_VPE;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_VPE;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_DGNSS:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_DGNSS;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_DGNSS;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_WIFI_STANDARD_POSITIONING:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_WIFI_STANDARD_POSITIONING;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_WIFI_STANDARD_POSITIONING;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_WIFI_PREMIUM_POSITIONING:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_WIFI_PREMIUM_POSITIONING;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_WIFI_PREMIUM_POSITIONING;
                }
                break;
            case LOCATION_QWES_FEATURE_NLOS_ML20:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_NLOS_ML20;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_NLOS_ML20;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_WWAN_STANDARD_POSITIONING:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_WWAN_STANDARD_POSITIONING;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_WWAN_STANDARD_POSITIONING;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_WWAN_PREMIUM_POSITIONING:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_WWAN_PREMIUM_POSITIONING;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_WWAN_PREMIUM_POSITIONING;
                }
                break;
            case LOCATION_QWES_FEATURE_STATUS_GNSS_NHZ:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_GNSS_NHZ;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_GNSS_NHZ;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_WOCS:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_WOCS;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_WOCS;
                }
                break;
            case LOCATION_QWES_FEATURE_TYPE_SBAS:
                if (itr->second) {
                    sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_SBAS;
                } else {
                    sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_SBAS;
                }
                break;
        }
    }

    // Set CV2X basic when time freq and tunc is set
    // CV2X_BASIC  = LOCATION_QWES_FEATURE_TYPE_TIME_FREQUENCY &
    //       LOCATION_QWES_FEATURE_TYPE_TIME_UNCERTAINTY

    // Set CV2X premium when time freq and tunc is set
    // CV2X_PREMIUM = CV2X_BASIC & LOCATION_QWES_FEATURE_TYPE_QDR3 &
    //       LOCATION_QWES_FEATURE_TYPE_CLOCK_ESTIMATE

    bool cv2xBasicEnabled = (1 == locQwesFeatType[LOCATION_QWES_FEATURE_TYPE_TIME_FREQUENCY]) &&
        (1 == locQwesFeatType[LOCATION_QWES_FEATURE_TYPE_TIME_UNCERTAINTY]);
    bool cv2xPremiumEnabled = cv2xBasicEnabled &&
        (1 == locQwesFeatType[LOCATION_QWES_FEATURE_TYPE_QDR3]) &&
        (1 == locQwesFeatType[LOCATION_QWES_FEATURE_TYPE_CLOCK_ESTIMATE]);

    LOC_LOGd("CV2X_BASIC:%d, CV2X_PREMIUM:%d", cv2xBasicEnabled, cv2xPremiumEnabled);
    if (cv2xBasicEnabled) {
        sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_BASIC;
    } else {
        sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_BASIC;
    }
    if (cv2xPremiumEnabled) {
        sQwesFeatureMask |= LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_PREMIUM;
    } else {
        sQwesFeatureMask &= ~LOCATION_CAPABILITIES_QWES_CV2X_LOCATION_PREMIUM;
    }
}

}
