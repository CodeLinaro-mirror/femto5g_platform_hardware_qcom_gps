/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#if defined(LOC_USE_CLOG) || defined(LOC_USE_DLT)

#include "log_util.h"
#include <string.h>
#include <mutex>
#include <unordered_map>
#include <pthread.h>

using std::string;
using std::unordered_map;

#if defined(LOC_USE_DLT) && defined(LOC_USE_CLOG)

#define ADD_CONTEXT(dltContext, contextId, description, clogContextId) \
    { dltContext, contextId, description, clogContextId, nullptr, 0 }

#elif defined(LOC_USE_CLOG)

#define ADD_CONTEXT(dltContext, contextId, description, clogContextId) \
    { contextId, description, clogContextId, nullptr, 0 }

#elif defined(LOC_USE_DLT)

#define ADD_CONTEXT(dltContext, contextId, description, clogContextId) \
    { dltContext, contextId, description }

#else

#define ADD_CONTEXT(dltContext, contextId, description, clogContextId) \
    { contextId, description }

#endif

// Register/Unregister API's
#if defined(LOC_USE_CLOG)

#define LOC_REGISTER_CONTEXT(entry)                                      \
    do {                                                                 \
        (entry)->clogContext = CLOG_REGISTER_CONTEXT(                   \
            (entry)->clogContextId,                                     \
            (entry)->contextId,                                         \
            (entry)->description,                                       \
            CLOG_DIAG_SSID_NOT_AVAILABLE,                               \
            (entry)->description,                                       \
            (entry)->description);                                      \
                                                                         \
        (entry)->clogRegistered =                                       \
            ((entry)->clogContext != nullptr) ? 1 : 0;                  \
    } while (0)

#define LOC_UNREGISTER_CONTEXT(entry)                                   \
    do {                                                                 \
        if ((entry)->clogRegistered != 0 &&                             \
            (entry)->clogContext != nullptr) {                          \
            CLOG_UNREGISTER_CONTEXT((entry)->clogContext);              \
            (entry)->clogContext = nullptr;                             \
            (entry)->clogRegistered = 0;                                \
        }                                                                \
    } while (0)

#elif defined(LOC_USE_DLT)

#define LOC_REGISTER_CONTEXT(entry)                                     \
    do {                                                                \
        DLT_REGISTER_CONTEXT(                                          \
            (entry)->dltContext,                                       \
            (entry)->contextId,                                        \
            (entry)->description);                                     \
    } while (0)

#define LOC_UNREGISTER_CONTEXT(entry)                                  \
    do {                                                                \
        DLT_UNREGISTER_CONTEXT((entry)->dltContext);                   \
    } while (0)

#else

#define LOC_REGISTER_CONTEXT(entry) \
    do {                            \
    } while (0)

#define LOC_UNREGISTER_CONTEXT(entry) \
    do {                              \
    } while (0)

#endif

// Fast lookup: tag/description -> ContextEntry
static pthread_rwlock_t rwLockForContext = PTHREAD_RWLOCK_INITIALIZER;
static std::unordered_map<std::string, ContextEntry> locLogTagToContext;

#ifdef LOC_USE_DLT
// Define DLT contexts used in the table
DLT_DECLARE_CONTEXT(ctxHalDaemon);
DLT_DECLARE_CONTEXT(ctxApiV02);
DLT_DECLARE_CONTEXT(ctxLocationApiMsg);
DLT_DECLARE_CONTEXT(ctxLocationApiPbMsgConv);
DLT_DECLARE_CONTEXT(ctxLocAdapterBase);
DLT_DECLARE_CONTEXT(ctxGnssAdapter);
DLT_DECLARE_CONTEXT(ctxLocSocket);
DLT_DECLARE_CONTEXT(ctxGpsUtils);
DLT_DECLARE_CONTEXT(ctxLogBuffer);
DLT_DECLARE_CONTEXT(ctxLocIpc);
DLT_DECLARE_CONTEXT(ctxMsgTask);
DLT_DECLARE_CONTEXT(ctxLocContext);
DLT_DECLARE_CONTEXT(ctxLocationAPIUtils);
DLT_DECLARE_CONTEXT(ctxDataItems);
DLT_DECLARE_CONTEXT(ctxGnssAutoPowerHandler);
DLT_DECLARE_CONTEXT(ctxGnssPowerHandler);
DLT_DECLARE_CONTEXT(ctxUtilsTarget);
DLT_DECLARE_CONTEXT(ctxLocUtilLinkedList);
DLT_DECLARE_CONTEXT(ctxLocMiscUtils);
DLT_DECLARE_CONTEXT(ctxUtilsMsgQ);
DLT_DECLARE_CONTEXT(ctxCtxBase);
DLT_DECLARE_CONTEXT(ctxLocSystemStatusOsObserver);
DLT_DECLARE_CONTEXT(ctxLocNativeAgpsHandler);
DLT_DECLARE_CONTEXT(ctxLocXSSO);
DLT_DECLARE_CONTEXT(ctxLocApiBase);
DLT_DECLARE_CONTEXT(ctxSystemStatus);
DLT_DECLARE_CONTEXT(ctxXmlParser);
DLT_DECLARE_CONTEXT(ctxAgps);
DLT_DECLARE_CONTEXT(ctxApiClientBase);
DLT_DECLARE_CONTEXT(ctxLocationAPI);
DLT_DECLARE_CONTEXT(ctxDataItemConcreteTypes);
DLT_DECLARE_CONTEXT(ctxUtilsCfg);
DLT_DECLARE_CONTEXT(ctxNmea);
DLT_DECLARE_CONTEXT(ctxXtraSystemStatusObs);
DLT_DECLARE_CONTEXT(ctxEngHubQwesIface);
DLT_DECLARE_CONTEXT(ctxEngHubQDGnssInterface);
DLT_DECLARE_CONTEXT(ctxEngHubAggregator);
DLT_DECLARE_CONTEXT(ctxEngHubMgr);
DLT_DECLARE_CONTEXT(ctxLocCoreLog);
DLT_DECLARE_CONTEXT(ctxLocDiagIface);
DLT_DECLARE_CONTEXT(ctxLBSApiRpc);
DLT_DECLARE_CONTEXT(ctxLBSAdapterBase);
DLT_DECLARE_CONTEXT(ctxPassiveLocListner);
DLT_DECLARE_CONTEXT(ctxIzatUtils);
DLT_DECLARE_CONTEXT(ctxLocNetIfaceBase);
DLT_DECLARE_CONTEXT(ctxLocNetIfaceHolder);
DLT_DECLARE_CONTEXT(ctxLocationProvider);
DLT_DECLARE_CONTEXT(ctxLBSApiBase);
DLT_DECLARE_CONTEXT(ctxOSFramework);
DLT_DECLARE_CONTEXT(ctxSubscription);
DLT_DECLARE_CONTEXT(ctxLocNetExtLE);
DLT_DECLARE_CONTEXT(ctxIzatZaxis);
DLT_DECLARE_CONTEXT(ctxIzatProvider);
DLT_DECLARE_CONTEXT(ctxEHMsgUtils);
DLT_DECLARE_CONTEXT(ctxIzatZppProxy);
DLT_DECLARE_CONTEXT(ctxLocNetIfaceWpaClient);
DLT_DECLARE_CONTEXT(ctxIzatRemoteApi);
DLT_DECLARE_CONTEXT(ctxIPCHandler);
DLT_DECLARE_CONTEXT(ctxMessageQClient);
DLT_DECLARE_CONTEXT(ctxLocNetIface);
DLT_DECLARE_CONTEXT(ctxLocNetTelSdkIfaceLe);
DLT_DECLARE_CONTEXT(ctxSysinfocache);
DLT_DECLARE_CONTEXT(ctxIzatApiBase);
DLT_DECLARE_CONTEXT(ctxIzatApiRpc);
DLT_DECLARE_CONTEXT(ctxQCDFWService);
DLT_DECLARE_CONTEXT(ctxQCDFWBinEncoder);
DLT_DECLARE_CONTEXT(ctxQCDFWBinDecoder);
DLT_DECLARE_CONTEXT(ctxQCDFWRemote);
DLT_DECLARE_CONTEXT(ctxQCDFWUtil);
DLT_DECLARE_CONTEXT(ctxQCDFW);
DLT_DECLARE_CONTEXT(ctxIzatApiV02);


//Engine-Service
DLT_DECLARE_CONTEXT(ctxEpMain);
DLT_DECLARE_CONTEXT(ctxEnginePlugin);
DLT_DECLARE_CONTEXT(ctxEpMsgConverter);
DLT_DECLARE_CONTEXT(ctxEpDiagLog);
DLT_DECLARE_CONTEXT(ctxEpControll);
DLT_DECLARE_CONTEXT(ctxEpSimulator);
DLT_DECLARE_CONTEXT(ctxDreService);
DLT_DECLARE_CONTEXT(ctxPpeService);
DLT_DECLARE_CONTEXT(ctxPpeCoreService);
DLT_DECLARE_CONTEXT(ctxEpLibApi);
DLT_DECLARE_CONTEXT(ctxEpLibApiImpl);
DLT_DECLARE_CONTEXT(ctxQfeCoreToEp);
DLT_DECLARE_CONTEXT(ctxQfeDiag);
DLT_DECLARE_CONTEXT(ctxQfeEpToCore);
DLT_DECLARE_CONTEXT(ctxQfeCal);
DLT_DECLARE_CONTEXT(ctxQfeSlim);

// XTRA-DAEMON
DLT_DECLARE_CONTEXT(ctxXtraDaemon);
DLT_DECLARE_CONTEXT(ctxPalNetIface);

// Loc-launcher
DLT_DECLARE_CONTEXT(ctxProcessLauncher);

// EDGNSS
DLT_DECLARE_CONTEXT(ctxNtripClient);
DLT_DECLARE_CONTEXT(ctxNtripSource);

//LCA test app
DLT_DECLARE_CONTEXT(ctxLocationClientApi);
DLT_DECLARE_CONTEXT(ctxLocationIntegrationApi);
DLT_DECLARE_CONTEXT(ctxLocationIntegrationApiImpl);

// LOCATION-IDL-SERVICE
DLT_DECLARE_CONTEXT(ctxLocationIDLService);
DLT_DECLARE_CONTEXT(ctxLocIDLPowerHandler);
DLT_DECLARE_CONTEXT(ctxLocLcaIDLConverter);

#endif


ContextEntry LHD_CONTEXTS[] = {
    ADD_CONTEXT(ctxHalDaemon,"HALD","LocSvc_HalDaemon",CLOG_ACTX_LHD_HALD),
    ADD_CONTEXT(ctxApiV02,"LAV2","LocSvc_ApiV02",CLOG_ACTX_LHD_LAV2),
    ADD_CONTEXT(ctxLocationApiMsg,"LOCM","LocSvc_LocationApiMsg",CLOG_ACTX_LHD_LOCM),
    ADD_CONTEXT(ctxLocationApiPbMsgConv,"PBMC","LocSvc_LocationApiPbMsgConv",CLOG_ACTX_LHD_PBMC),
    ADD_CONTEXT(ctxLocAdapterBase,"LADP","LocSvc_LocAdapterBase",CLOG_ACTX_LHD_LADP),
    ADD_CONTEXT(ctxGnssAdapter,"GADP","LocSvc_GnssAdapter",CLOG_ACTX_LHD_GADP),
    ADD_CONTEXT(ctxLocSocket,"LSOC","LocSvc_Qrtr",CLOG_ACTX_LHD_LSOC),
    ADD_CONTEXT(ctxGpsUtils,"UTIL","GPS_UTILS",CLOG_ACTX_LHD_UTIL),
    ADD_CONTEXT(ctxLogBuffer,"LBUF","LocSvc_LogBuffer",CLOG_ACTX_LHD_LBUF),
    ADD_CONTEXT(ctxLocIpc,"LIPC","LocSvc_LocIpc",CLOG_ACTX_LHD_LIPC),
    ADD_CONTEXT(ctxMsgTask,"MTSK","LocSvc_MsgTask",CLOG_ACTX_LHD_MTSK),
    ADD_CONTEXT(ctxLocContext,"LCTX","LocSvc_Ctx",CLOG_ACTX_LHD_LCTX),
    ADD_CONTEXT(ctxLocationAPIUtils,"APIU","LocSvc_LocationAPIUtils",CLOG_ACTX_LHD_APIU),
    ADD_CONTEXT(ctxDataItems,"DIFC","DataItemsFactoryProxy",CLOG_ACTX_LHD_DIFC),
    ADD_CONTEXT(ctxGnssAutoPowerHandler,"APWR","LocSvc_GnssAutoPowerHandler",CLOG_ACTX_LHD_APWR),
    ADD_CONTEXT(ctxGnssPowerHandler,"GPWR","LocSvc_GnssPowerHandler",CLOG_ACTX_LHD_GPWR),
    ADD_CONTEXT(ctxUtilsTarget,"UTLT","LocSvc_utils_target",CLOG_ACTX_LHD_UTLT),
    ADD_CONTEXT(ctxLocUtilLinkedList,"LINK","LocSvc_utils_ll",CLOG_ACTX_LHD_LINK),
    ADD_CONTEXT(ctxLocMiscUtils,"MISC","LocSvc_misc_utils",CLOG_ACTX_LHD_MISC),
    ADD_CONTEXT(ctxUtilsMsgQ,"MSGQ","LocSvc_utils_q",CLOG_ACTX_LHD_MSGQ),
    ADD_CONTEXT(ctxCtxBase,"CTXB","LocSvc_CtxBase",CLOG_ACTX_LHD_CTXB),
    ADD_CONTEXT(ctxLocSystemStatusOsObserver,
            "LSSO","LocSvc_SystemStatusOsObserver",CLOG_ACTX_LHD_LSSO),
    ADD_CONTEXT(ctxLocXSSO,"XSSO","LocSvc_XSSO",CLOG_ACTX_LHD_XSSO),
    ADD_CONTEXT(ctxLocNativeAgpsHandler,"NAGP","LocSvc_NativeAgpsHandler",CLOG_ACTX_LHD_NAGP),
    ADD_CONTEXT(ctxLocApiBase,"LABS","LocSvc_LocApiBase",CLOG_ACTX_LHD_LABS),
    ADD_CONTEXT(ctxSystemStatus,"SYSS","LocSvc_SystemStatus",CLOG_ACTX_LHD_SYSS),
    ADD_CONTEXT(ctxXmlParser,"XMLP","LocSvc_XmlParser",CLOG_ACTX_LHD_XMLP),
    ADD_CONTEXT(ctxAgps,"AGPS","LocSvc_Agps",CLOG_ACTX_LHD_AGPS),
    ADD_CONTEXT(ctxApiClientBase,"ACLB","LocSvc_APIClientBase",CLOG_ACTX_LHD_ACLB),
    ADD_CONTEXT(ctxLocationAPI,"LAPI","LocSvc_LocationAPI",CLOG_ACTX_LHD_LAPI),
    ADD_CONTEXT(ctxDataItemConcreteTypes,"DICT","DataItemConcreteTypes",CLOG_ACTX_LHD_DICT),
    ADD_CONTEXT(ctxUtilsCfg,"UCFG","LocSvc_utils_cfg",CLOG_ACTX_LHD_UCFG),
    ADD_CONTEXT(ctxNmea,"NMEA","LocSvc_nmea",CLOG_ACTX_LHD_NMEA),
    ADD_CONTEXT(ctxXtraSystemStatusObs,"XTSS","LocSvc_XtraSystemStatusObs",CLOG_ACTX_LHD_XTSS),
    ADD_CONTEXT(ctxEngHubQwesIface,"EHQW","EHQWES",CLOG_ACTX_LHD_EHQW),
    ADD_CONTEXT(ctxEngHubQDGnssInterface,"EQDG","Loc_EngHubQDGnss",CLOG_ACTX_LHD_EQDG),
    ADD_CONTEXT(ctxEngHubAggregator,"EAGG","EngHub_Aggregator",CLOG_ACTX_LHD_EAGG),
    ADD_CONTEXT(ctxEngHubMgr,"EMGR","Loc_EngHubMgr",CLOG_ACTX_LHD_EMGR),
    ADD_CONTEXT(ctxLocCoreLog,"LCLO","LocSvc_core_log",CLOG_ACTX_LHD_LCLO),
    ADD_CONTEXT(ctxLocDiagIface,"LDIF","LOC_DIAG_IFACE",CLOG_ACTX_LHD_LDIF),
    ADD_CONTEXT(ctxLBSApiRpc,"LBSR","LocSvc_LBSApiRpc",CLOG_ACTX_LHD_LBSR),
    ADD_CONTEXT(ctxLBSAdapterBase,"LBSA","LocSvc_LBSAdapterBase",CLOG_ACTX_LHD_LBSA),
    ADD_CONTEXT(ctxPassiveLocListner,"IPLL","IzatSvc_PassiveLocListener",CLOG_ACTX_LHD_IPLL),
    ADD_CONTEXT(ctxIzatUtils,"ISVU","IzatSvc_Utils",CLOG_ACTX_LHD_ISVU),
    ADD_CONTEXT(ctxLocNetIfaceBase,"LNIB","LocSvc_LocNetIfaceBase",CLOG_ACTX_LHD_LNIB),
    ADD_CONTEXT(ctxLocNetIfaceHolder,"LNIH","LocSvc_LocNetIfaceHolder",CLOG_ACTX_LHD_LNIH),
    ADD_CONTEXT(ctxLocationProvider,"ISLP","IzatSvc_LocationProvider",CLOG_ACTX_LHD_ISLP),
    ADD_CONTEXT(ctxLBSApiBase,"LAPB","LocSvc_LBSApiBase",CLOG_ACTX_LHD_LAPB),
    ADD_CONTEXT(ctxOSFramework,"OSFW","OSFramework",CLOG_ACTX_LHD_OSFW),
    ADD_CONTEXT(ctxSubscription,"SUBS","Subscription",CLOG_ACTX_LHD_SUBS),
    ADD_CONTEXT(ctxLocNetExtLE,"LNEL","LocSvc_LocNetExtLE",CLOG_ACTX_LHD_LNEL),
    ADD_CONTEXT(ctxIzatZaxis,"IZAX","IzatSvc_zaxis",CLOG_ACTX_LHD_IZAX),
    ADD_CONTEXT(ctxIzatProvider,"IZPR","Izat_Provider",CLOG_ACTX_LHD_IZPR),
    ADD_CONTEXT(ctxEHMsgUtils,"EHMU","Loc_EHMsgUtils",CLOG_ACTX_LHD_EHMU),
    ADD_CONTEXT(ctxIzatZppProxy,"IZPP","IzatSvc_ZppProxy",CLOG_ACTX_LHD_IZPP),
    ADD_CONTEXT(ctxLocNetIfaceWpaClient,"WPAC","LocSvc_LocNetIfaceWpaClient",CLOG_ACTX_LHD_WPAC),
    ADD_CONTEXT(ctxIzatRemoteApi,"IRAP","IzatRemoteApi",CLOG_ACTX_LHD_IRAP),
    ADD_CONTEXT(ctxIPCHandler,"IPCH","IPCHandler",CLOG_ACTX_LHD_IPCH),
    ADD_CONTEXT(ctxMessageQClient,"MQCL","MessageQ_Client",CLOG_ACTX_LHD_MQCL),
    ADD_CONTEXT(ctxLocNetIface,"LNIF","LocSvc_LocNetIface",CLOG_ACTX_LHD_LNIF),
    ADD_CONTEXT(ctxLocNetTelSdkIfaceLe,"LNTI","LocNetTelSdkIfaceLe",CLOG_ACTX_LHD_LNTI),
    ADD_CONTEXT(ctxSysinfocache,"SICH","LocSvc_sysinfocache",CLOG_ACTX_LHD_SICH),
    ADD_CONTEXT(ctxIzatApiBase,"LIZA","LocSvc_IzatApiBase",CLOG_ACTX_LHD_LIZA),
    ADD_CONTEXT(ctxIzatApiRpc,"LIRP","LocSvc_IzatApiRpc",CLOG_ACTX_LHD_LIRP),
    ADD_CONTEXT(ctxQCDFWService,"CDFS","QCDFW_Service",CLOG_ACTX_LHD_CDFS),
    ADD_CONTEXT(ctxQCDFWBinEncoder,"QCBE","QCDFW_BinEncoder",CLOG_ACTX_LHD_QCBE),
    ADD_CONTEXT(ctxQCDFWBinDecoder,"QCBD","QCDFW_BinDecoder",CLOG_ACTX_LHD_QCBD),
    ADD_CONTEXT(ctxQCDFWRemote,"QCFR","QCDFW_Remote",CLOG_ACTX_LHD_QCFR),
    ADD_CONTEXT(ctxQCDFWUtil,"QCDU","QCDFW_Util",CLOG_ACTX_LHD_QCDU),
    ADD_CONTEXT(ctxQCDFW,"CDFW","QCDFW",CLOG_ACTX_LHD_CDFW),
    ADD_CONTEXT(ctxIzatApiV02,"LIA2","LocSvc_IzatApiV02",CLOG_ACTX_LHD_LIA2),
};

const size_t LHD_CONTEXTS_COUNT =
        sizeof(LHD_CONTEXTS) / sizeof(LHD_CONTEXTS[0]);



ContextEntry XD_CONTEXTS[] = {
    ADD_CONTEXT(ctxXtraDaemon, "XTRA", "LocSvc_xtra2", CLOG_ACTX_LOC_XTRA),
    ADD_CONTEXT(ctxPalNetIface, "PALN", "LocSvc_PalNetIf", CLOG_ACTX_LOC_PALN),
};

const size_t XD_CONTEXTS_COUNT = sizeof(XD_CONTEXTS) / sizeof(XD_CONTEXTS[0]);

ContextEntry EDGNSS_CONTEXTS[] = {
    ADD_CONTEXT(ctxNtripClient, "NTCL", "NTRIP-CLIENT", CLOG_ACTX_EDGN_NTCL),
    ADD_CONTEXT(ctxNtripSource, "NSRC", "NtripSource",CLOG_ACTX_EDGN_NSRC),
};

const size_t EDGNSS_CONTEXTS_COUNT = sizeof(EDGNSS_CONTEXTS) / sizeof(EDGNSS_CONTEXTS[0]);

ContextEntry ENGINE_SERVICE_CONTEXTS[] = {
    ADD_CONTEXT(ctxEpMain, "MAIN", "EPMAIN", CLOG_ACTX_ENGS_MAIN),
    ADD_CONTEXT(ctxEnginePlugin, "EPLG", "ENGINE_PLUGIN", CLOG_ACTX_ENGS_EPLG),
    ADD_CONTEXT(ctxEpMsgConverter, "MSGC", "EPMSG-CONV", CLOG_ACTX_ENGS_MSGC),
    ADD_CONTEXT(ctxEpControll, "EPCT", "EP_CONTROLL", CLOG_ACTX_ENGS_EPCT),
    ADD_CONTEXT(ctxEpSimulator, "ESIM", "EPSIMULATOR", CLOG_ACTX_ENGS_ESIM),
    ADD_CONTEXT(ctxDreService, "QDRE", "QFE-Glue", CLOG_ACTX_ENGS_QDRE),
    ADD_CONTEXT(ctxPpeService, "QPPE", "QPPE", CLOG_ACTX_ENGS_QPPE),
    ADD_CONTEXT(ctxPpeCoreService, "PPEC", "QppeCore", CLOG_ACTX_ENGS_PPEC),
    ADD_CONTEXT(ctxEpLibApi, "EAPI", "EP_LIB_API", CLOG_ACTX_ENGS_EAPI),
    ADD_CONTEXT(ctxEpLibApiImpl, "APII", "EP_LIB_API_IMPL", CLOG_ACTX_ENGS_APII),
    ADD_CONTEXT(ctxEpDiagLog, "ELOG", "EP_LOG", CLOG_ACTX_ENGS_EPDIAG),
    ADD_CONTEXT(ctxQfeCoreToEp, "QCEP", "QFE-CORE-EP", CLOG_ACTX_ENGS_QCEP),
    ADD_CONTEXT(ctxQfeDiag, "QFED", "QFE-DIAG", CLOG_ACTX_ENGS_QFED),
    ADD_CONTEXT(ctxQfeEpToCore, "QEPC", "QFE-EP-CORE", CLOG_ACTX_ENGS_QEPC),
    ADD_CONTEXT(ctxQfeCal, "QFEC", "QFE-CAL", CLOG_ACTX_ENGS_QFEC),
    ADD_CONTEXT(ctxQfeSlim, "QFES", "QFE-SLIM", CLOG_ACTX_ENGS_QFES),
};

const size_t ENGINE_SERVICE_CONTEXTS_COUNT =
        sizeof(ENGINE_SERVICE_CONTEXTS) / sizeof(ENGINE_SERVICE_CONTEXTS[0]);



ContextEntry LOC_LAUNCHER_CONTEXTS[] = {
    ADD_CONTEXT(ctxProcessLauncher, "LOCL", "LocSvc_launcher", CLOG_ACTX_PROL_LOCL),
};
const size_t LOC_LAUNCHER_CONTEXTS_COUNT =
        sizeof(LOC_LAUNCHER_CONTEXTS) / sizeof(LOC_LAUNCHER_CONTEXTS[0]);

ContextEntry LCA_CONTEXTS[] = {
    ADD_CONTEXT(ctxLocationClientApi, "LCA_", "LocSvc_LocationClientApi",
                      CLOG_ACTX_LCAT_LCA_),
    ADD_CONTEXT(ctxLocationIntegrationApi, "LIA_", "LocSvc_LocationIntegrationApi",
                      CLOG_ACTX_LCAT_LIA_),
    ADD_CONTEXT(ctxLocationIntegrationApiImpl, "LIAI",
                      "LocSvc_LocationIntegrationApiImpl", CLOG_ACTX_LCAT_LIAI),
};
const size_t LCA_CONTEXTS_COUNT = sizeof(LCA_CONTEXTS) / sizeof(LCA_CONTEXTS[0]);

ContextEntry LOC_IDL_SERVICE_CONTEXTS[] = {
    ADD_CONTEXT(ctxLocationIDLService, "IDLS", "LOC_IDL_SERVICE", CLOG_ACTX_LIDL_IDLS),
    ADD_CONTEXT(ctxLocIDLPowerHandler, "IDPH", "IDL_POWER_HANDLER", CLOG_ACTX_LIDL_IDPH),
    ADD_CONTEXT(ctxLocLcaIDLConverter, "LCAC", "LOC_IDL_LCA_CONV", CLOG_ACTX_LIDL_LCAC),
};
const size_t LOC_IDL_SERVICE_CONTEXTS_COUNT =
        sizeof(LOC_IDL_SERVICE_CONTEXTS) / sizeof(LOC_IDL_SERVICE_CONTEXTS[0]);

#if defined(LOC_USE_CLOG)

typedef struct {
    const char* appId;
    clog_app_id_t clogAppId;
} LocClogAppMapEntry;

static const LocClogAppMapEntry locClogAppMap[] = {
    { "LHD",  CLOG_APP_LHD  },
    { "XTRA", CLOG_APP_XTRA },
    { "EDGN", CLOG_APP_EDGN },
    { "LCAT", CLOG_APP_LCAT },
    { "ESDR", CLOG_APP_ESDR },
    { "ESPI", CLOG_APP_ESPI },
    { "ESED", CLOG_APP_ESED },
    { "ESPP", CLOG_APP_ESPP },
    { "LIDL", CLOG_APP_LIDL },
    { "PROL", CLOG_APP_PROL },
};

static bool locGetClogAppId(const char* appId, clog_app_id_t* clogAppId)
{
    if (appId == nullptr || *appId == '\0' || clogAppId == nullptr) {
        return false;
    }

    for (size_t i = 0;
         i < sizeof(locClogAppMap) / sizeof(locClogAppMap[0]);
         ++i) {
        if (strncmp(appId, locClogAppMap[i].appId, 4) == 0) {
            *clogAppId = locClogAppMap[i].clogAppId;
            return true;
        }
    }

    return false;
}
#endif

int locRegisterLogApp(const char* appId, const char* appDescription)
{
    if (appId == nullptr ||
        appDescription == nullptr ||
        *appId == '\0' ||
        *appDescription == '\0') {
        return -1;
    }

#if defined(LOC_USE_CLOG)

    clog_app_id_t clogAppId;

    if (!locGetClogAppId(appId, &clogAppId)) {
        return -1;
    }

    return CLOG_REGISTER_APP(
        clogAppId,
        appId,
        appDescription) == CLOG_RETURN_OK ? 0 : -1;

#elif defined(LOC_USE_DLT)

    DLT_REGISTER_APP(appId, appDescription);
    return 0;

#else

    (void)appId;
    (void)appDescription;
    return 0;

#endif
}

void locRegisterLogContexts(ContextEntry* entries, size_t count)
{
    if ((entries == nullptr) || (count == 0U)) {
        return;
    }

    for (size_t index = 0U; index < count; ++index) {
        ContextEntry* entry = &entries[index];

        pthread_rwlock_wrlock(&rwLockForContext);

        if (locLogTagToContext.find(entry->description) ==
            locLogTagToContext.end()) {
            LOC_REGISTER_CONTEXT(entry);

            ContextEntry storedEntry = *entry;
            locLogTagToContext.emplace(
                entry->description,
                std::move(storedEntry));
        }

        pthread_rwlock_unlock(&rwLockForContext);
    }
}

/**
 * Deregisters each context in the given table and removes
 * the corresponding entries from locLogTagToDltCtx.
 * Thread-safe with write lock for erasure.
 *
 * Call this BEFORE DLT_UNREGISTER_APP().
 */
void locUnregisterLogContexts(ContextEntry* entries, size_t count)
{
    if (!entries || count == 0) {
        return;
    }

    /*
     * Unregister contexts from the active logging backend first.
     * Do not hold the registry lock while calling the backend API.
     */
#if defined(LOC_USE_CLOG)

    for (size_t i = 0; i < count; ++i) {
        ContextEntry* entry = &entries[i];

        if (entry->clogRegistered != 0 &&
            entry->clogContext != nullptr) {
            CLOG_UNREGISTER_CONTEXT(entry->clogContext);

            entry->clogContext = nullptr;
            entry->clogRegistered = 0;
        }
    }

#elif defined(LOC_USE_DLT)

    for (size_t i = 0; i < count; ++i) {
        DLT_UNREGISTER_CONTEXT(entries[i].dltContext);
    }

#else

    /*
     * Neither DLT nor CLOG is enabled.
     * The existing logging framework remains unchanged.
     */
    return;

#endif

    /*
     * Remove the contexts from the common lookup registry.
     */
    pthread_rwlock_wrlock(&rwLockForContext);

    for (size_t i = 0; i < count; ++i) {
        locLogTagToContext.erase(entries[i].description);
    }

    pthread_rwlock_unlock(&rwLockForContext);
}

void locUnregisterLogApp() {

#ifdef LOC_USE_CLOG
    CLOG_UNREGISTER_APP();
#elif defined(LOC_USE_DLT)
    DLT_UNREGISTER_APP();
#endif
}

/**
 * Thread-safe lookup of a logging context by tag or description.
 *
 * Returns:
 *   ContextEntry* if the context is registered
 *   nullptr otherwise
 *
 * The returned pointer is valid until the corresponding context is
 * unregistered. Callers must not retain it after unregistration.
 */
ContextEntry* locGetLogContext(const char* tagOrDesc)
{
    if (!tagOrDesc || !*tagOrDesc) {
        return nullptr;
    }

    pthread_rwlock_rdlock(&rwLockForContext);

    auto contextIterator = locLogTagToContext.find(tagOrDesc);
    ContextEntry* contextEntry = nullptr;

    if (contextIterator != locLogTagToContext.end()) {
        contextEntry = &contextIterator->second;
    }

    pthread_rwlock_unlock(&rwLockForContext);

    return contextEntry;
}

#endif //LOC_USE_DLT
