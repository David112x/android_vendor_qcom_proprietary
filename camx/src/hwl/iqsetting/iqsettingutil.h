// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  iqsettingutil.h
/// @brief Calculate IQ module Settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IQSETTINGUTIL_H
#define IQSETTINGUTIL_H

struct ISPIQTriggerData;
struct ADRCData;

#include "ispglobalelements.h"

// Bayer pattern
static const UINT8  RGGB_PATTERN      = 0;
static const UINT8  GRBG_PATTERN      = 1;
static const UINT8  BGGR_PATTERN      = 2;
static const UINT8  GBRG_PATTERN      = 3;
static const UINT8  YCBYCR422_PATTERN = 4;
static const UINT8  YCRYCB422_PATTERN = 5;
static const UINT8  CBYCRY422_PATTERN = 6;
static const UINT8  CRYCBY422_PATTERN = 7;
static const UINT8  Y_PATTERN         = 8;

// Max Values
static const UINT32 NMAX10            = 1024;
static const UINT32 MIN_BIT           = 0;
static const UINT32 MAX_BIT           = 1;
static const INT32  MIN_INT8          = -128;
static const INT32  MAX_INT8          = 127;
static const INT32  MIN_INT9          = -256;
static const INT32  MAX_INT9          = 255;
static const INT32  MIN_INT10         = -512;
static const INT32  MAX_INT10         = 511;
static const INT32  MIN_INT11         = -1024;
static const INT32  MAX_INT11         = 1023;
static const INT32  MIN_INT12         = -2048;
static const INT32  MAX_INT12         = 2047;
static const UINT32 MIN_UINT8         = 0;
static const UINT32 MAX_UINT8         = 255;
static const UINT32 MAX_UINT11        = 2048;
static const UINT32 MAX_UINT12        = 4095;
static const UINT32 MAX_UINT14        = 16383;
static const UINT32 MIN_UINT16        = 0;    // added
static const UINT32 MAX_UINT16        = 65535;// added
static const UINT32 PIPELINE_BITWIDTH = 14;
static const FLOAT  FLOATMAX12        = 4096.0f;
static const FLOAT  FLOATMAX8         = 256.0f;

// Interpolation Level
static const INT32  InitialLevel = 1;

// FD MAX Face Number
static const UINT   MAX_FACE_NUM = 5;

// Q Numbers
static const UINT8  QNumber_4U  = 4;
static const UINT8  QNumber_5U  = 5;
static const UINT8  QNumber_6U  = 6;
static const UINT8  QNumber_7U  = 7;
static const UINT8  QNumber_8U  = 8;
static const UINT8  QNumber_9U  = 9;
static const UINT8  QNumber_10U = 10;
static const UINT8  QNumber_11U = 11;
static const UINT8  QNumber_12U = 12;
static const UINT8  QNumber_13U = 13;
static const UINT8  QNumber_14U = 14;
static const UINT8  QNumber_15U = 15;
static const UINT8  QNumber_16U = 16;
static const UINT8  QNumber_17U = 17;
static const UINT8  QNumber_18U = 18;
static const UINT8  QNumber_19U = 19;
static const UINT8  QNumber_20U = 20;
static const UINT8  QNumber_21U = 21;
static const UINT8  QNumber_22U = 22;
static const UINT8  QNumber_23U = 23;
static const UINT8  QNumber_24U = 24;
static const UINT8  QNumber_25U = 25;
static const UINT8  QNumber_26U = 26;
static const UINT8  QNumber_27U = 27;
static const UINT8  QNumber_28U = 28;
static const UINT8  QNumber_29U = 29;
static const UINT8  QNumber_30U = 30;
static const UINT8  QNumber_31U = 31;
static const UINT8  QNumber_32U = 32;
static const UINT   MaxLUTBank  = 2;

static const UINT32 CHROMATIX_BIT_WIDTH = 12;

// PDPC BIT MASK
static const INT32  PDPCBitMask = 31;

// ADRC Const
static const FLOAT MIN_NORM_OUTPUT_GAIN = 0.0f;
static const FLOAT MAX_NORM_OUTPUT_GAIN = 1.0f;

/// @brief Input data for any initialization input/output data
// NOWHINE NC004c: Share code with system team
struct IQLibraryData
{
    VOID* pCustomticData;  ///< OEM library data
};

// Maxmium Region Per Node
static const UINT MaxNumRegion = 20;

// Maxmium Child Nodes for one Parent Node
static const UINT MaxNumChildNode = 3;

// Maxmium Number of the ratios values
static const UINT MaxInterpolationItem = MaxNumChildNode - 1;

// The size of the logBinNormalized
static const UINT NumBins = 1024;

// The size of the LUT table
static const UINT LUTSize = 64;

// Calculated by exp(logBin[i]) / 16384
const FLOAT logBinNormalized[NumBins] =
{
    0.0001220f, 0.0003662f, 0.0006103f, 0.0008545f, 0.0010987f, 0.0013427f, 0.0015870f, 0.0018311f,
    0.0020754f, 0.0023195f, 0.0025637f, 0.0028076f, 0.0030518f, 0.0032961f, 0.0035401f, 0.0037842f,
    0.0040287f, 0.0042727f, 0.0045170f, 0.0047609f, 0.0050050f, 0.0052490f, 0.0054934f, 0.0057376f,
    0.0059820f, 0.0062261f, 0.0064698f, 0.0067144f, 0.0069584f, 0.0072026f, 0.0074465f, 0.0076910f,
    0.0079347f, 0.0081788f, 0.0084237f, 0.0086672f, 0.0089116f, 0.0091555f, 0.0094004f, 0.0096441f,
    0.0098883f, 0.0101325f, 0.0103766f, 0.0106212f, 0.0108651f, 0.0111090f, 0.0113527f, 0.0115971f,
    0.0118420f, 0.0120860f, 0.0123302f, 0.0125743f, 0.0128180f, 0.0130626f, 0.0133065f, 0.0135509f,
    0.0137943f, 0.0140392f, 0.0142827f, 0.0145276f, 0.0147708f, 0.0150150f, 0.0152603f, 0.0155033f,
    0.0157486f, 0.0159914f, 0.0162364f, 0.0164801f, 0.0167241f, 0.0169684f, 0.0172128f, 0.0174572f,
    0.0177016f, 0.0179457f, 0.0181897f, 0.0184332f, 0.0186781f, 0.0189225f, 0.0191663f, 0.0194113f,
    0.0196554f, 0.0198987f, 0.0201429f, 0.0203861f, 0.0206301f, 0.0208750f, 0.0211186f, 0.0213629f,
    0.0216078f, 0.0218511f, 0.0220950f, 0.0223394f, 0.0225843f, 0.0228295f, 0.0230728f, 0.0233163f,
    0.0235601f, 0.0238040f, 0.0240505f, 0.0242946f, 0.0245388f, 0.0247829f, 0.0250270f, 0.0252709f,
    0.0255147f, 0.0257582f, 0.0260015f, 0.0262471f, 0.0264896f, 0.0267345f, 0.0269789f, 0.0272228f,
    0.0274661f, 0.0277117f, 0.0279566f, 0.0282009f, 0.0284445f, 0.0286873f, 0.0289322f, 0.0291762f,
    0.0294194f, 0.0296646f, 0.0299088f, 0.0301521f, 0.0303973f, 0.0306415f, 0.0308845f, 0.0311294f,
    0.0313732f, 0.0316189f, 0.0318633f, 0.0321064f, 0.0323513f, 0.0325948f, 0.0328402f, 0.0330841f,
    0.0333265f, 0.0335707f, 0.0338167f, 0.0340610f, 0.0343037f, 0.0345482f, 0.0347908f, 0.0350352f,
    0.0352813f, 0.0355256f, 0.0357680f, 0.0360121f, 0.0362578f, 0.0365015f, 0.0367469f, 0.0369903f,
    0.0372352f, 0.0374780f, 0.0377224f, 0.0379646f, 0.0382122f, 0.0384537f, 0.0386967f, 0.0389413f,
    0.0391874f, 0.0394311f, 0.0396763f, 0.0399191f, 0.0401633f, 0.0404091f, 0.0406523f, 0.0408969f,
    0.0411389f, 0.0413824f, 0.0416272f, 0.0418736f, 0.0421171f, 0.0423621f, 0.0426043f, 0.0428478f,
    0.0430927f, 0.0433391f, 0.0435825f, 0.0438272f, 0.0440689f, 0.0443164f, 0.0445563f, 0.0448021f,
    0.0450447f, 0.0452886f, 0.0455338f, 0.0457803f, 0.0460236f, 0.0462682f, 0.0465094f, 0.0467566f,
    0.0470003f, 0.0472454f, 0.0474869f, 0.0477345f, 0.0479786f, 0.0482191f, 0.0484656f, 0.0487086f,
    0.0489527f, 0.0491981f, 0.0494398f, 0.0496876f, 0.0499316f, 0.0501719f, 0.0504183f, 0.0506609f,
    0.0509047f, 0.0511496f, 0.0513957f, 0.0516378f, 0.0518811f, 0.0521255f, 0.0523711f, 0.0526178f,
    0.0528604f, 0.0531041f, 0.0533490f, 0.0535896f, 0.0538367f, 0.0540795f, 0.0543234f, 0.0545684f,
    0.0548145f, 0.0550562f, 0.0552990f, 0.0555429f, 0.0557878f, 0.0560338f, 0.0562753f, 0.0565234f,
    0.0567670f, 0.0570116f, 0.0572516f, 0.0574983f, 0.0577403f, 0.0579891f, 0.0582332f, 0.0584724f,
    0.0587185f, 0.0589656f, 0.0592079f, 0.0594511f, 0.0596954f, 0.0599407f, 0.0601869f, 0.0604281f,
    0.0606703f, 0.0609196f, 0.0611638f, 0.0614028f, 0.0616489f, 0.0618960f, 0.0621378f, 0.0623806f,
    0.0626244f, 0.0628691f, 0.0631148f, 0.0633614f, 0.0636026f, 0.0638448f, 0.0640879f, 0.0643319f,
    0.0645768f, 0.0648226f, 0.0650694f, 0.0653106f, 0.0655527f, 0.0658023f, 0.0660462f, 0.0662911f,
    0.0665301f, 0.0667768f, 0.0670176f, 0.0672660f, 0.0675086f, 0.0677521f, 0.0679964f, 0.0682416f,
    0.0684878f, 0.0687279f, 0.0689758f, 0.0692176f, 0.0694603f, 0.0697038f, 0.0699482f, 0.0701935f,
    0.0704396f, 0.0706795f, 0.0709273f, 0.0711688f, 0.0714184f, 0.0716616f, 0.0719057f, 0.0721506f,
    0.0723891f, 0.0726356f, 0.0728830f, 0.0731239f, 0.0733656f, 0.0736155f, 0.0738588f, 0.0741029f,
    0.0743479f, 0.0745862f, 0.0748327f, 0.0750801f, 0.0753207f, 0.0755697f, 0.0758119f, 0.0760549f,
    0.0762986f, 0.0765432f, 0.0767885f, 0.0770346f, 0.0772738f, 0.0775215f, 0.0777622f, 0.0780114f,
    0.0782536f, 0.0784966f, 0.0787403f, 0.0789848f, 0.0792300f, 0.0794760f, 0.0797148f, 0.0799623f,
    0.0802025f, 0.0804516f, 0.0806933f, 0.0809357f, 0.0811789f, 0.0814228f, 0.0816674f, 0.0819128f,
    0.0821589f, 0.0824057f, 0.0826451f, 0.0828934f, 0.0831341f, 0.0833756f, 0.0836261f, 0.0838689f,
    0.0841125f, 0.0843568f, 0.0846018f, 0.0848475f, 0.0850854f, 0.0853325f, 0.0855803f, 0.0858203f,
    0.0860609f, 0.0863108f, 0.0865529f, 0.0867955f, 0.0870389f, 0.0872830f, 0.0875277f, 0.0877731f,
    0.0880192f, 0.0882660f, 0.0885047f, 0.0887528f, 0.0889928f, 0.0892423f, 0.0894836f, 0.0897255f,
    0.0899681f, 0.0902113f, 0.0904552f, 0.0906998f, 0.0909450f, 0.0911909f, 0.0914375f, 0.0916847f,
    0.0919234f, 0.0921719f, 0.0924119f, 0.0926524f, 0.0929029f, 0.0931448f, 0.0933873f, 0.0936304f,
    0.0938742f, 0.0941186f, 0.0943636f, 0.0946093f, 0.0948556f, 0.0951025f, 0.0953406f, 0.0955888f,
    0.0958280f, 0.0960775f, 0.0963180f, 0.0965591f, 0.0968105f, 0.0970528f, 0.0972957f, 0.0975393f,
    0.0977834f, 0.0980282f, 0.0982736f, 0.0985196f, 0.0987563f, 0.0990035f, 0.0992513f, 0.0994898f,
    0.0997389f, 0.0999785f, 0.1002288f, 0.1004696f, 0.1007110f, 0.1009631f, 0.1012057f, 0.1014489f,
    0.1016927f, 0.1019370f, 0.1021820f, 0.1024275f, 0.1026634f, 0.1029101f, 0.1031573f, 0.1033949f,
    0.1036433f, 0.1038924f, 0.1041316f, 0.1043714f, 0.1046222f, 0.1048631f, 0.1051045f, 0.1053571f,
    0.1055997f, 0.1058428f, 0.1060866f, 0.1063308f, 0.1065757f, 0.1068211f, 0.1070671f, 0.1073029f,
    0.1075499f, 0.1077976f, 0.1080350f, 0.1082838f, 0.1085223f, 0.1087722f, 0.1090117f, 0.1092627f,
    0.1095034f, 0.1097445f, 0.1099972f, 0.1102395f, 0.1104823f, 0.1107256f, 0.1109695f, 0.1112139f,
    0.1114588f, 0.1117043f, 0.1119503f, 0.1121857f, 0.1124328f, 0.1126804f, 0.1129173f, 0.1131660f,
    0.1134152f, 0.1136536f, 0.1139039f, 0.1141434f, 0.1143833f, 0.1146353f, 0.1148762f, 0.1151177f,
    0.1153597f, 0.1156022f, 0.1158568f, 0.1161004f, 0.1163445f, 0.1165891f, 0.1168341f, 0.1170680f,
    0.1173141f, 0.1175608f, 0.1178079f, 0.1180438f, 0.1182919f, 0.1185406f, 0.1187779f, 0.1190276f,
    0.1192659f, 0.1195166f, 0.1197559f, 0.1200076f, 0.1202479f, 0.1204886f, 0.1207298f, 0.1209836f,
    0.1212259f, 0.1214685f, 0.1217117f, 0.1219554f, 0.1221996f, 0.1224442f, 0.1226893f, 0.1229350f,
    0.1231811f, 0.1234153f, 0.1236624f, 0.1239100f, 0.1241581f, 0.1243942f, 0.1246432f, 0.1248803f,
    0.1254937f, 0.1264764f, 0.1274540f, 0.1284263f, 0.1294061f, 0.1303803f, 0.1313618f, 0.1323375f,
    0.1333071f, 0.1342838f, 0.1352677f, 0.1362451f, 0.1372159f, 0.1381936f, 0.1391643f, 0.1401419f,
    0.1411263f, 0.1421035f, 0.1430731f, 0.1440493f, 0.1450322f, 0.1460071f, 0.1469887f, 0.1479620f,
    0.1489418f, 0.1499130f, 0.1508907f, 0.1518595f, 0.1528345f, 0.1538158f, 0.1547878f, 0.1557661f,
    0.1567505f, 0.1577254f, 0.1587063f, 0.1596774f, 0.1606544f, 0.1616374f, 0.1626101f, 0.1635887f,
    0.1645567f, 0.1655305f, 0.1665100f, 0.1674953f, 0.1684696f, 0.1694496f, 0.1704182f, 0.1713924f,
    0.1723721f, 0.1733574f, 0.1743310f, 0.1753099f, 0.1762768f, 0.1772490f, 0.1782266f, 0.1792095f,
    0.1801798f, 0.1811555f, 0.1821363f, 0.1831225f, 0.1840957f, 0.1850740f, 0.1860388f, 0.1870275f,
    0.1880025f, 0.1889827f, 0.1899490f, 0.1909202f, 0.1919156f, 0.1928775f, 0.1938637f, 0.1948355f,
    0.1958121f, 0.1967936f, 0.1977603f, 0.1987515f, 0.1997278f, 0.2006888f, 0.2016746f, 0.2026450f,
    0.2036200f, 0.2045997f, 0.2055842f, 0.2065527f, 0.2075258f, 0.2085034f, 0.2094857f, 0.2104516f,
    0.2114430f, 0.2124179f, 0.2133973f, 0.2143597f, 0.2153481f, 0.2163193f, 0.2172949f, 0.2182750f,
    0.2192594f, 0.2202263f, 0.2211974f, 0.2221728f, 0.2231525f, 0.2241366f, 0.2251024f, 0.2260951f,
    0.2270694f, 0.2280479f, 0.2290077f, 0.2299945f, 0.2309625f, 0.2319578f, 0.2329341f, 0.2338911f,
    0.2348755f, 0.2358640f, 0.2368331f, 0.2378061f, 0.2387831f, 0.2397641f, 0.2407492f, 0.2417141f,
    0.2426829f, 0.2436556f, 0.2446321f, 0.2456126f, 0.2465970f, 0.2475854f, 0.2485529f, 0.2495241f,
    0.2510007f, 0.2529408f, 0.2548960f, 0.2568406f, 0.2588000f, 0.2607483f, 0.2627113f, 0.2646625f,
    0.2666283f, 0.2685818f, 0.2705226f, 0.2724774f, 0.2744189f, 0.2763742f, 0.2783434f, 0.2802986f,
    0.2822394f, 0.2841936f, 0.2861613f, 0.2881138f, 0.2900507f, 0.2920005f, 0.2939635f, 0.2959101f,
    0.2978695f, 0.2998120f, 0.3017671f, 0.3037350f, 0.3056851f, 0.3076478f, 0.3095921f, 0.3115487f,
    0.3134863f, 0.3154359f, 0.3173977f, 0.3193398f, 0.3212937f, 0.3232596f, 0.3252050f, 0.3271621f,
    0.3291309f, 0.3310785f, 0.3330377f, 0.3349749f, 0.3369234f, 0.3388832f, 0.3408545f, 0.3428029f,
    0.3447625f, 0.3466985f, 0.3486455f, 0.3506034f, 0.3525723f, 0.3545168f, 0.3564720f, 0.3584380f,
    0.3603788f, 0.3623301f, 0.3642920f, 0.3662278f, 0.3681740f, 0.3701305f, 0.3720974f, 0.3740373f,
    0.3759874f, 0.3779476f, 0.3799181f, 0.3818606f, 0.3838131f, 0.3857755f, 0.3877092f, 0.3896916f,
    0.3916449f, 0.3935687f, 0.3955415f, 0.3974844f, 0.3994368f, 0.4013989f, 0.4033302f, 0.4053114f,
    0.4072616f, 0.4092211f, 0.4111490f, 0.4131273f, 0.4150735f, 0.4170290f, 0.4189517f, 0.4209254f,
    0.4228661f, 0.4248158f, 0.4267745f, 0.4287421f, 0.4306758f, 0.4326615f, 0.4346129f, 0.4365294f,
    0.4384982f, 0.4404759f, 0.4424182f, 0.4443692f, 0.4463287f, 0.4482520f, 0.4502287f, 0.4521689f,
    0.4541174f, 0.4560743f, 0.4580396f, 0.4599674f, 0.4619495f, 0.4638938f, 0.4658463f, 0.4678069f,
    0.4697759f, 0.4717059f, 0.4736439f, 0.4756373f, 0.4775915f, 0.4795057f, 0.4814757f, 0.4834054f,
    0.4853915f, 0.4873369f, 0.4892902f, 0.4912512f, 0.4931709f, 0.4951475f, 0.4970823f, 0.4990747f,
    0.5010248f, 0.5029827f, 0.5048976f, 0.5068706f, 0.5088003f, 0.5107885f, 0.5127332f, 0.5146853f,
    0.5166448f, 0.5186118f, 0.5205343f, 0.5225160f, 0.5244529f, 0.5263970f, 0.5283483f, 0.5303068f,
    0.5322726f, 0.5341922f, 0.5361724f, 0.5381061f, 0.5400467f, 0.5420486f, 0.5440035f, 0.5459109f,
    0.5478797f, 0.5498556f, 0.5517835f, 0.5537181f, 0.5557151f, 0.5576635f, 0.5596187f, 0.5615808f,
    0.5634934f, 0.5654691f, 0.5673950f, 0.5693844f, 0.5713236f, 0.5732694f, 0.5752218f, 0.5771809f,
    0.5791466f, 0.5811191f, 0.5830399f, 0.5850257f, 0.5869594f, 0.5888996f, 0.5908462f, 0.5927992f,
    0.5947587f, 0.5967246f, 0.5986970f, 0.6006159f, 0.6026013f, 0.6045327f, 0.6064703f, 0.6084141f,
    0.6103641f, 0.6123204f, 0.6142830f, 0.6162518f, 0.6182270f, 0.6201465f, 0.6221341f, 0.6240657f,
    0.6260033f, 0.6279470f, 0.6298966f, 0.6318523f, 0.6338141f, 0.6357820f, 0.6377560f, 0.6396721f,
    0.6416582f, 0.6435860f, 0.6455197f, 0.6475239f, 0.6494694f, 0.6514207f, 0.6533779f, 0.6553410f,
    0.6572442f, 0.6592189f, 0.6611996f, 0.6631198f, 0.6651122f, 0.6670438f, 0.6689810f, 0.6709239f,
    0.6728724f, 0.6748266f, 0.6767864f, 0.6787519f, 0.6807232f, 0.6826319f, 0.6846144f, 0.6865340f,
    0.6885278f, 0.6904584f, 0.6923944f, 0.6944052f, 0.6963523f, 0.6983048f, 0.7002628f, 0.7022263f,
    0.7041249f, 0.7060992f, 0.7080790f, 0.7099934f, 0.7119842f, 0.7139092f, 0.7158393f, 0.7178465f,
    0.7197873f, 0.7217333f, 0.7236846f, 0.7256412f, 0.7276031f, 0.7295703f, 0.7314696f, 0.7334473f,
    0.7354303f, 0.7373449f, 0.7393384f, 0.7412632f, 0.7431930f, 0.7451278f, 0.7471423f, 0.7490874f,
    0.7510376f, 0.7529928f, 0.7549532f, 0.7569186f, 0.7588133f, 0.7607887f, 0.7627694f, 0.7646787f,
    0.7666694f, 0.7685885f, 0.7705894f, 0.7725183f, 0.7744520f, 0.7763906f, 0.7783340f, 0.7802823f,
    0.7822354f, 0.7841934f, 0.7861564f, 0.7881242f, 0.7900970f, 0.7920747f, 0.7939780f, 0.7959654f,
    0.7978780f, 0.7998752f, 0.8017972f, 0.8037238f, 0.8057357f, 0.8076718f, 0.8096125f, 0.8115579f,
    0.8135080f, 0.8154627f, 0.8174222f, 0.8193864f, 0.8213553f, 0.8233289f, 0.8252247f, 0.8272076f,
    0.8291124f, 0.8311047f, 0.8330184f, 0.8350200f, 0.8369428f, 0.8389539f, 0.8408857f, 0.8428220f,
    0.8447627f, 0.8467079f, 0.8486575f, 0.8506117f, 0.8525704f, 0.8545335f, 0.8565012f, 0.8584734f,
    0.8603642f, 0.8623453f, 0.8643309f, 0.8662346f, 0.8682292f, 0.8701414f, 0.8721450f, 0.8740659f,
    0.8759909f, 0.8780080f, 0.8799418f, 0.8818798f, 0.8838220f, 0.8857686f, 0.8877194f, 0.8896746f,
    0.8916340f, 0.8935978f, 0.8955658f, 0.8975382f, 0.8994251f, 0.9014060f, 0.9033912f, 0.9052904f,
    0.9072842f, 0.9091915f, 0.9111939f, 0.9131094f, 0.9151205f, 0.9170443f, 0.9189721f, 0.9209960f,
    0.9229322f, 0.9248724f, 0.9268166f, 0.9287650f, 0.9307174f, 0.9326740f, 0.9346347f, 0.9365995f,
    0.9385684f, 0.9404474f, 0.9424244f, 0.9444056f, 0.9463909f, 0.9482856f, 0.9502791f, 0.9521816f,
    0.9541832f, 0.9560935f, 0.9581034f, 0.9600216f, 0.9619435f, 0.9639657f, 0.9658956f, 0.9678293f,
    0.9697669f, 0.9717084f, 0.9736537f, 0.9756030f, 0.9775562f, 0.9795132f, 0.9814742f, 0.9834391f,
    0.9854080f, 0.9873808f, 0.9893575f, 0.9912391f, 0.9932235f, 0.9952120f, 0.9971047f, 1.0000000f,
};

/// @brief Different sensor type
enum SensorType
{
    MONO_Y = 0,   ///< Mono Sensor
    BAYER_RGGB,   ///< Bayer Pattern
    BAYER_RCCB,   ///< RCCB format
    BAYER_PDAF,   ///< PDAF sensor
    BAYER_HDR,    ///< HDR sensor
};


// Trigger Interpolation Output
// NOWHINE NC004c : Shared file with system team so uses non-CamX file naming
struct InterpolationOutput
{
    UINT32  startIndex;              ///<  Index of start node
    UINT32  endIndex;                ///<  Index of end node
    FLOAT   interpolationRatio;      ///<  Interpolation Ratio
};

/// @brief Trigger Region
// NOWHINE NC004c : Shared file with system team so uses non-CamX file naming
struct TriggerRegion
{
    FLOAT start;                     ///< Region start value
    FLOAT end;                       ///< Region end value
};

// Define the chromtix intepoloation tree
/// @brief Node structure of the chromatix tree
// NOWHINE NC004c : Shared file with system team so uses non-CamX file naming
struct TuningNode
{
    BOOL         isValid;                                  ///< Is the tuning Node valid
    TuningNode*  pChildNode[MaxNumChildNode];              ///< Child Node Pointer Array
    UINT         numChild;                                 ///< Number of Child Node
    FLOAT        interpolationValue[MaxInterpolationItem]; ///< interpolation Value
    VOID*        pNodeData;                                ///< Pointer to current Node Data
    VOID*        pData;                                    ///< Pointer to the tuning data
    UINT         level;                                    ///< Which level in the tree that this Node is at, starting with 1
};

typedef UINT (*SearchChildNode)(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode);

// NOWHINE NC004c : Shared file with system team so uses non-CamX file naming
struct NodeOperation
{
    SearchChildNode  pSearchChildNode;  ///< Searching function for child node
    UINT             numChildPerNode;   ///< Define the number of change each node can have
};

// Interpolation Function pototype
typedef BOOL (*DoInterpolation)(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pOutput);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils
///
/// @brief Utility class for IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c : Shared file with system team so uses non-CamX file naming
class IQSettingUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DataConversion
    ///
    /// @brief  Convert the Data from float to UINT16
    ///
    /// @param  input Input float value
    /// @param  min   Minimum value
    /// @param  max   Maximum value
    ///
    /// @return The converted UINT16 value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT16 DataConversion(
        FLOAT input,
        UINT32 min,
        UINT32 max)
    {
        INT    result1 = IQSettingUtils::RoundFLOAT(input);
        UINT32 result2 = IQSettingUtils::AbsoluteINT(result1);
        UINT16 result3 = static_cast<UINT16>(IQSettingUtils::ClampUINT32(result2, min, max));

        return result3;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Memcpy
    ///
    /// @brief  Wraps the standard memcpy function; copies "numBytes" from source to destination
    ///
    /// @param  pDst       pointer to destination of copy
    /// @param  pSrc       pointer to source of copy
    /// @param  numBytes   size of data pointed at by pSrc
    ///
    /// @return Dest Pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline VOID* Memcpy(
        VOID*       pDst,
        const VOID* pSrc,
        SIZE_T      numBytes)
    {
        VOID* pData = NULL;

        if ((NULL != pDst) && (NULL != pSrc))
        {
            pData = memcpy(pDst, pSrc, numBytes);
        }

        return pData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Memset
    ///
    /// @brief  Wraps the standard memset function; sets "numBytes" at the destination pointer to the specified pattern
    ///
    /// @param  pDst        pointer to destination of copy
    /// @param  setVal      character valueto set
    /// @param  numBytes    number of characters (bytes) to copy from pSrc to pDst
    ///
    /// @return Destination pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline VOID* Memset(
        VOID*    pDst,
        INT      setVal,
        SIZE_T   numBytes)
    {
        if (NULL == pDst)
        {
            return NULL;
        }
        return memset(pDst, setVal, numBytes);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Memcmp
    ///
    /// @brief  Function to wrap the standard memcmp function; compares the first n bytes of memory between the two sources
    ///
    /// @param  pSrc1      pointer to first source
    /// @param  pSrc2      pointer to second source
    /// @param  numBytes   number of characters (bytes) to compare between pSrc1 and pSrc2
    ///
    /// @return An integer less than, equal to, or greater than zero if the first n bytes of s1 is found, respectively,
    ///         to be the same in relation to s2
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT Memcmp(
        const VOID*  pSrc1,
        const VOID*  pSrc2,
        SIZE_T       numBytes)
    {
        return memcmp(pSrc1, pSrc2, numBytes);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SquareRootFLOAT
    ///
    /// @brief  Wraps the standard sqrt function
    ///
    /// @param  val    Input value whose Square root need to be found
    ///
    /// @return Float  Square Root value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT SquareRootFLOAT(
        FLOAT val)
    {
        return static_cast<FLOAT>(sqrt(val));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SquareRootDOUBLE
    ///
    /// @brief  Wraps the standard sqrt function
    ///
    /// @param  val    Input value whose Square root need to be found
    ///
    /// @return DOUBLE  Square Root value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline DOUBLE SquareRootDOUBLE(
        DOUBLE val)
    {
        return sqrt(val);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CeilingDOUBLE
    ///
    /// @brief  Find the Ceil value of double
    ///
    /// @param  input  Input double value
    ///
    /// @return Ceil value of the input double value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline DOUBLE CeilingDOUBLE(
        DOUBLE input)
    {
        return ceil(input);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampUINT16
    ///
    /// @brief  Clamp UINT16 to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, below which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return UINT16 Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT16 ClampUINT16(
        UINT16 input1,
        UINT16 min,
        UINT16 max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampINT16
    ///
    /// @brief  Clamp INT16 to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, bellow which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return INT16 Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT16 ClampINT16(
        INT16 input1,
        INT16 min,
        INT16 max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampUINT32
    ///
    /// @brief  Clamp UINT32 to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, below which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return UINT32 Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 ClampUINT32(
        UINT32 input1,
        UINT32 min,
        UINT32 max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampDOUBLE
    ///
    /// @brief  Clamp DOUBLE to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, below which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return DOUBLE Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline DOUBLE ClampDOUBLE(
        DOUBLE input1,
        DOUBLE min,
        DOUBLE max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampFLOAT
    ///
    /// @brief  Clamp FLOAT to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, below which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return FLOAT Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT ClampFLOAT(
        FLOAT input1,
        FLOAT min,
        FLOAT max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClampINT32
    ///
    /// @brief  Clamp INT32 to the range of maximum and minimum
    ///
    /// @param  input1 input value to be clamped
    /// @param  min    Minimum value, bellow which the input will be clamped
    /// @param  max    Maximum value, beyond which the input will be clamped
    ///
    /// @return UINT32 Input if it is range of max and minimum, else clamped value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT32 ClampINT32(
        INT32 input1,
        INT32 min,
        INT32 max)
    {
        return (((input1) <= (min)) ? (min) : (((input1) >= (max)) ? (max) : (input1)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FloatToQNumber
    ///
    /// @brief  Converts Float value to Q Format
    ///
    /// @param  value  Float Value , for which Needs to converted to Q format
    /// @param  exp    Exponent bits of Q are used for that reg
    ///
    /// @return INT32 The converted value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT32 FloatToQNumber(
        FLOAT value,
        INT   exp)
    {
        return  RoundFLOAT((value * static_cast<FLOAT>(1 << exp)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QNumberToFloat
    ///
    /// @brief  Converts Q value to Float Format
    ///
    /// @param  value  Q Value , for which Needs to converted to Float
    /// @param  exp    Exponent bits of Q are used for that reg
    ///
    /// @return FLOAT The converted value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT QNumberToFloat(
        INT value,
        INT exp)
    {
        return (static_cast<FLOAT>(value) / static_cast<FLOAT>(1 << (exp)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MAXINT64BITFIELD
    ///
    /// @brief  Maximum Integer with given bit field
    ///
    /// @param  input  Bits for the maximum integer
    ///
    /// @return INT64  Returns the max value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT64 MAXINT64BITFIELD(
        INT input)
    {
        return ((input < 1) ? (0) : (((1ull) << ((input))) - 1));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RoundFLOAT
    ///
    /// @brief  Round the FLOAT to Nearest INT
    ///
    /// @param  input input value
    ///
    /// @return Round to the nearest INT value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT RoundFLOAT(
        FLOAT input)
    {
        return static_cast<INT>(roundf(input));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RoundDOUBLE
    ///
    /// @brief  Round the DOUBLE to Nearest INT64
    ///
    /// @param  input input value
    ///
    /// @return Round to the nearest int64 value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT64 RoundDOUBLE(
        DOUBLE input)
    {
        return static_cast<INT64>(llroundl(input));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AbsoluteINT
    ///
    /// @brief  Find the absolute value of a INT
    ///
    /// @param  input input value
    ///
    /// @return Absolute value of the input INT value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 AbsoluteINT(
        INT input)
    {
        return abs(input);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AbsoluteFLOAT
    ///
    /// @brief  Find the absolute value of a FLOAT
    ///
    /// @param  input input value
    ///
    /// @return Absolute value of the input FLOAT value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT AbsoluteFLOAT(
        FLOAT input)
    {
        return fabsf(input);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AbsoluteDOUBLE
    ///
    /// @brief  Find the absolute value of a DOUBLE
    ///
    /// @param  input input value
    ///
    /// @return Absolute value of the input DOUBLE value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline DOUBLE AbsoluteDOUBLE(
        DOUBLE input)
    {
        return fabs(input);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxFLOAT
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline FLOAT MaxFLOAT(
        FLOAT value1,
        FLOAT value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxDouble
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline DOUBLE MaxDouble(
        DOUBLE value1,
        DOUBLE value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxINT8
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT8 MaxINT8(
        INT8 value1,
        INT8 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxINT16
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT16 MaxINT16(
        INT16 value1,
        INT16 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxINT32
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT32 MaxINT32(
        INT32 value1,
        INT32 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxINT64
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT64 MaxINT64(
        INT64 value1,
        INT64 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT8
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT8 MaxUINT8(
        UINT8 value1,
        UINT8 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT16
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT16 MaxUINT16(
        UINT16 value1,
        UINT16 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT32
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT32 MaxUINT32(
        UINT32 value1,
        UINT32 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MaxUINT64
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT64 MaxUINT64(
        UINT64 value1,
        UINT64 value2)
    {
        return ((value1 > value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinFLOAT
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline FLOAT MinFLOAT(
        FLOAT value1,
        FLOAT value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinDouble
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline DOUBLE MinDouble(
        DOUBLE value1,
        DOUBLE value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinINT8
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT8 MinINT8(
        INT8 value1,
        INT8 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinINT16
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT16 MinINT16(
        INT16 value1,
        INT16 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinINT32
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT32 MinINT32(
        INT32 value1,
        INT32 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinINT64
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT64 MinINT64(
        INT64 value1,
        INT64 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinUINT8
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT8 MinUINT8(
        UINT8 value1,
        UINT8 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinUINT16
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT16 MinUINT16(
        UINT16 value1,
        UINT16 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinUINT32
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT32 MinUINT32(
        UINT32 value1,
        UINT32 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MinUINT64
    ///
    /// @brief  Returns the larger of the two input values.
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return The larger of the two input values
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline UINT64 MinUINT64(
        UINT64 value1,
        UINT64 value2)
    {
        return ((value1 < value2) ? value1 : value2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignOfINT32
    ///
    /// @brief  Returns the sign of the input value.
    ///
    /// @param  value value whose sign need to check
    ///
    /// @return The sign of the value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT SignOfINT32(
        INT32 value)
    {
        return ((value < 0) ? (-1) : (1));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignOfINT64
    ///
    /// @brief  Returns the sign of the input value.
    ///
    /// @param  value value whose sign need to check
    ///
    /// @return The sign of the value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline INT SignOfINT64(
        INT64 value)
    {
        return ((value < 0) ? (-1) : (1));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignOfFLOAT
    ///
    /// @brief  Returns the sign of the input value.
    ///
    /// @param  value value whose sign need to check
    ///
    /// @return The sign of the value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT SignOfFLOAT(
        FLOAT value)
    {
        return ((value < 0) ? (-1) : (1));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FEqualCoarse
    ///
    /// @brief  Compares the floating point numbers with small gap
    ///
    /// @param  value1      The first number to compare
    /// @param  value2      The second number to compare
    /// @param  difference  the difference to be allowed.
    ///
    /// @return BOOL TRUE if smaller than difference FALSE if not smaller than difference
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline BOOL FEqualCoarse(
        FLOAT value1,
        FLOAT value2,
        FLOAT difference)
    {
        return ((fabsf(value1 - value2) < difference) ? (TRUE) : (FALSE));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FEqual
    ///
    /// @brief  Compares the floating point numbers
    ///
    /// @param  value1  The first number to compare
    /// @param  value2  The second number to compare
    ///
    /// @return BOOL TRUE if equal FALSE if not equal
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline BOOL FEqual(
        FLOAT value1,
        FLOAT value2)
    {
        return ((fabs(value1 - value2) < 1e-9) ? (TRUE) : (FALSE));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateInterpolationRatio
    ///
    /// @brief  Calculate the interpolation Ratio within the region
    ///
    /// @param  value  The input value within the region
    /// @param  start  The start value of the region
    /// @param  end    The end value of the region
    ///
    /// @return The interpolation ratio value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static  __inline FLOAT CalculateInterpolationRatio(
        DOUBLE value,
        DOUBLE start,
        DOUBLE end)
    {
        FLOAT ratio;

        if (value < start)
        {
            ratio = 0.0f;
        }
        else if (value >= end)
        {
            ratio = 1.0f;
        }
        else
        {
            ratio = static_cast<FLOAT>((value  - start) / (end - start));
        }

        return ratio;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationFloatBilinear
    ///
    /// @brief  Perform Bilinear Interpolation to two float value
    ///
    /// @param  inputData1  Input parameter one, float value
    /// @param  inputData2  Input parameter two, float value
    /// @param  ratioData   Interpolation Ratio
    ///
    /// @return The interpolation result, float value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT InterpolationFloatBilinear(
        FLOAT             inputData1,
        FLOAT             inputData2,
        FLOAT             ratioData)
    {
        FLOAT  result        = 0.0f;
        DOUBLE output        = 0.0f;
        FLOAT  outputInteger = 0.0f;

        // To get only integer value. Type cast from float to int will be round down.
        INT32 input1d = static_cast<INT32>(inputData1);
        INT32 input2d = static_cast<INT32>(inputData2);

        // To get only decimal value and make big value by multiplying 1000000
        DOUBLE input1 = static_cast<DOUBLE>(inputData1 - input1d) * 1000000.0f;
        DOUBLE input2 = static_cast<DOUBLE>(inputData2 - input2d) * 1000000.0f;
        DOUBLE ratio  = static_cast<DOUBLE>(ratioData);

        // Calculate for decimal value and make proper decimal value by deviding 1000000
        output = input1 + ratio * (input2 - input1);
        result = static_cast<FLOAT>(output) / 1000000.0f;

        // Calculate for integer value
        outputInteger = static_cast<FLOAT>(input1d + ratio * static_cast<FLOAT>(input2d - input1d));

        // SUM interger and decimal
        result += outputInteger;

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolationFloatNearestNeighbour
    ///
    /// @brief  Perform Interpolation to nearest float value
    ///
    /// @param  inputData1  Input parameter one, float value
    /// @param  inputData2  Input parameter two, float value
    /// @param  ratioData   Interpolation Ratio
    ///
    /// @return The interpolation result, float value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT InterpolationFloatNearestNeighbour(
        FLOAT             inputData1,
        FLOAT             inputData2,
        FLOAT             ratioData)
    {
        FLOAT  result = 0.0f;
        DOUBLE output = 0;

        output = ((ratioData + 0.500f) >= 1.0f) ? inputData2 : inputData1;
        result = static_cast<FLOAT>(output);

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GettriggerHDRAEC
    ///
    /// @brief  Get Trigger HDR value
    ///
    /// @param  aec_hdr_control      HDR control method
    /// @param  exposureTimeTrigger  exposure time trigger
    /// @param  aecSensitivity       aec sensitivity
    /// @param  exposureGainRatio    exposure gain ratio
    ///
    /// @return The trigger value, float value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT GettriggerHDRAEC(
        ispglobalelements::tuning_control_aec_hdr_type aec_hdr_control,
        FLOAT                                          exposureTimeTrigger,
        FLOAT                                          aecSensitivity,
        FLOAT                                          exposureGainRatio)
    {
        FLOAT   triggerHDRAEC = 0.0f;  ///< HDRAEC trigger

        switch (aec_hdr_control)
        {
            case ispglobalelements::tuning_control_aec_hdr_type::control_exp_time_ratio:
                triggerHDRAEC = exposureTimeTrigger;
                break;
            case ispglobalelements::tuning_control_aec_hdr_type::control_aec_exp_sensitivity_ratio:
                triggerHDRAEC = aecSensitivity;
                break;
            case ispglobalelements::tuning_control_aec_hdr_type::control_exp_gain_ratio:
                triggerHDRAEC = exposureGainRatio;
                break;
            default:
                /// @todo (CAMX-1812) Need to add logging for Common library
                break;
        }

        return triggerHDRAEC;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GettriggerAEC
    ///
    /// @brief  Get Trigger AEC value
    ///
    /// @param  aec_exp_control  AEC control method
    /// @param  luxIndex         lux index
    /// @param  realIndex        real gain value
    ///
    /// @return The trigger value , float value
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline FLOAT GettriggerAEC(
        ispglobalelements::tuning_control_aec_type aec_exp_control,
        FLOAT                                      luxIndex,
        FLOAT                                      realGain)
    {
        FLOAT   triggerAEC = 0.0f;  ///< AEC trigger

        switch (aec_exp_control)
        {
            case ispglobalelements::tuning_control_aec_type::control_lux_idx:
                triggerAEC = luxIndex;
                break;
            case ispglobalelements::tuning_control_aec_type::control_gain:
                triggerAEC = realGain;
                break;
            default:
                /// @todo (CAMX-1812) Need to add logging for Common library
                break;
        }

        return triggerAEC;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyTriggerRegionHDRAEC
    ///
    /// @brief  Copy Trigger Region for HDRAEC Node
    ///
    /// @param  aec_hdr_control  HDR control method
    /// @param  hdr_aec_trigger  Input data
    /// @param  triggerRegion    Pointer to trigger region to be filled
    ///
    /// @return NONE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline VOID CopyTriggerRegionHDRAEC(
        ispglobalelements::tuning_control_aec_hdr_type aec_hdr_control,
        ispglobalelements::trigger_pt_type_aec_hdr*    hdr_aec_trigger,
        TriggerRegion*                                 pTriggerRegion)
    {
        if (NULL != pTriggerRegion)
        {
            switch (aec_hdr_control)
            {
                case ispglobalelements::tuning_control_aec_hdr_type::control_exp_time_ratio:
                    pTriggerRegion->start = hdr_aec_trigger->exp_time_start;
                    pTriggerRegion->end   = hdr_aec_trigger->exp_time_end;
                    break;
                case ispglobalelements::tuning_control_aec_hdr_type::control_aec_exp_sensitivity_ratio:
                    pTriggerRegion->start = hdr_aec_trigger->aec_sensitivity_start;
                    pTriggerRegion->end   = hdr_aec_trigger->aec_sensitivity_end;
                    break;
                case ispglobalelements::tuning_control_aec_hdr_type::control_exp_gain_ratio:
                    pTriggerRegion->start = hdr_aec_trigger->exp_gain_start;
                    pTriggerRegion->end   = hdr_aec_trigger->exp_gain_end;
                    break;
                default:
                    /// @todo (CAMX-1812) Need to add logging for Common library
                    break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyTriggerRegionAEC
    ///
    /// @brief Copy Trigger Region for AEC Node
    ///
    /// @param  aec_control      HDR control method
    /// @param  aec_trigger      Input data
    /// @param  triggerRegion    Pointer to trigger region to be filled
    ///
    /// @return NONE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline VOID CopyTriggerRegionAEC(
        ispglobalelements::tuning_control_aec_type aec_control,
        globalelements::trigger_pt_type_aec*       aec_trigger,
        TriggerRegion*                             pTriggerRegion)
    {
        if (NULL != pTriggerRegion)
        {
            switch (aec_control)
            {
                case ispglobalelements::tuning_control_aec_type::control_lux_idx:
                    pTriggerRegion->start = aec_trigger->lux_idx_start;
                    pTriggerRegion->end   = aec_trigger->lux_idx_end;
                    break;
                case ispglobalelements::tuning_control_aec_type::control_gain:
                    pTriggerRegion->start = aec_trigger->gain_start;
                    pTriggerRegion->end   = aec_trigger->gain_end;
                    break;
                default:
                    /// @todo (CAMX-1812) Need to add logging for Common library
                    break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyTriggerRegion
    ///
    /// @brief  Copy Trigger Region value from chromatix
    ///
    /// @param  pNodeTrigger     Pointer to the trigger region in chromatix
    /// @param  triggerRegion    Pointer to target trigger region
    ///
    /// @return NONE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline VOID CopyTriggerRegion(
        ispglobalelements::trigger_pt_type* pNodeTrigger,
        TriggerRegion*                      pTriggerRegion)
    {
        pTriggerRegion->start = pNodeTrigger->start;
        pTriggerRegion->end   = pNodeTrigger->end;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIndexPtTrigger
    ///
    /// @brief  Find out which region the input value is at.
    ///         If the input value is located between two regions, calculate the interpolation value.
    ///
    /// @param  pTriggerSet  The array of the trigger region
    /// @param  numRegion    Total number of region
    /// @param  triggerValue The input trigger value
    /// @param  pOutput0     The output value of trigger region index and interpolation ratio
    ///
    /// @return None
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void GetIndexPtTrigger(
        TriggerRegion*       pTriggerSet,
        UINT32               numRegion,
        FLOAT                triggerValue,
        InterpolationOutput* pOutput0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpolateTuningData
    ///
    /// @brief  Walk across the tree and perform interpolation to the tuning data
    ///
    /// @param  pNode            Pointer to the tip node
    /// @param  numOfNode        Number of the no leaf node
    /// @param  numOfLevel       Number of level of the tree
    /// @param  pDoInterpolation Function pointer to perform node level interpolation
    ///
    /// @return True if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL InterpolateTuningData(
        TuningNode*     pNode,
        UINT            numOfNode,
        UINT            numOfLevel,
        DoInterpolation pDoInterpolation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupInterpolationTree
    ///
    /// @brief  Setup Interpolation Tree
    ///         The tree is formed by an arry of nodes, in the following order:
    ///         level 1 ---------------------------> node[0]
    ///         level 2 ------------------>node[1]             node[2]
    ///         level 3 ------------>node[3]     node[4] node[5]      node[6]
    ///
    /// @param  pTipNode        Pointer to the first node of the tree
    /// @param  numOfLevel      Number of levels of the tree
    /// @param  pOperationTable Pointer to the level based node search function table
    /// @param  pTriggerList    Pointer to the trigger value list
    ///
    /// @return True if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL SetupInterpolationTree(
        TuningNode*    pTipNode,
        UINT           numOfLevel,
        NodeOperation* pOperationTable,
        VOID*          pTriggerList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GainCurveSampling
    ///
    /// @brief  Gain curve sampling
    ///
    /// @param  pKneeInput  Pointer to the Knee input.
    /// @param  pKneeOutput Pointer to the Knee output
    /// @param  pCoef       Pointer to the Coefficient
    /// @param  pInput      Pointer to the Input curve
    /// @param  pOutput     Pointer to the Output curve
    /// @param  lutSize     ADRC LUT Size
    /// @param  rate        rate to generate output curve.
    /// @param  gain        DRC gain
    /// @param  qFactor     qFactor
    ///
    /// @return BOOL  Return True if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GainCurveSampling(
        FLOAT*       pKneeInput,
        FLOAT*       pKneeOutput,
        FLOAT*       pCoef,
        const FLOAT* pInput,
        FLOAT*       pOutput,
        UINT32       lutSize,
        FLOAT        rate,
        FLOAT        gain,
        UINT32       qFactor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ModuleInitialize
    ///
    /// @brief  Function to Initiate the IQ Setting Module
    ///
    /// @param  pData Pointer to the IQ Library Data
    ///
    /// @return BOOL  Return True if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ModuleInitialize(
        IQLibraryData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ModuleUninitialize
    ///
    /// @brief  Function to Uninitialize the IQ Setting Module
    ///
    /// @param  pData Pointer to the IQ Library Data
    ///
    /// @return BOOL  Return True if succeed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ModuleUninitialize(
        IQLibraryData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTriggerCondition
    ///
    /// @brief  Dump Trigger Condition
    ///
    /// @param  pTriggerData Pointer to the Trigger Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpTriggerCondition(
        ISPIQTriggerData* pTriggerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MAXINT32BITFIELD
    ///
    /// @brief  Maximum Integer with given bit field
    ///
    /// @param  input  Bits for the maximum integer
    ///
    /// @return INT32  Returns the max value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT32 MAXINT32BITFIELD(
        INT32 input)
    {
        return (((input) < (1)) ? 0 : ((1ll) << ((input)-1)) - 1);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MININT32BITFIELD
    ///
    /// @brief  Minimum Integer with given bit field
    ///
    /// @param  input  Bits for the maximum integer
    ///
    /// @return INT32  Returns the min value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline INT32 MININT32BITFIELD(
        INT32 input)
    {
        return ((input) < (1)) ? 0 : (-(((1ll) << ((input)-1))));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MINUINTBITFIELD
    ///
    /// @brief  Minmum Unsigned Integer with given bit field
    ///
    /// @param  input  Bits for the maximum integer
    ///
    /// @return UINT32 Returns the min value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 MINUINTBITFIELD(
        INT input)
    {
        return input * 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MAXUINTBITFIELD
    ///
    /// @brief  Maximum Unsigned Integer with given bit field
    ///
    /// @param  input  Bits for the maximum integer
    ///
    /// @return UINT32 Returns the maxmium value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 MAXUINTBITFIELD(
        INT input)
    {
        return ((((input) < (1)) ? (0) : (((1ull) << ((input))) - 1)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Ceiling
    ///
    /// @brief  This function finds the integer ceiling of a FLOAT precision operand
    ///
    /// @param  input value whose ceiling needs to be calculated
    ///
    /// @return Integer ceiling
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 Ceiling(
        FLOAT input)
    {
        return static_cast<UINT32>(ceil(input));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Floor
    ///
    /// @brief  This function finds the integer floor of a FLOAT precision operand
    ///
    /// @param  input value whose floor needs to be calculated
    ///
    /// @return Integer ceiling
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static __inline UINT32 Floor(
        FLOAT input)
    {
        return static_cast<UINT32>(floor(input));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AlignGeneric32
    ///
    /// @brief  This function is used to align up any UINT32 value to any alignment value
    ///
    /// @param  operand     value to be aligned
    /// @param  alignment   desired alignment
    ///
    /// @return Value aligned as specified
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 AlignGeneric32(
        UINT32 operand,
        UINT   alignment)
    {
        UINT remainder = (operand % alignment);

        return (0 == remainder) ? operand : operand - remainder + alignment;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeNode
    ///
    /// @brief  Initialize an interpoloatin Node
    ///
    /// @param  pNode       Pointer to the interpolation node
    /// @param  pDataBuffer Pointer to the tuning data buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InitializeNode(
        TuningNode* pNode,
        VOID*       pDataBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddNodeToInterpolationTree
    ///
    /// @brief  Add a subnode to the interpoloation Tree
    ///
    /// @param  pParentNode      Pointer to the parent node
    /// @param  pChildNode       Pointer to the child node
    /// @param  pData            Pointer to the payload data of the child node
    /// @param  pTuningData      Pointer to the tunning data of the child node, it could be NULL
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID AddNodeToInterpolationTree(
        TuningNode* pParentNode,
        TuningNode* pChildNode,
        VOID*       pData,
        VOID*       pTuningData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IQSetHardwareVersion
    ///
    /// @brief  Set hardware version parameter
    ///
    /// @param  titanVersion       titan version parameter
    /// @param  hardwareVersion    hardware version parameter
    ///
    /// @return TRUE if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IQSetHardwareVersion(
        UINT32 titanVersion,
        UINT32 hardwareVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDynamicEnableFlag
    ///
    /// @brief  This function finds the if module needs to enabled based on hysterisis
    ///
    /// @param  dynEnTrig_enable            indicates if hysterisis is enabled
    /// @param  dynEnTrig_hyst_control_var  hysterisis control varaible
    /// @param  dynEnTrig_hyst_mode         hysterisis mode(UP/DOWN)
    /// @param  couplet_ptr                 trigger point
    /// @param  dynamicTrigger              trigger values for hysteriss
    /// @param  pEnable                     Indicates the module previous enable disable state
    ///
    /// @return Integer ceiling
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL GetDynamicEnableFlag(
        INT32                                       dynEnTrig_enable,
        ispglobalelements::control_var_type         dynEnTrig_hyst_control_var,
        ispglobalelements::hyst_direction           dynEnTrig_hyst_mode,
        ispglobalelements::trigger_pt_couplet_type* couplet_ptr,
        VOID*                                       dynamicTriggerPtr,
        BOOL*                                       pEnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PCHIPCurve
    ///
    /// @brief  This calculate the PCHIP curve
    ///
    /// @param  pKneeIn       Pointer to Knee Input data
    /// @param  pKneeOut      Pointer to Knee Output data
    /// @param  pPCHIPCoef    Pointer to PCHI coefficient
    /// @param  pIn           Pointer to Input
    /// @param  pOut          Pointer to Output
    /// @param  lutSize       size of LUT
    ///
    /// @return TRUE if calculation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL PCHIPCurve(
        FLOAT*       pKneeIn,
        FLOAT*       pKneeOut,
        FLOAT*       pPCHIPCoef,
        const FLOAT* pIn,
        FLOAT*       pOut,
        UINT32       lutSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BezierCurve
    ///
    /// @brief  This calculate the Bezier curve
    ///
    /// @param  pKneeIn       Pointer to Knee Input data
    /// @param  pKneeOut      Pointer to Knee Output data
    /// @param  pIn           Pointer to Input
    /// @param  pOut          Pointer to Output
    /// @param  lutSize       size of LUT
    ///
    /// @return TRUE if calculation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL BezierCurve(
        FLOAT*       pKneeIn,
        FLOAT*       pKneeOut,
        const FLOAT* pIn,
        FLOAT*       pOut,
        UINT32       lutSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnhanceGlobalContrast
    ///
    /// @brief  Enhance Global Contrast
    ///
    /// @param  pIn         Pointer to the input normalized data
    /// @param  pGain       Pointer to the gain data
    /// @param  pADRCData   Pointer to the ADRC data
    /// @param  percentage  GTM / LTM percentage for the output gain curve pGain
    /// @param  scaleFactor scaleFactor for LTM / GTM
    /// @param  clampFactor clampFactor for LTM / GTM
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void EnhanceGlobalContrast(
        const FLOAT*    pIn,
        FLOAT*          pGain,
        ADRCData*       pADRCData,
        FLOAT           percentage,
        UINT32          scaleFactor,
        UINT32          clampFactor);
};

#endif // IQSETTINGUTIL_H
