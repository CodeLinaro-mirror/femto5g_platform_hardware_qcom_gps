LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libgnss_car_powerpolicy
LOCAL_SANITIZE += $(GNSS_SANITIZE)
# activate the following line for debug purposes only, comment out for production
#LOCAL_SANITIZE_DIAG += $(GNSS_SANITIZE_DIAG)
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64

LOCAL_SRC_FILES := \
    src/GnssCarPowerHandler.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    vhal_v2_0_common_headers \
    liblocation_api_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhidlbase \
    libcutils \
    libutils \
    libgps.utils \
    liblocation_api \
    android.hardware.gnss@1.0 \
    libbase \
    libbinder_ndk \
    libbase


ifeq (12,$(filter 12 S ,$(filter-out . ,$(PLATFORM_VERSION))))
    LOCAL_SHARED_LIBRARIES += \
         android.frameworks.automotive.powerpolicy-V1-ndk_platform
else
    LOCAL_SHARED_LIBRARIES += \
        android.frameworks.automotive.powerpolicy-V1-ndk
endif

LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_SHARED_LIBRARY)
