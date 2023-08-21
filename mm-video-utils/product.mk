# Video proprietary packages

MM_VIDEO := beat
MM_VIDEO += libfastcrc
MM_VIDEO += libMpeg4SwEncoder
MM_VIDEO += libstreamparser
MM_VIDEO += libswvdec
MM_VIDEO += libUBWC
MM_VIDEO += libvideoutils
MM_VIDEO += libVC1DecDsp_skel
MM_VIDEO += libVC1DecDsp_skel.so
MM_VIDEO += libVC1Dec
MM_VIDEO += libVC1Dec.so
MM_VIDEO += mm-swvenc-test
MM_VIDEO += mm-swvdec-test
MM_VIDEO += mm-vidc-omx-test

#MM_COLOR_CONVERTOR
MM_COLOR_CONVERTOR := libmm-color-convertor
MM_COLOR_CONVERTOR += libI420colorconvert

PRODUCT_PACKAGES += $(MM_VIDEO)
PRODUCT_PACKAGES += $(MM_COLOR_CONVERTOR)

