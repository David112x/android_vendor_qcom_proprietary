////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchiiqutilshandler.cpp
/// @brief Implements the CAMX Chi IQ Utils Handler.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxchiiqutilshandler.h"
#include "chiipedefs.h"
#include "NcLibWarp.h"

#undef LOG_TAG
#define LOG_TAG "CHIIQUTILS"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateInverseGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult CalculateInverseGrid(
    CHIIQUTILSPARAMS* pChiIQUtilsParams)
{
    CDKResult result = CDKResultSuccess;
    BOOL      retVal = TRUE;

    CAMX_ASSERT(NULL != pChiIQUtilsParams);
    CAMX_ASSERT(NULL != pChiIQUtilsParams->pInputData);
    CAMX_ASSERT(NULL != pChiIQUtilsParams->pOutputData);

    IPEICAInverGridInput*    pInverGridInput  = reinterpret_cast<IPEICAInverGridInput*>(pChiIQUtilsParams->pInputData);
    IPEICAInverseGridOutput* pInverGridOutput = reinterpret_cast<IPEICAInverseGridOutput*>(pChiIQUtilsParams->pOutputData);

    if ((NULL == pInverGridInput->srcGridRow)    ||
        (NULL == pInverGridInput->srcGridColumn) ||
        (NULL == pInverGridInput->dstGridArray)  ||
        (NULL == pInverGridOutput->inverseGridArray))
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid inverse grid calculation arrays");
        result = CDKResultEInvalidPointer;
    }

    ///< Initialize reverse grid structures used by NcLib
    NcLibWarpReverseGridDirectionIn reversegridIn;
    memset(&reversegridIn, 0, sizeof(NcLibWarpReverseGridDirectionIn));

    if (CDKResultSuccess == result)
    {
        reversegridIn.numRows                         = pInverGridInput->numRows;
        reversegridIn.numColumns                      = pInverGridInput->numColumns;
        reversegridIn.srcGridRowCoords                = pInverGridInput->srcGridRow;
        reversegridIn.srcGridColumnCoords             = pInverGridInput->srcGridColumn;
        reversegridIn.dstCoords                       = reinterpret_cast<NcLibWarpGridCoord*>
                                                                         (pInverGridInput->dstGridArray);
        reversegridIn.inputTriangulationType          = static_cast<NcLibUniformGridTriangulation>
                                                                    (pInverGridInput->inputTriangulationType);
        reversegridIn.extrapolationType               = static_cast<NcLibReverseGridExtrapolationType>
                                                                    (pInverGridInput->extrapolationType);
        reversegridIn.reversedGridNumRows             = pInverGridInput->reversedGridNumRows;
        reversegridIn.reversedGridNumColumns          = pInverGridInput->reversedGridNumColumns;
        reversegridIn.reversedGridSourceTopLeft.x     = pInverGridInput->reversedGridSourceTopLeftX;
        reversegridIn.reversedGridSourceTopLeft.y     = pInverGridInput->reversedGridSourceTopLeftY;
        reversegridIn.reversedGridSourceBottomRight.x = pInverGridInput->reversedGridSourceBottomRightX;
        reversegridIn.reversedGridSourceBottomRight.y = pInverGridInput->reversedGridSourceBottomRightY;
        reversegridIn.useExtraCalcsDuringExtrpolation =
            ((pInverGridInput->useExtraCalcsDuringExtrapolation & TRUE ) == TRUE);

        LOG_VERBOSE(CamxLogGroupChi,
                    "Inverse grid: in row %u, in column %u, triangulation %d, extp type %d, out row %u, out column %u,"
                    "topLeft x %.6lf, topLeft y %.6lf, bottomRight x %.6lf, bottomRight y %.6lf, extp %d",
                    reversegridIn.numRows,
                    reversegridIn.numColumns,
                    reversegridIn.inputTriangulationType,
                    reversegridIn.extrapolationType,
                    reversegridIn.reversedGridNumRows,
                    reversegridIn.reversedGridNumColumns,
                    reversegridIn.reversedGridSourceTopLeft.x,
                    reversegridIn.reversedGridSourceTopLeft.y,
                    reversegridIn.reversedGridSourceBottomRight.x,
                    reversegridIn.reversedGridSourceBottomRight.y,
                    reversegridIn.useExtraCalcsDuringExtrpolation);

        NcLibWarpGridCoord* pOutputGrid = reinterpret_cast<NcLibWarpGridCoord*>
            (CAMX_CALLOC(sizeof(NcLibWarpGridCoord) * ICA20GridTransformWidth * ICA20GridTransformHeight));

        if (NULL == pOutputGrid)
        {
            LOG_ERROR(CamxLogGroupChi, "Memory alloc failed for output grid");
            result = CDKResultEFailed;
        }

        NcLibWarpReverseGridStatistics reversegridstats;
        memset(&reversegridstats, 0, sizeof(NcLibWarpReverseGridStatistics));

        if (CDKResultSuccess == result)
        {
            retVal = NcLibWarpReverseGridDirection(&reversegridIn, pOutputGrid, &reversegridstats);
            result = retVal ? CDKResultEFailed : CDKResultSuccess;
        }

        if (CDKResultSuccess == result)
        {
            pInverGridOutput->extrapolationPercentage = reversegridstats.extrapolationPercentage;

            for (UINT idx = 0; idx < (ICA20GridTransformHeight * ICA20GridTransformWidth); idx++)
            {
                pInverGridOutput->inverseGridArray[idx].x = pOutputGrid[idx].x;
                pInverGridOutput->inverseGridArray[idx].y = pOutputGrid[idx].y;
            }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "InverseGrid calculation failed");
        }

        CAMX_FREE(pOutputGrid);
        pOutputGrid = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ProcessChiIQUtils
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult ProcessChiIQUtils(
    CHIIQUTILSPARAMS* pChiIQUtilsParams)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pChiIQUtilsParams)             ||
        (NULL == pChiIQUtilsParams->pInputData) ||
        (NULL == pChiIQUtilsParams->pOutputData))
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid IQ Utils parameters");
        result = CDKResultEInvalidPointer;
    }
    else
    {
        switch (pChiIQUtilsParams->paramType)
        {
            case CHIIQUTILSPARAMSTYPE::IQUtilsCalculateInverseGrid:
                LOG_VERBOSE(CamxLogGroupChi, "IQUtilsCalculateInverseGrid Type");
                result = CalculateInverseGrid(pChiIQUtilsParams);
                break;

            default:
                LOG_VERBOSE(CamxLogGroupChi, "Unknown IQ utility");
                result = CDKResultEUnsupported;
                break;
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Process Chi IQ Utils failed result %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiIQUtilsGet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiIQUtilsGet(
    CHIIQUTILS* pChiIQUtils)
{
    if (NULL != pChiIQUtils)
    {
        pChiIQUtils->pProcessChiIQUtils = ProcessChiIQUtils;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
