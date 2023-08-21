////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSDKSENSORDRIVER_H
#define CAMXSDKSENSORDRIVER_H

// Note: sensor SDK header files are not compliant with CamX coding standards. The file will be removed after sensor SDK design
#include "camxsensorsdkcommon.h"

CAMX_NAMESPACE_BEGIN

/*
 * Sensor driver version is given by:
 * <Major version>.<Minor version>.<Patch version>
 */
#define SENSOR_DRIVER_VERSION "2.11.3"
#define SENSOR_SDK_CAPABILITIES "Bayer,YUV and mono sensor,\
short and long exposure-gain control, \
short exposure control in direct or auto mode based on HDR type, \
in-sensor HDR with AE stats, RAW HDR, PDAF with Phase difference \
or PD Pixel stats, HAL3, support common apis for gain calculation, \
filling integration time, fll and gain, and PDAF HW stats parsing,\
RDI data packing"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define MAX_CID 16
#define CSI_EMBED_DATA        0x12
#define CSI_RESERVED_DATA_0   0x13
#define CSI_YUV422_8          0x1E
#define CSI_RAW8              0x2A
#define CSI_RAW10             0x2B
#define CSI_RAW12             0x2C
#define CSI_RAW14             0x2D
#define CSI_DECODE_6BIT         0
#define CSI_DECODE_8BIT         1
#define CSI_DECODE_10BIT        2
#define CSI_DECODE_12BIT        3
#define CSI_DECODE_14BIT        8
#define CSI_DECODE_DPCM_10_8_10 5

/* rdi_mode options */
#define CSI_RDI_RAW             0
#define CSI_RDI_PACKED          (1<<6)
/* plain_alignment options*/
#define CSI_RDI_LSB_ALIGNED     0
#define CSI_RDI_MSB_ALIGNED     (1<<5)
/* plain_format options */
#define CSI_RDI_PLAIN8          0
#define CSI_RDI_PLAIN16         (1<<4)

#define CSI_DECODE_10BIT_PLAIN16_LSB \
  (CSI_RDI_PACKED|CSI_RDI_LSB_ALIGNED|CSI_RDI_PLAIN16|CSI_DECODE_10BIT)
#define CSI_DECODE_10BIT_PLAIN16_MSB \
  (CSI_RDI_PACKED|CSI_RDI_MSB_ALIGNED|CSI_RDI_PLAIN16|CSI_DECODE_10BIT)

/* Non HFR mode for normal camera, camcorder usecases */
#define SENSOR_DEFAULT_MODE (1 << 0)
/* HFR mode used to capture slow motion video */
#define SENSOR_HFR_MODE (1 << 1)
/* HDR mode used to High Dynamic Range imaging */
#define SENSOR_HDR_MODE (1 << 2)
/* RAW HDR mode used to stream raw HDR */
#define SENSOR_RAW_HDR_MODE (1 << 3)
/* Macro for using proper chromatix library. */
#define SENSOR_LOAD_CHROMATIX(name, mode) \
  "libchromatix_" name "_" #mode ".so"
/* MOUNT ANGLE > = to this value is considered invalid in sensor lib */
#define SENSOR_MOUNTANGLE_360 360
/* Sensor mount angle. */
#define SENSOR_MOUNTANGLE_0 0
#define SENSOR_MOUNTANGLE_90 90
#define SENSOR_MOUNTANGLE_180 180
#define SENSOR_MOUNTANGLE_270 270

/* OEM's can extend these modes */
#define MAX_RESOLUTION_MODES 13
#define MAX_META_DATA_SIZE 3
#define MAX_SENSOR_STREAM 5
#define MAX_SENSOR_DATA_TYPE 5
#define MAX_SENSOR_EFFECT 3
#define MAX_SENSOR_OPTICAL_BLACK_REGION 2

typedef unsigned long sensor_capability_t;

#define SENSOR_VIDEO_HDR_FLAG (((sensor_capability_t)1UL << 8) | 0)
#define SENSOR_SNAPSHOT_HDR_FLAG (((sensor_capability_t)1UL << 15) | 0)
#define SENSOR_AWB_UPDATE (((sensor_capability_t)1UL << 8) | 1)
#define MAX_SENSOR_SETTING_I2C_REG 1024

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

enum camsensor_mode {
  CAMSENSOR_MODE_2D =      (1 << 0),
  CAMSENSOR_MODE_3D =      (1 << 1),
  CAMSENSOR_MODE_INVALID = (1 << 2),
};

typedef enum {
  SENSOR_DELAY_EXPOSURE,            /* delay for exposure*/
  SENSOR_DELAY_ANALOG_SENSOR_GAIN,  /* delay for sensor analog gain*/
  SENSOR_DELAY_DIGITAL_SENSOR_GAIN, /* delay for sensor digital gain*/
  SENSOR_DELAY_ISP_GAIN,            /* delay for sensor ISP (error) gain*/
  SENSOR_DELAY_MAX,
} sensor_delay_type_t;

typedef enum {
  CAMIF_BAYER_G_B,
  CAMIF_BAYER_B_G,
  CAMIF_BAYER_G_R,
  CAMIF_BAYER_R_G,
  CAMIF_YCBCR_Y_CB_Y_CR,
  CAMIF_YCBCR_Y_CR_Y_CB,
  CAMIF_YCBCR_CB_Y_CR_Y,
  CAMIF_YCBCR_CR_Y_CB_Y,
  CAMIF_YCBCR_4_2_2_LINEPACKED, /* Only valid for Offline Snapshot */
  CAMIF_YCBCR_4_2_0_LINEPACKED, /* Only valid for Offline Snapshot */
  CAMIF_NUMBEROF_INPUTFORMATTYPE  /* Used for count purposes only */
} sensor_camif_inputformat_t;

typedef enum {
  SENSOR_BAYER,
  SENSOR_YCBCR,
  SENSOR_GREY,
  SENSOR_META,
} sensor_output_format_t;

typedef enum {
  SENSOR_PARALLEL,
  SENSOR_MIPI_CSI,
  SENSOR_MIPI_CSI_1,
  SENSOR_MIPI_CSI_2,
} sensor_connection_mode_t;

typedef enum {
  SENSOR_8_BIT_DIRECT,
  SENSOR_10_BIT_DIRECT,
  SENSOR_12_BIT_DIRECT,
  SENSOR_14_BIT_DIRECT
} sensor_raw_output_t;

typedef enum {
  SENSOR_BGGR,
  SENSOR_GBRG,
  SENSOR_GRBG,
  SENSOR_RGGB,
  SENSOR_Y,
  SENSOR_UYVY,
  SENSOR_YUYV,
  SENSOR_MONO,
} sensor_filter_arrangement;

typedef enum {
  SENSOR_SMETHOD_NOT_DEFINED = 1,
  SENSOR_SMETHOD_ONE_CHIP_COLOR_AREA_SENSOR=2,
  SENSOR_SMETHOD_TWO_CHIP_COLOR_AREA_SENSOR=3,
  SENSOR_SMETHOD_THREE_CHIP_COLOR_AREA_SENSOR=4,
  SENSOR_SMETHOD_COLOR_SEQ_AREA_SENSOR=5,
  SENSOR_SMETHOD_TRILINEAR_SENSOR=7,
  SENSOR_SMETHOD_COLOR_SEQ_LINEAR_SENSOR=8
}sensor_sensing_method_type_t;

typedef enum {
  SENSOR_TEST_PATTERN_OFF,
  SENSOR_TEST_PATTERN_SOLID_COLOR,
  SENSOR_TEST_PATTERN_COLOR_BARS,
  SENSOR_TEST_PATTERN_COLOR_BARS_FADE_TO_GRAY,
  SENSOR_TEST_PATTERN_PN9,
  SENSOR_TEST_PATTERN_CUSTOM1,
  SENSOR_TEST_PATTERN_MAX,
} sensor_test_pattern_t;

typedef enum {
  SENSOR_EFFECT_OFF,
  SENSOR_EFFECT_SET_SATURATION,
  SENSOR_EFFECT_SET_CONTRAST,
  SENSOR_EFFECT_SET_SHARPNESS,
  SENSOR_EFFECT_SET_ISO,
  SENSOR_EFFECT_SET_EXPOSURE_COMPENSATION,
  SENSOR_EFFECT_SET_ANTIBANDING,
  SENSOR_EFFECT_SET_WHITEBALANCE,
  SENSOR_EFFECT_SET_BEST_SHOT,
  SENSOR_EFFECT_SET_EFFECT,
  SENSOR_EFFECT_MAX,
} sensor_effect_t;

enum sensor_camera_id {
  CAMERA_ID_0,
  CAMERA_ID_1,
  CAMERA_ID_2,
  CAMERA_ID_3,
  CAMERA_ID_MAX,
};

typedef enum {
  SENSOR_HDR_OFF,
  SENSOR_HDR_IN_SENSOR,
  SENSOR_HDR_RAW,
  SENSOR_HDR_MAX,
} sensor_hdr_type_t;

struct sensor_id_info_t {
  UINT32 sensor_id_reg_addr;
  unsigned short sensor_id;
};

struct camera_sensor_slave_info {
  const char *sensor_name;
  unsigned short slave_addr;
  enum camera_i2c_freq_mode i2c_freq_mode;
  camera_i2c_reg_addr_type addr_type;
  camera_i2c_data_type data_type;
  struct sensor_id_info_t sensor_id_info;
  struct camera_power_setting_array power_setting_array;
};

struct sensor_test_mode_addr_t {
  unsigned short r_addr;
  unsigned short gr_addr;
  unsigned short gb_addr;
  unsigned short b_addr;
};

typedef struct {
  sensor_test_pattern_t mode;
  struct camera_i2c_reg_setting_array settings;
} sensor_test_pattern_settings;

typedef struct {
  sensor_test_pattern_settings test_pattern_settings[SENSOR_TEST_PATTERN_MAX];
  unsigned char size;
  struct sensor_test_mode_addr_t solid_mode_addr;
} sensor_test_info;

struct sensor_effect_settings{
  sensor_effect_t mode;
  struct camera_i2c_reg_setting_array settings;
};

struct sensor_effect_info{
  struct sensor_effect_settings effect_settings[MAX_SENSOR_EFFECT];
  unsigned char size;
};

struct sensor_crop_parms_t {
  unsigned short top_crop;
  unsigned short bottom_crop;
  unsigned short left_crop;
  unsigned short right_crop;
};

typedef struct {
  sensor_output_format_t output_format;
  sensor_connection_mode_t connection_mode;
  sensor_raw_output_t raw_output;
  sensor_filter_arrangement filter_arrangement;
} sensor_output_t;

/**
  Constants used in standard smia gain formula
  Analog Gain = (m0 * X + c0) / (m1 * X + c1)
  X is register gain
**/
typedef struct {
  int m0;
  int m1;
  int m2;
  int c0;
  int c1;
  int c2;
} analaog_gain_map_coeff;

/**
  dig_gain_decimator  -> Factor used for digital gain
                         adjustment
**/
typedef struct {
  float min_gain;
  float max_gain;
  float max_analog_gain;
  float min_analog_gain;
  float min_digital_gain;
  float max_digital_gain;
  int dig_gain_decimator;
  unsigned int max_linecount;
  analaog_gain_map_coeff smia_type_gain_coeff;
} sensor_aec_data_t;

typedef struct {
  float pix_size;
  sensor_sensing_method_type_t sensing_method;
  float crop_factor; //depends on sensor physical dimensions
} sensor_property_t;

typedef struct {
  int width;
  int height;
} sensor_dimension_t;

typedef struct {
  unsigned short x_start;
  unsigned short y_start;
  unsigned short width;
  unsigned short height;
} sensor_dimension_rectangle_t;

typedef struct {
  sensor_dimension_rectangle_t optical_black_region[
    MAX_SENSOR_OPTICAL_BLACK_REGION];
  unsigned char size;
} sensor_optical_black_region_t;

typedef struct {
  sensor_dimension_t active_array_size;
  unsigned short left_dummy;
  unsigned short right_dummy;
  unsigned short top_dummy;
  unsigned short bottom_dummy;
} sensor_imaging_pixel_array_size;

typedef struct {
  unsigned short white_level;
  unsigned short r_pedestal;
  unsigned short gr_pedestal;
  unsigned short gb_pedestal;
  unsigned short b_pedestal;
} sensor_color_level_info;

/* sensor_lib_out_info_t: store information about different resolution
 * supported by sensor
 *
 * x_output: sensor output width (pixels)
 * y_output: sensor output height (pixels)
 * line_length_pclk: number of pixels per line
 * frame_length_lines: number of lines per frame
 * vt_pixel_clk: sensor scanning rate (cycles / sec)
 * op_pixel_clk: actual sensor output rate (cycles / sec)
 * binning_factor: 1 for no binning, 2 for V 1/2 H 1/2 binning and so on
 * binning_method: 0 for average, 1 for addition (summed)
 * min_fps: minimum fps that can be supported for this resolution
 * max_fps: maximum fps that can be supported for this sensor
 * mode: mode / modes for which this resolution can be used
 *       SENSOR_DEFAULT_MODE / SENSOR_HFR_MODE*/
struct sensor_lib_out_info_t {
  unsigned short x_output;
  unsigned short y_output;
  unsigned short line_length_pclk;
  unsigned short frame_length_lines;
  unsigned int vt_pixel_clk;
  unsigned int op_pixel_clk;
  unsigned short binning_factor;
  unsigned short binning_method;
  float    min_fps;
  float    max_fps;
  unsigned int mode;
};

typedef struct {
  unsigned int full_size_width;
  unsigned int full_size_height;
  unsigned int full_size_left_crop;
  unsigned int full_size_top_crop;
} sensor_full_size_info_t;

typedef struct {
    unsigned char enable;
    sensor_full_size_info_t full_size_info;
} sensor_rolloff_config;

struct sensor_lib_out_info_array {
  /* sensor output for each resolutions */
  struct sensor_lib_out_info_t out_info[MAX_RESOLUTION_MODES];

  /* Number of valid entries in out_info array */
  unsigned short size;
};

struct camera_i2c_reg_setting {
  struct camera_i2c_reg_info *reg_setting;
  unsigned short size;
  camera_i2c_reg_addr_type addr_type;
  camera_i2c_data_type data_type;
  unsigned short delay;
};

struct sensor_lib_reg_settings_array {
  struct camera_i2c_reg_setting_array reg_settings[MAX_RESOLUTION_MODES];
  unsigned int                        size;
};

struct sensor_csi_params {
  unsigned char  lane_cnt;
  unsigned char  settle_cnt;
  unsigned char  is_csi_3phase;
};

struct sensor_csid_vc_cfg {
  unsigned char cid;
  unsigned char dt;
  unsigned char decode_format;
};

struct sensor_csid_lut_params {
  unsigned char num_cid;
  struct sensor_csid_vc_cfg vc_cfg_a[MAX_CID];
};

struct sensor_csid_lut_params_array {
  struct sensor_csid_lut_params lut_params[MAX_RESOLUTION_MODES];
  unsigned short size;
};

struct sensor_csid_testmode_parms {
  unsigned int num_bytes_per_line;
  unsigned int num_lines;
  unsigned int h_blanking_count;
  unsigned int v_blanking_count;
  unsigned int payload_mode;
};

struct sensor_lib_crop_params_array{
  struct sensor_crop_parms_t crop_params[MAX_RESOLUTION_MODES];
  unsigned short size;
};

typedef struct {
  unsigned int reg_gain;
  unsigned int line_count;
  unsigned int sensor_reg_digital_gain;
  unsigned int s_reg_gain;
  float        sensor_real_gain;
  float        sensor_real_digital_gain;
  float        isp_digital_gain;
} sensor_exposure_info_t;

typedef struct _sensor_stream_info_t {
  unsigned short vc_cfg_size;
  struct sensor_csid_vc_cfg vc_cfg[MAX_SENSOR_DATA_TYPE];
  sensor_output_format_t pix_data_fmt[MAX_SENSOR_DATA_TYPE];
} sensor_stream_info_t;

typedef struct _sensor_stream_info_array_t {
  sensor_stream_info_t sensor_stream_info[MAX_SENSOR_STREAM];
  unsigned short size;
} sensor_stream_info_array_t;

struct sensor_output_reg_addr_t {
  unsigned short x_output;
  unsigned short y_output;
  unsigned short line_length_pclk;
  unsigned short frame_length_lines;
};

struct sensor_exp_gain_info_t {
  unsigned short coarse_int_time_addr;
  unsigned short short_coarse_int_time_addr;
  unsigned short global_gain_addr;
  unsigned short short_global_gain_addr;
  unsigned short dig_gain_r_addr;
  unsigned short dig_gain_gr_addr;
  unsigned short dig_gain_b_addr;
  unsigned short dig_gain_gb_addr;
  unsigned short vert_offset;
};

typedef enum sensor_stats_types {
  HDR_STATS,
  PD_STATS,
} sensor_stats_type_t;

struct sensor_meta_data_out_info_t {
  unsigned int width;
  unsigned int height;
  unsigned int stats_type;
  unsigned char  dt;
};

struct sensor_lib_meta_data_info_array {
  /* meta data output */
  struct sensor_meta_data_out_info_t meta_data_out_info[MAX_META_DATA_SIZE];

  /* Number of valid entries in meta data array */
  unsigned short size;
};

struct sensor_noise_coefficient_t {
  double gradient_S;
  double offset_S;
  double gradient_O;
  double offset_O;
};

/** fill_exp structure explains about the way api is designed
    to update gain, exposure.
    FILL_CUSTOM_IN_LIB        -> Use api available
                                 in sensor library
    FILL_2B_GAIN_2B_IT_2B_FLL -> Use api which takes programs
                                 gain in 2Bytes, 2 bytes of
                                 Integration time and fll in 2bytes
    FILL_2B_GAIN_3B_IT_2B_FLL -> Use api which takes programs Gain
                                 in 2Bytes and Integration time in 3 bytes
                                 and fll of 2 bytes.
    FILL_CUSTOM_IN_CMN_LIB    -> Implement custom library in
                                 common library instead of sensor library.
**/
typedef enum {
  FILL_CUSTOM_IN_LIB,
  FILL_2B_GAIN_2B_IT_2B_FLL,
  FILL_2B_GAIN_3B_IT_2B_FLL,
  FILL_CUSTOM_IN_CMN_LIB,
} fill_exp;

typedef struct {
  /** Tells the way the gain is computed and how gain and exposure are applied
   fill_exp_array_type     -> Mentions how exposure and gain api is implemented
   calc_exp_array_type     -> Mentions how gain is calculated
  **/
  fill_exp fill_exp_array_type;
  /** Function to calculate exosure based on real gain and
   *  linecount value, 1st param - real gain, 2nd param -
   *  linecount, 3rd param - exposure info output, return staus -
   *  success / failure */
  int (*sensor_calculate_exposure) (float, unsigned int,
    sensor_exposure_info_t *, float);

  /** Function to create register table from exposure settings
   * input param1 - register gain value
   * input param2 - digital gain value
   * input param3 - coarse integration time value
   * input param4 - frame length line value
   * input param5 - hdr luma
   * input param6 - hdr param
   * input param7 - register settings
   * return value - 0 for success and negative value for
   * failure **/
  int (*sensor_fill_exposure_array)(unsigned int, unsigned int, unsigned int,
      unsigned int, int, unsigned int, struct camera_i2c_reg_setting *,
      unsigned int, int, int);
} sensor_exposure_table_t;

typedef enum {
  SENSOR_STATS_CUSTOM = 0,
  SENSOR_STATS_RAW10_8B_CONF_10B_PD, /*8 bits confidence, 10 bits PD*/
  SENSOR_STATS_RAW10_11B_CONF_11B_PD, /*11 bits confidence, 11 bits PD*/
} sensor_stats_format_t;

typedef struct {
  int (*parse_VHDR_stats)(unsigned int *, void *);
  int (*parse_PDAF_stats)(unsigned int *, void *);
  sensor_stats_format_t pd_data_format;
} sensor_RDI_parser_stats_t;

struct ImageSensorlib {
  /* sensor slave info */
  struct camera_sensor_slave_info sensor_slave_info;

  /* sensor output settings */
  sensor_output_t sensor_output;

  /* sensor output register address */
  struct sensor_output_reg_addr_t output_reg_addr;

  /* sensor exposure gain register address */
  struct sensor_exp_gain_info_t exp_gain_info;

  /* sensor aec info */
  sensor_aec_data_t aec_info;

  /* sensor port info that consists of cid mask and fourcc mapping */
  sensor_stream_info_array_t sensor_stream_info_array;

  /* Sensor Settings */
  struct camera_i2c_reg_setting_array start_settings;
  struct camera_i2c_reg_setting_array stop_settings;
  struct camera_i2c_reg_setting_array groupon_settings;
  struct camera_i2c_reg_setting_array groupoff_settings;
  /* Sensor Settings Array */
  struct sensor_lib_reg_settings_array init_settings_array;
  struct sensor_lib_reg_settings_array res_settings_array;

  struct sensor_lib_out_info_array     out_info_array;
  struct sensor_csi_params             csi_params;
  struct sensor_csid_lut_params_array  csid_lut_params_array;
  sensor_exposure_table_t exposure_func_table;

  const CHAR *pTunedDataFile;
} ;

CAMX_NAMESPACE_END

#endif // CAMXSDKSENSORDRIVER_H
