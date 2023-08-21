// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  icasetting.h
/// @brief ICA1_0 / ICA2_0 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ICASETTING_H
#define ICASETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "NcLibWarp.h"

/// @brief ICA10 Module Unpacked Data Structure
// NOWHINE NC004c: Share code with system team
struct ICAUnpackedField
{
    VOID*            pIcaParameter;          ///< Pointer pointing to IPE IQ settings
    VOID*            pRegData;               ///< Pointer to REG data
    VOID*            pCurrICAInData;         ///< ICA input current frame Data
    VOID*            pPrevICAInData;         ///< ICA reference current frame Data
    VOID*            pCurrWarpAssistData;    ///< Current Warp assist data
    VOID*            pPrevWarpAssistData;    ///< Current Warp assist data
    // Warp geometry data required by ANR/TF
    VOID*            pWarpGeometryData;      ///< Warp Geometry data
    VOID*            pCVPICAFrameCfgData;    ///< CVP ICA frame config data
    VOID*            pIcaGeoParameters;      ///< pICA geo pass parameters
};

/// @brief: This enumerator maps Look Up Tables indices with DMI LUT_SEL in ICA10 module SWI.
enum IPEICA10LUTIndex
{
    ICA10IndexPerspective = 0,    ///< Perspective Index
    ICA10IndexGrid0       = 1,    ///< Grid0 Index
    ICA10IndexGrid1       = 2,    ///< Grid1 Index
    ICA10Indexmax         = 3     ///< ICA Index max
};

/// @brief: This enumerator maps Look Up Tables indices with DMI LUT_SEL in ICA20 module SWI.
enum IPEICA20LUTIndex
{
    ICA20IndexPerspective   = 0,    ///< Perspective Index
    ICA20IndexGrid          = 1,    ///< Grid0 Index
    ICA20Indexmax           = 2     ///< ICA Index max
};

/// @brief: This enumerator maps Look Up Tables indices with DMI LUT_SEL in ICA10 module SWI.
enum IPEICA30LUTIndex
{
    ICA30IndexPerspective = 0,    ///< Perspective Index
    ICA30IndexGrid0       = 1,    ///< Grid0 Index
    ICA30IndexGrid1       = 2,    ///< Grid1 Index
    ICA30Indexmax         = 3     ///< ICA Index max
};

/// @ brief enumeration defining IPE path
enum class CamxIPEICAPath : UINT
{
    INPUT      = 0,    ///< Input
    REFERENCE  = 1,    ///< Reference
    CVPICA     = 2,    ///< CVP ICA
};

/// @brief: This structure has information of number of entries of each LUT in words for ICA10.
static const UINT32 IPEICA10LUTNumEntries[ICA10Indexmax] =
{
    72,     ///< Perspective LUT
    832,    ///< Grid LUT 0, these are 64 bit LUTs. 415 used.
    832,    ///< Grid LUT 1, these are 64 bit LUTs. 414 used.
};

/// @brief: This structure has information of number of entries of each LUT in words for ICA20.
static const UINT32 IPEICA20LUTNumEntries[ICA20Indexmax] =
{
    72,     ///< Perspective LUT
    1890,   ///< Grid LUT - 945 LUT entries 64 bit
};

/// @brief: This structure has information of number of entries of each LUT in words for ICA10.
static const UINT32 IPEICA30LUTNumEntries[ICA30Indexmax] =
{
    72,      ///< Perspective LUT
    3424,    ///< Grid LUT 0, these are 64 bit LUTs. 1709 used.
    3424,    ///< Grid LUT 1, these are 64 bit LUTs. 1708 used.
};

/// @brief: This array has information of number of entried of each LUT in bytes for ICA10.
static const UINT IPEICA10LUTSize[ICA10Indexmax] =
{
    static_cast<UINT>(IPEICA10LUTNumEntries[0] * sizeof(UINT32)),
    static_cast<UINT>(IPEICA10LUTNumEntries[1] * sizeof(UINT32)),
    static_cast<UINT>(IPEICA10LUTNumEntries[2] * sizeof(UINT32)),
};

/// @brief: This array has information of number of entried of each LUT in bytes for ICA20.
static const UINT IPEICA20LUTSize[ICA20Indexmax] =
{
    static_cast<UINT>(IPEICA20LUTNumEntries[0] * sizeof(UINT32)),
    static_cast<UINT>(IPEICA20LUTNumEntries[1] * sizeof(UINT32)),
};
/// @brief: This array has information of number of entried of each LUT in bytes for ICA30.
static const UINT IPEICA30LUTSize[ICA30Indexmax] =
{
    static_cast<UINT>(IPEICA30LUTNumEntries[0] * sizeof(UINT32)),
    static_cast<UINT>(IPEICA30LUTNumEntries[1] * sizeof(UINT32)),
    static_cast<UINT>(IPEICA30LUTNumEntries[2] * sizeof(UINT32)),
};

/// @brief: Total size of LUT Buffer for ICA10
static const UINT32 IPEICA10LUTBufferSize =
    IPEICA10LUTSize[ICA10IndexPerspective] +
    IPEICA10LUTSize[ICA10IndexGrid0] +
    IPEICA10LUTSize[ICA10IndexGrid1];

/// @brief: Total size of LUT Buffer for ICA20
static const UINT32 IPEICA20LUTBufferSize =
    IPEICA20LUTSize[ICA20IndexPerspective] +
    IPEICA20LUTSize[ICA20IndexGrid];

/// @brief: Total size of LUT Buffer for ICA30
static const UINT32 IPEICA30LUTBufferSize =
IPEICA30LUTSize[ICA30IndexPerspective] +
IPEICA30LUTSize[ICA30IndexGrid0] +
IPEICA30LUTSize[ICA30IndexGrid1];


// Total Entries 904 (Sum of all entries from all LUT curves)
static const UINT IPEMaxICA10LUTNumEntries  = 1736;
static const UINT IPEMaxICA20LUTNumEntries  = 1962;
static const UINT IPEMaxICA30LUTNumEntries  = 6920;

static const UINT ICA10GridRegSize          = 829;
static const UINT ICAPerspectiveSize        = 72;
static const UINT ICA10GridLUTSize          = 416;
static const UINT ICA30GridLUTSize          = 1709;

static const UINT ICA20GridRegSize          = 945;
static const UINT ICA30GridRegSize          = 3417;

static const UINT ICAInterpolationCoeffSets = 16;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements ICA10 / ICA20 /ICA30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class ICASetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput          ICA input data
    /// @param  pData           Chromatix region data pointer
    /// @param  pReserveData    Chromatix reserve data pointer
    /// @param  pOutput         Output data pointer
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const ICAInputData*                     pInput,
        VOID*                                   pData,
        VOID*                                   pReserveData,
        VOID*                                   pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInitializationData
    ///
    /// @brief  Get ICA initialization data
    ///
    /// @param  pData    Pointer to ICANCLibOutputData
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetInitializationData(
        struct ICANcLibOutputData* pData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateContextParams
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput  Pointer to input parameters
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ValidateContextParams(
        const ICAInputData*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpContextParams
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput    Pointer to input parameters
    /// @param  pData     Chromatix region data pointer
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DumpContextParams(
        const ICAInputData*            pInput,
        VOID*                          pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatixMod2ICAChromatix
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput           ICA input data
    /// @param  pReserveData     Chromatix reserve data pointer
    /// @param  pData            Chromatix region data pointer
    /// @param  pIcaChromatix    ICA Chromatix structure
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL  MapChromatixMod2ICAChromatix(
        const ICAInputData*                     pInput,
        VOID*                                   pReserveData,
        VOID*                                   pData,
        VOID*                                   pIcaChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatixMod2ICAv10Chromatix
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput           ICA input data
    /// @param  pReserveData     Chromatix reserve data pointer
    /// @param  pData            Chromatix region data pointer
    /// @param  pIcaChromatix    ICA Chromatix structure
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL  MapChromatixMod2ICAv10Chromatix(
        const ICAInputData*                     pInput,
        ica_1_0_0::chromatix_ica10_reserveType* pReserveData,
        ica_1_0_0::ica10_rgn_dataType*          pData,
        VOID*                                   pIcaChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatixMod2ICAv20Chromatix
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput           ICA input data
    /// @param  pReserveData     Chromatix reserve data pointer
    /// @param  pData            Chromatix region data pointer
    /// @param  pIcaChromatix    ICA Chromatix structure
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL  MapChromatixMod2ICAv20Chromatix(
        const ICAInputData*                     pInput,
        ica_2_0_0::chromatix_ica20_reserveType* pReserveData,
        ica_2_0_0::ica20_rgn_dataType*          pData,
        VOID*                                   pIcaChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapChromatixMod3ICAv30Chromatix
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput           ICA input data
    /// @param  pReserveData     Chromatix reserve data pointer
    /// @param  pData            Chromatix region data pointer
    /// @param  pIcaChromatix    ICA Chromatix structure
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL  MapChromatixMod2ICAv30Chromatix(
        const ICAInputData*                     pInput,
        ica_3_0_0::chromatix_ica30_reserveType* pReserveData,
        ica_3_0_0::ica30_rgn_dataType*          pData,
        VOID*                                   pIcaChromatix);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSuccess
    ///
    /// @brief  check if valid error returned
    ///
    /// @param  errorVal   error value
    /// @param  pStr       function string
    ///
    /// @return BOOL true if  success and false if error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsSuccess(
        BOOL errorVal,
        const CHAR* pStr)
    {
        BOOL retVal = FALSE;
        if (errorVal == 0)
        {
            retVal = TRUE;
        }
        else if (errorVal >= NC_LIB_WARN_INVALID_INPUT)
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "%s return warn %d", pStr, errorVal);
            retVal = TRUE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "%s returns error %d", pStr, errorVal);
        }
        return retVal;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckMctfTransformCondition
    ///
    /// @brief  Check MCTF Transform Condition.
    ///
    /// @param  pMCTF                             Pointer to NcLibCalcMctfIn Data
    /// @param  pWarpOut                          Pointer to NcLibWarp Data
    /// @param  mctfEis                           Flag indicating if mctf and eis are both enabled
    /// @param  byPassAlignmentMatrixAdjustement  Flag indicating either to bypass byPass Alignment Matrix Adjustement or not
    ///
    /// @return TRUE for doing MCTF Transform
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CheckMctfTransformCondition(
        VOID*  pMCTF,
        VOID*  pWarpOut,
        BOOL   mctfEis,
        BOOL   byPassAlignmentMatrixAdjustement);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckWarpGeometryCondition
    ///
    /// @brief  Check Warp geometry condition
    ///
    /// @param  pPrevInWarp    previous warp data
    /// @param  pCurInWarp     current warp dataPointer to NcLibWarp Data
    /// @param  pCurrAssist    Assist grid pointer
    /// @param  pGeomIn        Gometry input pointer
    ///
    /// @return TRUE for doing MCTF Transform
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CheckWarpGeometryCondition(
        NcLibWarp* pPrevInWarp,
        NcLibWarp* pCurInWarp,
        NcLibWarpBuildAssistGridOut* pCurrAssist,
        NcLibWarpGeomIn*             pGeomIn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMCTFInputOutput
    ///
    /// @brief  Dump mctf input / output data.
    ///
    /// @param  pInput   Ica input
    /// @param  pIn      Mctf input
    /// @param  pOut     mctf output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpMCTFInputOutput(
        const ICAInputData*    pInput,
        const NcLibCalcMctfIn* pIn,
        NcLibWarp*             pOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpGrid
    ///
    /// @brief  Dump grid
    ///
    /// @param  pFile  File input
    /// @param  pGrid  grid data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpGrid(
        FILE*                 pFile,
        const NcLibWarpGrid*  pGrid);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMatrices
    ///
    /// @brief  Dump matrices
    ///
    /// @param  pFile       File input
    /// @param  pMatrices   matrices data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpMatrices(
        FILE*                    pFile,
        const NcLibWarpMatrices* pMatrices);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpGeometryInputs
    ///
    /// @brief  Dump Geometry inputs
    ///
    /// @param  pGeomIn   Geomtery input parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpGeometryInputs(
        NcLibWarpGeomIn* pGeomIn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAssitGridInputs
    ///
    /// @brief  Dump Geometry inputs
    ///
    /// @param  pAssistGridIn   Assist Grid Input
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpAssitGridInputs(
        NcLibWarpBuildAssistGridIn* pAssistGridIn);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMfnrTransform
    ///
    /// @brief  Dump MFNR transform input
    ///
    /// @param  pMfnrTransform   MFNR transform input
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpMfnrTransform(
        NcLibWarp*      pMfnrTransform);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsvalidMCTFICAtransformcondition
    ///
    /// @brief  To check if Grid/Perspective transform is enabled in previous frame
    ///
    /// @param  pMCTF  Pointer to NcLibCalcMctfIn Data
    ///
    /// @return TRUE for doing MCTF Transform
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsvalidMCTFICAtransformcondition(
        VOID*  pMCTF)
    {
        NcLibCalcMctfIn* pIn    = static_cast<NcLibCalcMctfIn*>(pMCTF);
        BOOL             result = FALSE;
        if ((NULL != pIn->inputWarp) && (NULL != pIn->inputGridsPrevFrame) &&
            (NULL != pIn->inputGridsPrevFrame->inputWarp))
        {
            if ((pIn->inputWarp->grid.enable     == pIn->inputGridsPrevFrame->inputWarp->grid.enable)   &&
                (pIn->inputWarp->matrices.enable == pIn->inputGridsPrevFrame->inputWarp->matrices.enable))
            {
                result = TRUE;
            }
        }
        return result;
    }
};
#endif // ICASETTING_H
