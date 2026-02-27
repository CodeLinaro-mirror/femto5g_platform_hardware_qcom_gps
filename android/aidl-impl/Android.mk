
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss-aidl-impl-qti

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VINTF_FRAGMENTS := android.hardware.gnss-aidl-service-qti.xml

LOCAL_SRC_FILES := \
    Gnss.cpp \
    GnssConfiguration.cpp \
    GnssPowerIndication.cpp \
    GnssMeasurementInterface.cpp \
    GnssBatching.cpp \
    GnssGeofence.cpp \
    AGnss.cpp \
    AGnssRil.cpp \
    GnssDebug.cpp \
    GnssAntennaInfo.cpp \
    MeasurementCorrectionsInterface.cpp \
    GnssVisibilityControl.cpp \
    location_api/GnssAPIClient.cpp \
    location_api/BatchingAPIClient.cpp \
    location_api/GeofenceAPIClient.cpp \
    location_api/LocationUtil.cpp

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/location_api

ifeq ($(TARGET_SUPPORTS_WEAR_OS), true)
    LOCAL_HEADER_LIBRARIES += liblocbatterylistener_headers
    LOCAL_STATIC_LIBRARIES := liblocbatterylistener
    LOCAL_CFLAGS += -DENABLE_NATIVE_BAT_LISTENER
endif

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder_ndk \
    android.hardware.gnss-V7-ndk \
    android.hardware.health-V1-ndk \
    liblog \
    libcutils \
    libutils \
    libloc_core \
    libgps.utils \
    libdl \
    liblocation_api

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss-aidl-service-qti
LOCAL_VINTF_FRAGMENTS := android.hardware.gnss-aidl-service-qti.xml
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.gnss-aidl-service-qti.rc
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libgps.utils \
    liblocation_api \
    libbinder_ndk

LOCAL_SHARED_LIBRARIES += \
    android.hardware.gnss-V7-ndk \
    android.hardware.gnss-aidl-impl-qti

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/fuzzer/Android.mk
