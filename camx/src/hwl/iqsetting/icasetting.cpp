// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  icasetting.cpp
/// @brief IPE ICA10 / ICA20 /ICA30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "icasetting.h"
#include "Process_ICA.h"
#include "Process_CVP.h"
#include "NcLibChipInfo.h"
#include "GeoLibUtils.h"

CAMX_STATIC_ASSERT(sizeof(struct FDData) == sizeof(FD_CONFIG_CONTEXT));
CAMX_STATIC_ASSERT(0 == static_cast<UINT>(CamxIPEICAPath::INPUT));
CAMX_STATIC_ASSERT(1 == static_cast<UINT>(CamxIPEICAPath::REFERENCE));
CAMX_STATIC_ASSERT(2 == static_cast<UINT>(CamxIPEICAPath::CVPICA));

static const UINT ICAMaxPerspectiveTransform           = 9;
static const UINT ICAParametersPerPerspectiveTransform = 9;
static const UINT GridAssistRows                       = 16;
static const UINT GridAssistColumns                    = 16;
static const UINT GridExtarpolateCornerSize            = 4;

CAMX_STATIC_ASSERT(sizeof(GeoIcaParameters) == sizeof(IcaGeoParameters));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::ValidateContextParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::ValidateContextParams(
    const ICAInputData*             pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    // Taken care by NClib currently

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpContextParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::DumpContextParams(
    const ICAInputData*            pInput,
    VOID*                          pData)
{
    NcLibWarp* pWarp = static_cast<NcLibWarp*>(pInput->pCurrICAInData);
    ica_1_0_0::ica10_rgn_dataType*    pICA10Data = NULL;
    ica_2_0_0::ica20_rgn_dataType*    pICA20Data = NULL;
    ica_3_0_0::ica30_rgn_dataType*    pICA30Data = NULL;

    if (TRUE == NcLibChipInfo::IsKona())
    {
        pICA30Data = static_cast<ica_3_0_0::ica30_rgn_dataType*>(pData);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation type %f", pICA30Data->y_interpolation_type);
    }
    else if (TRUE == NcLibChipInfo::IsHana())
    {
        pICA20Data = static_cast<ica_2_0_0::ica20_rgn_dataType*>(pData);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation type %f", pICA20Data->y_interpolation_type);
    }
    else
    {
        pICA10Data = static_cast<ica_1_0_0::ica10_rgn_dataType*>(pData);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation type %f", pICA10Data->y_interpolation_type);
    }

    if (NULL != pWarp)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "warp enable %d , center %d, row %d, column %d",
                         pWarp->matrices.enable,
                         pWarp->matrices.centerType,
                         pWarp->matrices.numRows,
                         pWarp->matrices.numColumns);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "width  %d, height %d",
                         pWarp->matrices.transformDefinedOn.widthPixels,
                         pWarp->matrices.transformDefinedOn.heightLines);
        for (UINT i = 0; i < ICAMaxPerspectiveTransform; i++)
        {
            for (UINT j = 0; j < ICAParametersPerPerspectiveTransform; j++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " matrices [%d] [%d] :  %f",
                                 i , j, pWarp->matrices.perspMatrices[i].T[j]);
            }
        }
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::MapChromatixMod2ICAChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  ICASetting::MapChromatixMod2ICAChromatix(
    const ICAInputData*                     pInput,
    VOID*                                   pVReserveData,
    VOID*                                   pVData,
    VOID*                                   pChromatix)
{
    ica_1_0_0::chromatix_ica10_reserveType* pICA10ReserveData = NULL;
    ica_1_0_0::ica10_rgn_dataType*          pICA10Data        = NULL;
    ica_2_0_0::chromatix_ica20_reserveType* pICA20ReserveData = NULL;
    ica_2_0_0::ica20_rgn_dataType*          pICA20Data        = NULL;
    ica_3_0_0::chromatix_ica30_reserveType* pICA30ReserveData = NULL;
    ica_3_0_0::ica30_rgn_dataType*          pICA30Data        = NULL;

    if ((TRUE == NcLibChipInfo::IsKona()) && (static_cast<UINT>(CamxIPEICAPath::CVPICA) != pInput->IPEPath))
    {
        pICA30ReserveData = static_cast<ica_3_0_0::chromatix_ica30_reserveType*>(pVReserveData);
        pICA30Data = static_cast<ica_3_0_0::ica30_rgn_dataType*>(pVData);
        MapChromatixMod2ICAv30Chromatix(pInput, pICA30ReserveData, pICA30Data, pChromatix);
    }
    else if ((TRUE == NcLibChipInfo::IsHana()) || (static_cast<UINT>(CamxIPEICAPath::CVPICA) == pInput->IPEPath))
    {
        pICA20ReserveData = static_cast<ica_2_0_0::chromatix_ica20_reserveType*>(pVReserveData);
        pICA20Data        = static_cast<ica_2_0_0::ica20_rgn_dataType*>(pVData);
        MapChromatixMod2ICAv20Chromatix(pInput, pICA20ReserveData, pICA20Data, pChromatix);
    }
    else
    {
        pICA10ReserveData = static_cast<ica_1_0_0::chromatix_ica10_reserveType*>(pVReserveData);
        pICA10Data = static_cast<ica_1_0_0::ica10_rgn_dataType*>(pVData);
        MapChromatixMod2ICAv10Chromatix(pInput, pICA10ReserveData, pICA10Data, pChromatix);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::MapChromatixMod2ICAv10Chromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  ICASetting::MapChromatixMod2ICAv10Chromatix(
    const ICAInputData*                     pInput,
    ica_1_0_0::chromatix_ica10_reserveType* pReserveData,
    ica_1_0_0::ica10_rgn_dataType*          pData,
    VOID*                                   pChromatix)
{

    ICA_Chromatix*  pIcaChromatix           = static_cast<ICA_Chromatix*>(pChromatix);

    pIcaChromatix->top.y_interpolation_type = IQSettingUtils::RoundFLOAT(pData->y_interpolation_type);

    pIcaChromatix->opg.opg_invalid_output_treatment_calculate = pReserveData->opg_invalid_output_treatment_calculate;
    pIcaChromatix->opg.opg_invalid_output_treatment_y         = pReserveData->opg_invalid_output_treatment_y;
    pIcaChromatix->opg.opg_invalid_output_treatment_cb        = pReserveData->opg_invalid_output_treatment_cb;
    pIcaChromatix->opg.opg_invalid_output_treatment_cr        = pReserveData->opg_invalid_output_treatment_cr;

    // Add static assert
    for (UINT idx = 0; idx < ICAInterpolationCoeffSets; idx++)
    {
        pIcaChromatix->opg.opg_interpolation_lut_0[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_1[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_2[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[idx]);
    }

    // Add static assert
    for (UINT idx = 0; idx < ICA10GridRegSize; idx++)
    {
        pIcaChromatix->ctc.ctc_grid_x[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_x_tab.ctc_grid_x[idx]);
        pIcaChromatix->ctc.ctc_grid_y[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_y_tab.ctc_grid_y[idx]);
        pIcaChromatix->ld_i2u_grid_x[idx]  =
            IQSettingUtils::RoundFLOAT(
                pData->distorted_input_to_undistorted_ldc_grid_x_tab.distorted_input_to_undistorted_ldc_grid_x[idx]);
        pIcaChromatix->ld_i2u_grid_y[idx]  =
            IQSettingUtils::RoundFLOAT(
                pData->distorted_input_to_undistorted_ldc_grid_y_tab.distorted_input_to_undistorted_ldc_grid_y[idx]);
    }

    if (TRUE == pInput->isGridFromChromatixEnabled)
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable =
            (NULL != pInput->pChromatix) ? (static_cast <ica_1_0_0::chromatix_ica10Type*>(
                pInput->pChromatix))->enable_section.ctc_transform_grid_enable : 0;
    }
    else
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable = 0;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::MapChromatixMod2ICAv20Chromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  ICASetting::MapChromatixMod2ICAv20Chromatix(
    const ICAInputData*                     pInput,
    ica_2_0_0::chromatix_ica20_reserveType* pReserveData,
    ica_2_0_0::ica20_rgn_dataType*          pData,
    VOID*                                   pChromatix)
{

    ICA_Chromatix*  pIcaChromatix           = static_cast<ICA_Chromatix*>(pChromatix);

    pIcaChromatix->top.y_interpolation_type = IQSettingUtils::RoundFLOAT(pData->y_interpolation_type);

    pIcaChromatix->opg.opg_invalid_output_treatment_calculate         = pReserveData->opg_invalid_output_treatment_calculate;
    pIcaChromatix->opg.opg_invalid_output_treatment_y                 = pReserveData->opg_invalid_output_treatment_y;
    pIcaChromatix->opg.opg_invalid_output_treatment_cb                = pReserveData->opg_invalid_output_treatment_cb;
    pIcaChromatix->opg.opg_invalid_output_treatment_cr                = pReserveData->opg_invalid_output_treatment_cr;
    pIcaChromatix->ld_i2u_grid_valid                                  =
        pReserveData->distorted_input_to_undistorted_ldc_grid_valid;
    pIcaChromatix->ld_u2i_grid_valid                                  =
        pReserveData->undistorted_to_lens_distorted_output_ld_grid_valid;

    // Add static assert
    for (UINT idx = 0; idx < ICAInterpolationCoeffSets; idx++)
    {
        pIcaChromatix->opg.opg_interpolation_lut_0[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_1[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_2[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[idx]);
    }

    // Add static assert
    for (UINT idx = 0; idx < ICA20GridRegSize; idx++)
    {
        pIcaChromatix->ctc.ctc_grid_x[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_x_tab.ctc_grid_x[idx]);
        pIcaChromatix->ctc.ctc_grid_y[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_y_tab.ctc_grid_y[idx]);

        pIcaChromatix->ld_i2u_grid_x[idx]  =
            IQSettingUtils::RoundFLOAT(
                pData->distorted_input_to_undistorted_ldc_grid_x_tab.distorted_input_to_undistorted_ldc_grid_x[idx]);
        pIcaChromatix->ld_i2u_grid_y[idx]  =
            IQSettingUtils::RoundFLOAT(
                pData->distorted_input_to_undistorted_ldc_grid_y_tab.distorted_input_to_undistorted_ldc_grid_y[idx]);
    }

    if (TRUE == pInput->isGridFromChromatixEnabled)
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable =
            (NULL != pInput->pChromatix) ? (static_cast <ica_2_0_0::chromatix_ica20Type*>(
                pInput->pChromatix))->enable_section.ctc_transform_grid_enable : 0;
    }
    else
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable = 0;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::MapChromatixMod2ICAv30Chromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  ICASetting::MapChromatixMod2ICAv30Chromatix(
    const ICAInputData*                     pInput,
    ica_3_0_0::chromatix_ica30_reserveType* pReserveData,
    ica_3_0_0::ica30_rgn_dataType*          pData,
    VOID*                                   pChromatix)
{

    ICA_Chromatix*  pIcaChromatix           = static_cast<ICA_Chromatix*>(pChromatix);

    pIcaChromatix->top.y_interpolation_type                   = IQSettingUtils::RoundFLOAT(pData->y_interpolation_type);
    pIcaChromatix->ld_i2u_grid_geometry                       = pReserveData->ctc_grid_transform_geometry;
    pIcaChromatix->opg.opg_invalid_output_treatment_calculate = pReserveData->opg_invalid_output_treatment_calculate;
    pIcaChromatix->opg.opg_invalid_output_treatment_y         = pReserveData->opg_invalid_output_treatment_y;
    pIcaChromatix->opg.opg_invalid_output_treatment_cb        = pReserveData->opg_invalid_output_treatment_cb;
    pIcaChromatix->opg.opg_invalid_output_treatment_cr        = pReserveData->opg_invalid_output_treatment_cr;
    pIcaChromatix->ctc.ctc_grid_transform_geometry            = pReserveData->ctc_grid_transform_geometry;

    // Add static assert
    for (UINT idx = 0; idx < ICAInterpolationCoeffSets; idx++)
    {
        pIcaChromatix->opg.opg_interpolation_lut_0[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_1[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[idx]);
        pIcaChromatix->opg.opg_interpolation_lut_2[idx] =
            IQSettingUtils::RoundFLOAT(pData->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[idx]);
    }

    // Add static assert
    for (UINT idx = 0; idx < ICA30GridRegSize; idx++)
    {
        pIcaChromatix->ctc.ctc_grid_x[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_x_tab.ctc_grid_x[idx]);
        pIcaChromatix->ctc.ctc_grid_y[idx] = IQSettingUtils::RoundFLOAT(pData->ctc_grid_y_tab.ctc_grid_y[idx]);
    }

    if (TRUE == pInput->isGridFromChromatixEnabled)
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable =
            (NULL != pInput->pChromatix) ? (static_cast <ica_3_0_0::chromatix_ica30Type*>(
                pInput->pChromatix))->enable_section.ctc_transform_grid_enable : 0;
    }
    else
    {
        pIcaChromatix->ctc.ctc_transform_grid_enable = 0;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpAssitGridInputs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpAssitGridInputs(
    NcLibWarpBuildAssistGridIn* pAssistGridIn)
{
    if (NULL != pAssistGridIn)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "pAssistGridIn warp %p , row %d, column %d",
            pAssistGridIn->in, pAssistGridIn->numRows, pAssistGridIn->numColumns);
        if (NULL != pAssistGridIn->in)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp grid c %d,r %d,enable %d,trans-defined w %d, h %d,dir %d",
                pAssistGridIn->in->grid.numColumns,
                pAssistGridIn->in->grid.numRows,
                pAssistGridIn->in->grid.enable,
                pAssistGridIn->in->grid.transformDefinedOn.widthPixels,
                pAssistGridIn->in->grid.transformDefinedOn.heightLines,
                pAssistGridIn->in->direction);
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp-matrice c %d,r %d,en %d,trans-def w %d, h %d"
                "confid %d,center %d",
                pAssistGridIn->in->matrices.numColumns,
                pAssistGridIn->in->matrices.numRows,
                pAssistGridIn->in->matrices.enable,
                pAssistGridIn->in->matrices.transformDefinedOn.widthPixels,
                pAssistGridIn->in->matrices.transformDefinedOn.heightLines,
                pAssistGridIn->in->matrices.confidence,
                pAssistGridIn->in->matrices.centerType);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpGeometryInputs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpGeometryInputs(
    NcLibWarpGeomIn* pGeomIn)
{
    if (NULL != pGeomIn)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "input w*h %d x %d, margin w*h %d, %d, zoom full w*h %d, %d"
                       "window t %d, l %d, w: %d, h: %d, ife zoom full w*h %d, %d, window l %d, t %d, w %d, h %d",
                       pGeomIn->inputSize.widthPixels,
                       pGeomIn->inputSize.heightLines,
                       pGeomIn->stabilizationMargins.widthPixels,
                       pGeomIn->stabilizationMargins.heightLines,
                       pGeomIn->zoomWindow.fullWidth,
                       pGeomIn->zoomWindow.fullHeight,
                       pGeomIn->zoomWindow.windowLeft,
                       pGeomIn->zoomWindow.windowTop,
                       pGeomIn->zoomWindow.windowWidth,
                       pGeomIn->zoomWindow.windowHeight,
                       pGeomIn->ifeZoomWindow.fullWidth,
                       pGeomIn->ifeZoomWindow.fullHeight,
                       pGeomIn->ifeZoomWindow.windowLeft,
                       pGeomIn->ifeZoomWindow.windowTop,
                       pGeomIn->ifeZoomWindow.windowWidth,
                       pGeomIn->ifeZoomWindow.windowHeight);
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "fd prevFrame %d, scale %f ,inputGridsPrevFrame %p"
                       "alignment domain %d , grid %p tf x %d, y %d, anr x %d, y %d, asf %d, %d"
                       ",hnr x:%d,y:%d lenrx: %d y: %d, lnr domain %d",
                       pGeomIn->isFdConfigFromPrevFrame,
                       pGeomIn->ica1UpScaleRatio,
                       pGeomIn->inputGridsPrevFrame,
                       pGeomIn->alignmentDomain,
                       pGeomIn->inputGrids,
                       pGeomIn->tf_lnr_opt_center_x,
                       pGeomIn->tf_lnr_opt_center_y,
                       pGeomIn->anr_lnr_opt_center_x,
                       pGeomIn->anr_lnr_opt_center_y,
                       pGeomIn->asf_opt_center_x,
                       pGeomIn->asf_opt_center_y,
                       pGeomIn->hnr_opt_center_x,
                       pGeomIn->hnr_opt_center_y,
                       pGeomIn->lenr_opt_center_x,
                       pGeomIn->lenr_opt_center_y,
                       pGeomIn->lnrDomain);

        if (NULL != pGeomIn->fdConfig)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "face num %d", pGeomIn->fdConfig->faceNum);
            for (UINT fdNum = 0; fdNum < pGeomIn->fdConfig->faceNum; fdNum++)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "fdNum %d,centerx %d,center y %d,radius %d",
                    fdNum, pGeomIn->fdConfig->faceCenterX[fdNum],
                    pGeomIn->fdConfig->faceCenterY[fdNum],
                    pGeomIn->fdConfig->faceRadius[fdNum]);
            }
        }
        if (NULL != pGeomIn->inputGridsPrevFrame)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "assistgrid c %d, r %d, enable %d transform-defined w %d, h %d",
                pGeomIn->inputGridsPrevFrame->assistGrid->numColumns,
                pGeomIn->inputGridsPrevFrame->assistGrid->numRows,
                pGeomIn->inputGridsPrevFrame->assistGrid->enable,
                pGeomIn->inputGridsPrevFrame->assistGrid->transformDefinedOn.widthPixels,
                pGeomIn->inputGridsPrevFrame->assistGrid->transformDefinedOn.heightLines);
            if (NULL != pGeomIn->inputGridsPrevFrame->inputWarp)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp grid c %d,r %d,enable %d,trans-defined w %d, h %d,dir %d",
                    pGeomIn->inputGridsPrevFrame->inputWarp->grid.numColumns,
                    pGeomIn->inputGridsPrevFrame->inputWarp->grid.numRows,
                    pGeomIn->inputGridsPrevFrame->inputWarp->grid.enable,
                    pGeomIn->inputGridsPrevFrame->inputWarp->grid.transformDefinedOn.widthPixels,
                    pGeomIn->inputGridsPrevFrame->inputWarp->grid.transformDefinedOn.heightLines,
                    pGeomIn->inputGridsPrevFrame->inputWarp->direction);
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp-matrice c %d,r %d,en %d,trans-def w %d, h %d"
                    "confid %d,center %d",
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.numColumns,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.numRows,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.enable,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.transformDefinedOn.widthPixels,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.transformDefinedOn.heightLines,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.confidence,
                    pGeomIn->inputGridsPrevFrame->inputWarp->matrices.centerType);
            }
        }
        if (NULL != pGeomIn->inputGrids)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "assistgrid c %d, r %d, enable %d transform-defined w %d, h %d",
                pGeomIn->inputGrids->assistGrid->numColumns,
                pGeomIn->inputGrids->assistGrid->numRows,
                pGeomIn->inputGrids->assistGrid->enable,
                pGeomIn->inputGrids->assistGrid->transformDefinedOn.widthPixels,
                pGeomIn->inputGrids->assistGrid->transformDefinedOn.heightLines);
            if (NULL != pGeomIn->inputGrids->inputWarp)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp grid c %d,r %d,enable %d,trans-defined w %d, h %d,dir %d",
                    pGeomIn->inputGrids->inputWarp->grid.numColumns,
                    pGeomIn->inputGrids->inputWarp->grid.numRows,
                    pGeomIn->inputGrids->inputWarp->grid.enable,
                    pGeomIn->inputGrids->inputWarp->grid.transformDefinedOn.widthPixels,
                    pGeomIn->inputGrids->inputWarp->grid.transformDefinedOn.heightLines,
                    pGeomIn->inputGrids->inputWarp->direction);
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "warp-matrice c %d,r %d,en %d,trans-def w %d, h %d"
                    "confid %d,center %d",
                    pGeomIn->inputGrids->inputWarp->matrices.numColumns,
                    pGeomIn->inputGrids->inputWarp->matrices.numRows,
                    pGeomIn->inputGrids->inputWarp->matrices.enable,
                    pGeomIn->inputGrids->inputWarp->matrices.transformDefinedOn.widthPixels,
                    pGeomIn->inputGrids->inputWarp->matrices.transformDefinedOn.heightLines,
                    pGeomIn->inputGrids->inputWarp->matrices.confidence,
                    pGeomIn->inputGrids->inputWarp->matrices.centerType);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpMfnrTransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpMfnrTransform(
    NcLibWarp*  pMfnrTransform)
{
    if (NULL != pMfnrTransform)
    {
        //  Dump for matrix
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "DumpMfnrTransform matrix: c %d, r %d, enable %d, conf %u, centerType %u, W:%u H:%u",
                pMfnrTransform->matrices.numColumns,
                pMfnrTransform->matrices.numRows,
                pMfnrTransform->matrices.enable,
                pMfnrTransform->matrices.confidence,
                pMfnrTransform->matrices.centerType,
                pMfnrTransform->matrices.transformDefinedOn.widthPixels,
                pMfnrTransform->matrices.transformDefinedOn.heightLines);

        if (NULL != pMfnrTransform->matrices.perspMatrices)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "perspMatrices = %f, %f, %f, %f, %f, %f, %f, %f, %f",
                pMfnrTransform->matrices.perspMatrices[0].T[0],
                pMfnrTransform->matrices.perspMatrices[0].T[1],
                pMfnrTransform->matrices.perspMatrices[0].T[2],
                pMfnrTransform->matrices.perspMatrices[0].T[3],
                pMfnrTransform->matrices.perspMatrices[0].T[4],
                pMfnrTransform->matrices.perspMatrices[0].T[5],
                pMfnrTransform->matrices.perspMatrices[0].T[6],
                pMfnrTransform->matrices.perspMatrices[0].T[7],
                pMfnrTransform->matrices.perspMatrices[0].T[8]);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input MFNR transform is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::CalculateHWSetting(
    const ICAInputData*                     pInput,
    VOID*                                   pData,
    VOID*                                   pReserveData,
    VOID*                                   pOutput)
{
    BOOL   result                 = TRUE;
    BOOL   icaIsGridEnabledByFlow = 0;
    BOOL   icaPerspectiveEnabled  = 0;
    UINT32 ret                    = 0;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput))
    {
        ICAUnpackedField*   pUnpackedField = static_cast<ICAUnpackedField*>(pOutput);
        IcaParameters*      pIcaParams     = static_cast<IcaParameters*>(pUnpackedField->pIcaParameter);

        ICA_REG_v30*        pRegData       = static_cast<ICA_REG_v30*>(pInput->pNCRegData);
        ICA_Chromatix*      pChromatixData = static_cast<ICA_Chromatix*>(pInput->pNCChromatix);

        NcLibcvpDmeFrameConfigIca* pCVPICAFrameCfgOut =
            static_cast<NcLibcvpDmeFrameConfigIca*>(pUnpackedField->pCVPICAFrameCfgData);

        // Intialize Warp stucture used by NcLib
        NcLibWarp*          pCurInWarp     = static_cast<NcLibWarp*>(pInput->pCurrICAInData);
        NcLibWarp*          pCurRefWarp    = static_cast<NcLibWarp*>(pInput->pCurrICARefData);
        NcLibWarp*          pPrevInWarp    = static_cast<NcLibWarp*>(pInput->pPrevICAInData);
        NcLibWarpGeomOut*   pGeomOut       = static_cast<NcLibWarpGeomOut*>(pInput->pWarpGeomOut);

        NcLibWarpGridCoord*        pGrid   = static_cast<NcLibWarpGridCoord*>(pInput->pTempGridData);
        NcLibPerspTransformSingle  perspectiveMatrices[9];
        NcLibWarpGridCoord         gridExtrapolate[4];
        NcLibWarp                  warpOut;
        NcLibWarp*                 pRefWarp = pCurRefWarp;

        // Intialize Assist grid stucture used by NcLib
        NcLibWarpBuildAssistGridOut* pCurrAssist =
            static_cast<NcLibWarpBuildAssistGridOut*>(pInput->pCurWarpAssistData);
        NcLibWarpBuildAssistGridOut* pPrevAssist =
            static_cast<NcLibWarpBuildAssistGridOut*>(pInput->pPrevWarpAssistData);
        NcLibWarpBuildAssistGridIn      assistIn;
        NcLibWarpGeomIn                 geomIn;
        // Update with pointer from IQ module
        NcLibCalcMctfIn                 mctf;

        IcaGeoParameters* pIcaGeoParameters  = reinterpret_cast<IcaGeoParameters*>(pUnpackedField->pIcaGeoParameters);
        GeoLibIcaPassMapping* pIcaMapping    = static_cast<GeoLibIcaPassMapping *>(pInput->pIcaGeolibInputData);
        // SetDefaultsForGeoLibIcaPassMapping(&geoIcaMapping, 4);

        CAMX_LOG_INFO(CamxLogGroupIQMod, "icaVersion %d", pInput->icaVersion);

        // Allocate and asssign
        IQSettingUtils::Memset(&assistIn, 0x0, sizeof(assistIn));
        IQSettingUtils::Memset(&geomIn, 0x0, sizeof(geomIn));
        IQSettingUtils::Memset(&mctf, 0x0, sizeof(mctf));

        // Set Defaults values for Firmware Struct
        SetDefaultsForIcaStruct(pIcaParams, 0);

        // Set default values for ICA REG
        SetDefaultVal_ICA_REG_v30(pRegData, pInput->icaVersion);

        // Set default chromatix data
        SetDefaultVal_ICA_Chromatix(pChromatixData);
        if (TRUE == pInput->dumpICAOut)
        {
            DumpContextParams(pInput, pData);
        }

        // map chromatix parameters or pick from input if provided
        MapChromatixMod2ICAChromatix(pInput, pReserveData, pData,  pChromatixData);

        pCurInWarp                  = static_cast<NcLibWarp*>(pInput->pCurrICAInData);

        geomIn.inputSize.widthPixels            = pInput->pImageDimensions->widthPixels;
        geomIn.inputSize.heightLines            = pInput->pImageDimensions->heightLines;
        geomIn.stabilizationMargins.widthPixels = pInput->pMarginDimensions->widthPixels;
        geomIn.stabilizationMargins.heightLines = pInput->pMarginDimensions->heightLines;
        geomIn.zoomWindow.fullWidth             = pInput->pZoomWindow->fullWidth;
        geomIn.zoomWindow.fullHeight            = pInput->pZoomWindow->fullHeight;
        geomIn.zoomWindow.windowLeft            = pInput->pZoomWindow->windowLeft;
        geomIn.zoomWindow.windowTop             = pInput->pZoomWindow->windowTop;
        geomIn.zoomWindow.windowWidth           = pInput->pZoomWindow->windowWidth;
        geomIn.zoomWindow.windowHeight          = pInput->pZoomWindow->windowHeight;
        geomIn.ifeZoomWindow.fullWidth          = pInput->pIFEZoomWindow->fullWidth;
        geomIn.ifeZoomWindow.fullHeight         = pInput->pIFEZoomWindow->fullHeight;
        geomIn.ifeZoomWindow.windowLeft         = pInput->pIFEZoomWindow->windowLeft;
        geomIn.ifeZoomWindow.windowTop          = pInput->pIFEZoomWindow->windowTop;
        geomIn.ifeZoomWindow.windowWidth        = pInput->pIFEZoomWindow->windowWidth;
        geomIn.ifeZoomWindow.windowHeight       = pInput->pIFEZoomWindow->windowHeight;

        // Need to get proper values for FD/ANR/TF/ alignment domain/ lnr domain
        geomIn.fdConfig                = reinterpret_cast<FD_CONFIG_CONTEXT*>(pInput->pFDData);
        geomIn.ica1UpScaleRatio        = 1.0;
        if (ICA_CONFIG_MCTF_ICA1 == pInput->ICAMode)
        {
            geomIn.alignment               = static_cast<NcLibWarp*>(pInput->pCurrICARefData);
            geomIn.alignmentDomain         = (TRUE == NcLibChipInfo::IsKona()) ? OUTPUT_IMAGE_DOMAIN : INPUT_IMAGE_DOMAIN;
            geomIn.isFdConfigFromPrevFrame = 1;
            geomIn.inputGridsPrevFrame     = pPrevAssist;
        }
        else
        {
            geomIn.alignment               = NULL;
            geomIn.alignmentDomain         = INPUT_IMAGE_DOMAIN;
            geomIn.isFdConfigFromPrevFrame = 0;
            geomIn.inputGridsPrevFrame     = NULL;
        }
        geomIn.inputGrids = pCurrAssist;


        // Pass optical center in format: 15uQ14,  where logic value 0 is start of the image and
        // OPTICAL_CENTER_LOGICAL_MAX is the end of the image.
        FLOAT optCenterXRatio = static_cast<FLOAT>(pInput->pImageDimensions->widthPixels)
                            / static_cast<FLOAT>(pInput->opticalCenterX);
        FLOAT optCenterYRatio = static_cast<FLOAT>(pInput->pImageDimensions->heightLines)
                            / static_cast<FLOAT>(pInput->opticalCenterY);

        geomIn.tf_lnr_opt_center_x  = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterXRatio);
        geomIn.tf_lnr_opt_center_y  = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterYRatio);
        geomIn.anr_lnr_opt_center_x = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterXRatio);
        geomIn.anr_lnr_opt_center_y = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterYRatio);
        geomIn.asf_opt_center_x     = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterXRatio);
        geomIn.asf_opt_center_y     = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterYRatio);
        geomIn.hnr_opt_center_x     = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterXRatio);
        geomIn.hnr_opt_center_y     = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterYRatio);
        geomIn.lenr_opt_center_x    = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterXRatio);
        geomIn.lenr_opt_center_y    = IQSettingUtils::RoundFLOAT(OPTICAL_CENTER_LOGICAL_MAX / optCenterYRatio);

        if (NULL != pGeomOut)
        {
            pGeomOut->tf_lnr_opt_center_x  = geomIn.tf_lnr_opt_center_x;
            pGeomOut->tf_lnr_opt_center_y  = geomIn.tf_lnr_opt_center_y;
            pGeomOut->anr_lnr_opt_center_x = geomIn.anr_lnr_opt_center_x;
            pGeomOut->anr_lnr_opt_center_y = geomIn.anr_lnr_opt_center_y;
            pGeomOut->asf_opt_center_x     = geomIn.asf_opt_center_x;
            pGeomOut->asf_opt_center_y     = geomIn.asf_opt_center_y;
            pGeomOut->hnr_opt_center_x     = geomIn.hnr_opt_center_x;
            pGeomOut->hnr_opt_center_y     = geomIn.hnr_opt_center_y;
            pGeomOut->lenr_opt_center_x    = geomIn.lenr_opt_center_x;
            pGeomOut->lenr_opt_center_y    = geomIn.lenr_opt_center_y;

            if ((NULL != pInput->pFDData) &&
                (0 < pInput->pFDData->numberOfFace))
            {
                IQSettingUtils::Memcpy(pGeomOut->fdConfig, pInput->pFDData, sizeof(FDData));
            }
        }

        if ((NULL != pCurRefWarp) && (static_cast<UINT>(CamxIPEICAPath::REFERENCE) == pInput->IPEPath))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "REF: %d perspective %d, perspective c %d, r %d, w %d , h %d",
                             pInput->frameNum,
                             pCurRefWarp->matrices.enable,
                             pCurRefWarp->matrices.numColumns,
                             pCurRefWarp->matrices.numRows,
                             pCurRefWarp->matrices.transformDefinedOn.widthPixels,
                             pCurRefWarp->matrices.transformDefinedOn.heightLines);

            icaIsGridEnabledByFlow = pCurRefWarp->grid.enable;
            icaPerspectiveEnabled  = pCurRefWarp->matrices.enable;
            if (TRUE == result)
            {
                // calculate MCTF matrix of current frame
                ret = NcLibWarpConvertToVirtualDomain(pCurRefWarp, pCurRefWarp);
                result = IsSuccess(ret, "NcLibWarpConvertToVirtualDomainICA2");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibWarpConvertToVirtualDomain ref %d, path %d",
                                 result, pInput->IPEPath);
            }

            if (TRUE == result)
            {
                /* calculate MCTF matrix */
                warpOut.grid.grid = pGrid;
                warpOut.grid.gridExtrapolate   = gridExtrapolate;
                warpOut.matrices.perspMatrices = perspectiveMatrices;
                // reset before calling mctf adjustment calc for out
                warpOut.matrices.enable        = FALSE;
                warpOut.grid.enable            = FALSE;
                // geomIn.alignment          = pCurRefWarp; // check
                // set correct value

                mctf.alignmentDomain      = (TRUE == NcLibChipInfo::IsKona()) ? OUTPUT_IMAGE_DOMAIN : INPUT_IMAGE_DOMAIN;
                mctf.inputSize            = &geomIn.inputSize;
                mctf.stabilizationMargins = &geomIn.stabilizationMargins;
                mctf.alignment            = pCurRefWarp;
                mctf.inputWarp            = pCurInWarp;
                mctf.inputGridsPrevFrame  = pPrevAssist;
                mctf.inputGrids           = NULL;

                if (TRUE == CheckMctfTransformCondition(
                    static_cast<VOID*>(&mctf), static_cast<VOID*>(&warpOut),
                    (ICA_CONFIG_MCTF_ICA1 == pInput->ICAMode) ?  TRUE  : FALSE,
                    pInput->byPassAlignmentMatrixAdjustement))
                {
                    pRefWarp = &warpOut;
                    ret = NcLibCalcMctfTransform(&mctf, OUTPUT_IMAGE_DOMAIN, &warpOut);
                    result = IsSuccess(ret, "NcLibCalcMctfTransform");
                    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibCalcMctfTransform %d, path %d", result, pInput->IPEPath);
                    if (TRUE == pInput->dumpICAOut)
                    {
                        DumpMCTFInputOutput(pInput, &mctf, &warpOut);
                    }
                }
            }

            if (TRUE == result)
            {
                // Validate parameters input to NCLib as debug settings/
                // Currently part of NCLib
                ret = ICA_ProcessNcLib(pChromatixData,
                                       static_cast<uint8_t>(icaIsGridEnabledByFlow),
                                       pRegData,
                                       static_cast<IcaParameters*>(pUnpackedField->pIcaParameter));
                result = IsSuccess(ret, "ICA_ProcessNcLib_ICA2");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ICA_ProcessNcLib : result %d", result);
            }

            if (TRUE == result)
            {
                ret = ICA_ProcessNonChromatixParams(&pRefWarp->grid,
                                                    &pRefWarp->matrices,
                                                    (NULL != pIcaMapping) ? pIcaMapping : NULL,
                                                    static_cast<uint8_t>(icaIsGridEnabledByFlow),
                                                    1,
                                                    pInput->icaVersion,
                                                    pRegData,
                                                    static_cast<IcaParameters*>(pUnpackedField->pIcaParameter),
                                                    (NULL != pIcaGeoParameters) ? &pIcaGeoParameters->ica2[0] : NULL);
                result = IsSuccess(ret, "ICA_ProcessNonChromatixParams_ICA2");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ICA_ProcessNonChromatixParams : result %d", result);
            }
        }
        else if ((NULL != pCurInWarp) && (static_cast<UINT>(CamxIPEICAPath::INPUT) == pInput->IPEPath))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "grid enabled %d", pCurInWarp->grid.enable);
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "INPUT: %d,  perspective %d, perspective c %d, r %d, w %d , h %d",
                             pInput->frameNum,
                             pCurInWarp->matrices.enable,
                             pCurInWarp->matrices.numColumns,
                             pCurInWarp->matrices.numRows,
                             pCurInWarp->matrices.transformDefinedOn.widthPixels,
                             pCurInWarp->matrices.transformDefinedOn.heightLines);

            icaIsGridEnabledByFlow = pCurInWarp->grid.enable;
            icaPerspectiveEnabled  = pCurInWarp->matrices.enable;

            if  ((TRUE == icaIsGridEnabledByFlow) ||
                 (TRUE == icaPerspectiveEnabled))
            {
                // Convert input parameters to virtual domain
                ret    = NcLibWarpConvertToVirtualDomain(pCurInWarp, pCurInWarp);
                result = IsSuccess(ret, "NcLibWarpConvertToVirtualDomainICA1");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibWarpConvertToVirtualDomain %d", result);

                if ((TRUE == result)                        &&
                    (NULL != pCurInWarp)                    &&
                    (TRUE == pCurInWarp->matrices.enable)   &&
                    (ICA_CONFIG_MFNR_TEMPORAL_ANCHOR_AGGREGATE == pInput->ICAMode))
                {
                    ret = NcLibCalcMfnrTransform(pCurInWarp, pCurInWarp, pCurInWarp);
                    result = IsSuccess(ret, "NcLibCalcMfnrTransform");
                    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " NcLibCalcMfnrTransform %d, path %d",
                        result, pInput->IPEPath);

                    if (TRUE != result)
                    {
                        DumpMfnrTransform(pCurInWarp);
                        pCurInWarp->matrices.enable = FALSE;
                        result = TRUE;
                    }
                }

                if ((TRUE == result)                       &&
                    (NULL != pCurrAssist)                  &&
                    (NULL != pCurrAssist->assistGrid)      &&
                    ((TRUE == pCurInWarp->matrices.enable) ||
                     (TRUE == pCurInWarp->grid.enable)))
                {
                    if ((FALSE == icaIsGridEnabledByFlow) &&
                        ((pCurInWarp->matrices.numRows * pCurInWarp->matrices.numColumns) < 2))
                    {
                        pCurrAssist->assistGrid->enable = false;
                    }
                    else
                    {
                        // Build assist grid of current frame and calculate geometric parameters
                        assistIn.in         = pCurInWarp;
                        assistIn.numColumns = GridAssistColumns;
                        assistIn.numRows    = GridAssistRows;
                        ret = NcLibWarpBuildAssistGrid(&assistIn, pCurrAssist);
                        result = IsSuccess(ret, "NcLibWarpBuildAssistGrid");
                        if (TRUE != result)
                        {
                            DumpAssitGridInputs(&assistIn);
                            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibWarpBuildAssistGrid %d, path %d",
                                                result, pInput->IPEPath);
                        }
                    }
                }

                if ((TRUE == result) &&
                    (TRUE  == CheckWarpGeometryCondition(
                    pPrevInWarp,
                    pCurInWarp,
                    pCurrAssist,
                    &geomIn)))
                {
                    pCurrAssist->inputWarp = pCurInWarp;
                    ret = NcLibWarpGeometries(&geomIn, pGeomOut);
                    result = IsSuccess(ret, "NcLibWarpGeometries");
                    if (TRUE != result)
                    {
                        DumpGeometryInputs(&geomIn);
                        CAMX_LOG_ERROR(CamxLogGroupIQMod, " NcLibWarpGeometries %d, path %d",
                                        result, pInput->IPEPath);
                    }
                }
            }

            if (TRUE == result)
            {
                // Validate parameters input to NCLib as debug settings/
                // Currently part of NCLib
                ret = ICA_ProcessNcLib(pChromatixData,
                                       static_cast<uint8_t>(icaIsGridEnabledByFlow),
                                       pRegData,
                                       static_cast<IcaParameters*>(pUnpackedField->pIcaParameter));
                result = IsSuccess(ret, "ICA_ProcessNcLib_ICA1");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ICA_ProcessNcLib : result %d", result);
            }

            if (TRUE == result)
            {
                ret = ICA_ProcessNonChromatixParams(&pCurInWarp->grid,
                                                    &pCurInWarp->matrices,
                                                    (NULL != pIcaMapping) ? pIcaMapping : NULL,
                                                    static_cast<uint8_t>(icaIsGridEnabledByFlow),
                                                    1,
                                                    pInput->icaVersion,
                                                    pRegData,
                                                    static_cast<IcaParameters*>(pUnpackedField->pIcaParameter),
                                                    (NULL != pIcaGeoParameters) ? &pIcaGeoParameters->ica1[0] : NULL);
                result = IsSuccess(ret, "ICA_ProcessNonChromatixParams_ICA1");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ICA_ProcessNonChromatixParams : result %d", result);
            }

            pUnpackedField->pCurrICAInData      = pCurInWarp;
            pUnpackedField->pPrevICAInData      = pPrevInWarp;
            pUnpackedField->pCurrWarpAssistData = pCurrAssist;
            pUnpackedField->pPrevWarpAssistData = pPrevAssist;
            pUnpackedField->pWarpGeometryData   = pGeomOut;
        }
        else if ((NULL != pCurInWarp) && (static_cast<UINT>(CamxIPEICAPath::CVPICA) == pInput->IPEPath))
        {

            CAMX_LOG_INFO(CamxLogGroupIQMod, "CVP ICA input dims %dX%d",
                          pIcaMapping->icaMappingFull.inputImageSize.widthPixels,
                          pIcaMapping->icaMappingFull.inputImageSize.heightLines);

            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CVPICA: frameNum %d, grid enabled %d",
                             pInput->frameNum, pCurInWarp->grid.enable);

            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CVPICA: frameNum %d,  perspective %d, perspective c %d, r %d, w %d , h %d",
                             pInput->frameNum,
                             pCurInWarp->matrices.enable,
                             pCurInWarp->matrices.numColumns,
                             pCurInWarp->matrices.numRows,
                             pCurInWarp->matrices.transformDefinedOn.widthPixels,
                             pCurInWarp->matrices.transformDefinedOn.heightLines);

            icaIsGridEnabledByFlow = pCurInWarp->grid.enable;
            icaPerspectiveEnabled  = pCurInWarp->matrices.enable;

            if ((TRUE == result)                  &&
                ((TRUE == icaIsGridEnabledByFlow) ||
                 (TRUE == icaPerspectiveEnabled)))
            {
                // Convert input parameters to virtual domain
                ret    = NcLibWarpConvertToVirtualDomain(pCurInWarp, pCurInWarp);
                result = IsSuccess(ret, "NcLibWarpConvertToVirtualDomainCVPICA");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibWarpConvertToVirtualDomain %d", result);
            }

            if (TRUE == result)
            {
                // Validate parameters input to NCLib as debug settings/
                // Currently part of NCLib
                ret = ICA_ProcessNcLib(pChromatixData,
                                       static_cast<uint8_t>(icaIsGridEnabledByFlow),
                                       pRegData,
                                       static_cast<IcaParameters*>(pUnpackedField->pIcaParameter));
                result = IsSuccess(ret, "ICA_ProcessNcLib_CVPICA");
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ICA_ProcessNcLib : result %d", result);
            }

            if (TRUE == result)
            {
                if ((NULL != pInput->pCVPICAIntermediateBuffer) &&
                    (NULL != pCVPICAFrameCfgOut))
                {
                    ret = NcLibProcessCvpIca(&pCurInWarp->grid,
                                             &pCurInWarp->matrices,
                                             &pIcaMapping->icaMappingFull,
                                             0,
                                             pInput->pCVPICAIntermediateBuffer,
                                             pCVPICAFrameCfgOut);
                    result = IsSuccess(ret, "NcLibProcessCvpIca");
                    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NcLibProcessCvpIca : result %d", result);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid cvp ica intermediate buffer: %pK, frame config: %pK ptr",
                                   pInput->pCVPICAIntermediateBuffer,
                                   pCVPICAFrameCfgOut);
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid IPE path or pCurRefWarp %p, pCurInWarp %p",
                           pCurRefWarp, pCurInWarp);
            result = FALSE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::CheckWarpGeometryCondition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::CheckWarpGeometryCondition(
    NcLibWarp* pPrevInWarp,
    NcLibWarp* pCurInWarp,
    NcLibWarpBuildAssistGridOut* pCurrAssist,
    NcLibWarpGeomIn*             pGeomIn)

{
    BOOL needWarpGeometry = FALSE;

    if ((NULL != pCurrAssist) &&
        ((TRUE == pCurInWarp->matrices.enable) ||
        (TRUE == pCurInWarp->grid.enable)))
    {
        if (TRUE == pGeomIn->isFdConfigFromPrevFrame)
        {
            if (INPUT_IMAGE_DOMAIN == pGeomIn->alignmentDomain)
            {
                needWarpGeometry = TRUE;
            }
            else if ((INTERMEDIATE_IMAGE_DOMAIN == pGeomIn->alignmentDomain) ||
                (OUTPUT_IMAGE_DOMAIN == pGeomIn->alignmentDomain))
            {
                if ((TRUE == pPrevInWarp->matrices.enable) ||
                    (TRUE == pPrevInWarp->grid.enable))
                {
                    needWarpGeometry = TRUE;
                }
            }
        }
        else
        {
            needWarpGeometry = TRUE;
        }
    }
    return needWarpGeometry;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::CheckMctfTransformCondition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::CheckMctfTransformCondition(
    VOID* pMCTF,
    VOID* pWarpOut,
    BOOL  mctfEis,
    BOOL  byPassAlignmentMatrixAdjustement)
{
    NcLibCalcMctfIn* pIn  = static_cast<NcLibCalcMctfIn*>(pMCTF);
    NcLibWarp*       pOut = static_cast<NcLibWarp*>(pWarpOut);

    if ((NULL  == pIn->alignment)                            ||
        (NULL  == pIn->inputGridsPrevFrame)                  ||
        (NULL  == pIn->inputGridsPrevFrame->assistGrid)      ||
        (NULL  == pIn->inputGridsPrevFrame->inputWarp)       ||
        (NULL  == pIn->inputWarp)                            ||
        (NULL  == pIn->inputSize)                            ||
        (NULL  == pIn->stabilizationMargins)                 ||
        (NULL  == pOut->matrices.perspMatrices)              ||
        (FALSE == pIn->alignment->matrices.enable)           ||
        (FALSE == IsvalidMCTFICAtransformcondition(pIn))     ||
        ((FALSE == mctfEis) ? TRUE : (TRUE == byPassAlignmentMatrixAdjustement)))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "alignment %p, inputGridsPrevFrame %p, inputWarp %p,"
                         "inputSize %p, stabilizationMargins %p, perspMatrices %p, mctfEis %d,"
                         "byPassAlignmentMatrixAdjustement %d, IsvalidMCTFICAtransformcondition %d",
                         pIn->alignment, pIn->inputGridsPrevFrame, pIn->inputWarp, pIn->inputSize,
                         pIn->stabilizationMargins, pOut->matrices.perspMatrices, mctfEis,
                         byPassAlignmentMatrixAdjustement, IsvalidMCTFICAtransformcondition(pIn));

        if (NULL != pIn->inputGridsPrevFrame)
        {
            CAMX_LOG_INFO(CamxLogGroupIQMod, " PassistG %p, PinputWarp %p,",
                          pIn->inputGridsPrevFrame->assistGrid,
                          pIn->inputGridsPrevFrame->inputWarp);

        }
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICASetting::GetInitializationData(
    struct ICANcLibOutputData* pData)
{
    pData->ICAChromatixSize = (sizeof(ICA_Chromatix));
    pData->ICARegSize       = (sizeof(ICA_REG_v30));
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpMatrices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpMatrices(
    FILE* pFile,
    const NcLibWarpMatrices*    pMatrices)
{
    if (TRUE == pMatrices->enable)
    {
        CamX::OsUtils::FPrintF(pFile, "warp enable = %d\n", pMatrices->enable);
        CamX::OsUtils::FPrintF(pFile, "center      = %d\n", pMatrices->centerType);
        CamX::OsUtils::FPrintF(pFile, "confidence  = %d\n", pMatrices->confidence);
        CamX::OsUtils::FPrintF(pFile, "numColumns  = %d\n", pMatrices->numColumns);
        CamX::OsUtils::FPrintF(pFile, "numRows     = %d\n", pMatrices->numRows);
        CamX::OsUtils::FPrintF(pFile, "transformdefinedwidth = %d\n", pMatrices->transformDefinedOn.widthPixels);
        CamX::OsUtils::FPrintF(pFile, "transformdefinedwidth = %d\n", pMatrices->transformDefinedOn.heightLines);
        for (UINT i = 0; i < ICAMaxPerspectiveTransform; i++)
        {
            for (UINT j = 0; j < ICAParametersPerPerspectiveTransform; j++)
            {
                CamX::OsUtils::FPrintF(pFile, " matrices [%d] [%d] :  %f\n",
                    i, j, pMatrices->perspMatrices[i].T[j]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpGrid(
    FILE* pFile,
    const NcLibWarpGrid*  pGrid)
{
    if (TRUE == pGrid->enable)
    {
        CamX::OsUtils::FPrintF(pFile, "enable = %d\n", pGrid->enable);
        CamX::OsUtils::FPrintF(pFile, "num columns = %d\n", pGrid->numColumns);
        CamX::OsUtils::FPrintF(pFile, "num rows = %d\n", pGrid->numRows);
        CamX::OsUtils::FPrintF(pFile, "tranform width = %d\n", pGrid->transformDefinedOn.widthPixels);
        CamX::OsUtils::FPrintF(pFile, "tranform height = %d\n", pGrid->transformDefinedOn.heightLines);
        CamX::OsUtils::FPrintF(pFile, "extrapolateType = %d\n", pGrid->extrapolateType);
        for (UINT32 i = 0; i < (pGrid->numRows * pGrid->numColumns); i++)
        {
            CamX::OsUtils::FPrintF(pFile, "i %d, gridx %f\n", i, pGrid->grid[i].x);
            CamX::OsUtils::FPrintF(pFile, "i %d  gridy %f\n", i, pGrid->grid[i].y);
        }
        if (EXTRAPOLATION_TYPE_FOUR_CORNERS == pGrid->extrapolateType)
        {
            for (UINT32 i = 0; i < GridExtarpolateCornerSize; i++)
            {
                CamX::OsUtils::FPrintF(pFile, "corners i %d, gridx %f\n", i, pGrid->gridExtrapolate[i].x);
                CamX::OsUtils::FPrintF(pFile, "corners i %d, gridy %f\n", i, pGrid->gridExtrapolate[i].y);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICASetting::DumpMCTFInputOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICASetting::DumpMCTFInputOutput(
    const ICAInputData*  pInput,
    const NcLibCalcMctfIn* pIn,
    NcLibWarp* pOut)
{
    CHAR              dumpFilename[256];
    FILE*             pFile = NULL;
    const NcLibWarp*  pWarp = NULL;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
        "%s/icamctfinout_Out_%d_path_%d_instance_%d.txt",
        CamX::ConfigFileDirectory, pInput->frameNum, pInput->IPEPath, pInput->instanceID);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FPrintF(pFile, "-----------Input mctf parameters---------------------\n");
        CamX::OsUtils::FPrintF(pFile, "inputSize width  =  %d\n", pIn->inputSize->widthPixels);
        CamX::OsUtils::FPrintF(pFile, "inputSize height =  %d\n", pIn->inputSize->heightLines);
        CamX::OsUtils::FPrintF(pFile, "stabilizationMargins width  =  %d\n", pIn->stabilizationMargins->widthPixels);
        CamX::OsUtils::FPrintF(pFile, "stabilizationMargins height =  %d\n", pIn->stabilizationMargins->heightLines);
        CamX::OsUtils::FPrintF(pFile, "alignmentDomain  =  %d\n", pIn->alignmentDomain);
        pWarp = pIn->alignment;
        if (NULL != pWarp)
        {
            CamX::OsUtils::FPrintF(pFile, "-----------Alignment++---------------------\n");
            CamX::OsUtils::FPrintF(pFile, "-----------Alignment matrice---------------------\n");
            DumpMatrices(pFile, &pWarp->matrices);
            CamX::OsUtils::FPrintF(pFile, "-----------Alignment grid---------------------\n");
            DumpGrid(pFile, &pWarp->grid);
            CamX::OsUtils::FPrintF(pFile, "alignment direction %d\n", pWarp->direction);
            CamX::OsUtils::FPrintF(pFile, "-----------Alignment done---------------------\n");
        }
        pWarp = pIn->inputWarp;

        if (NULL != pWarp)
        {
            CamX::OsUtils::FPrintF(pFile, "-----------Input warp++---------------------\n");
            CamX::OsUtils::FPrintF(pFile, "-----------Input warp matrice---------------------\n");
            DumpMatrices(pFile, &pWarp->matrices);
            CamX::OsUtils::FPrintF(pFile, "-----------Input warp grid---------------------\n");
            DumpGrid(pFile, &pWarp->grid);
            CamX::OsUtils::FPrintF(pFile, "Input warp direction %d\n", pWarp->direction);
            CamX::OsUtils::FPrintF(pFile, "-----------Input warp done---------------------\n");
        }

        if (NULL != pIn->inputGridsPrevFrame)
        {
            if (NULL != pIn->inputGridsPrevFrame->assistGrid)
            {
                CamX::OsUtils::FPrintF(pFile, "-----------Previous Assist grid++---------------------\n");
                DumpGrid(pFile, pIn->inputGridsPrevFrame->assistGrid);
            }
            if (NULL != pIn->inputGridsPrevFrame->inputWarp)
            {
                CamX::OsUtils::FPrintF(pFile, "-----------input warp in GridsPrevFrame++---------------------\n");
                DumpMatrices(pFile, &pIn->inputGridsPrevFrame->inputWarp->matrices);
                CamX::OsUtils::FPrintF(pFile, "-----------Input warp in GridsPrevFrame grid---------------------\n");
                DumpGrid(pFile, &pIn->inputGridsPrevFrame->inputWarp->grid);
                CamX::OsUtils::FPrintF(pFile, "Input warp in GridsPrevFrame direction %d\n",
                    pIn->inputGridsPrevFrame->inputWarp->direction);
            }
            CamX::OsUtils::FPrintF(pFile, "-----------Previous Assist grid done---------------------\n");
        }
        CamX::OsUtils::FPrintF(pFile, "-----------Input mctf parameters done---------------------\n");

        CamX::OsUtils::FPrintF(pFile, "-----------Output mctf parameters---------------------\n");
        pWarp = pOut;

        if (NULL != pWarp)
        {
            CamX::OsUtils::FPrintF(pFile, "-----------output warp++---------------------\n");
            CamX::OsUtils::FPrintF(pFile, "-----------output warp matrice---------------------\n");
            DumpMatrices(pFile, &pWarp->matrices);
            CamX::OsUtils::FPrintF(pFile, "-----------output warp grid---------------------\n");
            DumpGrid(pFile, &pWarp->grid);
            CamX::OsUtils::FPrintF(pFile, "output warp direction %d\n", pWarp->direction);
            CamX::OsUtils::FPrintF(pFile, "-----------output warp done---------------------\n");
        }
        CamX::OsUtils::FPrintF(pFile, "-----------Output mctf parameters done---------------------\n");
        CamX::OsUtils::FPrintF(pFile, "--------------------------------\n");
        CamX::OsUtils::FClose(pFile);
    }
}
