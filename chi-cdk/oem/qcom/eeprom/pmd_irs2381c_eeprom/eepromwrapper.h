/****************************************************************************\
* Copyright (C) 2017 pmdtechnologies ag
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#ifdef _MSC_VER
#define EEPROM_API __declspec(dllexport)
#else
#define EEPROM_API __attribute__ ((visibility ("default")))
#endif


/**
* Enum for the EEPROM layout version
*/
typedef enum
{
    EEPROM_V1,
    EEPROM_V2,
    EEPROM_V7,
    EEPROM_ZWETSCHGE
} EEPROMVersion;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intr_cal_info
{
    float c_x;
    float c_y;
    float f_x;
    float f_y;
    float distortionTangential_p1;
    float distortionTangential_p2;
    float distortionRadial_k1;
    float distortionRadial_k2;
    float distortionRadial_k3;
    float s; //skew
} int_cal_info;

/// Dynamic library handle
typedef VOID* EEPROMCUTTERLIBRARYHANDLE;
typedef VOID* OSLIBRARYHANDLE;

typedef int (*CUTEEPROMCONTENT)(
    EEPROMVersion cutterVersion,
    char const *inputFile,
    char const *outputPath);

typedef int(*READLENSPARAMETERS)(
    EEPROMVersion cutterVersion,
    char const *inputFile,
    int_cal_info *intrinsics);

EEPROMCUTTERLIBRARYHANDLE m_hEepromLib;
CUTEEPROMCONTENT  m_cuteepromcontent;
READLENSPARAMETERS m_readlensparameters;

/**
* Wraps the cutting of different EEPROM formats.
* The formats that are currently suppported are V1 and V2
*
* \param cutterVersion Decides which EEPROMVersion should be cut
* \param inputFile Path to the EEPROM content file, that should be cut
* \param outputPath Path to the folder, where the cut EEPROM content should be stored
*
* \return 0 for success, 1 otherwise
*/
EEPROM_API int cutEEPROMContent (EEPROMVersion cutterVersion, char const *inputFile, char const *outputPath);


/**
* Reads the lens parameters from file and returns them
* This is currently only supported for the V7 format
*
* \param cutterVersion Decides which EEPROMVersion should be used
* \param inputFile Path to the lens parameters file, that shoudl be read
* \param intrinsics Struct of the intrinsics
*
* \return 0 for success, 1 otherwise
*/
EEPROM_API int readLensParameters (EEPROMVersion cutterVersion, char const *inputFile, int_cal_info *intrinsics);

#ifdef __cplusplus
}
#endif