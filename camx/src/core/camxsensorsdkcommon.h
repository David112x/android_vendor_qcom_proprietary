////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SENSOR_SDK_COMMON_H__
#define __SENSOR_SDK_COMMON_H__

// Note: sensor SDK header files are not compliant with CamX coding standards. The file will be removed after sensor SDK design
/// @todo (CAMX-772) Translate SDK headers to CSL definitions

#define NAME_SIZE_MAX           64
#define I2C_REG_SET_MAX         100
#define MAX_POWER_CONFIG        12
#define U_I2C_SEQ_REG_DATA_MAX  1024

#define GPIO_OUT_LOW            (0 << 1)
#define GPIO_OUT_HIGH           (1 << 1)
#define MAX_PDAF_WIN            200

enum camera_power_sequence_type
{
    CAMERA_POWER_SEQUENCE_MCLK = 0,              ///< Clock
    CAMERA_POWER_SEQUENCE_VANA,                  ///< Analog voltage
    CAMERA_POWER_SEQUENCE_VDIG,                  ///< Digital voltage
    CAMERA_POWER_SEQUENCE_VIO,                   ///< I/O voltage
    CAMERA_POWER_SEQUENCE_VAF,                   ///< AF voltage
    CAMERA_POWER_SEQUENCE_VAFPWDN,               ///< AF power down
    CAMERA_POWER_SEQUENCE_CUSTOMREG1,            ///< Customer Reg1
    CAMERA_POWER_SEQUENCE_CUSTOMREG2,            ///< Customer Reg2
    CAMERA_POWER_SEQUENCE_RESET,                 ///< Reset
    CAMERA_POWER_SEQUENCE_STANDBY,               ///< Standby
    CAMERA_POWER_SEQUENCE_CUSTOMGPIO1,           ///< GPIO1
    CAMERA_POWER_SEQUENCE_CUSTOMGPIO2,           ///< GPIO2
    CAMERA_POWER_SEQUENCE_TYPEMAX,               ///< MAX
};

enum camera_i2c_freq_mode {
  SENSOR_I2C_MODE_STANDARD,
  SENSOR_I2C_MODE_FAST,
  SENSOR_I2C_MODE_CUSTOM,
  SENSOR_I2C_MODE_FAST_PLUS,
  SENSOR_I2C_MODE_MAX,
};

enum camera_i2c_reg_addr_type {
  CAMERA_I2C_BYTE_ADDR = 1,
  CAMERA_I2C_WORD_ADDR,
  CAMERA_I2C_3B_ADDR,
  CAMERA_I2C_DWORD_ADDR,
  CAMERA_I2C_ADDR_TYPE_MAX,
};

enum camera_i2c_data_type {
  CAMERA_I2C_BYTE_DATA = 1,
  CAMERA_I2C_WORD_DATA,
  CAMERA_I2C_DWORD_DATA,
  CAMERA_I2C_SET_BYTE_MASK,
  CAMERA_I2C_UNSET_BYTE_MASK,
  CAMERA_I2C_SET_WORD_MASK,
  CAMERA_I2C_UNSET_WORD_MASK,
  CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA,
  CAMERA_I2C_DATA_TYPE_MAX,
};

enum camera_power_seq_type {
  CAMERA_POW_SEQ_CLK,
  CAMERA_POW_SEQ_GPIO,
  CAMERA_POW_SEQ_VREG,
  CAMERA_POW_SEQ_I2C_MUX,
  CAMERA_POW_SEQ_I2C,
};

enum camera_gpio_type {
  CAMERA_GPIO_RESET,
  CAMERA_GPIO_STANDBY,
  CAMERA_GPIO_AF_PWDM,
  CAMERA_GPIO_VIO,
  CAMERA_GPIO_VANA,
  CAMERA_GPIO_VDIG,
  CAMERA_GPIO_VAF,
  CAMERA_GPIO_FL_EN,
  CAMERA_GPIO_FL_NOW,
  CAMERA_GPIO_FL_RESET,
  CAMERA_GPIO_CUSTOM1,
  CAMERA_GPIO_CUSTOM2,
  CAMERA_GPIO_MAX,
};

enum camera_clk_type {
  CAMERA_MCLK,
  CAMERA_CLK,
  CAMERA_CLK_MAX,
};

enum camera_vreg_name {
  CAMERA_VDIG,
  CAMERA_VIO,
  CAMERA_VANA,
  CAMERA_VAF,
  CAMERA_V_CUSTOM1,
  CAMERA_V_CUSTOM2,
  CAMERA_VREG_MAX,
};

struct camera_i2c_reg_info {
  unsigned int reg_addr;
  unsigned int reg_data;
  unsigned int delay;
};

enum camera_i2c_operation {
  CAMERA_I2C_OP_WRITE = 0,
  CAMERA_I2C_OP_WRITE_BURST,
  CAMERA_I2C_OP_WRITE_SEQUENTIAL,
  CAMERA_I2C_OP_POLL,
  CAMERA_I2C_OP_READ,
  CAMERA_I2C_OP_MAX,
};

struct camera_reg_settings_t {
  unsigned int reg_addr;
  camera_i2c_reg_addr_type addr_type;
  unsigned int reg_data;
  camera_i2c_data_type data_type;
  camera_i2c_operation i2c_operation;
  unsigned int delay;
};

struct camera_i2c_reg_setting_array {
  camera_i2c_operation       operation;
  struct camera_i2c_reg_info reg_setting_a[I2C_REG_SET_MAX];
  unsigned short             size;
  camera_i2c_reg_addr_type   addr_type;
  camera_i2c_data_type       data_type;
};

struct camera_power_setting {
  enum camera_power_seq_type seq_type;
  unsigned short seq_val;
  long config_val;
  unsigned short delay;
};

struct camera_power_setting_array {
  struct camera_power_setting  power_setting_a[MAX_POWER_CONFIG];
  unsigned short size;
  struct camera_power_setting  power_down_setting_a[MAX_POWER_CONFIG];
  unsigned short size_down;
};

#endif /* __SENSOR_SDK_COMMON_H__ */

