////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeica20titan17x.cpp
/// @brief IPEICA20Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "titan170_ipe.h"
#include "icasetting.h"
#include "camxutils.h"
#include "camxipeica20titan17x.h"
#include "camxipeica20.h"
#include "Process_ICA.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::IPEICA20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA20Titan17x::IPEICA20Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA20Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pICAHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*           pInputData             = static_cast<ISPInputData*>(pSettingData);
    ICA20HWSettingParams*   pModuleHWSettingParams = reinterpret_cast<ICA20HWSettingParams*>(pICAHWSettingParams);

    m_pLUTCmdBuffer  = pModuleHWSettingParams->pLUTCmdBuffer;
    m_pICAModuleData = pModuleHWSettingParams->pModuleData;

    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Fail to WriteLUTtoDMI, result: %d", result);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA20Titan17x::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult        result         = CamxResultSuccess;
    CmdBuffer*        pDMICmdBuffer  = NULL;
    if (pInputData->pipelineIPEData.ppIPECmdBuffer)
    {
        pDMICmdBuffer  = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
    }
    ICA20ModuleData*  pICAModuleData = reinterpret_cast<ICA20ModuleData*>(m_pICAModuleData);
    UINT8             ICADMIBank[ICA20Indexmax];
    UINT32            ICADMIAddr;

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        if (INPUT == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA20IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_PERSP_LUT_HANA;
            ICADMIBank[ICA20IndexGrid]        = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_GRID_LUT_HANA;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_0_DMI_CFG;
        }
        else if (REFERENCE == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA20IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_PERSP_LUT_HANA;
            ICADMIBank[ICA20IndexGrid]        = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_GRID_LUT_HANA;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_1_DMI_CFG;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Path selected");
        }

        CAMX_ASSERT(NULL != pDMICmdBuffer);

        if (CamxResultSuccess == result)
        {
            // Loop for a batch if separate tranform for each frames in a batch
            // Multiply the batch index to update for each frame in a batch if separate
            // While using the same data for each frames in a batch, just update the same buffer in cdm program
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA20IndexPerspective],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA20IndexPerspective],
                                             IPEICA20LUTSize[ICA20IndexPerspective]);
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA20IndexGrid],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA20IndexGrid],
                                             IPEICA20LUTSize[ICA20IndexGrid]);
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "WriteLUTtoDMI failed %d for path %d",
                       result, pICAModuleData->IPEPath);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA20Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    ICAOutputData*       pOutput            = static_cast<ICAOutputData*>(pOutputVal);
    ICAUnpackedField*    pUnpackedData      = static_cast<ICAUnpackedField*>(pInput);

    UINT32*           pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA20IndexPerspective]);
    UINT64*           pGrid                 =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA20IndexGrid]);
    ICA_REG_v30*      pRegData              =
        static_cast<ICA_REG_v30*>(pUnpackedData->pRegData);
    IcaParameters*    pIcaParams            =
        static_cast<IcaParameters*>(pUnpackedData->pIcaParameter);

    if (1 == pIcaParams->isPerspectiveEnable)
    {
        for (UINT i = 0; i < ICAPerspectiveSize; i++)
        {
            pPerspectivetransform[i] =
                (((pRegData->CTC_PERSPECTIVE_PARAMS_M[i] & 0xFFFF)) |
                (pRegData->CTC_PERSPECTIVE_PARAMS_E[i] & 0x3F) << 16);
        }
    }

    if (1 == pIcaParams->isGridEnable)
    {
        for (UINT i = 0; i < ICA20GridRegSize; i++)
        {
            pGrid[i] = (((static_cast<UINT64>(pRegData->CTC_GRID_X[i])) << 17) |
                (static_cast<UINT64>(pRegData->CTC_GRID_Y[i]) & 0x1FFFF)) & 0x3FFFFFFFF;
        }
    }

    // Assign Output data that needs to be stored by software
    pOutput->pCurrICAInData      = pUnpackedData->pCurrICAInData;
    pOutput->pPrevICAInData      = pUnpackedData->pPrevICAInData;
    pOutput->pCurrWarpAssistData = pUnpackedData->pCurrWarpAssistData;
    pOutput->pPrevWarpAssistData = pUnpackedData->pPrevWarpAssistData;
    pOutput->pWarpGeometryData   = pUnpackedData->pWarpGeometryData;


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA20Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::~IPEICA20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA20Titan17x::~IPEICA20Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA20Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA20Titan17x::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
