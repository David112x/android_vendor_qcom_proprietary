// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  sce11setting.cpp
/// @brief SCE11 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "sce11setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SCE11Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SCE11Setting::CalculateHWSetting(
    const SCE11InputData*                                 pInput,
    sce_1_1_0::sce11_rgn_dataType*                        pData,
    sce_1_1_0::chromatix_sce11_reserveType*               pReserveType,
    sce_1_1_0::chromatix_sce11Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);

    BOOL   result    = TRUE;
    FLOAT  tol       = -1e-10f;
    FLOAT  small_tol = 1e-5f;
    DOUBLE dOrig[SCE11MaxTriangleRow][SCE11MaxTriangleCol];
    DOUBLE dDest[SCE11MaxTriangleRow][SCE11MaxTriangleCol];
    DOUBLE dAffine[SCE11MaxAffineTriangeSet][SCE11MaxTriangleRow][SCE11MaxTriangleCol];
    DOUBLE dDet;
    DOUBLE dInv[SCE11MaxTriangleRow][SCE11MaxTriangleCol];
    BOOL   resultTriangle[10];
    UINT   i;
    UINT   j;
    UINT   t;
    UINT   k;
    DOUBLE dMax;
    DOUBLE q;
    UINT16 q1;
    UINT16 q2;

    if ((NULL != pData) && (NULL != pOutput))
    {
        SCE11UnpackedField* pUnpackedField = static_cast<SCE11UnpackedField*>(pOutput);

        pUnpackedField->enable = static_cast<UINT8>(pModuleEnable->sce_enable);

        // Compute the 5 affine transforms from the original triangle to the destination triangle
        sce_1_1_0::cr_cb_triangle* triangles_original[5] =
        {
            &pData->ori_triangle.traingle1,
            &pData->ori_triangle.traingle2,
            &pData->ori_triangle.traingle3,
            &pData->ori_triangle.traingle4,
            &pData->ori_triangle.traingle5,
        };

        sce_1_1_0::cr_cb_triangle* triangles_target[5] =
        {
            &pData->target_triangle.traingle1,
            &pData->target_triangle.traingle2,
            &pData->target_triangle.traingle3,
            &pData->target_triangle.traingle4,
            &pData->target_triangle.traingle5,
        };

        for (t = 0; t < 5; t++)
        {
            // Reset affine transform buffer
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    dAffine[t][i][j] = 0;
                }
            }

            // Triangle vertices
            dOrig[0][0] = triangles_original[t]->point1[0];
            dOrig[1][0] = triangles_original[t]->point1[1];
            dOrig[2][0] = 1.0;
            dOrig[0][1] = triangles_original[t]->point3[0];
            dOrig[1][1] = triangles_original[t]->point3[1];
            dOrig[2][1] = 1.0;
            dOrig[0][2] = triangles_original[t]->point2[0];
            dOrig[1][2] = triangles_original[t]->point2[1];
            dOrig[2][2] = 1.0;
            dDest[0][0] = triangles_target[t]->point1[0];
            dDest[1][0] = triangles_target[t]->point1[1];
            dDest[2][0] = 1.0;
            dDest[0][1] = triangles_target[t]->point3[0];
            dDest[1][1] = triangles_target[t]->point3[1];
            dDest[2][1] = 1.0;
            dDest[0][2] = triangles_target[t]->point2[0];
            dDest[1][2] = triangles_target[t]->point2[1];
            dDest[2][2] = 1.0;

            // Matrix inversion of the original
            dDet = dOrig[0][0] * dOrig[1][1] * dOrig[2][2] +
                dOrig[0][1] * dOrig[1][2] * dOrig[2][0] +
                dOrig[1][0] * dOrig[2][1] * dOrig[0][2] -
                dOrig[0][2] * dOrig[1][1] * dOrig[2][0] -
                dOrig[0][1] * dOrig[1][0] * dOrig[2][2] -
                dOrig[1][2] * dOrig[2][1] * dOrig[0][0];

            // ~Singularity, skip to the next transform
            if (IQSettingUtils::AbsoluteDOUBLE(dDet) < 1e-10)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("TriggerSCE11Module: Triangle #%d is (nearly) singular.  Skipped.\n");
                continue;
            }

            dInv[0][0] = (dOrig[1][1] * dOrig[2][2] - dOrig[1][2] * dOrig[2][1]) / dDet;
            dInv[0][1] = -(dOrig[0][1] * dOrig[2][2] - dOrig[0][2] * dOrig[2][1]) / dDet;
            dInv[0][2] = (dOrig[0][1] * dOrig[1][2] - dOrig[0][2] * dOrig[1][1]) / dDet;
            dInv[1][0] = -(dOrig[1][0] * dOrig[2][2] - dOrig[1][2] * dOrig[2][0]) / dDet;
            dInv[1][1] = (dOrig[0][0] * dOrig[2][2] - dOrig[0][2] * dOrig[2][0]) / dDet;
            dInv[1][2] = -(dOrig[0][0] * dOrig[1][2] - dOrig[0][2] * dOrig[1][0]) / dDet;
            dInv[2][0] = (dOrig[1][0] * dOrig[2][1] - dOrig[1][1] * dOrig[2][0]) / dDet;
            dInv[2][1] = -(dOrig[0][0] * dOrig[2][1] - dOrig[0][1] * dOrig[2][0]) / dDet;
            dInv[2][2] = (dOrig[0][0] * dOrig[1][1] - dOrig[0][1] * dOrig[1][0]) / dDet;

            // The affine transform of triangle t
            for (i = 0; i < SCE11MaxTriangleRow; i++)
            {
                for (j = 0; j < SCE11MaxTriangleCol; j++)
                {
                    dAffine[t][i][j] = 0;

                    for (k = 0; k < SCE11MaxTriangleCol; k++)
                    {
                        dAffine[t][i][j] += dDest[i][k] * dInv[k][j];
                    }
                }
            }
        }

        // The affine transform outside the 5 triangles
        dAffine[5][0][0] = pReserveType->a;
        dAffine[5][0][1] = pReserveType->b;
        dAffine[5][0][2] = pReserveType->c;
        dAffine[5][1][0] = pReserveType->d;
        dAffine[5][1][1] = pReserveType->e;
        dAffine[5][1][2] = pReserveType->f;
        dAffine[5][2][0] = 0;
        dAffine[5][2][1] = 0;
        dAffine[5][2][2] = 1;

        for (i = 0; i < 5; i++)
        {

            resultTriangle[2 * i]     = RearrangeTriangle(triangles_original[i]);
            resultTriangle[2 * i + 1] = RearrangeTriangle(triangles_target[i]);

            if (!(resultTriangle[2 * i] || resultTriangle[2 * i + 1]))
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("TriggerSCE11Module: can not rearrange the triangle $d\n");
            }
        }

        // Setup the original triangle vertices
        pUnpackedField->t0_Cr1 = static_cast<UINT32>(pData->ori_triangle.traingle1.point1[0]);
        pUnpackedField->t0_Cb1 = static_cast<UINT32>(pData->ori_triangle.traingle1.point1[1]);
        pUnpackedField->t0_Cr2 = static_cast<UINT32>(pData->ori_triangle.traingle1.point2[0]);
        pUnpackedField->t0_Cb2 = static_cast<UINT32>(pData->ori_triangle.traingle1.point2[1]);
        pUnpackedField->t0_Cr3 = static_cast<UINT32>(pData->ori_triangle.traingle1.point3[0]);
        pUnpackedField->t0_Cb3 = static_cast<UINT32>(pData->ori_triangle.traingle1.point3[1]);
        pUnpackedField->t1_Cr1 = static_cast<UINT32>(pData->ori_triangle.traingle2.point1[0]);
        pUnpackedField->t1_Cb1 = static_cast<UINT32>(pData->ori_triangle.traingle2.point1[1]);
        pUnpackedField->t1_Cr2 = static_cast<UINT32>(pData->ori_triangle.traingle2.point2[0]);
        pUnpackedField->t1_Cb2 = static_cast<UINT32>(pData->ori_triangle.traingle2.point2[1]);
        pUnpackedField->t1_Cr3 = static_cast<UINT32>(pData->ori_triangle.traingle2.point3[0]);
        pUnpackedField->t1_Cb3 = static_cast<UINT32>(pData->ori_triangle.traingle2.point3[1]);
        pUnpackedField->t2_Cr1 = static_cast<UINT32>(pData->ori_triangle.traingle3.point1[0]);
        pUnpackedField->t2_Cb1 = static_cast<UINT32>(pData->ori_triangle.traingle3.point1[1]);
        pUnpackedField->t2_Cr2 = static_cast<UINT32>(pData->ori_triangle.traingle3.point2[0]);
        pUnpackedField->t2_Cb2 = static_cast<UINT32>(pData->ori_triangle.traingle3.point2[1]);
        pUnpackedField->t2_Cr3 = static_cast<UINT32>(pData->ori_triangle.traingle3.point3[0]);
        pUnpackedField->t2_Cb3 = static_cast<UINT32>(pData->ori_triangle.traingle3.point3[1]);
        pUnpackedField->t3_Cr1 = static_cast<UINT32>(pData->ori_triangle.traingle4.point1[0]);
        pUnpackedField->t3_Cb1 = static_cast<UINT32>(pData->ori_triangle.traingle4.point1[1]);
        pUnpackedField->t3_Cr2 = static_cast<UINT32>(pData->ori_triangle.traingle4.point2[0]);
        pUnpackedField->t3_Cb2 = static_cast<UINT32>(pData->ori_triangle.traingle4.point2[1]);
        pUnpackedField->t3_Cr3 = static_cast<UINT32>(pData->ori_triangle.traingle4.point3[0]);
        pUnpackedField->t3_Cb3 = static_cast<UINT32>(pData->ori_triangle.traingle4.point3[1]);
        pUnpackedField->t4_Cr1 = static_cast<UINT32>(pData->ori_triangle.traingle5.point1[0]);
        pUnpackedField->t4_Cb1 = static_cast<UINT32>(pData->ori_triangle.traingle5.point1[1]);
        pUnpackedField->t4_Cr2 = static_cast<UINT32>(pData->ori_triangle.traingle5.point2[0]);
        pUnpackedField->t4_Cb2 = static_cast<UINT32>(pData->ori_triangle.traingle5.point2[1]);
        pUnpackedField->t4_Cr3 = static_cast<UINT32>(pData->ori_triangle.traingle5.point3[0]);
        pUnpackedField->t4_Cb3 = static_cast<UINT32>(pData->ori_triangle.traingle5.point3[1]);

        // Setup the 6 affine transforms
        // T0
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[0][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[0][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[0][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[0][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol)
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[0][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[0][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax))) <= small_tol)
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }


        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t0_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t0_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t0_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t0_Q1 + pUnpackedField->t0_Q2));

        pUnpackedField->t0_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t0_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t0_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t0_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        q = static_cast<DOUBLE>(1 << pUnpackedField->t0_Q2);

        pUnpackedField->t0_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][0][2]));
        pUnpackedField->t0_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[0][1][2]));

        // T1
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[1][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[1][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[1][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[1][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol)
        {
            dMax = static_cast<DOUBLE>((IQSettingUtils::RoundDOUBLE(dMax)));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }


        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[1][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[1][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }


        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t1_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t1_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t1_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t1_Q1 + pUnpackedField->t1_Q2));

        pUnpackedField->t1_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t1_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t1_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t1_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);

        q = static_cast<DOUBLE>(1 << pUnpackedField->t1_Q2);

        pUnpackedField->t1_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][0][2]));
        pUnpackedField->t1_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[1][1][2]));

        // T2
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[2][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[2][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[2][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[2][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[2][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[2][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t2_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t2_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t2_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t2_Q1 + pUnpackedField->t2_Q2));

        pUnpackedField->t2_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t2_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t2_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t2_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);

        q = static_cast<DOUBLE>(1 << pUnpackedField->t2_Q2);

        pUnpackedField->t2_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][0][2]));
        pUnpackedField->t2_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[2][1][2]));

        // T3
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[3][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[3][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[3][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[3][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[3][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[3][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t3_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t3_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t3_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t3_Q1 + pUnpackedField->t3_Q2));

        pUnpackedField->t3_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t3_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t3_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t3_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);

        q = static_cast<DOUBLE>(1 << pUnpackedField->t3_Q2);

        pUnpackedField->t3_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][0][2]));
        pUnpackedField->t3_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[3][1][2]));

        // T4
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[4][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[4][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[4][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[4][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }


        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[4][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[4][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t4_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t4_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t4_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t4_Q1 + pUnpackedField->t4_Q2));

        pUnpackedField->t4_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t4_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t4_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t4_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);

        q = static_cast<DOUBLE>(1 << pUnpackedField->t4_Q2);

        pUnpackedField->t4_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][0][2]));
        pUnpackedField->t4_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[4][1][2]));

        // T5
        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[5][0][0]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[5][0][1]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[5][1][0]));
        dMax = IQSettingUtils::MaxDouble(dMax, IQSettingUtils::AbsoluteDOUBLE(dAffine[5][1][1]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q1 = IQSettingUtils::ClampUINT16(11 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, 12);  // 12s

        dMax = IQSettingUtils::MaxDouble(IQSettingUtils::AbsoluteDOUBLE(dAffine[5][0][2]),
                                         IQSettingUtils::AbsoluteDOUBLE(dAffine[5][1][2]));

        if (IQSettingUtils::AbsoluteDOUBLE(dMax - IQSettingUtils::RoundDOUBLE(dMax)) <= small_tol) // to limit the precision
        {
            dMax = static_cast<DOUBLE>(IQSettingUtils::RoundDOUBLE(dMax));
        }

        if (dMax > 0.00000001)
        {
            q = log(dMax) / log(2.0);
        }
        else
        {
            q = 0;
        }

        if ((IQSettingUtils::AbsoluteDOUBLE(q - static_cast<INT>(q)) < small_tol) && (q >= tol))
        {
            q += 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        q2 = IQSettingUtils::ClampUINT16(16 - static_cast<UINT16>(IQSettingUtils::CeilingDOUBLE(q)), 0, q1);  // 17s

        pUnpackedField->t5_Q2 = IQSettingUtils::ClampUINT16(q2, 0, 12);
        pUnpackedField->t5_Q1 = IQSettingUtils::ClampUINT16(q1 - pUnpackedField->t5_Q2, 0, 12);

        q = static_cast<DOUBLE>(1 << (pUnpackedField->t5_Q1 + pUnpackedField->t5_Q2));

        pUnpackedField->t5_A = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][0][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t5_B = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][0][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t5_D = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][1][0])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);
        pUnpackedField->t5_E = IQSettingUtils::ClampINT32(static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][1][1])),
                                                           SCE11_LEVEL_MIN,
                                                           SCE11_LEVEL_MAX);

        q = static_cast<DOUBLE>(1 << pUnpackedField->t5_Q2);

        pUnpackedField->t5_C = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][0][2]));
        pUnpackedField->t5_F = static_cast<INT32>(IQSettingUtils::RoundDOUBLE(q * dAffine[5][1][2]));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SCE11Setting::RearrangeTriangle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SCE11Setting::RearrangeTriangle(sce_1_1_0::cr_cb_triangle* pInput)
{
    BOOL result = true;

    // initialize
    UINT8    index[SCE11MaxTriangleIdx];
    UINT8    temp;
    UINT8    i;
    INT8     ii;
    UINT8    j;
    Triangle rTriang;

    // copy the data to tTriang
    rTriang.point[0].cb = static_cast<FLOAT>(pInput->point1[1]);
    rTriang.point[0].cr = static_cast<FLOAT>(pInput->point1[0]);
    rTriang.point[1].cb = static_cast<FLOAT>(pInput->point2[1]);
    rTriang.point[1].cr = static_cast<FLOAT>(pInput->point2[0]);
    rTriang.point[2].cb = static_cast<FLOAT>(pInput->point3[1]);
    rTriang.point[2].cr = static_cast<FLOAT>(pInput->point3[0]);

    for (i = 0; i < SCE11MaxTriangleIdx; i++)
    {
        index[i] = i;
    }

    /// select the first vertex
    for (i = 1; i < 3; i++)
    {
        // Pick the first one with smallest y value
        if (rTriang.point[index[0]].cb > rTriang.point[index[i]].cb)
        {
            temp     = index[0];
            index[0] = index[i];
            index[i] = temp;
        }
        // If there is another point with the same y value, then pick the one with the smallest x value.
        else if (rTriang.point[index[0]].cb == rTriang.point[index[i]].cb)
        {
            if (rTriang.point[index[0]].cr > rTriang.point[index[i]].cr)
            {
                temp     = index[0];
                index[0] = index[i];
                index[i] = temp;
            }
        }
    }

    /// Select the second vertex
    // Pick the one with larger x value
    if (rTriang.point[index[1]].cr < rTriang.point[index[2]].cr)
    {
        temp     = index[1];
        index[1] = index[2];
        index[2] = temp;
    }
    // If there is another point with the same x value, then pick the one with the smaller y value.
    else if (rTriang.point[index[1]].cr == rTriang.point[index[2]].cr)
    {
        if (rTriang.point[index[1]].cb > rTriang.point[index[2]].cb)
        {
            temp     = index[1];
            index[1] = index[2];
            index[2] = temp;
        }
    }

    /// Dont need select the third vertex as the result of previous two swaps
    if (index[0] == index[1] || index[0] == index[2] || index[1] == index[2])
    {
        result = FALSE;
    }

    // copy back the data to for counter clockwise
    if (result == TRUE)
    {
        // vertices rotation
        // find the start vertex
        ii = -1;
        for (j = 0; j < 3; j++)
        {
            if (rTriang.point[index[j]].cr == pInput->point1[0] &&
                rTriang.point[index[j]].cb == pInput->point1[1])
            {
                ii = j;
                break;
            }
        }

        // rotate vertices
        if (ii == 0)
        {
            pInput->point1[1] = static_cast<INT32>(rTriang.point[index[0]].cb);
            pInput->point1[0] = static_cast<INT32>(rTriang.point[index[0]].cr);
            pInput->point2[1] = static_cast<INT32>(rTriang.point[index[1]].cb);
            pInput->point2[0] = static_cast<INT32>(rTriang.point[index[1]].cr);
            pInput->point3[1] = static_cast<INT32>(rTriang.point[index[2]].cb);
            pInput->point3[0] = static_cast<INT32>(rTriang.point[index[2]].cr);
        }
        else if (ii == 1)
        {
            pInput->point1[1] = static_cast<INT32>(rTriang.point[index[1]].cb);
            pInput->point1[0] = static_cast<INT32>(rTriang.point[index[1]].cr);
            pInput->point2[1] = static_cast<INT32>(rTriang.point[index[2]].cb);
            pInput->point2[0] = static_cast<INT32>(rTriang.point[index[2]].cr);
            pInput->point3[1] = static_cast<INT32>(rTriang.point[index[0]].cb);
            pInput->point3[0] = static_cast<INT32>(rTriang.point[index[0]].cr);
        }
        else
        {
            pInput->point1[1] = static_cast<INT32>(rTriang.point[index[2]].cb);
            pInput->point1[0] = static_cast<INT32>(rTriang.point[index[2]].cr);
            pInput->point2[1] = static_cast<INT32>(rTriang.point[index[0]].cb);
            pInput->point2[0] = static_cast<INT32>(rTriang.point[index[0]].cr);
            pInput->point3[1] = static_cast<INT32>(rTriang.point[index[1]].cb);
            pInput->point3[0] = static_cast<INT32>(rTriang.point[index[1]].cr);
        }
    }

    return result;
}
