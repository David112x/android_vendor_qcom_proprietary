////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  streamconfigparser.cpp
/// @brief Implementation of parser for streamconfigparser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "streamconfigparser.h"
#include <sstream>
#ifdef __cplusplus
extern "C" {
#endif
#include "xmllib_parser.h"
#ifdef __cplusplus
}
#endif

BOOL                   StreamConfigParser::m_initialized                                = FALSE;
UINT                   StreamConfigParser::m_numInputStreams                            = 0;
UINT                   StreamConfigParser::m_numOutputStreams                           = 0;
CHISTREAM              StreamConfigParser::m_inputStreams[MaxNumInputStreams]           = {};
CHISTREAM              StreamConfigParser::m_outputStreams[MaxNumOutputStreams]         = {};

#define MAX_CONTENT_LENGTH 256

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StreamConfigParser::ParseStreamConfigXml
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult StreamConfigParser::ParseStreamConfigXml(
    const CHAR* pStreamConfigName)
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_initialized)
    {
        return result;
    }

    XmlParser parser(pStreamConfigName, &result);

    // Parse input streams
    if (CDKResultSuccess == result)
    {
        const VOID* element = NULL;
        for (UINT i = 0; i < MaxNumInputStreams; i++)
        {
            std::ostringstream StreamTag;
            StreamTag << "InputStream" << i;
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Type" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].streamType = static_cast<CHISTREAMTYPE>(value);
                    m_numInputStreams++;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Width" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].width = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Height" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].height = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Format" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].format = static_cast<CHISTREAMFORMAT>(value);
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "PlaneStride" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].streamParams.planeStride = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "SliceHeight" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_inputStreams[i].streamParams.sliceHeight = value;
                }
            }
        }
    }

    // Parse output streams
    if (CDKResultSuccess == result)
    {
        const VOID* element = NULL;
        for (UINT i = 0; i < MaxNumOutputStreams; i++)
        {
            std::ostringstream StreamTag;
            StreamTag << "OutputStream" << i;
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Type" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].streamType = static_cast<CHISTREAMTYPE>(value);
                    m_numOutputStreams++;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Width" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].width = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Height" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].height = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "Format" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].format = static_cast<CHISTREAMFORMAT>(value);
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "PlaneStride" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].streamParams.planeStride = value;
                }
            }
            {
                INT         value        = 0;
                std::string tag          = StreamTag.str();
                const CHAR* pNameArray[] = { "DeviceSetting", tag.c_str(), "SliceHeight" };

                if (TRUE == parser.getNamedElementContentInt(pNameArray, sizeof(pNameArray) / sizeof(CHAR*), &value))
                {
                    m_outputStreams[i].streamParams.sliceHeight = value;
                }
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        m_initialized = TRUE;
    }

    return result;
}
