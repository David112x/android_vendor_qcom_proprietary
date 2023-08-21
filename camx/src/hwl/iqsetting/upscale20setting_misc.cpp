// NOWHINE ENTIRE FILE  --- keep the file to be in sync with PCSIM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010,2017,2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// *****************************************************************************
/// *   Project Title       : New PCSim
/// *   File Name           : 2d_upscale_v10.cpp
/// *   Author              : Minl
/// *   Created             : Aug. 15, 2015
/// *
/// *****************************************************************************
/// *   Description         : 2D Upscalar
/// *
/// *
/// *   History             : <>  08-15-2015 Creation.
/// *
/// *****************************************************************************

#include "upscale20setting_misc.h"

/*****************************************************************/
/*       0   1   2                                               */
/*       |       |                                               */
/*   0  - A - D - G -                                            */
/*       |       |                                               */
/*  16    B   E   H                                              */
/*       |       |                                               */
/*  32  - C - F - I -                                            */
/*       |       |                                               */
/*****************************************************************/

static const UINT32 NbItsChromaSiteResolution = 7;
static const UINT32 VerticalNum2DFilterTaps   = 4;
static const UINT32 HorizontalNum2DFilterTaps = 4;
static const UINT32 ChromaSiteBaseTableSize   = 9;
static const UINT32 DirectionLUTSize          = 200;
static const UINT32 CircularLUTSize           = 60;
static const UINT32 SeparableLUTSize          = 60;
static const UINT32 CircularLUTSet            = 9;
static const UINT32 SeparableLUTSet           = 10;

// curve min / max limits
static const INT16  CurveA0Min = 0;
static const INT16  CurveA0Max = 511;
static const INT16  CurveA1Min = -63;
static const INT16  CurveA1Max = 511;
static const INT16  CurveA2Min = -64;
static const INT16  CurveA2Max = 0;
static const INT16  CurveB0Min = -511;
static const INT16  CurveB0Max = 511;
static const INT16  CurveB1Min = -511;
static const INT16  CurveB1Max = 511;
static const INT16  CurveB2Min = 0;
static const INT16  CurveB2Max = 511;
static const INT16  CurveC0Min = -511;
static const INT16  CurveC0Max = 511;
static const INT16  CurveC1Min = -511;
static const INT16  CurveC1Max = 511;
static const INT16  CurveC2Min = -511;
static const INT16  CurveC2Max = 511;

// qseed3_chroma_site_x_x2_base_tbl updated from {0,0,0,16,16,16,32,32,32}.
// The resolution is not enough when rotator downscaler is taken into account.
static INT32 qseed3_chroma_site_x_x2_base_tbl[ChromaSiteBaseTableSize] = {
    0, 0, 0, 64, 64, 64, 128, 128, 128 };
static INT32 qseed3_chroma_site_y_x2_base_tbl[ChromaSiteBaseTableSize] = {
    0, 64, 128, 0, 64, 128, 0, 64, 128 };

// LUT tables
static UINT32 dirLUTs[DirectionLUTSize]                  = {
    0X3BF93B36, 0XF4396D80, 0XF2341CFB, 0X1F3E7D00, 0X15FF4E1E, 0XF26E66FF, 0XFA481179, 0X1F3E3E00, 0XE5309, 0XA551FC, 0X125206F8, 0X1F9E3F00, 0X4EF9151E, 0XF8226680, 0XF0483779, 0X1FFF7CFF, 0X21F82110, 0XF052677F, 0XFA6D2977, 0X1F1F7C7F, 0X6FD2A03, 0XF485587F, 0X1A8016F9, 0X1F3F3D00, 0X53FC0009, 0XFC0D5180, 0XF05252F8, 0X1C2407C, 0X2AF90603, 0XF42D5800, 0XF88042F9, 0X1FA37D7F, 0XBF80BFE, 0XF0595100, 0X16A42CFE, 0X1F02FC00, 0X3B353BF9, 0XF4396C79, 0X1F71C80, 0X6A03D79, 0X144A24F9, 0XF26D667D, 0XFFFD2580, 0X3C03CF8, 0X530EFC, 0XA55189, 0XF80D2900, 0X13FBC78, 0X241E14F9, 0XF24B6678, 0X1FD3680, 0X95FFCFD, 0X72807FC, 0XF48059FB, 0XFA0F4000, 0X51F7D7B, 0XFA2AFDFF, 0XCB24183, 0XF22D4000, 0X1BEBE79, 0XE0900FC, 0XF05251F8, 0XFC0D5280, 0XA7F0009, 0XFD0DFAFF, 0XF88041F9, 0XF42D5900, 0X55E4303, 0XF80BF800, 0X16A42BFE, 0XF0595200, 0X17E05FE, 0X26FB462A, 0XF2526B80, 0XF43F177A, 0X1F1E7D80, 0X8055213, 0XF68A5E7E, 0X44F0B79, 0X1F5E3E80, 0X36F81C18, 0XF4376800, 0XF45A2FF9, 0X1F7FBC00, 0X12FA270A, 0XF06A6100, 0X6771F79, 0X1F1F7C80, 0X3DFB0207, 0XF81B5700, 0XF2694BF8, 0X62FEFE, 0X19F80900, 0XF2425600, 0X294387B, 0X1F433C80, 0X26402FF9, 0XF25269FA, 0X1FA2200, 0X5403CF9, 0X85018FB, 0XF68A5C82, 0XFC042880, 0X27FFC78, 0X13240DFA, 0XF06660F9, 0XFE053D00, 0X73FBCFC, 0XFF2A01FE, 0XFE9A4EFF, 0XF61C4100, 0X33F3DFA, 0X30BFDFE, 0XF2694BF8, 0XF81B5700, 0X7BEC107, 0XFA0CF900, 0X294377B, 0XF2425700, 0X33E0480, 0X46F8262A, 0XF62E6B80, 0XF23F297A, 0X1F7EBC80, 0X1CFB3618, 0XF05F6800, 0XFC5A1BF9, 0X1F1EBD00, 0X2033D07, 0XFA97577E, 0X16690DF8, 0X1F7E7E00, 0X52FA0813, 0XFA165E00, 0XF04F4579, 0XA0BDFE, 0X27F8120A, 0XF23E6100, 0XFA773579, 0X1F40FC00, 0X9FA1900, 0XF2705600, 0X1894217B, 0X1F007C80, 0X2F2A26F9, 0XF24469F9, 0X1FA2900, 0X8003CFA, 0XD3913FA, 0XF27A60FC, 0XFC053300, 0X49FFC79, 0XFD3D03FE, 0X4AE4B87, 0XF61B3480, 0X17F3CF8, 0X181308FB, 0XF0515CF8, 0XFE044500, 0XA1FBD82, 0X119FFFE, 0XF6824EFA, 0XF81C4D00, 0X55EFFFF, 0XF919FA00, 0X12AE3700, 0XF0424A00, 0X19E40FB, 0X2FF92F21, 0XF2466B00, 0XF44E2379, 0X1F3EBC80, 0XDFF3B0F, 0XF27B61FF, 0X6631478, 0X1F3EBD80, 0X3BF90D0F, 0XF6286180, 0XF4633DF8, 0X1FE0FCFF, 0X16F81604, 0XF0565E00, 0X6882B7A, 0X1F00FC00, 0X1C321CF9, 0XF05D67FA, 0X1FE2E80, 0X6403C7A, 0X33C0BFC, 0XFA945780, 0XFA0E3480, 0X2FF7CF9, 0XB1703FC, 0XF26957F9, 0XFA0E4A00, 0X79F7E80, 0XFB19FBFE, 0XFE9D44FC, 0XF2304EFF, 0X33E7FFC };

static UINT32 circLUTs[CircularLUTSet][CircularLUTSize]  = {
    { 0x2DF32DF3, 0x2CC180, 0x1F21600, 0x1E600000, 0xFDFD24F3, 0xF087AF00, 0x1FC1180, 0x1E600000, 0xF21111F9, 0xE4FC7E79, 0xF20F0780, 0x1F200000, 0xFAF1FAF7, 0xEE7AA07F, 0xF61E3D7F, 0x1E3EFBFF, 0xF1F4F4FD, 0xE2E571FD, 0xE84E277F, 0x1E9E7CFF, 0xF2F2F2FF, 0xE49D4EFF, 0xE49D4EFF, 0x1E5CB97F, 0xFF72BF2, 0xF854BE80, 0x1F51480, 0x1E400000, 0xF4061BF6, 0xE8C199FD, 0xFA040C80, 0x1EC00000, 0xF3F3F8FB, 0xE8AB8A00, 0xF2323180, 0x1E5EFC80, 0xFDF50AF5, 0xF083AC00, 0xFC082400, 0x1E403E00, 0xF1FFFFFB, 0xE2F97CFB, 0xEC2B15FF, 0x1EDF3E7F, 0xF1F1F1FF, 0xE2C4617F, 0xE6753AFF, 0x1E7D7AFF, 0xEF20EF3, 0xF852B980, 0x1FE2900, 0x1E403E00, 0xF3F904F7, 0xE6BF987D, 0xF41A1E7F, 0x1E7F7E7F, 0xF2F2F2FD, 0xEA917800, 0xEE514880, 0x1E5DFA80 },
    { 0x3CFC3CFC, 0xF63B9680, 0x1FB1D80, 0x1F803D80, 0xC0C36F5, 0xEA888E7B, 0xF60B1A80, 0x1EBFBD80, 0xF82323F6, 0xF0DC6E76, 0xEC211080, 0x1EDF3E00, 0x9F909F6, 0xEC7B857E, 0xF02E3D80, 0x1F3E3B7E, 0xF70101FA, 0xEECB65FA, 0xEA582C00, 0x1EBDBB00, 0xF4F4F4FC, 0xE89C4E7C, 0xE89C4E7C, 0x1E9D3A7C, 0x20023AF7, 0xEE6098FD, 0xFA031DFF, 0x1EFFBD7F, 0xFF172DF5, 0xEAB280F9, 0xF2151580, 0x1EBF7E00, 0xFDFD05F7, 0xEAA4777C, 0xEC433600, 0x1EBDFB00, 0xB001CF5, 0xEA848DFD, 0xF41A2A80, 0x1EFEFC00, 0xF70F0FF8, 0xEEDA6C78, 0xEA3B1D80, 0x1EBE7C80, 0xF5F8F8FA, 0xEAB85C7A, 0xE87B3DFE, 0x1E9D3A7E, 0x20FB20F5, 0xF05C95FF, 0xF80C2E00, 0x1F7F3C7F, 0xFF0716F6, 0xEAAF7F7A, 0xEE2A2480, 0x1EBEBC80, 0xFAF6FAF8, 0xE8936CFD, 0xE86049FF, 0x1EDD3A7D },
    { 0x470D470D, 0xF2466AFB, 0xF60C2300, 0x1BEFCFB, 0x1F1F42FF, 0xF47A66F8, 0xF021227F, 0x1FFE7C7C, 0x53333F9, 0xAAC5579, 0xF2351AFE, 0x1F3E3C7E, 0x1D091DFA, 0xF472617A, 0xF03E397E, 0x13E3D7A, 0x51515F8, 0xAA150F8, 0xFA5B2DFC, 0x1FBE7CFC, 0x10101F9, 0x28542F9, 0x28542F9, 0x2040F9, 0x32164606, 0xF25E6A7A, 0xF4142200, 0xDEBCFC, 0x10293CFB, 0xFC945FF8, 0xF02B1F7E, 0x1F7E3C7D, 0xE0E19F8, 0xFA8D5B78, 0xF24F35FC, 0x3E3CFA, 0x1F132EFC, 0xF4796678, 0xEE302DFE, 0x7E3C7A, 0x62323F9, 0xCA85479, 0xF646237D, 0x1F7E3C7D, 0x20808F8, 0x4984B78, 0xFC743A7A, 0x1FDEFDFA, 0x310C3100, 0xF25E68FA, 0xF2212F00, 0x19E7CFA, 0x111C2AFA, 0xFE905DF8, 0xF23A28FE, 0x1FFE3C7C, 0xD050DF8, 0xFA805379, 0xF660407C, 0xBEFEF9 },
    { 0x47184718, 0xF649517A, 0xF41A24FE, 0x31EBDFA, 0x2929440B, 0x6674BFC, 0xF82821FE, 0x17EFE7B, 0x12393901, 0x24844201, 0x2371BFC, 0x3F3E7C, 0x27152702, 0x46248FB, 0xFC3F317C, 0x2BF817B, 0x101F1FFD, 0x207F3FFD, 0xC542A7B, 0xC0007B, 0xC0C0CFB, 0x186D36FB, 0x186D36FB, 0x183067B, 0x38214612, 0xFE564FFB, 0xF61F227E, 0x25EFE7B, 0x1D313F05, 0x127747FE, 0xFC301F7D, 0xBEFE7C, 0x1B1B2400, 0x107044FC, 0x2492DFB, 0x1A000FB, 0x291F3406, 0x6664AFB, 0xFA33297D, 0x21F3FFB, 0x122C2CFF, 0x248340FF, 0x64522FB, 0x7FBF7B, 0xF1515FC, 0x1E773AFC, 0x126130FB, 0x12142FB, 0x3718370B, 0xFE554E7B, 0xF62A2AFD, 0x31EFFFB, 0x1C253102, 0x127447FD, 0x3C267C, 0x13F7F7B, 0x181118FD, 0xE6A417B, 0x656357B, 0x220C3FB },
    { 0x44204420, 0x243407D, 0xFA1F21FD, 0x41F40FD, 0x2D2D4012, 0x145A3D00, 0x2D207C, 0x25FC07C, 0x19383808, 0x326C3608, 0x10371BFC, 0x100007C, 0x2A1C2A09, 0x125439FF, 0x83C2A7C, 0x38104FF, 0x17242402, 0x2E673282, 0x1C4B25FD, 0x1C1C3FD, 0x131313FF, 0x265B2DFF, 0x265B2DFF, 0x264C9FF, 0x38264319, 0xA4E3E7F, 0xFE25217D, 0x33F80FD, 0x22333D0D, 0x22633904, 0x8321E7C, 0x1A0007C, 0x20202705, 0x205F3680, 0x104428FC, 0x281447E, 0x2C24350E, 0x14573C00, 0x434257C, 0x2E0027D, 0x182E2E05, 0x306A3505, 0x164120FC, 0x160C1FC, 0x151B1B00, 0x2A613080, 0x225329FE, 0x22346FE, 0x371E3713, 0xA4D3D7E, 0x2D26FC, 0x3C002FE, 0x21293209, 0x22613882, 0xC3A237C, 0x220827D, 0x1E181E02, 0x1C59347F, 0x164B2CFD, 0x302C77F },
    { 0x3C253C25, 0x163B2D84, 0x8241DFE, 0x4A10584, 0x2D2D381A, 0x28472B0A, 0x142D1C7F, 0x3418501, 0x1F333311, 0x3E512791, 0x22341A00, 0x2220400, 0x2A212A12, 0x24432986, 0x1A352180, 0x4234906, 0x1D26260B, 0x3A4D258B, 0x2C3E1F02, 0x2C40802, 0x1A1A1A06, 0x34462306, 0x34462306, 0x3468D06, 0x34293A20, 0x1E412C07, 0xE291D7E, 0x4014503, 0x26303616, 0x324C2A0D, 0x1A301B7F, 0x2C1C480, 0x2323280F, 0x30482888, 0x22392001, 0x363C884, 0x2C263116, 0x26452A08, 0x18311F7F, 0x3C28703, 0x1E2C2C0E, 0x3C4F268E, 0x28391C81, 0x2830601, 0x1C202009, 0x38492489, 0x30412084, 0x3054A84, 0x3323331B, 0x1C402B85, 0x102D207F, 0x4620705, 0x252A2F12, 0x324A280B, 0x1E351D80, 0x322C682, 0x221E220C, 0x2C452606, 0x263E2282, 0x3C4CB06 },
    { 0x37263726, 0x1E362689, 0x12251B00, 0x4C24789, 0x2B2B341D, 0x2E3F248E, 0x1C2C1A81, 0x3A2C706, 0x21303015, 0x42452295, 0x2A301803, 0x2A34683, 0x29222916, 0x2C3C228A, 0x22321E03, 0x4444B0A, 0x1F262610, 0x3E412090, 0x32371B86, 0x3250A06, 0x1C1C1C0B, 0x383D1E8B, 0x383D1E8B, 0x3870E0B, 0x31293621, 0x263B258B, 0x16291B00, 0x4228787, 0x262E3219, 0x38422312, 0x242E1902, 0x3230704, 0x24242713, 0x343E228D, 0x2A341C85, 0x3A4CA88, 0x2A262E19, 0x2E3E238C, 0x202F1C82, 0x3E38908, 0x202B2B12, 0x40432192, 0x2E341A05, 0x2E40805, 0x1E21210D, 0x3C3F1E8D, 0x363A1D09, 0x3660C09, 0x3024301D, 0x2639240A, 0x1A2C1C81, 0x483498A, 0x25292D16, 0x3640238F, 0x26311B03, 0x363C886, 0x221F2210, 0x323C210B, 0x2E371E07, 0x3E5CC8B },
    { 0x32263226, 0x2832210D, 0x1A261902, 0x4C34A0D, 0x2A2A301F, 0x34361F93, 0x26291785, 0x3E3C98A, 0x212D2D18, 0x423C1E18, 0x302E1707, 0x3044887, 0x28222819, 0x32341E0F, 0x2A2D1A08, 0x4454C8F, 0x20252513, 0x403A1C13, 0x3632190B, 0x365CB8B, 0x1E1E1E0F, 0x3C351A8F, 0x3C351A8F, 0x3C78F0F, 0x2E283122, 0x2E352010, 0x20281883, 0x443898C, 0x252B2F1B, 0x3C3A1E15, 0x2A2C1806, 0x3640909, 0x24242616, 0x38371D11, 0x30301989, 0x3E58C0D, 0x29262C1C, 0x34361E11, 0x282C1906, 0x4048B0C, 0x21292916, 0x423A1D16, 0x342F1789, 0x3450A09, 0x1F212111, 0x3E371B91, 0x3A341A0D, 0x3A68D0D, 0x2D242D1F, 0x2C341F0E, 0x222A1A05, 0x4844B0E, 0x25272A19, 0x3A381D93, 0x2E2E1888, 0x3A4CA8B, 0x23202314, 0x36351C8F, 0x32321A8B, 0x4064D8F },
    { 0x2C252C25, 0x302D1A13, 0x26261689, 0x4A4CC13, 0x27272B20, 0x3A2F1997, 0x2E27158B, 0x4054B91, 0x2229291C, 0x4431179C, 0x3829148E, 0x3858B0E, 0x2522251C, 0x382D1994, 0x3229168E, 0x4464E14, 0x20242418, 0x40311798, 0x3A2C1611, 0x3A68D11, 0x1F1F1F14, 0x3E2E1714, 0x3E2E1714, 0x3E7CF94, 0x2A262C22, 0x342E1A15, 0x2A26160A, 0x4450C12, 0x24282A1E, 0x3E301919, 0x3228150D, 0x3C58B8F, 0x2323251A, 0x3C2E1816, 0x362A1610, 0x4068D93, 0x2624281E, 0x382E1996, 0x3028160D, 0x425CD12, 0x2126261A, 0x4231189A, 0x382B158F, 0x3860C0F, 0x20212116, 0x402F1696, 0x3C2D1693, 0x3C74E93, 0x29232920, 0x342D1A14, 0x2C27168B, 0x4658D14, 0x2425271C, 0x3E2F1898, 0x3429158E, 0x3E60C91, 0x22212218, 0x3A2E1814, 0x382B1711, 0x4270E94 } };

static UINT32 sepLUTs[SeparableLUTSet][SeparableLUTSize] = {
    { 0x0, 0x8000, 0x0, 0x80000000, 0xDF000000, 0xF06EDD80, 0x0, 0x40000000, 0xE9000000, 0xD3178B80, 0x0, 0x20000000, 0xE3F9E302, 0xF25FC001, 0xFC182F80, 0x3F3FBC81, 0xECEEEE01, 0xD8F27801, 0xF63D1E80, 0x3F7F3E00, 0xF3F3F301, 0xE6994C81, 0xE6994C81, 0x1E7CF981, 0xF3000000, 0xFC177B00, 0x10080, 0x80000000, 0xE1000000, 0xE4BEB780, 0x0, 0x20000000, 0xE5F4E802, 0xE8A49F81, 0xFA292780, 0x3F3F7D00, 0xE0FBEA02, 0xF06BD580, 0xFE0B1400, 0x3FBFFE80, 0xEAF2F201, 0xD50E8701, 0xFC190C80, 0x3FDFBF00, 0xF0EFEF01, 0xE0C86401, 0xEE683401, 0x3EFE3C01, 0xE8FEE801, 0xFA2CEE80, 0x41600, 0x3FC03E80, 0xE2F7EE02, 0xE4B8B101, 0xFE111000, 0x3FBFFF00, 0xEAF4EA02, 0xEC888381, 0xF6474400, 0x3E9EFB01 },
    { 0x0, 0x8000, 0x0, 0x80000000, 0x0, 0x80C000, 0x0, 0x0, 0x0, 0x1008000, 0x0, 0x0, 0x0, 0x609000, 0x203000, 0x0, 0x0, 0xC06000, 0x402000, 0x0, 0x0, 0x804000, 0x804000, 0x0, 0x0, 0x40E000, 0x0, 0x0, 0x0, 0xC0A000, 0x0, 0x0, 0x0, 0x907800, 0x302800, 0x0, 0x0, 0x70A800, 0x101800, 0x0, 0x0, 0xE07000, 0x201000, 0x0, 0x0, 0xA05000, 0x603000, 0x0, 0x0, 0x38C400, 0x81C00, 0x0, 0x0, 0xA88C00, 0x181400, 0x0, 0x0, 0x786400, 0x483C00, 0x0 },
    { 0x29052905, 0x29A400, 0x51480, 0xA00000, 0x150F2103, 0x7B8500, 0xF1080, 0x600000, 0x1A1A00, 0xCD6580, 0x1A0D00, 0x0, 0x11081101, 0x646B80, 0x2E3200, 0x1000000, 0xD0D00, 0xA65300, 0x4D2680, 0x0, 0x0, 0x804000, 0x804000, 0x0, 0x1E0A2504, 0x529500, 0xA1280, 0x800000, 0xA141D01, 0xA57580, 0x150F00, 0x200000, 0x80A0F01, 0x855F80, 0x3E2C00, 0x800000, 0x130B1902, 0x707880, 0x1F2100, 0xA00000, 0x131300, 0xBA5D00, 0x331980, 0x0, 0x70700, 0x934880, 0x673380, 0x0, 0x1C081C03, 0x4A8680, 0x142500, 0x1000000, 0x90F1601, 0x956A80, 0x291D80, 0x600000, 0x7050700, 0x765500, 0x523B00, 0xA00000 },
    { 0x390E390E, 0x397200, 0xE1C80, 0x1C00000, 0x1D1C3107, 0x736300, 0x1D1900, 0xE00000, 0x2B2B00, 0xAB5480, 0x2B1580, 0x0, 0x190E1904, 0x645680, 0x393200, 0x1C00000, 0x161600, 0x954980, 0x562B00, 0x0, 0x0, 0x804000, 0x804000, 0x0, 0x2B15350B, 0x566A80, 0x151A80, 0x1600000, 0xE232E03, 0x8F5C80, 0x241780, 0x600000, 0xC121702, 0x7C5100, 0x472E80, 0xE00000, 0x1B152505, 0x6B5D00, 0x2B2580, 0x1600000, 0x202000, 0xA05000, 0x402000, 0x0, 0xB0B00, 0x8B4480, 0x6B3580, 0x0, 0x28102808, 0x506400, 0x202800, 0x2000000, 0xD1B2303, 0x855680, 0x352300, 0xA00000, 0xB090B01, 0x744B00, 0x593A00, 0x1200000 },
    { 0x3F183F18, 0x3E5380, 0x171F00, 0x3000000, 0x1F273B0C, 0x684E80, 0x271D80, 0x1800000, 0x373700, 0x924900, 0x371B80, 0x0, 0x1D141D06, 0x624980, 0x413100, 0x2800000, 0x1C1C00, 0x894380, 0x5C2E00, 0x0, 0x0, 0x804000, 0x804000, 0x0, 0x2F1F3D12, 0x535100, 0x1F1E80, 0x2400000, 0xF2F3906, 0x7D4C00, 0x2F1C80, 0xC00000, 0xE181D03, 0x754700, 0x4E2F80, 0x1400000, 0x1F1D2C09, 0x664B80, 0x342700, 0x2000000, 0x292900, 0x8E4700, 0x492480, 0x0, 0xE0E00, 0x854180, 0x6E3700, 0x0, 0x2D172D0D, 0x514F80, 0x2A2880, 0x2E00000, 0xF232B04, 0x794A00, 0x3E2600, 0x1000000, 0xE0C0E01, 0x724480, 0x5E3900, 0x1800000 },
    { 0x40204020, 0x404000, 0x202000, 0x4000000, 0x20304010, 0x604000, 0x302000, 0x2000000, 0x404000, 0x804000, 0x402000, 0x0, 0x20182008, 0x604000, 0x483000, 0x3000000, 0x202000, 0x804000, 0x603000, 0x0, 0x0, 0x804000, 0x804000, 0x0, 0x30284018, 0x504000, 0x282000, 0x3000000, 0x10384008, 0x704000, 0x382000, 0x1000000, 0x101C2004, 0x704000, 0x543000, 0x1800000, 0x2024300C, 0x604000, 0x3C2800, 0x2800000, 0x303000, 0x804000, 0x502800, 0x0, 0x101000, 0x804000, 0x703800, 0x0, 0x301E3012, 0x504000, 0x322800, 0x3C00000, 0x102A3006, 0x704000, 0x462800, 0x1400000, 0x100E1002, 0x704000, 0x623800, 0x1C00000 },
    { 0x3D2E3D2E, 0x3E2880, 0x2F1F00, 0x5C00000, 0x29363D1F, 0x14472908, 0x10361E80, 0x3E00000, 0x143E3E0F, 0x2852290F, 0x1E3E1F00, 0x1E00000, 0x29242915, 0x14482805, 0x12402401, 0x4824505, 0x1429290A, 0x2852290A, 0x24482403, 0x2428503, 0x14141405, 0x28532985, 0x28532985, 0x2850A05, 0x33323D27, 0xA432884, 0x8321E80, 0x4E00000, 0x1F3A3E17, 0x1E4D288B, 0x163A1F00, 0x2E00000, 0x1F27290F, 0x1E4D2888, 0x1A432402, 0x3628504, 0x292D331A, 0x14472807, 0x103A2181, 0x4414283, 0x1433330D, 0x2852290D, 0x22432181, 0x2214281, 0x141F1F08, 0x28522908, 0x264D2684, 0x263C784, 0x332A3320, 0xA432903, 0x8362180, 0x5410283, 0x1F303313, 0x1E4D288A, 0x183F2181, 0x3214282, 0x1F1D1F0C, 0x1E4D2886, 0x1C482683, 0x3A38786 },
    { 0x39393939, 0x391C00, 0x391C80, 0x7200000, 0x2B39392B, 0x1C391C0E, 0x1C391C80, 0x5600000, 0x1C39391C, 0x383A1C1C, 0x383A1D00, 0x3800000, 0x2B2B2B20, 0x1C391B8B, 0x1C391C84, 0x563870B, 0x1C2B2B15, 0x383A1C15, 0x383A1D07, 0x3838707, 0x1C1C1C0E, 0x383A1D0E, 0x383A1D0E, 0x3870E0E, 0x31393932, 0xE391C87, 0xE391C80, 0x6400000, 0x24393924, 0x2A391C15, 0x2A391C80, 0x4800000, 0x242B2B1B, 0x2A391C10, 0x2A391C85, 0x4838709, 0x2B323125, 0x1C391C8D, 0x1C391C82, 0x561C385, 0x1C323219, 0x383A1C19, 0x383A1D03, 0x381C383, 0x1C242412, 0x38391C92, 0x38391C8B, 0x3854A8B, 0x3232322B, 0xE391C86, 0xE391C81, 0x641C386, 0x2432311F, 0x2A391C93, 0x2A391C83, 0x481C384, 0x24242416, 0x2A391C8D, 0x2A391C88, 0x4854A8D },
    { 0x2A2A2A2A, 0x2A2A1415, 0x2A2A150A, 0x5454A95, 0x252A2A25, 0x342A141A, 0x342A150D, 0x4A54A92, 0x202A2A1F, 0x402A141F, 0x3E2A1510, 0x3E54A90, 0x25252520, 0x342A1417, 0x342A1510, 0x4A68D17, 0x2025251C, 0x4029149C, 0x3E291494, 0x3E68D14, 0x1F1F1F18, 0x3E2A1518, 0x3E2A1518, 0x3E7CF98, 0x272A2A27, 0x302A1497, 0x2E2A150C, 0x4E54A94, 0x222A2A22, 0x3A2A141D, 0x3A2A150E, 0x4454A91, 0x2225251E, 0x3A2A1419, 0x3A2A1512, 0x4468D15, 0x25272722, 0x342A1498, 0x342A150F, 0x4A5CC15, 0x2027271D, 0x402A151D, 0x3E2A1512, 0x3E5CB92, 0x1F22221A, 0x3E2A141A, 0x3E2A1516, 0x3E74E96, 0x27272725, 0x302A1516, 0x2E2A150D, 0x4E5CC16, 0x22272720, 0x3A2A151B, 0x3A2A1510, 0x445CB93, 0x2222221C, 0x3A2A1417, 0x3A2A1514, 0x4474E97 },
    { 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020, 0x20202020, 0x40201020, 0x40201020, 0x4081020 } };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_phase_step_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_phase_step_for_qseed3(
    UINT32* comp_in_width,     // Input width for Y and UV channels
    UINT32* comp_in_height,    // Input height for Y and UV channels
    UINT32* comp_out_width,    // Output width for Y and UV channels
    UINT32* comp_out_height,   // Output height for Y and UV channels
    UINT32* comp_phase_step_x, // horizontal phase step
    UINT32* comp_phase_step_y) // vertical phase step
{
    DOUBLE temp;
    UINT32 i;

    for (i = 0; i < MaxYorUVChannels; i++)
    {
        temp = static_cast<DOUBLE>(comp_in_width[i]) * static_cast<DOUBLE>(FixPixelUnitScale) /
            static_cast<DOUBLE>(comp_out_width[i]);
        comp_phase_step_x[i] = static_cast<UINT32>(temp);

        temp = static_cast<DOUBLE>(comp_in_height[i]) * static_cast<DOUBLE>(FixPixelUnitScale) /
            static_cast<DOUBLE>(comp_out_height[i]);
        comp_phase_step_y[i] = static_cast<UINT32>(temp);
    }

    if ((comp_in_width[1] != (comp_in_width[0] >> 1)) && (comp_in_width[1] != comp_in_width[0]))
    {
        comp_phase_step_x[1] = comp_phase_step_x[0] >> 1;
    }

    if ((comp_in_height[1] != (comp_in_height[0] >> 1)) && (comp_in_height[1] != comp_in_height[0]))
    {
        comp_phase_step_y[1] = comp_phase_step_y[0] >> 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_init_phase_core
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_init_phase_core(
    INT32*  new_delta_phase,
    UINT32  chroma_subsample_flg,
    UINT32* comp_phase_step,
    UINT16* skip,
    INT32*  comp_init_phase)
{
    // unreference these parameters to avoid compilation warning
    // keep these parameters for easy syncing up with PCSIM file
    CAMX_UNREFERENCED_PARAM(skip);

    comp_init_phase[0] = (static_cast<INT32>(comp_phase_step[0]) - static_cast<INT32>(FixPixelUnitScale)) >> 1;

    if (TRUE == chroma_subsample_flg)
    {
        INT32 temp         = (*new_delta_phase) * static_cast<INT32>(FixPixelUnitScale);
        comp_init_phase[1] = (comp_init_phase[0] - (temp >> NbItsChromaSiteResolution)) >> 1; // original L=2.
    }
    else
    {
        comp_init_phase[1] = comp_init_phase[0];
    }

    // pre-add residual bits.
    comp_init_phase[0] += static_cast<INT32>(FirPhaseResidual);
    comp_init_phase[1] += static_cast<INT32>(FirPhaseResidual);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_init_phase_overall
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_init_phase_overall(
    UINT32                 chroma_subsample_x_flg,
    UINT32                 chroma_subsample_y_flg,
    MDP_Chroma_Sample_Site chroma_site_io,
    UINT32*                comp_phase_step_x,
    UINT32*                comp_phase_step_y,
    INT32*                 comp_init_phase_x,
    INT32*                 comp_init_phase_y)
{
    UINT32 chroma_site = 0;

    INT32  org_chroma_site_hor; // use 8 bits to represent, values are from 0 to 128.
    INT32  org_chroma_site_ver;//use 8 bits to represent, values are from 0 to 128.
    INT32  org_delta_phase_hor;
    INT32  org_delta_phase_ver;

    if (( chroma_site_io == MDP_CHROMA_SAMPLE_SITE_NONE) &&
        ((chroma_subsample_x_flg == 1) || (chroma_subsample_y_flg == 1)))
    {
        // warning: Chroma site can't be MDP_CHROMA_SAMPLE_SITE_NONE when chroma subsampling is true.");
        chroma_site_io = MDP_CHROMA_SAMPLE_SITE_E;
    }

    chroma_site         = static_cast<UINT32>((chroma_site_io > MDP_CHROMA_SAMPLE_SITE_NONE) ? (chroma_site_io - 1) : 0);
    org_chroma_site_hor = qseed3_chroma_site_x_x2_base_tbl[chroma_site];
    org_chroma_site_ver = qseed3_chroma_site_y_x2_base_tbl[chroma_site];
    org_delta_phase_hor = org_chroma_site_hor;
    org_delta_phase_ver = org_chroma_site_ver;

    cal_init_phase_core(&org_delta_phase_hor,
                        chroma_subsample_x_flg,
                        comp_phase_step_x,
                        0,
                        comp_init_phase_x);

    cal_init_phase_core(&org_delta_phase_ver,
                        chroma_subsample_y_flg,
                        comp_phase_step_y,
                        0,
                        comp_init_phase_y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_start_phase_common_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_start_phase_common_qseed3(
    UINT32                 chroma_subsample_x_flg,
    UINT32                 chroma_subsample_y_flg,
    MDP_Chroma_Sample_Site chroma_site_io,
    UINT32*                comp_phase_step_x,
    UINT32*                comp_phase_step_y,
    UINT32*                input_comp_offset,
    UINT32*                output_comp_offset,
    INT32*                 comp_init_phase_x,
    INT32*                 comp_init_phase_y)
{
    INT   i;
    INT32 overall_comp_init_phase_x[MaxYorUVChannels];
    INT32 overall_comp_init_phase_y[MaxYorUVChannels];

    cal_init_phase_overall(chroma_subsample_x_flg,
                           chroma_subsample_y_flg,
                           chroma_site_io,
                           comp_phase_step_x,
                           comp_phase_step_y,
                           overall_comp_init_phase_x,
                           overall_comp_init_phase_y);

    for (i = 0; i < MaxYorUVChannels; i++)
    {
        comp_init_phase_x[i] = overall_comp_init_phase_x[i] + static_cast<INT32>(
            (output_comp_offset[i] * comp_phase_step_x[i]) - (input_comp_offset[i] << FixPixelUnitScaleBits));
    }

    for (i = 0; i < MaxYorUVChannels; i++)
    {
        comp_init_phase_y[i] = overall_comp_init_phase_y[i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::derive_init_phase_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::derive_init_phase_for_qseed3(
    INT32* comp_init_phase_x,
    INT32* comp_init_phase_y,
    INT32* i32InitPhaseX,
    INT32* i32InitPhaseY)
{
    INT8  i;
    for (i = 0; i < MaxYorUVChannels; i++ )
    {
        i32InitPhaseX[i] = comp_init_phase_x[i];
        i32InitPhaseY[i] = comp_init_phase_y[i];
    }

    for (i = 0; i < MaxYorUVChannels; i++ )
    {
        while (i32InitPhaseX[i] < 0)
        {
            i32InitPhaseX[i] += static_cast<INT32>(FixPixelUnitScale);
        }

        while (i32InitPhaseX[i] >= static_cast<INT32>(FixPixelUnitScale))
        {
            i32InitPhaseX[i] -= static_cast<INT32>(FixPixelUnitScale);
        }

        while (i32InitPhaseY[i] < 0)
        {
            i32InitPhaseY[i] += static_cast<INT32>(FixPixelUnitScale);
        }

        while (i32InitPhaseY[i] >= static_cast<INT32>(FixPixelUnitScale))
        {
            i32InitPhaseY[i] -= static_cast<INT32>(FixPixelUnitScale);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_num_extended_pels_core_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_num_extended_pels_core_for_qseed3(
    INT32  extra,
    INT32* num_extended_pels,
    BOOL   end_ind)
{
    UINT32 extra_temp;

    if (extra == 0)
    {
        *num_extended_pels = 0;
        if (end_ind)
        {
            *num_extended_pels = 1;
        }
    }
    else if (extra > 0)
    {
        // drop pixels
        extra_temp         = static_cast<UINT32>(extra);
        *num_extended_pels = static_cast<INT32>(extra_temp >> FixPixelUnitScaleBits);
        *num_extended_pels = -(*num_extended_pels);
    }
    else
    {
        // padding pixels
        extra_temp         = static_cast<UINT32>(-extra);
        *num_extended_pels = static_cast<INT32>(extra_temp >> FixPixelUnitScaleBits);
        if (((*num_extended_pels) << FixPixelUnitScaleBits) != static_cast<INT32>(extra_temp))
        {
            (*num_extended_pels)++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::clean_frac
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 TwoDUpscaleMISCS::clean_frac(
    INT32 in)
{
    INT32 out;
    INT64 temp;
    INT32 MAX_SHIFT_BIT = 31;

    if (in < 0)
    {
        temp = static_cast<INT64>(in) + (static_cast<INT64>(1) << MAX_SHIFT_BIT);
        temp = temp & 0xFFFF8000;
        temp = temp - (static_cast<INT64>(1) << MAX_SHIFT_BIT);
    }
    else
    {
        temp = static_cast<INT64>(in) & 0xFFFF8000;
    }

    out = static_cast<INT32>(temp);

    return out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_num_extended_pels_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_num_extended_pels_for_qseed3(
    UINT32  enable,
    UINT32* comp_in_width,
    UINT32* comp_in_height,
    UINT32* comp_out_width,
    UINT32* comp_out_height,
    UINT32* comp_phase_step_x,
    UINT32* comp_phase_step_y,
    INT32*  comp_init_phase_x,
    INT32*  comp_init_phase_y,
    INT32*  comp_num_left_extended_pels,
    INT32*  comp_num_right_extended_pels,
    INT32*  comp_num_top_extended_pels,
    INT32*  comp_num_bot_extended_pels)
{
    UINT32 i;
    UINT32 num_taps_x;
    UINT32 num_taps_y;
    INT64  left;
    INT64  right;
    INT64  top;
    INT64  bot;

    if (FALSE == enable)
    {
        for (i = 0; i < MaxYorUVChannels; i++)
        {
            comp_num_left_extended_pels[i]  = 0;
            comp_num_right_extended_pels[i] = 0;
            comp_num_top_extended_pels[i]   = 0;
            comp_num_bot_extended_pels[i]   = 0;
        }
    }
    else
    {
        for (i = 0; i < MaxYorUVChannels; i++)
        {
            num_taps_x = 2; // 2=(6/2-1) 6tap 2d

            left       = clean_frac(comp_init_phase_x[i]) - static_cast<INT32>(num_taps_x) * FixPixelUnitScale;

            cal_num_extended_pels_core_for_qseed3(static_cast<INT32>(left),
                                                  &comp_num_left_extended_pels[i],
                                                  0);
            UINT32 compOutWidthMinusOne = (comp_out_width[i] >= 1) ? (comp_out_width[i] - 1) : 0;
            UINT32 compInWidthMinusOne  = (comp_in_width[i]  >= 1) ? (comp_in_width[i]  - 1) : 0;
            INT64  compOutWidthTimesCompPhaseStepx   = static_cast<INT64>(compOutWidthMinusOne) * comp_phase_step_x[i];
            INT64  compInWidthTimesFixPixelUnitScale = static_cast<INT64>(compInWidthMinusOne) * FixPixelUnitScale;
            INT64  numTapsxTimesFixPixelUnitScale    = static_cast<INT64>(num_taps_x * FixPixelUnitScale);

            right      = clean_frac(comp_init_phase_x[i]) + compOutWidthTimesCompPhaseStepx -
                compInWidthTimesFixPixelUnitScale + numTapsxTimesFixPixelUnitScale;
            right      = -right;

            cal_num_extended_pels_core_for_qseed3(static_cast<INT32>(right),
                                                  &comp_num_right_extended_pels[i],
                                                  1);

            num_taps_y = 1; // 1=(4/2-1) 4tap 2d
            top        = clean_frac(comp_init_phase_y[i]) - static_cast<INT32>(num_taps_y) * FixPixelUnitScale;

            cal_num_extended_pels_core_for_qseed3(static_cast<INT32>(top),
                                                  &comp_num_top_extended_pels[i],
                                                  0);

            // +1 residual already results in one pixel padding.
            UINT32 compOutHeightMinusOne = (comp_out_height[i] >= 1) ? (comp_out_height[i] - 1) : 0;
            UINT32 compInHeightMinusOne  = (comp_in_height[i]  >= 1) ? (comp_in_height[i]  - 1) : 0;
            INT64  compOutHeightTimesCompPhaseStepy   = static_cast<INT64>(compOutHeightMinusOne) * comp_phase_step_y[i];
            INT64  compInHeightTimesFixPixelUnitScale = static_cast<INT64>(compInHeightMinusOne) * FixPixelUnitScale;
            INT64  numTapsyTimesFixPixelUnitScale     = static_cast<INT64>(num_taps_y) * FixPixelUnitScale;

            bot        = clean_frac(comp_init_phase_y[i]) + compOutHeightTimesCompPhaseStepy -
              compInHeightTimesFixPixelUnitScale + numTapsyTimesFixPixelUnitScale;
            bot        = -bot;

            cal_num_extended_pels_core_for_qseed3(static_cast<INT32>(bot),
                                                  &comp_num_bot_extended_pels[i],
                                                  1);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_preload_pels_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_preload_pels_for_qseed3(
    INT32*  pCompNumLeftExtendedPels,
    INT32*  pCompNumRightExtendedPels,
    INT32*  pCompNumTopExtendedPels,
    INT32*  pCompNumBotExtendedPels,
    UINT32  phase_step_y_h,
    UINT32  phase_step_y_v,
    UINT8   repeatOnly,
    UINT32* pCompInHorOffset,
    UINT32* pCompInVerOffset,
    UINT32* pCompSurInWidth,
    UINT32* pCompSurInHeight,
    UINT32* pCompROIInWidth,
    UINT32* pCompROIInHeight,
    UINT32  outWidth,
    UINT32  outHeight,
    UINT8*  pYPreloadH,
    UINT8*  pYPreloadV,
    UINT8*  pUVPreloadH,
    UINT8*  pUVPreloadV,
    UINT32* pInputROIWidth,
    UINT32* pInputROIHeight,
    UINT32* pInputROIHorOffset,
    UINT32* pInputROIVerOffset)
{
    // unreference these parameters to avoid compilation warning
    // keep these parameters for easy syncing up with PCSIM file
    CAMX_UNREFERENCED_PARAM(phase_step_y_h);
    CAMX_UNREFERENCED_PARAM(phase_step_y_v);
    CAMX_UNREFERENCED_PARAM(outWidth);
    CAMX_UNREFERENCED_PARAM(outHeight);

    INT8  i;
    INT32 leftPERequired;
    INT32 rightPERequired;
    INT32 topPERequired;
    INT32 botPERequired;
    INT32 leftPEOverfetch[MaxYorUVChannels];
    INT32 rightPEOverfetch[MaxYorUVChannels];
    INT32 topPEOverfetch[MaxYorUVChannels];
    INT32 botPEOverfetch[MaxYorUVChannels];
    INT32 RAU_alignment_h[MaxYorUVChannels] = { 0, 0 };// preloaded pixels due to RAU alignment;
    INT32 RAU_alignment_v[MaxYorUVChannels] = { 0, 0 };
    UINT8 leftRepeat;
    UINT8 rightRepeat;
    UINT8 topRepeat;
    UINT8 botRepeat;

    topRepeat   = repeatOnly & FORCE_REPEAT_TOP;
    leftRepeat  = repeatOnly & FORCE_REPEAT_LEFT;
    botRepeat   = repeatOnly & FORCE_REPEAT_BOTTOM;
    rightRepeat = repeatOnly & FORCE_REPEAT_RIGHT;

    for (i = 0; i < MaxYorUVChannels; i++)
    {
        leftPERequired      = pCompNumLeftExtendedPels[i];
        rightPERequired     = pCompNumRightExtendedPels[i];
        topPERequired       = pCompNumTopExtendedPels[i];
        botPERequired       = pCompNumBotExtendedPels[i];
        leftPEOverfetch[i]  = 0;
        rightPEOverfetch[i] = 0;
        topPEOverfetch[i]   = 0;
        botPEOverfetch[i]   = 0;

        if (leftRepeat != FORCE_REPEAT_LEFT) // 1: overfetch, 0: pixel repeat
        {
            INT32 temp         = static_cast<INT32>(pCompInHorOffset[i]) - leftPERequired;
            leftPEOverfetch[i] = (temp > 0) ? leftPERequired : static_cast<INT32>(pCompInHorOffset[i]);
        }

        if (rightRepeat != FORCE_REPEAT_RIGHT)
        {
            INT32 right_offset  = static_cast<INT32>(pCompSurInWidth[i] - pCompInHorOffset[i] - pCompROIInWidth[i]);
            INT32 temp          = right_offset - rightPERequired;
            rightPEOverfetch[i] = (temp > 0) ? rightPERequired : right_offset;
        }

        if (topRepeat != FORCE_REPEAT_TOP)
        {
            INT32 temp        = static_cast<INT32>(pCompInVerOffset[i]) - topPERequired;
            topPEOverfetch[i] = (temp > 0) ? topPERequired : static_cast<INT32>(pCompInVerOffset[i]);
        }

        if (botRepeat != FORCE_REPEAT_BOTTOM)
        {
            INT32 bot_offset  = static_cast<INT32>(pCompSurInHeight[i] - pCompInVerOffset[i] - pCompROIInHeight[i]);
            INT32 temp        = bot_offset - botPERequired;
            botPEOverfetch[i] = (temp > 0) ? botPERequired : bot_offset;
        }
    }

    *pYPreloadH  = static_cast<UINT8>(
        RAU_alignment_h[0] + leftPEOverfetch[0]- pCompNumLeftExtendedPels[0] +
        static_cast<INT32>(HorizontalNum2DFilterTaps) + 2); // alpha will be y_preload_h - 2
    *pYPreloadV  = static_cast<UINT8>(
        RAU_alignment_v[0] + topPEOverfetch[0] - pCompNumTopExtendedPels[0] +
        static_cast<INT32>(VerticalNum2DFilterTaps)); //alpha will be y_preload_v - 1
    *pUVPreloadH = static_cast<UINT8>(
        RAU_alignment_h[1] + leftPEOverfetch[1]- pCompNumLeftExtendedPels[1] +
        static_cast<INT32>(HorizontalNum2DFilterTaps) + 2);
    *pUVPreloadV = static_cast<UINT8>(
        RAU_alignment_v[1] + topPEOverfetch[1] - pCompNumTopExtendedPels[1] +
        static_cast<INT32>(VerticalNum2DFilterTaps));

    for (i = 0; i < MaxYorUVChannels; i++ )
    {
        pInputROIWidth[i]     = pCompROIInWidth[i]  + leftPEOverfetch[i] + rightPEOverfetch[i];
        pInputROIHeight[i]    = pCompROIInHeight[i] + topPEOverfetch[i]  + botPEOverfetch[i];
        pInputROIHorOffset[i] = pCompInHorOffset[i] - leftPEOverfetch[i];
        pInputROIVerOffset[i] = pCompInVerOffset[i] - topPEOverfetch[i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::range_check
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT8 TwoDUpscaleMISCS::range_check(
    INT16* CurveA0,
    INT16* CurveB0,
    INT16* CurveC0,
    INT16* CurveA1,
    INT16* CurveB1,
    INT16* CurveC1,
    INT16* CurveA2,
    INT16* CurveB2,
    INT16* CurveC2)
{
    INT8 a_ind = ((*CurveA0 > CurveA0Max) || (*CurveA0 < CurveA0Min) ||
                  (*CurveA1 > CurveA1Max) || (*CurveA1 < CurveA1Min) ||
                  (*CurveA2 > CurveA2Max) || (*CurveA2 < CurveA2Min));
    INT8 b_ind = ((*CurveB0 > CurveB0Max) || (*CurveB0 < CurveB0Min) ||
                  (*CurveB1 > CurveB1Max) || (*CurveB1 < CurveB1Min) ||
                  (*CurveB2 > CurveB2Max) || (*CurveB2 < CurveB2Min));
    INT8 c_ind = ((*CurveC0 > CurveC0Max) || (*CurveC0 < CurveC0Min) ||
                  (*CurveC1 > CurveC1Max) || (*CurveC1 < CurveC1Min) ||
                  (*CurveC2 > CurveC2Max) || (*CurveC2 < CurveC2Min));

    return a_ind + b_ind + c_ind;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_de_curve_core
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_de_curve_core(
    UINT16 T1,
    UINT16 T2,
    UINT16 Tq,
    UINT16 Td,
    UINT8  Nbits,
    INT16* CurveA0,
    INT16* CurveB0,
    INT16* CurveC0,
    INT16* CurveA1,
    INT16* CurveB1,
    INT16* CurveC1,
    INT16* CurveA2,
    INT16* CurveB2,
    INT16* CurveC2)
{
    // passing points (0,0) and (T1-Tq, T1).
    // Later after shifting to the right by Tq, it'll pass (Tq, 0) and (T1,T1).
    // -B/2A<=0, choose B=0.
    // ThrshldT1 >= ThrshldTq.
    // ThrshldT1 - ThrshldTq > 20 if ThrshldT1 > ThrshldTq
    // ThrshldTd - ThrshldT2 > 200 if  ThrshldTd > ThrshldT2
    // ThrshldT1 < 100
    if (T1 > Tq)
    {
        *CurveA0 = (T1 << Nbits ) / ((T1 - Tq) * (T1 - Tq));
        *CurveB0 = 0; //( T1 << ( Nbits - 8 ))/((T1 -Tq) * ( T1 - Tq + 1 ));
        *CurveC0 = 0;
    }
    else
    {
        *CurveA0 = 0;
        *CurveB0 = 0;
        *CurveC0 = 0;
    }

    *CurveA1 = 0;
    *CurveB1 = 1 << (Nbits - 8);
    *CurveC1 = 0;

    if (Td > T2)
    {
        *CurveA2 = -( T2 << Nbits ) / (( static_cast<INT32>(Td) - T2 ) * (static_cast<INT32>(Td) - T2 ));
        *CurveB2 = (( 2 * static_cast<INT64>(T2) * T2 ) << ( Nbits - 8 )) /
            ((static_cast<INT32>(Td) - T2 ) * (static_cast<INT32>(Td) - T2 ));
        *CurveC2 = T2 - (( (static_cast<INT32>(T2) * T2 * T2 )) /
            ((static_cast<INT32>(Td) - T2 ) * ( static_cast<INT32>(Td) - T2 ))); // T2(1-(T2^2)/((Td-T2)^2))
    }
    else
    {
        *CurveA2 = 0;
        *CurveB2 = 0;
        *CurveC2 = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::cal_de_curve_for_qseed3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::cal_de_curve_for_qseed3(
    UINT16 ThrshldT1, // input
    UINT16 ThrshldT2, // input
    UINT16 ThrshldTq, // input
    UINT16 ThrshldTd, // input
    UINT8* PrecBitN,  // output
    INT16* CurveA0,   // output
    INT16* CurveB0,   // output
    INT16* CurveC0,   // output
    INT16* CurveA1,   // output
    INT16* CurveB1,   // output
    INT16* CurveC1,   // output
    INT16* CurveA2,   // output
    INT16* CurveB2,   // output
    INT16* CurveC2)   // output
{
    // curve section 1: A0*x^2 + B0*x + C0=y//passing dots(0,0) and(T1,T1);
    // curve section 2: A1*x^2 + B1*x + C1=y;//passing dots (T1,T1) (T2,T2);
    // curve section 3: A2*x^2 + B2*x + C2=y; //passing dots (T2,T2) (1023, 0);
    UINT8  Nbits     = 12;
    UINT8  range_ind = 1;
    UINT16 T1        = ThrshldT1;
    UINT16 T2        = ThrshldT2;
    UINT16 Tq        = ThrshldTq;
    UINT16 Td        = ThrshldTd;

    while ((range_ind > 0) && (Nbits >= 8))
    {
         cal_de_curve_core(T1, T2, Tq, Td, Nbits, CurveA0, CurveB0, CurveC0, CurveA1,
                           CurveB1, CurveC1, CurveA2, CurveB2, CurveC2);
         range_ind = range_check(CurveA0, CurveB0, CurveC0, CurveA1, CurveB1, CurveC1,
                                 CurveA2, CurveB2, CurveC2);
         Nbits    -= 1;
    }

    *PrecBitN = Nbits + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::detCircVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 TwoDUpscaleMISCS::detCircVal(
    UINT32 phase_step,
    UINT8 blurry_level)
{
    if (blurry_level > 0)
    {
        return blurry_level;
    }
    else
    {
        if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 2)))
        {
            // 1.25x FIR_PIXEL_UNIT_SCALE
            return  0;
        }
        else if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 1)))
        {
            // 1.5x FIR_PIXEL_UNIT_SCALE
            return 1;
        }
        else if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 1) + (FixPixelUnitScale >> 2)))
        {
            // 1.75x FIR_PIXEL_UNIT_SCALE
            return 2;
        }
        else if (phase_step < (FixPixelUnitScale << 1))
        {
            // 2.0x FIR_PIXEL_UNIT_SCALE
            return 3;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale >> 1)))
        {
            // 2.5 FIR_PIXEL_UNIT_SCALE
            return 4;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale)))
        {
            // 3.0x FIR_PIXEL_UNIT_SCALE
            return 5;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale)+(FixPixelUnitScale >> 1)))
        {
            // 3.5x FIR_PIXEL_UNIT_SCALE
            return 6;
        }
        else if (phase_step < (FixPixelUnitScale << 2))
        {
            // 4.0x FIR_PIXEL_UNIT_SCALE
            return 7;
        }
        else
        {
            // greater than or equal to 4.0x FIR_PIXEL_UNIT_SCALE
            return 8;
        }
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::detSepVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 TwoDUpscaleMISCS::detSepVal(
    UINT32 phase_step,
    UINT8 blurry_level)
{
    if (blurry_level > 0)
    {
        return blurry_level;
    }
    else
    {
        if (phase_step < FixPixelUnitScale)
        {
            // 1.0x FIR_PIXEL_UNIT_SCALE
            return  0;
        }
        else if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 2)))
        {
            // 1.25x FIR_PIXEL_UNIT_SCALE
            return  1;
        }
        else if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 1)))
        {
            // 1.5x FIR_PIXEL_UNIT_SCALE
            return 2;
        }
        else if (phase_step < (FixPixelUnitScale + (FixPixelUnitScale >> 1) + (FixPixelUnitScale >> 2)))
        {
            // 1.75x FIR_PIXEL_UNIT_SCALE
            return 3;
        }
        else if (phase_step < (FixPixelUnitScale << 1))
        {
            // 2.0x FIR_PIXEL_UNIT_SCALE
            return 4;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale >> 1)))
        {
            // 2.5x FIR_PIXEL_UNIT_SCALE
            return 5;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale)))
        {
            // 3.0x FIR_PIXEL_UNIT_SCALE
            return 6;
        }
        else if (phase_step < ((FixPixelUnitScale << 1) + (FixPixelUnitScale)+(FixPixelUnitScale >> 1)))
        {
            // 3.5x FIR_PIXEL_UNIT_SCALE
            return 7;
        }
        else if (phase_step < (FixPixelUnitScale << 2))
        {
            // 4.0x FIR_PIXEL_UNIT_SCALE
            return 8;
        }
        else
        {
            // greater than or equal to 4.0x FIR_PIXEL_UNIT_SCALE
            return 9;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::DetFlags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::DetFlags(
    UpdateIn* pInParams,
    UINT8*    enable_dir_wr,
    UINT8*    enable_y_cir_wr,
    UINT8*    enable_uv_cir_wr,
    UINT8*    enable_y_sep_wr,
    UINT8*    enable_uv_sep_wr,
    UINT8*    y_cir_lut_value,
    UINT8*    y_sep_lut_value,
    UINT8*    uv_cir_lut_value,
    UINT8*    uv_sep_lut_value)
{
    *enable_dir_wr    = 0;
    *enable_y_cir_wr  = 0;
    *enable_uv_cir_wr = 0;
    *enable_y_sep_wr  = 0;
    *enable_uv_sep_wr = 0;
    *y_cir_lut_value  = 0;
    *y_sep_lut_value  = 0;
    *uv_cir_lut_value = 0;
    *uv_sep_lut_value = 0;

    UINT32 curStepY  = IQSettingUtils::MaxUINT32((pInParams->curPhaseStepHor[0]), (pInParams->curPhaseStepVer[0]));
    UINT32 curStepUV = IQSettingUtils::MaxUINT32((pInParams->curPhaseStepHor[1]), (pInParams->curPhaseStepVer[1]));

    if (TRUE == pInParams->isInitProgram)
    {
        *enable_dir_wr = 1;

        if ((pInParams->curBlndCfg == 0) && (pInParams->curYCfg == 0))
        {
            *enable_y_cir_wr = 1;
            *y_cir_lut_value = 0;
        }
        else if ((pInParams->curBlndCfg == 1) && (pInParams->curYCfg == 0))
        {
            *enable_y_sep_wr = 1;
            *y_sep_lut_value = 0;
        }

        if (pInParams->curYCfg == 1)
        {
            *enable_y_cir_wr = 1;
            *y_cir_lut_value = detCircVal(curStepY, pInParams->blurry_level);
        }
        else if (pInParams->curYCfg == 2)
        {
            *enable_y_sep_wr = 1;
            *y_sep_lut_value = detSepVal(curStepY, pInParams->blurry_level);
        }

        if((pInParams->curColorSpace == 1 ) && (pInParams->curUVCfg != 0)) //yuv format
        {
            if(pInParams->curUVCfg == 1)
            {
                *enable_uv_cir_wr = 1;
                *uv_cir_lut_value = detCircVal(curStepUV, pInParams->blurry_level);
            }
            else
            {
                *enable_uv_sep_wr = 1;
                *uv_sep_lut_value = detSepVal(curStepUV, pInParams->blurry_level);
            }
        }
    }
    else
    {
        UINT32 prevStepY   = IQSettingUtils::MaxUINT32((pInParams->prevPhaseStepHor[0]), (pInParams->prevPhaseStepVer[0]));
        UINT32 prevStepUV  = IQSettingUtils::MaxUINT32((pInParams->prevPhaseStepHor[1]), (pInParams->prevPhaseStepVer[1]));

        // partial update doesn't support non-zero blurry levels. As no previous blrry level info is passed to the current setting.
        UINT8 blurry_level = 0;

        if (((pInParams->curYCfg == 0) && (pInParams->prevYCfg != 0)))
        {
            *enable_dir_wr = 1;
            if ((pInParams->curBlndCfg == 0) && (pInParams->prevBlndCfg != 0))
            {
                *enable_y_cir_wr = 1;
                *y_cir_lut_value = 0;
            }
            else if ((pInParams->curBlndCfg == 1) && (pInParams->prevBlndCfg != 1))
            {
                *enable_y_sep_wr = 1;
                *y_sep_lut_value = 0;
            }
        }
        else if (pInParams->curYCfg == 1)
        {
            if(pInParams->prevYCfg != 1)
            {
                *enable_y_cir_wr = 1;
                *y_cir_lut_value = detCircVal(curStepY, blurry_level);
            }
            else
            {
                INT8 prev_val;
                INT8 cur_val;

                cur_val  = detCircVal(curStepY, blurry_level );
                prev_val = detCircVal(prevStepY, blurry_level );

                if(cur_val != prev_val)
                {
                    *enable_y_cir_wr = 1;
                    *y_cir_lut_value = cur_val;
                }
            }
        }
        else if (pInParams->curYCfg == 2)
        {
            if(pInParams->prevYCfg != 2)
            {
                *enable_y_sep_wr = 1;
                *y_sep_lut_value = detSepVal(curStepY, blurry_level );
            }
            else
            {
                INT8 prev_val;
                INT8 cur_val;

                cur_val  = detSepVal(curStepY, blurry_level );
                prev_val = detSepVal(prevStepY, blurry_level );

                if(cur_val != prev_val)
                {
                    *enable_y_sep_wr = 1;
                    *y_sep_lut_value = cur_val;
                }
            }
        }

        if((pInParams->curColorSpace == 1 ) && (pInParams->curUVCfg != 0)) //yuv format
        {
            if(pInParams->prevColorSpace == 1)
            {
                if(pInParams->curUVCfg == 1)
                {
                    if(pInParams->prevUVCfg != 1)
                    {
                        *enable_uv_cir_wr = 1;
                        *uv_cir_lut_value = detCircVal(curStepUV, blurry_level);
                    }
                    else
                    {
                        INT8 prev_val;
                        INT8 cur_val;

                        cur_val  = detCircVal(curStepUV, blurry_level);
                        prev_val = detCircVal(prevStepUV, blurry_level);

                        if(cur_val != prev_val)
                        {
                            *enable_uv_cir_wr = 1;
                            *uv_cir_lut_value = cur_val;
                        }
                    }
                }
                else
                {
                    // curUVCfg==2
                    if(pInParams->prevUVCfg != 2)
                    {
                        *enable_uv_sep_wr = 1;
                        *uv_sep_lut_value = detSepVal(curStepUV, blurry_level);
                    }
                    else
                    {
                        INT8 prev_val;
                        INT8 cur_val;

                        cur_val  = detSepVal(curStepUV, blurry_level);
                        prev_val = detSepVal(prevStepUV, blurry_level);

                        if(cur_val != prev_val)
                        {
                            *enable_uv_sep_wr = 1;
                            *uv_sep_lut_value = cur_val;
                        }
                    }
                }
            }
            else // pInParams->prevColorSpace==0
            {
                if(pInParams->curUVCfg == 1)
                {
                    *enable_uv_cir_wr = 1;
                    *uv_cir_lut_value = detCircVal(curStepUV, blurry_level);
                }
                else
                {
                    *enable_uv_sep_wr = 2;
                    *uv_sep_lut_value = detSepVal(curStepUV, blurry_level);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::loadCircSepLUTs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::loadCircSepLUTs(
    UpdateIn* pInParams,
    UINT32    pLUTs[][60],
    INT16     offsetA,
    INT16     offsetBC,
    INT16     offsetD,
    INT16     nSet)
{
    INT8   i;
    INT8   j;
    INT8   lengA      = 6;
    INT8   lengBCD    = 3;
    UINT16 LUTBOffset = 24;
    UINT16 LUTCOffset = LUTBOffset + 12;
    UINT16 LUTDOffset = LUTCOffset + 12;

    for (i = 0; i < lengA; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pInParams->pTwoDFilterA[CoeffLUTSizeA * j + offsetA + i] = pLUTs[nSet][4 * i + j];
        }
    }

    for (i = 0; i < lengBCD; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pInParams->pTwoDFilterB[CoeffLUTSizeB * j + offsetBC + i ] = pLUTs[nSet][LUTBOffset + 4 * i + j];
            pInParams->pTwoDFilterC[CoeffLUTSizeC * j + offsetBC + i ] = pLUTs[nSet][LUTCOffset + 4 * i + j];
        }
    }

    for (i = 0; i < lengBCD; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pInParams->pTwoDFilterD[CoeffLUTSizeD * j + offsetD + i] = pLUTs[nSet][LUTDOffset + 4 * i + j];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::CoeffLUTsUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::CoeffLUTsUpdate(
    UpdateIn*  pInParams,
    UpdateOut* pOutParams)
{
    INT8  i;
    INT8  j;
    INT8  dirLengA       = 9;
    INT8  dirLengBC      = 6;
    INT8  dirLengD       = 4;
    INT8  circSepLengA   = 6;
    INT8  circSepLengBCD = 3;

    UINT8 enable_dir_wr;
    UINT8 enable_y_cir_wr;
    UINT8 enable_uv_cir_wr;
    UINT8 enable_y_sep_wr;
    UINT8 enable_uv_sep_wr;
    UINT8 y_cir_lut_value;
    UINT8 y_sep_lut_value;
    UINT8 uv_cir_lut_value;
    UINT8 uv_sep_lut_value;

    DetFlags(pInParams, &enable_dir_wr, &enable_y_cir_wr, &enable_uv_cir_wr,
             &enable_y_sep_wr, &enable_uv_sep_wr, &y_cir_lut_value, &y_sep_lut_value,
             &uv_cir_lut_value, &uv_sep_lut_value);

    pOutParams->enable_dir_wr    = enable_dir_wr;
    pOutParams->enable_y_cir_wr  = enable_y_cir_wr;
    pOutParams->enable_y_sep_wr  = enable_y_sep_wr;
    pOutParams->enable_uv_cir_wr = enable_uv_cir_wr;
    pOutParams->enable_uv_sep_wr = enable_uv_sep_wr;
    pOutParams->y_cir_lut_value  = y_cir_lut_value;
    pOutParams->y_sep_lut_value  = y_sep_lut_value;
    pOutParams->uv_cir_lut_value = uv_cir_lut_value;
    pOutParams->uv_sep_lut_value = uv_sep_lut_value;

    // initialize lut pointers to null
    pOutParams->dir_lut     = NULL;
    pOutParams->y_circ_lut  = NULL;
    pOutParams->y_sep_lut   = NULL;
    pOutParams->uv_circ_lut = NULL;
    pOutParams->uv_sep_lut  = NULL;

    if (enable_dir_wr == 1)
    {
        UINT16 LUTBOffset = 36 * 2;
        UINT16 LUTCOffset = LUTBOffset + 24 * 2;
        UINT16 LUTDOffset = LUTCOffset + 24 * 2;

        for (i = 0; i < dirLengA * 2; i++)
        {
            for (j = 0; j < 4; j++)
            {
                pInParams->pTwoDFilterA[CoeffLUTSizeA * j + i] = dirLUTs[4 * i + j];
            }
        }

        for (i = 0; i < dirLengBC * 2; i++)
        {
            for (j = 0; j < 4; j++)
            {
                pInParams->pTwoDFilterB[ CoeffLUTSizeB * j + i ] = dirLUTs[ LUTBOffset + 4 * i + j];
                pInParams->pTwoDFilterC[ CoeffLUTSizeC * j + i ] = dirLUTs[ LUTCOffset + 4 * i + j];
            }
        }

        for (i = 0; i < dirLengD * 2; i++)
        {
            for (j = 0; j < 4; j++)
            {
                pInParams->pTwoDFilterD[CoeffLUTSizeD * j + i] = dirLUTs[LUTDOffset + 4 * i + j];
            }
        }

        pOutParams->dir_lut = &dirLUTs[0];
    }

    if (enable_y_cir_wr == 1)
    {
        INT16 yCircOffsetA  = (dirLengA  << 1);
        INT16 yCircOffsetBC = (dirLengBC << 1);
        INT16 yCircOffsetD  = (dirLengD  << 1);

        loadCircSepLUTs(pInParams, circLUTs, yCircOffsetA, yCircOffsetBC,
                        yCircOffsetD, y_cir_lut_value);

        pOutParams->y_circ_lut = circLUTs[y_cir_lut_value];
    }

    if (enable_y_sep_wr == 1)
    {
        INT16 ySepcOffsetA  = (dirLengA  << 1) + circSepLengA;
        INT16 ySepcOffsetBC = (dirLengBC << 1) + circSepLengBCD;
        INT16 ySepcOffsetD  = (dirLengD  << 1) + circSepLengBCD;

        loadCircSepLUTs(pInParams, sepLUTs, ySepcOffsetA, ySepcOffsetBC,
                        ySepcOffsetD, y_sep_lut_value);

        pOutParams->y_sep_lut = sepLUTs[y_sep_lut_value];
    }

    if (enable_uv_cir_wr == 1)
    {
        INT16 uvCircOffsetA  = (dirLengA  + circSepLengA)   * 2;
        INT16 uvCircOffsetBC = (dirLengBC + circSepLengBCD) * 2;
        INT16 uvCircOffsetD  = (dirLengD  + circSepLengBCD) * 2;

        loadCircSepLUTs(pInParams, circLUTs, uvCircOffsetA, uvCircOffsetBC,
                        uvCircOffsetD, uv_cir_lut_value);

        pOutParams->uv_circ_lut = circLUTs[uv_cir_lut_value];
    }

    if (enable_uv_sep_wr == 1)
    {
        INT16 uvSepcOffsetA  = dirLengA  * 2 + circSepLengA   * 3;
        INT16 uvSepcOffsetBC = dirLengBC * 2 + circSepLengBCD * 3;
        INT16 uvSepcOffsetD  = dirLengD  * 2 + circSepLengBCD * 3;

        loadCircSepLUTs(pInParams, sepLUTs, uvSepcOffsetA, uvSepcOffsetBC,
                        uvSepcOffsetD, uv_sep_lut_value);

        pOutParams->uv_sep_lut = circLUTs[uv_sep_lut_value];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TwoDUpscaleMISCS::StripingForTwoDScale
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TwoDUpscaleMISCS::StripingForTwoDScale(
    SurfaceInfo Surface,
    StripeCfg   pStripe[])
{
    INT32  i;
    INT32  c;
    UINT32 comp_scale_ratio_x[MaxYorUVChannels];

    for (i = 0; i < MaxYorUVChannels; i++ )
    {
        DOUBLE temp = static_cast<DOUBLE>(Surface.surf_comp_output_width[i] << FixPixelUnitScaleBits) /
            static_cast<DOUBLE>(Surface.surf_comp_input_width[i]);
        comp_scale_ratio_x[i] = static_cast<UINT32>(temp);
    }

    INT32 nStripe = ( Surface.surf_comp_input_width[0] + Surface.stripe_comp_width[0] - 1 ) /
        Surface.stripe_comp_width[0];
    for (i = 0; i < nStripe - 1; i++ )
    {
        pStripe[i].stripe_comp_in_width[0] = Surface.stripe_comp_width[0];
        pStripe[i].stripe_comp_in_width[1] = Surface.stripe_comp_width[1];
    }

    pStripe[nStripe -1].stripe_comp_in_width[0] = Surface.surf_comp_input_width[0] -
        Surface.stripe_comp_width[0] * (nStripe - 1);
    pStripe[nStripe -1].stripe_comp_in_width[1] = Surface.surf_comp_input_width[1] -
        Surface.stripe_comp_width[1] * (nStripe - 1);

    //output width calculation
    UINT32 prev_out_w = 0;
    UINT32 prev_in_w[MaxYorUVChannels] = {0, 0};
    UINT32 temp_in_w[MaxYorUVChannels];

    for (i=0; i < nStripe; i++)
    {
        for (c = 0; c < MaxYorUVChannels; c++)
        {
            pStripe[i].stripe_comp_in_offset_h[c] = prev_in_w[c];
            temp_in_w[c] = prev_in_w[c] + pStripe[c].stripe_comp_in_width[c];
            prev_in_w[c] = temp_in_w[c];
        }

        UINT32 temp_out_w = static_cast<UINT32>(
            (static_cast<INT64>(temp_in_w[0]) * comp_scale_ratio_x[0]) >> FixPixelUnitScaleBits);
        pStripe[i].stripe_comp_output_width[0] = temp_out_w - prev_out_w;
        pStripe[i].stripe_comp_out_offset_h[0] = prev_out_w;
        pStripe[i].stripe_comp_out_offset_h[1] = pStripe[i].stripe_comp_out_offset_h[0];
        prev_out_w                             = temp_out_w;
        pStripe[i].stripe_comp_output_width[1] = pStripe[i].stripe_comp_output_width[0];
    }

    UINT8  repeatOnly = 0;
    INT32  comp_init_phase_x[MaxYorUVChannels];
    INT32  comp_init_phase_y[MaxYorUVChannels];
    INT32  i32InitPhaseX[MaxYorUVChannels];
    INT32  i32InitPhaseY[MaxYorUVChannels];
    INT32  comp_num_left_extended_pels[MaxYorUVChannels];
    INT32  comp_num_right_extended_pels[MaxYorUVChannels];
    INT32  comp_num_top_extended_pels[MaxYorUVChannels];
    INT32  comp_num_bot_extended_pels[MaxYorUVChannels];
    UINT32 input_roi_width[MaxYorUVChannels];
    UINT32 input_roi_height[MaxYorUVChannels];
    UINT32 input_roi_hor_offset[MaxYorUVChannels];
    UINT32 input_roi_ver_offset[MaxYorUVChannels];

    for (i = 0; i < nStripe; i++)
    {
        cal_start_phase_common_qseed3(
            Surface.chroma_subsample_x_flg,
            Surface.chroma_subsample_y_flg,
            Surface.chroma_site_io,
            Surface.comp_phase_step_x,
            Surface.comp_phase_step_y,
            pStripe[i].stripe_comp_in_offset_h,
            pStripe[i].stripe_comp_out_offset_h,
            comp_init_phase_x,
            comp_init_phase_y);

        cal_num_extended_pels_for_qseed3(
            1, //enabling scaler
            pStripe[i].stripe_comp_in_width,
            Surface.surf_comp_input_height,
            pStripe[i].stripe_comp_output_width,
            Surface.surf_comp_output_height,
            Surface.comp_phase_step_x,
            Surface.comp_phase_step_y,
            comp_init_phase_x,
            comp_init_phase_y,
            comp_num_left_extended_pels,
            comp_num_right_extended_pels,
            comp_num_top_extended_pels,
            comp_num_bot_extended_pels);

        derive_init_phase_for_qseed3(
            comp_init_phase_x,
            comp_init_phase_y,
            i32InitPhaseX,
            i32InitPhaseY);

        pStripe[i].y_phase_init_h  = static_cast<UINT8>(i32InitPhaseX[0]);
        pStripe[i].uv_phase_init_h = static_cast<UINT8>(i32InitPhaseX[1]);
        pStripe[i].y_phase_init_v  = static_cast<UINT8>(i32InitPhaseY[0]);
        pStripe[i].uv_phase_init_v = static_cast<UINT8>(i32InitPhaseY[1]);

        cal_preload_pels_for_qseed3(
            comp_num_left_extended_pels,
            comp_num_right_extended_pels,
            comp_num_top_extended_pels,
            comp_num_bot_extended_pels,
            Surface.comp_phase_step_x[0], //phase_step_y_h,
            Surface.comp_phase_step_y[0],//phase_step_y_v,
            repeatOnly,
            pStripe[i].stripe_comp_in_offset_h,// comp_in_hor_offset,
            Surface.comp_in_offset_v, //comp_in_ver_offset,
            Surface.surf_comp_input_width, //comp_sur_in_width,
            Surface.surf_comp_input_height,//comp_sur_in_height,
            pStripe[i].stripe_comp_in_width,
            Surface.surf_comp_input_height,
            pStripe[i].stripe_comp_output_width[0],
            Surface.surf_comp_output_height[0],
            &pStripe[i].y_preload_h,
            &pStripe[i].y_preload_v,
            &pStripe[i].uv_preload_h,
            &pStripe[i].uv_preload_v,
            input_roi_width,
            input_roi_height,
            input_roi_hor_offset,
            input_roi_ver_offset);

        for (c = 0; c < MaxYorUVChannels; c++)
        {
            pStripe[i].required_comp_input_width[c] = input_roi_width[c];

            pStripe[i].overfetch_left[c]  = pStripe[i].stripe_comp_in_offset_h[c] - input_roi_hor_offset[c];
            pStripe[i].overfetch_right[c] = input_roi_width[c] - pStripe[i].overfetch_left[c] -
                pStripe[i].stripe_comp_in_width[c];
            pStripe[i].overfetch_top[c]   = Surface.comp_in_offset_v[c] -input_roi_ver_offset[c];
            pStripe[i].overfetch_bot[c]   = input_roi_height[c] - pStripe[i].overfetch_top[c] -
                Surface.surf_comp_input_height[c];
        }
    }
}
