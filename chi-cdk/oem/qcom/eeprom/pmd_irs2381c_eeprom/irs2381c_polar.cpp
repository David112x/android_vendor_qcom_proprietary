////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 - 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <android/log.h>
#include <string.h>
#include <unistd.h>
#include "camxeepromdriverapi.h"
#include "eepromwrapper.h"
#include "depthsensorutil.h"


static const CHAR   SharedObjectName[] = "libeepromcutter"; ///< The name of the shared object to be loaded
                                                            /// File system path separator character
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetLensIntrinsicInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetLensIntrinsicInfo()
{
    intr_cal_info temp;

    if (m_readlensparameters(EEPROM_ZWETSCHGE, "/data/vendor/camera/lensdata.dat", &temp))
    {
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%s:%d readLensParameters failed",
                             __func__, __LINE__);
        return FALSE;
    }

    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: f_x: %.10f", temp.f_x);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: f_y: %.10f", temp.f_y);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: c_x: %.10f", temp.c_x);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: c_y: %.10f", temp.c_y);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: distortionRadial_k1: %.10f", temp.distortionRadial_k1);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: distortionRadial_k2: %.10f", temp.distortionRadial_k2);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: distortionTangential_p1: %.10f",
                         temp.distortionTangential_p1);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: distortionTangential_p2: %.10f",
                         temp.distortionTangential_p2);
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Lens parameters: distortionRadial_k3: %.10f", temp.distortionRadial_k3);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// format_lensshading
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void format_lensshading (UINT8 *pOTPdata, UINT32 pOTPdatasize, LSCCalibration *pLSCData)
{
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "We are in the lensshading function");

    if (pOTPdata == NULL || pOTPdatasize == 0 || pLSCData == NULL)
    {
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Failed. %p, %d", pOTPdata, pOTPdatasize);
    }
    else
    {
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "First few bytes:");
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[0]);
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[1]);
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[2]);
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[3]);
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[4]);
        __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%c", pOTPdata[5]);

        pLSCData->settings.regSettingCount = 0;

        FILE *pFile;
        pFile = fopen ("/data/vendor/camera/eeprom.bin", "wb");
        if (pFile != NULL)
        {
            SIZE_T len = fwrite (pOTPdata, sizeof (UINT8), pOTPdatasize, pFile);
            if (len == 0 || len != pOTPdatasize)
            {
                __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Failed to write eeprom data, len %lu, expected %u",
                                    (unsigned long) len, pOTPdatasize);
            }
            fclose (pFile);

            if (pOTPdata[0] == 'P')
            {
                __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Detected V7 EEPROM format");
                m_cuteepromcontent(EEPROM_V7, "/data/vendor/camera/eeprom.bin", "/data/vendor/camera");
            }
            else if (pOTPdata[0] == 'Z')
            {
                __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "Detected ZWETSCHGE EEPROM format");
                m_cuteepromcontent(EEPROM_ZWETSCHGE, "/data/vendor/camera/eeprom.bin", "/data/vendor/camera");
            }
        }
        else
        {
            __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "%s:%d fopen failed",
                                __func__, __LINE__);
        }

        GetLensIntrinsicInfo();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetEEPROMLibraryAPIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetEEPROMLibraryAPIs (
    EEPROMLibraryAPI *pEEPROMLibraryAPI)
{
    __android_log_print (ANDROID_LOG_ERROR, "ROYALE", "We are in the EEPROM driver");

    char libFullName[FILENAME_MAX];

    DepthSensorUtil::SNPrintF(libFullName,
        FILENAME_MAX,
        "%s%s%s.%s",
        VendorLibPath,
        PathSeparator,
        SharedObjectName,
        SharedLibraryExtension);
    m_hEepromLib = DepthSensorUtil::LibMapFullName(libFullName);
    if (NULL == m_hEepromLib)
    {
        __android_log_print(ANDROID_LOG_ERROR, "ROYALE", "Error loading lib");
    }
    else
    {
        m_cuteepromcontent = reinterpret_cast<CUTEEPROMCONTENT>(
            DepthSensorUtil::LibGetAddr(m_hEepromLib, "cutEEPROMContent"));
        m_readlensparameters = reinterpret_cast<READLENSPARAMETERS>(
            DepthSensorUtil::LibGetAddr(m_hEepromLib, "readLensParameters"));
        if (NULL == m_cuteepromcontent || NULL == m_readlensparameters)
        {
            __android_log_print(ANDROID_LOG_ERROR, "ROYALE", "Error getting function address(es) from library:");
        }
    }

    pEEPROMLibraryAPI->majorVersion          = 1;
    pEEPROMLibraryAPI->minorVersion          = 0;
    pEEPROMLibraryAPI->pFormatLSCSettings    = format_lensshading;
}
