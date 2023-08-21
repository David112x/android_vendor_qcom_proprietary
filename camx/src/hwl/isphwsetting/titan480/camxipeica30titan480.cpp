////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeica30titan480.cpp
/// @brief IPEICA30Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "icasetting.h"
#include "camxutils.h"
#include "camxipeica30titan480.h"
#include "camxipeica30.h"
#include "Process_ICA.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA30Titan480::IPEICA30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA30Titan480::IPEICA30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA30Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA30Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pICAHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*           pInputData             = static_cast<ISPInputData*>(pSettingData);
    ICA30HWSettingParams*   pModuleHWSettingParams = reinterpret_cast<ICA30HWSettingParams*>(pICAHWSettingParams);

    m_pLUTCmdBuffer       = pModuleHWSettingParams->pLUTCmdBuffer;
    m_pICAModuleData      = pModuleHWSettingParams->pModuleData;

    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Fail to WriteLUTtoDMI, result: %d", result);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA30Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA30Titan480::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;
    CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
    UINT8       ICADMIBank[ICA30Indexmax];
    UINT32      ICADMIAddr;
    ICA30ModuleData*  pICAModuleData = reinterpret_cast<ICA30ModuleData*>(m_pICAModuleData);

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        if (INPUT == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA30IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_PERSP_LUT;
            ICADMIBank[ICA30IndexGrid0]       = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_GRID_0_LUT;
            ICADMIBank[ICA30IndexGrid1]       = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_GRID_1_LUT;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_0_DMI_CFG;
        }
        else if (REFERENCE == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA30IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_PERSP_LUT;
            ICADMIBank[ICA30IndexGrid0]       = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_GRID_0_LUT;
            ICADMIBank[ICA30IndexGrid1]       = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_GRID_1_LUT;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_1_DMI_CFG;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Path selected");
        }

        CAMX_ASSERT(NULL != pDMICmdBuffer);

        // Record the offset within DMI Header buffer, it is the offset at which this module has written a CDM DMI header.
        if (CamxResultSuccess == result)
        {
            // Loop for a batch if separate tranform for each frames in a batch
            // Multiply the batch index to update for each frame in a batch if separate
            // While using the same data for each frames in a batch, just update the same buffer in cdm program
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA30IndexPerspective],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA30IndexPerspective],
                                             IPEICA30LUTSize[ICA30IndexPerspective]);
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA30IndexGrid0],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA30IndexGrid0],
                                             IPEICA30LUTSize[ICA30IndexGrid0]);
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA30IndexGrid1],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA30IndexGrid1],
                                             IPEICA30LUTSize[ICA30IndexGrid1]);
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
// IPEICA30Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA30Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    ICAOutputData*       pOutput            = static_cast<ICAOutputData*>(pOutputVal);
    ICAUnpackedField*    pUnpackedData      = static_cast<ICAUnpackedField*>(pInput);

    UINT32*           pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA30IndexPerspective]);
    UINT64*           pGrid0                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA30IndexGrid0]);
    UINT64*           pGrid1                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA30IndexGrid1]);
    ICA_REG_v30*      pRegData              =
        static_cast<ICA_REG_v30*>(pUnpackedData->pRegData);
    IcaParameters*    pIcaParams            =
        static_cast<IcaParameters*>(pUnpackedData->pIcaParameter);

    if (1 == pIcaParams->isPerspectiveEnable)
    {
        for (UINT i = 0; i < ICAPerspectiveSize; i++)
        {
            pPerspectivetransform[i] =
                ((pRegData->CTC_PERSPECTIVE_PARAMS_M[i] & 0x3FFFF) |
                ((pRegData->CTC_PERSPECTIVE_PARAMS_E[i] & 0x3F) << 18));
        }
    }

    if (1 == pIcaParams->isGridEnable)
    {
        UINT  j = 0;
        UINT  k = 0;
        for (UINT i = 0; i < ICA30GridRegSize; i++)
        {
            if (0 == (i % 2))
            {
                pGrid0[j] = (((static_cast<UINT64>(pRegData->CTC_GRID_X[i])) << 18) |
                    (static_cast<UINT64>(pRegData->CTC_GRID_Y[i]) & 0x3FFFF)) & 0xFFFFFFFFF;
                j++;
            }
            else
            {
                pGrid1[k] = (((static_cast<UINT64>(pRegData->CTC_GRID_X[i])) << 18) |
                    (static_cast<UINT64>(pRegData->CTC_GRID_Y[i]) & 0x3FFFF)) & 0xFFFFFFFFF;
                k++;
            }
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
// IPEICA30Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA30Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA30Titan480::~IPEICA30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA30Titan480::~IPEICA30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA30Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA30Titan480::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
