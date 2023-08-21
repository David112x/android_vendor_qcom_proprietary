////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxresourcemanager.cpp
/// @brief ResourceManager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxresourcemanager.h"

// NOWHINE FILE CP006:  Need whiner update: std::map allowed in exceptional cases

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceManager* ResourceManager::Create(
    ResourceMgrCreateInfo* pCreateInfo)
{
    CDKResult        result           = CDKResultSuccess;
    ResourceManager* pResourceManager = CAMX_NEW ResourceManager();

    if (NULL == pResourceManager)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory: pResourceManager is NULL");
    }
    else
    {
        result = pResourceManager->Initialize(pCreateInfo);

        if (CDKResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "ResourceManager failed to initialize!!");
            pResourceManager->Destroy();
            pResourceManager = NULL;
        }
    }

    return pResourceManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::Initialize(
    ResourceMgrCreateInfo* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pCreateInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input params");
        result = CDKResultEInvalidArg;
    }
    else
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore, "ResourceManager: %p, name: %s", this, pCreateInfo->pResourceMgrName);

        m_pName = pCreateInfo->pResourceMgrName;
        m_pLock = Mutex::Create("ResMgrLock");

        if (NULL == m_pLock)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Fail to create m_pLock");
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::~ResourceManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceManager::~ResourceManager()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::RegisterResourceWithClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::RegisterResourceWithClient(
    const ResourceInputInfo*  pResourceInfo,
    const ClientInputInfo*    pClientInfo)
{
    CDKResult                    result        = CDKResultSuccess;
    ResourceMgrResourceDataInfo* pResourceData = NULL;

    result = ValidateResourceAndClientParams(pResourceInfo, pClientInfo);

    if (CDKResultSuccess == result)
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore, "resource {%s, %d}, client {%s, %p}",
            pResourceInfo->pResourceName,
            pResourceInfo->resourceId,
            pClientInfo->pClientName,
            pClientInfo->hClient);

        m_pLock->Lock();

        std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it;

        it = m_pResourceIdToDataMap.find(pResourceInfo->resourceId);

        if (it == m_pResourceIdToDataMap.end())
        {
            // find the first unused slot
            for (UINT32 i = 0; i < MaxResourceNum; ++i)
            {
                if (FALSE == m_resourceData[i].isValid)
                {
                    pResourceData = &m_resourceData[i];
                    break;
                }
            }

            if (NULL == pResourceData)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "All slots are used, total slot num:%d", MaxResourceNum);
                result = CDKResultEFailed;
            }

            if (CDKResultSuccess == result)
            {
                // initialize resource data info structure
                Utils::Memset(pResourceData, 0, sizeof(ResourceMgrResourceDataInfo));

                pResourceData->isValid                = TRUE;
                pResourceData->resourceId             = pResourceInfo->resourceId;
                pResourceData->pResourceName          = pResourceInfo->pResourceName;
                pResourceData->totalAvailableResource = pResourceInfo->totalAvailableResource;
                pResourceData->remainingResource      = pResourceData->totalAvailableResource;
                pResourceData->pLock                  = Mutex::Create("ResourceManagerPerResourceLock");
                pResourceData->pCondition             = Condition::Create("ResourceManagerPerResourceCondition");
                pResourceData->numWaitingClients      = 0;

                if ((NULL == pResourceData->pLock) || (NULL == pResourceData->pCondition))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Fail to create pLock or pCondition");
                    result = CDKResultENoMemory;
                }

                if (CDKResultSuccess == result)
                {
                    pResourceData->pClients.reserve(MaxClientsPerResource);

                    m_pResourceIdToDataMap.insert({pResourceInfo->resourceId, pResourceData});

                    m_validResourceNum++;

                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resource {%s, %d}, pResourceData: %p, m_validResourceNum: %d",
                        pResourceData->pResourceName, pResourceData->resourceId, pResourceData, m_validResourceNum);
                }
            }
        }
        else
        {
            pResourceData = (*it).second;
        }

        if ((CDKResultSuccess == result) && (NULL != pResourceData))
        {
            pResourceData->pLock->Lock();

            // add current client
            ResourceClientInfo client = {0};

            client.hClient      = pClientInfo->hClient;
            client.pClientName  = pClientInfo->pClientName;
            client.clientCbOps  = *(pClientInfo->pCallbackOps);
            client.pPrivateData = pClientInfo->pPrivateData;
            client.resourceCost = pClientInfo->maxResourceCost;
            client.refCount     = 0;

            SetClientState(&client, ResourceClientState::Inactive);

            pResourceData->pClients.push_back(client);

            CAMX_LOG_CONFIG(CamxLogGroupCore, "Add client {%s, %p}, cost: %d, clients size: %d",
                client.pClientName, client.hClient, client.resourceCost, pResourceData->pClients.size());

            pResourceData->pLock->Unlock();
        }

        m_pLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input parameters");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::UnregisterResourceWithClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::UnregisterResourceWithClient(
    ResourceID              resourceId,
    ResourceClientHandle    hClient)
{
    CDKResult result = CDKResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resourceId: %d, client: %p", resourceId, hClient);

    m_pLock->Lock();

    std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it;

    it = m_pResourceIdToDataMap.find(resourceId);

    if (it != m_pResourceIdToDataMap.end())
    {
        ResourceMgrResourceDataInfo* pResourceData = it->second;

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "pResourceData: %p, clients size: %d",
            pResourceData, static_cast<UINT32>(pResourceData->pClients.size()));

        pResourceData->pLock->Lock();

        // delete the client from client array
        for (auto clientIter = pResourceData->pClients.begin(); clientIter != pResourceData->pClients.end(); ++clientIter)
        {
            if (hClient == clientIter->hClient)
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "removing client {%s, %p}", clientIter->pClientName, hClient);

                pResourceData->pClients.erase(clientIter);

                break;
            }
        }

        pResourceData->pLock->Unlock();

        if (0 == pResourceData->pClients.size())
        {
            // destroy the whole pResourceData!

            if (NULL != pResourceData->pLock)
            {
                pResourceData->pLock->Destroy();
                pResourceData->pLock = NULL;
            }

            if (NULL != pResourceData->pCondition)
            {
                pResourceData->pCondition->Destroy();
                pResourceData->pCondition = NULL;
            }

            if (0 < pResourceData->numWaitingClients)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort to debug. pending clients: %d",
                    pResourceData->numWaitingClients);
                OsUtils::RaiseSignalAbort();
            }

            pResourceData->pClients.shrink_to_fit();
            pResourceData->isValid = FALSE;

            m_pResourceIdToDataMap.erase(it);

            m_validResourceNum--;

            CAMX_LOG_INFO(CamxLogGroupCore, "m_validResourceNum: %d", m_validResourceNum);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid Resource Id: %d", resourceId);
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::CheckAndAcquireResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::CheckAndAcquireResource(
    ResourceID                  resourceId,
    const ResourceClientHandle  hClient,
    UINT32                      timeout)
{
    CDKResult                    result        = CDKResultSuccess;
    ResourceMgrResourceDataInfo* pResourceData = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resourceId: %d, handle: %p, timeout: %d", resourceId, hClient, timeout);

    m_pLock->Lock();
    std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it = m_pResourceIdToDataMap.find(resourceId);
    if (it != m_pResourceIdToDataMap.end())
    {
        pResourceData = it->second;
    }
    m_pLock->Unlock();

    if (NULL != pResourceData)
    {
        // 1. if currently client has already occupied the resource (acquired or inUse), then go ahead
        // 2. otherwise, check if remainingResource is enough for this client.
        // 2.1   if remainingResource is enough, go ahead
        // 2.2   otherwise, must waiting other clients to release resource, then go ahead

        pResourceData->pLock->Lock();

        ResourceClientInfo* pClientInfo = GetResourceClientInfo(pResourceData, hClient);

        if (NULL == pClientInfo)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "client: %p NOT registered for resource {%s, %d}",
                hClient, pResourceData->pResourceName, pResourceData->resourceId);

            result = CDKResultENoSuch;
        }

        if (CDKResultSuccess == result)
        {
            if (ResourceClientState::Inactive == pClientInfo->clientState)
            {
                // check if remaining resource is enough or not,
                // if not enough, need to trigger other clients to release resource

                CAMX_LOG_VERBOSE(CamxLogGroupCore, "client resource cost: %d, available resource: %d",
                    pClientInfo->resourceCost, pResourceData->remainingResource);

                if (pClientInfo->resourceCost > pResourceData->remainingResource)
                {
                    // if some clients already in ResourceAcquired state (occupied, but not using),
                    // release their resource first.
                    for (std::vector<ResourceClientInfo>::iterator pClientIter = pResourceData->pClients.begin();
                         pClientIter != pResourceData->pClients.end();
                         pClientIter++)
                    {
                        if (pClientIter->hClient != hClient)
                        {
                            if ((ResourceClientState::ResourceAcquired == pClientIter->clientState) &&
                                (0 == pClientIter->refCount))
                            {
                                if (NULL != pClientIter->clientCbOps.ReleaseResourceFunc)
                                {
                                    CAMX_LOG_INFO(CamxLogGroupCore,
                                        "calling ReleaseResourceFunc, client {%s, %p}, release func:%p",
                                        pClientIter->pClientName,
                                        pClientIter->hClient,
                                        pClientIter->clientCbOps.ReleaseResourceFunc);

                                    pClientIter->clientCbOps.ReleaseResourceFunc(
                                        pClientIter->hClient, pClientIter->pPrivateData);
                                }

                                pClientIter->clientState = ResourceClientState::Inactive;

                                pResourceData->remainingResource += pClientIter->resourceCost;

                                CAMX_LOG_INFO(CamxLogGroupCore, "remainingResource: %d", pResourceData->remainingResource);

                                if (pClientInfo->resourceCost <= pResourceData->remainingResource)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    // If still not enough resource, then wait for active clients to release their reference
                    if (pClientInfo->resourceCost > pResourceData->remainingResource)
                    {
                        pResourceData->numWaitingClients++;

                        result = WaitTillResourceAvailabe(pResourceData, pClientInfo->resourceCost, timeout);

                        if (CDKResultSuccess == result)
                        {
                            pResourceData->numWaitingClients--;
                        }
                    }
                }

                if ((CDKResultSuccess == result) &&
                    (pClientInfo->resourceCost <= pResourceData->remainingResource))
                {
                    CAMX_LOG_INFO(CamxLogGroupCore, "calling client's AcquireResourceFunc, client {%s, %p}, acquire func:%p",
                        pClientInfo->pClientName, pClientInfo->hClient, pClientInfo->clientCbOps.AcquireResourceFunc);

                    if (NULL != pClientInfo->clientCbOps.AcquireResourceFunc)
                    {
                        result = pClientInfo->clientCbOps.AcquireResourceFunc(pClientInfo->hClient, pClientInfo->pPrivateData);
                    }

                    if (CDKResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupCore, "Acquire Resource failed");
                    }
                    else
                    {
                        if (pResourceData->remainingResource >= pClientInfo->resourceCost)
                        {
                            pResourceData->remainingResource -= pClientInfo->resourceCost;
                        }

                        SetClientState(pClientInfo, ResourceClientState::ResourceAcquired);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Not enough resource availabe! remaining: %d, required: %d",
                        pResourceData->remainingResource, pClientInfo->resourceCost);

                    result = CDKResultENoMore;
                }
            }
        }

        pResourceData->pLock->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid resourceId: %d", resourceId);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::SetResourceAcquired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::SetResourceAcquired(
    ResourceID                  resourceId,
    const ResourceClientHandle  hClient)
{
    CDKResult result = CDKResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resourceId: %d, handle: %p", resourceId, hClient);

    std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it = m_pResourceIdToDataMap.find(resourceId);

    if (it != m_pResourceIdToDataMap.end())
    {
        ResourceMgrResourceDataInfo* pResourceData = it->second;
        ResourceClientInfo*          pClientInfo   = GetResourceClientInfo(pResourceData, hClient);

        if (NULL == pClientInfo)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "client: %p NOT regisgtred for resource {%s, %d]",
                hClient, pResourceData->pResourceName, pResourceData->resourceId);
            result = CDKResultENoSuch;
        }

        if (CDKResultSuccess == result)
        {
            if (ResourceClientState::Inactive == pClientInfo->clientState)
            {
                if (pResourceData->remainingResource > pClientInfo->resourceCost)
                {
                    SetClientState(pClientInfo, ResourceClientState::ResourceAcquired);
                    pResourceData->remainingResource -= pClientInfo->resourceCost;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Not enough resource availabe! remaining: %d, required: %d",
                        pResourceData->remainingResource, pClientInfo->resourceCost);

                    result = CDKResultENoMore;
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Already acquired! resource {%s, %d}, client {%s, %p}, state: %d",
                    pResourceData->pResourceName, pResourceData->resourceId,
                    pClientInfo->pClientName, pClientInfo->hClient, pClientInfo->clientState);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid resourceId: %d", resourceId);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::AddResourceReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::AddResourceReference(
    ResourceID                  resourceId,
    const ResourceClientHandle  hClient,
    UINT32                      timeout)
{
    CDKResult                    result        = CDKResultSuccess;
    ResourceMgrResourceDataInfo* pResourceData = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resourceId: %d, handle: %p", resourceId, hClient);

    m_pLock->Lock();
    std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it = m_pResourceIdToDataMap.find(resourceId);
    if (it != m_pResourceIdToDataMap.end())
    {
        pResourceData = it->second;
    }
    m_pLock->Unlock();

    if (NULL != pResourceData)
    {
        result = CheckAndAcquireResource(resourceId, hClient, timeout);

        if (CDKResultSuccess == result)
        {
            pResourceData->pLock->Lock();

            ResourceClientInfo* pClientInfo = GetResourceClientInfo(pResourceData, hClient);

            if (NULL == pClientInfo)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "client: %p NOT registered for resource {%s, %d}",
                    hClient, pResourceData->pResourceName, pResourceData->resourceId);

                result = CDKResultENoSuch;
            }

            if (CDKResultSuccess == result)
            {
                if ((ResourceClientState::ResourceAcquired == pClientInfo->clientState) ||
                    (ResourceClientState::ResourceInUse    == pClientInfo->clientState))
                {
                    // already occupied the resource, just add reference
                    pClientInfo->refCount++;

                    if (ResourceClientState::ResourceAcquired == pClientInfo->clientState)
                    {
                        SetClientState(pClientInfo, ResourceClientState::ResourceInUse);
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "resource {%s, %d}, client {%s, %p}, refCount: %d",
                        pResourceData->pResourceName, resourceId,
                        pClientInfo->pClientName, hClient, pClientInfo->refCount);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid client state! client {%s, %p}, state: %d",
                        pClientInfo->pClientName, pClientInfo->hClient, pClientInfo->clientState);
                }
            }

            pResourceData->pLock->Unlock();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid resourceId: %d", resourceId);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::ReleaseResourceReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::ReleaseResourceReference(
    ResourceID                  resourceId,
    const ResourceClientHandle  hClient,
    BOOL                        lazyMode)
{
    CDKResult                    result        = CDKResultSuccess;
    ResourceMgrResourceDataInfo* pResourceData = NULL;

    if (CDKResultSuccess == result)
    {
        m_pLock->Lock();
        std::map<ResourceID, ResourceMgrResourceDataInfo*>::iterator it = m_pResourceIdToDataMap.find(resourceId);
        if (it != m_pResourceIdToDataMap.end())
        {
            pResourceData = it->second;
        }
        m_pLock->Unlock();
    }

    if ((CDKResultSuccess == result) && (NULL != pResourceData))
    {
        pResourceData->pLock->Lock();

        for (auto& client : pResourceData->pClients)
        {
            if (client.hClient == hClient)
            {
                client.refCount--;

                CAMX_LOG_VERBOSE(CamxLogGroupCore, "resource {%s, %d}, client {%s, %p}, refCount: %d, lazyMode: %d",
                    pResourceData->pResourceName, resourceId, client.pClientName, hClient, client.refCount, lazyMode);

                if (0 == client.refCount)
                {
                    // Idle state mean client still occupy the resource, but not actually using it.
                    client.clientState = ResourceClientState::ResourceAcquired;

                    if ((FALSE == lazyMode) ||
                        (0 < GetNumOfWaitingClients(pResourceData)))
                    {
                        if (NULL != client.clientCbOps.ReleaseResourceFunc)
                        {
                            CAMX_LOG_INFO(CamxLogGroupCore, "calling ReleaseResourceFunc, client {%s, %p}, release func:%p",
                                client.pClientName, client.hClient, client.clientCbOps.ReleaseResourceFunc);

                            client.clientCbOps.ReleaseResourceFunc(client.hClient, client.pPrivateData);
                        }

                        client.clientState = ResourceClientState::Inactive;

                        pResourceData->remainingResource += client.resourceCost;

                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "remainingResource: %d", pResourceData->remainingResource);
                    }

                    // notify resource availabe.
                    pResourceData->pCondition->Signal();
                }
            }
        }

        pResourceData->pLock->Unlock();
    }
    else
    {
        result = CDKResultEFailed;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ResourceManager::Destroy()
{
    for (UINT i = 0; i < MaxResourceNum; i++)
    {
        ResourceMgrResourceDataInfo* pResourceData = &m_resourceData[i];

        if (TRUE == pResourceData->isValid)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "resource: %s may still in use, destroy anyway!", pResourceData->pResourceName);

            if (NULL != pResourceData->pLock)
            {
                pResourceData->pLock->Destroy();
                pResourceData->pLock = NULL;
            }

            if (NULL != pResourceData->pCondition)
            {
                pResourceData->pCondition->Destroy();
                pResourceData->pCondition = NULL;
            }

            pResourceData->pClients.clear();
            pResourceData->pClients.shrink_to_fit();

            pResourceData->isValid = FALSE;
        }
    }

    if (NULL != m_pLock)
    {
        m_pLock->Destroy();
        m_pLock = NULL;
    }

    m_pResourceIdToDataMap.clear();

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::WaitTillResourceAvailabe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::WaitTillResourceAvailabe(
    const ResourceMgrResourceDataInfo* pResourceData,
    UINT32                             requiredResource,
    UINT32                             timeout)
{
    CDKResult result = CDKResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "remaining resource: %d, required cost: %d, timeout: %d",
        pResourceData->remainingResource, requiredResource, timeout);

    while (requiredResource > pResourceData->remainingResource)
    {
        pResourceData->pCondition->Wait(pResourceData->pLock->GetNativeHandle());
    }

    CAMX_LOG_INFO(CamxLogGroupCore, "now remaining resource: %d", pResourceData->remainingResource);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceManager::ValidateResourceAndClientParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ResourceManager::ValidateResourceAndClientParams(
    const ResourceInputInfo*  pResourceInfo,
    const ClientInputInfo*    pClientInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pResourceInfo)                                  ||
        (NULL == pResourceInfo->pResourceName)                   ||
        (NULL == pClientInfo)                                    ||
        (NULL == pClientInfo->hClient)                           ||
        (NULL == pClientInfo->pClientName)                       ||
        (NULL == pClientInfo->pPrivateData)                      ||
        (NULL == pClientInfo->pCallbackOps)                      ||
        (NULL == pClientInfo->pCallbackOps->AcquireResourceFunc) ||
        (NULL == pClientInfo->pCallbackOps->ReleaseResourceFunc))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input params");
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (0 >= pResourceInfo->totalAvailableResource))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid totalAvailableResource");
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (pClientInfo->minResourceCost != pClientInfo->maxResourceCost))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Don't support variable resource cost per client!");
        result = CDKResultENotImplemented;
    }

    if ((CDKResultSuccess == result) && (pClientInfo->maxResourceCost > pResourceInfo->totalAvailableResource))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Client requires resource (%d) exceed max availabe resource (%d)",
            pClientInfo->maxResourceCost, pResourceInfo->totalAvailableResource);
        result = CDKResultEInvalidArg;
    }

    return result;
}

CAMX_NAMESPACE_END
