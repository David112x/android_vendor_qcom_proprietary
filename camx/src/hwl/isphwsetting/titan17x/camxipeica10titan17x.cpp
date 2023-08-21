////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeica10titan17x.cpp
/// @brief IPEICA10Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "icasetting.h"
#include "camxutils.h"
#include "camxipeica10.h"
#include "camxipeica10titan17x.h"
#include "Process_ICA.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10Titan17x::IPEICA10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA10Titan17x::IPEICA10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pICAHWSettingParams)
{
    CamxResult result = CamxResultSuccess;

    ISPInputData*           pInputData             = static_cast<ISPInputData*>(pSettingData);
    ICA10HWSettingParams*   pModuleHWSettingParams = reinterpret_cast<ICA10HWSettingParams*>(pICAHWSettingParams);

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
// IPEICA10Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10Titan17x::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult        result         = CamxResultSuccess;
    CmdBuffer*        pDMICmdBuffer  = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
    ICA10ModuleData*  pICAModuleData = reinterpret_cast<ICA10ModuleData*>(m_pICAModuleData);
    UINT8             ICADMIBank[ICA10Indexmax];
    UINT32            ICADMIAddr;


    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        if (INPUT == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA10IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_PERSP_LUT;
            ICADMIBank[ICA10IndexGrid0]       = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_GRID_LUT_0;
            ICADMIBank[ICA10IndexGrid1]       = IPE_IPE_0_NPS_CLC_ICA_0_DMI_LUT_CFG_LUT_SEL_GRID_LUT_1;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_0_DMI_CFG;
        }
        else if (REFERENCE == pICAModuleData->IPEPath)
        {
            ICADMIBank[ICA10IndexPerspective] = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_PERSP_LUT;
            ICADMIBank[ICA10IndexGrid0]       = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_GRID_LUT_0;
            ICADMIBank[ICA10IndexGrid1]       = IPE_IPE_0_NPS_CLC_ICA_1_DMI_LUT_CFG_LUT_SEL_GRID_LUT_1;
            ICADMIAddr                        = regIPE_IPE_0_NPS_CLC_ICA_1_DMI_CFG;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Path selected");
        }

        CAMX_ASSERT(NULL != m_pLUTCmdBuffer);
        CAMX_ASSERT(NULL != pDMICmdBuffer);

        if (CamxResultSuccess == result)
        {
            // Loop for a batch if separate tranform for each frames in a batch
            // Multiply the batch index to update for each frame in a batch if separate
            // While using the same data for each frames in a batch, just update the same buffer in cdm program
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA10IndexPerspective],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA10IndexPerspective],
                                             IPEICA10LUTSize[ICA10IndexPerspective]);
        }

        if (CamxResultSuccess == result)
        {

            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA10IndexGrid0],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA10IndexGrid0],
                                             IPEICA10LUTSize[ICA10IndexGrid0]);
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             ICADMIAddr,
                                             ICADMIBank[ICA10IndexGrid1],
                                             m_pLUTCmdBuffer,
                                             pICAModuleData->offsetLUTCmdBuffer[ICA10IndexGrid1],
                                             IPEICA10LUTSize[ICA10IndexGrid1]);
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
// IPEICA10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);
    ICAOutputData*       pOutput         = static_cast<ICAOutputData*>(pOutputVal);
    ICAUnpackedField*    pUnpackedData   = static_cast<ICAUnpackedField*>(pInput);

    UINT32*           pPerspectivetransform =
        reinterpret_cast<UINT32*>(pOutput->pLUT[ICA10IndexPerspective]);
    UINT64*           pGrid0                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA10IndexGrid0]);
    UINT64*           pGrid1                =
        reinterpret_cast<UINT64*>(pOutput->pLUT[ICA10IndexGrid1]);
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
        UINT  j = 0;
        UINT  k = 0;
        for (UINT i = 0; i < ICA10GridRegSize; i++)
        {
            if (0 == (i % 2))
            {
                pGrid0[j] = (((static_cast<UINT64>(pRegData->CTC_GRID_X[i])) << 17) |
                    (static_cast<UINT64>(pRegData->CTC_GRID_Y[i]) & 0x1FFFF)) & 0x3FFFFFFFF;
                j++;
            }
            else
            {
                pGrid1[k] = (((static_cast<UINT64>(pRegData->CTC_GRID_X[i])) << 17) |
                    (static_cast<UINT64>(pRegData->CTC_GRID_Y[i]) & 0x1FFFF)) & 0x3FFFFFFFF;
                k++;
            }
        }
    }

    // Assign Output data that needs to be stored by software
    pOutput->pCurrICAInData       = pUnpackedData->pCurrICAInData;
    pOutput->pPrevICAInData       = pUnpackedData->pPrevICAInData;
    pOutput->pCurrWarpAssistData  = pUnpackedData->pCurrWarpAssistData;
    pOutput->pPrevWarpAssistData  = pUnpackedData->pPrevWarpAssistData;
    pOutput->pWarpGeometryData    = pUnpackedData->pWarpGeometryData;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10Titan17x::~IPEICA10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA10Titan17x::~IPEICA10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10Titan17x::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
