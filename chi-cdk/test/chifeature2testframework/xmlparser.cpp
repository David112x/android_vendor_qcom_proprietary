////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  xmlparser.cpp
/// @brief Implementation of xml parser.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xmlparser.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "xmllib_parser.h"
#ifdef __cplusplus
}
#endif

// Structure for storing xml data from file, current read position and size information
struct xmlData
{
    UINT32 size;   // Actual size of the file data
    UINT32 read;   // Current read position in buff
    CHAR*  buff;   // Buffer containing the xml file contents
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlPeekBytes
///
/// @brief Peek bytes callback function during xml parsing
///
/// @param pMetaInfo Pointer to metaInfo structure
/// @param offset    offset from current read position
/// @param byteCount number of bytes to peek
/// @param pBuffer   Pointer to store peeked bytes
///
/// @return XMLLIB_SUCCESS if success or XMLLIB_ERROR if fail
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static int32 XmlPeekBytes(xmllib_metainfo_s_type* pMetaInfo, int32 offset, int32 byteCount, uint8* pBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlGetBytes
///
/// @brief Consume bytes callback function during xml parsing
///
/// @param pMetaInfo Pointer to metaInfo structure
/// @param byteCount number of bytes to get
/// @param pBuffer   Pointer to store read bytes
///
/// @return XMLLIB_SUCCESS if success or XMLLIB_ERROR if fail
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static int32 XmlGetBytes(xmllib_metainfo_s_type* pMetaInfo, int32 byteCount, char* pBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlMemAlloc
///
/// @brief Alloc memory for Xml parser
///
/// @param size of bytes to allocate
///
/// @return Pointer to block of memory of size bytes, NULL if memory coultn't be allocated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static void* XmlMemAlloc(int32 size);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlMemFree
///
/// @brief Free memory for Xml parser
///
/// @param membuffer Point to memory to free
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static void XmlMemFree(void* membuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::XmlReadFile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult XmlParser::XmlReadFile(
    const CHAR* pXmlFilePath,
    VOID*       pData)
{
    xmlData*  pXmlText = static_cast<xmlData*>(pData);
    CDKResult result   = CDKResultSuccess;
    FILE*     hFile    = CdkUtils::FOpen(pXmlFilePath, "r");

    if (NULL == hFile)
    {
        CF2_LOG_ERROR("Can not open xml file %s!\n", pXmlFilePath);
        result = CDKResultEUnableToLoad;
    }
    else
    {
        CdkUtils::FSeek(hFile, 0, SEEK_END);
        pXmlText->size = CdkUtils::FTell(hFile);
        CdkUtils::FSeek(hFile, 0, SEEK_SET);

        pXmlText->buff = static_cast<CHAR*>(CHX_CALLOC(pXmlText->size));
        if (NULL == pXmlText->buff)
        {
            CF2_LOG_ERROR("Fail to alloc memory for XmlText buffer");
            pXmlText->size = 0;
            result = CDKResultENoMemory;
        }
        else
        {
            UINT32 readInSize = CdkUtils::FRead(pXmlText->buff, pXmlText->size, 1, pXmlText->size, hFile);
            if (readInSize != pXmlText->size)
            {
                CF2_LOG_ERROR("readInSize (%d)!= file size (%d)", readInSize, pXmlText->size);
                CHX_FREE(pXmlText->buff);
                pXmlText->buff = NULL;
                pXmlText->size = 0;
                result         = CDKResultEFailed;
            }
            else
            {
                pXmlText->read = 0;
                ChxUtils::Memcpy(m_xmlFilePath, pXmlFilePath, strlen(pXmlFilePath));
            }
        }
        CdkUtils::FClose(hFile);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlPeekBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 XmlPeekBytes(
    xmllib_metainfo_s_type* pMetaInfo,
    int32                   offset,
    int32                   byteCount,
    uint8*                  pBuffer)
{
    int32                   result    = XMLLIB_SUCCESS;
    xmlData*                pXmlText  = NULL;

    if ((NULL == pMetaInfo)                                              ||
        (NULL == (pXmlText = static_cast<xmlData*>(pMetaInfo->xmltext))) ||
        (offset < 0)                                                     ||
        (byteCount < 0)                                                  ||
        (NULL == pBuffer)                                                ||
        (pXmlText->read + (UINT32)offset + (UINT32)byteCount > pXmlText->size))
    {
        result = XMLLIB_ERROR;
    }

    if (XMLLIB_SUCCESS == result)
    {
        ChxUtils::Memcpy(pBuffer, pXmlText->buff + pXmlText->read + offset, (SIZE_T)byteCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlGetBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 XmlGetBytes(
    xmllib_metainfo_s_type* pMetaInfo,
    int32                   byteCount,
    char*                   pBuffer)
{
    int32                   result    = XMLLIB_SUCCESS;
    xmlData*                pXmlText  = NULL;

    if ((NULL == pMetaInfo)                                              ||
        (NULL == (pXmlText = static_cast<xmlData*>(pMetaInfo->xmltext))) ||
        (byteCount < 0)                                                  ||
        (pXmlText->read + (UINT32)byteCount > pXmlText->size))
    {
        CF2_LOG_ERROR("GetBytes() Invalid input parameters");
        result = XMLLIB_ERROR;
    }

    if (XMLLIB_SUCCESS == result)
    {
        if (NULL != pBuffer)
        {
            ChxUtils::Memcpy(pBuffer, pXmlText->buff + pXmlText->read, (SIZE_T)byteCount);
        }
        pXmlText->read += (UINT32)byteCount;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlMemAlloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void* XmlMemAlloc(int32 size)
{
    return CHX_CALLOC(static_cast<SIZE_T>(size));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlMemFree
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void XmlMemFree(void* membuffer)
{
    if (NULL != membuffer)
    {
        CHX_FREE(membuffer);
    }
    else
    {
        CHX_LOG_ERROR("Error: Free NULL pointer!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::~XmlParser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
XmlParser::~XmlParser()
{
    if (NULL != m_pXMLRoot)
    {
        xmllib_parser_free(XmlMemFree, static_cast<xmllib_parsetree_node_s_type *>(m_pXMLRoot));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::XmlParser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
XmlParser::XmlParser(const CHAR* pXmlFilePath, CDKResult* err)
    : m_pXMLRoot(NULL)
    , m_pRootElement(NULL)
{
    ChxUtils::Memset(m_xmlFilePath, 0, sizeof(m_xmlFilePath));

    CDKResult result  = CDKResultSuccess;
    xmlData   xmlText = { 0 };

    result = XmlReadFile(pXmlFilePath, static_cast<VOID*>(&xmlText));
    if (CDKResultSuccess == result)
    {
        xmllib_error_e_type error       = XMLLIB_ERROR_MAX;
        int32               xmllib_ret  = XMLLIB_SUCCESS;

        xmllib_metainfo_s_type metaInfo = { 0 };

        metaInfo.xmltext                = (void*)&xmlText;
        metaInfo.memalloc               = XmlMemAlloc;
        metaInfo.memfree                = XmlMemFree;
        metaInfo.peekbytes              = XmlPeekBytes;
        metaInfo.getbytes               = XmlGetBytes;

        xmllib_parsetree_node_s_type* pXMLRoot = NULL;
        // Invoke the XML parser and obtain the parsed tree
        xmllib_ret = xmllib_parser_parse(XMLLIB_ENCODING_UTF8, &metaInfo, &pXMLRoot, &error);
        if (XMLLIB_SUCCESS == xmllib_ret)
        {
            m_pXMLRoot = (VOID*)pXMLRoot;
            CF2_LOG_INFO("xmllib_parser_parse %s SUCCESS: %d", pXmlFilePath, xmllib_ret);
        }
        else
        {
            CF2_LOG_ERROR("xmllib_parser_parse %s Failed %d, error code= %d", pXmlFilePath, xmllib_ret, error);
        }

        if (XMLLIB_SUCCESS != xmllib_ret)
        {
            result     = CDKResultEFailed;
        }
        else
        {
            // Store root element
            for (xmllib_parsetree_node_s_type* pNode = pXMLRoot; NULL != pNode; pNode = pNode->sibling)
            {
                if (XMLLIB_PARSETREE_NODE_ELEMENT == pNode->nodetype)
                {
                    m_pRootElement = (VOID*)pNode;
                    break;
                }
            }
        }
    }

    if (NULL != xmlText.buff)
    {
        CHX_FREE(xmlText.buff);
    }

    *err = result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const VOID* XmlParser::getNamedElement(
    const CHAR* pName,
    const VOID* pStartElement)
{
    const VOID*                         pStart        = (NULL == pStartElement) ? m_pRootElement : pStartElement;
    const xmllib_parsetree_node_s_type* curNode       = static_cast<const xmllib_parsetree_node_s_type*>(pStart);
    const VOID*                         pNamedElement = NULL;

    while (NULL != curNode)
    {
        if (XMLLIB_PARSETREE_NODE_ELEMENT == curNode->nodetype)
        {
                if ((curNode->payload.element.name.len == strlen(pName)) &&
                    (0 == strncmp(pName, curNode->payload.element.name.string, (SIZE_T)(curNode->payload.element.name.len))))
                {
                    pNamedElement = static_cast<const VOID*>(curNode);
                    break;
                }
                else if (NULL != (pNamedElement = getNamedElement(pName, curNode->payload.element.child)))
                {
                    break;
                }
        }
        curNode = curNode->sibling;
    } // end of while ((NULL != curNode)) && (CDKResultSuccess == result)

    return pNamedElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const VOID* XmlParser::getNamedElement(
    const CHAR**                        ppName,
    INT                                 level,
    const VOID*                         pStartElement)
{
    const VOID*                         pStart        = (NULL == pStartElement) ? m_pRootElement : pStartElement;
    const xmllib_parsetree_node_s_type* curNode       = static_cast<const xmllib_parsetree_node_s_type*>(pStart);
    const VOID*                         pNamedElement = NULL;

    while ((NULL != curNode) && (level > 0))
    {
        if (XMLLIB_PARSETREE_NODE_ELEMENT == curNode->nodetype)
        {
            if ((curNode->payload.element.name.len == strlen(ppName[0])) &&
                (0 == strncmp(ppName[0], curNode->payload.element.name.string, (SIZE_T)(curNode->payload.element.name.len))))
            {
                if (level == 1)
                {
                    pNamedElement = static_cast<const VOID*>(curNode);
                    break;
                }
                else if (NULL != (pNamedElement = getNamedElement(&ppName[1], level - 1, curNode)))
                {
                    break;
                }
            }
            else if (NULL != (pNamedElement = getNamedElement(&ppName[0], level, curNode->payload.element.child)))
            {
                break;
            }
        }
        curNode = curNode->sibling;
    } // end of while ((NULL != curNode)) && (CDKResultSuccess == result)

    return pNamedElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getRootElement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const VOID* XmlParser::getRootElement(VOID)
{
    return m_pRootElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getElementContentStr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getElementContentStr(
    const VOID* pVOID,
    CHAR*       pContent,
    SIZE_T*     pLen)
{
    const xmllib_parsetree_node_s_type* pElement      = static_cast<const xmllib_parsetree_node_s_type*>(pVOID);
    BOOL                                isLeafElement = FALSE;
    SIZE_T                              buffLen       = *pLen;

    ChxUtils::Memset(pContent, 0, (*pLen));
    *pLen = 0;

    if ((NULL != pElement)                                    &&
        (XMLLIB_PARSETREE_NODE_ELEMENT == pElement->nodetype) &&
        (XMLLIB_PARSETREE_NODE_CONTENT == pElement->payload.element.child->nodetype))
    {
        SIZE_T contentLen = pElement->payload.element.child->payload.content.len;
        SIZE_T minLen     = (contentLen > buffLen) ? buffLen : contentLen;

        ChxUtils::Memcpy(pContent, pElement->payload.element.child->payload.content.string, minLen);
        *pLen         = minLen;
        isLeafElement = TRUE;
    }

    return isLeafElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentStr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentStr(
    const CHAR* pElementName,
    CHAR*       pContent,
    SIZE_T*     pContentLen)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(pElementName, NULL);

    if ((NULL != pElement) && (NULL != pContent))
    {
        isLeaf = getElementContentStr(pElement, pContent, pContentLen);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentStr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentStr(
    const CHAR** ppName,
    INT          level,
    CHAR*        pContent,
    SIZE_T*      pContentLen)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(ppName, level, NULL);

    if ((NULL != pElement) && (NULL != pContent))
    {
        isLeaf = getElementContentStr(pElement, pContent, pContentLen);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentInt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentInt(
    const CHAR* pElementName,
    INT*        pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(pElementName, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentInt(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentFloat(
    const CHAR* pElementName,
    FLOAT*      pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(pElementName, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentFloat(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentInt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentInt(
    const CHAR** ppName,
    INT          level,
    INT*         pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(ppName, level, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentInt(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentFloat(
    const CHAR** ppName,
    INT          level,
    FLOAT*       pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(ppName, level, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentFloat(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentULong
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentULong(
    const CHAR* pElementName,
    UINT64*     pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(pElementName, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentULong(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getNamedElementContentULong
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getNamedElementContentULong(
    const CHAR** ppName,
    INT          level,
    UINT64*      pValue)
{
    BOOL        isLeaf   = FALSE;
    const VOID* pElement = getNamedElement(ppName, level, NULL);

    if ((NULL != pElement) && (NULL != pValue))
    {
        isLeaf = getElementContentULong(pElement, pValue);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getElementNameStr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T XmlParser::getElementNameStr(
    const VOID* pVOID,
    CHAR*       pNameStr,
    SIZE_T      nameLen)
{
    const xmllib_parsetree_node_s_type* pElement = static_cast<const xmllib_parsetree_node_s_type* >(pVOID);
    SIZE_T                              minLen   = 0;

    ChxUtils::Memset(pNameStr, 0, nameLen);

    if (XMLLIB_PARSETREE_NODE_ELEMENT == pElement->nodetype)
    {
        SIZE_T minLen = (nameLen > pElement->payload.element.name.len) ? pElement->payload.element.name.len : nameLen;
        ChxUtils::Memcpy(pNameStr, pElement->payload.element.name.string, minLen);
    }

    return minLen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getElementContentInt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getElementContentInt(
    const VOID* pElement,
    INT*        pValue)
{
    BOOL   isLeaf       = FALSE;
    CHAR   content[256] = { 0 };
    SIZE_T len          = sizeof(content);

    if (NULL != pElement)
    {
        isLeaf = getElementContentStr(pElement, content, &len);
    }

    if (TRUE == isLeaf)
    {
        *pValue = strtol(content, NULL, 0);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getElementContentULong
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getElementContentULong(
    const VOID* pElement,
    UINT64*     pValue)
{
    BOOL   isLeaf       = FALSE;
    CHAR   content[256] = { 0 };
    SIZE_T len          = sizeof(content);

    if (NULL != pElement)
    {
        isLeaf = getElementContentStr(pElement, content, &len);
    }

    if (TRUE == isLeaf)
    {
        *pValue = strtoul(content, NULL, 10);
    }

    return isLeaf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// XmlParser::getElementContentFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XmlParser::getElementContentFloat(
    const VOID* pElement,
    FLOAT*      pValue)
{
    BOOL   isLeaf       = FALSE;
    CHAR   content[256] = { 0 };
    SIZE_T len          = sizeof(content);

    if (NULL != pElement)
    {
        isLeaf = getElementContentStr(pElement, content, &len);
    }

    if (TRUE == isLeaf)
    {
        *pValue = atof(content);
    }

    return isLeaf;
}

