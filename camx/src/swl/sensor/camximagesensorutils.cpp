////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximagesensorutils.cpp
/// @brief  Utility functions for camera image sensor and submodules.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camximagesensorutils.h"
#include "camxincs.h"
#include "camxsensorcommon.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::CreatePowerSequenceCmd(
    VOID*         pCmdBuffer,
    BOOL          isPowerUp,
    UINT          powerSequenceSize,
    PowerSetting* pSettings)
{
    CamxResult               result           = CamxResultSuccess;

    UINT                     baseSize         = sizeof(CSLSensorPowerCmd) - sizeof(CSLSensorPowerSetting);
    CSLSensorPowerCmd*       pPowerCmd        = NULL;
    CSLSensorWaitCmd*        pWait            = NULL;
    UINT                     offset           = 0;
    UINT16                   zeroDelayCount   = 0;
    CSLSensorPowerSetting*   pPowerSettings   = NULL;

    pPowerCmd       = reinterpret_cast<CSLSensorPowerCmd*>(static_cast<BYTE*>(pCmdBuffer));
    pPowerSettings  = &pPowerCmd->powerSettings[0];

    for (UINT index = 0; index < powerSequenceSize; index++)
    {
        Utils::Memset(&pPowerSettings[zeroDelayCount], 0, sizeof(CSLSensorPowerSetting));

        pPowerSettings[zeroDelayCount].powerSequenceType = static_cast<UINT16>(pSettings[index].configType);
        pPowerSettings[zeroDelayCount].configValLow      = static_cast<UINT32>(pSettings[index].configValue);
        pPowerSettings[zeroDelayCount].configValHigh     = 0;

        offset += sizeof(CSLSensorPowerSetting);

        if (pSettings[index].delayMs > 0)
        {
            pPowerCmd->count         = zeroDelayCount + 1;
            pPowerCmd->cmdType       = static_cast<UINT8>
                                       ((TRUE == isPowerUp) ? CSLSensorCmdTypePowerUp : CSLSensorCmdTypePowerDown);

            offset                   += baseSize;

            pWait                    = reinterpret_cast<CSLSensorWaitCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);
            pWait->delayMicroseconds = static_cast<UINT16>(pSettings[index].delayMs);
            pWait->opcode            = CSLSensorWaitOpcodeUnconditionalSw;
            pWait->cmdType           = CSLSensorCmdTypeWait;

            offset                   += sizeof(CSLSensorWaitCmd);

            pPowerCmd                = reinterpret_cast<CSLSensorPowerCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);
            pPowerSettings           = &pPowerCmd->powerSettings[0];

            zeroDelayCount           = 0;
        }
        else
        {
            zeroDelayCount++;
        }
    }

    if (0 != zeroDelayCount)
    {
        pPowerCmd->count   = zeroDelayCount;
        pPowerCmd->cmdType = static_cast<UINT8>
                             ((TRUE == isPowerUp) ? CSLSensorCmdTypePowerUp : CSLSensorCmdTypePowerDown);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageSensorUtils::GetPowerSequenceCmdSize(
    UINT          powerSequenceSize,
    PowerSetting* pSettings)
{
    UINT   baseSize         = sizeof(CSLSensorPowerCmd) - sizeof(CSLSensorPowerSetting);
    UINT   totalSize        = 0;
    UINT   zeroDelayCount   = 0;

    for (UINT i = 0; i < powerSequenceSize; i++)
    {
        totalSize += sizeof(CSLSensorPowerSetting);

        if (pSettings[i].delayMs > 0)
        {
            totalSize      += baseSize;
            totalSize      += sizeof(CSLSensorWaitCmd);
            zeroDelayCount = 0;
        }
        else
        {
            zeroDelayCount++;
        }
    }

    if (0 != zeroDelayCount)
    {
        totalSize += baseSize;
    }

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::CreateI2CInfoCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::CreateI2CInfoCmd(
    CSLSensorI2CInfo*   pI2CInfoCmd,
    UINT16              slaveAddr,
    I2CFrequencyMode    I2CFrequencyMode)
{
    CamxResult result               = CamxResultSuccess;

    pI2CInfoCmd->slaveAddr          = slaveAddr;
    pI2CInfoCmd->I2CFrequencyMode   = static_cast<UINT8>(I2CFrequencyMode);
    pI2CInfoCmd->cmdType            = CSLSensorCmdTypeI2CInfo;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::UpdateWaitCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorUtils::UpdateWaitCommand(
    CSLSensorWaitCmd* pWaitCmd,
    UINT32            delayUs)
{
    /// @todo (CAMX-2350999) Move this logic to KMD
    if (MaximumI2CHardwareDelayinUSec > delayUs)
    {
        pWaitCmd->delayMicroseconds = static_cast<UINT16>(delayUs);
        pWaitCmd->opcode            = CSLSensorWaitOpcodeUnconditionalHw;
    }
    else
    {
        // Software delay to be configured in milli seconds, so converting to milli seconds.
        pWaitCmd->delayMicroseconds = static_cast<UINT16>(Utils::Ceiling(static_cast<FLOAT>(delayUs) / 1000.0f));
        pWaitCmd->opcode            = CSLSensorWaitOpcodeUnconditionalSw;
    }

    pWaitCmd->cmdType = CSLSensorCmdTypeWait;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetRegisterSettingsCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageSensorUtils::GetRegisterSettingsCmdSize(
    UINT             settingsCount,
    RegisterSetting* pSettings,
    UINT* pRegSettingIndex)
{
    static const UINT CSLSensorI2CHeaderSizeBytes = sizeof(CSLSensorI2CRandomWriteCmd) - sizeof(CSLSensorI2CRegValPair);

    UINT               totalSize        = 0;
    I2CRegAddrDataType currentDataType  = 0;
    I2CRegAddrDataType currentAddrType  = 0;
    OperationType      lastOperation    = OperationType::MAX;
    UINT16             prevSlaveAddr    = 0;
    UINT               i                = *pRegSettingIndex;

    while ( i < settingsCount )
    {
        if (TRUE == pSettings[i].slaveAddrExists)
        {
            if (0 == prevSlaveAddr)
            {
                totalSize    += sizeof(CSLSensorI2CInfo);
                prevSlaveAddr = pSettings[i].slaveAddr;
            }
            else if (prevSlaveAddr != pSettings[i].slaveAddr)
            {
                *pRegSettingIndex = i;
                return totalSize;
            }
        }
        switch (pSettings[i].operation)
        {
            case OperationType::POLL:
                lastOperation = OperationType::POLL;

                totalSize += sizeof(CSLSensorWaitConditionalCmd);
                break;

            case OperationType::READ:
                lastOperation = OperationType::READ;

                totalSize += sizeof(CSLSensorI2CContinuousReadCmd);

                break;
            case OperationType::WRITE:
                if ((lastOperation != OperationType::WRITE) ||
                    (currentDataType != pSettings[i].regDataType) ||
                    (currentAddrType != pSettings[i].regAddrType))
                {
                    lastOperation = OperationType::WRITE;
                    currentDataType = pSettings[i].regDataType;
                    currentAddrType = pSettings[i].regAddrType;

                    totalSize += CSLSensorI2CHeaderSizeBytes;
                }
                totalSize += sizeof(CSLSensorI2CRegValPair);

                if (pSettings[i].delayUs > 0)
                {
                    totalSize += sizeof(CSLSensorWaitCmd);
                    lastOperation = OperationType::MAX;
                }

                break;

            case OperationType::WRITE_BURST:
            case OperationType::WRITE_SEQUENTIAL:
                lastOperation = pSettings[i].operation;

                totalSize += sizeof(CSLSensorI2CBurstWriteCmd) - sizeof(CSLSensorI2CVal);
                totalSize += sizeof(CSLSensorI2CVal) * pSettings[i].registerDataCount;
                if (pSettings->delayUs > 0)
                {
                    totalSize += sizeof(CSLSensorWaitCmd);
                }
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Unsupported operation %d", pSettings[i].operation);
                break;
        }
        i++;
    }

    // This condition indicates that all the slave addresses have been addressed and the start
    // index can be reset to 0
    if (i == settingsCount)
    {
        *pRegSettingIndex = 0;
    }
    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::CreateRegisterSettingsCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::CreateRegisterSettingsCmd(
    VOID*            pCmdBuffer,
    UINT             settingsCount,
    RegisterSetting* pSettings,
    UINT             regSettingIdx,
    I2CFrequencyMode sensorFrequencyMode)
{
    CamxResult                       result            = CamxResultSuccess;
    UINT                             baseSizeWrite     = sizeof(CSLSensorI2CRandomWriteCmd) - sizeof(CSLSensorI2CRegValPair);
    I2CRegAddrDataType               currentDataType   = 0;
    I2CRegAddrDataType               currentAddrType   = 0;
    CSLSensorI2CRandomWriteCmd*      pWriteCmd         = NULL;
    CSLSensorI2CContinuousReadCmd*   pReadCmd          = NULL;
    CSLSensorWaitConditionalCmd*     pPollCmd          = NULL;
    CSLSensorWaitCmd*                pWaitCmd          = NULL;
    CSLSensorI2CRegValPair*          pRegValPair       = NULL;
    UINT                             offset            = 0;
    OperationType                    lastOperation     = OperationType::MAX;
    CSLSensorI2CInfo*                pCmdI2CInfo       = NULL;
    UINT16                           prevSlaveAddr     = 0;
    UINT                             i                 = regSettingIdx;

    while (i < settingsCount)
    {
        if (TRUE == pSettings[i].slaveAddrExists)
        {
            if (0 == prevSlaveAddr)
            {
                pCmdI2CInfo = reinterpret_cast<CSLSensorI2CInfo*>(static_cast<BYTE*>(pCmdBuffer) + offset);
                CreateI2CInfoCmd(pCmdI2CInfo, pSettings[i].slaveAddr, sensorFrequencyMode);
                offset += sizeof(CSLSensorI2CInfo);
                prevSlaveAddr = pSettings[i].slaveAddr;
            }
            else if (prevSlaveAddr != pSettings[i].slaveAddr)
            {
                return result;
            }
        }
        switch (pSettings[i].operation)
        {
            case OperationType::POLL:
                lastOperation = OperationType::POLL;
                pPollCmd = reinterpret_cast<CSLSensorWaitConditionalCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);

                pPollCmd->dataType            = static_cast<UINT8>(pSettings[i].regDataType);
                pPollCmd->addrType            = static_cast<UINT8>(pSettings[i].regAddrType);
                pPollCmd->opcode              = CSLSensorWaitOpcodeConditional;
                pPollCmd->cmdType             = CSLSensorCmdTypeWait;
                pPollCmd->timeoutMilliseconds =
                    static_cast<UINT16>(Utils::Ceiling(static_cast<FLOAT>(pSettings[i].delayUs) / 1000.0f));
                pPollCmd->reg                 = pSettings[i].registerAddr;
                pPollCmd->val                 = pSettings[i].registerData[0];
                pPollCmd->mask                = 0xFFFF;

                offset += sizeof(CSLSensorWaitConditionalCmd);
                break;

            case OperationType::READ:
                lastOperation = OperationType::READ;
                pReadCmd = reinterpret_cast<CSLSensorI2CContinuousReadCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);

                pReadCmd->header.dataType = static_cast<UINT8>(pSettings[i].regDataType);
                pReadCmd->header.addrType = static_cast<UINT8>(pSettings[i].regAddrType);
                pReadCmd->header.opcode   = CSLSensorI2COpcodeContinuousRead;
                pReadCmd->header.cmdType  = CSLSensorCmdTypeI2CContinuousRegRead;
                pReadCmd->header.count    = static_cast<UINT32>(pSettings[i].registerData[0]);
                pReadCmd->reg             = pSettings[i].registerAddr;

                offset += sizeof(CSLSensorI2CContinuousReadCmd);

                break;
            case OperationType::WRITE:
                if ((lastOperation != OperationType::WRITE) ||
                    (currentDataType != pSettings[i].regDataType) ||
                    (currentAddrType != pSettings[i].regAddrType))
                {
                    lastOperation              = OperationType::WRITE;
                    currentDataType            = pSettings[i].regDataType;
                    currentAddrType            = pSettings[i].regAddrType;
                    pWriteCmd                  = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>
                                                 (static_cast<BYTE*>(pCmdBuffer) + offset);
                    pRegValPair                = &pWriteCmd->regValPairs[0];
                    pWriteCmd->header.count    = 0;
                    pWriteCmd->header.opcode   = CSLSensorI2COpcodeRandomWrite;
                    pWriteCmd->header.cmdType  = CSLSensorCmdTypeI2CRandomRegWrite;
                    pWriteCmd->header.dataType = static_cast<UINT8>(pSettings[i].regDataType);
                    pWriteCmd->header.addrType = static_cast<UINT8>(pSettings[i].regAddrType);

                    offset += baseSizeWrite;
                }
                if (NULL != pRegValPair)
                {
                    pRegValPair[pWriteCmd->header.count].reg = pSettings[i].registerAddr;
                    pRegValPair[pWriteCmd->header.count].val = pSettings[i].registerData[0];
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Reg Val pair data is NULL");
                    return CamxResultEInvalidPointer;
                }
                pWriteCmd->header.count++;
                offset += sizeof(CSLSensorI2CRegValPair);

                if (pSettings[i].delayUs > 0)
                {
                    pWaitCmd        = reinterpret_cast<CSLSensorWaitCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);
                    offset         += sizeof(CSLSensorWaitCmd);
                    // Reset to initialize state so it start from beginning
                    lastOperation   = OperationType::MAX;
                    UpdateWaitCommand(pWaitCmd, pSettings[i].delayUs);
                }

                break;
            case OperationType::WRITE_BURST:
            case OperationType::WRITE_SEQUENTIAL:
            {
                CSLSensorI2CVal*           pVals;
                CSLSensorI2CBurstWriteCmd* pBurstCmd;

                lastOperation              = pSettings[i].operation;
                pBurstCmd                  = reinterpret_cast<CSLSensorI2CBurstWriteCmd*>
                                                (static_cast<BYTE*>(pCmdBuffer) + offset);
                pVals                      = &pBurstCmd->data[0];
                pBurstCmd->header.count    = static_cast<UINT32>(pSettings[i].registerDataCount);

                if (OperationType::WRITE_BURST == pSettings[i].operation)
                {
                    pBurstCmd->header.opcode = CSLSensorI2COpcodeContinuousWriteBurst;
                }
                else
                {
                    pBurstCmd->header.opcode = CSLSensorI2COpcodeContinuousWriteSequence;
                }

                pBurstCmd->header.cmdType  = CSLSensorCmdTypeI2CContinuousRegWrite;
                pBurstCmd->header.dataType = static_cast<UINT8>(pSettings[i].regDataType);
                pBurstCmd->header.addrType = static_cast<UINT8>(pSettings[i].regAddrType);
                pBurstCmd->reg = pSettings[i].registerAddr;

                offset += sizeof(CSLSensorI2CBurstWriteCmd) - sizeof(CSLSensorI2CVal);

                for (UINT32 j = 0; j < pSettings[i].registerDataCount; j++)
                {
                    pVals[j].val = pSettings[i].registerData[j];
                    pVals[j].reserved = 0;
                }
                offset += pSettings[i].registerDataCount * sizeof(CSLSensorI2CVal);

                if (pSettings[i].delayUs > 0)
                {
                    pWaitCmd = reinterpret_cast<CSLSensorWaitCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);
                    offset  += sizeof(CSLSensorWaitCmd);
                    UpdateWaitCommand(pWaitCmd, pSettings[i].delayUs);
                }

                break;
            }

            default:
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Unsupported operation %d", pSettings[i].operation);
                result = CamxResultENotImplemented;
                break;
        }
        i++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetClosestColorCorrectionTransformIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageSensorUtils::GetClosestColorCorrectionTransformIndex(
    cc_1_3_0::mod_cc13_aec_dataType* pCC13AECData,
    FLOAT                            colorTemperature)
{
    UINT                             dataCount     = static_cast<UINT>(pCC13AECData->aec_data.mod_cc13_cct_dataCount);
    cc_1_3_0::mod_cc13_cct_dataType* pCC13CCTData  = pCC13AECData->aec_data.mod_cc13_cct_data;
    UINT32                           closestIndex  = 0;
    FLOAT                            diff          = 0.0;

    if ((0 != dataCount) && (NULL != pCC13CCTData))
    {
        diff = fabs(colorTemperature - pCC13CCTData[0].cct_trigger.start);

        for (UINT32 index = 0; index < dataCount; index++)
        {
            if ((colorTemperature <= pCC13CCTData[index].cct_trigger.end) &&
                (colorTemperature >= pCC13CCTData[index].cct_trigger.start))
            {
                closestIndex = index;
                break;
            }
            else
            {
                FLOAT startDiff = fabs(colorTemperature - pCC13CCTData[index].cct_trigger.start);
                FLOAT endDiff   = fabs(colorTemperature - pCC13CCTData[index].cct_trigger.end);
                if ((diff > startDiff) || (diff > endDiff))
                {
                    closestIndex = index;
                    diff         = (startDiff > endDiff) ? endDiff : startDiff;
                }
            }
        }
    }

    return closestIndex;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetTunedColorCorrectionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::GetTunedColorCorrectionData(
    TuningDataManager* pTuningManager,
    TuningMode*        pTuningMode,
    UINT32             numSelectors,
    FLOAT*             pColorCorrectionData,
    SIZE_T             colorCorrectionDataSize,
    FLOAT              colorTemperature)
{
    CamxResult                      result          = CamxResultSuccess;
    UINT32                          cctIndex        = 0;
    cc_1_3_0::chromatix_cc13Type*   pCC13Data       = NULL;

    pCC13Data = pTuningManager->GetChromatix()->GetModule_cc13_bps(pTuningMode, numSelectors);

    CAMX_ASSERT(NULL != pCC13Data);

    if ((NULL != pCC13Data))
    {
        cctIndex = GetClosestColorCorrectionTransformIndex(
                      pCC13Data->chromatix_cc13_core.mod_cc13_drc_gain_data->
                        drc_gain_data.mod_cc13_hdr_aec_data->
                        hdr_aec_data. mod_cc13_led_idx_data->led_idx_data.mod_cc13_aec_data,
                        colorTemperature);

        Utils::Memcpy(pColorCorrectionData,
                      pCC13Data->chromatix_cc13_core.mod_cc13_drc_gain_data->
                        drc_gain_data.mod_cc13_hdr_aec_data->
                        hdr_aec_data. mod_cc13_led_idx_data->led_idx_data.mod_cc13_aec_data->
                        aec_data.mod_cc13_cct_data[cctIndex].cc13_rgn_data.c_tab.c,
                      colorCorrectionDataSize);
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Tuned Color Correction result: %d, cc13data: %p",
                       result, pCC13Data);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetTunedMWBData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::GetTunedMWBData(
    TuningDataManager*                  pTuningManager,
    TuningMode*                         pTuningMode,
    UINT32                              numSelectors,
    awbglobalelements::awbRGBDataType*  pMWBData,
    UINT8                               MWBIlluminantIndex)
{
    CamxResult                      result          = CamxResultSuccess;
    modmwbv1::chromatixMWBV1Type*   pMWBModuleData  = NULL;

    pMWBModuleData = pTuningManager->GetChromatix()->GetModule_MWBGainV1(pTuningMode, numSelectors);

    CAMX_ASSERT(NULL != pMWBModuleData);

    if (NULL != pMWBModuleData)
    {
        Utils::Memcpy(pMWBData, &pMWBModuleData->MWBData[MWBIlluminantIndex], sizeof(awbglobalelements::awbRGBDataType));
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get tuned MWB result: %d, module data: %p",
                       result, pMWBModuleData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetISO100GainData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::GetISO100GainData(
    TuningDataManager*                  pTuningManager,
    TuningMode*                         pTuningMode,
    FLOAT *                             pISO100Gain)
{
    CamxResult                              result       = CamxResultSuccess;
    aecArbitration::AECCoreArbitrationType* pCore        = NULL;
    UINT                                    numSelectors = 1;

    if (NULL != pTuningManager)
    {
        pCore = pTuningManager->GetChromatix()->GetModule_Arbitration(pTuningMode, numSelectors);

        if (NULL != pCore)
        {
            *pISO100Gain = pCore->ISOData.previewISO100Gain;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get m_pCore result: %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get pTuningManager result: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GetMaxExposureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::GetMaxExposureData(
    TuningDataManager*  pTuningManager,
    TuningMode*         pTuningMode,
    UINT                numSelectors,
    UINT64*             pMaxExposure)
{
    CamxResult                              result = CamxResultSuccess;
    aecArbitration::AECCoreArbitrationType* pCore = NULL;

    if (NULL != pTuningManager)
    {
        pCore = pTuningManager->GetChromatix()->GetModule_Arbitration(pTuningMode, numSelectors);

        if (NULL != pCore)
        {
            UINT32 expTablesCount = pCore->expTablesCount;
            UINT32 expKneeEntriesCount = pCore->expTables[expTablesCount-1].expKneeEntriesCount;
            *pMaxExposure = pCore->expTables[expTablesCount-1].expKneeEntries[expKneeEntriesCount-1].expTime;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get pCore, result: %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get Tuning Manager, result: %d", result);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorUtils::ReadDeviceData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorUtils::ReadDeviceData(
    INT32         deviceID,
    UINT16        numOfBytes,
    UINT8*        pReadData,
    UINT32        opcode,
    INT32         hCSLDeviceHandle,
    CSLDeviceType device,
    INT32         hCSLSession,
    Packet*       pSensorReadPacket)
{
    BufferManagerCreateData createData       = { };
    ImageFormat*            pFormat          = &createData.bufferProperties.imageFormat;
    CamxResult              result;

    // Image buffer and corresponding manager object
    ImageBufferManager*     pImageBufferManager = NULL;
    ImageBuffer*            pImage              = NULL;

    // Pointer to the data read from sensor submodule
    // Calling function should free this memory once the data is consumed
    UINT8*                  pSensorReadData  = NULL;

    // Create Image buffer manager
    pFormat->width     = numOfBytes;
    pFormat->height    = 1;
    pFormat->format    = Format::Blob;
    pFormat->alignment = 8;

    createData.maxBufferCount               = 1;
    createData.immediateAllocBufferCount    = 1;
    createData.deviceCount                  = 1;
    createData.deviceIndices[0]             = deviceID;
    createData.bufferProperties.memFlags    = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
    createData.allocateBufferMemory         = TRUE;
    createData.numBatchedFrames             = 1;
    createData.bufferProperties.bufferHeap  = CSLBufferHeapIon;
    createData.bufferManagerType            = BufferManagerType::CamxBufferManager;

    result = ImageBufferManager::Create("SensorReadRequest", &createData, &pImageBufferManager);

    // Get IO buffer
    if (CamxResultSuccess == result)
    {
        pImage = pImageBufferManager->GetImageBuffer();
        if (NULL == pImage)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer");
            result = CamxResultEFailed;
        }
    }

    // Add IO config to the packet
    if (CamxResultSuccess == result)
    {
        result = pSensorReadPacket->AddIOConfig(pImage,
                                                0,
                                                CSLIODirection::CSLIODirectionOutput,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL,
                                                0);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to add IO config =%d", result);
        }
    }

    // Set the IO buffer information in packet and commit
    if (CamxResultSuccess == result)
    {
        pSensorReadPacket->SetOpcode(device, opcode);
        result = pSensorReadPacket->CommitPacket();
    }

    // Submit the read request to kernel
    if (CamxResultSuccess == result)
    {
        result = CSLSubmit(hCSLSession,
                            hCSLDeviceHandle,
                            pSensorReadPacket->GetMemHandle(),
                            pSensorReadPacket->GetOffset());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit read request");
        }
    }

    // Copy the read data from kernel
    if (CamxResultSuccess == result)
    {
        pSensorReadData = pImage->GetPlaneVirtualAddr(0, 0);
        if ((NULL != pReadData) && (NULL != pSensorReadData))
        {
            Utils::Memcpy(reinterpret_cast<VOID*>(pReadData),
                            static_cast<VOID*>(pSensorReadData),
                            static_cast<UINT64>(numOfBytes));
            CAMX_LOG_INFO(CamxLogGroupSensor, "Copied %d bytes to %p", numOfBytes, pReadData);
            for (UINT index =0; index < numOfBytes; index++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Byte[%d]: %d", index, pReadData[index]);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to read data from the device %d", deviceID);
        }
    }

    // Free IO buffer manager and image buffer
    if ((NULL != pImageBufferManager) && (NULL != pImage))
    {
        pImageBufferManager->ReleaseReference(pImage);
        pImage = NULL;

        pImageBufferManager->Destroy();
        pImageBufferManager = NULL;
    }

    return result;
}


static FLOAT sRGB2XYZ [3][3]    =
{
    {0.4360747f, 0.3850649f, 0.1430804f},
    {0.2225045f, 0.7168786f, 0.0606169f},
    {0.0139322f, 0.0971045f, 0.7141733f}
};

static FLOAT XYZ2RGB[3][3]      =
{
    {0.4124564f, 0.3575761f, 0.1804375f},
    {0.2126729f, 0.7151522f, 0.0721750f},
    {0.0193339f, 0.1191920f, 0.9503041f}
};

static FLOAT D65_to_ref_A[3][3] =
{
    {  1.2164557f,    0.1109905f, (-0.1549325f)},
    {  0.1533326f,    0.9152313f, (-0.0559953f)},
    {(-0.0239469f),   0.0358984f,   0.3147529f}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GenerateTransformMatrix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorUtils::GenerateTransformMatrix(
    Rational                            forwardMatrix[3][3],
    Rational                            colorMatrix[3][3],
    FLOAT*                              pColorCorrectionData,
    awbglobalelements::awbRGBDataType*  pMWBData,
    BOOL                                isD65)
{
    FLOAT   colorCorrectionMatrix[3][3] = {{0}};
    FLOAT   MWBMatrix[3][3]             = {{0}};
    FLOAT   outputMatrix[3][3]          = {{0}};
    FLOAT*  pPtr1                       = NULL;
    FLOAT*  pPtr2                       = NULL;
    FLOAT   temporaryMatrix[3][3]       = {{0}};
    FLOAT   temporaryMatrix1[3][3]      = {{0}};
    FLOAT   temporaryMatrix2[3][3]      = {{0}};

    Utils::Memcpy(static_cast<VOID*>(colorCorrectionMatrix),
                    static_cast<const VOID *>(pColorCorrectionData),
                    (sizeof(colorCorrectionMatrix)));

    MWBMatrix[0][0] = pMWBData->red;
    MWBMatrix[0][1] = 0;
    MWBMatrix[0][2] = 0;
    MWBMatrix[1][0] = 0;
    MWBMatrix[1][1] = pMWBData->green;
    MWBMatrix[1][2] = 0;
    MWBMatrix[2][0] = 0;
    MWBMatrix[2][1] = 0;
    MWBMatrix[2][2] = pMWBData->blue;

    /* Forward Transform: sRGB2XYZ * CC */
    Utils::MatrixMultiply(sRGB2XYZ, colorCorrectionMatrix, outputMatrix, 3);
    FloatToRational3x3(outputMatrix, forwardMatrix);

    if (TRUE == isD65)
    {
        Utils::MatrixMultiply(XYZ2RGB, colorCorrectionMatrix, temporaryMatrix1, 3);
    }
    else
    {
        Utils::MatrixMultiply(D65_to_ref_A, XYZ2RGB, temporaryMatrix, 3);
        Utils::MatrixMultiply(temporaryMatrix, colorCorrectionMatrix, temporaryMatrix1, 3);
    }
    Utils::MatrixMultiply(temporaryMatrix1, MWBMatrix, temporaryMatrix2, 3);

    pPtr1 = reinterpret_cast<FLOAT*>(temporaryMatrix2);
    pPtr2 = reinterpret_cast<FLOAT*>(outputMatrix);
    Utils::MatrixInverse3x3(pPtr1, pPtr2);

    FloatToRational3x3(outputMatrix, colorMatrix);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GenerateCalibrationTransformMatrix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorUtils::GenerateCalibrationTransformMatrix(
    Rational    transformMatrix[3][3],
    FLOAT       rGain,
    FLOAT       bGain)
{
    FLOAT WBCalibrationMatrix[3][3] = {{0}};

    WBCalibrationMatrix[0][0] = rGain;
    WBCalibrationMatrix[1][1] = 1;
    WBCalibrationMatrix[2][2] = bGain;

    FloatToRational3x3(WBCalibrationMatrix, transformMatrix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorUtils::GenerateCalibrationTransformMatrix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorUtils::GenerateUnitMatrix(
    Rational    unitMatrix[3][3])
{
    FLOAT WBCalibrationMatrix[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    FloatToRational3x3(WBCalibrationMatrix, unitMatrix);
}

CAMX_NAMESPACE_END
