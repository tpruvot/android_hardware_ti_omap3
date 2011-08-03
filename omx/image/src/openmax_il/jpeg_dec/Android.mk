ifeq ($(BUILD_JPEG_DECODER),1)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/OMX_JpegDec_Thread.c \
	src/OMX_JpegDec_Utils.c \
	src/OMX_JpegDecoder.c \

TI_OMX_IMAGE ?= $(TOP)/hardware/ti/omap3/omx/image/src/openmax_il

TI_OMX_COMP_C_INCLUDES ?= \
	$(TOP)/hardware/ti/omap3/omx/system/src/openmax_il/lcml/inc \
	$(TOP)/hardware/ti/omap3/omx/system/src/openmax_il/common/inc \
	$(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/hardware/ti/omap3/dspbridge/libbridge/inc \

LOCAL_C_INCLUDES := \
	$(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_IMAGE)/jpeg_dec/inc \

TI_OMX_COMP_SHARED_LIBRARIES ?= libc libdl liblog

LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES)

	
LOCAL_CFLAGS += $(TI_OMX_CFLAGS) -DOMAP_2430 -DOMX_DEBUG=1

LOCAL_MODULE := libOMX.TI.JPEG.decoder
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)
endif #BUILD_JPEG_DECODER

#########################################################

ifeq ($(BUILD_JPEG_DEC_TEST),1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= tests/JPEGTest.c

TI_OMX_IMAGE ?= $(TOP)/hardware/ti/omap3/omx/image/src/openmax_il

TI_OMX_COMP_C_INCLUDES ?= \
	$(TOP)/hardware/ti/omap3/omx/system/src/openmax_il/lcml/inc \
	$(TOP)/hardware/ti/omap3/omx/system/src/openmax_il/common/inc \
	$(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/hardware/ti/omap3/dspbridge/libbridge/inc \

LOCAL_C_INCLUDES := \
	$(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_IMAGE)/jpeg_dec/inc \

TI_OMX_COMP_SHARED_LIBRARIES ?= libc libdl liblog
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) libOMX.TI.JPEG.decoder

LOCAL_CFLAGS := -Wall -fpic -pipe -O0 -DOMX_DEBUG=1

LOCAL_MODULE:= JpegTestCommon
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
endif