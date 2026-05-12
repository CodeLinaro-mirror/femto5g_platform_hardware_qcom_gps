/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifdef LOC_USE_DLT

#include "log_util.h"
#include <string.h>
#include <mutex>
#include <unordered_map>
#include <pthread.h>

using std::string;
using std::unordered_map;

// Fast lookup: tag/description -> ContextEntry
static unordered_map<string, ContextEntry> locLogTagToDltCtx;
static pthread_rwlock_t rwLock = PTHREAD_RWLOCK_INITIALIZER;

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

ContextEntry LHD_CONTEXTS[] = {
    { ctxHalDaemon,                 "HALD", "LocSvc_HalDaemon" },
    { ctxApiV02,                    "LAV2", "LocSvc_ApiV02" },
    { ctxLocationApiMsg,            "LOCM", "LocSvc_LocationApiMsg" },
    { ctxLocationApiPbMsgConv,      "PBMC","LocSvc_LocationApiPbMsgConv" },
    { ctxLocAdapterBase,            "LADP", "LocSvc_LocAdapterBase" },
    { ctxGnssAdapter,               "GADP", "LocSvc_GnssAdapter" },
    { ctxLocSocket,                 "LSOC", "LocSvc_Qrtr" },
    { ctxGpsUtils,                  "UTIL", "GPS_UTILS" },
    { ctxLogBuffer,                 "LBUF", "LocSvc_LogBuffer" },
    { ctxLocIpc,                    "LIPC", "LocSvc_LocIpc" },
    { ctxMsgTask,                   "MTSK", "LocSvc_MsgTask" },
    { ctxLocContext,                "LCTX", "LocSvc_Ctx" },
    { ctxLocationAPIUtils,          "APIU", "LocSvc_LocationAPIUtils" },
    { ctxDataItems,                 "DIFC", "DataItemsFactoryProxy" },
    { ctxGnssAutoPowerHandler,      "APWR", "LocSvc_GnssAutoPowerHandler" },
    { ctxGnssPowerHandler,          "GPWR", "LocSvc_GnssPowerHandler"},
    { ctxUtilsTarget,               "UTLT", "LocSvc_utils_target" },
    { ctxLocUtilLinkedList,         "LINK", "LocSvc_utils_ll" },
    { ctxLocMiscUtils,              "MISC", "LocSvc_misc_utils" },
    { ctxUtilsMsgQ,                 "MSGQ", "LocSvc_utils_q" },
    { ctxCtxBase,                   "CTXB", "LocSvc_CtxBase" },
    { ctxLocSystemStatusOsObserver, "LSSO", "LocSvc_SystemStatusOsObserver" },
    { ctxLocXSSO,                   "XSSO", "LocSvc_XSSO" },
    { ctxLocNativeAgpsHandler,      "NAGP", "LocSvc_NativeAgpsHandler"},
    { ctxLocApiBase,                "LABS", "LocSvc_LocApiBase" },
    { ctxSystemStatus,              "SYSS", "LocSvc_SystemStatus" },
    { ctxXmlParser,                 "XMLP", "LocSvc_XmlParser" },
    { ctxAgps,                      "AGPS", "LocSvc_Agps" },
    { ctxApiClientBase,             "ACLB", "LocSvc_APIClientBase" },
    { ctxLocationAPI,               "LAPI", "LocSvc_LocationAPI" },
    { ctxDataItemConcreteTypes,     "DICT", "DataItemConcreteTypes" },
    { ctxUtilsCfg,                  "UCFG", "LocSvc_utils_cfg" },
    { ctxNmea,                      "NMEA", "LocSvc_nmea" },
    { ctxXtraSystemStatusObs,       "XTSS", "LocSvc_XtraSystemStatusObs" },
    { ctxEngHubQwesIface,        "EHQW", "EHQWES" },
    { ctxEngHubQDGnssInterface,  "EQDG", "Loc_EngHubQDGnss" },
    { ctxEngHubAggregator,       "EAGG", "EngHub_Aggregator" },
    { ctxEngHubMgr,              "EMGR", "Loc_EngHubMgr" },
    { ctxLocCoreLog,             "LCLO", "LocSvc_core_log" },
    { ctxLocDiagIface,           "LDIF", "LOC_DIAG_IFACE" },
    { ctxLBSApiRpc,              "LBSR", "LocSvc_LBSApiRpc" },
    { ctxLBSAdapterBase,         "LBSA", "LocSvc_LBSAdapterBase" },
    { ctxPassiveLocListner,      "IPLL", "IzatSvc_PassiveLocListener" },
    { ctxIzatUtils,              "ISVU", "IzatSvc_Utils" },
    { ctxLocNetIfaceBase,        "LNIB", "LocSvc_LocNetIfaceBase" },
    { ctxLocNetIfaceHolder,      "LNIH", "LocSvc_LocNetIfaceHolder" },
    { ctxLocationProvider,       "ISLP", "IzatSvc_LocationProvider" },
    { ctxLBSApiBase,             "LAPB", "LocSvc_LBSApiBase" },
    { ctxOSFramework,            "OSFW", "OSFramework" },
    { ctxSubscription,           "SUBS", "Subscription" },
    { ctxLocNetExtLE,            "LNEL", "LocSvc_LocNetExtLE" },
    { ctxIzatZaxis,              "IZAX", "IzatSvc_zaxis" },
    { ctxIzatProvider,           "IZPR", "Izat_Provider" },
    { ctxEHMsgUtils,             "EHMU", "Loc_EHMsgUtils" },
    { ctxIzatZppProxy,           "IZPP", "IzatSvc_ZppProxy" },
    { ctxLocNetIfaceWpaClient,   "WPAC", "LocSvc_LocNetIfaceWpaClient" },
    { ctxIzatRemoteApi,          "IRAP", "IzatRemoteApi" },
    { ctxIPCHandler,             "IPCH", "IPCHandler" },
    { ctxMessageQClient,         "MQCL", "MessageQ_Client" },
    { ctxLocNetIface,            "LNIF", "LocSvc_LocNetIface" },
    { ctxLocNetTelSdkIfaceLe,    "LNTI", "LocNetTelSdkIfaceLe" },
    { ctxSysinfocache,           "SICH", "LocSvc_sysinfocache" },
    { ctxIzatApiBase,            "LIZA", "LocSvc_IzatApiBase" },
    { ctxIzatApiRpc,             "LIRP", "LocSvc_IzatApiRpc" },
    { ctxQCDFWService,           "CDFS", "QCDFW_Service" },
    { ctxQCDFWBinEncoder,        "QCBE", "QCDFW_BinEncoder" },
    { ctxQCDFWBinDecoder,        "QCBD", "QCDFW_BinDecoder" },
    { ctxQCDFWRemote,            "QCFR", "QCDFW_Remote" },
    { ctxQCDFWUtil,              "QCDU", "QCDFW_Util" },
    { ctxQCDFW,                  "CDFW", "QCDFW" },
    { ctxIzatApiV02,             "LIA2", "LocSvc_IzatApiV02" },
};
const size_t LHD_CONTEXTS_COUNT = sizeof(LHD_CONTEXTS) / sizeof(LHD_CONTEXTS[0]);

// XTRA-DAEMON
DLT_DECLARE_CONTEXT(ctxXtraDaemon);
DLT_DECLARE_CONTEXT(ctxPalNetIface);

ContextEntry XD_CONTEXTS[] = {
    {ctxXtraDaemon, "XTRA", "LocSvc_xtra2"},
    {ctxPalNetIface, "PALN", "LocSvc_PalNetIf"},
};

const size_t XD_CONTEXTS_COUNT = sizeof(XD_CONTEXTS) / sizeof(XD_CONTEXTS[0]);

// EDGNSS
DLT_DECLARE_CONTEXT(ctxNtripClient);
DLT_DECLARE_CONTEXT(ctxNtripSource);

ContextEntry EDGNSS_CONTEXTS[] = {
    {ctxNtripClient,   "NTCL", "NTRIP-CLIENT"},
    {ctxNtripSource,   "NSRC", "NtripSource"},
};
const size_t EDGNSS_CONTEXTS_COUNT = sizeof(EDGNSS_CONTEXTS) / sizeof(EDGNSS_CONTEXTS[0]);

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

ContextEntry ENGINE_SERVICE_CONTEXTS[] = {
    {ctxEpMain,                 "MAIN", "EPMAIN"},
    {ctxEnginePlugin,           "EPLG", "ENGINE_PLUGIN"},
    {ctxEpMsgConverter,         "MSGC", "EPMSG-CONV"},
    {ctxEpControll,             "EPCT", "EP_CONTROLL"},
    {ctxEpSimulator,            "ESIM", "EPSIMULATOR"},
    {ctxDreService,             "QDRE", "QFE-Glue"},
    {ctxPpeService,             "QPPE", "QPPE"},
    {ctxPpeCoreService,         "PPEC", "QppeCore"},
    {ctxEpLibApi,               "EAPI", "EP_LIB_API"},
    {ctxEpLibApiImpl,           "APII", "EP_LIB_API_IMPL"},
    {ctxEpDiagLog,              "ELOG", "EP_LOG"},
    {ctxQfeCoreToEp,            "QCEP", "QFE-CORE-EP"},
    {ctxQfeDiag,                "QFED", "QFE-DIAG"},
    {ctxQfeEpToCore,            "QEPC", "QFE-EP-CORE"},
    {ctxQfeCal,                 "QFEC", "QFE-CAL"},
    {ctxQfeSlim,                "QFES", "QFE-SLIM"},
};

const size_t ENGINE_SERVICE_CONTEXTS_COUNT =
        sizeof(ENGINE_SERVICE_CONTEXTS) / sizeof(ENGINE_SERVICE_CONTEXTS[0]);

// Loc-launcher
DLT_DECLARE_CONTEXT(ctxProcessLauncher);

ContextEntry LOC_LAUNCHER_CONTEXTS[] = {
    {ctxProcessLauncher, "LOCL", "LocSvc_launcher"},
};
const size_t LOC_LAUNCHER_CONTEXTS_COUNT =
        sizeof(LOC_LAUNCHER_CONTEXTS) / sizeof(LOC_LAUNCHER_CONTEXTS[0]);

//LCA test app
DLT_DECLARE_CONTEXT(ctxLocationClientApi);
DLT_DECLARE_CONTEXT(ctxLocationIntegrationApi);
DLT_DECLARE_CONTEXT(ctxLocationIntegrationApiImpl);

ContextEntry LCA_CONTEXTS[] = {
    {ctxLocationClientApi, "LCA_", "LocSvc_LocationClientApi"},
    {ctxLocationIntegrationApi, "LIA_", "LocSvc_LocationIntegrationApi"},
    {ctxLocationIntegrationApiImpl, "LIAI", "LocSvc_LocationIntegrationApiImpl"},
};
const size_t LCA_CONTEXTS_COUNT = sizeof(LCA_CONTEXTS) / sizeof(LCA_CONTEXTS[0]);

void registerDltApp(const char* appName, const char* desp) {
    DLT_REGISTER_APP(appName, desp);
}

/**
 * Registers each context from a table if not already present,
 * and inserts the entry into the unordered_map for O(1) lookup.
 * Thread-safe with RW lock (read for presence, write for insert).
 */
void registerDltContexts(ContextEntry *entries, size_t count) {
    if (!entries || count == 0) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        ContextEntry *e = &entries[i];

        // Ensure ctxid is exactly 4 chars + '\0'
        char ctxid4[5];
        memcpy(ctxid4, e->ctxid, 4);
        ctxid4[4] = '\0';

        // Use write lock from the start to avoid race condition
        pthread_rwlock_wrlock(&rwLock);
        auto it = locLogTagToDltCtx.find(e->desc);
        if (it == locLogTagToDltCtx.end()) {
            // Register with DLT
            DLT_REGISTER_CONTEXT(e->ctx, ctxid4, e->desc);

            // Store a copy of the entry
            ContextEntry stored = *e;
            memcpy(stored.ctxid, ctxid4, 5);
            locLogTagToDltCtx.emplace(e->desc, std::move(stored));
        }
        pthread_rwlock_unlock(&rwLock);
    }
}

/**
 * Deregisters each context in the given table and removes
 * the corresponding entries from locLogTagToDltCtx.
 * Thread-safe with write lock for erasure.
 *
 * Call this BEFORE DLT_UNREGISTER_APP().
 */
void deregisterDltContexts(ContextEntry *entries, size_t count) {
    if (!entries || count == 0) return;

    // First unregister the DLT contexts (no lock needed for DLT API)
    for (size_t i = 0; i < count; ++i) {
        DLT_UNREGISTER_CONTEXT(entries[i].ctx);
    }

    // Now remove them from our registry map under write lock
    pthread_rwlock_wrlock(&rwLock);
    for (size_t i = 0; i < count; ++i) {
        locLogTagToDltCtx.erase(entries[i].desc);
    }
    pthread_rwlock_unlock(&rwLock);
}

void deregisterDltApp() {
    DLT_UNREGISTER_APP();
}

// Thread-safe lookup: returns DltContext* for the given tag/desc (LOG_TAG),
// or nullptr if not found. No lazy registration here.
DltContext* getDltContextForTag(const char* tagOrDesc) {
    if (!tagOrDesc || !*tagOrDesc) {
        return nullptr;
    }

    // Acquire read lock to allow concurrent readers
    pthread_rwlock_rdlock(&rwLock);
    auto it = locLogTagToDltCtx.find(tagOrDesc);
    DltContext* ctx = nullptr;
    if (it != locLogTagToDltCtx.end()) {
        // Return pointer to the context which is stable in the map
        // The DltContext itself is not moved/copied once registered
        ctx = &it->second.ctx;
    }
    pthread_rwlock_unlock(&rwLock);
    return ctx;
}

#endif //LOC_USE_DLT
