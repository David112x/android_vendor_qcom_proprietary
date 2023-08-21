////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxhashmap.cpp
///
/// @brief Hashmap implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhashmap.h"
#include "camxmem.h"
#include "camxutils.h"
#include "camxlist.h"

CAMX_NAMESPACE_BEGIN

static const UINT   HashmapMaxNumBuckets = 1024;
static const UINT   HashFactor           = 33;
static const FLOAT  LoadFactor           = 1.0f;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Hash
/// @note: Modified Bernstein method
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT Hash(
    VOID*  pKey,
    SIZE_T len)
{
    UINT  h         = 0;
    BYTE* pKeyBytes = static_cast<BYTE*>(pKey);

    CAMX_ASSERT(NULL != pKeyBytes);

    for (UINT i = 0; i < len; i++)
    {
        h = static_cast<UINT>((static_cast<UINT64>(h) * HashFactor) ^ pKeyBytes[i]);
    }
    return h;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// KeyEqual
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL KeyEqual(
    VOID*  pKey1,
    VOID*  pKey2,
    SIZE_T len)
{
    return (0 == Utils::Memcmp(pKey1, pKey2, len));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Hashmap Function Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Hashmap* Hashmap::Create(
    const HashmapParams* pParams)
{
    Hashmap* pHashmap = NULL;

    pHashmap = CAMX_NEW_NO_SPY Hashmap();
    if (NULL != pHashmap)
    {
        CamxResult result = pHashmap->Initialize(pParams);
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE_NO_SPY pHashmap;
            pHashmap = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory; cannot create Hashmap");
    }

    return pHashmap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Hashmap::Destroy()
{
    CAMX_DELETE_NO_SPY this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Put
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::Put(
    VOID* pKey,
    VOID* pVal)
{
    CamxResult result = CamxResultSuccess;

    LightweightDoublyLinkedList* pBucket = FindBucket(pKey, TRUE);

    CAMX_ASSERT(NULL != pBucket);

    LDLLNode* pLLNode = FindElement(pBucket, pKey);
    BOOL      isFound = (NULL != pLLNode) ? TRUE : FALSE;

    // Overwrite data if we have a node and don't need to retain all nodes
    if ((TRUE == isFound) && (0 == m_params.multiMap))
    {
        HashmapNode* pNode = static_cast<HashmapNode*>(pLLNode->pData);
        CAMX_ASSERT(NULL != pNode);
        CopyValToNode(pNode, pVal);
    }
    else
    {
        LDLLNode*    pNewLLNode = static_cast<LDLLNode*>(CAMX_CALLOC_NO_SPY(sizeof(LDLLNode)));
        HashmapNode* pNode      = static_cast<HashmapNode*>(CAMX_CALLOC_NO_SPY(m_realHashmapNodeSize));

        if ((NULL != pNewLLNode) && (NULL != pNode))
        {
            CopyKeyToNode(pNode, pKey);
            CopyValToNode(pNode, pVal);

            pNewLLNode->pData = pNode;

            // If found will insert it after that node, which is required for multiMap, or at the head if not found
            pBucket->InsertAfterNode(pLLNode, pNewLLNode);

            m_numPairs++;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args, pNewLLNode or pNode is NULL");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Get
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::Get(
    VOID*   pKey,
    VOID*   pVal
    ) const
{
    CamxResult result       = CamxResultSuccess;
    VOID*      pValInPlace  = NULL;

    if ((NULL == pKey) || (NULL == pVal))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        result = GetInPlace(pKey, &pValInPlace);

        if ((CamxResultSuccess == result) && (pValInPlace != NULL))
        {
            Utils::Memcpy(pVal, pValInPlace, m_realValSize);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::GetInPlace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::GetInPlace(
    VOID*   pKey,
    VOID**  ppVal
    ) const
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pKey) || (NULL == ppVal))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        LightweightDoublyLinkedList* pBucket = FindBucket(pKey, FALSE);
        if (NULL != pBucket)
        {
            LDLLNode* pLLNode = FindElement(pBucket, pKey);
            if (NULL != pLLNode)
            {
                HashmapNode* pNode = static_cast<HashmapNode*>(pLLNode->pData);
                CAMX_ASSERT(NULL != pNode);
                *ppVal = &pNode->data[m_realKeySize];
            }
            else
            {
                result = CamxResultENoSuch;
                CAMX_LOG_VERBOSE(CamxLogGroupUtils, "Key not found");
            }
        }
        else
        {
            result = CamxResultENoSuch;
            CAMX_LOG_VERBOSE(CamxLogGroupUtils, "Key not found");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::GetValList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::GetValList(
    LightweightDoublyLinkedList& rList
    ) const
{
    CamxResult result = CamxResultSuccess;

    // Iterate through buckets to populate list with valid hashmap values
    for (UINT bucketIndex = 0; m_params.maxNumBuckets > bucketIndex; ++bucketIndex)
    {
        // If there are no nodes in the map, return nothing
        if (0 == m_numPairs)
        {
            result = CamxResultENoSuch;
            break;
        }

        // Stop searching for values when we have inserted all of them in the list
        if (m_numPairs == rList.NumNodes())
        {
            break;
        }

        if (NULL != m_ppTable[bucketIndex])
        {
            // Go through the bucket to find the values
            LDLLNode* pNode = m_ppTable[bucketIndex]->Head();
            if (NULL == pNode)
            {
                // Skip empty lists
                continue;
            }

            while (NULL != pNode)
            {
                HashmapNode* pHashNode = static_cast<HashmapNode*>(pNode->pData);

                VOID* pVal = static_cast<VOID*>(&(pHashNode->data[m_realKeySize]));

                // Create new LDLLNode
                LDLLNode* pValue = static_cast<LDLLNode*>(CAMX_CALLOC_NO_SPY(sizeof(LDLLNode)));
                if (NULL != pValue)
                {
                    pValue->pNext    = NULL;
                    pValue->pPrev    = NULL;

                    pValue->pData = static_cast<VOID*>(CAMX_CALLOC_NO_SPY(m_realValSize));
                    if (NULL != pValue->pData)
                    {
                        // Copy HashmapNode data to LDLLNode data
                        Utils::Memcpy(pValue->pData, pVal, m_realValSize);

                        // Insert LDLLNode in client's list
                        rList.InsertToTail(pValue);
                    }
                }
                pNode = LightweightDoublyLinkedList::NextNode(pNode);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Remove
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::Remove(
    VOID*  pKey)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pKey)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        LightweightDoublyLinkedList* pBucket = FindBucket(pKey, FALSE);

        CAMX_ASSERT(NULL != pBucket);

        if (NULL != pBucket)
        {
            LDLLNode* pNode = FindElement(pBucket, pKey);
            if (NULL != pNode)
            {
                pBucket->RemoveNode(pNode);

                // Delete HashmapNode
                CAMX_FREE_NO_SPY(pNode->pData);
                pNode->pData = NULL;

                // Delete LDLL
                CAMX_FREE_NO_SPY(pNode);
                pNode = NULL;
                m_numPairs--;
            }
            else
            {
                result = CamxResultENoSuch;
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
            }
        }
        else
        {
            result = CamxResultENoSuch;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::GetKey
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::GetKey(
    HashmapNode* pNode,
    VOID*        pKey
    ) const
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL != pNode) && (NULL != pKey));

    if ((NULL == pNode) || (NULL == pKey))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        Utils::Memcpy(pKey, &(pNode->data[0]), m_realKeySize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::GetVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::GetVal(
    HashmapNode* pNode,
    VOID*        pVal
    ) const
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL != pNode) && (NULL != pVal));

    if ((NULL == pNode) || (NULL == pVal))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        Utils::Memcpy(pVal, &(pNode->data[m_realKeySize]), m_realValSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::GetValInPlace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::GetValInPlace(
    HashmapNode* pNode,
    VOID**       ppVal
    ) const
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL != pNode) && (NULL != ppVal));

    if ((NULL == pNode) || (NULL == ppVal))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        *ppVal = &(pNode->data[m_realKeySize]);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Foreach
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Hashmap::Foreach(
    HashmapDataOp func,
    BOOL          remove)
{
    for (UINT bucketIndex = 0; m_params.maxNumBuckets > bucketIndex; ++bucketIndex)
    {
        if (NULL != m_ppTable[bucketIndex])
        {
            LDLLNode* pNode = m_ppTable[bucketIndex]->Head();

            while (NULL != pNode)
            {
                LDLLNode*    pNext     = LightweightDoublyLinkedList::NextNode(pNode);
                HashmapNode* pHashNode = static_cast<HashmapNode*>(pNode->pData);

                if (NULL != func)
                {
                    func(&pHashNode->data[m_realKeySize]);
                }

                if (TRUE == remove)
                {
                    // Free HashmapNode
                    CAMX_FREE_NO_SPY(pNode->pData);
                    pNode->pData = NULL;

                    // Free LDLL
                    m_ppTable[bucketIndex]->RemoveNode(pNode);
                    CAMX_FREE_NO_SPY(pNode);
                    pNode = NULL;

                    m_numPairs--;
                }

                pNode = pNext;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Hashmap Function Defintions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::~Hashmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Hashmap::~Hashmap()
{
    if (NULL != m_ppTable)
    {
        for (UINT i = 0; i < m_params.maxNumBuckets; i++)
        {
            if (NULL != m_ppTable[i])
            {
                LDLLNode* pNode = m_ppTable[i]->RemoveFromHead();

                while (NULL != pNode)
                {
                    // Free HashmapNode
                    CAMX_FREE_NO_SPY(pNode->pData);
                    pNode->pData = NULL;

                    // Free LDLL
                    CAMX_FREE_NO_SPY(pNode);
                    pNode = m_ppTable[i]->RemoveFromHead();
                }

                CAMX_DELETE_NO_SPY(m_ppTable[i]);
                m_ppTable[i] = NULL;
            }
        }
        CAMX_DELETE_NO_SPY [] m_ppTable;
        m_ppTable = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::Initialize(
    const HashmapParams* pParams)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pParams)
    {
        m_params = *pParams;
    }

    if (0 == m_params.maxNumBuckets)
    {
        m_params.maxNumBuckets = HashmapMaxNumBuckets;
    }

    if (m_params.loadFactor <= 0.0)
    {
        m_params.loadFactor = LoadFactor;
    }

    m_ppTable = CAMX_NEW_NO_SPY LightweightDoublyLinkedList*[m_params.maxNumBuckets];
    if (NULL == m_ppTable)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory; cannot create List");
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
        CAMX_ASSERT_ALWAYS();
    }
    else
    {
        m_realKeySize = m_params.keySize;
        if (0 == m_params.keySize)
        {
            m_realKeySize = sizeof(VOID*);
        }

        m_realValSize = m_params.valSize;
        if (0 == m_params.valSize)
        {
            m_realValSize = sizeof(VOID*);
        }

        m_realHashmapNodeSize = sizeof(HashmapNode) - 1 + m_realKeySize + m_realValSize;

        if (TRUE == m_params.preallocateBuckets)
        {
            for (UINT i = 0; i < m_params.maxNumBuckets; i++)
            {
                m_ppTable[i] = CAMX_NEW_NO_SPY LightweightDoublyLinkedList();
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::FindBucket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LightweightDoublyLinkedList* Hashmap::FindBucket(
    VOID* pKey,
    BOOL  createIfNotExist
    ) const
{
    UINT index = Hash(pKey, m_realKeySize) % m_params.maxNumBuckets;

    if ((TRUE == createIfNotExist) && (NULL == m_ppTable[index]))
    {
        m_ppTable[index] = CAMX_NEW_NO_SPY LightweightDoublyLinkedList();
    }

    return m_ppTable[index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::FindElement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LDLLNode* Hashmap::FindElement(
    LightweightDoublyLinkedList* pBucket,
    VOID*                        pKey
    ) const
{
    LDLLNode* pNode = NULL;

    CAMX_ASSERT(NULL != pBucket);

    if (NULL != pBucket)
    {
        VOID* pRealKey = pKey;
        if (0 == m_params.keySize)
        {
            pRealKey = &pKey;
        }

        pNode = pBucket->Head();

        while (NULL != pNode)
        {
            if ( NULL != pNode->pData)
            {
                if (TRUE == KeyEqual(&static_cast<HashmapNode*>(pNode->pData)->data[0], pRealKey, m_realKeySize))
                {
                    break;
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupUtils, "pNode->pData is NULL!");
            }

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
    }

    return pNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::CopyKeyToNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::CopyKeyToNode(
    HashmapNode* pNode,
    const VOID*  pKey)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL != pNode) && (NULL != pKey));

    if ((NULL == pNode) || (NULL == pKey))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        if (0 == m_params.keySize)
        {
            Utils::Memcpy(&pNode->data[0], &pKey, m_realKeySize);
        }
        else
        {
            Utils::Memcpy(&pNode->data[0], pKey, m_realKeySize);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashmap::CopyValToNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Hashmap::CopyValToNode(
    HashmapNode* pNode,
    const VOID*  pVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL != pNode) && (NULL != pVal));

    if ((NULL == pNode) || (NULL == pVal))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        if (0 == m_params.valSize)
        {
            Utils::Memcpy(&pNode->data[m_realKeySize], &pVal, m_realValSize);
        }
        else
        {
            Utils::Memcpy(&pNode->data[m_realKeySize], pVal, m_realValSize);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Hashmap::Clear
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Hashmap::Clear()
{
    Foreach(NULL, TRUE);
}

CAMX_NAMESPACE_END
