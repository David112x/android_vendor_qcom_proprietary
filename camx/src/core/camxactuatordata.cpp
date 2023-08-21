////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxactuatordata.cpp
/// @brief Implements ActuatorData methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxactuatordata.h"
#include "camximagesensorutils.h"
#include "camxincs.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ActuatorData::ActuatorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActuatorData::ActuatorData(
    ActuatorDriverData* pActuatorDriverData)
{
    m_pActuatorDriverData = pActuatorDriverData;
    m_currentPositionStep = InvalidLensData;
    m_currentPositionDAC  = InvalidLensData;
    m_infinityDAC         = 0;
    m_macroDAC            = 0;
    Utils::Memset(m_stepTable, 0, sizeof(m_stepTable));

    CAMX_ASSERT(InvalidLensData > Utils::Power(2.0, m_pActuatorDriverData->slaveInfo.dataBitWidth));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ActuatorData::~ActuatorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActuatorData::~ActuatorData()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ActuatorData::GetCurrentPosition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ActuatorData::GetCurrentPosition(
    PositionUnit unit)
{
    INT position = 0;

    if (PositionUnit::Step == unit)
    {
        position = m_currentPositionStep;
    }
    else if (PositionUnit::DAC == unit)
    {
        position = (m_currentPositionDAC != InvalidLensData) ? m_currentPositionDAC : m_stepTable[0];
    }

    return position;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::CalibrateActuatorDriverData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::CalibrateActuatorDriverData(
    const AFCalibrationData* pAFData)
{
    CamxResult              result          = CamxResultSuccess;
    UINT32                  regionSize      = m_pActuatorDriverData->tunedParams.regionParams.regionCount;
    ActuatorRegionParams*   pRegionParams   = m_pActuatorDriverData->tunedParams.regionParams.region;
    UINT32                  stepTableSize   = pRegionParams[regionSize - 1].macroStepBoundary;
    INT32                   DACRange        = 0;
    FLOAT                   codePerStep     = 0;
    INT32                   signSpecifier   = 1;
    INT16                   infinityDAC     = 0;
    INT16                   macroDAC        = 0;
    UINT16                  dataBitWidth    = m_pActuatorDriverData->slaveInfo.dataBitWidth;
    ActuatorType            actuatorType    = m_pActuatorDriverData->slaveInfo.actuatorType;
    INT16                   lowerBound      = 0;
    INT16                   upperBound      = 0;

    CAMX_ASSERT(0 != stepTableSize);
    CAMX_ASSERT(NULL != pAFData);
    CAMX_ASSERT(0 != (pAFData->macroDAC - pAFData->infinityDAC));

    if (ActuatorType::VCM == actuatorType)
    {
        lowerBound = 0;
        upperBound = static_cast<INT16>(Utils::Power(2.0, dataBitWidth) - 1);
    }
    else if (ActuatorType::BIVCM == actuatorType)
    {
        lowerBound  = static_cast<INT16>(-Utils::Power(2.0, dataBitWidth-1));
        upperBound  = static_cast<INT16>(Utils::Power(2.0, dataBitWidth-1) - 1);
    }

    infinityDAC     = pAFData->infinityDAC;
    macroDAC        = pAFData->macroDAC;
    DACRange        = macroDAC - infinityDAC;
    signSpecifier   = (infinityDAC > 0) ? (1) : (-1);
    infinityDAC    += static_cast<INT16>(pAFData->infinityMargin * Utils::AbsoluteINT32(DACRange) * signSpecifier);

    signSpecifier   = (macroDAC > 0) ? (1) : (-1);
    macroDAC       += static_cast<INT16>(pAFData->macroMargin * Utils::AbsoluteINT32(DACRange) * signSpecifier);
    CAMX_LOG_INFO(CamxLogGroupSensor,
                   "Infinity:%d, Macro: %d: WithMargin: Infinity:%d, Macro: %d, InfinityMargin:%f, MacroMargin: %f",
                   pAFData->infinityDAC,
                   pAFData->macroDAC,
                   infinityDAC,
                   macroDAC,
                   pAFData->infinityMargin,
                   pAFData->macroMargin);

    if ((ActuatorType::VCM == actuatorType) || (ActuatorType::BIVCM == actuatorType))
    {
        macroDAC    = (macroDAC > upperBound) ? (upperBound) : (macroDAC);
        infinityDAC = (infinityDAC < lowerBound) ? (lowerBound) : (infinityDAC);
    }

    DACRange    = (macroDAC) - (infinityDAC);
    codePerStep = (DACRange / static_cast<FLOAT>(stepTableSize));

    for (UINT size = 0; size < regionSize; size++)
    {
        pRegionParams[size].codePerStep = codePerStep;
    }

    for (UINT size = 0; size < m_pActuatorDriverData->initSettings.regSettingCount; size++)
    {
        if (pAFData->hallRegisterAddr == m_pActuatorDriverData->initSettings.regSetting[size].registerAddr)
        {
            m_pActuatorDriverData->initSettings.regSetting[size].registerData[0] = pAFData->hallOffsetBias;
        }
    }

    m_pActuatorDriverData->tunedParams.initialCode = infinityDAC;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::InitializeStepTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::InitializeStepTable(
    AFCalibrationData* pAFData)
{
    CamxResult                result        = CamxResultEOutOfBounds;
    UINT                      regionSize    = m_pActuatorDriverData->tunedParams.regionParams.regionCount;
    ActuatorRegionParams*     pRegionParams = m_pActuatorDriverData->tunedParams.regionParams.region;

    if ((NULL != pAFData) && (TRUE == pAFData->isAvailable))
    {
        result = CalibrateActuatorDriverData(pAFData);
    }
    else
    {
        // Step table can be initialized even without AF calibration data
        CAMX_LOG_INFO(CamxLogGroupSensor, "AF calibration data is not available in OTP data, pAFData: %p", pAFData);
    }

    CAMX_ASSERT(0 == pRegionParams[0].infinityStepBoundary);

    m_stepTableSize = pRegionParams[regionSize - 1].macroStepBoundary;

    CAMX_ASSERT_MESSAGE(m_stepTableSize <= MaxSteps,
                        "Step table size: %d exceeds maximum number of steps: %d",
                        m_stepTableSize,
                        MaxSteps);

    if (m_stepTableSize <= MaxSteps)
    {
        UINT  stepIndex   = 0;
        UINT  j           = 0;
        FLOAT currentCode = m_pActuatorDriverData->tunedParams.initialCode;

        for (UINT i = 0; i < regionSize; i++)
        {
            if (i > 0)
            {
                CAMX_ASSERT_MESSAGE(pRegionParams[i].macroStepBoundary > pRegionParams[i-1].macroStepBoundary,
                                    "Step Boundary check failed for index: %d, step boundary: %d, previous step boundary: %d",
                                    i, pRegionParams[i].macroStepBoundary, pRegionParams[i - 1].macroStepBoundary);
            }

            for (;(stepIndex < pRegionParams[i].macroStepBoundary); stepIndex++)
            {
                // infinity position stored at largest index to match AF algorithm definition
                j              = m_stepTableSize - 1 - stepIndex;
                CAMX_ASSERT(j < MaxSteps);
                m_stepTable[j] = static_cast<INT16>(currentCode);
                currentCode   += pRegionParams[i].codePerStep;

                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Table[%d] %d", j, m_stepTable[j]);
            }
        }

        CAMX_ASSERT_MESSAGE(stepIndex == m_stepTableSize, "Step index: %d, table size: %d", stepIndex, m_stepTableSize);
        m_infinityDAC = m_pActuatorDriverData->tunedParams.initialCode;
        m_macroDAC    = m_stepTable[j];
        result        = CamxResultSuccess;

        if ((NULL != pAFData) && (0 != pAFData->numberOfDistances))
        {
            BOOL   isAscending  = (m_stepTable[0] < m_stepTable[m_stepTableSize - 1]) ? TRUE : FALSE;
            UINT   stepPosition = 0;
            FLOAT  factor       = 1;

            if (0 != pRegionParams[0].codePerStep)
            {
                factor = (1 / pRegionParams[0].codePerStep);
            }

            for (UINT i = 0; i < pAFData->numberOfDistances ; i++)
            {
                INT16 DAC = pAFData->calibrationInfo[i].DACValue;

                stepPosition = Utils::FindClosestInSortedIntArray(m_stepTable, m_stepTableSize, DAC);
                if (TRUE == isAscending)
                {
                    if (pAFData->calibrationInfo[i].DACValue < m_stepTable[stepPosition])
                    {
                        pAFData->calibrationInfo[i].stepPosition =
                            (stepPosition) - ((abs(m_stepTable[stepPosition]) - abs(DAC)) * (abs(factor)));
                    }
                    else
                    {
                        pAFData->calibrationInfo[i].stepPosition =
                            (stepPosition) + ((abs(DAC) - abs(m_stepTable[stepPosition])) * (abs(factor)));
                    }
                }
                else
                {
                    if (pAFData->calibrationInfo[i].DACValue < m_stepTable[stepPosition])
                    {
                        pAFData->calibrationInfo[i].stepPosition =
                            (stepPosition) + ((abs(m_stepTable[stepPosition]) - abs(DAC)) * (abs(factor)));
                    }
                    else
                    {
                        pAFData->calibrationInfo[i].stepPosition =
                            stepPosition - ((abs(DAC) - abs(m_stepTable[stepPosition])) * (abs(factor)));
                    }
                }
                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                               "calibration Distance: %d, DAC: %d, stepPosition: %f",
                               pAFData->calibrationInfo[i].chartDistanceCM,
                               pAFData->calibrationInfo[i].DACValue,
                               pAFData->calibrationInfo[i].stepPosition);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to initilize step table");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::GetMoveFocusCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT ActuatorData::GetMoveFocusCmdSize(
    UINT16 additionalDelay)
{
    ActuatorRegConfig* pRegConfigTable = &m_pActuatorDriverData->registerConfig;
    UINT               totalSize       = 0;

    for (UINT i = 0; i < pRegConfigTable->registerParamCount; i++)
    {
        UINT16 delayMicroseconds = static_cast<UINT16>(pRegConfigTable->registerParam[i].delayUs);

        if (i == static_cast<UINT>(pRegConfigTable->registerParamCount - 1))
        {
            delayMicroseconds += additionalDelay;
        }

        switch (pRegConfigTable->registerParam[i].operation)
        {
            case ActuatorOperation::POLL:
                totalSize += sizeof(CSLSensorWaitConditionalCmd);
                break;
            case ActuatorOperation::READ_WRITE:
                totalSize += sizeof(CSLSensorI2CRandomReadCmd);
                totalSize += sizeof(CSLSensorI2CRandomWriteCmd);
                if (delayMicroseconds > 0)
                {
                    totalSize += sizeof(CSLSensorWaitCmd);
                }
                break;
            case ActuatorOperation::WRITE_DAC_VALUE:
            case ActuatorOperation::WRITE_DIR_REG:
            case ActuatorOperation::WRITE_HW_DAMP:
            case ActuatorOperation::WRITE:
                totalSize += sizeof(CSLSensorI2CRandomWriteCmd);
                if (delayMicroseconds > 0)
                {
                    totalSize += sizeof(CSLSensorWaitCmd);
                }
                break;
            default:
                CAMX_LOG_ERROR(
                    CamxLogGroupSensor,
                    "Unhandled actuator operation reg-index:%d operation:%d",
                    i,
                    pRegConfigTable->registerParam[i].operation);
                break;
        }
    }
    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::StepToDAC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT16 ActuatorData::StepToDAC(
    const UINT targetPositionStep)
{
    INT16 DACCode = 0;

    CAMX_ASSERT_MESSAGE(targetPositionStep < MaxSteps, "Target step: %d is greater than max step: %d",
                        targetPositionStep, MaxSteps);
    if (targetPositionStep < MaxSteps)
    {
        DACCode = m_stepTable[targetPositionStep];
    }
    else
    {
        DACCode = m_stepTable[MaxSteps-1];
    }

    return DACCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::CreateI2CInfoCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::CreateI2CInfoCmd(
    CSLSensorI2CInfo* pI2CInfoCmd)
{
    CamxResult result = CamxResultSuccess;

    result = ImageSensorUtils::CreateI2CInfoCmd(pI2CInfoCmd,
                                                m_pActuatorDriverData->slaveInfo.slaveAddress,
                                                m_pActuatorDriverData->slaveInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ActuatorData::GetPowerSequenceCmdSize(
    BOOL isPowerUp)
{
    UINT            powerSequenceSize        = 0;
    UINT            powerSequenceCommandSize = 0;
    PowerSetting*   pPowerSettings           = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pActuatorDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pActuatorDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pActuatorDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pActuatorDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    if ((0 != powerSequenceSize) && (NULL != pPowerSettings))
    {
        powerSequenceCommandSize =  ImageSensorUtils::GetPowerSequenceCmdSize(powerSequenceSize, pPowerSettings);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Power settings are not valid. Will use default settings");
    }

    return powerSequenceCommandSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::CreatePowerSequenceCmd(
    BOOL    isPowerUp,
    VOID*   pCmdBuffer)
{
    UINT            powerSequenceSize   = 0;
    PowerSetting*   pPowerSettings      = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pActuatorDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pActuatorDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pActuatorDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pActuatorDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::CreatePowerSequenceCmd(pCmdBuffer, isPowerUp, powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::GetInitializeCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ActuatorData::GetInitializeCmdSize()
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    settingsCount = m_pActuatorDriverData->initSettings.regSettingCount;
    pSettings     = m_pActuatorDriverData->initSettings.regSetting;

    totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::CreateInitializeCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::CreateInitializeCmd(
    VOID*   pCmdBuffer)
{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    settingsCount = m_pActuatorDriverData->initSettings.regSettingCount;
    pSettings     = m_pActuatorDriverData->initSettings.regSetting;

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                         settingsCount,
                                                         pSettings,
                                                         0,
                                                         m_pActuatorDriverData->slaveInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::GetMaxMoveFocusCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ActuatorData::GetMaxMoveFocusCmdSize()
{
    ActuatorRegConfig* pRegConfigTable = &m_pActuatorDriverData->registerConfig;
    UINT               totalSize       = 0;

    for (UINT i = 0; i < pRegConfigTable->registerParamCount; i++)
    {

        if (pRegConfigTable->registerParam[i].operation == ActuatorOperation::POLL)
        {
            totalSize += sizeof(CSLSensorWaitConditionalCmd);
        }
        else if (pRegConfigTable->registerParam[i].operation == ActuatorOperation::READ_WRITE)
        {
            totalSize += sizeof(CSLSensorI2CRandomReadCmd);
            totalSize += sizeof(CSLSensorI2CRandomWriteCmd);

            if (pRegConfigTable->registerParam[i].delayUs > 0)
            {
                totalSize += sizeof(CSLSensorWaitCmd);
            }
        }
        else
        {
            totalSize += sizeof(CSLSensorI2CRandomWriteCmd);

            if (pRegConfigTable->registerParam[i].delayUs > 0)
            {
                totalSize += sizeof(CSLSensorWaitCmd);
            }
        }
    }
    // Allow additional delay added in MoveFocus
    totalSize += sizeof(CSLSensorWaitCmd);
    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActuatorData::CreateMoveFocusCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ActuatorData::CreateMoveFocusCmd(
    INT          targetPosition,
    PositionUnit unit,
    VOID*        pCmdBuffer,
    UINT16       additionalDelay)
{
    CamxResult                   result             = CamxResultSuccess;
    CSLSensorI2CRandomWriteCmd*  pWriteCmd          = NULL;
    CSLSensorWaitConditionalCmd* pPollCmd           = NULL;
    CSLSensorWaitCmd*            pWaitCmd           = NULL;
    UINT                         offset             = 0;
    ActuatorRegConfig*           pRegConfigTable    = &m_pActuatorDriverData->registerConfig;
    INT16                        targetPositionDAC  = 0;
    UINT                         targetPositionStep = InvalidLensData;

    if (unit == PositionUnit::Step)
    {
        targetPositionDAC  = StepToDAC(static_cast<UINT>(targetPosition));
        targetPositionStep = targetPosition;
    }
    else if (unit == PositionUnit::DAC)
    {
        targetPositionDAC = static_cast<INT16>(targetPosition);
    }

    /// @todo (CAMX-673)  to optimize: only direction and target position value changes for this Cmd, the rest stays the same
    for (UINT i = 0; i < pRegConfigTable->registerParamCount; i++)
    {
        UINT16 delayMicroseconds = static_cast<UINT16>(pRegConfigTable->registerParam[i].delayUs);

        if (i == static_cast<UINT>(pRegConfigTable->registerParamCount - 1))
        {
            delayMicroseconds += additionalDelay;
        }

        switch (pRegConfigTable->registerParam[i].operation)
        {
            case ActuatorOperation::WRITE_DIR_REG:
            {
                UINT32 directionRegData = 0;
                // Determine direction by checking if the table stores Macro or Infinity at the 0 or largest index
                if ((targetPositionStep > m_currentPositionStep) == (MacroIndex > InfinityIndex))
                {
                    // Move to macro
                    directionRegData = m_pActuatorDriverData->tunedParams.forwardDamping.
                        scenarioDampingParams.scenario[0].region[0].hwParams;
                }
                else
                {
                    // Move to infinity
                    directionRegData = m_pActuatorDriverData->tunedParams.backwardDamping.
                        scenarioDampingParams.scenario[0].region[0].hwParams;
                }

                pWriteCmd                  = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>
                                                (static_cast<BYTE*>(pCmdBuffer) + offset);
                pWriteCmd->header.count    = 1;
                pWriteCmd->header.opcode   = CSLSensorI2COpcodeRandomWrite;
                pWriteCmd->header.cmdType  = CSLSensorCmdTypeI2CRandomRegWrite;
                pWriteCmd->header.dataType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regDataType);
                pWriteCmd->header.addrType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regAddrType);

                pWriteCmd->regValPairs[0].reg = pRegConfigTable->registerParam[i].registerAddr;
                pWriteCmd->regValPairs[0].val = directionRegData;

                offset += sizeof(CSLSensorI2CRandomWriteCmd);
                break;
            }
            case ActuatorOperation::WRITE_DAC_VALUE:
                pWriteCmd                  = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>
                                             (static_cast<BYTE*>(pCmdBuffer) + offset);
                pWriteCmd->header.count    = 1;
                pWriteCmd->header.opcode   = CSLSensorI2COpcodeRandomWrite;
                pWriteCmd->header.cmdType  = CSLSensorCmdTypeI2CRandomRegWrite;
                pWriteCmd->header.dataType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regDataType);
                pWriteCmd->header.addrType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regAddrType);

                pWriteCmd->regValPairs[0].reg = pRegConfigTable->registerParam[i].registerAddr;
                pWriteCmd->regValPairs[0].val = targetPositionDAC << pRegConfigTable->registerParam[i].dataShift;

                offset += sizeof(CSLSensorI2CRandomWriteCmd);
                break;

            case ActuatorOperation::WRITE:
                pWriteCmd                  = reinterpret_cast<CSLSensorI2CRandomWriteCmd*>
                                             (static_cast<BYTE*>(pCmdBuffer) + offset);
                pWriteCmd->header.count    = 1;
                pWriteCmd->header.opcode   = CSLSensorI2COpcodeRandomWrite;
                pWriteCmd->header.cmdType  = CSLSensorCmdTypeI2CRandomRegWrite;
                pWriteCmd->header.dataType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regDataType);
                pWriteCmd->header.addrType = static_cast<UINT8>(pRegConfigTable->registerParam[i].regAddrType);

                pWriteCmd->regValPairs[0].reg = pRegConfigTable->registerParam[i].registerAddr;
                pWriteCmd->regValPairs[0].val = pRegConfigTable->registerParam[i].registerData;

                offset += sizeof(CSLSensorI2CRandomWriteCmd);
                break;
            case ActuatorOperation::POLL:
                pPollCmd = reinterpret_cast<CSLSensorWaitConditionalCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);

                pPollCmd->dataType            = static_cast<UINT8>(pRegConfigTable->registerParam[i].regDataType);
                pPollCmd->addrType            = static_cast<UINT8>(pRegConfigTable->registerParam[i].regAddrType);
                pPollCmd->opcode              = CSLSensorWaitOpcodeConditional;
                pPollCmd->cmdType             = CSLSensorCmdTypeWait;
                pPollCmd->timeoutMilliseconds =
                    static_cast<UINT16>(Utils::Ceiling(static_cast<FLOAT>(delayMicroseconds) / 1000.0f));
                pPollCmd->reg                 = pRegConfigTable->registerParam[i].registerAddr;
                pPollCmd->val                 = pRegConfigTable->registerParam[i].registerData;
                pPollCmd->mask                = 0xFFFF;

                offset += sizeof(CSLSensorWaitConditionalCmd);
                break;
            case ActuatorOperation::WRITE_HW_DAMP:
            case ActuatorOperation::READ_WRITE:
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported write type %d", pRegConfigTable->registerParam[i].operation);
                break;
        }

        if ((delayMicroseconds > 0) && (pRegConfigTable->registerParam[i].operation != ActuatorOperation::POLL))
        {
            pWaitCmd                    = reinterpret_cast<CSLSensorWaitCmd*>(static_cast<BYTE*>(pCmdBuffer) + offset);
            ImageSensorUtils::UpdateWaitCommand(pWaitCmd, delayMicroseconds);

            offset += sizeof(CSLSensorWaitCmd);
        }
    }

    m_currentPositionStep = targetPositionStep;
    m_currentPositionDAC  = targetPositionDAC;

    return result;
}

CAMX_NAMESPACE_END
