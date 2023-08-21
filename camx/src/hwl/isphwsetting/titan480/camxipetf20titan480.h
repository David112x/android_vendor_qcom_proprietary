////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipetf20titan480.h
/// @brief IPE TF20 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIPETF20TITAN480_H
#define CAMXIPETF20TITAN480_H

#include "camxisphwsetting.h"
#include "ipe_data.h"
#include "camxipetf20.h"
CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IPE TF20 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPETF20Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

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
    /// @param  pInput       Pointer to the Input data to the Demosaic36 module for calculation
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
    /// @param  pInput       Pointer to the Input data to the Demosaic36 module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegSize
    ///
    /// @brief  Returns register size
    ///
    /// @return Number of bytes of register structure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetRegSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupInternalData
    ///
    /// @brief  Update module internal Data from register
    ///
    /// @param  pData Pointer to internal data to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupInternalData(
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeDefaultRegisterValues
    ///
    /// @brief  Initialize default values for registers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void InitializeDefaultRegisterValues();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculationFullPass
    ///
    /// @brief  Calculate the Register Value
    ///
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculationFullPass();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculationDS4Pass
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculationDS4Pass();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculationDS16Pass
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculationDS16Pass();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillFirmwareConfig0
    ///
    /// @brief  Fill Firmware with HW Config0 register data
    ///
    /// @param  pIPEIQSettings   Pointer to the Firmware IQ Setting Data
    /// @param  passType         pass type: FULL, DC4, DC16, DC64
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillFirmwareConfig0(
        IpeIQSettings*  pIPEIQSettings,
        UINT32          passType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillFirmwareConfig1
    ///
    /// @brief  Fill Firmware with HW Config1 register data
    ///
    /// @param  pIPEIQSettings   Pointer to the Firmware IQ Setting Data
    /// @param  passType         pass type: FULL, DC4, DC16, DC64
    /// @param  bypassMode       bypass enable indication
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillFirmwareConfig1(
        IpeIQSettings*  pIPEIQSettings,
        UINT32          passType,
        BOOL            bypassMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillFirmwareFStoA1A4Map
    ///
    /// @brief  Fill Firmware with HW FS to A1/A4 register data
    ///
    /// @param  pIPEIQSettings   Pointer to the Firmware IQ Setting Data
    /// @param  passType         pass type: FULL, DC4, DC16, DC64
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillFirmwareFStoA1A4Map(
        IpeIQSettings*  pIPEIQSettings,
        UINT32          passType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTFEnable
    ///
    /// @brief  Set module enable for TF
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pDependenceData Pointer to the TF dependance param
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult SetTFEnable(
        ISPInputData* pInputData,
        TF20InputData* pDependenceData)
    {
        CamxResult      result         = CamxResultSuccess;
        IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
        TfParameters* pTFParams = static_cast<TfParameters*>(pDependenceData->pTFParameters);
        // So it needs to be set by "master enable" AND "region enable"
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "E passes %d pDependenceData->moduleE %d pDependenceData %p",
            pDependenceData->maxUsedPasses, pDependenceData->moduleEnable, pDependenceData);
        for (UINT pass = 0; pass < pDependenceData->maxUsedPasses; pass++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " anr %d", pIPEIQSettings->anrParameters.parameters[pass].moduleCfg.EN)
            if (TRUE == pIPEIQSettings->anrParameters.parameters[pass].moduleCfg.EN)
            {
                pTFParams->parameters[pass].moduleCfg.EN &= pDependenceData->moduleEnable;
            }
            else
            {
                pTFParams->parameters[pass].moduleCfg.EN = 0;
            }
            pIPEIQSettings->tfParameters.parameters[pass].moduleCfg.EN =
                pTFParams->parameters[pass].moduleCfg.EN;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " #### Module enable on pass full - DC64 : %d, %d, %d, %d, lmc %d",
                         pIPEIQSettings->tfParameters.parameters[PASS_NAME_FULL].moduleCfg.EN,
                         pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN,
                         pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN,
                         pIPEIQSettings->tfParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN,
                         pIPEIQSettings->lmcParameters.enableLMC);

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRefinementEnable
    ///
    /// @brief  Set module enable for Refinement
    ///
    /// @param  pInputData              Pointer to the ISP input data
    /// @param  bDisableTFRefinement    flag to indicate if TF refinement is disabled by settings
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult SetRefinementEnable(
        const ISPInputData* pInputData,
        BOOL                bDisableTFRefinement)
    {
        CamxResult      result         = CamxResultSuccess;
        IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

        // So it needs to be set by "master enable" AND "region enable"
        if ((FALSE == pIPEIQSettings->tfParameters.parameters[1].moduleCfg.EN) || (TRUE == bDisableTFRefinement))
        {
            pIPEIQSettings->refinementParameters.dc[0].refinementCfg.TRENABLE = 0;
        }

        if ((FALSE == pIPEIQSettings->tfParameters.parameters[2].moduleCfg.EN) || (TRUE == bDisableTFRefinement))
        {
            pIPEIQSettings->refinementParameters.dc[1].refinementCfg.TRENABLE = 0;
        }


        if ((FALSE == pIPEIQSettings->tfParameters.parameters[3].moduleCfg.EN) || (TRUE == bDisableTFRefinement))
        {
            pIPEIQSettings->refinementParameters.dc[2].refinementCfg.TRENABLE = 0;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " #### Refinement enable on pass DC4 - DC64 : %d, %d, %d",
            pIPEIQSettings->refinementParameters.dc[0].refinementCfg.TRENABLE,
            pIPEIQSettings->refinementParameters.dc[1].refinementCfg.TRENABLE,
            pIPEIQSettings->refinementParameters.dc[2].refinementCfg.TRENABLE);

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLMCEnable
    ///
    /// @brief  Set module enable for LMC
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pDependenceData Pointer to the TF dependance param
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult SetLMCEnable(
        const ISPInputData* pInputData,
        TF20InputData* pDependenceData)
    {
        CamxResult      result         = CamxResultSuccess;
        IpeIQSettings*  pIPEIQSettings =
            reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
        if ((NULL != pDependenceData->pLMCParameters)                         &&
            (TRUE == pIPEIQSettings->tfParameters.parameters[0].moduleCfg.EN) &&
            (TRUE == pIPEIQSettings->tfParameters.parameters[1].moduleCfg.EN))
        {
            Utils::Memcpy(&pIPEIQSettings->lmcParameters,
                pDependenceData->pLMCParameters,
                sizeof(pIPEIQSettings->lmcParameters));
        }
        else
        {
            pIPEIQSettings->lmcParameters.enableLMC = 0;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAndSetParams
    ///
    /// @brief  Validate and set params
    ///
    /// @param  pParam         input parameter.
    /// @param  pass           parameter for particular pass.
    /// @param  expectedValue  expected value of input param.
    /// @param  pParamString   String refering to the parameter name.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ValidateAndSetParams(
        UINT32*     pParam,
        UINT32      pass,
        UINT32      expectedValue,
        const CHAR* pParamString)
    {
        if (expectedValue != *pParam)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "%s: pass %d incorrect, resetting", pParamString, pass);
            *pParam = expectedValue;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAndCorrectTFParams
    ///
    /// @brief  Validate and correct MCTF pass params
    ///
    /// @param  pInputData        Pointer to the ISP input data
    /// @param  pDependenceData   Pointer to the TF dependance param
    /// @param  bValidateTFParams Flag to indicate validation of TF Params
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateAndCorrectTFParams(
        const ISPInputData* pInputData,
        TF20InputData* pDependenceData,
        BOOL           bValidateTFParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAndCorrectMCTFParameters
    ///
    /// @brief  Validate and correct MCTF pass parameters
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pDependenceData Pointer to the TF dependance param
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateAndCorrectMCTFParameters(
        const ISPInputData* pInputData,
        TF20InputData* pDependenceData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAndCorrectStillModeParameters
    ///
    /// @brief  Validate and correct Still Mode parameters
    ///
    /// @param  pInputData      Pointer to the ISP input data
    /// @param  pDependenceData Pointer to the TF dependance param
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateAndCorrectStillModeParameters(
        const ISPInputData* pInputData,
        TF20InputData* pDependenceData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPETF20Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPETF20Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPETF20Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPETF20Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    VOID*  m_pRegCmd;                       ///< Pointer to the register command buffer
    UINT   m_offsetPass[PASS_NAME_MAX];     ///< Offset where pass information starts for multipass

    IPETF20Titan480(const IPETF20Titan480&)            = delete; ///< Disallow the copy constructor
    IPETF20Titan480& operator=(const IPETF20Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIPETF20TITAN480_H