// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tmc12interpolation.h
/// @brief TMC12 Tuning Data Interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TMC12INTERPOLATION_H
#define TMC12INTERPOLATION_H

#include "tmc_1_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const FLOAT  TMC12_UNIT_GAIN_STEP                    = 1.03f;
static const INT32  TMC12_DRC_INDEX_MIN                     = 94;
static const FLOAT  TMC12_GAIN_RATIO_MIN                    = 1.0f;
static const FLOAT  TMC12_GAIN_RATIO_MAX                    = 16.0f;
static const FLOAT  TMC12_DRC_GAIN_POW_X                    = 1.03f;
static const INT    TMC12_KNEE_POINTS_NUM                   = 5;
static const UINT32 TMC12_COEF_NUM                          = 9;
static const FLOAT  TMC12_MAX_LTM_GAIN                      = 8.0f;
static const UINT32 TMC12_HIST_BIN_NUM                      = 1024;
static const UINT32 TMC12_KNEE_MIN                          = 1;

static const FLOAT  TMC12_OVERRIDE_DARK_BOOST_OFFSET_MIN    = 0.0f;
static const FLOAT  TMC12_OVERRIDE_DARK_BOOST_OFFSET_MAX    = 1.0f;

// NOWHINE NC004c: Share code with system team
struct TMC12TriggerList
{
    tmc_1_2_0::chromatix_tmc12Type::control_methodStruct controlType;       ///< chromatix data
    FLOAT                                                triggerDRCgain;    ///< DRC Gain
    FLOAT                                                triggerHDRAEC;     ///< HDRAEC trigger
    FLOAT                                                triggerAEC;        ///< AEC trigger
};

// NOWHINE NC004c: Share code with system team
struct TMC12PerFrameInfo
{
    FLOAT   sceneChangeFlag;                        ///< Flag for scene change
    FLOAT   drcGainDark;                            ///< DRC dark gain
    FLOAT   numOfHistogram;                         ///< Total count of histogram
    FLOAT*  pProcessedHistogram;                    ///< pointer to processed histogram array
    FLOAT*  pKneeX;                                 ///< pointer to kneeX array
    FLOAT*  pKneeY;                                 ///< pointer to kneeY array
    FLOAT*  pPchipCoeffficient;                     ///< pointer to PCHIP coefficient array
    FLOAT*  pContrastEnhanceCurve;                  ///< pointer to contrast enhance curve array
    FLOAT*  pCalculatedCDF;                         ///< pointer to calculated CDF array

};

static const FLOAT log_bin_tmc12[TMC12_HIST_BIN_NUM] =
{
    0.6931f, 1.7918f, 2.3026f, 2.6391f, 2.8904f, 3.0910f, 3.2581f, 3.4012f, 3.5264f, 3.6376f, 3.7377f, 3.8286f,
    3.9120f, 3.9890f, 4.0604f, 4.1271f, 4.1897f, 4.2485f, 4.3041f, 4.3567f, 4.4067f, 4.4543f, 4.4998f, 4.5433f,
    4.5850f, 4.6250f, 4.6634f, 4.7005f, 4.7362f, 4.7707f, 4.8040f, 4.8363f, 4.8675f, 4.8978f, 4.9273f, 4.9558f,
    4.9836f, 5.0106f, 5.0370f, 5.0626f, 5.0876f, 5.1120f, 5.1358f, 5.1591f, 5.1818f, 5.2040f, 5.2257f, 5.2470f,
    5.2679f, 5.2883f, 5.3083f, 5.3279f, 5.3471f, 5.3660f, 5.3845f, 5.4027f, 5.4205f, 5.4381f, 5.4553f, 5.4723f,
    5.4889f, 5.5053f, 5.5215f, 5.5373f, 5.5530f, 5.5683f, 5.5835f, 5.5984f, 5.6131f, 5.6276f, 5.6419f, 5.6560f,
    5.6699f, 5.6836f, 5.6971f, 5.7104f, 5.7236f, 5.7366f, 5.7494f, 5.7621f, 5.7746f, 5.7869f, 5.7991f, 5.8111f,
    5.8230f, 5.8348f, 5.8464f, 5.8579f, 5.8693f, 5.8805f, 5.8916f, 5.9026f, 5.9135f, 5.9243f, 5.9349f, 5.9454f,
    5.9558f, 5.9661f, 5.9764f, 5.9865f, 5.9965f, 6.0064f, 6.0162f, 6.0259f, 6.0355f, 6.0450f, 6.0544f, 6.0638f,
    6.0730f, 6.0822f, 6.0913f, 6.1003f, 6.1092f, 6.1181f, 6.1269f, 6.1356f, 6.1442f, 6.1527f, 6.1612f, 6.1696f,
    6.1779f, 6.1862f, 6.1944f, 6.2025f, 6.2106f, 6.2186f, 6.2265f, 6.2344f, 6.2422f, 6.2500f, 6.2577f, 6.2653f,
    6.2729f, 6.2804f, 6.2879f, 6.2953f, 6.3026f, 6.3099f, 6.3172f, 6.3244f, 6.3315f, 6.3386f, 6.3456f, 6.3526f,
    6.3596f, 6.3665f, 6.3733f, 6.3801f, 6.3869f, 6.3936f, 6.4003f, 6.4069f, 6.4135f, 6.4200f, 6.4265f, 6.4329f,
    6.4394f, 6.4457f, 6.4520f, 6.4583f, 6.4646f, 6.4708f, 6.4770f, 6.4831f, 6.4892f, 6.4953f, 6.5013f, 6.5073f,
    6.5132f, 6.5191f, 6.5250f, 6.5309f, 6.5367f, 6.5425f, 6.5482f, 6.5539f, 6.5596f, 6.5653f, 6.5709f, 6.5765f,
    6.5820f, 6.5876f, 6.5930f, 6.5985f, 6.6039f, 6.6093f, 6.6147f, 6.6201f, 6.6254f, 6.6307f, 6.6359f, 6.6412f,
    6.6464f, 6.6516f, 6.6567f, 6.6619f, 6.6670f, 6.6720f, 6.6771f, 6.6821f, 6.6871f, 6.6921f, 6.6970f, 6.7020f,
    6.7069f, 6.7117f, 6.7166f, 6.7214f, 6.7262f, 6.7310f, 6.7358f, 6.7405f, 6.7452f, 6.7499f, 6.7546f, 6.7593f,
    6.7639f, 6.7685f, 6.7731f, 6.7776f, 6.7822f, 6.7867f, 6.7912f, 6.7957f, 6.8002f, 6.8046f, 6.8090f, 6.8134f,
    6.8178f, 6.8222f, 6.8265f, 6.8309f, 6.8352f, 6.8395f, 6.8437f, 6.8480f, 6.8522f, 6.8565f, 6.8607f, 6.8648f,
    6.8690f, 6.8732f, 6.8773f, 6.8814f, 6.8855f, 6.8896f, 6.8937f, 6.8977f, 6.9017f, 6.9058f, 6.9098f, 6.9137f,
    6.9177f, 6.9217f, 6.9256f, 6.9295f, 6.9334f, 6.9373f, 6.9412f, 6.9451f, 6.9489f, 6.9527f, 6.9565f, 6.9603f,
    6.9641f, 6.9679f, 6.9717f, 6.9754f, 6.9791f, 6.9829f, 6.9866f, 6.9903f, 6.9939f, 6.9976f, 7.0012f, 7.0049f,
    7.0085f, 7.0121f, 7.0157f, 7.0193f, 7.0229f, 7.0264f, 7.0300f, 7.0335f, 7.0370f, 7.0405f, 7.0440f, 7.0475f,
    7.0510f, 7.0544f, 7.0579f, 7.0613f, 7.0648f, 7.0682f, 7.0716f, 7.0750f, 7.0783f, 7.0817f, 7.0851f, 7.0884f,
    7.0917f, 7.0951f, 7.0984f, 7.1017f, 7.1050f, 7.1082f, 7.1115f, 7.1148f, 7.1180f, 7.1213f, 7.1245f, 7.1277f,
    7.1309f, 7.1341f, 7.1373f, 7.1405f, 7.1436f, 7.1468f, 7.1499f, 7.1531f, 7.1562f, 7.1593f, 7.1624f, 7.1655f,
    7.1686f, 7.1717f, 7.1747f, 7.1778f, 7.1808f, 7.1839f, 7.1869f, 7.1899f, 7.1929f, 7.1959f, 7.1989f, 7.2019f,
    7.2049f, 7.2079f, 7.2108f, 7.2138f, 7.2167f, 7.2196f, 7.2226f, 7.2255f, 7.2284f, 7.2313f, 7.2342f, 7.2371f,
    7.2399f, 7.2428f, 7.2457f, 7.2485f, 7.2513f, 7.2542f, 7.2570f, 7.2598f, 7.2626f, 7.2654f, 7.2682f, 7.2710f,
    7.2738f, 7.2766f, 7.2793f, 7.2821f, 7.2848f, 7.2876f, 7.2903f, 7.2930f, 7.2957f, 7.2984f, 7.3011f, 7.3038f,
    7.3065f, 7.3092f, 7.3119f, 7.3146f, 7.3172f, 7.3199f, 7.3225f, 7.3251f, 7.3278f, 7.3304f, 7.3330f, 7.3356f,
    7.3382f, 7.3408f, 7.3434f, 7.3460f, 7.3486f, 7.3512f, 7.3537f, 7.3563f, 7.3588f, 7.3614f, 7.3639f, 7.3664f,
    7.3690f, 7.3715f, 7.3740f, 7.3765f, 7.3790f, 7.3815f, 7.3840f, 7.3865f, 7.3889f, 7.3914f, 7.3939f, 7.3963f,
    7.3988f, 7.4012f, 7.4037f, 7.4061f, 7.4085f, 7.4110f, 7.4134f, 7.4158f, 7.4182f, 7.4206f, 7.4230f, 7.4254f,
    7.4277f, 7.4301f, 7.4325f, 7.4348f, 7.4372f, 7.4396f, 7.4419f, 7.4442f, 7.4466f, 7.4489f, 7.4512f, 7.4536f,
    7.4559f, 7.4582f, 7.4605f, 7.4628f, 7.4651f, 7.4674f, 7.4697f, 7.4719f, 7.4742f, 7.4765f, 7.4787f, 7.4810f,
    7.4832f, 7.4855f, 7.4877f, 7.4900f, 7.4922f, 7.4944f, 7.4967f, 7.4989f, 7.5011f, 7.5033f, 7.5055f, 7.5077f,
    7.5099f, 7.5121f, 7.5143f, 7.5164f, 7.5186f, 7.5208f, 7.5229f, 7.5251f, 7.5273f, 7.5294f, 7.5316f, 7.5337f,
    7.5358f, 7.5380f, 7.5401f, 7.5422f, 7.5443f, 7.5464f, 7.5486f, 7.5507f, 7.5528f, 7.5549f, 7.5570f, 7.5590f,
    7.5611f, 7.5632f, 7.5653f, 7.5673f, 7.5694f, 7.5715f, 7.5735f, 7.5756f, 7.5776f, 7.5797f, 7.5817f, 7.5838f,
    7.5858f, 7.5878f, 7.5898f, 7.5919f, 7.5939f, 7.5959f, 7.5979f, 7.5999f, 7.6019f, 7.6039f, 7.6059f, 7.6079f,
    7.6099f, 7.6118f, 7.6138f, 7.6158f, 7.6178f, 7.6197f, 7.6217f, 7.6236f, 7.6285f, 7.6363f, 7.6440f, 7.6516f,
    7.6592f, 7.6667f, 7.6742f, 7.6816f, 7.6889f, 7.6962f, 7.7035f, 7.7107f, 7.7178f, 7.7249f, 7.7319f, 7.7389f,
    7.7459f, 7.7528f, 7.7596f, 7.7664f, 7.7732f, 7.7799f, 7.7866f, 7.7932f, 7.7998f, 7.8063f, 7.8128f, 7.8192f,
    7.8256f, 7.8320f, 7.8383f, 7.8446f, 7.8509f, 7.8571f, 7.8633f, 7.8694f, 7.8755f, 7.8816f, 7.8876f, 7.8936f,
    7.8995f, 7.9054f, 7.9113f, 7.9172f, 7.9230f, 7.9288f, 7.9345f, 7.9402f, 7.9459f, 7.9516f, 7.9572f, 7.9628f,
    7.9683f, 7.9738f, 7.9793f, 7.9848f, 7.9902f, 7.9956f, 8.0010f, 8.0064f, 8.0117f, 8.0170f, 8.0222f, 8.0275f,
    8.0327f, 8.0379f, 8.0430f, 8.0481f, 8.0533f, 8.0583f, 8.0634f, 8.0684f, 8.0734f, 8.0784f, 8.0833f, 8.0883f,
    8.0932f, 8.0980f, 8.1029f, 8.1077f, 8.1125f, 8.1173f, 8.1221f, 8.1268f, 8.1315f, 8.1362f, 8.1409f, 8.1455f,
    8.1502f, 8.1548f, 8.1594f, 8.1639f, 8.1685f, 8.1730f, 8.1775f, 8.1820f, 8.1865f, 8.1909f, 8.1953f, 8.1997f,
    8.2041f, 8.2085f, 8.2128f, 8.2172f, 8.2215f, 8.2258f, 8.2300f, 8.2343f, 8.2385f, 8.2428f, 8.2470f, 8.2511f,
    8.2553f, 8.2595f, 8.2636f, 8.2677f, 8.2718f, 8.2759f, 8.2800f, 8.2840f, 8.2880f, 8.2920f, 8.2960f, 8.3000f,
    8.3040f, 8.3080f, 8.3119f, 8.3158f, 8.3217f, 8.3294f, 8.3371f, 8.3447f, 8.3523f, 8.3598f, 8.3673f, 8.3747f,
    8.3821f, 8.3894f, 8.3966f, 8.4038f, 8.4109f, 8.4180f, 8.4251f, 8.4321f, 8.4390f, 8.4459f, 8.4528f, 8.4596f,
    8.4663f, 8.4730f, 8.4797f, 8.4863f, 8.4929f, 8.4994f, 8.5059f, 8.5124f, 8.5188f, 8.5252f, 8.5315f, 8.5378f,
    8.5440f, 8.5502f, 8.5564f, 8.5625f, 8.5686f, 8.5747f, 8.5807f, 8.5867f, 8.5927f, 8.5986f, 8.6045f, 8.6103f,
    8.6161f, 8.6219f, 8.6277f, 8.6334f, 8.6391f, 8.6447f, 8.6503f, 8.6559f, 8.6615f, 8.6670f, 8.6725f, 8.6780f,
    8.6834f, 8.6888f, 8.6942f, 8.6995f, 8.7048f, 8.7101f, 8.7154f, 8.7206f, 8.7258f, 8.7310f, 8.7362f, 8.7413f,
    8.7464f, 8.7515f, 8.7565f, 8.7616f, 8.7666f, 8.7715f, 8.7765f, 8.7814f, 8.7863f, 8.7912f, 8.7960f, 8.8009f,
    8.8057f, 8.8105f, 8.8152f, 8.8200f, 8.8247f, 8.8294f, 8.8340f, 8.8387f, 8.8433f, 8.8479f, 8.8525f, 8.8571f,
    8.8616f, 8.8662f, 8.8707f, 8.8751f, 8.8796f, 8.8841f, 8.8885f, 8.8929f, 8.8973f, 8.9016f, 8.9060f, 8.9103f,
    8.9146f, 8.9189f, 8.9232f, 8.9274f, 8.9317f, 8.9359f, 8.9401f, 8.9443f, 8.9485f, 8.9526f, 8.9567f, 8.9609f,
    8.9650f, 8.9690f, 8.9731f, 8.9771f, 8.9812f, 8.9852f, 8.9892f, 8.9932f, 8.9971f, 9.0011f, 9.0050f, 9.0090f,
    9.0129f, 9.0168f, 9.0206f, 9.0245f, 9.0283f, 9.0322f, 9.0360f, 9.0398f, 9.0436f, 9.0474f, 9.0511f, 9.0549f,
    9.0586f, 9.0623f, 9.0660f, 9.0697f, 9.0734f, 9.0770f, 9.0807f, 9.0843f, 9.0879f, 9.0916f, 9.0952f, 9.0987f,
    9.1023f, 9.1059f, 9.1094f, 9.1129f, 9.1165f, 9.1200f, 9.1235f, 9.1270f, 9.1304f, 9.1339f, 9.1373f, 9.1408f,
    9.1442f, 9.1476f, 9.1510f, 9.1544f, 9.1578f, 9.1612f, 9.1645f, 9.1679f, 9.1712f, 9.1745f, 9.1778f, 9.1811f,
    9.1844f, 9.1877f, 9.1910f, 9.1942f, 9.1975f, 9.2007f, 9.2039f, 9.2071f, 9.2103f, 9.2135f, 9.2167f, 9.2199f,
    9.2231f, 9.2262f, 9.2294f, 9.2325f, 9.2356f, 9.2387f, 9.2418f, 9.2449f, 9.2480f, 9.2511f, 9.2542f, 9.2572f,
    9.2603f, 9.2633f, 9.2663f, 9.2694f, 9.2724f, 9.2754f, 9.2784f, 9.2814f, 9.2843f, 9.2873f, 9.2903f, 9.2932f,
    9.2962f, 9.2991f, 9.3020f, 9.3049f, 9.3078f, 9.3107f, 9.3136f, 9.3165f, 9.3194f, 9.3222f, 9.3251f, 9.3279f,
    9.3308f, 9.3336f, 9.3364f, 9.3393f, 9.3421f, 9.3449f, 9.3477f, 9.3505f, 9.3532f, 9.3560f, 9.3588f, 9.3615f,
    9.3643f, 9.3670f, 9.3697f, 9.3725f, 9.3752f, 9.3779f, 9.3806f, 9.3833f, 9.3860f, 9.3887f, 9.3913f, 9.3940f,
    9.3967f, 9.3993f, 9.4020f, 9.4046f, 9.4072f, 9.4098f, 9.4125f, 9.4151f, 9.4177f, 9.4203f, 9.4229f, 9.4255f,
    9.4280f, 9.4306f, 9.4332f, 9.4357f, 9.4383f, 9.4408f, 9.4434f, 9.4459f, 9.4484f, 9.4509f, 9.4534f, 9.4559f,
    9.4584f, 9.4609f, 9.4634f, 9.4659f, 9.4684f, 9.4709f, 9.4733f, 9.4758f, 9.4782f, 9.4807f, 9.4831f, 9.4855f,
    9.4880f, 9.4904f, 9.4928f, 9.4952f, 9.4976f, 9.5000f, 9.5024f, 9.5048f, 9.5072f, 9.5096f, 9.5119f, 9.5143f,
    9.5166f, 9.5190f, 9.5213f, 9.5237f, 9.5260f, 9.5284f, 9.5307f, 9.5330f, 9.5353f, 9.5376f, 9.5399f, 9.5422f,
    9.5445f, 9.5468f, 9.5491f, 9.5514f, 9.5536f, 9.5559f, 9.5582f, 9.5604f, 9.5627f, 9.5649f, 9.5672f, 9.5694f,
    9.5716f, 9.5739f, 9.5761f, 9.5783f, 9.5805f, 9.5827f, 9.5849f, 9.5871f, 9.5893f, 9.5915f, 9.5937f, 9.5959f,
    9.5980f, 9.6002f, 9.6024f, 9.6045f, 9.6067f, 9.6088f, 9.6110f, 9.6131f, 9.6153f, 9.6174f, 9.6195f, 9.6217f,
    9.6238f, 9.6259f, 9.6280f, 9.6301f, 9.6322f, 9.6343f, 9.6364f, 9.6385f, 9.6406f, 9.6426f, 9.6447f, 9.6468f,
    9.6489f, 9.6509f, 9.6530f, 9.6550f, 9.6571f, 9.6591f, 9.6612f, 9.6632f, 9.6652f, 9.6673f, 9.6693f, 9.6713f,
    9.6733f, 9.6753f, 9.6773f, 9.6793f, 9.6813f, 9.6833f, 9.6853f, 9.6873f, 9.6893f, 9.6913f, 9.6933f, 9.6952f,
    9.6972f, 9.6992f, 9.7011f, 9.7040f
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements TMC12 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class TMC12Interpolation
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckUpdateTrigger
    ///
    /// @brief  Check and update the trigger condition
    ///
    /// @param  pInput  Pointer to the Input data of the latest trigger data
    /// @param  pOutput Pointer to the dependence data of this IQ module
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CheckUpdateTrigger(
        ISPIQTriggerData*    pInput,
        TMC12InputData* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunInterpolation
    ///
    /// @brief  Perform the interpolation.
    ///
    /// @param  pInput Pointer to the Input data to the TMC12 Module
    /// @param  pData  Pointer to the Output of the interpolation algorithm
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RunInterpolation(
        const TMC12InputData*          pInput,
        tmc_1_2_0::tmc12_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoInterpolation
    ///
    /// @brief  TMC12 Interpolation function
    ///
    /// @param  pData1   Pointer to the input data set 1
    /// @param  pData2   Pointer to the input data set 2
    /// @param  ratio    Interpolation Ratio
    /// @param  pOutput  Pointer to the result set
    ///
    /// @return BOOL     return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL DoInterpolation(
        VOID* pData1,
        VOID* pData2,
        FLOAT ratio,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDRModeSearchNode
    ///
    /// @brief  Search Function for HDRMode Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT HDRModeSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DRCGainSearchNode
    ///
    /// @brief  Search Function for DRCGain Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Search Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT DRCGainSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HDRAECSearchNode
    ///
    /// @brief  Search Function for HDRAEC Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Search Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT HDRAECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECSearchNode
    ///
    /// @brief  Search Function for AEC Nodes
    ///
    /// @param  pParentNode     Pointer to the Parent Node
    /// @param  pTriggerData    Pointer to the Trigger Value List
    /// @param  pChildNode      Pointer to the Child Node
    ///
    /// @return Number of Child Node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT AECSearchNode(
        TuningNode* pParentNode,
        VOID*       pTriggerData,
        TuningNode* pChildNode);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationData
    ///
    /// @brief  Interpolation Function for TMC12 Module
    ///
    /// @param  pInput1 Poiner to the Input Data Set 1
    /// @param  pInput2 Pointer to the Input Data Set 2
    /// @param  ratio   Ratio Value
    /// @param  pOutput Pointer to the Output Data
    ///
    /// @return BOOL    return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolationData(tmc_1_2_0::tmc12_rgn_dataType* pInput1,
                                  tmc_1_2_0::tmc12_rgn_dataType* pInput2,
                                  FLOAT                                    ratio,
                                  tmc_1_2_0::tmc12_rgn_dataType* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainCurve
    ///
    /// @brief  Calculate ADRC Gain curve values
    ///
    /// @param  pInput     Pointer to input data
    /// @param  pTmcData   Pointer to rgn data
    ///
    /// @return BOOL       Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateGainCurve(
        const TMC12InputData*          pInput,
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HistogramPreProcess
    ///
    /// @brief  Preprocess for histogram data.
    ///
    /// @param  pTmcData            Pointer to TMC data
    /// @param  pInput              Pointer to TMC12 input data
    /// @param  pTmc12PerFrameInfo  Pointer to TMC12 per frame info
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL HistogramPreProcess(
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
        const TMC12InputData*          pInput,
        TMC12PerFrameInfo*             pTmc12PerFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateKneePoints
    ///
    /// @brief  Knee point calculation function
    ///
    /// @param  pTmcData            Pointer to TMC data
    /// @param  pInput              Pointer to TMC12 input data
    /// @param  pTmc12PerFrameInfo  Pointer to TMC12 per frame info
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateKneePoints(
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
        const TMC12InputData*          pInput,
        TMC12PerFrameInfo*             pTmc12PerFrameInf);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateAnchorKneePoints
    ///
    /// @brief  Anchor knee point calculation function
    ///
    /// @param  pTmcData        Pointer to TMC data
    /// @param  pInput          Pointer to input
    /// @param  pAnchorKneeX    Pointer to array anchorKneeX
    /// @param  pAnchorKneey    Pointer to array anchorKneey
    /// @param  pDrcGainDark    Pointer to drc dark gain
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateAnchorKneePoints(
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
        const TMC12InputData*          pInput,
        FLOAT*                         pAnchorKneeX,
        FLOAT*                         pAnchorKneey,
        FLOAT*                         pDrcGainDark);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHistKneePoints
    ///
    /// @brief  Histogram knee point calculation function
    ///
    /// @param  pTmcData            Pointer to TMC data
    /// @param  pInput              Pointer to input
    /// @param  kneeX               kneeX
    /// @param  kneeY               kneeY
    /// @param  pTmc12PerFrameInfo  Pointer to TMC12 per frame info
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHistKneePoints(
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
        const TMC12InputData*          pInput,
        FLOAT*                         kneeX,
        FLOAT*                         kneeY,
        TMC12PerFrameInfo*             pTmc12PerFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxLTMPercentageCheck
    ///
    /// @brief  Max LTM percentage check function
    ///
    /// @param  pTmcData  Pointer to TMC data
    /// @param  maxGain   Maximum input gain
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL MaxLTMPercentageCheck(
        tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
        FLOAT                          maxGain);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePCHIPCoefficient
    ///
    /// @brief  PCHIP coefficient calculation function
    ///
    /// @param  pIn    Pointer to input
    /// @param  pOut   Pointer to Output
    /// @param  pCoef  Pointer to Coef
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculatePCHIPCoefficient(
        FLOAT* pIn,
        FLOAT* pOut,
        FLOAT* pCoef);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateContrastEnhanceCurve
    ///
    /// @brief  contrast enhance curve calculation function.
    ///
    /// @param  pTmcData            Pointer to TMC data
    /// @param  pInput              Pointer to input data
    /// @param  pTmc12PerFrameInfo  Pointer to TMC12 per frame info
    ///
    /// @return BOOL   Return TRUE on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateContrastEnhanceCurve(
        tmc_1_2_0::tmc12_rgn_dataType*  pTmcData,
        const TMC12InputData*           pInput,
        TMC12PerFrameInfo*              pTmc12PerFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetZerosToTMCData
    ///
    /// @brief  Set zeros to TMC data when the input stats in NULL
    ///
    /// @param  pTmcData            Pointer to TMC data
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID SetZerosToTMCData(
        tmc_1_2_0::tmc12_rgn_dataType*  pTmcData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOverrideDarkBoostOffsetValid
    ///
    /// @brief  Check whether the override dark boost offset is valid or not
    ///
    /// @param  overrideDarkBoostOffset     Value of override dark boost offset
    ///
    /// @return TRUE for valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    __inline static BOOL IsOverrideDarkBoostOffsetValid(
        FLOAT overrideDarkBoostOffset)
    {
        BOOL isValid = FALSE;

        if ((-1 != overrideDarkBoostOffset)                                     &&
            (overrideDarkBoostOffset >= TMC12_OVERRIDE_DARK_BOOST_OFFSET_MIN)   &&
            (overrideDarkBoostOffset <= TMC12_OVERRIDE_DARK_BOOST_OFFSET_MAX))
        {
            isValid = TRUE;
        }

        return isValid;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOverrideFourthToneAnchorValid
    ///
    /// @brief  Check whether the fourth tone anchor is valid or not
    ///
    /// @param  overrideFourthToneAnchor    Value of fourth tone anchor
    /// @param  pToneAnchor                 Pointer to the tone anchor array
    ///
    /// @return TRUE for valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    __inline static BOOL IsOverrideFourthToneAnchorValid(
        FLOAT   overrideFourthToneAnchor,
        FLOAT*  pToneAnchor)
    {
        BOOL isValid = FALSE;

        if ((-1 != overrideFourthToneAnchor)                &&
            (overrideFourthToneAnchor > pToneAnchor[2])     &&
            (overrideFourthToneAnchor < pToneAnchor[4]))
        {
            isValid = TRUE;
        }

        return isValid;
    }
};
#endif // TMC12INTERPOLATION_H
