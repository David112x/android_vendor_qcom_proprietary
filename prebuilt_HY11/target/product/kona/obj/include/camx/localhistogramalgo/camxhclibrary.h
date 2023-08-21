////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHCLIBRARY_H__
#define CAMXHCLIBRARY_H__
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "camxdefs.h"
#include "chihistalgointerface.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "camxincs.h"
#include "camxhcfunc.h"
#include "camxhclibwrapper.h"

CAMX_NAMESPACE_BEGIN

#ifdef __cplusplus
extern "C" {
#endif


static const UINT D_IMAGE_FORMAT_RAW10PACKED = 0; // Image Format of RAW10 Packed

typedef enum
{
    D_HC_SUCCESS                        = 0,
    D_HC_ERROR_ALREADY_INITIALIZED      = -1,
    D_HC_ERROR_CONF_NULL                = -2,
    D_HC_ERROR_IMAGE_SIZE_OUTRANGE      = -3,
    D_HC_ERROR_IMAGE_FORMAT_OUTRANGE    = -4,
    D_HC_ERROR_THREAD_NUM_OUTRANGE      = -5,
    D_HC_ERROR_NOT_INITIALIZED          = -6,
    D_HC_ERROR_DATA_IN_NULL             = -7,
    D_HC_ERROR_RAW_IMG_PACKED_NULL      = -8,
    D_HC_ERROR_RAW_IMG_NOPACKED_NULL    = -9,
    D_HC_ERROR_HIST_NULL                = -10,
    D_HC_ERROR_PARAM_NULL               = -11,
    D_HC_ERROR_GAIN_OUTRANGE            = -12,
    D_HC_ERROR_PARAM_MODE_OUTRANGE      = -13,
    D_HC_ERROR_DATA_OUT_NULL            = -14,
    D_HC_ERROR_SYSTEM_OFFSET            = -100
} HistLibraryErrorCodes;

/// @brief Configuration Data
struct HCLibConfig
{
    INT32           Width;              // Width of the Image data
    INT32           Height;             // Height of the Image data
    INT32           ImageFormat;        // Image Format ( RAW10 Packed Format or 16Bits Format )
    INT32           nThreads;           // The Number of Thread.
};
// Input Data
struct HCLibDataIn
{
    UINT8           *pImgRaw10Packed;  // Image Data in RAW10 Packed Format.
    UINT16          *pHist;       // Local Y HIST Data of 8x6 Blocks with 18 Bin Data.
};// Parameter
struct HCLibParam
{
    UINT32          TotalGain;          // Total Gain ( Analog Gain x Digital Gain )
    UINT16          ParamMode;          // Parameter Mode
};
// Output Data
struct HCLibDataOut
{
    UINT8           LTCRatio;               // LTC Ratio for Still
    UINT8           LTCRatioLowpassOn;      // LTC Ratio for Preview
    INT32           LTCRatio_percentage;    // LTC Ratio for Still
};


/// @brief Histogram Internat Data struct
struct HistProcessInternalData
{
    FLOAT DRCGainLTM;             ///<  DRC Gain
    FLOAT darkBoostGain;          ///<  Dark Boost Gain
    FLOAT sensorGain;             ///<  SensorGain
    BYTE* pImageAddress;          ///<  Image Address
    UINT8 LTCRegUnit;             ///<  Converted LTC ratio to register unit
    INT32 LTCRatioPercentage;     ///<  LTC Ratio for Still

};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HCLibInitialize
///
/// @brief  This method initializes Histogram Library
///
/// @param  pConfiguration    Configuration Data
///
/// @return error code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 HCLibInitialize(
    HCLibConfig* pConfiguration);



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HCLibExecute
///
/// @brief  This executes Histogram Library
///
/// @param  pDataIn    Input Data
/// @param  pParam     Parameter
/// @param  pDataOut   Output Data
///
/// @return error code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 HCLibExecute(
    HCLibDataIn*  pDataIn,
    HCLibParam*   pParam,
    HCLibDataOut* pDataOut);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistCoreProcess
///
/// @brief  This method creates an instance to the Local histogram algo.
///
/// @param  pChistCoreAlgorithm   Pointer to CHIHistAlgorithm instance
/// @param  pInputs                 Pointer to stats inputs interface
/// @param  pOutputs                Pointer to stats data output by the algorithm
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistCoreProcess(
    HistCoreAlgorithm*                 pChiHistCoreAlgorithm,
    const HistAlgoProcessInputList*    pInputs,
    HistAlgoProcessOutputList*         pOutputs);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MapHistAlgoInputParameters
///
/// @brief  This method maps the algo input parameters to the core histogram algo data
///
/// @param  pInputs                Pointer to stats inputs interface
/// @param  pHistData              Pointer to Hist Internal data
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MapHistAlgoInputParameters(
    const HistAlgoProcessInputList*    pInputs,
    HistProcessInternalData*           pHistData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MapHistAlgoOutputParameters
///
/// @brief  This method maps the algo input parameters to the core Hist algo data
///
/// @param  pOutput                  Pointer to stats output interface
/// @param  pHistData                Pointer to Hist Internal data
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MapHistAlgoOutputParameters(
    const HistAlgoProcessOutputList*   pOutput,
    HistProcessInternalData*           pHistData);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistCoreInit
///
/// @brief  This method creates an instance to the Local histogram algo.
///
/// @param  pCreateParams          Param list
/// @param  ppHistAlgoInstance     hist Algo Instance
///
/// @return CDKResult Success/Failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult HistCoreInit(
    const HistAlgoProcessCreateParamList*   pCreateParams,
    HistCoreAlgorithm**                     ppHistAlgoInstance);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistCoreDestroy
///
/// @brief  This method destorys the instance to the Local histogram algo.
///
/// @param  pHistAlgoInstance     Hist Algo Instance
/// @param  pDestroyParams        Destroy Param list
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistCoreDestroy(
    HistCoreAlgorithm*                       pHistAlgoInstance,
    const HistAlgoProcessDestroyParamList*   pDestroyParams);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LocalHistCalculation
///
/// @brief  This function runs local histogram library function
///
/// @param  pHistData   Pointer to CHIHistAlgorithm instance
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID LocalHistCalculation(
    HistProcessInternalData*    pHistData);
#ifdef __cplusplus
}
#endif

CAMX_NAMESPACE_END
#endif
