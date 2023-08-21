////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeica10.cpp
/// @brief CAMXIPEICA10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxipeica10.h"
#include "camxcdmdefs.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "NcLibWarp.h"
#include "NcLibWarpCommonDef.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxipeicatestdata.h"
#include "camxipeica10titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult  result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEICA10* pModule = CAMX_NEW IPEICA10(pCreateData->pNodeIdentifier, pCreateData);

        if (NULL != pModule)
        {
            result = pModule->Initialize();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::InitializeModuleDataParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::InitializeModuleDataParameters()
{
    ICANcLibOutputData data;
    CamxResult  result = CamxResultSuccess;

    result = IQInterface::IPEICAGetInitializationData(&data);
    CAMX_ASSERT(CamxResultSuccess == result);

    m_moduleData.ICARegSize                                = data.ICARegSize;
    m_moduleData.ICAChromatixSize                          = data.ICAChromatixSize;
    m_moduleData.numLUTs                                   = ICA10Indexmax;
    m_moduleData.IPEPath                                   = m_moduleData.IPEPath;
    m_numLUT                                               = ICA10Indexmax;
    m_moduleData.pLUT[ICA10IndexPerspective]               = PERSPECTIVE_LUT;
    m_moduleData.pLUT[ICA10IndexGrid0]                     = GRID_LUT0;
    m_moduleData.pLUT[ICA10IndexGrid1]                     = GRID_LUT1;
    m_moduleData.offsetLUTCmdBuffer[ICA10IndexPerspective] = 0;
    m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid0]       = IPEICA10LUTSize[ICA10IndexPerspective];
    m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid1]       = IPEICA10LUTSize[ICA10IndexGrid0] +
                                                             IPEICA10LUTSize[ICA10IndexPerspective];
    m_moduleData.ICALUTBufferSize                          = IPEICA10LUTBufferSize;
    m_moduleData.ICAGridTransformHeight                    = ICA10GridTransformHeight;
    m_moduleData.ICAGridTransformWidth                     = ICA10GridTransformWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::Initialize()
{
    CamxResult          result       = CamxResultSuccess;

    InitializeModuleDataParameters();
    m_pHWSetting = NULL;

    switch (m_moduleData.titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan170:
            m_pHWSetting = CAMX_NEW IPEICA10Titan17x;
            break;

        default:
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid version %x", m_moduleData.titanVersion);
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "HW setting allocation failed ");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT i = 0; i < ICAReferenceNumber; i++)
        {
            // Allocate Warp Data for Input
            if ((CamxResultSuccess == result) && (INPUT == m_moduleData.IPEPath))
            {
                result = AllocateWarpData(&m_pICAInWarpData[i]);
            }
        }

        // Allocate Warp Data for REFERENCE
        if (CamxResultSuccess == result)
        {
            result = AllocateWarpData(&m_pICARefWarpData);
        }

        for (UINT i = 0; i < ICAReferenceNumber; i++)
        {
            // Allocate Warp Assist Data for Input
            if ((CamxResultSuccess == result) && (INPUT == m_moduleData.IPEPath))
            {
                result = AllocateWarpAssistData(&m_pWarpAssistData[i]);
            }
        }

        // Allocate Warp Geometric Output Data for Input
        if ((CamxResultSuccess == result) && (INPUT == m_moduleData.IPEPath))
        {
            result = AllocateWarpGeomOut(&m_pWarpGeometryData);
        }

        if (CamxResultSuccess == result)
        {
            // Assert size of each of these
            m_dependenceData.pNCRegData = CAMX_CALLOC(m_moduleData.ICARegSize);
            if (NULL == m_dependenceData.pNCRegData)
            {
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            m_dependenceData.pNCChromatix = CAMX_CALLOC(m_moduleData.ICAChromatixSize);
            if (NULL == m_dependenceData.pNCChromatix)
            {
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            m_dependenceData.pTempGridData = static_cast<NcLibWarpGridCoord*>
                (CAMX_CALLOC(sizeof(NcLibWarpGridCoord) *
                    m_moduleData.ICAGridTransformWidth * m_moduleData.ICAGridTransformHeight));
            if (NULL == m_dependenceData.pTempGridData)
            {
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            result = AllocateCommonLibraryData();
            if (result != CamxResultSuccess)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::FillCmdBufferManagerParams(
    const ISPInputData*     pInputData,
    IQModuleCmdBufferParam* pParam)
{
    CamxResult result                  = CamxResultSuccess;
    ResourceParams* pResourceParams    = NULL;
    CHAR*           pBufferManagerName = NULL;

    if ((NULL != pParam) && (NULL != pParam->pCmdBufManagerParam) && (NULL != pInputData))
    {
        // The Resource Params and Buffer Manager Name will be freed by caller Node
        pResourceParams = static_cast<ResourceParams*>(CAMX_CALLOC(sizeof(ResourceParams)));
        if (NULL != pResourceParams)
        {
            pBufferManagerName = static_cast<CHAR*>(CAMX_CALLOC((sizeof(CHAR) * MaxStringLength256)));
            if (NULL != pBufferManagerName)
            {
                OsUtils::SNPrintF(pBufferManagerName, (sizeof(CHAR) * MaxStringLength256), "CBM_%s_%s",
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEICA10");
                pResourceParams->resourceSize                 = m_moduleData.ICALUTBufferSize;
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess | CSLMemFlagHw;
                pResourceParams->pDeviceIndices               = pInputData->pipelineIPEData.pDeviceIndex;
                pResourceParams->numDevices                   = 1;

                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pBufferManagerName = pBufferManagerName;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pParams            = pResourceParams;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].ppCmdBufferManager = &m_pLUTCmdBufferManager;
                pParam->numberOfCmdBufManagers++;

            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_FREE(pResourceParams);
                pResourceParams = NULL;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize =
        (sizeof(ica_1_0_0::ica10_rgn_dataType) * (ICAMaxNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::UpdateIPEInternalData(
    ISPInputData* pInputData)
{
    CamxResult              result                  = CamxResultSuccess;
    IpeIQSettings*          pIPEIQSettings          =
        static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IpeFrameProcessData*    pFrameProcessData       =
        static_cast<IpeFrameProcessData*>(pInputData->pipelineIPEData.pFrameProcessData);
    IcaParameters*          pIcaParameters          =
        (REFERENCE == m_moduleData.IPEPath) ? &pIPEIQSettings->ica2Parameters : &pIPEIQSettings->ica1Parameters;

    if (TRUE == m_enableCommonIQ)
    {
        Utils::Memcpy(pIcaParameters->opgInterpolationCoefficients0,
            m_ICAParameter.opgInterpolationCoefficients0,
            sizeof(m_ICAParameter.opgInterpolationCoefficients0));
        Utils::Memcpy(pIcaParameters->opgInterpolationCoefficients1,
            m_ICAParameter.opgInterpolationCoefficients1,
            sizeof(m_ICAParameter.opgInterpolationCoefficients1));
        Utils::Memcpy(pIcaParameters->opgInterpolationCoefficients2,
            m_ICAParameter.opgInterpolationCoefficients2,
            sizeof(m_ICAParameter.opgInterpolationCoefficients2));

        pIcaParameters->eightBitOutputAlignment              = m_ICAParameter.eightBitOutputAlignment;
        pIcaParameters->invalidPixelModeConstCb              = m_ICAParameter.invalidPixelModeConstCb;
        pIcaParameters->invalidPixelModeConstCr              = m_ICAParameter.invalidPixelModeConstCr;
        pIcaParameters->invalidPixelModeConstY               = m_ICAParameter.invalidPixelModeConstY;
        pIcaParameters->invalidPixelModeInterpolationEnabled = m_ICAParameter.invalidPixelModeInterpolationEnabled;
        pIcaParameters->yInterpolationMode                   = m_ICAParameter.yInterpolationMode;
        pIcaParameters->isGridEnable                         = m_ICAParameter.isGridEnable;
        pIcaParameters->isPerspectiveEnable                  = m_ICAParameter.isPerspectiveEnable;
        pIcaParameters->perspGeomM                           = m_ICAParameter.perspGeomM;
        pIcaParameters->perspGeomN                           = m_ICAParameter.perspGeomN;
        pIcaParameters->shiftOnlyXQ16                        = m_ICAParameter.shiftOnlyXQ16;
        pIcaParameters->shiftOnlyYQ16                        = m_ICAParameter.shiftOnlyYQ16;
        pIcaParameters->gridTransformGeometry                = m_ICAParameter.gridTransformGeometry;

        if ((INPUT == m_moduleData.IPEPath) &&
            ((TRUE == m_ICAParameter.isPerspectiveEnable) ||
            (TRUE == m_ICAParameter.isGridEnable)))
        {

            // Update ISP input data with ICA1 output required for ICA2
            pInputData->ICAConfigData.pCurrICAInData      = m_dependenceData.pCurrICAInData;
            pInputData->ICAConfigData.pPrevICAInData      = m_dependenceData.pPrevICAInData;
            pInputData->ICAConfigData.pCurWarpAssistData  = m_dependenceData.pCurWarpAssistData;
            pInputData->ICAConfigData.pPrevWarpAssistData = m_dependenceData.pPrevWarpAssistData;
            // Update ISP input data with ICA1 output required for ANR TF
            pInputData->pipelineIPEData.pWarpGeometryData = m_pWarpGeometryData;
        }
    }
    else
    {
        pIcaParameters->eightBitOutputAlignment = 1;
        pIcaParameters->yInterpolationMode      = LUT_INTERPOLATION_TYPE;
        pIcaParameters->isGridEnable            = 0;
        pIcaParameters->isPerspectiveEnable     = 1;

        pIcaParameters->invalidPixelModeInterpolationEnabled = 1;
        pIcaParameters->invalidPixelModeConstY               = 0;
        pIcaParameters->invalidPixelModeConstCb              = 0x200;
        pIcaParameters->invalidPixelModeConstCr              = 0x200;

        if (1 == pIcaParameters->isPerspectiveEnable)
        {
            pIcaParameters->perspGeomM = 1;
            pIcaParameters->perspGeomN = 1;
        }

        pIcaParameters->shiftOnlyXQ16 = 0;
        pIcaParameters->shiftOnlyYQ16 = 0;

        pIcaParameters->opgInterpolationCoefficients0[0] = 0x0;
        pIcaParameters->opgInterpolationCoefficients0[1] = 0x2fc4;
        pIcaParameters->opgInterpolationCoefficients0[2] = 0x3F8F;
        pIcaParameters->opgInterpolationCoefficients0[3] = 0x3F62;
        pIcaParameters->opgInterpolationCoefficients0[4] = 0x3F3C;
        pIcaParameters->opgInterpolationCoefficients0[5] = 0x3F1C;
        pIcaParameters->opgInterpolationCoefficients0[6] = 0x3F02;
        pIcaParameters->opgInterpolationCoefficients0[7] = 0x3EEF;
        pIcaParameters->opgInterpolationCoefficients0[8] = 0x3EE0;
        pIcaParameters->opgInterpolationCoefficients0[9] = 0x3ED6;
        pIcaParameters->opgInterpolationCoefficients0[10] = 0x3ED1;
        pIcaParameters->opgInterpolationCoefficients0[11] = 0x3ED1;
        pIcaParameters->opgInterpolationCoefficients0[12] = 0x3ED4;
        pIcaParameters->opgInterpolationCoefficients0[13] = 0x3EDB;
        pIcaParameters->opgInterpolationCoefficients0[14] = 0x3EE4;
        pIcaParameters->opgInterpolationCoefficients0[15] = 0x3EF1;


        pIcaParameters->opgInterpolationCoefficients1[0] = 0x1200;
        pIcaParameters->opgInterpolationCoefficients1[1] = 0x1FED;
        pIcaParameters->opgInterpolationCoefficients1[2] = 0x1FB5;
        pIcaParameters->opgInterpolationCoefficients1[3] = 0x1F57;
        pIcaParameters->opgInterpolationCoefficients1[4] = 0x1ED9;
        pIcaParameters->opgInterpolationCoefficients1[5] = 0x1E3B;
        pIcaParameters->opgInterpolationCoefficients1[6] = 0x1D83;
        pIcaParameters->opgInterpolationCoefficients1[7] = 0x1CAD;
        pIcaParameters->opgInterpolationCoefficients1[8] = 0x1BC1;
        pIcaParameters->opgInterpolationCoefficients1[9] = 0x1ABF;
        pIcaParameters->opgInterpolationCoefficients1[10] = 0x19A9;
        pIcaParameters->opgInterpolationCoefficients1[11] = 0x1881;
        pIcaParameters->opgInterpolationCoefficients1[12] = 0x1749;
        pIcaParameters->opgInterpolationCoefficients1[13] = 0x1605;
        pIcaParameters->opgInterpolationCoefficients1[14] = 0x14B7;
        pIcaParameters->opgInterpolationCoefficients1[15] = 0x135F;


        pIcaParameters->opgInterpolationCoefficients2[0] = 0x3F00;
        pIcaParameters->opgInterpolationCoefficients2[1] = 0x0048;
        pIcaParameters->opgInterpolationCoefficients2[2] = 0x009F;
        pIcaParameters->opgInterpolationCoefficients2[3] = 0x0103;
        pIcaParameters->opgInterpolationCoefficients2[4] = 0x0174;
        pIcaParameters->opgInterpolationCoefficients2[5] = 0x01F1;
        pIcaParameters->opgInterpolationCoefficients2[6] = 0x0278;
        pIcaParameters->opgInterpolationCoefficients2[7] = 0x0308;
        pIcaParameters->opgInterpolationCoefficients2[8] = 0x03A0;
        pIcaParameters->opgInterpolationCoefficients2[9] = 0x043F;
        pIcaParameters->opgInterpolationCoefficients2[10] = 0x04E5;
        pIcaParameters->opgInterpolationCoefficients2[11] = 0x058E;
        pIcaParameters->opgInterpolationCoefficients2[12] = 0x063C;
        pIcaParameters->opgInterpolationCoefficients2[13] = 0x06EC;
        pIcaParameters->opgInterpolationCoefficients2[14] = 0x079E;
        pIcaParameters->opgInterpolationCoefficients2[15] = 0x084f;

        CAMX_ASSERT(NULL != pFrameProcessData);

    }

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pIcaParameters->opgInterpolationCoefficients0[2] : %d",
        pIcaParameters->opgInterpolationCoefficients0[2]);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pIcaParameters->opgInterpolationCoefficients0[3] : %d",
        pIcaParameters->opgInterpolationCoefficients0[3]);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "pIcaParameters->opgInterpolationCoefficients0[4] : %d",
        pIcaParameters->opgInterpolationCoefficients0[4]);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "titver %x,path %d,Ealign %d,mode %d,y %d,cb %d,cr %d,inv %d,grid %d,p %d,m %d,n %d",
                     m_moduleData.titanVersion,
                     m_moduleData.IPEPath,
                     pIcaParameters->eightBitOutputAlignment,
                     pIcaParameters->yInterpolationMode,
                     pIcaParameters->invalidPixelModeConstY,
                     pIcaParameters->invalidPixelModeConstCb,
                     pIcaParameters->invalidPixelModeConstCr,
                     pIcaParameters->invalidPixelModeInterpolationEnabled,
                     pIcaParameters->isGridEnable,
                     pIcaParameters->isPerspectiveEnable,
                     pIcaParameters->perspGeomM,
                     pIcaParameters->perspGeomN);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IPEICA10::GetRegCmd()

{
    return m_dependenceData.pNCRegData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::CheckGridDependencyChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEICA10::CheckGridDependencyChange(
    ISPInputData* pInputData)
{
    BOOL  isChanged      = FALSE;
    BOOL  gridInChanged  = 0;
    BOOL  gridRefChanged = 0;

    // Grid Input enabled by flow
    gridInChanged =
        ((FALSE == pInputData->ICAConfigData.ICAInGridParams.reuseGridTransform) &&
        (TRUE == pInputData->ICAConfigData.ICAInGridParams.gridTransformEnable)) ?
        TRUE : FALSE;

    // Grid Reference enabled by flow
    gridRefChanged =
        ((FALSE == pInputData->ICAConfigData.ICARefGridParams.reuseGridTransform) &&
        (TRUE == pInputData->ICAConfigData.ICARefGridParams.gridTransformEnable)) ?
        TRUE : FALSE;

    if ((TRUE == gridInChanged) ||
        (TRUE == gridRefChanged))
    {
        isChanged = TRUE;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " grid changed %d", isChanged);
    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::CheckPerspectiveDependencyChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEICA10::CheckPerspectiveDependencyChange(
    ISPInputData* pInputData)
{
    BOOL isChanged                  = FALSE;
    BOOL perspectiveInChange        = 0;
    BOOL perspectiveRefChange       = 0;
    BOOL perspectiveAlignmentChange = 0;

    perspectiveInChange =
        ((TRUE == pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable) &&
        (FALSE == pInputData->ICAConfigData.ICAInPerspectiveParams.ReusePerspectiveTransform)) ?
        TRUE : FALSE;

    perspectiveAlignmentChange =
        ((TRUE == pInputData->ICAConfigData.ICAReferenceParams.perspectiveTransformEnable) &&
        (FALSE == pInputData->ICAConfigData.ICAReferenceParams.ReusePerspectiveTransform)) ?
        TRUE : FALSE;

    perspectiveRefChange =
        ((TRUE == pInputData->ICAConfigData.ICARefPerspectiveParams.perspectiveTransformEnable) &&
        (FALSE == pInputData->ICAConfigData.ICARefPerspectiveParams.ReusePerspectiveTransform)) ?
        TRUE : FALSE;

    if ((TRUE == perspectiveRefChange) ||
        (TRUE == perspectiveAlignmentChange) ||
        (TRUE == perspectiveInChange))
    {
        isChanged = TRUE;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " perspective changed %d", isChanged);
    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::SetModuleEnable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::SetModuleEnable(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    m_moduleEnable    = TRUE;
    CAMX_UNREFERENCED_PARAM(pInputData);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " m_moduleEnable %d", m_moduleEnable);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::CheckAndUpdateChromatixData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEICA10::CheckAndUpdateChromatixData(
    ISPInputData* pInputData)
{
    BOOL isChromatixChanged = FALSE;
    TuningDataManager* pTuningManager = pInputData->pTuningDataManager;

    if ((NULL != pTuningManager) &&
        (TRUE == pTuningManager->IsValidChromatix()))
    {
        CAMX_ASSERT(NULL != pInputData->pTuningData);

        // Search through the tuning data (tree), only when there
        // are changes to the tuning mode data as an optimization
        if (TRUE == pInputData->tuningModeChanged)
        {

            m_moduleData.pChromatix = (REFERENCE == m_moduleData.IPEPath) ?
                pTuningManager->GetChromatix()->GetModule_ica10_ipe_module2(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter) :
                pTuningManager->GetChromatix()->GetModule_ica10_ipe_module1(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);

            CAMX_ASSERT(NULL != m_moduleData.pChromatix);

            if (m_moduleData.pChromatix !=
                static_cast<ica_1_0_0::chromatix_ica10Type*>(m_dependenceData.pChromatix))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                m_dependenceData.pChromatix = m_moduleData.pChromatix;
                isChromatixChanged = TRUE;
            }
        }
    }
    return isChromatixChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEICA10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL           isChanged  = FALSE;
    IcaParameters* pParameter = NULL;

    CAMX_ASSERT(NULL != pInputData);

    if ((NULL != pInputData) &&
        (NULL != pInputData->pAECUpdateData))
    {
        AECFrameControl* pNewAECUpdate = pInputData->pAECUpdateData;

        pParameter = (REFERENCE == m_moduleData.IPEPath) ?
            &(static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings)->ica2Parameters) :
            &(static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings)->ica1Parameters);

        if ((NULL != pNewAECUpdate) && (NULL != pInputData->pHwContext))
        {
            isChanged = CheckAndUpdateChromatixData(pInputData);
            SetModuleEnable(pInputData);

            if (TRUE == m_moduleEnable)
            {
                if ((FALSE == Utils::FEqual(m_dependenceData.luxIndex, pNewAECUpdate->luxIndex)) ||
                    (FALSE == Utils::FEqual(m_dependenceData.digitalGain,
                        pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain))              ||
                    (FALSE == Utils::FEqual(m_dependenceData.lensPosition,
                        pInputData->lensPosition))                                               ||
                    (FALSE == Utils::FEqual(m_dependenceData.lensZoomRatio,
                        pInputData->lensZoom))                                                   ||
                    (FALSE == CheckPerspectiveDependencyChange(pInputData))                      ||
                    (FALSE == CheckGridDependencyChange(pInputData))                             ||
                    (pInputData->frameNum != m_dependenceData.frameNum))
                {
                    m_dependenceData.luxIndex                = pNewAECUpdate->luxIndex;
                    m_dependenceData.digitalGain             = pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain;
                    m_dependenceData.lensPosition            = pInputData->lensPosition;
                    m_dependenceData.lensZoomRatio           = pInputData->lensZoom;
                    m_dependenceData.pZoomWindow             = &pParameter->zoomWindow;
                    m_dependenceData.pIFEZoomWindow          = &pParameter->ifeZoomWindow;
                    m_dependenceData.pImageDimensions        = &pInputData->pipelineIPEData.inputDimension;
                    m_dependenceData.pFullImageDimensions    = &pInputData->pipelineIPEData.fullInputDimension;
                    m_dependenceData.pMarginDimensions       = &pInputData->pipelineIPEData.marginDimension;
                    m_dependenceData.IPEPath                 = m_moduleData.IPEPath;
                    m_dependenceData.pInterpolationParamters = (REFERENCE == m_moduleData.IPEPath) ?
                        &pInputData->ICAConfigData.ICARefInterpolationParams :
                        &pInputData->ICAConfigData.ICAInInterpolationParams;
                    m_dependenceData.ICAMode                 = pInputData->pipelineIPEData.ICAMode;
                    m_dependenceData.icaVersion              = pInputData->icaVersion;
                    m_dependenceData.frameNum                = pInputData->frameNum;
                    m_dependenceData.dumpICAOut              = m_dumpICAOutput;
                    m_dependenceData.pFDData                 = &pInputData->fDData;
                    m_dependenceData.instanceID              = pInputData->pipelineIPEData.instanceID;
                    m_dependenceData.opticalCenterX          = pInputData->opticalCenterX;
                    m_dependenceData.opticalCenterY          = pInputData->opticalCenterY;

                    // Set all the dependency data required for transforms
                    SetTransformData(pInputData);
                    isChanged = TRUE;
                }
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Invalid Input: pNewAECUpdate %x HwContext %x",
                pNewAECUpdate, pInputData->pHwContext);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Lux Index [0x%f] path %d, changed %d, frameNum %llu mctf_eis %d",
                         pInputData->pAECUpdateData->luxIndex, m_moduleData.IPEPath, isChanged,
                         pInputData->frameNum, m_dependenceData.ICAMode);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null input pointer");
        isChanged = FALSE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::UpdateLUTFromChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::UpdateLUTFromChromatix(
    VOID* pLUT)
{
    CAMX_ASSERT(NULL != pLUT);

    BYTE*  pLookupTable = static_cast<BYTE*>(pLUT);
    CamX::Utils::Memcpy(pLookupTable, m_moduleData.pLUT[ICA10IndexPerspective],
        IPEICA10LUTSize[ICA10IndexPerspective]);
    pLookupTable += IPEICA10LUTSize[ICA10IndexPerspective];
    CamX::Utils::Memcpy(pLookupTable, m_moduleData.pLUT[ICA10IndexGrid0],
        IPEICA10LUTSize[ICA10IndexGrid0]);
    pLookupTable += IPEICA10LUTSize[ICA10IndexGrid0];
    CamX::Utils::Memcpy(pLookupTable, m_moduleData.pLUT[ICA10IndexGrid1],
        IPEICA10LUTSize[ICA10IndexGrid1]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::DumpTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::DumpTuningMetadata(
    ISPInputData*   pInputData,
    UINT32*         pLUT)
{
    CamxResult       result = CamxResultSuccess;
    if (NULL != pInputData->pIPETuningMetadata)
    {
        IPETuningICAPath TuningICAPath = (m_moduleData.IPEPath == INPUT) ? TuningICAInput : TuningICAReference;
        if (m_moduleData.ICALUTBufferSize <=
            sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.ICALUT[TuningICAPath]))
        {
            if (TRUE == m_ICAParameter.isPerspectiveEnable)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->
                    IPETuningMetadata17x.IPEDMIData.ICALUT[TuningICAPath].PerspectiveLUT,
                    (reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexPerspective]),
                    IPEICA10LUTSize[ICA10IndexPerspective]);
            }
            if (TRUE == m_ICAParameter.isGridEnable)
            {
                Utils::Memcpy(
                    &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.ICALUT[TuningICAPath].grid0LUT,
                    (reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid0]),
                    IPEICA10LUTSize[ICA10IndexGrid0]);

                Utils::Memcpy(
                    &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.ICALUT[TuningICAPath].grid1LUT,
                    (reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid1]),
                    IPEICA10LUTSize[ICA10IndexGrid1]);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning data, incorrect LUT buffer size");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult       result          = CamxResultSuccess;
    PacketResource*  pPacketResource = NULL;
    ICAOutputData  outputData;

    Utils::Memset(&outputData, 0x0, sizeof(ICAOutputData));

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
            m_pLUTCmdBuffer = NULL;
        }

        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Cmd Buffer is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
        UINT32* pLUT    =
            reinterpret_cast<UINT32*>(
                m_pLUTCmdBuffer->BeginCommands((m_moduleData.ICALUTBufferSize / RegisterWidthInBytes)));

        if (NULL != pLUT)
        {
            outputData.pICAParameter     = &m_ICAParameter;
            outputData.pWarpGeometryData = m_pWarpGeometryData;

            for (UINT count = 0; count < m_numLUT; count++)
            {
                outputData.pLUT[count] = reinterpret_cast<UINT32*>(
                    reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[count]);
            }

            if (TRUE == m_enableCommonIQ)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Lux Index [0x%f] version %x ",
                    pInputData->pAECUpdateData->luxIndex,
                    m_moduleData.titanVersion);
                result = IQInterface::ICA10CalculateSetting(&m_dependenceData, &outputData);
            }
            else
            {
                UpdateLUTFromChromatix(pLUT);
            }

            m_pICAPerspectiveLUT = reinterpret_cast<UINT32*>(
                    reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexPerspective]);
            m_pICAGrid0LUT       = reinterpret_cast<UINT32*>(
                    reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid0]);
            m_pICAGrid1LUT       = reinterpret_cast<UINT32*>(
                    reinterpret_cast<UCHAR*>(pLUT) + m_moduleData.offsetLUTCmdBuffer[ICA10IndexGrid1]);
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "pLUT is NULL");
        }

        if (CamxResultSuccess == result)
        {
            result = m_pLUTCmdBuffer->CommitCommands();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependency is published and valid
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                ICA10HWSettingParams hwSetting;
                CmdBuffer*           pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
                m_offsetLUT                        = pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32);
                hwSetting.pLUTCmdBuffer            = m_pLUTCmdBuffer;
                hwSetting.pModuleData              = &m_moduleData;
                result = m_pHWSetting->CreateCmdList(pInputData, reinterpret_cast<UINT32*>(&hwSetting));
            }
            if (CamxResultSuccess == result)
            {
                result = UpdateIPEInternalData(pInputData);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::GetModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::GetModuleData(
    VOID* pModuleData)
{
    CAMX_ASSERT(NULL != pModuleData);
    IPEIQModuleData* pData = reinterpret_cast<IPEIQModuleData*>(pModuleData);

    // data is expected to be filled after execute
    pData->IPEPath = m_moduleData.IPEPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::IPEICA10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA10::IPEICA10(
    const CHAR*          pNodeIdentifier,
    IPEModuleCreateData* pCreateData)
{
    m_pNodeIdentifier         = pNodeIdentifier;
    m_type                    = ISPIQModuleType::IPEICA;
    m_moduleEnable            = TRUE;
    m_cmdLength               = 0;
    m_numLUT                  = 0;
    m_pLUTCmdBuffer           = NULL;
    m_32bitDMILength          = 0;
    m_64bitDMILength          = 0;
    m_pWarpGeometryData       = NULL;
    m_moduleData.titanVersion = 0;
    m_moduleData.hwVersion    = 0;
    Utils::Memset(&m_dependenceData, 0x0, sizeof(m_dependenceData));
    Utils::Memset(&m_ICAParameter, 0x0, sizeof(m_ICAParameter));

    // Set to 1 always so IQ settings is configured even when no grid/ perspective
    m_ICAParameter.eightBitOutputAlignment              = 1;
    m_ICAParameter.invalidPixelModeInterpolationEnabled = 1;

    if (TRUE == pCreateData->initializationData.registerBETEn)
    {
        m_enableCommonIQ = TRUE;
        m_dumpICAOutput  = FALSE;
        m_moduleData.IPEPath          = pCreateData->path;
        m_moduleData.titanVersion     = CSLCameraTitanVersion::CSLTitan170;
        m_dependenceData.titanVersion = CSLCameraTitanVersion::CSLTitan170;

    }
    else
    {
        Titan17xContext* pContext     = static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext);
        m_enableCommonIQ              =
            pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableICACommonIQModule;
        m_dumpICAOutput               =
            pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->dumpICAOut;
        m_moduleData.titanVersion     = static_cast<Titan17xContext *>(pContext)->GetTitanVersion();
        m_moduleData.hwVersion        = static_cast<Titan17xContext *>(pContext)->GetHwVersion();
        m_moduleData.IPEPath          = pCreateData->path;
        m_dependenceData.hwVersion    = m_moduleData.hwVersion;
        m_dependenceData.titanVersion = m_moduleData.titanVersion;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IPE ICA m_numLUT %d , m_EnableCommonIQ %d",
        m_numLUT, m_enableCommonIQ);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::AllocateWarpData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::AllocateWarpData(
    VOID** ppWarpData)
{
    CamxResult result = CamxResultSuccess;
    NcLibWarp* pWarp = NULL;

    pWarp = CAMX_NEW NcLibWarp;

    if (NULL == pWarp)
    {
        result = CamxResultENoMemory;
    }

    // Allocate perspective matrices structure for frame N
    if (CamxResultSuccess == result)
    {
        pWarp->matrices.perspMatrices = CAMX_NEW NcLibPerspTransformSingle[9];
        if (NULL == pWarp->matrices.perspMatrices)
        {
            result = CamxResultENoMemory;
        }
    }

    // Allocate grid matrices structure for frame N - 1
    if (CamxResultSuccess == result)
    {
        pWarp->grid.grid = static_cast<NcLibWarpGridCoord*>
            (CAMX_CALLOC(sizeof(NcLibWarpGridCoord) *
                m_moduleData.ICAGridTransformWidth * m_moduleData.ICAGridTransformHeight));
        if (NULL == pWarp->grid.grid)
        {
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        pWarp->grid.gridExtrapolate =
            static_cast<NcLibWarpGridCoord*>(CAMX_CALLOC(sizeof(NcLibWarpGridCoord) * 4));
        if (NULL == pWarp->grid.gridExtrapolate)
        {
            result = CamxResultENoMemory;
        }
    }
    *ppWarpData = static_cast<NcLibWarp*>(pWarp);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::DeAllocateWarpData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::DeAllocateWarpData(
    VOID** ppWarpData)
{
    NcLibWarp* pWarp = static_cast<NcLibWarp*>(*ppWarpData);

    if (NULL != pWarp->matrices.perspMatrices)
    {
        CAMX_DELETE [] pWarp->matrices.perspMatrices;
        pWarp->matrices.perspMatrices = NULL;
    }

    if (NULL != pWarp->grid.grid)
    {
        CAMX_FREE(pWarp->grid.grid);
        pWarp->grid.grid = NULL;
    }

    if (NULL != pWarp->grid.gridExtrapolate)
    {
        CAMX_FREE(pWarp->grid.gridExtrapolate);
        pWarp->grid.gridExtrapolate = NULL;
    }

    CAMX_DELETE pWarp;
    pWarp = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEICA10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::AllocateWarpAssistData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::AllocateWarpAssistData(
    VOID** ppWarpAssistData)
{
    CamxResult                   result      = CamxResultSuccess;
    NcLibWarpBuildAssistGridOut* pWarpAssist = NULL;

    pWarpAssist = CAMX_NEW NcLibWarpBuildAssistGridOut;

    if (NULL == pWarpAssist)
    {
        result = CamxResultENoMemory;
    }

    // Allocate perspective matrices structure for frame N
    if (CamxResultSuccess == result)
    {
        pWarpAssist->inputWarp = NULL; // this is assigned by user
        pWarpAssist->assistGrid = CAMX_NEW NcLibWarpGrid;
        if (NULL == pWarpAssist->assistGrid)
        {
            result = CamxResultENoMemory;
        }
    }

    // Allocate grid matrices structure for frame N - 1
    if (CamxResultSuccess == result)
    {
        pWarpAssist->assistGrid->grid = static_cast<NcLibWarpGridCoord*>
            (CAMX_CALLOC(sizeof(NcLibWarpGridCoord) *
             m_moduleData.ICAGridTransformHeight * m_moduleData.ICAGridTransformWidth));
        if (NULL == pWarpAssist->assistGrid->grid)
        {
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        pWarpAssist->assistGrid->gridExtrapolate =
            static_cast<NcLibWarpGridCoord*>(CAMX_CALLOC(sizeof(NcLibWarpGridCoord) * 4));
        if (NULL == pWarpAssist->assistGrid->gridExtrapolate)
        {
            result = CamxResultENoMemory;
        }
    }

    *ppWarpAssistData = static_cast<NcLibWarpBuildAssistGridOut*>(pWarpAssist);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::DeAllocateWarpAssistData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::DeAllocateWarpAssistData(
    VOID** ppWarpAssistData)
{
    NcLibWarpBuildAssistGridOut* pWarpAssist =
        static_cast<NcLibWarpBuildAssistGridOut*>(*ppWarpAssistData);

    if (NULL != pWarpAssist)
    {
        if (NULL != pWarpAssist->assistGrid)
        {
            if (NULL != pWarpAssist->assistGrid->grid)
            {
                CAMX_FREE(pWarpAssist->assistGrid->grid);
                pWarpAssist->assistGrid->grid = NULL;
            }

            if (NULL != pWarpAssist->assistGrid->gridExtrapolate)
            {
                CAMX_FREE(pWarpAssist->assistGrid->gridExtrapolate);
                pWarpAssist->assistGrid->gridExtrapolate = NULL;
            }

            CAMX_DELETE(pWarpAssist->assistGrid);
            pWarpAssist->assistGrid = NULL;
        }

        CAMX_DELETE(pWarpAssist);
        pWarpAssist = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::AllocateWarpGeomOut
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::AllocateWarpGeomOut(
    VOID** ppWarpGeomOut)
{
    CamxResult        result = CamxResultSuccess;
    NcLibWarpGeomOut* pWarpGeomOut = NULL;

    pWarpGeomOut = CAMX_NEW NcLibWarpGeomOut;
    if (NULL == pWarpGeomOut)
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        pWarpGeomOut->fdConfig = CAMX_NEW FD_CONFIG_CONTEXT;
        if (NULL == pWarpGeomOut->fdConfig)
        {
            result = CamxResultENoMemory;
        }
    }

    *ppWarpGeomOut = static_cast<NcLibWarpGeomOut*>(pWarpGeomOut);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::DeAllocateWarpGeomOut
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEICA10::DeAllocateWarpGeomOut(
    VOID** ppWarpGeomOut)
{
    NcLibWarpGeomOut*   pWarpGeomOut = static_cast<NcLibWarpGeomOut*>(*ppWarpGeomOut);

    if (NULL != pWarpGeomOut)
    {
        if (NULL != pWarpGeomOut->fdConfig)
        {
            CAMX_DELETE(pWarpGeomOut->fdConfig);
            pWarpGeomOut->fdConfig = NULL;
        }

        CAMX_DELETE(pWarpGeomOut);
        pWarpGeomOut = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::~IPEICA10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICA10::~IPEICA10()
{
    if (INPUT == m_moduleData.IPEPath)
    {
        for (UINT i = 0; i < ICAReferenceNumber; i++)
        {
            if (NULL != m_pICAInWarpData[i])
            {
                DeAllocateWarpData(&m_pICAInWarpData[i]);
            }

            if (NULL != m_pWarpAssistData[i])
            {
                DeAllocateWarpAssistData(&m_pWarpAssistData[i]);
            }
        }

        DeAllocateWarpGeomOut(&m_pWarpGeometryData);
    }

    if (NULL != m_pICARefWarpData)
    {
        DeAllocateWarpData(&m_pICARefWarpData);
    }

    if (NULL != m_dependenceData.pNCChromatix)
    {
        CAMX_FREE(m_dependenceData.pNCChromatix);
        m_dependenceData.pNCChromatix = NULL;
    }

    if (NULL != m_dependenceData.pTempGridData)
    {
        CAMX_FREE(m_dependenceData.pTempGridData);
        m_dependenceData.pTempGridData = NULL;
    }

    if (NULL != m_dependenceData.pNCRegData)
    {
        CAMX_FREE(m_dependenceData.pNCRegData);
        m_dependenceData.pNCRegData = NULL;
    }

    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    DeallocateCommonLibraryData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::UpdatePerspectiveParamsToContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::UpdatePerspectiveParamsToContext(
    VOID* pWrap,
    VOID* pICAPerspectiveParams)
{
    CamxResult                  result                =
        CamxResultSuccess;
    NcLibWarp*                  pCurWarp              =
        static_cast<NcLibWarp*>(pWrap);
    IPEICAPerspectiveTransform* pPerspectiveTransform =
        static_cast<IPEICAPerspectiveTransform *>(pICAPerspectiveParams);

    if (NULL != pCurWarp)
    {
        pCurWarp->direction = static_cast<NcLibWarpDirection>(0);

        if (TRUE == IsPerspectiveEnabled(pPerspectiveTransform))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " path %d, perspective %d, column %d, row %d, width %d, height %d",
                m_moduleData.IPEPath, pPerspectiveTransform->perspectiveTransformEnable,
                pPerspectiveTransform->perspetiveGeometryNumColumns,
                pPerspectiveTransform->perspectiveGeometryNumRows,
                pPerspectiveTransform->transformDefinedOnWidth,
                pPerspectiveTransform->transformDefinedOnHeight);

            pCurWarp->matrices.enable = TRUE;
            pCurWarp->matrices.numColumns                     = pPerspectiveTransform->perspetiveGeometryNumColumns;
            pCurWarp->matrices.numRows                        = pPerspectiveTransform->perspectiveGeometryNumRows;
            pCurWarp->matrices.transformDefinedOn.widthPixels = pPerspectiveTransform->transformDefinedOnWidth;
            pCurWarp->matrices.transformDefinedOn.heightLines = pPerspectiveTransform->transformDefinedOnHeight;
            pCurWarp->matrices.confidence                     = pPerspectiveTransform->perspectiveConfidence;
            // Always Centered for ICA
            pCurWarp->matrices.centerType                     = CENTERED;

            for (UINT i = 0; i < pCurWarp->matrices.numRows * pCurWarp->matrices.numColumns; i++)
            {
                Utils::Memcpy(pCurWarp->matrices.perspMatrices[i].T,
                    pPerspectiveTransform->perspectiveTransformArray + (i * ICAParametersPerPerspectiveTransform),
                    sizeof(float) * ICAParametersPerPerspectiveTransform);
            }
        }
        else
        {
            pCurWarp->matrices.enable = FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Perspective not enabled for path %d", m_moduleData.IPEPath);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid warp data for path %d", m_moduleData.IPEPath);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::SetGridTransformGeometry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::SetGridTransformGeometry(
    VOID* pWrap,
    VOID* pICAGridParams)
{
    CamxResult           result = CamxResultSuccess;
    NcLibWarp*           pCurWarp = static_cast<NcLibWarp*>(pWrap);
    IPEICAGridTransform* pGridTransform =
        static_cast<IPEICAGridTransform *>(pICAGridParams);
    pCurWarp->grid.numRows    = ICA10GridTransformHeight;
    pCurWarp->grid.numColumns = ICA10GridTransformWidth;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::UpdateGridParamsToContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::UpdateGridParamsToContext(
    VOID* pWrap,
    VOID* pICAGridParams)
{
    CamxResult           result         = CamxResultSuccess;
    NcLibWarp*           pCurWarp       = static_cast<NcLibWarp*>(pWrap);
    IPEICAGridTransform* pGridTransform =
        static_cast<IPEICAGridTransform *>(pICAGridParams);

    if (NULL != pCurWarp)
    {
        if (TRUE == IsGridEnabled(pGridTransform))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "path %d, grid %d", m_moduleData.IPEPath, pGridTransform->gridTransformEnable);

            pCurWarp->grid.enable                         = TRUE;
            result = SetGridTransformGeometry(pWrap, pICAGridParams);
            if (CamxResultSuccess == result)
            {
                pCurWarp->grid.transformDefinedOn.widthPixels = pGridTransform->transformDefinedOnWidth;
                pCurWarp->grid.transformDefinedOn.heightLines = pGridTransform->transformDefinedOnHeight;

                for (UINT i = 0; i < (pCurWarp->grid.numRows * pCurWarp->grid.numColumns); i++)
                {
                    pCurWarp->grid.grid[i].x = pGridTransform->gridTransformArray[i].x;
                    pCurWarp->grid.grid[i].y = pGridTransform->gridTransformArray[i].y;
                }

                pCurWarp->grid.extrapolateType = EXTRAPOLATION_TYPE_NONE;
                if (TRUE == pGridTransform->gridTransformArrayExtrapolatedCorners)
                {
                    pCurWarp->grid.extrapolateType = EXTRAPOLATION_TYPE_FOUR_CORNERS;
                    for (UINT i = 0; i < GridExtarpolateCornerSize; i++)
                    {
                        pCurWarp->grid.gridExtrapolate[i].x = pGridTransform->gridTransformArrayCorners[i].x;
                        pCurWarp->grid.gridExtrapolate[i].y = pGridTransform->gridTransformArrayCorners[i].y;
                    }
                }
            }
        }
        else
        {
            pCurWarp->grid.enable = FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Grid not enabled for path %d", m_moduleData.IPEPath);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid warp data for path %d", m_moduleData.IPEPath);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::SetTransformData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::SetTransformData(
    ISPInputData* pISPInputData)
{
    CamxResult  result = CamxResultSuccess;
    if (REFERENCE == m_moduleData.IPEPath)
    {
        m_dependenceData.pCurrICARefData                  = m_pICARefWarpData;
        m_dependenceData.pPrevWarpAssistData              = pISPInputData->ICAConfigData.pPrevWarpAssistData;
        m_dependenceData.pCurWarpAssistData               = pISPInputData->ICAConfigData.pCurWarpAssistData;
        m_dependenceData.pCurrICAInData                   = pISPInputData->ICAConfigData.pCurrICAInData;
        m_dependenceData.pPrevICAInData                   = pISPInputData->ICAConfigData.pPrevICAInData;
        m_dependenceData.byPassAlignmentMatrixAdjustement = FALSE;

        if (TRUE != pISPInputData->registerBETEn)
        {
            if ((0 != (IPEStabilizationMCTF & pISPInputData->pipelineIPEData.instanceProperty.stabilizationType)) &&
                (pISPInputData->pHwContext->GetStaticSettings()->enableMCTF))
            {
                UpdatePerspectiveParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICAReferenceParams);
                m_dependenceData.byPassAlignmentMatrixAdjustement =
                                       pISPInputData->ICAConfigData.ICAReferenceParams.byPassAlignmentMatrixAdjustement;
                // Update unity matrix for ICA
            }
            else
            {
                UpdatePerspectiveParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICARefPerspectiveParams);
                UpdateGridParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICARefGridParams);
            }
        }
        else
        {
            if (0 != (IPEStabilizationMCTF & pISPInputData->pipelineIPEData.instanceProperty.stabilizationType))
            {
                UpdatePerspectiveParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICAReferenceParams);
                m_dependenceData.byPassAlignmentMatrixAdjustement =
                                       pISPInputData->ICAConfigData.ICAReferenceParams.byPassAlignmentMatrixAdjustement;
                // Update unity matrix for ICA
            }
            else
            {
                UpdatePerspectiveParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICARefPerspectiveParams);
                UpdateGridParamsToContext(
                    m_dependenceData.pCurrICARefData,
                    &pISPInputData->ICAConfigData.ICARefGridParams);
            }
        }
    }
    else
    {
        m_dependenceData.pCurrICARefData     = m_pICARefWarpData;

        m_dependenceData.pCurrICAInData      =
            (pISPInputData->frameNum % 2) ? m_pICAInWarpData[0] : m_pICAInWarpData[1];
        m_dependenceData.pPrevICAInData      =
            (pISPInputData->frameNum % 2) ? m_pICAInWarpData[1] : m_pICAInWarpData[0];
        m_dependenceData.pCurWarpAssistData  =
            (pISPInputData->frameNum % 2) ? m_pWarpAssistData[0] : m_pWarpAssistData[1];
        m_dependenceData.pPrevWarpAssistData =
            (pISPInputData->frameNum % 2) ? m_pWarpAssistData[1] : m_pWarpAssistData[0];
        m_dependenceData.pWarpGeomOut = m_pWarpGeometryData;
        // Add FD data
        UpdatePerspectiveParamsToContext(
            m_dependenceData.pCurrICAInData,
            &pISPInputData->ICAConfigData.ICAInPerspectiveParams);
        UpdateGridParamsToContext(
            m_dependenceData.pCurrICAInData,
            &pISPInputData->ICAConfigData.ICAInGridParams);
        if (0 != (IPEStabilizationMCTF & pISPInputData->pipelineIPEData.instanceProperty.stabilizationType))
        {
            UpdatePerspectiveParamsToContext(
                m_dependenceData.pCurrICARefData,
                &pISPInputData->ICAConfigData.ICAReferenceParams);
            m_dependenceData.byPassAlignmentMatrixAdjustement =
                pISPInputData->ICAConfigData.ICAReferenceParams.byPassAlignmentMatrixAdjustement;
        }

    }

    m_dependenceData.isGridFromChromatixEnabled = pISPInputData->pHwContext->GetStaticSettings()->isGridFromChromatixEnabled;

    DumpInputConfiguration(pISPInputData, &m_dependenceData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEICA10::DumpInputConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEICA10::DumpInputConfiguration(
    ISPInputData*  pInputData,
    ICAInputData*  pICAInputData)
{
    CamxResult  result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInputData);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " gain %f ", pICAInputData->digitalGain);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " lensPosition %f ", pICAInputData->lensPosition);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " Config mode %u ", pICAInputData->configMode);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " IPE Path %u ", pICAInputData->IPEPath);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " lens zoom %f ", pICAInputData->lensZoomRatio);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " luxIndex %f ", pICAInputData->luxIndex);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " dimension width %d height %d",
                     pICAInputData->pImageDimensions->widthPixels,
                     pICAInputData->pImageDimensions->heightLines);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " margin dimension width %d height %d",
                     pICAInputData->pMarginDimensions->widthPixels,
                     pICAInputData->pMarginDimensions->heightLines);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " zoom window left %d, top %d, width %d, height %d ",
                     pICAInputData->pZoomWindow->windowLeft,
                     pICAInputData->pZoomWindow->windowTop,
                     pICAInputData->pZoomWindow->windowWidth,
                     pICAInputData->pZoomWindow->windowHeight);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " IFE zoom window  left %d, top %d, width %d, height %d ",
                     pICAInputData->pIFEZoomWindow->windowLeft,
                     pICAInputData->pIFEZoomWindow->windowTop,
                     pICAInputData->pIFEZoomWindow->windowWidth,
                     pICAInputData->pIFEZoomWindow->windowHeight);

    return result;
}

CAMX_NAMESPACE_END
