////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxswtmc11.h
/// @brief camxswtmc11 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSWTMC11_H
#define CAMXSWTMC11_H

// Camx includes
#include "camxformats.h"
#include "camxispiqmodule.h"
#include "camxsensorproperty.h"
#include "titan170_ife.h"
#include "tmc_1_1_0.h"

// Common library includes
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

static FLOAT gamma15tableTMC11[Gamma15TableSize] =
{
    0.0f,   14.0f,  26.0f,  38.0f,  50.0f,  62.0f,  74.0f,  86.0f,
    100.0f, 114.0f, 127.0f, 141.0f, 155.0f, 167.0f, 179.0f, 193.0f,
    207.0f, 219.0f, 231.0f, 241.0f, 251.0f, 263.0f, 275.0f, 285.0f,
    295.0f, 305.0f, 315.0f, 325.0f, 333.0f, 343.0f, 353.0f, 361.0f,
    369.0f, 377.0f, 386.0f, 394.0f, 402.0f, 410.0f, 418.0f, 424.0f,
    430.0f, 438.0f, 444.0f, 450.0f, 458.0f, 464.0f, 470.0f, 476.0f,
    482.0f, 488.0f, 494.0f, 500.0f, 506.0f, 512.0f, 518.0f, 524.0f,
    528.0f, 534.0f, 540.0f, 544.0f, 548.0f, 554.0f, 560.0f, 564.0f,
    568.0f, 572.0f, 578.0f, 582.0f, 588.0f, 592.0f, 596.0f, 600.0f,
    604.0f, 608.0f, 612.0f, 618.0f, 622.0f, 626.0f, 630.0f, 634.0f,
    638.0f, 643.0f, 647.0f, 649.0f, 653.0f, 657.0f, 661.0f, 665.0f,
    669.0f, 672.0f, 675.0f, 679.0f, 683.0f, 687.0f, 689.0f, 693.0f,
    697.0f, 700.0f, 703.0f, 707.0f, 709.0f, 713.0f, 716.0f, 719.0f,
    723.0f, 725.0f, 728.0f, 731.0f, 735.0f, 737.0f, 740.0f, 743.0f,
    747.0f, 749.0f, 752.0f, 755.0f, 757.0f, 760.0f, 763.0f, 765.0f,
    768.0f, 771.0f, 773.0f, 776.0f, 779.0f, 781.0f, 784.0f, 787.0f,
    789.0f, 792.0f, 794.0f, 796.0f, 799.0f, 801.0f, 804.0f, 806.0f,
    808.0f, 811.0f, 813.0f, 816.0f, 818.0f, 820.0f, 822.0f, 824.0f,
    827.0f, 829.0f, 831.0f, 833.0f, 835.0f, 837.0f, 840.0f, 842.0f,
    844.0f, 846.0f, 848.0f, 850.0f, 852.0f, 854.0f, 856.0f, 858.0f,
    860.0f, 862.0f, 864.0f, 866.0f, 868.0f, 870.0f, 872.0f, 874.0f,
    875.0f, 877.0f, 879.0f, 881.0f, 883.0f, 885.0f, 887.0f, 888.0f,
    890.0f, 892.0f, 894.0f, 895.0f, 897.0f, 899.0f, 901.0f, 903.0f,
    905.0f, 907.0f, 909.0f, 911.0f, 912.0f, 914.0f, 915.0f, 917.0f,
    919.0f, 920.0f, 922.0f, 923.0f, 925.0f, 927.0f, 929.0f, 931.0f,
    932.0f, 934.0f, 935.0f, 937.0f, 939.0f, 940.0f, 942.0f, 943.0f,
    945.0f, 947.0f, 948.0f, 950.0f, 951.0f, 953.0f, 955.0f, 956.0f,
    958.0f, 959.0f, 961.0f, 963.0f, 964.0f, 966.0f, 967.0f, 969.0f,
    971.0f, 972.0f, 974.0f, 975.0f, 977.0f, 979.0f, 980.0f, 982.0f,
    983.0f, 985.0f, 987.0f, 988.0f, 990.0f, 991.0f, 993.0f, 995.0f,
    996.0f, 998.0f, 999.0f, 1001.0f, 1003.0f, 1005.0f, 1007.0f, 1008.0f,
    1010.0f, 1011.0f, 1013.0f, 1015.0f, 1017.0f, 1019.0f, 1020.0f, 1022.0f,
    1023.0f
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class for IFE TMC11 Module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SWTMC11 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateBPS
    ///
    /// @brief  Create SWTMC11 Object
    ///
    /// @param  pCreateData Pointer to the BPSPedestal13 Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateBPS(
        BPSModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIFE
    ///
    /// @brief  Create SWTMC11 Object
    ///
    /// @param  pCreateData Pointer to data for Demux Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateIFE(
        IFEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Execute
    ///
    /// @brief  Generate Settings for TMC11 Object
    ///
    /// @param  pInputData Pointer to the Inputdata
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Execute(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVersion
    ///
    /// @brief  Get the TMC version
    ///
    /// @return TMC version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetVersion()
    {
        return TMC11;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SWTMC11
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SWTMC11();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~SWTMC11
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~SWTMC11();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependenceChange
    ///
    /// @brief  Check if the Dependence Data has changed
    ///
    /// @param  pInputData Pointer to the Input Data
    ///
    /// @return TRUE if dependencies met
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Perform the Interpolation and Calculation
    ///
    /// @param  pInputData Pointer to the Input Data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIFEInternalData
    ///
    /// @brief  Update IFE internal data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIFEInternalData(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocatePreGamma15Data
    ///
    /// @brief  Allocate data for Gamma15 pre-calculation
    ///
    /// @return CamxResult success if the allocation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocatePreGamma15Data();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeallocatePreGamma15Data
    ///
    /// @brief  Deallocate data for Gamma15 pre-calculation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeallocatePreGamma15Data();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGammaOutput
    ///
    /// @brief  Get gamma array for TMC 1.1 calculation
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGammaOutput(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PreGammaCheckDependenceChange
    ///
    /// @brief  Check the depedency change for pre-gamma15 calculation
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PreGammaCheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCalculatedUnpackedLUT
    ///
    /// @brief  Get the pointer of calculated unpacked LUT out
    ///
    /// @param  pInput Pointer to the source input
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE FLOAT* GetCalculatedUnpackedLUT(
        VOID* pInput)
    {
        return (static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>
                                        (pInput))->mod_gamma15_channel_data[GammaLUTChannel0].gamma15_rgn_data.table;
    }

    SWTMC11(const SWTMC11&)            = delete;            ///< Disallow the copy constructor
    SWTMC11& operator=(const SWTMC11&) = delete;            ///< Disallow assignment operator

    BOOL                            m_preGamma15Enabled;    ///< Flag to indicate pre-gamma calculation enabled
    ADRCData*                       m_pADRCData;            ///< ADRC Data
    tmc_1_1_0::chromatix_tmc11Type* m_pChromatix;           ///< Pointer to tuning mode data
    TMC11InputData                  m_dependenceData;       ///< Dependence Data for this module

    UINT                                m_offsetLUTCmdBuffer[MaxGammaLUTNum];   ///< Offset of all tables within LUT CmdBuffer
    UINT32*                             m_pPreCalculationPacked;            ///< Pointer to the pre-gamma calculation packed
    FLOAT*                              m_pPreCalculationUnpacked;          ///< Pointer to the pre-gamma calculation unpacked
    Gamma15InputData                    m_preGamma15Data;                   ///< Dependence Data for TMC Pre-Gamma15
    gamma_1_5_0::chromatix_gamma15Type* m_pPreGammaChromatix;               ///< Pointer to tuning mode data
};

CAMX_NAMESPACE_END

#endif // CAMXSWTMC11_H
