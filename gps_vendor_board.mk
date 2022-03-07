# Flags from BoardConfigVendor.mk
ifneq ($(EXCLUDE_LOCATION_FEATURES),true)
  ifneq ($(TARGET_USES_QMAA),true)
    BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := default
  else ifneq ($(TARGET_USES_QMAA_OVERRIDE_GPS),false)
    BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := default
  endif
endif

