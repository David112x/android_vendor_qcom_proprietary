////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamifrdititan480.cpp
/// @brief camxifecamifrdititan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecamifrdititan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFRdiTitan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    UINT32 registerOfffsetForInstance = IFECamifRdiRegInstanceOffset * m_instance;
    UINT32 registerOffsetForCmd1      = IFECamifRdiRegCmd1Offset;
    UINT32 registerOffsetForCmd2      = IFECamifRdiRegCmd2Offset;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        switch (pInputData->pStripeConfig->pCAMIFConfigInfo->IFEHWType)
        {
            case IFEHWTypeNormal:
                registerOffsetForCmd1 = IFECamifRdiRegCmd1Offset;
                registerOffsetForCmd2 = IFECamifRdiRegCmd2Offset;
                break;
            case IFEHWTypeLite:
                registerOffsetForCmd1 = IFELiteCamifRdiRegCmd1Offset;
                registerOffsetForCmd2 = IFELiteCamifRdiRegCmd2Offset;
                break;
            default:
                registerOffsetForCmd1 = IFECamifRdiRegCmd1Offset;
                registerOffsetForCmd2 = IFECamifRdiRegCmd2Offset;

                CAMX_LOG_ERROR(CamxLogGroupISP,
                    "Not support IFE HW Type %d, using normal IFE as default",
                    pInputData->pStripeConfig->pCAMIFConfigInfo->IFEHWType);

                break;
        }

        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              registerOffsetForCmd1 + registerOfffsetForInstance,
                                              IFECAMIFRdiRegCmd1LengthDWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  registerOffsetForCmd2 + registerOfffsetForInstance,
                                                  IFECAMIFRdiRegCmd2LengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFRdiTitan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pBayerPattern)
{
    CamxResult result           = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);
    UINT8*        pPixelPattern = static_cast<UINT8*>(pBayerPattern);

    // setup camif module
    m_regCmd1.configRegister.bitfields.EN            = 0x1;            // enable camif module
    m_regCmd1.configRegister.bitfields.STRIPE_LOC    = 0x0;            // stripe location. For IFE, "0" is full frame

    // 0x0:RGRGRG, 0x1:GRGRGR, 0x2:BGBGBG, 0x3:GBGBGB,
    // 0x4:YCBYCR, 0x5:YCRYCB, 0x6:CBYCRY, 0x7:CRYCBY
    m_regCmd1.configRegister.bitfields.PIXEL_PATTERN = *pPixelPattern; // Bayer pattern

    // Setup camif subsample
    if (0 == pInputData->pipelineIFEData.numBatchedFrames)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid batch frame number zero");
    }

    if (CamxResultSuccess == result)
    {
        m_regCmd2.irqSubsamplePattern.bitfields.PATTERN             = 0xFFFFFFFF;
        m_regCmd2.skipPeriodRegister.bitfields.IRQ_SUBSAMPLE_PERIOD = 0x0;

        if (1 < pInputData->pipelineIFEData.numBatchedFrames)
        {
            m_regCmd2.skipPeriodRegister.bitfields.IRQ_SUBSAMPLE_PERIOD = pInputData->pipelineIFEData.numBatchedFrames - 1;
            m_regCmd2.irqSubsamplePattern.bitfields.PATTERN             =
                1 << (pInputData->pipelineIFEData.numBatchedFrames - 1);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFRdiTitan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECAMIFRdiTitan480::CopyRegCmd(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "No need to implement");

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::~IFECAMIFRdiTitan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFRdiTitan480::~IFECAMIFRdiTitan480()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Destroyed");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECAMIFRdiTitan480::IFECAMIFRdiTitan480()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFRdiTitan480::IFECAMIFRdiTitan480(
    UINT32 instance)
{

    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRdiRegCmd1LengthDWord) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFRdiRegCmd2LengthDWord));
    Utils::Memset(&m_regCmd1, 0, sizeof(m_regCmd1));
    Utils::Memset(&m_regCmd2, 0, sizeof(m_regCmd2));

    m_instance = instance;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFRdiTitan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIFRdiTitan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif module configure: 0x%x", m_regCmd1.configRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif irq subsample pattern: 0x%x", m_regCmd2.irqSubsamplePattern.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Camif subsample period : 0x%x", m_regCmd2.skipPeriodRegister.u32All);
}

CAMX_NAMESPACE_END
