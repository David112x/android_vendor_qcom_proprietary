/*==============================================================================
 Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
==============================================================================*/
#ifndef TITAN480_CPAS_TOP_H
#define TITAN480_CPAS_TOP_H

/*----------------------------------------------------------------------
        Offset and Mask
----------------------------------------------------------------------*/

#define CPAS_TOP_REGS_FIRST 0x0

#define CPAS_TOP_REGS_LAST 0xff0

#define CPAS_TOP_REGS_COUNT 0x23

#define regCPAS_TOP_CPAS_0_TITAN_VERSION 0x0  /*register offset*/
#define CPAS_TOP_CPAS_0_TITAN_VERSION_STEP_MASK 0xff
#define CPAS_TOP_CPAS_0_TITAN_VERSION_STEP_SHIFT 0x0
#define CPAS_TOP_CPAS_0_TITAN_VERSION_TIER_MASK 0xff00
#define CPAS_TOP_CPAS_0_TITAN_VERSION_TIER_SHIFT 0x8
#define CPAS_TOP_CPAS_0_TITAN_VERSION_GENERATION_MASK 0xff0000
#define CPAS_TOP_CPAS_0_TITAN_VERSION_GENERATION_SHIFT 0x10
#define CPAS_TOP_CPAS_0_TITAN_VERSION_UNUSED0_MASK 0xff000000
#define CPAS_TOP_CPAS_0_TITAN_VERSION_UNUSED0_SHIFT 0x18

#define regCPAS_TOP_CPAS_0_HW_VERSION 0x4  /*register offset*/
#define CPAS_TOP_CPAS_0_HW_VERSION_STEP_MASK 0xffff
#define CPAS_TOP_CPAS_0_HW_VERSION_STEP_SHIFT 0x0
#define CPAS_TOP_CPAS_0_HW_VERSION_MINOR_MASK 0xfff0000
#define CPAS_TOP_CPAS_0_HW_VERSION_MINOR_SHIFT 0x10
#define CPAS_TOP_CPAS_0_HW_VERSION_MAJOR_MASK 0xf0000000
#define CPAS_TOP_CPAS_0_HW_VERSION_MAJOR_SHIFT 0x1c

#define regCPAS_TOP_CPAS_0_HW_CAPABILITY 0x8  /*register offset*/
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_ICP_MASK 0x1
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_ICP_SHIFT 0x0
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_SECURITY_MASK 0x2
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_SECURITY_SHIFT 0x1
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_XPU3_MASK 0x4
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_XPU3_SHIFT 0x2
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CAMNOC_MASK 0x8
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CAMNOC_SHIFT 0x3
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CPAS_CDM_MASK 0x10
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CPAS_CDM_SHIFT 0x4
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CCI_MASK 0x20
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CCI_SHIFT 0x5
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CCI_1_MASK 0x40
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_CCI_1_SHIFT 0x6
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_JPEG_ENCODE_MASK 0x80
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_JPEG_ENCODE_SHIFT 0x7
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_JPEG_DMA_MASK 0x100
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_JPEG_DMA_SHIFT 0x8
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_FD_MASK 0x200
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_FD_SHIFT 0x9
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_BPS_0_MASK 0x400
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_BPS_0_SHIFT 0xa
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IPE_0_MASK 0x800
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IPE_0_SHIFT 0xb
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_UNUSED0_MASK 0x1000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_UNUSED0_SHIFT 0xc
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_0_MASK 0x2000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_0_SHIFT 0xd
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_1_MASK 0x4000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_1_SHIFT 0xe
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_2_MASK 0x8000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_2_SHIFT 0xf
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_3_MASK 0x10000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_3_SHIFT 0x10
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_4_MASK 0x20000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_4_SHIFT 0x11
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_5_MASK 0x40000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_PHY_5_SHIFT 0x12
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_0_MASK 0x80000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_0_SHIFT 0x13
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_1_MASK 0x100000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_1_SHIFT 0x14
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_LITE_WRP_MASK 0x200000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_IFE_LITE_WRP_SHIFT 0x15
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_SBI_MASK 0x400000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_SBI_SHIFT 0x16
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_UNUSED1_MASK 0xff800000
#define CPAS_TOP_CPAS_0_HW_CAPABILITY_UNUSED1_SHIFT 0x17

#define regCPAS_TOP_CPAS_0_TESTBUS_SEL 0xc  /*register offset*/
#define CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_MASK 0x3f
#define CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_SHIFT 0x0
#define CPAS_TOP_CPAS_0_TESTBUS_SEL_UNUSED0_MASK 0xffffffc0
#define CPAS_TOP_CPAS_0_TESTBUS_SEL_UNUSED0_SHIFT 0x6

#define regCPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON 0x10  /*register offset*/
#define CPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON 0x14  /*register offset*/
#define CPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON 0x18  /*register offset*/
#define CPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_FD_FORCE_CLK_ON 0x1c  /*register offset*/
#define CPAS_TOP_CPAS_0_FD_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_FD_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_FD_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_FD_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON 0x20  /*register offset*/
#define CPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON 0x24  /*register offset*/
#define CPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON 0x28  /*register offset*/
#define CPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON 0x2c  /*register offset*/
#define CPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON 0x30  /*register offset*/
#define CPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON 0x34  /*register offset*/
#define CPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_IPE_0_PWR_STATUS 0x3c  /*register offset*/
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_HW_CONTROL_MASK 0x1
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_HW_CONTROL_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_CLK_OFF_MASK 0x2
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_CLK_OFF_SHIFT 0x1
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_PWR_ON_MASK 0x4
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_PWR_ON_SHIFT 0x2
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_IPE_0_PWR_CTRL 0x40  /*register offset*/
#define CPAS_TOP_CPAS_0_IPE_0_PWR_CTRL_COLLAPSE_MASK 0x1
#define CPAS_TOP_CPAS_0_IPE_0_PWR_CTRL_COLLAPSE_SHIFT 0x0
#define CPAS_TOP_CPAS_0_IPE_0_PWR_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_IPE_0_PWR_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_BPS_PWR_STATUS 0x44  /*register offset*/
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_HW_CONTROL_MASK 0x1
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_HW_CONTROL_SHIFT 0x0
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_CLK_OFF_MASK 0x2
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_CLK_OFF_SHIFT 0x1
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_PWR_ON_MASK 0x4
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_PWR_ON_SHIFT 0x2
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_BPS_PWR_STATUS_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_BPS_PWR_CTRL 0x48  /*register offset*/
#define CPAS_TOP_CPAS_0_BPS_PWR_CTRL_COLLAPSE_MASK 0x1
#define CPAS_TOP_CPAS_0_BPS_PWR_CTRL_COLLAPSE_SHIFT 0x0
#define CPAS_TOP_CPAS_0_BPS_PWR_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_BPS_PWR_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_AHB_ARB_CTRL 0x4c  /*register offset*/
#define CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_MASK 0x7
#define CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_SHIFT 0x0
#define CPAS_TOP_CPAS_0_AHB_ARB_CTRL_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_AHB_ARB_CTRL_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN 0x50  /*register offset*/
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN0_MASK 0x1
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN0_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN1_MASK 0x2
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN1_SHIFT 0x1
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN2_MASK 0x4
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN2_SHIFT 0x2
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN3_MASK 0x8
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN3_SHIFT 0x3
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN4_MASK 0x10
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_MUX_EN4_SHIFT 0x4
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_UNUSED0_MASK 0xffffffe0
#define CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN_UNUSED0_SHIFT 0x5

#define regCPAS_TOP_CPAS_0_CCI_MUX_EN 0x54  /*register offset*/
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL0_MASK 0x7
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL0_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED0_MASK 0xf8
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED0_SHIFT 0x3
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL1_MASK 0x700
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL1_SHIFT 0x8
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED1_MASK 0xf800
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED1_SHIFT 0xb
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL2_MASK 0x70000
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL2_SHIFT 0x10
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED2_MASK 0xf80000
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED2_SHIFT 0x13
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL3_MASK 0x7000000
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_MUX_SEL3_SHIFT 0x18
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED3_MASK 0xf8000000
#define CPAS_TOP_CPAS_0_CCI_MUX_EN_UNUSED3_SHIFT 0x1b

#define regCPAS_TOP_CPAS_0_CCI_1_MUX_EN 0x58  /*register offset*/
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL0_MASK 0x7
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL0_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED0_MASK 0xf8
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED0_SHIFT 0x3
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL1_MASK 0x700
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL1_SHIFT 0x8
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED1_MASK 0xf800
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED1_SHIFT 0xb
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL2_MASK 0x70000
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL2_SHIFT 0x10
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED2_MASK 0xf80000
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED2_SHIFT 0x13
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL3_MASK 0x7000000
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_MUX_SEL3_SHIFT 0x18
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED3_MASK 0xf8000000
#define CPAS_TOP_CPAS_0_CCI_1_MUX_EN_UNUSED3_SHIFT 0x1b

#define regCPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL 0x5c  /*register offset*/
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL_QREQN_MASK 0x1
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL_QREQN_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS 0x60  /*register offset*/
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QACCEPTN_MASK 0x1
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QACCEPTN_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QDENY_MASK 0x2
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QDENY_SHIFT 0x1
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QACTIVE_MASK 0x4
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_QACTIVE_SHIFT 0x2
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL 0x64  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL 0x68  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL 0x6c  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL 0x70  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL 0x74  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL 0x78  /*register offset*/
#define CPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL_CES_MASK 0x1
#define CPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL_CES_SHIFT 0x0
#define CPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_DBG_IF_CTRL 0x80  /*register offset*/
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_ATB_IF_ENABLE_MASK 0x1
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_ATB_IF_ENABLE_SHIFT 0x0
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_NTS_IF_ENABLE_MASK 0x2
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_NTS_IF_ENABLE_SHIFT 0x1
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_APB_IF_ENABLE_MASK 0x4
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_APB_IF_ENABLE_SHIFT 0x2
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_DBG_IF_CTRL_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_DBG_IF_STATUS 0x84  /*register offset*/
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_ATB_IF_ENABLE_ACK_MASK 0x1
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_ATB_IF_ENABLE_ACK_SHIFT 0x0
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_NTS_IF_ENABLE_ACK_MASK 0x2
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_NTS_IF_ENABLE_ACK_SHIFT 0x1
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_APB_IF_ENABLE_ACK_MASK 0x4
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_APB_IF_ENABLE_ACK_SHIFT 0x2
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_UNUSED0_MASK 0xfffffff8
#define CPAS_TOP_CPAS_0_DBG_IF_STATUS_UNUSED0_SHIFT 0x3

#define regCPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON 0x88  /*register offset*/
#define CPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON_FORCE_CLK_ON_MASK 0x1
#define CPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON_FORCE_CLK_ON_SHIFT 0x0
#define CPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON_UNUSED0_SHIFT 0x1

#define regCPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL 0x8c  /*register offset*/
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_CCI_I2C_0_OR_2_SEL_MASK 0x1
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_CCI_I2C_0_OR_2_SEL_SHIFT 0x0
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_UNUSED0_MASK 0xfe
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_UNUSED0_SHIFT 0x1
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_CCI_I2C_1_OR_3_SEL_MASK 0x100
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_CCI_I2C_1_OR_3_SEL_SHIFT 0x8
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_UNUSED1_MASK 0xfffffe00
#define CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL_UNUSED1_SHIFT 0x9

#define regCPAS_TOP_CPAS_0_SPARE 0xff0  /*register offset*/
#define CPAS_TOP_CPAS_0_SPARE_SPARE_MASK 0x1
#define CPAS_TOP_CPAS_0_SPARE_SPARE_SHIFT 0x0
#define CPAS_TOP_CPAS_0_SPARE_UNUSED0_MASK 0xfffffffe
#define CPAS_TOP_CPAS_0_SPARE_UNUSED0_SHIFT 0x1

/*----------------------------------------------------------------------
        Register Data Structures
----------------------------------------------------------------------*/

typedef struct{
    unsigned  STEP : 8; /* 7:0 */
    unsigned  TIER : 8; /* 15:8 */
    unsigned  GENERATION : 8; /* 23:16 */
    unsigned  UNUSED0 : 8; /* 31:24 */
} _cpas_top_cpas_0_titan_version;

typedef union{
    _cpas_top_cpas_0_titan_version bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_TITAN_VERSION;

typedef struct{
    unsigned  STEP : 16; /* 15:0 */
    unsigned  MINOR : 12; /* 27:16 */
    unsigned  MAJOR : 4; /* 31:28 */
} _cpas_top_cpas_0_hw_version;

typedef union{
    _cpas_top_cpas_0_hw_version bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_HW_VERSION;

typedef struct{
    unsigned  ICP : 1; /* 0:0 */
    unsigned  SECURITY : 1; /* 1:1 */
    unsigned  XPU3 : 1; /* 2:2 */
    unsigned  CAMNOC : 1; /* 3:3 */
    unsigned  CPAS_CDM : 1; /* 4:4 */
    unsigned  CCI : 1; /* 5:5 */
    unsigned  CCI_1 : 1; /* 6:6 */
    unsigned  JPEG_ENCODE : 1; /* 7:7 */
    unsigned  JPEG_DMA : 1; /* 8:8 */
    unsigned  FD : 1; /* 9:9 */
    unsigned  BPS_0 : 1; /* 10:10 */
    unsigned  IPE_0 : 1; /* 11:11 */
    unsigned  UNUSED0 : 1; /* 12:12 */
    unsigned  PHY_0 : 1; /* 13:13 */
    unsigned  PHY_1 : 1; /* 14:14 */
    unsigned  PHY_2 : 1; /* 15:15 */
    unsigned  PHY_3 : 1; /* 16:16 */
    unsigned  PHY_4 : 1; /* 17:17 */
    unsigned  PHY_5 : 1; /* 18:18 */
    unsigned  IFE_0 : 1; /* 19:19 */
    unsigned  IFE_1 : 1; /* 20:20 */
    unsigned  IFE_LITE_WRP : 1; /* 21:21 */
    unsigned  SBI : 1; /* 22:22 */
    unsigned  UNUSED1 : 9; /* 31:23 */
} _cpas_top_cpas_0_hw_capability;

typedef union{
    _cpas_top_cpas_0_hw_capability bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_HW_CAPABILITY;

typedef struct{
    unsigned  TESTBUS_SEL : 6; /* 5:0 */
    unsigned  UNUSED0 : 26; /* 31:6 */
} _cpas_top_cpas_0_testbus_sel;

typedef union{
    _cpas_top_cpas_0_testbus_sel bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_TESTBUS_SEL;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_cci_force_clk_on;

typedef union{
    _cpas_top_cpas_0_cci_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CCI_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_cci_1_force_clk_on;

typedef union{
    _cpas_top_cpas_0_cci_1_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CCI_1_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_jpeg_force_clk_on;

typedef union{
    _cpas_top_cpas_0_jpeg_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_JPEG_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_fd_force_clk_on;

typedef union{
    _cpas_top_cpas_0_fd_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_FD_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_ife_0_force_clk_on;

typedef union{
    _cpas_top_cpas_0_ife_0_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IFE_0_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_ife_1_force_clk_on;

typedef union{
    _cpas_top_cpas_0_ife_1_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IFE_1_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_ife_lite_wrap_force_clk_on;

typedef union{
    _cpas_top_cpas_0_ife_lite_wrap_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IFE_LITE_WRAP_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_ipe_0_force_clk_on;

typedef union{
    _cpas_top_cpas_0_ipe_0_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IPE_0_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_bps_force_clk_on;

typedef union{
    _cpas_top_cpas_0_bps_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_BPS_FORCE_CLK_ON;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_cdm_force_clk_on;

typedef union{
    _cpas_top_cpas_0_cdm_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CDM_FORCE_CLK_ON;

typedef struct{
    unsigned  HW_CONTROL : 1; /* 0:0 */
    unsigned  CLK_OFF : 1; /* 1:1 */
    unsigned  PWR_ON : 1; /* 2:2 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_ipe_0_pwr_status;

typedef union{
    _cpas_top_cpas_0_ipe_0_pwr_status bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IPE_0_PWR_STATUS;

typedef struct{
    unsigned  COLLAPSE : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_ipe_0_pwr_ctrl;

typedef union{
    _cpas_top_cpas_0_ipe_0_pwr_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_IPE_0_PWR_CTRL;

typedef struct{
    unsigned  HW_CONTROL : 1; /* 0:0 */
    unsigned  CLK_OFF : 1; /* 1:1 */
    unsigned  PWR_ON : 1; /* 2:2 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_bps_pwr_status;

typedef union{
    _cpas_top_cpas_0_bps_pwr_status bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_BPS_PWR_STATUS;

typedef struct{
    unsigned  COLLAPSE : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_bps_pwr_ctrl;

typedef union{
    _cpas_top_cpas_0_bps_pwr_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_BPS_PWR_CTRL;

typedef struct{
    unsigned  DEFAULT_AHB_MASTER : 3; /* 2:0 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_ahb_arb_ctrl;

typedef union{
    _cpas_top_cpas_0_ahb_arb_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_AHB_ARB_CTRL;

typedef struct{
    unsigned  MUX_EN0 : 1; /* 0:0 */
    unsigned  MUX_EN1 : 1; /* 1:1 */
    unsigned  MUX_EN2 : 1; /* 2:2 */
    unsigned  MUX_EN3 : 1; /* 3:3 */
    unsigned  MUX_EN4 : 1; /* 4:4 */
    unsigned  UNUSED0 : 27; /* 31:5 */
} _cpas_top_cpas_0_cci_gpio_mux_en;

typedef union{
    _cpas_top_cpas_0_cci_gpio_mux_en bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CCI_GPIO_MUX_EN;

typedef struct{
    unsigned  MUX_SEL0 : 3; /* 2:0 */
    unsigned  UNUSED0 : 5; /* 7:3 */
    unsigned  MUX_SEL1 : 3; /* 10:8 */
    unsigned  UNUSED1 : 5; /* 15:11 */
    unsigned  MUX_SEL2 : 3; /* 18:16 */
    unsigned  UNUSED2 : 5; /* 23:19 */
    unsigned  MUX_SEL3 : 3; /* 26:24 */
    unsigned  UNUSED3 : 5; /* 31:27 */
} _cpas_top_cpas_0_cci_mux_en;

typedef union{
    _cpas_top_cpas_0_cci_mux_en bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CCI_MUX_EN;

typedef struct{
    unsigned  MUX_SEL0 : 3; /* 2:0 */
    unsigned  UNUSED0 : 5; /* 7:3 */
    unsigned  MUX_SEL1 : 3; /* 10:8 */
    unsigned  UNUSED1 : 5; /* 15:11 */
    unsigned  MUX_SEL2 : 3; /* 18:16 */
    unsigned  UNUSED2 : 5; /* 23:19 */
    unsigned  MUX_SEL3 : 3; /* 26:24 */
    unsigned  UNUSED3 : 5; /* 31:27 */
} _cpas_top_cpas_0_cci_1_mux_en;

typedef union{
    _cpas_top_cpas_0_cci_1_mux_en bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CCI_1_MUX_EN;

typedef struct{
    unsigned  QREQN : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_camnoc_qchannel_ctrl;

typedef union{
    _cpas_top_cpas_0_camnoc_qchannel_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_CTRL;

typedef struct{
    unsigned  QACCEPTN : 1; /* 0:0 */
    unsigned  QDENY : 1; /* 1:1 */
    unsigned  QACTIVE : 1; /* 2:2 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_camnoc_qchannel_status;

typedef union{
    _cpas_top_cpas_0_camnoc_qchannel_status bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CAMNOC_QCHANNEL_STATUS;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_0_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_0_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_0_AHB2CRIF_CTRL;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_1_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_1_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_1_AHB2CRIF_CTRL;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_2_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_2_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_2_AHB2CRIF_CTRL;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_3_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_3_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_3_AHB2CRIF_CTRL;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_4_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_4_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_4_AHB2CRIF_CTRL;

typedef struct{
    unsigned  CES : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_phy_5_ahb2crif_ctrl;

typedef union{
    _cpas_top_cpas_0_phy_5_ahb2crif_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_PHY_5_AHB2CRIF_CTRL;

typedef struct{
    unsigned  ATB_IF_ENABLE : 1; /* 0:0 */
    unsigned  NTS_IF_ENABLE : 1; /* 1:1 */
    unsigned  APB_IF_ENABLE : 1; /* 2:2 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_dbg_if_ctrl;

typedef union{
    _cpas_top_cpas_0_dbg_if_ctrl bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_DBG_IF_CTRL;

typedef struct{
    unsigned  ATB_IF_ENABLE_ACK : 1; /* 0:0 */
    unsigned  NTS_IF_ENABLE_ACK : 1; /* 1:1 */
    unsigned  APB_IF_ENABLE_ACK : 1; /* 2:2 */
    unsigned  UNUSED0 : 29; /* 31:3 */
} _cpas_top_cpas_0_dbg_if_status;

typedef union{
    _cpas_top_cpas_0_dbg_if_status bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_DBG_IF_STATUS;

typedef struct{
    unsigned  FORCE_CLK_ON : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_sbi_force_clk_on;

typedef union{
    _cpas_top_cpas_0_sbi_force_clk_on bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_SBI_FORCE_CLK_ON;

typedef struct{
    unsigned  CCI_I2C_0_OR_2_SEL : 1; /* 0:0 */
    unsigned  UNUSED0 : 7; /* 7:1 */
    unsigned  CCI_I2C_1_OR_3_SEL : 1; /* 8:8 */
    unsigned  UNUSED1 : 23; /* 31:9 */
} _cpas_top_cpas_0_cam_cci_i2c_tlmm_mux_sel;

typedef union{
    _cpas_top_cpas_0_cam_cci_i2c_tlmm_mux_sel bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_CAM_CCI_I2C_TLMM_MUX_SEL;

typedef struct{
    unsigned  SPARE : 1; /* 0:0 */
    unsigned  UNUSED0 : 31; /* 31:1 */
} _cpas_top_cpas_0_spare;

typedef union{
    _cpas_top_cpas_0_spare bitfields,bits;
    unsigned int u32All;

} CPAS_TOP_CPAS_0_SPARE;

/*----------------------------------------------------------------------
        ENUM Data Structures
----------------------------------------------------------------------*/

typedef enum{
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_DISABLE_TEST_BUS  = 0x0,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_LITE_WRAP_CSID_TEST_BUS  = 0x1,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_LITE_WRAP_TEST_BUS  = 0x2,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_CPAS_CDM_TEST_BUS  = 0x3,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_CCI_TEST_BUS  = 0x4,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_FD_TEST_BUS  = 0x5,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_JPEG_TEST_BUS  = 0x6,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_0_TEST_BUS  = 0x7,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_1_TEST_BUS  = 0x8,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IPE_0_TEST_BUS  = 0x9,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_SBI_TEST_BUS  = 0xa,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_BPS_TEST_BUS  = 0xb,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_0_CSID_TEST_BUS  = 0xc,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_IFE_1_CSID_TEST_BUS  = 0xd,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_CCI_1_TEST_BUS  = 0xe,
    CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_SBI_CSID_TESTBUS  = 0xf
} CPAS_TOP_CPAS_0_TESTBUS_SEL_TESTBUS_SEL_ENUM;


typedef enum{
    CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_EXTERNAL  = 0x0,
    CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_ICP_0  = 0x1,
    CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_CPAS_CDM  = 0x2
} CPAS_TOP_CPAS_0_AHB_ARB_CTRL_DEFAULT_AHB_MASTER_ENUM;

#endif // TITAN480_CPAS_TOP_H
