/*
 * Copyright (c) 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

//
// -----------------------------------------------------------------------------
// Copyright (c) 2011 Qualcomm Atheros, Inc.  All rights reserved.
// -----------------------------------------------------------------------------
// FILE         : efuse_reg.h
// DESCRIPTION  : Software Header File for WiFi 2.0
// THIS FILE IS AUTOMATICALLY GENERATED BY DENALI BLUEPRINT, DO NOT EDIT
// -----------------------------------------------------------------------------
//

#ifndef _EFUSE_REG_H_
#define _EFUSE_REG_H_

#include "rtc_soc_reg.h"

#ifndef __EFUSE_REG_BASE_ADDRESS
#define __EFUSE_REG_BASE_ADDRESS (0x87000)
#endif

#define SEQ_EOS_TOP_REG_EFUSE_REG_DEC_OFFSET 0x0

// 0x0 (EFUSE_WR_ENABLE_REG)
#define EFUSE_WR_ENABLE_REG_V_LSB                                              0
#define EFUSE_WR_ENABLE_REG_V_MSB                                              0
#define EFUSE_WR_ENABLE_REG_V_MASK                                             0x1
#define EFUSE_WR_ENABLE_REG_V_GET(x)                                           (((x) & EFUSE_WR_ENABLE_REG_V_MASK) >> EFUSE_WR_ENABLE_REG_V_LSB)
#define EFUSE_WR_ENABLE_REG_V_SET(x)                                           (((0 | (x)) << EFUSE_WR_ENABLE_REG_V_LSB) & EFUSE_WR_ENABLE_REG_V_MASK)
#define EFUSE_WR_ENABLE_REG_V_RESET                                            0x0
#define EFUSE_WR_ENABLE_REG_ADDRESS                                            (0x0 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_WR_ENABLE_REG_RSTMASK                                            0x1
#define EFUSE_WR_ENABLE_REG_RESET                                              0x0

// 0x4 (EFUSE_INT_ENABLE_REG)
#define EFUSE_INT_ENABLE_REG_V_LSB                                             0
#define EFUSE_INT_ENABLE_REG_V_MSB                                             0
#define EFUSE_INT_ENABLE_REG_V_MASK                                            0x1
#define EFUSE_INT_ENABLE_REG_V_GET(x)                                          (((x) & EFUSE_INT_ENABLE_REG_V_MASK) >> EFUSE_INT_ENABLE_REG_V_LSB)
#define EFUSE_INT_ENABLE_REG_V_SET(x)                                          (((0 | (x)) << EFUSE_INT_ENABLE_REG_V_LSB) & EFUSE_INT_ENABLE_REG_V_MASK)
#define EFUSE_INT_ENABLE_REG_V_RESET                                           0x1
#define EFUSE_INT_ENABLE_REG_ADDRESS                                           (0x4 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_INT_ENABLE_REG_RSTMASK                                           0x1
#define EFUSE_INT_ENABLE_REG_RESET                                             0x1

// 0x8 (EFUSE_INT_STATUS_REG)
#define EFUSE_INT_STATUS_REG_V_LSB                                             0
#define EFUSE_INT_STATUS_REG_V_MSB                                             0
#define EFUSE_INT_STATUS_REG_V_MASK                                            0x1
#define EFUSE_INT_STATUS_REG_V_GET(x)                                          (((x) & EFUSE_INT_STATUS_REG_V_MASK) >> EFUSE_INT_STATUS_REG_V_LSB)
#define EFUSE_INT_STATUS_REG_V_SET(x)                                          (((0 | (x)) << EFUSE_INT_STATUS_REG_V_LSB) & EFUSE_INT_STATUS_REG_V_MASK)
#define EFUSE_INT_STATUS_REG_V_RESET                                           0x0
#define EFUSE_INT_STATUS_REG_ADDRESS                                           (0x8 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_INT_STATUS_REG_RSTMASK                                           0x1
#define EFUSE_INT_STATUS_REG_RESET                                             0x0

// 0xc (BITMASK_WR_REG)
#define BITMASK_WR_REG_V_LSB                                                   0
#define BITMASK_WR_REG_V_MSB                                                   31
#define BITMASK_WR_REG_V_MASK                                                  0xffffffff
#define BITMASK_WR_REG_V_GET(x)                                                (((x) & BITMASK_WR_REG_V_MASK) >> BITMASK_WR_REG_V_LSB)
#define BITMASK_WR_REG_V_SET(x)                                                (((0 | (x)) << BITMASK_WR_REG_V_LSB) & BITMASK_WR_REG_V_MASK)
#define BITMASK_WR_REG_V_RESET                                                 0x0
#define BITMASK_WR_REG_ADDRESS                                                 (0xc + __EFUSE_REG_BASE_ADDRESS)
#define BITMASK_WR_REG_RSTMASK                                                 0xffffffff
#define BITMASK_WR_REG_RESET                                                   0x0

// 0x10 (VDDQ_SETTLE_TIME_REG)
#define VDDQ_SETTLE_TIME_REG_V_LSB                                             0
#define VDDQ_SETTLE_TIME_REG_V_MSB                                             31
#define VDDQ_SETTLE_TIME_REG_V_MASK                                            0xffffffff
#define VDDQ_SETTLE_TIME_REG_V_GET(x)                                          (((x) & VDDQ_SETTLE_TIME_REG_V_MASK) >> VDDQ_SETTLE_TIME_REG_V_LSB)
#define VDDQ_SETTLE_TIME_REG_V_SET(x)                                          (((0 | (x)) << VDDQ_SETTLE_TIME_REG_V_LSB) & VDDQ_SETTLE_TIME_REG_V_MASK)
#define VDDQ_SETTLE_TIME_REG_V_RESET                                           0x30
#define VDDQ_SETTLE_TIME_REG_ADDRESS                                           (0x10 + __EFUSE_REG_BASE_ADDRESS)
#define VDDQ_SETTLE_TIME_REG_RSTMASK                                           0xffffffff
#define VDDQ_SETTLE_TIME_REG_RESET                                             0x30

// 0x14 (VDDQ_HOLD_TIME_REG)
#define VDDQ_HOLD_TIME_REG_V_LSB                                               0
#define VDDQ_HOLD_TIME_REG_V_MSB                                               31
#define VDDQ_HOLD_TIME_REG_V_MASK                                              0xffffffff
#define VDDQ_HOLD_TIME_REG_V_GET(x)                                            (((x) & VDDQ_HOLD_TIME_REG_V_MASK) >> VDDQ_HOLD_TIME_REG_V_LSB)
#define VDDQ_HOLD_TIME_REG_V_SET(x)                                            (((0 | (x)) << VDDQ_HOLD_TIME_REG_V_LSB) & VDDQ_HOLD_TIME_REG_V_MASK)
#define VDDQ_HOLD_TIME_REG_V_RESET                                             0x1
#define VDDQ_HOLD_TIME_REG_ADDRESS                                             (0x14 + __EFUSE_REG_BASE_ADDRESS)
#define VDDQ_HOLD_TIME_REG_RSTMASK                                             0xffffffff
#define VDDQ_HOLD_TIME_REG_RESET                                               0x1

// 0x18 (RD_STROBE_PW_REG)
#define RD_STROBE_PW_REG_V_LSB                                                 0
#define RD_STROBE_PW_REG_V_MSB                                                 31
#define RD_STROBE_PW_REG_V_MASK                                                0xffffffff
#define RD_STROBE_PW_REG_V_GET(x)                                              (((x) & RD_STROBE_PW_REG_V_MASK) >> RD_STROBE_PW_REG_V_LSB)
#define RD_STROBE_PW_REG_V_SET(x)                                              (((0 | (x)) << RD_STROBE_PW_REG_V_LSB) & RD_STROBE_PW_REG_V_MASK)
#define RD_STROBE_PW_REG_V_RESET                                               0x2
#define RD_STROBE_PW_REG_ADDRESS                                               (0x18 + __EFUSE_REG_BASE_ADDRESS)
#define RD_STROBE_PW_REG_RSTMASK                                               0xffffffff
#define RD_STROBE_PW_REG_RESET                                                 0x2

// 0x1c (PG_STROBE_PW_REG)
#define PG_STROBE_PW_REG_V_LSB                                                 0
#define PG_STROBE_PW_REG_V_MSB                                                 31
#define PG_STROBE_PW_REG_V_MASK                                                0xffffffff
#define PG_STROBE_PW_REG_V_GET(x)                                              (((x) & PG_STROBE_PW_REG_V_MASK) >> PG_STROBE_PW_REG_V_LSB)
#define PG_STROBE_PW_REG_V_SET(x)                                              (((0 | (x)) << PG_STROBE_PW_REG_V_LSB) & PG_STROBE_PW_REG_V_MASK)
#define PG_STROBE_PW_REG_V_RESET                                               0x1e1
#define PG_STROBE_PW_REG_ADDRESS                                               (0x1c + __EFUSE_REG_BASE_ADDRESS)
#define PG_STROBE_PW_REG_RSTMASK                                               0xffffffff
#define PG_STROBE_PW_REG_RESET                                                 0x1e1

// 0x20 (PGENB_SETUP_HOLD_TIME_REG)
#define PGENB_SETUP_HOLD_TIME_REG_V_LSB                                        0
#define PGENB_SETUP_HOLD_TIME_REG_V_MSB                                        31
#define PGENB_SETUP_HOLD_TIME_REG_V_MASK                                       0xffffffff
#define PGENB_SETUP_HOLD_TIME_REG_V_GET(x)                                     (((x) & PGENB_SETUP_HOLD_TIME_REG_V_MASK) >> PGENB_SETUP_HOLD_TIME_REG_V_LSB)
#define PGENB_SETUP_HOLD_TIME_REG_V_SET(x)                                     (((0 | (x)) << PGENB_SETUP_HOLD_TIME_REG_V_LSB) & PGENB_SETUP_HOLD_TIME_REG_V_MASK)
#define PGENB_SETUP_HOLD_TIME_REG_V_RESET                                      0x2
#define PGENB_SETUP_HOLD_TIME_REG_ADDRESS                                      (0x20 + __EFUSE_REG_BASE_ADDRESS)
#define PGENB_SETUP_HOLD_TIME_REG_RSTMASK                                      0xffffffff
#define PGENB_SETUP_HOLD_TIME_REG_RESET                                        0x2

// 0x24 (STROBE_PULSE_INTERVAL_REG)
#define STROBE_PULSE_INTERVAL_REG_V_LSB                                        0
#define STROBE_PULSE_INTERVAL_REG_V_MSB                                        31
#define STROBE_PULSE_INTERVAL_REG_V_MASK                                       0xffffffff
#define STROBE_PULSE_INTERVAL_REG_V_GET(x)                                     (((x) & STROBE_PULSE_INTERVAL_REG_V_MASK) >> STROBE_PULSE_INTERVAL_REG_V_LSB)
#define STROBE_PULSE_INTERVAL_REG_V_SET(x)                                     (((0 | (x)) << STROBE_PULSE_INTERVAL_REG_V_LSB) & STROBE_PULSE_INTERVAL_REG_V_MASK)
#define STROBE_PULSE_INTERVAL_REG_V_RESET                                      0x1
#define STROBE_PULSE_INTERVAL_REG_ADDRESS                                      (0x24 + __EFUSE_REG_BASE_ADDRESS)
#define STROBE_PULSE_INTERVAL_REG_RSTMASK                                      0xffffffff
#define STROBE_PULSE_INTERVAL_REG_RESET                                        0x1

// 0x28 (CSB_ADDR_LOAD_SETUP_HOLD_REG)
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_LSB                                     0
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_MSB                                     31
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_MASK                                    0xffffffff
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_GET(x)                                  (((x) & CSB_ADDR_LOAD_SETUP_HOLD_REG_V_MASK) >> CSB_ADDR_LOAD_SETUP_HOLD_REG_V_LSB)
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_SET(x)                                  (((0 | (x)) << CSB_ADDR_LOAD_SETUP_HOLD_REG_V_LSB) & CSB_ADDR_LOAD_SETUP_HOLD_REG_V_MASK)
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_V_RESET                                   0x1
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_ADDRESS                                   (0x28 + __EFUSE_REG_BASE_ADDRESS)
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_RSTMASK                                   0xffffffff
#define CSB_ADDR_LOAD_SETUP_HOLD_REG_RESET                                     0x1

// 0x2c (SUR_PD_CSB_REG)
#define SUR_PD_CSB_REG_V_LSB                                                   0
#define SUR_PD_CSB_REG_V_MSB                                                   31
#define SUR_PD_CSB_REG_V_MASK                                                  0xffffffff
#define SUR_PD_CSB_REG_V_GET(x)                                                (((x) & SUR_PD_CSB_REG_V_MASK) >> SUR_PD_CSB_REG_V_LSB)
#define SUR_PD_CSB_REG_V_SET(x)                                                (((0 | (x)) << SUR_PD_CSB_REG_V_LSB) & SUR_PD_CSB_REG_V_MASK)
#define SUR_PD_CSB_REG_V_RESET                                                 0x19
#define SUR_PD_CSB_REG_ADDRESS                                                 (0x2c + __EFUSE_REG_BASE_ADDRESS)
#define SUR_PD_CSB_REG_RSTMASK                                                 0xffffffff
#define SUR_PD_CSB_REG_RESET                                                   0x19

// 0x30 (SUP_PS_CSB_REG)
#define SUP_PS_CSB_REG_V_LSB                                                   0
#define SUP_PS_CSB_REG_V_MSB                                                   31
#define SUP_PS_CSB_REG_V_MASK                                                  0xffffffff
#define SUP_PS_CSB_REG_V_GET(x)                                                (((x) & SUP_PS_CSB_REG_V_MASK) >> SUP_PS_CSB_REG_V_LSB)
#define SUP_PS_CSB_REG_V_SET(x)                                                (((0 | (x)) << SUP_PS_CSB_REG_V_LSB) & SUP_PS_CSB_REG_V_MASK)
#define SUP_PS_CSB_REG_V_RESET                                                 0x3
#define SUP_PS_CSB_REG_ADDRESS                                                 (0x30 + __EFUSE_REG_BASE_ADDRESS)
#define SUP_PS_CSB_REG_RSTMASK                                                 0xffffffff
#define SUP_PS_CSB_REG_RESET                                                   0x3

// 0x34 (EFUSE_BUZY_STATUS_REG)
#define EFUSE_BUZY_STATUS_REG_V_LSB                                            0
#define EFUSE_BUZY_STATUS_REG_V_MSB                                            0
#define EFUSE_BUZY_STATUS_REG_V_MASK                                           0x1
#define EFUSE_BUZY_STATUS_REG_V_GET(x)                                         (((x) & EFUSE_BUZY_STATUS_REG_V_MASK) >> EFUSE_BUZY_STATUS_REG_V_LSB)
#define EFUSE_BUZY_STATUS_REG_V_SET(x)                                         (((0 | (x)) << EFUSE_BUZY_STATUS_REG_V_LSB) & EFUSE_BUZY_STATUS_REG_V_MASK)
#define EFUSE_BUZY_STATUS_REG_V_RESET                                          0x0
#define EFUSE_BUZY_STATUS_REG_ADDRESS                                          (0x34 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_BUZY_STATUS_REG_RSTMASK                                          0x1
#define EFUSE_BUZY_STATUS_REG_RESET                                            0x0

// 0x800 (EFUSE_INTF0)
#define EFUSE_INTF0_R_LSB                                                      0
#define EFUSE_INTF0_R_MSB                                                      31
#define EFUSE_INTF0_R_MASK                                                     0xffffffff
#define EFUSE_INTF0_R_GET(x)                                                   (((x) & EFUSE_INTF0_R_MASK) >> EFUSE_INTF0_R_LSB)
#define EFUSE_INTF0_R_SET(x)                                                   (((0 | (x)) << EFUSE_INTF0_R_LSB) & EFUSE_INTF0_R_MASK)
#define EFUSE_INTF0_R_RESET                                                    0x0
#define EFUSE_INTF0_ADDRESS                                                    (0x800 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_INTF0_RSTMASK                                                    0xffffffff
#define EFUSE_INTF0_RESET                                                      0x0

// 0x1000 (EFUSE_INTF1)
#define EFUSE_INTF1_R_LSB                                                      0
#define EFUSE_INTF1_R_MSB                                                      31
#define EFUSE_INTF1_R_MASK                                                     0xffffffff
#define EFUSE_INTF1_R_GET(x)                                                   (((x) & EFUSE_INTF1_R_MASK) >> EFUSE_INTF1_R_LSB)
#define EFUSE_INTF1_R_SET(x)                                                   (((0 | (x)) << EFUSE_INTF1_R_LSB) & EFUSE_INTF1_R_MASK)
#define EFUSE_INTF1_R_RESET                                                    0x0
#define EFUSE_INTF1_ADDRESS                                                    (0x1000 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_INTF1_RSTMASK                                                    0xffffffff
#define EFUSE_INTF1_RESET                                                      0x0

// 0x1800 (EFUSE_INTF2)
#define EFUSE_INTF2_R_LSB                                                      0
#define EFUSE_INTF2_R_MSB                                                      31
#define EFUSE_INTF2_R_MASK                                                     0xffffffff
#define EFUSE_INTF2_R_GET(x)                                                   (((x) & EFUSE_INTF2_R_MASK) >> EFUSE_INTF2_R_LSB)
#define EFUSE_INTF2_R_SET(x)                                                   (((0 | (x)) << EFUSE_INTF2_R_LSB) & EFUSE_INTF2_R_MASK)
#define EFUSE_INTF2_R_RESET                                                    0x0
#define EFUSE_INTF2_ADDRESS                                                    (0x1800 + __EFUSE_REG_BASE_ADDRESS)
#define EFUSE_INTF2_RSTMASK                                                    0xffffffff
#define EFUSE_INTF2_RESET                                                      0x0



#endif /* _EFUSE_REG_H_ */
