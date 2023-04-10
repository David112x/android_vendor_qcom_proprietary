# ---------------------------------------------------------------------------------
#                       Make the pp daemon (mm-pp-daemon)
# ---------------------------------------------------------------------------------

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_COPY_HEADERS_TO   := pp/inc
#LOCAL_COPY_HEADERS      := inc/disp_osal.h inc/lib-postproc.h
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := pp/inc
#include $(BUILD_COPY_HEADERS)

