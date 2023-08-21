////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  metaconfigparser.cpp
/// @brief Implementation of parser for metaconfigparser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "metaconfigparser.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "xmllib_parser.h"
#ifdef __cplusplus
}
#endif

BOOL                   MetaConfigParser::m_initialized                                = FALSE;
UINT                   MetaConfigParser::m_CCT                                        = 0;
FLOAT                  MetaConfigParser::m_LuxIdx                                     = 0.0;
FLOAT                  MetaConfigParser::m_Gain                                       = 1.0;
FLOAT                  MetaConfigParser::m_shortGain                                  = 1.0;
FLOAT                  MetaConfigParser::m_digitalGain                                = 1.0;
FLOAT                  MetaConfigParser::m_ExpSensitivityRatio                        = 1.0;
FLOAT                  MetaConfigParser::m_ExpTimeRatio                               = 1.0;
FLOAT                  MetaConfigParser::m_ExpGainRatio                               = 1.0;
FLOAT                  MetaConfigParser::m_DrcGain                                    = 1.0;
FLOAT                  MetaConfigParser::m_DrcGainDark                                = 1.0;
FLOAT                  MetaConfigParser::m_LensPosition                               = 0.0;
UINT                   MetaConfigParser::m_HDRMode                                    = 0;
FLOAT                  MetaConfigParser::m_gain_r                                     = 1.0;
FLOAT                  MetaConfigParser::m_gain_g                                     = 1.0;
FLOAT                  MetaConfigParser::m_gain_b                                     = 1.0;
UINT                   MetaConfigParser::m_HDRZrecSel                                 = 0;
UINT                   MetaConfigParser::m_HDRZrecPattern                             = 0;
UINT                   MetaConfigParser::m_HDRZrecFirstRBExp                          = 0;
FLOAT                  MetaConfigParser::m_LEDSensitivity                             = 0.0;
UINT                   MetaConfigParser::m_NumOfLed                                   = 1;
FLOAT                  MetaConfigParser::m_Led1IdxRatio                               = 0.0;
FLOAT                  MetaConfigParser::m_Led2IdxRatio                               = 0.0;
UINT                   MetaConfigParser::m_SensorType                                 = 1;
UINT64                 MetaConfigParser::m_AecExpTimeShort                            = 1;
UINT64                 MetaConfigParser::m_AecExpTimeSafe                             = 1;
UINT64                 MetaConfigParser::m_AecExpTimeLong                             = 1;
BOOL                   MetaConfigParser::m_CCMIFEFlag                                 = FALSE;
FLOAT                  MetaConfigParser::m_CCMIFEMatrix[AWBNumCCMRows][AWBNumCCMCols] = { { 0 } };
FLOAT                  MetaConfigParser::m_CCMIFEOffset[AWBNumCCMRows]                = { 0 };
BOOL                   MetaConfigParser::m_CCMBPSFlag                                 = FALSE;
FLOAT                  MetaConfigParser::m_CCMBPSMatrix[AWBNumCCMRows][AWBNumCCMCols] = { { 0 } };
FLOAT                  MetaConfigParser::m_CCMBPSOffset[AWBNumCCMRows]                = { 0 };
BOOL                   MetaConfigParser::m_CCMIPEFlag                                 = FALSE;
FLOAT                  MetaConfigParser::m_CCMIPEMatrix[AWBNumCCMRows][AWBNumCCMCols] = { { 0 } };
FLOAT                  MetaConfigParser::m_CCMIPEOffset[AWBNumCCMRows]                = { 0 };

#define MAX_CONTENT_LENGTH 256

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MetaConfigParser::ParseMetaConfigXml
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MetaConfigParser::ParseMetaConfigXml(
    const CHAR* pMetaConfigName)
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_initialized)
    {
        return result;
    }

    XmlParser parser(pMetaConfigName, &result);

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "CCT" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_CCT = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "LuxIdx" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_LuxIdx = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "Gain" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_Gain = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "shortGain" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_shortGain = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "digitalGain" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_digitalGain = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "ExpSensitivityRatio" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_ExpSensitivityRatio = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "ExpTimeRatio" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_ExpTimeRatio = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "ExpGainRatio" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_ExpGainRatio = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "DrcGain" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_DrcGain = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "DrcGainDark" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_DrcGainDark = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "LensPosition" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_LensPosition = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "HDRMode" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_HDRMode = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value      = 0;
        const CHAR* pNameArray[] = { "gain_r" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_gain_r = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value      = 0;
        const CHAR* pNameArray[] = { "gain_g" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_gain_g = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT       value        = 0;
        const CHAR* pNameArray[] = { "gain_b" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_gain_b = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "HDRZrecSel" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_HDRZrecSel = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "HDRZrecPattern" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_HDRZrecPattern = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "HDRZrecFirstRBExp" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_HDRZrecFirstRBExp = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "LEDSensitivity" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_LEDSensitivity = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "NumOfLed" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_NumOfLed = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "Led1IdxRatio" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_Led1IdxRatio = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        FLOAT         value        = 0;
        const CHAR*   pNameArray[] = { "Led2IdxRatio" };

        if (TRUE == parser.getNamedElementContentFloat(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_Led2IdxRatio = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "SensorType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_SensorType = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        UINT64      value        = 0;
        const CHAR* pNameArray[] = { "aec_exp_time", "Short" };

        if (TRUE == parser.getNamedElementContentULong(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_AecExpTimeShort = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        UINT64      value        = 0;
        const CHAR* pNameArray[] = { "aec_exp_time", "Safe" };

        if (TRUE == parser.getNamedElementContentULong(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_AecExpTimeSafe = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        UINT64      value        = 0;
        const CHAR* pNameArray[] = { "aec_exp_time", "Long" };

        if (TRUE == parser.getNamedElementContentULong(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_AecExpTimeLong = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "CCM", "IFE", "Flag" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_CCMIFEFlag = static_cast<BOOL>(value);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "IFE", "Matrix" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |\n";

            UINT numOfElem         = sizeof(m_CCMIFEMatrix) / sizeof(m_CCMIFEMatrix[0][0]);
            UINT numCol            = sizeof(m_CCMIFEMatrix[0]) / sizeof(m_CCMIFEMatrix[0][0]);
#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMIFEMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMIFEMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "IFE", "Offset" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |";

#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMIFEOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMIFEOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value = 0;
        const CHAR* pNameArray[] = { "CCM", "BPS", "Flag" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_CCMBPSFlag = static_cast<BOOL>(value);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "BPS", "Matrix" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |\n";

            UINT numOfElem         = sizeof(m_CCMBPSMatrix) / sizeof(m_CCMBPSMatrix[0][0]);
            UINT numCol            = sizeof(m_CCMBPSMatrix[0]) / sizeof(m_CCMBPSMatrix[0][0]);
#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMBPSMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMBPSMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "BPS", "Offset" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |";

#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMBPSOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMBPSOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value = 0;
        const CHAR* pNameArray[] = { "CCM", "IPE", "Flag" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_CCMIPEFlag = static_cast<BOOL>(value);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "IPE", "Matrix" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |\n";

            UINT numOfElem         = sizeof(m_CCMIPEMatrix) / sizeof(m_CCMIPEMatrix[0][0]);
            UINT numCol            = sizeof(m_CCMIPEMatrix[0]) / sizeof(m_CCMIPEMatrix[0][0]);
#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMIPEMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (numOfElem > tokenIndex))
            {
                m_CCMIPEMatrix[tokenIndex / numCol][tokenIndex % numCol] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "CCM", "IPE", "Offset" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = ", |";

#ifdef _LINUX
            const CHAR* pToken = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMIPEOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (AWBNumCCMRows > tokenIndex))
            {
                m_CCMIPEOffset[tokenIndex] = atof(pToken);
                tokenIndex++;
                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
        }
    }

    if (CDKResultSuccess == result)
    {
        m_initialized = TRUE;
    }

    return result;
}
