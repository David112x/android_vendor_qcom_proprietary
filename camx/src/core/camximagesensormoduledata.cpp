////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagesensormoduledata.cpp
/// @brief Image sensor module data class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxsdksensordriver.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camxhwenvironment.h"
#include "camxpacket.h"
#include "camxsensordriverapi.h"
#include "camxstaticcaps.h"
#include "camxeepromdata.h"
#include "camxflashdata.h"
#include "camximagesensordata.h"
#include "camxactuatordata.h"
#include "camxoisdata.h"
#include "camxpdafdata.h"
#include "camximagesensorutils.h"
#include "camximagesensormoduledata.h"
#include "camxcawbioutil.h"

CAMX_NAMESPACE_BEGIN

CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_BYTE_DATA)                == static_cast<INT>(CSLSensorI2CDataTypeByte));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_WORD_DATA)                == static_cast<INT>(CSLSensorI2CDataTypeWord));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_DWORD_DATA)               == static_cast<INT>(CSLSensorI2CDataTypeDword));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_SET_BYTE_MASK)            == static_cast<INT>(CSLSensorI2CDataTypeSetByteMask));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_UNSET_BYTE_MASK)          ==
                   static_cast<INT>(CSLSensorI2CDataTypeUnsetByteMask));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_SET_WORD_MASK)            == static_cast<INT>(CSLSensorI2CDataTypeSetWordMask));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_UNSET_WORD_MASK)          ==
                   static_cast<INT>(CSLSensorI2CDataTypeUnsetWordMask));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA) ==
                   static_cast<INT>(CSLSensorI2CDataTypeSetByteWriteMask));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_BYTE_ADDR)                == static_cast<INT>(CSLSensorI2CTypeByte));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_WORD_ADDR)                == static_cast<INT>(CSLSensorI2CTypeWord));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_3B_ADDR)                  == static_cast<INT>(CSLSensorI2CType3Byte));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_I2C_DWORD_ADDR)               == static_cast<INT>(CSLSensorI2CTypeDword));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_MCLK)          == static_cast<INT>(CSLSensorSequenceMCLK));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_VANA)          == static_cast<INT>(CSLSensorSequenceVANA));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_VDIG)          == static_cast<INT>(CSLSensorSequenceVDIG));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_VIO)           == static_cast<INT>(CSLSensorSequenceVIO));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_VAF)           == static_cast<INT>(CSLSensorSequenceVAF));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_VAFPWDN)       == static_cast<INT>(CSLSensorSequenceVAFPWDN));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_CUSTOMREG1)    == static_cast<INT>(CSLSensorSequenceCustomReg1));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_CUSTOMREG2)    == static_cast<INT>(CSLSensorSequenceCustomReg2));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_RESET)         == static_cast<INT>(CSLSensorSequenceReset));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_STANDBY)       == static_cast<INT>(CSLSensorSequenceStandby));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_CUSTOMGPIO1)   == static_cast<INT>(CSLSensorSequenceCustomGPIO1));
CAMX_STATIC_ASSERT(static_cast<INT>(CAMERA_POWER_SEQUENCE_CUSTOMGPIO2)   == static_cast<INT>(CSLSensorSequenceCustomGPIO2));


static const UINT32 LensShadingMapSizeWidth  = 17;
static const UINT32 LensShadingMapSizeHeight = 13;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::Create(
    ImageSensorModuleDataCreateData* pCreateData)
{
    CamxResult              result    = CamxResultEInvalidArg;
    ImageSensorModuleData*  pInstance = NULL;

    if (NULL != pCreateData)
    {
        pInstance = CAMX_NEW ImageSensorModuleData(pCreateData->pSensorModuleManagerObj);

        if (NULL != pInstance)
        {
            pInstance->m_pDataManager = pCreateData->pDataManager;
            result = pInstance->Initialize();

            if (CamxResultSuccess != result)
            {
                CAMX_DELETE pInstance;
                pInstance = NULL;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "No memory for ImageSensorModuleData creation!");
            result = CamxResultENoMemory;
        }

        pCreateData->pImageSensorModuleData = pInstance;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::ImageSensorModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSensorModuleData::ImageSensorModuleData(
    ImageSensorModuleSetManager* pSensorModuleManagerObj)
    : m_pSensorModuleManagerObj(pSensorModuleManagerObj)
    , m_pSensorData(NULL)
    , m_pActuatorData(NULL)
{
    UINT32 moduleConfigurationCount = GetCameraModuleDataObj()->moduleGroup.moduleConfigurationCount;

    if (0 != moduleConfigurationCount)
    {
        for (UINT32 i = 0; i < moduleConfigurationCount; i++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                         "module[%d] : cameraId %d chromatix name %s", i,
                         GetCameraModuleDataObj()->moduleGroup.moduleConfiguration[i].cameraId,
                         GetCameraModuleDataObj()->moduleGroup.moduleConfiguration[i].chromatixName);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetActuatorDriverDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActuatorDriverData* ImageSensorModuleData::GetActuatorDriverDataObj()
{
    CHAR                symbolName[]        = "actuatorDriver";
    ActuatorDriverData* pActuatorDriverData = NULL;
    TuningMode          pSelector[1]        = { { ModeType::Default, { 0 } } };

    pActuatorDriverData = static_cast<ActuatorDriverData*>(static_cast<ActuatorDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pActuatorDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetActuatorDriverDataObj1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActuatorDriverData* ImageSensorModuleData::GetActuatorDriverDataObj1()
{
    CHAR                symbolName1[]       = "actuatorDriver1";
    ActuatorDriverData* pActuatorDriverData = NULL;
    TuningMode          pSelector[1]        = { { ModeType::Default, { 0 } } };

    pActuatorDriverData = static_cast<ActuatorDriverData*>(static_cast<ActuatorDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName1, &pSelector[0], 1)));

    return pActuatorDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetOisDriverDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OISDriverData* ImageSensorModuleData::GetOisDriverDataObj()
{
    CHAR                symbolName[]        = "OISDriver";
    OISDriverData* pOisDriverData           = NULL;
    TuningMode   pSelector[1] = { { ModeType::Default, { 0 } } };

    pOisDriverData = static_cast<OISDriverData*>(static_cast<OISDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pOisDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetEEPROMDriverDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMDriverData* ImageSensorModuleData::GetEEPROMDriverDataObj()
{
    CHAR                symbolName[]        = "EEPROMDriverData";
    EEPROMDriverData*   pEEPROMDriverData   = NULL;
    TuningMode   pSelector[1] = { { ModeType::Default, { 0 } } };

    pEEPROMDriverData = static_cast<EEPROMDriverData*>(static_cast<EEPROMDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pEEPROMDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetSensorDriverDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorDriverData* ImageSensorModuleData::GetSensorDriverDataObj()
{
    CHAR              symbolName[]      = "sensorDriverData";
    SensorDriverData* pSensorDriverData = NULL;
    TuningMode   pSelector[1] = { { ModeType::Default, { 0 } } };

    pSensorDriverData = static_cast<SensorDriverData*>(static_cast<SensorDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pSensorDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetFlashDriverDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FlashDriverData* ImageSensorModuleData::GetFlashDriverDataObj()
{
    CHAR              symbolName[]    = "flashDriverData";
    FlashDriverData* pFlashDriverData = NULL;

    pFlashDriverData = static_cast<FlashDriverData*>(static_cast<FlashDriverDataClass*>(
        m_pSensorModuleManagerObj->GetModule(symbolName, 0, 0)));

    return pFlashDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetCameraModuleDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CameraModuleData* ImageSensorModuleData::GetCameraModuleDataObj()
{
    CHAR              symbolName[]      = "cameraModuleData";
    CameraModuleData* pCameraModuleData = NULL;
    TuningMode   pSelector[1] = { { ModeType::Default, { 0 } } };

    pCameraModuleData = static_cast<CameraModuleData*>(
            static_cast<CameraModuleDataClass*>(m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pCameraModuleData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetPDAFConfigDataObj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PDAFConfigurationData* ImageSensorModuleData::GetPDAFConfigDataObj() const
{
    CHAR                   symbolName[]  = "PDConfigData";
    PDAFConfigurationData* pPdConfigData = NULL;
    TuningMode   pSelector[1] = { { ModeType::Default, { 0 } } };

    pPdConfigData = static_cast<PDAFConfigurationData*>(
        static_cast<PDAFConfigurationDataClass*>(m_pSensorModuleManagerObj->GetModule(symbolName, &pSelector[0], 1)));

    return pPdConfigData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetCSIInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CSIInformation* ImageSensorModuleData::GetCSIInfo() const
{
    const CSIInformation* pCSIInfo = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration)
    {
        pCSIInfo = &(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].CSIInfo);
    }

    return pCSIInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetLensInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const LensInformation* ImageSensorModuleData::GetLensInfo()
{
    const LensInformation* pLensInfo = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration)
    {
        pLensInfo = &(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].lensInfo);
    }

    return pLensInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetOverrideSensorSlaveAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const UINT16* ImageSensorModuleData::GetOverrideSensorSlaveAddress() const
{
    const UINT16* pSlaveAddressPtr = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration &&
        m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].sensorSlaveAddressExists)
    {
        pSlaveAddressPtr =
            static_cast<UINT16*>(&(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].sensorSlaveAddress));
    }

    return pSlaveAddressPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetOverrideEepromSlaveAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const UINT16* ImageSensorModuleData::GetOverrideEepromSlaveAddress() const
{
    const UINT16* pSlaveAddressPtr = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration &&
        m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].eepromSlaveAddressExists)
    {
        pSlaveAddressPtr =
            static_cast<UINT16*>(&(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].eepromSlaveAddress));
        CAMX_LOG_INFO(CamxLogGroupSensor, "slave address to override : %x ", *pSlaveAddressPtr);
    }

    return pSlaveAddressPtr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetChromatixName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* ImageSensorModuleData::GetChromatixName()
{
    const CHAR* pName = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration &&
        m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].chromatixNameExists)
    {
        pName = m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].chromatixName;
    }

    return pName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetOverrideI2CFrequencyMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const I2CFrequencyMode* ImageSensorModuleData::GetOverrideI2CFrequencyMode()
{
    const I2CFrequencyMode* pMode = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration &&
        m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].i2cFrequencyModeExists)
    {
        pMode = &m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].i2cFrequencyMode;
        CAMX_LOG_INFO(CamxLogGroupSensor, "I2CFrenquencyMode 0x%x, mode:%d", pMode, *pMode)
    }

    return pMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetActuatorSensitivity
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT ImageSensorModuleData::GetActuatorSensitivity()
{
    FLOAT actuatorSensitivity = 0.0f;

    if (NULL != m_pActuatorData)
    {
        actuatorSensitivity = m_pActuatorData->GetSensitivity();
    }
    return actuatorSensitivity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::GetCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::GetCameraId(
    UINT16* pCameraId
    ) const
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration)
    {
        *pCameraId = m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].cameraId;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "m_pCameraModuleData->moduleGroup.moduleConfiguration is NULL");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageSensorModuleData::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::Initialize()
{
    CamxResult        result                   = CamxResultSuccess;
    SensorDriverData* pSensorDriverData        = NULL;
    const CHAR*       pSensorName              = NULL;
    UINT32            moduleConfigurationCount = GetCameraModuleDataObj()->moduleGroup.moduleConfigurationCount;

    CAMX_ASSERT(NULL != m_pSensorModuleManagerObj);

    if (NULL == m_pSensorModuleManagerObj)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid input!");
        result = CamxResultEInvalidPointer;
    }
    else
    {
        pSensorDriverData = GetSensorDriverDataObj();
        if (NULL == pSensorDriverData)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot get sensor data object from module");
            result = CamxResultENoSuch;
        }
        else
        {
            if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->forceDebugQCFASWRemosaic)
            {
                pSensorDriverData->resolutionInfo->resolutionData[0].capability[0] = SensorCapability::QUADCFA;
                pSensorDriverData->resolutionInfo->resolutionData[0].RemosaicTypeInfoExists = TRUE;
                pSensorDriverData->resolutionInfo->resolutionData[0].RemosaicTypeInfo = RemosaicType::SWRemosaic;
                CAMX_LOG_INFO(CamxLogGroupSensor, "Adding QCFA Capability with SWRemosaic for Mode 0 due to override setting");
            }
            m_pSensorData = CAMX_NEW ImageSensorData(pSensorDriverData, this);
            if (NULL == m_pSensorData)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Couldn't create ImageSensorData!");
                result = CamxResultENoMemory;
            }
        }

        m_pCameraModuleData = GetCameraModuleDataObj();
        if (NULL == m_pCameraModuleData)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot find camera module data to create ImageSensorData");
            result = CamxResultENoSuch;
        }

        if (CamxResultSuccess == result)
        {
            m_usedModuleId = 0;
            pSensorName    = m_pSensorData->GetSensorName();
            for (; m_usedModuleId < moduleConfigurationCount; m_usedModuleId++)
            {
                CHAR* pName = GetCameraModuleDataObj()->moduleGroup.moduleConfiguration[m_usedModuleId].sensorName;

                if (0 == OsUtils::StrCmp(pSensorName, pName))
                {
                    break;
                }
            }

            if (m_usedModuleId >= moduleConfigurationCount)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot match sensor: %s and module.", pSensorName);
                result = CamxResultENoSuch;
            }
            else
            {
                if (GetOverrideI2CFrequencyMode())
                {
                    pSensorDriverData->slaveInfo.i2cFrequencyMode = m_pCameraModuleData->moduleGroup
                                                        .moduleConfiguration[m_usedModuleId].i2cFrequencyMode;
                }
            }
        }

        m_pPDAFData = CAMX_NEW PDAFData(GetPDAFConfigDataObj());
        if (NULL == m_pPDAFData)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Couldn't create PDAF Sensor Data!");
            result = CamxResultENoMemory;
        }

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleData::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::~ImageSensorModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSensorModuleData::~ImageSensorModuleData()
{

    if (NULL != m_pActuatorData)
    {
        CAMX_DELETE m_pActuatorData;
        m_pActuatorData = NULL;
    }

    if (NULL != m_pOISData)
    {
        CAMX_DELETE m_pOISData;
        m_pOISData = NULL;
    }

    if (NULL != m_pSensorData)
    {
        CAMX_DELETE m_pSensorData;
        m_pSensorData = NULL;
    }

    if (NULL != m_pFlashData)
    {
        CAMX_DELETE m_pFlashData;
        m_pFlashData = NULL;
    }

    if (NULL != m_pCameraModuleData)
    {
        m_pCameraModuleData = NULL;
    }

    if (NULL != m_pPDAFData)
    {
        CAMX_DELETE m_pPDAFData;
        m_pPDAFData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::Probe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::Probe(
    BOOL*   pDetected,
    INT32*  pDeviceIndex)
{
    CamxResult                  result                   = CamxResultEInvalidArg;
    CSLImageSensorProbeResult   probeResult              = {0};
    CmdBufferManager*           pPacketManager           = NULL;
    CmdBufferManager*           pCmdManagerSlaveInfo     = NULL;
    CmdBufferManager*           pCmdManagerPowerSequence = NULL;
    Packet*                     pProbePacket             = NULL;
    CmdBuffer*                  pSlaveInfoCmd            = NULL;
    CmdBuffer*                  pPowerSequenceCmd        = NULL;
    ResourceParams              packetResourceParams     = {0};
    ResourceParams              cmdResourceParams        = {0};
    VOID*                       pCmdBegin                = NULL;

    UINT I2CInfoCmdSize     = sizeof(CSLSensorI2CInfo);
    UINT probeCmdSize       = sizeof(CSLSensorProbeCmd);
    UINT powerUpCmdSize     = GetSensorDataObject()->GetPowerSequenceCmdSize(TRUE);
    UINT powerDownCmdSize   = GetSensorDataObject()->GetPowerSequenceCmdSize(FALSE);
    UINT slaveInfoSize      = (I2CInfoCmdSize + probeCmdSize);
    UINT powerSequenceSize  = (powerUpCmdSize + powerDownCmdSize);

    CAMX_LOG_INFO(CamxLogGroupSensor, "slaveInfoSize: %d, powerSequenceSize: %d", slaveInfoSize, powerSequenceSize);

    packetResourceParams.usageFlags.packet                = 1;
    packetResourceParams.packetParams.maxNumCmdBuffers    = 2;
    packetResourceParams.packetParams.maxNumIOConfigs     = 0;
    packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
    packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                   = NULL;
    packetResourceParams.numDevices                       = 0;
    packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CmdBufferManager::Create("ImageSensorPacketManager", &packetResourceParams, &pPacketManager);

    if (CamxResultSuccess == result)
    {
        cmdResourceParams.resourceSize         = slaveInfoSize;
        cmdResourceParams.poolSize             = 1 * slaveInfoSize;
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.cmdParams.type       = CmdType::Legacy;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("ImageSensorCmdManagerSlaveInfo", &cmdResourceParams, &pCmdManagerSlaveInfo);

        if (CamxResultSuccess == result)
        {
            cmdResourceParams.resourceSize         = powerSequenceSize;
            cmdResourceParams.poolSize             = 1 * powerSequenceSize;
            cmdResourceParams.usageFlags.cmdBuffer = 1;
            cmdResourceParams.cmdParams.type       = CmdType::I2C;
            cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
            cmdResourceParams.pDeviceIndices       = NULL;
            cmdResourceParams.numDevices           = 0;
            cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

            result = CmdBufferManager::Create("ImageSensorCmdManagerPowerSequence", &cmdResourceParams,
                                              &pCmdManagerPowerSequence);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Power Sequence cmdbuffermanager");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Slave Info cmdbuffermanager");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Packet Manager cmdbuffermanager");
    }

    PacketResource* pPacketResource        = NULL;

    if (CamxResultSuccess == result)
    {
        if (CamxResultSuccess == pCmdManagerSlaveInfo->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

            // We know pResource actually points to a CmdBuffer so we may static_cast
            pSlaveInfoCmd = static_cast<CmdBuffer*>(pPacketResource);
        }

        if (CamxResultSuccess == pCmdManagerPowerSequence->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);

            // We know pResource actually points to a CmdBuffer so we may static_cast
            pPowerSequenceCmd = static_cast<CmdBuffer*>(pPacketResource);
        }

        if (CamxResultSuccess == pPacketManager->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);

            // We know pResource actually points to a Packet so we may static_cast
            pProbePacket = static_cast<Packet*>(pPacketResource);
        }

        if ((NULL == pProbePacket)  || (NULL == pSlaveInfoCmd) || (NULL == pPowerSequenceCmd))
        {
            result = CamxResultENoMemory;
        }
        else
        {
            pCmdBegin = pSlaveInfoCmd->BeginCommands(I2CInfoCmdSize / sizeof(UINT32));
            if (NULL != pCmdBegin)
            {
                CSLSensorI2CInfo* pCmdI2CInfo = reinterpret_cast<CSLSensorI2CInfo*>(pCmdBegin);

                result = GetSensorDataObject()->CreateI2CInfoCmd(pCmdI2CInfo);
                if (CamxResultSuccess == result)
                {
                    result = pSlaveInfoCmd->CommitCommands();
                }
            }

            if (CamxResultSuccess == result)
            {
                pCmdBegin = pSlaveInfoCmd->BeginCommands(probeCmdSize / sizeof(UINT32));
                if (NULL != pCmdBegin)
                {
                    CSLSensorProbeCmd* pCmdProbe = reinterpret_cast<CSLSensorProbeCmd*>(pCmdBegin);
                    UINT16             cameraId;
                    result = GetCameraId(&cameraId);

                    if (CamxResultSuccess == result)
                    {
                        result = GetSensorDataObject()->CreateProbeCmd(pCmdProbe, cameraId);
                        if (CamxResultSuccess == result)
                        {
                            result = pSlaveInfoCmd->CommitCommands();
                        }
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                pCmdBegin = pPowerSequenceCmd->BeginCommands(powerUpCmdSize / sizeof(UINT32));
                if (NULL != pCmdBegin)
                {
                    VOID* pCmdPowerUp = pCmdBegin;

                    result = GetSensorDataObject()->CreatePowerSequenceCmd(TRUE, pCmdPowerUp);
                    if (CamxResultSuccess == result)
                    {
                        result = pPowerSequenceCmd->CommitCommands();
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                pCmdBegin = pPowerSequenceCmd->BeginCommands(powerDownCmdSize / sizeof(UINT32));
                if (NULL != pCmdBegin)
                {
                    VOID* pCmdPowerDown = pCmdBegin;

                    result = GetSensorDataObject()->CreatePowerSequenceCmd(FALSE, pCmdPowerDown);
                    if (CamxResultSuccess == result)
                    {
                        result = pPowerSequenceCmd->CommitCommands();
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                // Not associated with any request. Won't be recycled.
                pSlaveInfoCmd->SetRequestId(CamxInvalidRequestId);
                pPowerSequenceCmd->SetRequestId(CamxInvalidRequestId);

                // Not associated with any request. Won't be recycled.
                pProbePacket->SetRequestId(CamxInvalidRequestId);

                pProbePacket->SetOpcode(CSLDeviceTypeImageSensor, CSLPacketOpcodesSensorProbe);

                result = pProbePacket->AddCmdBufferReference(pSlaveInfoCmd, NULL);
                if (CamxResultSuccess == result)
                {
                    result = pProbePacket->AddCmdBufferReference(pPowerSequenceCmd, NULL);
                }
            }

            if (CamxResultSuccess == result)
            {
                result = pProbePacket->CommitPacket();
            }
        }

        if (CamxResultSuccess == result)
        {
            result = CSLImageSensorProbe(pProbePacket->GetMemHandle(), pProbePacket->GetOffset(), &probeResult);
            if ((CamxResultSuccess == result) &&
                (NULL != pDetected)           &&
                (NULL != pDeviceIndex))
            {
                *pDetected      = probeResult.detected;
                *pDeviceIndex   = probeResult.deviceIndex;
                CAMX_LOG_INFO(CamxLogGroupSensor,
                            "Probe success results - Detected: %d, DeviceIndex: %d,sensorname:%s",
                            *pDetected,
                            *pDeviceIndex,
                            GetSensorDataObject()->GetSensorName());
                result = GetSensorDataObject()->LoadSensorLibrary();
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupSensor,
                            "Probe failed- sensorname:%s,", GetSensorDataObject()->GetSensorName());
            }
        }
    }

    if (NULL != pPacketManager)
    {
        pPacketManager->Recycle(pProbePacket);
        CAMX_DELETE pPacketManager;
    }
    if (NULL != pCmdManagerSlaveInfo)
    {
        pCmdManagerSlaveInfo->Recycle(pSlaveInfoCmd);
        CAMX_DELETE pCmdManagerSlaveInfo;
    }
    if (NULL != pCmdManagerPowerSequence)
    {
        pCmdManagerPowerSequence->Recycle(pPowerSequenceCmd);
        CAMX_DELETE pCmdManagerPowerSequence;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::CreateSensorSubModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleData::CreateSensorSubModules(
    HwSensorInfo*           pSensorInfoTable,
    const HwDeviceTypeInfo* pCSLDeviceTable)
{
    ActuatorDriverData* pActuatorDriverData  = NULL;
    ActuatorDriverData* pActuatorDriverData1 = NULL;
    FlashDriverData*    pFlashDriverData     = NULL;
    OISDriverData*      pOISDriverData       = NULL;
    UINT16              cameraId             = 0;
    CamxResult          result               = CamxResultSuccess;

    if (CamxResultSuccess == GetCameraId(&cameraId))
    {
        result = CreateAndReadEEPROMData(pSensorInfoTable, &pCSLDeviceTable[CSLDeviceTypeEEPROM], 0);

        pActuatorDriverData  = ImageSensorModuleData::GetActuatorDriverDataObj();
        pActuatorDriverData1 = ImageSensorModuleData::GetActuatorDriverDataObj1();

        if ((NULL != pActuatorDriverData) && (NULL != pActuatorDriverData1))
        {
            if ((TRUE == pActuatorDriverData1->slaveInfo.actuatorIDExists) &&
                (pActuatorDriverData1->slaveInfo.actuatorID == pSensorInfoTable->moduleCaps.OTPData.AFCalibration.actuatorID))
            {
                pActuatorDriverData = pActuatorDriverData1;
                CAMX_LOG_INFO(CamxLogGroupSensor,
                              "Multiple actuators found on camera %d and selected second actuator with ID: %d",
                              cameraId,
                              pActuatorDriverData1->slaveInfo.actuatorID);
            }
        }

        if (NULL != pActuatorDriverData)
        {
            m_pActuatorData = CAMX_NEW ActuatorData(pActuatorDriverData);
            if (NULL == m_pActuatorData)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create actuator data module on camera %d", cameraId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupSensor,
                              "Actuator device found on camera %d, name:%s",
                              cameraId,
                              pActuatorDriverData->slaveInfo.actuatorName);
                result = m_pActuatorData->InitializeStepTable(&(pSensorInfoTable->moduleCaps.OTPData.AFCalibration));
                if (result != CamxResultSuccess)
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "Actuator step table failed on camera %d", cameraId);
                }

                if (GetOverrideI2CFrequencyMode())
                {
                    pActuatorDriverData->slaveInfo.i2cFrequencyMode = m_pCameraModuleData->moduleGroup
                                                        .moduleConfiguration[m_usedModuleId].i2cFrequencyMode;
                }
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Actuator driver data not found, skip Actuator setup camera %d", cameraId);
        }

        pOISDriverData = ImageSensorModuleData::GetOisDriverDataObj();
        pSensorInfoTable->moduleCaps.hasOIS = FALSE;
        if (NULL != pOISDriverData)
        {
            m_pOISData = CAMX_NEW OISData(pOISDriverData);
            if (NULL == m_pOISData)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create OIS data module camera %d", cameraId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupSensor, "OIS device found on camera %d", cameraId);
                pSensorInfoTable->moduleCaps.hasOIS = TRUE;
                if (GetOverrideI2CFrequencyMode())
                {
                    pOISDriverData->slaveInfo.i2cFrequencyMode = m_pCameraModuleData->moduleGroup
                                                        .moduleConfiguration[m_usedModuleId].i2cFrequencyMode;
                }
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "OIS driver data not found, skip OIS setup camera %d", cameraId);
        }

        pFlashDriverData                      = ImageSensorModuleData::GetFlashDriverDataObj();
        pSensorInfoTable->moduleCaps.hasFlash = FALSE;
        m_pFlashData                          = NULL;

        if (NULL != pFlashDriverData)
        {
            m_pFlashData = CAMX_NEW FlashData(pFlashDriverData, pSensorInfoTable, &pCSLDeviceTable[CSLDeviceTypeFlash]);
            if (NULL == m_pFlashData)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to initialize Flash data module")
            }
            else
            {
                /// Check if kernel device is available
                pSensorInfoTable->moduleCaps.hasFlash = TRUE;
                if (GetOverrideI2CFrequencyMode())
                {
                    pFlashDriverData->i2cInfo.i2cFrequencyMode = m_pCameraModuleData->moduleGroup
                                                        .moduleConfiguration[m_usedModuleId].i2cFrequencyMode;
                }
                CAMX_LOG_INFO(CamxLogGroupSensor, "Flash device found on camera %d", cameraId)
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupSensor, "Flash driver data not found, sensor %d have no flash support", cameraId)
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::CreateAndReadEEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::CreateAndReadEEPROMData(
    HwSensorInfo*           pSensorInfoTable,
    const HwDeviceTypeInfo* pCSLDeviceTable,
    CSLHandle               hCSL)
{
    CamxResult          result              = CamxResultSuccess;
    EEPROMDriverData*   pEEPROMDriverData   = NULL;

    pEEPROMDriverData = ImageSensorModuleData::GetEEPROMDriverDataObj();
    if (NULL != pEEPROMDriverData)
    {
        const UINT16* pEepromSlaveAddressPtr = GetOverrideEepromSlaveAddress();
        if (pEepromSlaveAddressPtr != NULL)
        {
            UINT32 i;
            pEEPROMDriverData->slaveInfo.slaveAddress = *pEepromSlaveAddressPtr;
            for (i = 0; i < pEEPROMDriverData->memoryMap.regSettingCount; i++)
            {
                if (pEEPROMDriverData->memoryMap.regSetting[i].slaveAddrExists)
                {
                    pEEPROMDriverData->memoryMap.regSetting[i].slaveAddr = *pEepromSlaveAddressPtr;
                }
            }
        }
        EEPROMData* pEEPROMData = CAMX_NEW EEPROMData(pEEPROMDriverData,
                                                      pSensorInfoTable,
                                                      pCSLDeviceTable,
                                                      hCSL);
        if (NULL != pEEPROMData)
        {
            CAMX_DELETE pEEPROMData;
            pEEPROMData = NULL;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor,
                           "Failed to initialize EEPROM module for sensor: %s",
                           m_pSensorData->GetSensorName());
            result = CamxResultEFailed;
        }

        CAMX_ASSERT(MaxEEPROMCustomInfoCount >= pEEPROMDriverData->customInfoCount);
        Utils::Memset(&pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.customInfo, 0, sizeof(CustomInfo));
        pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.customInfoCount = pEEPROMDriverData->customInfoCount;

        for (UINT count =0; count < pEEPROMDriverData->customInfoCount; count++)
        {
            SIZE_T length = OsUtils::StrLen(pEEPROMDriverData->customInfo[count].name);

            CAMX_ASSERT(length < MaximumStringLength);
            pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.customInfo[count].value =
                pEEPROMDriverData->customInfo[count].value;
            OsUtils::StrLCpy(pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.customInfo[count].name,
                             pEEPROMDriverData->customInfo[count].name,
                             length + 1);
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Failed to get EEPROM driver data for sensor: %s",
                      m_pSensorData->GetSensorName());
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::DumpEEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleData::DumpEEPROMData(
    HwSensorInfo*           pSensorInfoTable)
{
    EEPROMDriverData* pEEPROMDriverData = NULL;

    pEEPROMDriverData = ImageSensorModuleData::GetEEPROMDriverDataObj();
    if (NULL != pEEPROMDriverData)
    {
        EEPROMDataDump* pEEPROMDataDump = CAMX_NEW EEPROMDataDump(pEEPROMDriverData,
                                                                  pSensorInfoTable);
        if (NULL != pEEPROMDataDump)
        {
            CAMX_DELETE pEEPROMDataDump;
            pEEPROMDataDump = NULL;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "Failed to dump EEPROM data for sensor %s!",
                           m_pSensorData->GetSensorName());
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Failed to get EEPROM driver data for senssor: %s",
                      m_pSensorData->GetSensorName());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetOverrideSensorMountAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageSensorModuleData::GetOverrideSensorMountAvailable()
{
    BOOL result = FALSE;
    const LensInformation* pLensInfo = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration)
    {
        pLensInfo = &(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].lensInfo);

        // 360 degree corresponds to invalid rotation angle. If one of the angle specified in the module configuration xml is
        // other than 360, we need to override that angle specified in dtsi by the angle specified in xml.
        if ((360 != pLensInfo->maxRollDegree)  ||
            (360 != pLensInfo->maxPitchDegree) ||
            (360 != pLensInfo->maxYawDegree))
        {
            result = TRUE;
        }
        else
        {
            result = FALSE;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetOverrideSensorMount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::GetOverrideSensorMount(
    UINT32* pRoll,
    UINT32* pPitch,
    UINT32* pYaw)
{
    CamxResult result = CamxResultSuccess;
    const LensInformation* pLensInfo = NULL;

    if (NULL != m_pCameraModuleData->moduleGroup.moduleConfiguration)
    {
        pLensInfo = &(m_pCameraModuleData->moduleGroup.moduleConfiguration[m_usedModuleId].lensInfo);
    }

    if ((NULL != pRoll) && (NULL != pPitch) && (NULL != pYaw) &&
        (NULL != pLensInfo) && (TRUE == GetOverrideSensorMountAvailable()))
    {
        // override only valid angles, 360 coresponds to invalid angle
        if (360 != pLensInfo->maxRollDegree)
        {
            *pRoll  = static_cast<UINT32>(pLensInfo->maxRollDegree);
        }
        if (360 != pLensInfo->maxPitchDegree)
        {
            *pPitch  = static_cast<UINT32>(pLensInfo->maxPitchDegree);
        }
        if (360 != pLensInfo->maxYawDegree)
        {
            *pYaw  = static_cast<UINT32>(pLensInfo->maxYawDegree);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetStaticCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::GetStaticCaps(
    SensorModuleStaticCaps* pCapability,
    TuningDataManager*      pTuningManager,
    UINT                    cameraID)
{
    CamxResult  result  = CamxResultEInvalidArg;

    if ((NULL != pCapability) && (NULL != pTuningManager))
    {
        FLOAT                             colorCorrectionData[9] = {0};
        awbglobalelements::awbRGBDataType MWBData                = {0};
        const LensInformation*            pLensInfo              = GetLensInfo();
        const CSIInformation*             pCSIInfo               = GetCSIInfo();
        UINT32                            numSelectors           = MaxTuningMode;
        TuningMode                        selectors[MaxTuningMode];
        FLOAT                             colorTemperature;

        UINT32               illuminantD65           = static_cast<UINT32>(EEPROMIlluminantType::D65);
        UINT32               illuminantA             = static_cast<UINT32>(EEPROMIlluminantType::A);
        WBCalibrationData*   pWBCalibrationDataD65   = &(pCapability->OTPData.WBCalibration[illuminantD65]);
        WBCalibrationData*   pWBCalibrationDataA     = &(pCapability->OTPData.WBCalibration[illuminantA]);

        Utils::Memset(&selectors[0], 0, sizeof(selectors));
        for (UINT modeIndex = 0; modeIndex < MaxTuningMode; modeIndex++)
        {
            selectors[modeIndex].mode = static_cast<ModeType>(modeIndex);
        }

        result = m_pSensorData->GetSensorStaticCapability(pCapability, cameraID);
        UINT16 sensorId       = 0xFFFF;
        GetCameraId(&sensorId);
        pCapability->sensorId = sensorId;
        if (NULL != pLensInfo)
        {
            pCapability->numFocalLengths            = 1;
            pCapability->focalLengths[0]            = static_cast<FLOAT>(pLensInfo->focalLength);
            pCapability->numAperatures              = 1;
            pCapability->aperatures[0]              = static_cast<FLOAT>(pLensInfo->fNumber);

            // mininum focus distance in diopters ( i.e reciprocal of the focus distance)
            if (0 != pLensInfo->minFocusDistance)
            {
                pCapability->minimumFocusDistance   = static_cast<FLOAT>(1.0 / (pLensInfo->minFocusDistance));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "minFocusDistance is 0");
            }

            if (0 != pCapability->focalLengths[0])
            {
                pCapability->hyperfocalDistance = (pCapability->aperatures[0] * 2 * pCapability->pixelSize) /
                                                  (pCapability->focalLengths[0] * pCapability->focalLengths[0]);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "focal length is 0");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "pLensInfo is NULL");
        }

        if (NULL != pCSIInfo)
        {
            pCapability->CSILaneAssign              = pCSIInfo->laneAssign;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "pCSIInfo is NULL");
        }

        pCapability->numNDFs                    = 1;
        pCapability->NDFs[0]                    = 0;
        pCapability->position                   = static_cast<UINT> (
                GetCameraModuleDataObj()->moduleGroup.moduleConfiguration[m_usedModuleId].position);

        if ( NULL != GetActuatorDataObject())
        {
            pCapability->isFixedFocus = FALSE;
        }
        else
        {
            pCapability->isFixedFocus = TRUE;
        }

        pCapability->hasFlash                   = (NULL == m_pFlashData)? FALSE: TRUE;
        pCapability->hasOIS                     = (NULL == m_pOISData)? FALSE: TRUE;
        pCapability->lensShadingAppliedInSensor = 1;
        pCapability->lensShadingMapSize.width   = LensShadingMapSizeWidth;
        pCapability->lensShadingMapSize.height  = LensShadingMapSizeHeight;

        pCapability->focusDistanceCalibration   = LensInfoFocusDistanceCalibrationApproximate;

        colorTemperature                        = static_cast<FLOAT>(AWBAlgoColorTemperateD65);
        result = ImageSensorUtils::GetTunedColorCorrectionData(pTuningManager,
                                                               selectors,
                                                               numSelectors,
                                                               colorCorrectionData,
                                                               sizeof(colorCorrectionData),
                                                               colorTemperature);
        if (CamxResultSuccess == result)
        {
            pCapability->referenceIlluminant1       = SensorReferenceIlluminant1D65;

            result = ImageSensorUtils::GetTunedMWBData(pTuningManager, selectors, numSelectors,
                                                       &MWBData, StatsIlluminantD65);
            if (CamxResultSuccess == result)
            {
                ImageSensorUtils::GenerateTransformMatrix(pCapability->forwardMatrix1,
                                                          pCapability->colorTransform1,
                                                          colorCorrectionData,
                                                          &MWBData,
                                                          TRUE);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain tuned MWB data 0: %d", result);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain tuned color correction data 0: %d", result);
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == pWBCalibrationDataD65->isAvailable)
            {
                ImageSensorUtils::GenerateCalibrationTransformMatrix(pCapability->calibrationTransform1,
                                                                    pWBCalibrationDataD65->rOverG,
                                                                    pWBCalibrationDataD65->bOverG);
            }
            else
            {
                ImageSensorUtils::GenerateUnitMatrix(pCapability->calibrationTransform1);
            }
        }

        colorTemperature = static_cast<FLOAT>(AWBAlgoColorTemperateIncandescent);
        result           = ImageSensorUtils::GetTunedColorCorrectionData(pTuningManager,
                                                                         selectors,
                                                                         numSelectors,
                                                                         colorCorrectionData,
                                                                         sizeof(colorCorrectionData),
                                                                         colorTemperature);
        if (CamxResultSuccess == result)
        {
            pCapability->referenceIlluminant2       = SensorReferenceIlluminant1StandardA;

            result = ImageSensorUtils::GetTunedMWBData(pTuningManager, selectors, numSelectors,
                                                       &MWBData, StatsIlluminantIncandescent);
            if (CamxResultSuccess == result)
            {
                ImageSensorUtils::GenerateTransformMatrix(pCapability->forwardMatrix2,
                                                          pCapability->colorTransform2,
                                                          colorCorrectionData,
                                                          &MWBData,
                                                          FALSE);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain tuned MWB data 1: %d", result);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed: To obtain tuned color correction data 1: %d", result);
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == pWBCalibrationDataA->isAvailable)
            {
                ImageSensorUtils::GenerateCalibrationTransformMatrix(pCapability->calibrationTransform2,
                                                                    pWBCalibrationDataA->rOverG,
                                                                    pWBCalibrationDataA->bOverG);
            }
            else
            {
                ImageSensorUtils::GenerateUnitMatrix(pCapability->calibrationTransform2);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid inputs %p, %p", pCapability, pTuningManager);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetCameraPosition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageSensorModuleData::GetCameraPosition(
    UINT* pCameraPosition)
{
    CamxResult result = CamxResultSuccess;
    if (NULL != pCameraPosition)
    {
        *pCameraPosition =
            static_cast<UINT>(GetCameraModuleDataObj()->moduleGroup.moduleConfiguration[m_usedModuleId].position);
    }
    else
    {
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::TranslateUsecaseMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseMode ImageSensorModuleData::TranslateUsecaseMode(
    INT32 type)
{
    UsecaseMode result = UsecaseMode::Invalid;

    switch (type)
    {
        case SENSOR_DEFAULT_MODE:
            result = UsecaseMode::Default;
            break;
        case SENSOR_HFR_MODE:
            result = UsecaseMode::HFR;
            break;
        case SENSOR_HDR_MODE:
            result = UsecaseMode::HDR;
            break;
        case SENSOR_RAW_HDR_MODE:
            result = UsecaseMode::RawHDR;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid type %d", type);
            break;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::TranslatePixelFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PixelFormat ImageSensorModuleData::TranslatePixelFormat(
    INT32 type)
{
    PixelFormat result = PixelFormat::Invalid;

    switch (type)
    {
        case SENSOR_BGGR:
            result = PixelFormat::BayerBGGR;
            break;
        case SENSOR_GBRG:
            result = PixelFormat::BayerGBRG;
            break;
        case SENSOR_GRBG:
            result = PixelFormat::BayerGRBG;
            break;
        case SENSOR_RGGB:
            result = PixelFormat::BayerRGGB;
            break;
        case SENSOR_Y:
            result = PixelFormat::YUVFormatY;
            break;
        case SENSOR_UYVY:
            result = PixelFormat::YUVFormatUYVY;
            break;
        case SENSOR_MONO:
            result = PixelFormat::Monochrome;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid type %d", type);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::IsExternalSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageSensorModuleData::IsExternalSensor() const
{
    return GetSensorDataObject()->IsExternalSensor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleData::GetPDAFInformation(
    UINT32    modeIndex,
    BOOL*     pIsSupported,
    PDAFType* pPDType
    ) const
{
    CamxResult                   result          = CamxResultSuccess;
    UINT32                       PDAFModeIdx     = 0;
    PDAFModeInformation*         pPDAFModeInfo   = NULL;

    if ((NULL == pIsSupported) || (NULL == pPDType) || (NULL == m_pPDAFData))
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Null pointer received");
    }

    if (CamxResultSuccess == result)
    {
        result = m_pPDAFData->GetCurrentPDAFModeIndex(modeIndex, &PDAFModeIdx);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "No PDAF Mode found for this sensor resolution %d", modeIndex);
            *pIsSupported = FALSE;
            *pPDType      = PDAFType::PDTypeUnknown;
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pPDAFData->GetPDAFModeInformation(PDAFModeIdx, &pPDAFModeInfo);
        if ((NULL != pPDAFModeInfo) && (CamxResultSuccess == result))
        {
            *pPDType      = pPDAFModeInfo->PDType;
            *pIsSupported = TRUE;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "PDAF Mode information doesn't exist for sensor resolution %d", modeIndex);
            *pIsSupported = FALSE;
            *pPDType      = PDAFType::PDTypeUnknown;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::Get3ExposureHDRInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageSensorModuleData::Get3ExposureHDRInformation(
    UINT32                  modeIndex,
    BOOL*                   pIsSupported,
    HDR3ExposureTypeInfo*   pHDR3ExposureType
    ) const
{
    CAMX_ASSERT(NULL != pIsSupported);
    CAMX_ASSERT(NULL != pHDR3ExposureType);

    HDR3ExposureTypeInfo HDR3ExposureType = m_pSensorData->GetHDR3ExposureType(modeIndex);

    *pIsSupported = FALSE;

    if (modeIndex < m_pSensorData->GetResolutionInfo()->resolutionDataCount)
    {
        ResolutionData resolutionData = m_pSensorData->GetResolutionInfo()->resolutionData[modeIndex];

        for (UINT index = 0; index < resolutionData.streamInfo.streamConfigurationCount; index++)
        {
            if (StreamType::HDR == resolutionData.streamInfo.streamConfiguration[index].type)
            {
                if (HDR3ExposureTypeInfo::HDR3ExposureType1 == HDR3ExposureType ||
                    HDR3ExposureTypeInfo::HDR3ExposureType2 == HDR3ExposureType ||
                    HDR3ExposureTypeInfo::HDR3ExposureType3 == HDR3ExposureType)
                {
                    *pIsSupported = TRUE;
                    switch(HDR3ExposureType)
                    {
                        case HDR3ExposureTypeInfo::HDR3ExposureType1:
                            *pHDR3ExposureType = HDR3ExposureTypeInfo::HDR3ExposureType1;
                            break;
                        case HDR3ExposureTypeInfo::HDR3ExposureType2:
                            *pHDR3ExposureType = HDR3ExposureTypeInfo::HDR3ExposureType2;
                            break;
                        case HDR3ExposureTypeInfo::HDR3ExposureType3:
                            *pHDR3ExposureType = HDR3ExposureTypeInfo::HDR3ExposureType3;
                            break;
                        default:
                            *pHDR3ExposureType = HDR3ExposureTypeInfo::HDR3ExposureTypeUnknown;
                            break;
                    }
                    break;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageSensorModuleData::GetStatsParseFuncPtr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ImageSensorModuleData::GetStatsParseFuncPtr() const
{
    return m_pSensorData->GetStatsParseFuncPtr();
}

CAMX_NAMESPACE_END
