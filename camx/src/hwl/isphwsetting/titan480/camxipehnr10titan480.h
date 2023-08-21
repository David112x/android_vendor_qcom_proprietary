////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipehnr10titan480.h
/// @brief IPE HNR10 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIPEHNR10TITAN480_H
#define CAMXIPEHNR10TITAN480_H

#include "titan480_ipe.h"
#include "camxisphwsetting.h"
#include "hnr10setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief HNR10 register Configuration
struct IPEHNR10RegConfig
{
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_0            nrGainCoefficient0;        ///< HNR NR GAIN REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_1            nrGainCoefficient1;        ///< HNR NR GAIN REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_2            nrGainCoefficient2;        ///< HNR NR GAIN REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_3            nrGainCoefficient3;        ///< HNR NR GAIN REGISTER3
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_4            nrGainCoefficient4;        ///< HNR NR GAIN REGISTER4
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_5            nrGainCoefficient5;        ///< HNR NR GAIN REGISTER5
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_6            nrGainCoefficient6;        ///< HNR NR GAIN REGISTER6
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_7            nrGainCoefficient7;        ///< HNR NR GAIN REGISTER7
    IPE_IPE_0_PPS_CLC_HNR_NR_GAIN_TABLE_8            nrGainCoefficient8;        ///< HNR NR GAIN REGISTER8
    IPE_IPE_0_PPS_CLC_HNR_CNR_CFG_0                  cnrCFGCoefficient0;        ///< HNR CNR CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_CNR_CFG_1                  cnrCFGCoefficient1;        ///< HNR CNR CONFIG REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_CNR_GAIN_TABLE_0           cnrGaninCoeffincient0;     ///< HNR CNR GAIN REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_CNR_GAIN_TABLE_1           cnrGaninCoeffincient1;     ///< HNR CNR GAIN REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_CNR_GAIN_TABLE_2           cnrGaninCoeffincient2;     ///< HNR CNR GAIN REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_CNR_GAIN_TABLE_3           cnrGaninCoeffincient3;     ///< HNR CNR GAIN REGISTER3
    IPE_IPE_0_PPS_CLC_HNR_CNR_GAIN_TABLE_4           cnrGaninCoeffincient4;     ///< HNR CNR GAIN REGISTER4
    IPE_IPE_0_PPS_CLC_HNR_SNR_CFG_0                  snrCoefficient0;           ///< HNR SNR CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_SNR_CFG_1                  snrCoefficient1;           ///< HNR SNR CONFIG REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_SNR_CFG_2                  snrCoefficient2;           ///< HNR SNR CONFIG REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_FACE_CFG                   faceConfigCoefficient;     ///< HNR FACE CONFIG REGISTER
    IPE_IPE_0_PPS_CLC_HNR_FACE_OFFSET_CFG            faceOffsetCoefficient;     ///< HNR FACE OFFSET CONFIG REGISTER
    IPE_IPE_0_PPS_CLC_HNR_FACE_0_CENTER_CFG          faceCoefficient0;          ///< HNR FACE CENTER CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_FACE_1_CENTER_CFG          faceCoefficient1;          ///< HNR FACE CENTER CONFIG REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_FACE_2_CENTER_CFG          faceCoefficient2;          ///< HNR FACE CENTER CONFIG REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_FACE_3_CENTER_CFG          faceCoefficient3;          ///< HNR FACE CENTER CONFIG REGISTER3
    IPE_IPE_0_PPS_CLC_HNR_FACE_4_CENTER_CFG          faceCoefficient4;          ///< HNR FACE CENTER CONFIG REGISTER4
    IPE_IPE_0_PPS_CLC_HNR_FACE_0_RADIUS_CFG          radiusCoefficient0;        ///< HNR FACE RADIUS CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_FACE_1_RADIUS_CFG          radiusCoefficient1;        ///< HNR FACE RADIUS CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_FACE_2_RADIUS_CFG          radiusCoefficient2;        ///< HNR FACE RADIUS CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_FACE_3_RADIUS_CFG          radiusCoefficient3;        ///< HNR FACE RADIUS CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_FACE_4_RADIUS_CFG          radiusCoefficient4;        ///< HNR FACE RADIUS CONFIG REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_0 rnrAnchorCoefficient0;     ///< RNR ANCHOR BASE SETTING REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_1 rnrAnchorCoefficient1;     ///< RNR ANCHOR BASE SETTING REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_2 rnrAnchorCoefficient2;     ///< RNR ANCHOR BASE SETTING REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_3 rnrAnchorCoefficient3;     ///< RNR ANCHOR BASE SETTING REGISTER3
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_4 rnrAnchorCoefficient4;     ///< RNR ANCHOR BASE SETTING REGISTER4
    IPE_IPE_0_PPS_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_5 rnrAnchorCoefficient5;     ///< RNR ANCHOR BASE SETTING REGISTER5
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_0 rnrSlopeShiftCoefficient0; ///< RNR SLOPE SHIFT SETTING REGISTER0
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_1 rnrSlopeShiftCoefficient1; ///< RNR SLOPE SHIFT SETTING REGISTER1
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_2 rnrSlopeShiftCoefficient2; ///< RNR SLOPE SHIFT SETTING REGISTER2
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_3 rnrSlopeShiftCoefficient3; ///< RNR SLOPE SHIFT SETTING REGISTER3
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_4 rnrSlopeShiftCoefficient4; ///< RNR SLOPE SHIFT SETTING REGISTER4
    IPE_IPE_0_PPS_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_5 rnrSlopeShiftCoefficient5; ///< RNR SLOPE SHIFT SETTING REGISTER5
    IPE_IPE_0_PPS_CLC_HNR_RNR_INIT_HV_OFFSET         hnrCoefficient1;           ///< RNR INIT HV OFFSET REGISTER
    IPE_IPE_0_PPS_CLC_HNR_RNR_R_SQUARE_INIT          rnr_r_squareCoefficient;   ///< RNR R SQUARE INIT REGISTER
    IPE_IPE_0_PPS_CLC_HNR_RNR_R_SCALE_SHIFT          rnr_r_ScaleCoefficient;    ///< RNR R SCALE SHIFT REGISTER
    IPE_IPE_0_PPS_CLC_HNR_LPF3_CFG                   lpf3ConfigCoefficient;     ///< HNR LPF3 CONFIG REGISTER
    IPE_IPE_0_PPS_CLC_HNR_MISC_CFG                   miscConfigCoefficient;     ///< HNR MIS CONFIG REGISTER
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 HNRMaxLUT                    = 6;
static const UINT32 LNRGainLUT                   = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_LNR_GAIN_ARR_LUT;
static const UINT32 MergedFNRGainAndGainClampLUT = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_MERGED_FNR_ARR_LUT;
static const UINT32 FNRAcLUT                     = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_FNR_ACTH_ARR_LUT;
static const UINT32 SNRGainLUT                   = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_SNR_GAIN_ARR_LUT;
static const UINT32 BlendLNRGainLUT              = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_BLEND_LNRGAIN_ARR_LUT;
static const UINT32 BlendSNRGainLUT              = IPE_IPE_0_PPS_CLC_HNR_DMI_LUT_CFG_LUT_SEL_BLEND_SNRGAIN_ARR_LUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IPE HNR10 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEHNR10Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteLUTEntrytoDMI
    ///
    /// @brief  Write LUT Entry to DMI
    ///
    /// @param  pDMICmdBuffer   Pointer to the DMI command buffer used for programming HFI CDM packet/payload
    /// @param  LUTIndex        HNR LUT Index [1..HNRMaxLUT-1]
    /// @param  pLUTOffset      [io] Pointer to LUT offset into DMI command buffer; incremented by LUTSize, if not-NULL
    /// @param  LUTSize         Size of LUT (in bytes)
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteLUTEntrytoDMI(
        CmdBuffer*   pDMICmdBuffer,
        const UINT8  LUTIndex,
        UINT32*      pLUTOffset,
        const UINT32 LUTSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteLUTtoDMI
    ///
    /// @brief  Write Look up table data pointer into DMI header
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if write to DMI header was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteLUTtoDMI(
        VOID* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteConfigCmds
    ///
    /// @brief  Write Configuration Commands
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if configuration was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteConfigCmds(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData          Pointer to the Inputdata
    /// @param  pHNRHWSettingParams Pointer to HNR HW Setting params structure contains pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateCmdList(
        VOID*   pInputData,
        UINT32* pHNRHWSettingParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateTuningMetadata
    ///
    /// @brief  Update Tuning Metadata
    ///
    /// @param  pInputData      Pointer to the InputData
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UpdateTuningMetadata(
        VOID*  pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pInput       Pointer to the Input data to the HNR10 module for calculation
    /// @param  pOutput      Pointer to the Output data for DMI
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PackIQRegisterSetting(
        VOID*  pInput,
        VOID*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRegisterSetting
    ///
    /// @brief  Setup register value based on CamX Input
    ///
    /// @param  pInput       Pointer to the Input data to the HNR10 module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPEHNR10Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEHNR10Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEHNR10Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPEHNR10Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegSize
    ///
    /// @brief  Returns register size
    ///
    /// @return Number of bytes of register structure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetRegSize();

private:
    VOID*                             m_pRegCmd;                      ///< Pointer to the register command buffer
    CmdBuffer*                        m_pLUTDMICmdBuffer;             ///< Shadow copy of module LUT command buffer

    IPEHNR10Titan480(const IPEHNR10Titan480&)               = delete; ///< Disallow the copy constructor
    IPEHNR10Titan480& operator=(const IPEHNR10Titan480&)    = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIPEHNR10TITAN480_H
