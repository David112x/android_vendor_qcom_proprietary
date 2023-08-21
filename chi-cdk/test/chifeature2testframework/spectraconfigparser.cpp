////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  spectraconfigparser.cpp
/// @brief Implementation of parser for spectraconfigparser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "spectraconfigparser.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "xmllib_parser.h"
#ifdef __cplusplus
}
#endif

BOOL                   SpectraConfigParser::m_initialized             = FALSE;
std::string            SpectraConfigParser::m_featureName             = "";
UINT                   SpectraConfigParser::m_frameCount              = 0;
std::string            SpectraConfigParser::m_imageFileName           = "";
std::string            SpectraConfigParser::m_outputFileNamePrefix    = "";
ChiTuningModeParameter SpectraConfigParser::m_tuningMode              = { 0 };

UINT                   SpectraConfigParser::m_usecase                 = 1;

UINT                   SpectraConfigParser::m_subModeValue0           = 0;
UINT                   SpectraConfigParser::m_subModeValue1           = 0;
UINT                   SpectraConfigParser::m_modeUsecaseSubModeType  = 0;
UINT                   SpectraConfigParser::m_modeFeature1SubModeType = 0;
UINT                   SpectraConfigParser::m_modeFeature2SubModeType = 0;
UINT                   SpectraConfigParser::m_modeSceneSubModeType    = 0;
UINT                   SpectraConfigParser::m_modeEffectSubModeType   = 0;

std::string            SpectraConfigParser::m_metaConfigName          = "";
std::string            SpectraConfigParser::m_streamConfigName        = "";
UINT                   SpectraConfigParser::m_SensorID                = 0;
UINT                   SpectraConfigParser::m_StartFrameIndex         = 1;
#define MAX_CONTENT_LENGTH 256

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SpectraConfigParser::ParseSpectraConfigXml
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SpectraConfigParser::ParseSpectraConfigXml(
    const CHAR* pSpectraConfigName)
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_initialized)
    {
        return result;
    }

    XmlParser parser(pSpectraConfigName, &result);

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH]  = { 0 };
        SIZE_T      len                          = sizeof(content);
        const CHAR* pNameArray[]                 = { "DeviceSetting", "Feature", "Name" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            m_featureName = std::string(content);
        }
        else
        {
            m_featureName.clear();
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "DeviceSetting", "Feature", "FrameCount" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_frameCount = value;
        }
        else
        {
            m_frameCount = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "DeviceSetting", "InputSetting", "ImageFile" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            m_imageFileName = std::string(content);
        }
        else
        {
            m_imageFileName.clear();
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "DeviceSetting", "OutputSetting", "OutputPrefix" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            m_outputFileNamePrefix = std::string(content);
        }
        else
        {
            m_outputFileNamePrefix.clear();
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH]  = { 0 };
        SIZE_T      len                          = sizeof(content);
        const CHAR* pNameArray[]                 = { "DeviceSetting", "Feature", "Mode" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            UINT        tokenIndex = 0;
            CHAR*       pContext   = NULL;
            const CHAR* pDelim     = "., |";
#ifdef _LINUX
            const CHAR* pToken     = strtok_r(content, pDelim, &pContext);

            while ((NULL != pToken) && (MaxTuningMode > tokenIndex))
            {
                m_tuningMode.TuningMode[tokenIndex].mode          = static_cast<ChiModeType>(tokenIndex);
                m_tuningMode.TuningMode[tokenIndex].subMode.value = static_cast<UINT16>(atoi(pToken));
                tokenIndex++;

                pToken = strtok_r(NULL, pDelim, &pContext);
            }
#else // _WINDOWS
            const CHAR* pToken = strtok_s(content, pDelim, &pContext);

            while ((NULL != pToken) && (MaxTuningMode > tokenIndex))
            {
                m_tuningMode.TuningMode[tokenIndex].mode          = static_cast<ChiModeType>(tokenIndex);;
                m_tuningMode.TuningMode[tokenIndex].subMode.value = static_cast<UINT16>(atoi(pToken));
                tokenIndex++;

                pToken = strtok_s(NULL, pDelim, &pContext);
            }
#endif // _LINUX
            m_tuningMode.noOfSelectionParameter = tokenIndex;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "UseCase" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_usecase = value;
        }
        else
        {
            m_usecase = 1;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "SubModeValue0" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_subModeValue0 = value;
        }
        else
        {
            m_subModeValue0 = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "SubModeValue1" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_subModeValue1 = value;
        }
        else
        {
            m_subModeValue1 = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "ModeUsecaseSubModeType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_modeUsecaseSubModeType = value;
        }
        else
        {
            m_modeUsecaseSubModeType = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "ModeFeature1SubModeType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_modeFeature1SubModeType = value;
        }
        else
        {
            m_modeFeature1SubModeType = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "ModeFeature2SubModeType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_modeFeature2SubModeType = value;
        }
        else
        {
            m_modeFeature2SubModeType = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "ModeSceneSubModeType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_modeSceneSubModeType = value;
        }
        else
        {
            m_modeSceneSubModeType = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value        = 0;
        const CHAR* pNameArray[] = { "TuningModeSelector", "ModeEffectSubModeType" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_modeEffectSubModeType = value;
        }
        else
        {
            m_modeEffectSubModeType = 0;
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "DeviceSetting", "InputSetting", "MetaConfig" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            m_metaConfigName = std::string(content);
        }
        else
        {
            m_metaConfigName.clear();
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value = 0;
        const CHAR* pNameArray[] = { "DeviceSetting", "Feature", "SensorID" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_SensorID = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        INT         value = 0;
        const CHAR* pNameArray[] = { "DeviceSetting", "Feature", "StartFrameIndex" };

        if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
        {
            m_StartFrameIndex = value;
        }
    }

    if (CDKResultSuccess == result)
    {
        CHAR        content[MAX_CONTENT_LENGTH] = { 0 };
        SIZE_T      len                         = sizeof(content);
        const CHAR* pNameArray[]                = { "DeviceSetting", "InputSetting", "StreamConfig" };

        if (TRUE == parser.getNamedElementContentStr(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), content, &len))
        {
            m_streamConfigName = std::string(content);
        }
        else
        {
            m_streamConfigName.clear();
        }
    }

    if (CDKResultSuccess == result)
    {
        m_initialized = TRUE;
    }

    return result;
}
