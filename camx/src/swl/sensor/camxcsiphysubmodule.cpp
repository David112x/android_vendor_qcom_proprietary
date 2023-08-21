////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxcsiphysubmodule.cpp
/// @brief Implements CSI Phy methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxsensorproperty.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxsensorproperty.h"
#include "camxactuatordata.h"
#include "camxpdafconfig.h"
#include "camxpdafdata.h"
#include "camxcsiphysubmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSIPHYSubmodule::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSIPHYSubmodule::Create(
    HwContext*        pHwContext,
    CSLHandle         hCSLSession,
    CSIPHYSubmodule** ppCSIPHYSubModule,
    INT32             csiPHYDeviceIndex,
    UINT32            cameraId,
    const CHAR*       pDeviceName)
{
    CamxResult              result                  = CamxResultSuccess;
    CSIPHYSubmodule*        pCSIPHY                 = NULL;
    CSLCSIPHYAcquireDevice  CSIPHYAcquireDevice     = { 0 };
    CSLDeviceResource       deviceResourceRequest   = { 0 };

    if (NULL == ppCSIPHYSubModule)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPHYSubModule pointer is NULL");
        return CamxResultEInvalidPointer;
    }
    else
    {
        pCSIPHY = CAMX_NEW CSIPHYSubmodule(pHwContext);

        if (NULL == pCSIPHY)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPHY submodule creation failed - out of memory");
            result = CamxResultENoMemory;
        }
        else
        {
            const CSIInformation* pCSIInfo  = (pHwContext->GetImageSensorModuleData(cameraId))->GetCSIInfo();
            if (NULL == pCSIInfo)
            {
                CSIPHYAcquireDevice.comboMode = 0;
            }
            else
            {
                CSIPHYAcquireDevice.comboMode = pCSIInfo->isComboMode;
            }

            CSIPHYAcquireDevice.reserved                    = 0;
            deviceResourceRequest.pDeviceResourceParam      = &CSIPHYAcquireDevice;
            deviceResourceRequest.deviceResourceParamSize   = sizeof(CSLCSIPHYAcquireDevice);

            result = pCSIPHY->Initialize(hCSLSession, csiPHYDeviceIndex, &deviceResourceRequest, cameraId, pDeviceName);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPHY submodule initialization failed result: %d", result);
                pCSIPHY->Destroy();
                pCSIPHY = NULL;
            }
        }

        if (NULL != pCSIPHY)
        {
            *ppCSIPHYSubModule = pCSIPHY;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSIPHYSubmodule::CSIPHYSubmodule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSIPHYSubmodule::CSIPHYSubmodule(
    HwContext*    pHwContext)
    : SensorSubModuleBase(pHwContext)
{
    m_CSLDeviceType = CSLDeviceTypeCSIPHY;
    m_resourseSize  = sizeof(CSLSensorCSIPHYInfo);
    m_poolSize      = sizeof(CSLSensorCSIPHYInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSIPHYSubmodule::~CSIPHYSubmodule
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSIPHYSubmodule::~CSIPHYSubmodule()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSIPHYSubmodule::Configure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSIPHYSubmodule::Configure(
    CSLHandle                    hCSLSession,
    const CSLSensorCSIPHYInfo*   pCmdCSIPHYConfig)
{
    CamxResult      result     = CamxResultSuccess;
    PacketResource* pResource  = NULL;
    CmdBuffer*      pCmdBuffer = NULL;
    Packet*         pPacket    = NULL;

    result = m_pPacketManager->GetBuffer(&pResource);
    if (CamxResultSuccess == result)
    {
        pPacket = static_cast<Packet*>(pResource);
        result = m_pCmdBufferManager->GetBuffer(&pResource);
        if (CamxResultSuccess == result)
        {
            pCmdBuffer = static_cast<CmdBuffer*>(pResource);
        }
    }
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get buffer packet resource: %p, packet: %p, cmd buffer: %p, result: %d",
                       pResource, pPacket, pCmdBuffer, result);
    }

    if (CamxResultSuccess == result)
    {
        VOID* pCmdBegin = pCmdBuffer->BeginCommands(sizeof(CSLSensorCSIPHYInfo) / sizeof(UINT32));
        if (NULL != pCmdBegin)
        {
            CSLSensorCSIPHYInfo* pCSIPHYConfig = reinterpret_cast<CSLSensorCSIPHYInfo*>(pCmdBegin);

            *pCSIPHYConfig = *pCmdCSIPHYConfig;

            result = pCmdBuffer->CommitCommands();

            if (CamxResultSuccess == result)
            {
                pPacket->SetOpcode(CSLDeviceTypeInvalidDevice, CSLPacketOpcodesCSIPHYInitialConfig);

                result = pPacket->AddCmdBufferReference(pCmdBuffer, NULL);
            }

            if (CamxResultSuccess == result)
            {
                result = pPacket->CommitPacket();
            }
        }
        if (CamxResultSuccess == result)
        {
            m_pHwContext->Submit(hCSLSession, m_hCSLDeviceHandle, pPacket);
        }
        // Recycle the buffers.
        m_pCmdBufferManager->Recycle(pCmdBuffer);
        m_pPacketManager->Recycle(pPacket);
    }

    return result;
}

CAMX_NAMESPACE_END
