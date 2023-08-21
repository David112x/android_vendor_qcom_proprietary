////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxresourcemanager.h
/// @brief ResourceManager class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXRESOURCEMANAGER_H
#define CAMXRESOURCEMANAGER_H

#include <map>
#include <vector>

#include "camxincs.h"

// NOWHINE FILE CP006:  Need whiner update: std::map allowed in exceptional cases

CAMX_NAMESPACE_BEGIN

typedef VOID*  ResourceClientHandle;
typedef UINT32 ResourceID;

static const UINT32 MaxResourceNum        = 5;
static const UINT32 MaxClientsPerResource = 2;

/// @brief resource type
enum class ResourceType
{
    RealtimePipeline = 1,   ///< Whole realtime pipeline as a resource
    SensorHw         = 2,   ///< Sensor HW resource type
    IFEHw            = 3,   ///< IFE resource type
};

/// @brief resource manager callbacks
struct ResourceMgrCallbackOps
{
    /// @brief Callback method to acquire resource for client
    CamxResult (*AcquireResourceFunc)(
        VOID*   pClientHandle,
        VOID*   pPrivateCallbackData);

    /// @brief Callback method to prepare release resource for client
    CamxResult (*PrepareReleaseResourceFunc)(
        VOID*   pClientHandle,
        VOID*   pPrivateCallbackData);

    /// @brief Callback method to release resource for client
    CamxResult (*ReleaseResourceFunc)(
        VOID*   pClientHandle,
        VOID*   pPrivateCallbackData);
};

/// @brief client state for a resource
enum class ResourceClientState
{
    Inactive = 0,     ///< client never acquired resource, or has released the resource
    ResourceAcquired, ///< Client acquired the resource, but currently not using it,
                      ///  resource manager can call releaseResource callback to client to safely release resource
    ResourceInUse     ///< Client acquired and is using the resource, can't release resource directly
};

/// @brief client info for a resource
struct ResourceClientInfo
{
    ResourceClientHandle   hClient;       ///< Client handle
    const CHAR*            pClientName;   ///< Client name
    UINT32                 resourceCost;  ///< The resource cost for the client
    ResourceClientState    clientState;   ///< Client state
    UINT32                 refCount;      ///< The reference count of using the resource by this client
    ResourceMgrCallbackOps clientCbOps;   ///< Callbacks the resource manager will call to client
    VOID*                  pPrivateData;  ///< Callback private data
};

/// @brief data structure to describe a resource
struct ResourceMgrResourceDataInfo
{
    BOOL                            isValid;                ///< This structure is in use or not
    const CHAR*                     pResourceName;          ///< The name of the resource
    ResourceID                      resourceId;             ///< The ID of the resource
    UINT32                          totalAvailableResource; ///< Use interger value to describe the total availability of
                                                            ///  the resource, a client my use part or full of the resource
    UINT32                          remainingResource;      ///< The remaining resource can be used by new client
    Mutex*                          pLock;                  ///< Lock per resource
    Condition*                      pCondition;             ///< Condition per resource
    std::vector<ResourceClientInfo> pClients;               ///< A list of clients registered with this resource
    UINT32                          numWaitingClients;      ///< Num of clients who are waiting for resource
};

/// @brief resource manager create info
struct ResourceMgrCreateInfo
{
    const CHAR*         pResourceMgrName;                   ///< This structure Name
};

/// @brief input info about a resource
struct ResourceInputInfo
{
    ResourceID    resourceId;                               ///< The ID of the resource
    const CHAR*   pResourceName;                            ///< The name of the resource
    UINT32        totalAvailableResource;                   ///< Total available resource
};

/// @brief input info for a client of a resource
struct ClientInputInfo
{
    ResourceClientHandle    hClient;                        ///< Client Handle
    const CHAR*             pClientName;                    ///< Client Name
    UINT32                  minResourceCost;                ///< Max resource cost the client may use
    UINT32                  maxResourceCost;                ///< Min resource cost the client may use
    ResourceMgrCallbackOps* pCallbackOps;                   ///< Callbacks the resource manager will call to client
    VOID*                   pPrivateData;                   ///< Callback private data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief ResourceManager class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ResourceManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief   Create the Resource Manager
    ///
    /// @param   pCreateInfo    data needed to create the resource manager.
    ///
    /// @return  Pointer of ResourceManager if successful or NULL in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ResourceManager* Create(
            ResourceMgrCreateInfo* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterResourceWithClient
    ///
    /// @brief   Register a client for the given resource info
    ///
    /// @param   pResourceInfo    Resource info.
    /// @param   pClientInfo      Client info.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RegisterResourceWithClient(
        const ResourceInputInfo*  pResourceInfo,
        const ClientInputInfo*    pClientInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterResourceWithClient
    ///
    /// @brief   Unregister a client for the given resource ID
    ///
    /// @param   resourceId     The resource ID.
    /// @param   hClient        The client handle need to unregister
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UnregisterResourceWithClient(
        ResourceID              resourceId,
        ResourceClientHandle    hClient);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndAcquireResource
    ///
    /// @brief   Check and require acquire the resource of the given resource ID.
    ///          If resource is not availabe, it will wait until other clients resource resource.
    ///          After aquire resrouce successfully, the client will set its state to ResourceAcquired.
    ///
    /// @param   resourceId     The resource ID.
    /// @param   hClient        The client handle
    /// @param   timeout        Waiting time if resource is not availabe, zero means wait forever.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CheckAndAcquireResource(
        ResourceID                  resourceId,
        const ResourceClientHandle  hClient,
        UINT32                      timeout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetResourceAcquired
    ///
    /// @brief   Set a client acquire the resource of the give resour ID.
    ///
    /// @param   resourceId     The resource ID.
    /// @param   hClient        The client handle
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetResourceAcquired(
        ResourceID                  resourceId,
        const ResourceClientHandle  hClient);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddResourceReference
    ///
    /// @brief   Add resource reference for a client
    ///
    /// @param   resourceId    The unique resoure id
    /// @param   hClient       The handle of the client
    /// @param   timeout       Waiting time if resource is not availabe, zero means wait forever.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AddResourceReference(
        ResourceID                  resourceId,
        const ResourceClientHandle  hClient,
        UINT32                      timeout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResourceReference
    ///
    /// @brief   Release the reference for a given resource ID
    ///
    /// @param   resourceId    The unique resoure id
    /// @param   hClient       The handle of the client
    /// @param   lazyMode      If lazyMode is not set, call client to actually release resource when refcount is 0,
    ///                        otherwise don't release the resource, just set the state to IDLE.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseResourceReference(
        ResourceID                  resourceId,
        const ResourceClientHandle  hClient,
        BOOL                        lazyMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief   Flush operation during usecase flush
    ///
    /// @param   resourceId    The unique resoure id
    /// @param   hClient       The handle of the client
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush(
        ResourceID                  resourceId,
        const ResourceClientHandle  hClient);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief   Initialize the resource manager
    ///
    /// @param   pCreateInfo    data needed to create the resource manager.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        ResourceMgrCreateInfo* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateResourceAndClientParams
    ///
    /// @brief   Check if the input resource info and client info is valid or not
    ///
    /// @param   pResourceInfo    Resource info.
    /// @param   pClientInfo      Client info.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ValidateResourceAndClientParams(
        const ResourceInputInfo*  pResourceInfo,
        const ClientInputInfo*    pClientInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetResourceClientInfo
    ///
    /// @brief   Get the client info for the given client handle with in the resource data structure.
    ///
    /// @param   pResourceData    The resource data structure.
    /// @param   hClient          The handle of the client
    ///
    /// @return  Pointer of ResourceClientInfo if successful or NULL in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ResourceClientInfo* GetResourceClientInfo(
        ResourceMgrResourceDataInfo* pResourceData,
        const ResourceClientHandle         hClient)
    {
        for (auto& client : pResourceData->pClients)
        {
            if (client.hClient == hClient)
            {
                return &client;
            }
        }

        return NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumOfWaitingClients
    ///
    /// @brief   Get the number of clients who are waiting for the resource
    ///
    /// @param   pResourceData    The resource data structure.
    ///
    /// @return  Number of the waiting clients for the specified resource.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetNumOfWaitingClients(
        const ResourceMgrResourceDataInfo* pResourceData)
    {
        return pResourceData->numWaitingClients;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetClientState
    ///
    /// @brief   Set the client state
    ///
    /// @param   pClientInfo    The client data info.
    /// @param   state          The client state we want to set
    ///
    /// @return  None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetClientState(
        ResourceClientInfo* pClientInfo,
        ResourceClientState state)
    {
        if (NULL != pClientInfo)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "client {%s, %p}, state transition: %d -> %d",
                pClientInfo->pClientName, pClientInfo->hClient, pClientInfo->clientState, state);
            pClientInfo->clientState = state;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitTillResourceAvailabe
    ///
    /// @brief   Wait for the resource availabe
    ///
    /// @param   pResourceData     The resource data structure.
    /// @param   requiredResource  Resource needed.
    /// @param   timeout           Waiting time if resource is not availabe, zero means wait forever.
    ///
    /// @return  CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult WaitTillResourceAvailabe(
        const ResourceMgrResourceDataInfo* pResourceData,
        UINT32                             requiredResource,
        UINT32                             timeout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResourceManager
    ///
    /// @brief  Default constructor.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ResourceManager() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ResourceManager
    ///
    /// @brief  Destructor for the ResourceManager class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ResourceManager();

    ResourceManager(const ResourceManager&)             = delete; ///< Disallow the copy constructor
    ResourceManager& operator= (const ResourceManager&) = delete; ///< Disallow assignment operator

    const CHAR*                   m_pName;                        ///< The name of the resource manager
    Mutex*                        m_pLock;                        ///< Mutex lock for the resource manager
    UINT32                        m_validResourceNum;             ///< Valid resource number in this resource manager
    ResourceMgrResourceDataInfo   m_resourceData[MaxResourceNum]; ///< Array of resource data info

    std::map<ResourceID, ResourceMgrResourceDataInfo*> m_pResourceIdToDataMap;  ///< Resource ID to resource data structure map
};

CAMX_NAMESPACE_END

#endif // CAMXRESOURCEMANAGER_H
