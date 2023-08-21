////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxeebindata.cpp
/// @brief Implements EEBinData methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxdebug.h"
#include "camxeebindata.h"
#include "camxhwdefs.h"
#include "camxhwenvironment.h"
#include "camximagebuffer.h"
#include "camximagesensorutils.h"
#include "camxmem.h"
#include "camxosutils.h"
#include "camxpacket.h"
#include "camxpacketdefs.h"
#include "camxstaticcaps.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::EEbinData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEbinData::EEbinData(
    INT32       deviceIndex,
    CSLHandle   hCSL)
{
    CamxResult result           = CamxResultSuccess;
    m_pEEbinDataReadPacket      = NULL;
    m_pEEbinVersionReadPacket   = NULL;
    m_isEEBinDataReadRequired   = FALSE;
    m_eebinDeviceIndex          = deviceIndex;
    m_hEEbinSessionHandle       = hCSL;
    m_isCSLOpenByEEbin          = FALSE;
    m_hEEbinDevice              = 0;
    m_pDataImage                = NULL;
    m_pVersionImage             = NULL;
    m_pVersionReadData          = NULL;
    m_pEEbinData                = NULL;
    m_pEEBinDataSize            = 0;
    m_pVersionSize              = 0;
    m_pEEbinmoduleSetManager    = NULL;

    CAMX_LOG_INFO(CamxLogGroupSensor, "eebin data deviceIndex: %d", deviceIndex);

    // Load memory map info from bin file generated using xml settings
    result = LoadMemoryMapData();
    if (CamxResultSuccess == result)
    {
        result = InitializeCSL(m_eebinDeviceIndex);
        if (CamxResultSuccess == result)
        {
            result = ConfigurePowerSettings();
            if (CamxResultSuccess == result)
            {
                // Write to EEBin data before reading. To write, eebin.bin should be available in /data/misc/camera
                // This property should be set only to Write and remains disable during normal usage
                if (TRUE == OsUtils::GetPropertyBool("persist.camera.eebin.write", FALSE))
                {
                    result = PrepareAndWriteEEBin();
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "Write to EEbin is not enabled!!!");
                }

                if (CamxResultSuccess == result)
                {
                    result = ReadEEBinVersion();
                    if (CamxResultSuccess == result)
                    {
                        result = FormatEEBinVersionData();
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::ReadEEBin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::ReadEEBin()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == m_isEEBinDataReadRequired)
    {
        result = ReadEEBinData();
        if ((CamxResultSuccess == result) && (NULL != m_pEEbinData))
        {
            result = FormatEEBinData();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "EEBin data header format failed!!!");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to read EEBin data, m_pEEbinData: %p", m_pEEbinData);
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "EEBin data read not required: EEBin has lower version or insufficient data to read");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::~EEbinData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEbinData::~EEbinData()
{
    CamxResult result = CamxResultSuccess;

    if (0 != m_hEEbinSessionHandle)
    {
        result = UnInitialize();
    }

    if (NULL != m_pDataPacketManger)
    {
        if (NULL != m_pEEbinDataReadPacket)
        {
            m_pDataPacketManger->Recycle(m_pEEbinDataReadPacket);
        }

        CAMX_DELETE m_pDataPacketManger;
        m_pDataPacketManger = NULL;
    }

    if (NULL != m_pVersionReadPacketManger)
    {
        if (NULL != m_pEEbinVersionReadPacket)
        {
            m_pVersionReadPacketManger->Recycle(m_pEEbinVersionReadPacket);
        }

        CAMX_DELETE m_pVersionReadPacketManger;
        m_pVersionReadPacketManger = NULL;
    }

    if ((NULL != m_pVersionImageBufferManager) && (NULL != m_pVersionImage))
    {
        m_pVersionImageBufferManager->ReleaseReference(m_pVersionImage);
        m_pVersionImage = NULL;

        m_pVersionImageBufferManager->Destroy();
        m_pVersionImageBufferManager = NULL;
    }

    if ((NULL != m_pDataImageBufferManager) && (NULL != m_pDataImage))
    {
        m_pDataImageBufferManager->ReleaseReference(m_pDataImage);
        m_pDataImage = NULL;

        m_pDataImageBufferManager->Destroy();
        m_pDataImageBufferManager = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::LoadMemoryMapData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::LoadMemoryMapData()
{
    CamxResult  result       = CamxResultSuccess;
    FILE*       phFileHandle = NULL;
    CHAR        fullName[FILENAME_MAX];

    OsUtils::GetBinaryFileName(fullName, NULL, FILENAME_MAX, XMLBinaryFileName);
    phFileHandle = OsUtils::FOpen(fullName, "rb");

    if (NULL != phFileHandle)
    {
        UINT64     fileSizeBytes     = OsUtils::GetFileSize(fullName);
        UCHAR*     pBuffer           = static_cast<BYTE*>(CAMX_CALLOC(static_cast<SIZE_T>(fileSizeBytes)));
        TuningMode pSelectors[1]     = { { ModeType::Default, { 0 } } };
        UINT32     modeCount         = 1;

        if ((NULL != pBuffer) && (0 != fileSizeBytes))
        {
            UINT64 sizeRead = OsUtils::FRead(pBuffer,
                                             static_cast<SIZE_T>(fileSizeBytes),
                                             1,
                                             static_cast<SIZE_T>(fileSizeBytes),
                                             phFileHandle);

            CAMX_ASSERT(fileSizeBytes == sizeRead);

            m_pEEbinmoduleSetManager = CAMX_NEW ImageSensorModuleSetManager();

            if (NULL != m_pEEbinmoduleSetManager)
            {
                if (TRUE == m_pEEbinmoduleSetManager->LoadBinaryParameters(pBuffer, fileSizeBytes))
                {
                    m_pEEbinDriver = m_pEEbinmoduleSetManager->GetModule_EEbinDriver(&pSelectors[0], modeCount);
                    if (NULL == m_pEEbinDriver)
                    {
                        result = CamxResultEFailed;
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Could not get EEBin driver");
                    }
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Could not LoadBinaryParameters");
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Could not get eebinmoduleSetManager");
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor,
                           "read/memory alloc failed for %s, fileSizeBytes:%d, pBuffer:%p",
                           fullName,
                           fileSizeBytes,
                           pBuffer);
        }

        if (NULL != pBuffer)
        {
            // Delete the buffer
            CAMX_FREE(pBuffer);
            pBuffer = NULL;
        }

        OsUtils::FClose(phFileHandle);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "open of %s filed", fullName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::InitializeCSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::InitializeCSL(
    UINT32 eebinDeviceIndex)
{
    CamxResult result = CamxResultSuccess;

    if (0 == m_hEEbinSessionHandle)
    {
        /// Need to create a new CSL session as EEBin data can be read before the actual CSL session is created
        /// This session will be closed once the data is read based on m_isCSLOpenByEEbin variable.
        result = CSLOpen(&m_hEEbinSessionHandle);
        if (CamxResultSuccess == result)
        {
            m_isCSLOpenByEEbin = TRUE;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Opened CSL");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to open CSL session");
        }
    }

    if (CamxResultSuccess == result)
    {
        result = CSLAcquireDevice(m_hEEbinSessionHandle, &m_hEEbinDevice, eebinDeviceIndex, NULL, 0, NULL, 0, "EEBin");
        if ((CamxResultSuccess == result) && (0 != m_hEEbinDevice))
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "AcquireDevice on EEbin is success");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "AcquireDevice on EEbin device failed!");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::UnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::UnInitialize()
{
    CamxResult result = CamxResultSuccess;

    if (0 != m_hEEbinDevice)
    {
        result = CSLReleaseDevice(m_hEEbinSessionHandle, m_hEEbinDevice);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "ReleaseDevice on EEBin failed!");
        }
    }

    if (TRUE == m_isCSLOpenByEEbin)
    {
        result = CSLClose(m_hEEbinSessionHandle);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "CSL Close on EEBin failed, error: %d", result);
        }
    }

    if (NULL != m_pEEbinmoduleSetManager)
    {
        // Destroy the eebin module manager object
        CAMX_DELETE m_pEEbinmoduleSetManager;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::GetEEBinModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 EEbinData::GetEEBinModules(
    CHAR*  pFileNames,
    SIZE_T maxFileNameLength)
{
    UINT16 fileCountMM            = 0;
    UINT16 fileCount              = 0;
    CHAR   fullName[FILENAME_MAX] = { 0 };

    fileCountMM = OsUtils::GetFilesFromPath(MmSensorModulesPath,
                                            FILENAME_MAX,
                                            pFileNames,
                                            "*",
                                            "sensormodule",
                                            "*",
                                            "*",
                                            "bin");

    if (NULL != m_pEEbinDriver)
    {
        for (UINT32 index = 0; index < m_pEEbinDriver->moduleInfoCount; index++)
        {
            /// MmSensorModulesPath takes higher priority than SensorModulesPath
            OsUtils::GetBinaryFileName(fullName, SensorModulesPath, FILENAME_MAX, m_pEEbinDriver->moduleInfo[index].binaryName);
            if (0 == OsUtils::GetFileSize(fullName))
            {
                Utils::Memset(fullName, 0 , FILENAME_MAX);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "found binary: %s", m_pEEbinDriver->moduleInfo[index].binaryName);
                OsUtils::SNPrintF(pFileNames + ((fileCountMM + fileCount) * maxFileNameLength),
                                  maxFileNameLength,
                                  "%s%s%s",
                                  SensorModulesPath,
                                  PathSeparator,
                                  m_pEEbinDriver->moduleInfo[index].binaryName);

                Utils::Memset(fullName, 0 , FILENAME_MAX);
                fileCount++;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "EEBin driver data is NULL");
    }

    CAMX_LOG_INFO(CamxLogGroupSensor, "fileCount: %d, fileCountMM: %d, Total fileCount: %d", (fileCount + fileCountMM));
    return (fileCount + fileCountMM);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::ReadWord
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 EEbinData::ReadWord(
    UINT8*     pData,
    UINT16     numberOfBytes,
    EndianType endian)
{
    UINT32 returnValue = 0;

    switch (endian)
    {
        case EndianType::BIG:
            // Read MSB which is at low address in the buffer and left shift by 8 and so on until all the number of bytes
            // specifiied by mask are read to store the Big endian data from buff to integer.
            for (UINT16 index = 0; index < 4; index++)
            {
                returnValue = (returnValue << 8) | (pData[index]);
            }
            break;
        case EndianType::LITTLE:
            // Read MSB which is at high address in the buffer and left shift by 8 and so on until all the number
            // of bytes specifiied by mask are read to store the Little endian data from buff to integer.
            for (; numberOfBytes > 0; numberOfBytes--)
            {
                returnValue = (returnValue << 8) | (pData[(numberOfBytes - 1)]);
            }
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid format\n");
            break;
    }
    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::WriteToFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::WriteToFile(
    UCHAR*      pData,
    UINT32      dataSize,
    CHAR*       pFileName)
{
    CamxResult result                     = CamxResultSuccess;
    CHAR       dumpFilename[FILENAME_MAX] = { 0 };

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "out file name %s, size: %d", pFileName, dataSize);

    if (NULL != pData)
    {
        OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s%s%s", MmSensorModulesPath, "/", pFileName);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "dump file path %s", dumpFilename);

        // Delete the file if its already exists
        OsUtils::FDelete(dumpFilename);

        FILE* pDestFile = CamX::OsUtils::FOpen(dumpFilename, "wb");

        if (NULL != pDestFile)
        {
            SIZE_T writtenSize = OsUtils::FWrite(pData, 1, dataSize, pDestFile);
            if (writtenSize != dataSize)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "dataSize: %d, writtenSize:%d", dataSize, writtenSize);
                result = CamxResultEFailed;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "data copied to %s", dumpFilename);
            }
            OsUtils::FClose(pDestFile);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "File open failed");
            result = CamxResultEInvalidPointer;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "invalid file data");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::FormatEEBinData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::FormatEEBinData()
{
    CamxResult      result          = CamxResultSuccess;
    UINT8*          pRawData        = NULL;
    // NOWHINE GR017: The native API uses unsigned long
    unsigned long   calculatedCRC   = 0;
    // NOWHINE GR017: The native API uses unsigned long
    unsigned long   readCRC         = 0;
    BYTE*           pOutputBuffer   = NULL;
    UINT            outputLength    = 0;
    EEbinModuleData parsedData;

    result = OsUtils::FindEstimatedSizeOfUnCompressed(m_pEEbinData, m_pEEBinDataSize, &outputLength);
    if (CamxResultSuccess == result)
    {
        pOutputBuffer = reinterpret_cast<BYTE *>(CAMX_CALLOC(outputLength));
    }

    if (NULL != pOutputBuffer)
    {
        result = OsUtils::UnCompress(m_pEEbinData, m_pEEBinDataSize, &pOutputBuffer, &outputLength);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory to un compress data");
    }

    if (CamxResultSuccess == result)
    {
        // CRC needs to be calculated on data part excluding the CRC field
        calculatedCRC  = OsUtils::ComputeCRC32(calculatedCRC, (pOutputBuffer + 4), (outputLength -4));
        // NOWHINE GR017: The native API uses unsigned long
        readCRC        = static_cast<unsigned long>(ReadWord(pOutputBuffer, sizeof(UINT32), EndianType::LITTLE));
        if (readCRC == calculatedCRC)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "CRC matched");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor,
                           "CRC match failed. calculatedCRC: %u, readCRC:%u",
                           calculatedCRC, readCRC);
            CAMX_FREE(pOutputBuffer);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        pRawData = pOutputBuffer;

        Utils::Memset(&parsedData , 0, sizeof(EEbinModuleData));
        Utils::Memcpy(&(parsedData.masterHeaderData), pRawData, sizeof(EEbinHeaderInfo));
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Number of modules: %d", parsedData.masterHeaderData.numberOfModules);

        pRawData = pRawData + sizeof(EEbinHeaderInfo);
        for (UINT index = 0; index < parsedData.masterHeaderData.numberOfModules; index++)
        {
            Utils::Memcpy(&(parsedData.moduleHeaderInfo[index]), pRawData, sizeof(EEbinModuleHeaderInfo));
            pRawData = pRawData + sizeof(EEbinModuleHeaderInfo);

            result = WriteToFile((pOutputBuffer + parsedData.moduleHeaderInfo[index].offset),
                                 parsedData.moduleHeaderInfo[index].size,
                                 parsedData.moduleHeaderInfo[index].name);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to save binary files");
                break;
            }
        }

        CAMX_FREE(pOutputBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::GetCommandBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::GetCommandBuffer(
    CmdBufferManager*   pCmdBufferManager,
    PacketResource**    ppPacketResource)
{
    CamxResult result = CamxResultSuccess;

    result = pCmdBufferManager->GetBuffer(ppPacketResource);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get buffer: %d", result)
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT EEbinData::GetPowerSequenceCmdSize(
    BOOL isPowerUp)
{
    UINT            powerSequenceSize = 0;
    PowerSetting*   pPowerSettings    = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pEEbinDriver->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pEEbinDriver->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pEEbinDriver->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pEEbinDriver->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::GetPowerSequenceCmdSize(powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEbinData::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::CreatePowerSequenceCmd(
    BOOL    isPowerUp,
    VOID*   pCmdBuffer)

{
    UINT            powerSequenceSize   = 0;
    PowerSetting*   pPowerSettings      = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pEEbinDriver->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pEEbinDriver->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pEEbinDriver->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pEEbinDriver->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::CreatePowerSequenceCmd(pCmdBuffer, isPowerUp, powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::PrepareAndWriteEEBin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::PrepareAndWriteEEBin()
{
    CamxResult result              = CamxResultSuccess;
    FILE*       phFileHandle       = NULL;
    BYTE*       pBuffer            = NULL;
    BYTE*       pCompressedBuffer  = NULL;
    UINT        compressedLength   = 0;
    UINT32      fileSizeBytes      = 0;
    CHAR        fullName[FILENAME_MAX];
    CHAR        fwVersion[VersionSize];

    /// Step 1: Read the EEBin data to write
    OsUtils::GetBinaryFileName(fullName, MmSensorModulesPath, FILENAME_MAX, EEBinDataFileName);
    phFileHandle    = OsUtils::FOpen(fullName, "rb");
    fileSizeBytes   = static_cast<UINT32>(OsUtils::GetFileSize(fullName));

    if ((NULL != phFileHandle) && (0 != fileSizeBytes))
    {
        pBuffer = static_cast<BYTE*>(CAMX_CALLOC(fileSizeBytes));

        if (NULL != pBuffer)
        {
            UINT64 sizeRead = OsUtils::FRead(pBuffer,
                                             static_cast<SIZE_T>(fileSizeBytes),
                                             1,
                                             static_cast<SIZE_T>(fileSizeBytes),
                                             phFileHandle);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "fileSizeBytes: %d, sizeRead: %d",
                             fileSizeBytes,
                             static_cast<UINT32>(sizeRead));

            CAMX_ASSERT(fileSizeBytes == sizeRead);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate write buffer to write data");
            result = CamxResultENoMemory;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to open %s, phFileHandle: %p, fileSizeBytes: %d",
            fullName, phFileHandle, fileSizeBytes);
        result = CamxResultEInvalidPointer;
    }

    /// Step2: Calculate the CRC and Compress the data and prepare the compressed to write
    if (CamxResultSuccess == result)
    {
        // NOWHINE GR017: The native API uses unsigned long
        unsigned long calculatedCRC = 0;

        /// Calculate the CRC for the data to be written except for the CRC data which is first 4 bytes
        calculatedCRC = OsUtils::ComputeCRC32(calculatedCRC, (pBuffer + 4), (fileSizeBytes - 4));

        /// Copy the CRC to buffer. CRC is of length 4 bytes from begining
        Utils::Memcpy(pBuffer, &calculatedCRC, sizeof(UINT32));

        /// Obtain fwVersion from the eebin.bin file (first 4 bytes are CRC)
        Utils::Memcpy(fwVersion, (pBuffer + 4), VersionSize);
        pCompressedBuffer = reinterpret_cast<BYTE *>(CAMX_CALLOC(fileSizeBytes));
        if (NULL != pCompressedBuffer)
        {
            result = OsUtils::Compress(pBuffer, fileSizeBytes, &pCompressedBuffer, &compressedLength);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "fileSizeBytes: %d, compressedSize: %d, fwVersion:%s, calculatedCRC: %ul",
                             fileSizeBytes,
                             compressedLength,
                             fwVersion,
                             calculatedCRC);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory for data compression");
        }

        if (CamxResultSuccess == result)
        {
            m_pEEbinDriver->dataWriteInfo.regSettingCount                 = 1;
            m_pEEbinDriver->dataWriteInfo.regSetting[0].registerDataCount = compressedLength;
            m_pEEbinDriver->dataWriteInfo.regSetting[0].registerData      =
                static_cast<RegisterData*>(CAMX_CALLOC(static_cast<SIZE_T>(compressedLength) * sizeof(RegisterData)));

            if (NULL != m_pEEbinDriver->dataWriteInfo.regSetting[0].registerData)
            {
                for (UINT i=0; i < compressedLength; i++)
                {
                    m_pEEbinDriver->dataWriteInfo.regSetting[0].registerData[i] =
                        static_cast<RegisterData>(pCompressedBuffer[i]);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory");
                result = CamxResultENoMemory;
            }
        }
    }

    /// Step3: Read whole of version/header related data and update Read start, end addresses
    if (CamxResultSuccess == result)
    {
        SettingsInfo headerWriteInfo;

        Utils::Memset(&headerWriteInfo, 0, sizeof(SettingsInfo));

        result = ReadEEBinVersion();
        if (CamxResultSuccess == result)
        {
            /// Update the header data with Read start address, End address and FW version
            UINT32 startAddress = m_pEEbinDriver->dataWriteInfo.regSetting[0].registerAddr;
            UINT32 endAddress   = m_pEEbinDriver->dataWriteInfo.regSetting[0].registerAddr + compressedLength - 1;
            Utils ::Memcpy(m_pVersionReadData + m_pEEbinDriver->versionFormatInfo.readStartAddress.offset,
                           &startAddress,
                           sizeof(UINT32));
            Utils ::Memcpy(m_pVersionReadData + m_pEEbinDriver->versionFormatInfo.readEndAddress.offset,
                           &endAddress,
                           sizeof(UINT32));
            Utils ::Memcpy(m_pVersionReadData + m_pEEbinDriver->versionFormatInfo.version.offset,
                           &fwVersion,
                           VersionSize);

            headerWriteInfo.regSettingCount                 = 1;
            headerWriteInfo.regSetting                      =
                reinterpret_cast<RegisterSetting*>(CAMX_CALLOC(sizeof(RegisterSetting)));
            if (NULL != headerWriteInfo.regSetting)
            {
                headerWriteInfo.regSetting[0].registerData = reinterpret_cast<RegisterData*>(CAMX_CALLOC(m_pVersionSize));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "headerWriteInfo.regSetting is NULL");
                result = CamxResultEFailed;
            }

            if (CDKResultSuccess == result)
            {
                if (NULL != headerWriteInfo.regSetting[0].registerData)
                {
                    for (UINT i=0; i < m_pVersionSize; i++)
                    {
                        headerWriteInfo.regSetting[0].registerData[i] =
                            static_cast<RegisterData>(m_pVersionReadData[i]);
                    }

                    headerWriteInfo.regSetting[0].registerAddr      =
                        m_pEEbinDriver->versionReadInfo.regSetting[0].registerAddr;
                    headerWriteInfo.regSetting[0].registerDataCount = m_pVersionSize;
                    headerWriteInfo.regSetting[0].regAddrType       = m_pEEbinDriver->versionReadInfo.regSetting[0].regAddrType;
                    headerWriteInfo.regSetting[0].regDataType       = m_pEEbinDriver->versionReadInfo.regSetting[0].regDataType;
                    headerWriteInfo.regSetting[0].operation         = OperationType::WRITE_SEQUENTIAL;

                    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                     "Writing: ReadStartAddress: 0x%X, ReadEndAddress: 0x%X",
                                     startAddress,
                                     endAddress);

                    result = WriteEEBinData(&headerWriteInfo);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to write EEBin");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory");
                    result = CamxResultENoMemory;
                }
            }

            /// Release the data allocated for write data
            CAMX_FREE(headerWriteInfo.regSetting[0].registerData);
            CAMX_FREE(headerWriteInfo.regSetting);

            /// Release the version read related data
            if (NULL != m_pVersionReadPacketManger)
            {
                if (NULL != m_pEEbinVersionReadPacket)
                {
                    m_pVersionReadPacketManger->Recycle(m_pEEbinVersionReadPacket);
                }

                CAMX_DELETE m_pVersionReadPacketManger;
                m_pVersionReadPacketManger = NULL;
            }

            if ((NULL != m_pVersionImageBufferManager) && (NULL != m_pVersionImage))
            {
                m_pVersionImageBufferManager->ReleaseReference(m_pVersionImage);
                m_pVersionImage = NULL;
                m_pVersionImageBufferManager->Destroy();
                m_pVersionImageBufferManager = NULL;
                m_pVersionReadData           = NULL;
                m_pVersionSize               = 0;
            }
        }
    }

    /// Step 4: Write the eebin data
    if (CamxResultSuccess == result)
    {
        /// update the size to be written each time to a fixed value due to limitaion in KMD kzalloc
        UINT size      = 12288;
        UINT totalSize = 0;

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Size of each write operation:%d, total: %d", size, compressedLength);

        for (totalSize = 0; totalSize < compressedLength; )
        {
            if ((totalSize + size) > compressedLength)
            {
                size = compressedLength - totalSize;
            }
            totalSize += size;
            m_pEEbinDriver->dataWriteInfo.regSetting[0].registerDataCount = size;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                            "size:%d reg address:%p, totalsize:%d, compressedLength:%d",
                            size,
                            &(m_pEEbinDriver->dataWriteInfo.regSetting[0].registerData[0]),
                            totalSize,
                            compressedLength);
            result = WriteEEBinData(&(m_pEEbinDriver->dataWriteInfo));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to write EEBin");
                break;
            }
            m_pEEbinDriver->dataWriteInfo.regSetting[0].registerAddr += size;
            m_pEEbinDriver->dataWriteInfo.regSetting[0].registerData += size;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Total size written:%d", totalSize);
    }

    CAMX_FREE(pBuffer);
    CAMX_FREE(pCompressedBuffer);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::ConfigurePowerSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::ConfigurePowerSettings()
{
    CamxResult           result                     = CamxResultSuccess;
    CmdBufferManager*    pCmdManagerPowerSequence   = NULL;
    CmdBufferManager     cmdManagerMemoryMapInfo;
    CmdBufferManager*    pPowerPacketManger         = NULL;
    Packet*              pPowerPacket               = NULL;
    PacketResource*      pPacketResource            = NULL;
    CmdBuffer*           pPowerSequenceCmd          = NULL;
    ResourceParams       packetResourceParams       = {0};
    ResourceParams       cmdResourceParams          = {0};
    UINT                 powerUpCmdSize             = GetPowerSequenceCmdSize(TRUE);
    UINT                 powerDownCmdSize           = GetPowerSequenceCmdSize(FALSE);
    UINT                 powerSequenceSize          = (powerUpCmdSize + powerDownCmdSize);
    UINT32               cmdBufferIndex             = 0;

    // Step1: Initialize the EEPROM init read packet
    packetResourceParams.usageFlags.packet                = 1;
    // one for power sequence
    packetResourceParams.packetParams.maxNumCmdBuffers    = 1;
    packetResourceParams.packetParams.maxNumIOConfigs     = 0;
    packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
    packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                   = NULL;
    packetResourceParams.numDevices                       = 0;
    packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CmdBufferManager::Create("EEBinPowerPacketManager", &packetResourceParams, &pPowerPacketManger);
    if (CamxResultSuccess == result)
    {
        result = GetCommandBuffer(pPowerPacketManger, &pPacketResource);
        if ((CamxResultSuccess == result) && (NULL != pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
            // pResource points to a Packet so we may static_cast
            pPowerPacket = static_cast<Packet*>(pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get command buffer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create power packet manager result: %d", result);
    }

    // Step2: Initialize and add power sequence commnds to the packet
    if (CamxResultSuccess == result)
    {
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEBinCmdManagerPowerSequence", &cmdResourceParams, &pCmdManagerPowerSequence);
        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerPowerSequence, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Power Sequence cmdbuffermanager, result: %d", result);
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pPowerSequenceCmd = static_cast<CmdBuffer*>(pPacketResource);

            // step2a: Commit power up command
            VOID* pCmdPowerUp = pPowerSequenceCmd->BeginCommands(powerUpCmdSize / sizeof(UINT32));
            if (NULL != pCmdPowerUp)
            {
                if (CamxResultSuccess == CreatePowerSequenceCmd(TRUE, pCmdPowerUp))
                {
                    result = pPowerSequenceCmd->CommitCommands();
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create power up command");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve powerup command area in command buffer");
                result = CamxResultEFailed;
            }

            // step2b: Commit power down command
            if (CamxResultSuccess == result)
            {
                VOID* pCmdPowerDown = pPowerSequenceCmd->BeginCommands(powerDownCmdSize / sizeof(UINT32));
                if (NULL != pCmdPowerDown)
                {
                    if (CamxResultSuccess == CreatePowerSequenceCmd(FALSE, pCmdPowerDown))
                    {
                        result = pPowerSequenceCmd->CommitCommands();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create power down command");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve powerdown command area in command buffer");
                    result = CamxResultEFailed;
                }
            }

            // step2c: Add power sequence (power up + power down) command buffer to packet
            if (CamxResultSuccess == result)
            {
                result = pPowerPacket->AddCmdBufferReference(pPowerSequenceCmd, &cmdBufferIndex);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain power command buffer");
        }
    }

    // Step3: Add opcode and commit the packet
    if (CamxResultSuccess == result)
    {
        pPowerPacket->SetOpcode(CSLDeviceTypeEEPROM, CSLPacketOpcodesEEPROMInitialConfig);
        result = pPowerPacket->CommitPacket();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to commit packet =%d", result);
        }
    }

    // Step4: Submit the prepared packet to CSL/KMD
    if (CamxResultSuccess == result)
    {
        result = CSLSubmit(m_hEEbinSessionHandle,
                           m_hEEbinDevice,
                           pPowerPacket->GetMemHandle(),
                           pPowerPacket->GetOffset());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit packet =%d", result);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Power packet submitted result: %d", result);
        }
    }

    // Step5: Clean up the resources
    // These resources are managed manually, hence recycled manually.
    if (NULL != pCmdManagerPowerSequence)
    {
        pCmdManagerPowerSequence->Recycle(pPowerSequenceCmd);
        CAMX_DELETE pCmdManagerPowerSequence;
    }

    if (NULL != pPowerPacketManger)
    {
        if (NULL != pPowerPacket)
        {
            pPowerPacketManger->Recycle(pPowerPacket);
        }

        CAMX_DELETE pPowerPacketManger;
        pPowerPacketManger = NULL;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::WriteEEBinData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::WriteEEBinData(
    SettingsInfo* pDataWriteInfo)
{
    CamxResult          result                   = CamxResultSuccess;
    UINT                I2CInfoCmdSize           = sizeof(CSLSensorI2CInfo);
    UINT                regSettingIndex          = 0;
    UINT                writeDataSize            = ImageSensorUtils::GetRegisterSettingsCmdSize(pDataWriteInfo->regSettingCount,
                                                                                                pDataWriteInfo->regSetting,
                                                                                                &regSettingIndex);
    PacketResource*     pPacketResource          = NULL;
    ResourceParams      packetResourceParams     = {0};
    ResourceParams      cmdResourceParams        = {0};
    CmdBuffer*          pI2CInfoCmd              = NULL;
    CmdBuffer*          pWriteDataCmd            = NULL;
    Packet*             pWritePacket             = NULL;
    UINT32              cmdBufferIndex           = 0;
    CmdBufferManager*   pWritePacketManager      = NULL;
    CmdBufferManager*   pCmdManagerI2CInfo       = NULL;
    CmdBufferManager*   pCmdManagerWriteDataInfo = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "I2CInfoCmdSize: %d, writeDataSize: %d", I2CInfoCmdSize, writeDataSize);

    // Step1: Initialize the FROM Write packet
    packetResourceParams.usageFlags.packet                = 1;
    // one for slave info and one for write data
    packetResourceParams.packetParams.maxNumCmdBuffers    = 2;
    packetResourceParams.packetParams.maxNumIOConfigs     = 0;
    packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
    packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                   = NULL;
    packetResourceParams.numDevices                       = 0;
    packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CmdBufferManager::Create("EEBinWritePacketManager", &packetResourceParams, &pWritePacketManager);
    if (CamxResultSuccess == result)
    {
        result = GetCommandBuffer(pWritePacketManager, &pPacketResource);
        if ((CamxResultSuccess == result) && (NULL != pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
            // pResource points to a Packet so we may static_cast
            pWritePacket = static_cast<Packet*>(pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get command buffer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create write packet manager result: %d", result);
    }

    // Step2: Initialize and add i2c info to the packet
    if (CamxResultSuccess == result)
    {
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = I2CInfoCmdSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEBinCmdManagerI2CInfo", &packetResourceParams, &pCmdManagerI2CInfo);
        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerI2CInfo, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create I2C Info cmdbuffermanager");
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pI2CInfoCmd = static_cast<CmdBuffer*>(pPacketResource);

            // Step2a: Commit the I2cInfo command
            CSLSensorI2CInfo* pCmdI2CInfo =
                reinterpret_cast<CSLSensorI2CInfo*>(pI2CInfoCmd->BeginCommands(I2CInfoCmdSize / sizeof(UINT32)));
            if (NULL != pCmdI2CInfo)
            {
                result = ImageSensorUtils::CreateI2CInfoCmd(pCmdI2CInfo,
                                                            m_pEEbinDriver->slaveInfo.slaveAddress,
                                                            m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                if (CamxResultSuccess == result)
                {
                    result = pI2CInfoCmd->CommitCommands();
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create i2c command");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve i2c info command area in command buffer");
                result = CamxResultEFailed;
            }
        }

        // step2b: Add I2C info command buffer to packet
        if (CamxResultSuccess == result)
        {
            // Not associated with any request. Won't be recycled.
            pWritePacket->SetRequestId(CamxInvalidRequestId);
            result = pWritePacket->AddCmdBufferReference(pI2CInfoCmd, &cmdBufferIndex);
        }
    }

    // Step3: Initialize and add Write info to the packet
    if (CamxResultSuccess == result)
    {
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = writeDataSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEBinCmdManagerWriteData", &packetResourceParams, &pCmdManagerWriteDataInfo);
        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerWriteDataInfo, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create write data Info cmdbuffermanager");
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pWriteDataCmd = static_cast<CmdBuffer*>(pPacketResource);

            // Step3a: Commit the write data command
            VOID* pCmdBuffer = pWriteDataCmd->BeginCommands(writeDataSize / sizeof(UINT32));
            if (NULL != pCmdBuffer)
            {
                result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                                     pDataWriteInfo->regSettingCount,
                                                                     pDataWriteInfo->regSetting,
                                                                     regSettingIndex,
                                                                     m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                if (CamxResultSuccess == result)
                {
                    result = pWriteDataCmd->CommitCommands();
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create write data command");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve write data command area in command buffer");
                result = CamxResultEFailed;
            }
        }

        // step3b: Add write data command buffer to packet
        if (CamxResultSuccess == result)
        {
            // Not associated with any request. Won't be recycled.
            pWritePacket->SetRequestId(CamxInvalidRequestId);
            result = pWritePacket->AddCmdBufferReference(pWriteDataCmd, &cmdBufferIndex);
        }
    }

    // Step4: Add opcode and commit the packet
    if (CamxResultSuccess == result)
    {
        pWritePacket->SetOpcode(CSLDeviceTypeEEPROM, CSLPacketOpcodesEEPROMWriteData);
        result = pWritePacket->CommitPacket();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to commit packet =%d", result);
        }
    }

    // Step5: Submit the prepared packet to CSL/KMD
    if (CamxResultSuccess == result)
    {
        result = CSLSubmit(m_hEEbinSessionHandle,
                           m_hEEbinDevice,
                           pWritePacket->GetMemHandle(),
                           pWritePacket->GetOffset());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit packet =%d", result);
        }
    }

    // Step8: Clean up the resources
    // These resources are managed manually, hence recycled manually.
    if (NULL != pCmdManagerI2CInfo)
    {
        pCmdManagerI2CInfo->Recycle(pI2CInfoCmd);
        CAMX_DELETE pCmdManagerI2CInfo;
    }

    if (NULL != pCmdManagerWriteDataInfo)
    {
        pCmdManagerWriteDataInfo->Recycle(pWriteDataCmd);
        CAMX_DELETE pCmdManagerWriteDataInfo;
    }

    if (NULL != pWritePacketManager)
    {
        if (NULL != pWritePacket)
        {
            pWritePacketManager->Recycle(pWritePacket);
        }

        CAMX_DELETE pWritePacketManager;
        pWritePacketManager = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::FormatEEBinVersionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::FormatEEBinVersionData()
{
    CHAR*       pVersionRead                 = NULL;
    CamxResult  result                       = CamxResultSuccess;
    UINT32      readStartAddress             = 0;
    UINT32      lengthToRead                 = 0;
    UINT32      readEndAddress               = 0;
    CHAR        versionHW[HWVersionSize + 1] = {};
    CHAR        versionSW[SWVersionSize + 1] = {};

    CAMX_ASSERT(m_pEEbinDriver->versionFormatInfo.version.lengthInBytes > (HWVersionSize + SWVersionSize));

    pVersionRead = static_cast<CHAR*>(CAMX_CALLOC(m_pEEbinDriver->versionFormatInfo.version.lengthInBytes + 1));
    if (NULL != pVersionRead)
    {
        for (UINT i=0; i < m_pEEbinDriver->versionFormatInfo.version.lengthInBytes; i++)
        {
            pVersionRead[i] = m_pVersionReadData[m_pEEbinDriver->versionFormatInfo.version.offset + i];
        }
        OsUtils::StrLCpy(versionHW, pVersionRead, HWVersionSize + 1);
        OsUtils::StrLCpy(versionSW, (pVersionRead + HWVersionSize), SWVersionSize + 1);
        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                         "version read: %s, versionHW: %s, versionSW: %s, moduleInfoCount: %d",
                         pVersionRead,
                         versionHW,
                         versionSW,
                         m_pEEbinDriver->moduleInfoCount);

        for (UINT i=0; i< m_pEEbinDriver->moduleInfoCount; i++)
        {
            if (0 == OsUtils::StrNICmp(versionHW, m_pEEbinDriver->moduleInfo[i].HWVersion, HWVersionSize))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "HWInfo matched for module index: %d", i);
                if (0 < OsUtils::StrNICmp(versionSW, m_pEEbinDriver->moduleInfo[i].SWVersion, SWVersionSize))
                {
                    CAMX_LOG_INFO(CamxLogGroupSensor, "SWVersion in EEBin is latest, so need to read from EEBin");
                    m_isEEBinDataReadRequired = TRUE;
                    break;
                }
            }
        }
        CAMX_FREE(pVersionRead);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory for version");
        result = CamxResultENoMemory;
    }

    if ((CamxResultSuccess == result) && (TRUE == m_isEEBinDataReadRequired))
    {
        readStartAddress = ReadWord((m_pVersionReadData + m_pEEbinDriver->versionFormatInfo.readStartAddress.offset),
                                    m_pEEbinDriver->versionFormatInfo.readStartAddress.lengthInBytes,
                                    EndianType::LITTLE);
        readEndAddress   = ReadWord((m_pVersionReadData + m_pEEbinDriver->versionFormatInfo.readEndAddress.offset),
                                    m_pEEbinDriver->versionFormatInfo.readEndAddress.lengthInBytes,
                                    EndianType::LITTLE);
        lengthToRead     = readEndAddress - readStartAddress + 1;
        if (1 >= lengthToRead)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid size to read, m_pEEBinReadDataLength: %d", lengthToRead);
            result = CamxResultEInvalidArg;
        }
        else
        {
            /// Updating the reg settings to read based on the information read from EEBin header
            /// expecting remaining values to populate from EEBin drive rxml.
            m_pEEbinDriver->dataReadInfo.regSettingCount                 = 1;
            m_pEEbinDriver->dataReadInfo.regSetting[0].registerAddr      = readStartAddress;
            m_pEEbinDriver->dataReadInfo.regSetting[0].registerData[0]   = lengthToRead;
            m_pEEbinDriver->dataReadInfo.regSetting[0].registerDataCount = 1;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "readStartAddress:0X%x, readEndAddress:0X%x, lengthToRead:%d",
                             readStartAddress,
                             readEndAddress,
                             lengthToRead);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::ReadEEBinVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::ReadEEBinVersion()
{
    CamxResult           result                   = CamxResultSuccess;
    CmdBufferManager*    pCmdManagerMemoryMapInfo = NULL;
    PacketResource*      pPacketResource          = NULL;
    CmdBuffer*           pMemoryMapCmd            = NULL;
    ResourceParams       packetResourceParams     = {0};
    ResourceParams       cmdResourceParams        = {0};
    UINT                 I2CInfoCmdSize           = sizeof(CSLSensorI2CInfo);
    UINT                 regSettingIndex          = 0;
    UINT                 readSequenceSize         =
        ImageSensorUtils::GetRegisterSettingsCmdSize(m_pEEbinDriver->versionReadInfo.regSettingCount,
                                                     m_pEEbinDriver->versionReadInfo.regSetting,
                                                     &regSettingIndex);
    UINT                 memoryMapSize            = readSequenceSize + I2CInfoCmdSize;
    UINT32               cmdBufferIndex           = 0;

    CAMX_LOG_INFO(CamxLogGroupSensor, "I2CInfoCmdSize: %d, memoryMapSize:%d", I2CInfoCmdSize, memoryMapSize);

    // Step1: Initialize the EEPROM init read packet
    packetResourceParams.usageFlags.packet                = 1;
    // one for power sequence and one for memory map info
    packetResourceParams.packetParams.maxNumCmdBuffers    = 1;
    packetResourceParams.packetParams.maxNumIOConfigs     = 1;
    packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
    packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                   = NULL;
    packetResourceParams.numDevices                       = 0;
    packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CmdBufferManager::Create("EEBinVersionPacketManager", &packetResourceParams, &m_pVersionReadPacketManger);
    if (CamxResultSuccess == result)
    {
        result = GetCommandBuffer(m_pVersionReadPacketManger, &pPacketResource);
        if ((CamxResultSuccess == result) && (NULL != pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
            // pResource points to a Packet so we may static_cast
            m_pEEbinVersionReadPacket = static_cast<Packet*>(pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get command buffer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create version read packet manager result: %d", result);
    }

    // Step2: Initialize and add memory map commnds(i2c info and memory details) to the packet
    if (CamxResultSuccess == result)
    {
        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = memoryMapSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEBinMemoryMapCmdBuffManager", &packetResourceParams, &pCmdManagerMemoryMapInfo);
        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerMemoryMapInfo, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create memory map cmd buffer manager result: %d", result);
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pMemoryMapCmd = static_cast<CmdBuffer*>(pPacketResource);

            // Step2a: Commit the I2cInfo command specific to the memory block
            CSLSensorI2CInfo* pCmdI2CInfo =
                reinterpret_cast<CSLSensorI2CInfo*>(pMemoryMapCmd->BeginCommands(I2CInfoCmdSize / sizeof(UINT32)));
            if (NULL != pCmdI2CInfo)
            {
                result = ImageSensorUtils::CreateI2CInfoCmd(pCmdI2CInfo,
                                                            m_pEEbinDriver->versionReadInfo.regSetting[0].slaveAddr,
                                                            m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                if (CamxResultSuccess == result)
                {
                    result = pMemoryMapCmd->CommitCommands();
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create i2c command");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve i2c info command area in command buffer");
                result = CamxResultEFailed;
            }

            // Step2b: Commit the memorymap command specific to the memory block
            if (CamxResultSuccess == result)
            {
                VOID* pCmdMemoryMapInfo =
                    pMemoryMapCmd->BeginCommands(readSequenceSize / sizeof(UINT32));
                if (NULL != pCmdMemoryMapInfo)
                {
                    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdMemoryMapInfo,
                                                                         m_pEEbinDriver->versionReadInfo.regSettingCount,
                                                                         m_pEEbinDriver->versionReadInfo.regSetting,
                                                                         regSettingIndex,
                                                                         m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Addr: 0x%X: length:%d",
                        m_pEEbinDriver->versionReadInfo.regSetting[0].registerAddr,
                        m_pEEbinDriver->versionReadInfo.regSetting[0].registerData[0]);
                    if (CamxResultSuccess == result)
                    {
                        result = pMemoryMapCmd->CommitCommands();
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to commit command");
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create memory map command");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve memory map command area in command buffer");
                    result = CamxResultEFailed;
                }
            }

            // step2c: Add memory map (I2C info + memory info) command buffer to packet
            if (CamxResultSuccess == result)
            {
                // Not associated with any request. Won't be recycled.
                m_pEEbinVersionReadPacket->SetRequestId(CamxInvalidRequestId);
                result = m_pEEbinVersionReadPacket->AddCmdBufferReference(pMemoryMapCmd, &cmdBufferIndex);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain memory map command buffer");
        }
    }

    // Step3: Initialize and add read data commnds to the packet
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Create IOConfig buffer");
    if (CamxResultSuccess == result)
    {
        BufferManagerCreateData createData  = { };
        ImageFormat*            pFormat     = &createData.bufferProperties.imageFormat;
        UINT32                  IOConfigIndex;

        m_pVersionSize     = static_cast<UINT32>(m_pEEbinDriver->versionReadInfo.regSetting[0].registerData[0]);
        pFormat->width     = m_pVersionSize;
        pFormat->height    = 1;
        pFormat->format    = Format::Blob;
        pFormat->alignment = 8;

        createData.maxBufferCount               = 1;
        createData.immediateAllocBufferCount    = 1;
        createData.deviceCount                  = 1;
        createData.deviceIndices[0]             = m_eebinDeviceIndex;
        createData.bufferProperties.memFlags    = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
        createData.allocateBufferMemory         = TRUE;
        createData.numBatchedFrames             = 1;
        createData.bufferProperties.bufferHeap  = CSLBufferHeapIon;
        createData.bufferManagerType            = BufferManagerType::CamxBufferManager;

        result = ImageBufferManager::Create("EEBIN_VERSION", &createData, &m_pVersionImageBufferManager);

        if (CamxResultSuccess == result)
        {
            m_pVersionImage = m_pVersionImageBufferManager->GetImageBuffer();

            if (NULL == m_pVersionImage)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer");
                result = CamxResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer manager");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            result = m_pEEbinVersionReadPacket->AddIOConfig(m_pVersionImage,
                                                            0,
                                                            CSLIODirection::CSLIODirectionOutput,
                                                            NULL,
                                                            0,
                                                            &IOConfigIndex,
                                                            NULL,
                                                            0);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to add IO config =%d", result);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate image buffer");
        }
    }

    // Step4: Add opcode and commit the packet
    if (CamxResultSuccess == result)
    {
        m_pEEbinVersionReadPacket->SetOpcode(CSLDeviceTypeEEPROM, CSLPacketOpcodesEEPROMInitialConfig);
        result = m_pEEbinVersionReadPacket->CommitPacket();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to commit packet =%d", result);
        }
    }

    // Step5: Submit the prepared packet to CSL/KMD
    if (CamxResultSuccess == result)
    {
        result = CSLSubmit(m_hEEbinSessionHandle,
                           m_hEEbinDevice,
                           m_pEEbinVersionReadPacket->GetMemHandle(),
                           m_pEEbinVersionReadPacket->GetOffset());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit packet =%d", result);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Read version packet submitted result: %d", result);
        }
    }

    // Step6: get the buffer to which OTP data copied from CSL
    // Note: This logic works fine as the specifc IOCTL is synchronous but will use the fence logic to comply with design.
    // @todo (CAMX-1996) - Use fence to read the buffer in order to comply with the design.
    if (CamxResultSuccess == result)
    {
        m_pVersionReadData = m_pVersionImage->GetPlaneVirtualAddr(0, 0);
        if (NULL == m_pVersionReadData)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain eebin version memory buffer, result =%d", result);
        }
        else
        {
            for (UINT i=0; i < m_pVersionSize ; i++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "m_pVersionReadData[%d] =%d", i, m_pVersionReadData[i]);
            }
        }
    }

    // Step7: Clean up the resources
    // These resources are managed manually, hence recycled manually.
    if (NULL != pCmdManagerMemoryMapInfo)
    {
        pCmdManagerMemoryMapInfo->Recycle(pMemoryMapCmd);
        CAMX_DELETE pCmdManagerMemoryMapInfo;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEbinData::ReadEEBinData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEbinData::ReadEEBinData()
{
    CamxResult          result                   = CamxResultSuccess;
    CmdBufferManager*   pCmdManagerMemoryMapInfo = NULL;
    PacketResource*     pPacketResource          = NULL;
    CmdBuffer*          pMemoryMapCmd            = NULL;
    ResourceParams      packetResourceParams     = {0};
    ResourceParams      cmdResourceParams        = {0};
    UINT                I2CInfoCmdSize           = sizeof(CSLSensorI2CInfo);
    UINT                readSequenceSize         = 0;
    UINT                memoryMapSize            = 0;
    UINT32              cmdBufferIndex           = 0;
    UINT                regSettingIndex          = 0;

    m_pEEBinDataSize = static_cast<UINT32>(m_pEEbinDriver->dataReadInfo.regSetting[0].registerData[0]);
    if (0 != m_pEEBinDataSize)
    {
        readSequenceSize = ImageSensorUtils::GetRegisterSettingsCmdSize(m_pEEbinDriver->dataReadInfo.regSettingCount,
                                                                    m_pEEbinDriver->dataReadInfo.regSetting,
                                                                    &regSettingIndex);
        memoryMapSize    = readSequenceSize + I2CInfoCmdSize;

        // Step1: Initialize the EEBin init read packet
        packetResourceParams.usageFlags.packet                = 1;
        // one for memory map info
        packetResourceParams.packetParams.maxNumCmdBuffers    = 1;
        packetResourceParams.packetParams.maxNumIOConfigs     = 1;
        packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
        packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
        packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
        packetResourceParams.pDeviceIndices                   = NULL;
        packetResourceParams.numDevices                       = 0;
        packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEBinDataReadPacketManager", &packetResourceParams, &m_pDataPacketManger);
        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(m_pDataPacketManger, &pPacketResource);
            if ((CamxResultSuccess == result) && (NULL != pPacketResource))
            {
                CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
                // pResource points to a Packet so we may static_cast
                m_pEEbinDataReadPacket = static_cast<Packet*>(pPacketResource);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get command buffer");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create data read packet manager result: %d", result);
        }

        // Step2: Initialize and add memory map commnds(i2c info and memory details) to the packet
        if (CamxResultSuccess == result)
        {
            cmdResourceParams.usageFlags.cmdBuffer = 1;
            cmdResourceParams.resourceSize         = memoryMapSize;
            cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
            cmdResourceParams.cmdParams.type       = CmdType::I2C;
            cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
            cmdResourceParams.pDeviceIndices       = NULL;
            cmdResourceParams.numDevices           = 0;
            cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

            result = CmdBufferManager::Create("EEBinReadMemMapCmdBuffManager",
                                              &packetResourceParams,
                                              &pCmdManagerMemoryMapInfo);
            if (CamxResultSuccess == result)
            {
                result = GetCommandBuffer(pCmdManagerMemoryMapInfo, &pPacketResource);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create memory map cmd buffer manager result: %d", result);
            }

            if (CamxResultSuccess == result)
            {
                CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
                pMemoryMapCmd = static_cast<CmdBuffer*>(pPacketResource);

                // Step2a: Commit the I2cInfo command specific to the memory block
                CSLSensorI2CInfo* pCmdI2CInfo =
                    reinterpret_cast<CSLSensorI2CInfo*>(pMemoryMapCmd->BeginCommands(I2CInfoCmdSize / sizeof(UINT32)));
                if (NULL != pCmdI2CInfo)
                {
                    result = ImageSensorUtils::CreateI2CInfoCmd(pCmdI2CInfo,
                                                                m_pEEbinDriver->dataReadInfo.regSetting[0].slaveAddr,
                                                                m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                    if (CamxResultSuccess == result)
                    {
                        result = pMemoryMapCmd->CommitCommands();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create i2c command");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve i2c info command area in command buffer");
                    result = CamxResultEFailed;
                }

                // Step2b: Commit the memorymap command specific to the memory block
                if (CamxResultSuccess == result)
                {
                    VOID* pCmdMemoryMapInfo =
                        pMemoryMapCmd->BeginCommands(readSequenceSize / sizeof(UINT32));
                    if (NULL != pCmdMemoryMapInfo)
                    {
                        result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdMemoryMapInfo,
                                                                             m_pEEbinDriver->dataReadInfo.regSettingCount,
                                                                             m_pEEbinDriver->dataReadInfo.regSetting,
                                                                             regSettingIndex,
                                                                             m_pEEbinDriver->slaveInfo.i2cFrequencyMode);
                        if (CamxResultSuccess == result)
                        {
                            result = pMemoryMapCmd->CommitCommands();
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create memory map command");
                            result = CamxResultEFailed;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve memory map command area in command buffer");
                        result = CamxResultEFailed;
                    }
                }

                // step2c: Add memory map (I2C info + memory info) command buffer to packet
                if (CamxResultSuccess == result)
                {
                    // Not associated with any request. Won't be recycled.
                    m_pEEbinDataReadPacket->SetRequestId(CamxInvalidRequestId);
                    result = m_pEEbinDataReadPacket->AddCmdBufferReference(pMemoryMapCmd, &cmdBufferIndex);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain memory map command buffer");
            }
        }

        // Step3: Initialize and add read data commnds to the packet
        if (CamxResultSuccess == result)
        {
            BufferManagerCreateData createData  = { };
            ImageFormat*            pFormat     = &createData.bufferProperties.imageFormat;
            UINT32                  IOConfigIndex;

            pFormat->width     = m_pEEBinDataSize;
            pFormat->height    = 1;
            pFormat->format    = Format::Blob;
            pFormat->alignment = 8;

            createData.maxBufferCount               = 1;
            createData.immediateAllocBufferCount    = 1;
            createData.deviceCount                  = 1;
            createData.deviceIndices[0]             = m_eebinDeviceIndex;
            createData.bufferProperties.memFlags    = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
            createData.allocateBufferMemory         = TRUE;
            createData.numBatchedFrames             = 1;
            createData.bufferProperties.bufferHeap  = CSLBufferHeapIon;
            createData.bufferManagerType            = BufferManagerType::CamxBufferManager;

            result = ImageBufferManager::Create("EEBIN_DATA", &createData, &m_pDataImageBufferManager);

            if (CamxResultSuccess == result)
            {
                m_pDataImage = m_pDataImageBufferManager->GetImageBuffer();

                if (NULL == m_pDataImage)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer manager");
                result = CamxResultEFailed;
            }

            if (CamxResultSuccess == result)
            {
                result = m_pEEbinDataReadPacket->AddIOConfig(m_pDataImage,
                                                             0,
                                                             CSLIODirection::CSLIODirectionOutput,
                                                             NULL,
                                                             0,
                                                             &IOConfigIndex,
                                                             NULL,
                                                             0);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to add IO config =%d", result);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate image buffer");
            }
        }

        // Step4: Add opcode and commit the packet
        if (CamxResultSuccess == result)
        {
            m_pEEbinDataReadPacket->SetOpcode(CSLDeviceTypeEEPROM, CSLPacketOpcodesEEPROMInitialConfig);
            result = m_pEEbinDataReadPacket->CommitPacket();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to commit packet =%d", result);
            }
        }

        // Step5: Submit the prepared packet to CSL/KMD
        if (CamxResultSuccess == result)
        {
            result = CSLSubmit(m_hEEbinSessionHandle,
                               m_hEEbinDevice,
                               m_pEEbinDataReadPacket->GetMemHandle(),
                               m_pEEbinDataReadPacket->GetOffset());
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit packet =%d", result);
            }
        }

        // Step6: get the buffer to which OTP data copied from CSL
        // Note: This logic works fine as the specifc IOCTL is synchronous but will use the fence logic to comply with design.
        // @todo (CAMX-1996) - Use fence to read the buffer in order to comply with the design.
        if (CamxResultSuccess == result)
        {
            m_pEEbinData = m_pDataImage->GetPlaneVirtualAddr(0, 0);
            if (NULL == m_pEEbinData)
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain eebin version memory buffer, result =%d", result);
            }
        }

        // Step7: Clean up the resources
        // These resources are managed manually, hence recycled manually.
        if (NULL != pCmdManagerMemoryMapInfo)
        {
            pCmdManagerMemoryMapInfo->Recycle(pMemoryMapCmd);
            CAMX_DELETE pCmdManagerMemoryMapInfo;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid legth to read: legth: %d", m_pEEBinDataSize);
        result = CamxResultEInvalidArg;
    }

    return result;
}

CAMX_NAMESPACE_END
