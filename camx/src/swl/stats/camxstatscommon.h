////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatscommon.h
/// @brief Stats common definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSTATSCOMMON_H
#define CAMXSTATSCOMMON_H

#include "camxtypes.h"
#include "camxpropertyblob.h"
#include "camxcsl.h"
#include "camxmetadatapool.h"
#include "camxnode.h"
#include "camxthreadmanager.h"
#include "camxhwcontext.h"
#include "camximagebuffer.h"
#include "camxpipeline.h"
#include "camxpropertyblob.h"
#include "camxhwdefs.h"

#include "chituningmodeparam.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 MaxStatsProperties      = 20;   ///< Maximum number of properties statistics node dependent on
static const UINT32 MaxStatsDependentFences = 20;   ///< Maximum number of fences statistics node dependent on
static const UINT32 PreviousFrameDependency = 1;    ///< Define dependency to previous frame based one current frame
static const UINT32 NMinus2FrameDependency  = 2;    ///< Define dependency to n - 2 frame based one current frame

static const UINT MaxReadProperties    = 40;        ///< Maximum number of Read  Properties
static const UINT MaxWriteProperties   = 20;        ///< Maximum number of Write Properties

static const UINT32 MaxActiveRealtimePipeline = 2;  /// Maximum active realtime pipelines

/// @brief Structure describing the property dependency and the set it belongs to.
struct StatsPropertyDependency
{
    PropertyID              property;   ///< Property dependency
    UINT64                  offset;     ///< Offset from implicit requestId on which wait is performed
};

/// @brief Structure describing the property dependency and the set it belongs to.
struct StatsBufferDependency
{
    CSLFence*               phFences;           ///< Fences to wait for
    BOOL*                   pIsFenceSignaled;   ///< Pointer to fence variable whose signaled status is checked
};

/// @brief This enum is used to index into node request dependency array to update any dependency not satisfied yet
enum NodeRequestDependencyIndex
{
    BufferDependencyIndex      = 0,    ///< Index to be used for Buffer dependency
    PropertyDependencyIndex,           ///< Index to be used for Property dependency
    CrossPropertyDependencyIndex       ///< Index to be used for cross pipeline Property dependency
};

/// @brief Structure describing the stats dependencies information.
struct StatsDependency
{
    INT32                   propertyCount;                      ///< Number of properties in this unit
    StatsPropertyDependency properties[MaxStatsProperties];     ///< Property dependencies in this unit
    INT32                   fenceCount;                         ///< Number of fences to wait for
    StatsBufferDependency   fences[MaxStatsDependentFences];    ///< Fences to wait for
};

/// @brief Structure describing the stats buffer information
struct StatsBufferInfo
{
    ISPStatsType    statsType;          ///< Stats type
    ImageBuffer*    pBuffer;            ///< Buffer associated with the stats coming in the port
    CSLFence*       phFences;           ///< Fences associated with the stats coming in the port
    BOOL*           pIsFenceSignaled;   ///< Pointer to fence variable whose signaled status is checked.
};

/// @brief This enum is to identify what kind of action Stats need to do when processing request
enum StatsAlgoAction
{
    StatsAlgoProcessRequest,                ///< Process regular request
    StatsAlgoProcessMapping,                ///< Process follower mapping
    StatsAlgoProcessSwitchingSlave,         ///< Process procedures that master is switching to slave. In this case, we do
                                            ///  process request first, then get peer info for new master
    StatsAlgoProcessSwitchingMaster         ///< Process procedures that slave is switching to master. In this case, we set
                                            ///  peer info first got from original master, then process request
};

/// @brief The string name of the type of the StatsAlgoAction. Must be in order of StatsAlgoAction.
#if __GNUC__
static const CHAR* CamxStatsAlgoActionStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxStatsAlgoActionStrings[] =
#endif // _GNUC__
{
    "StatsAlgoProcessRequest",                ///< Process regular request
    "StatsAlgoProcessMapping",                ///< Process follower mapping
    "StatsAlgoProcessSwitchingSlave",         ///< Process procedures that master is switching to slave. In this case, we do
                                            ///  process request first, then get peer info for new master
    "StatsAlgoProcessSwitchingMaster"         ///< Process procedures that slave is switching to master. In this case, we set
                                            ///  peer info first got from original master, then process request
};

/// @brief Structure describing the process request information.
struct StatsProcessRequestData
{
    Node*                   pNode;                                 ///< Node requesting processing
    UINT64                  requestId;                             ///< The unique frame number for this capture request
    INT32                   processSequenceId;                     ///< Identifier for the node to track its processing order
    UINT32                  pipelineDelay;                         ///< The pipeline delay of the CamX subsystem
    INT32                   bufferCount;                           ///< Buffer count
    StatsBufferInfo         bufferInfo[MaxStatsDependentFences];   ///< The buffer information of stats input buffer
    DependencyUnit*         pDependencyUnit;                       ///< Pointer to dependency list
    BOOL                    reportConverged;                       ///< Report that the algo is converged, even if it isn't
    ChiTuningModeParameter* pTuningModeData;                       ///< pointer to tuning mode data
    BOOL                    skipProcessing;                        ///< Flag to indicate skip processing and
                                                                   ///  publish previous frame control output
    StatsOperationMode      operationMode;                         ///< Indicates whether Fast or Normal Stats Processing
    StatsCameraInfo         cameraInfo;                            ///< Holds camera information
    StatsAlgoAction         algoAction;                            ///< The action Stats processor need to perform
    MultiRequestSyncData*   pMultiRequestSync;                     ///< Holds multi request sync data from higher layer
    PeerSyncInfo            peerSyncInfo;                          ///< Holds peer sync information
};

/// @brief Structure describing the callback information for initialization.
struct StatsInitializeCallback
{
    CHIAFALGORITHMCALLBACKS*        pAFCCallback;       ///< Stats algo entry pointer
    CHIAECALGORITHMCALLBACKS*       pAECCallback;       ///< Stats algo entry pointer
    CHIAWBALGORITHMCALLBACKS*       pAWBCallback;       ///< Stats algo entry pointer
    CHIAFDALGORITHMCALLBACKS*       pAFDCallback;       ///< Stats algo entry pointer
    CHIASDALGORITHMCALLBACKS*       pASDCallback;       ///< Stats algo entry pointer
    CHIPDLIBRARYCALLBACKS*          pPDCallback;        ///< PD Library entry pointer
    CHITRACKERALGORITHMCALLBACKS*   pTrackerCallback;   ///< Tracker entry pointer
};

/// @brief Structure describing the data types information for initialization.
struct StatsInitializeData
{
    Node*                   pNode;                  ///< Owning node
    MetadataPool*           pStaticPool;            ///< Static Property/metadata pool pointer
    MetadataPool*           pDebugDataPool;         ///< DebugData Property pool pointer
    ThreadManager*          pThreadManager;         ///< Pointer to thread manager.
    HwContext*              pHwContext;             ///< Pointer to HW context.
    BOOL                    isStatsNodeAvailable;   ///< Special purpose flags
    StatsInitializeCallback initializecallback;     ///< Algorithm callback pointer
    TuningDataManager*      pTuningDataManager;     ///< Tuning Data Manager
    Pipeline*               pPipeline;              ///< Pipeline to which the stats node belongs
    StatsStreamInitConfig   statsStreamInitConfig;  ///< Stats Stream Init Config
    ChiContext*             pChiContext;            ///< Chi Context information
    StatsCameraInfo         cameraInfo;             ///< Holds camera information
};

/// @brief This structure holds Read Property List
struct ReadProperty
{
    PropertyID          propertyID;          ///< Number of properties in this unit
    UINT64              offset;              ///< offset for this property
    VOID*               pReadData;           ///< Pointer to Read Data
};

/// @brief This structure holds Write Property List
struct WriteProperty
{
    PropertyID          propertyID;           ///< Number of properties in this unit
    UINT                writtenDataSize;      ///< size of the written data
    VOID*               pWriteData;           ///< Pointer to Write Data
};

/// @brief Node vendor tag information
struct PublishVendorTag
{
    const CHAR*  pTagName;      ///< Vendor tag name
    const CHAR*  pSectionName;  ///< Vendor tag section
    VOID*        pData;         ///< Data pointer
    UINT         dataSize;      ///< Data Size
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements statistics observer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE CP017,CP018,CP044: Interface does not need copy/assignment/default overrides
class IStatsNotifier
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyJobProcessRequestDone
    ///
    /// @brief  Notifies the statistic node that a job from the worker thread is completed
    ///
    /// @param  requestId   Request Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult NotifyJobProcessRequestDone(
        UINT64          requestId) = 0;

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IStatsNotifier
    ///
    /// @brief  Protected destructor to prevent accidental deletion of the observer.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IStatsNotifier() = default;
};

/// @brief Structure describing the data types information for initialization.
struct StatsNodeCreateData
{
    Node*           pNode;            ///< Pointer to the stats processing node
    IStatsNotifier* pStatsNotifier;   ///< Pointer to IStatsNotifier
    UINT32          instanceId;       ///< Instance Id
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements common stats utility functions.
///        This class contains only static functions and no state should be added to this class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StatsUtil
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsType
    ///
    /// @brief  Map to ISP stats type from port id.
    ///
    /// @param  portId      port id associated with the stats
    ///
    /// @return ISPStatsType ISP stats type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ISPStatsType GetStatsType(
        UINT portId)
    {
        ISPStatsType statsType = ISPStatsTypeUndefined;

        switch (portId)
        {
            case StatsInputPortAWBBG:
                statsType = ISPStatsTypeAWBBG;
                break;
            case StatsInputPortBF:
                statsType = ISPStatsTypeBF;
                break;
            case StatsInputPortBHist:
                statsType = ISPStatsTypeBHist;
                break;
            case StatsInputPortHDRBE:
                statsType = ISPStatsTypeHDRBE;
                break;
            case StatsInputPortHDRBHist:
                statsType = ISPStatsTypeHDRBHist;
                break;
            case StatsInputPortIHist:
                statsType = ISPStatsTypeIHist;
                break;
            case StatsInputPortCS:
                statsType = ISPStatsTypeCS;
                break;
            case StatsInputPortRS:
                statsType = ISPStatsTypeRS;
                break;
            case StatsInputPortPDAFType3:
            case StatsInputPortRDIPDAF:
                statsType = ISPStatsTypeRDIPDAF;
                break;
            case StatsInputPortDualPDHWPDAF:
                statsType = ISPStatsTypeDualPDHWPDAF;
                break;
            case StatsInputPortTintlessBG:
                statsType = ISPStatsTypeTintlessBG;
                break;
            case StatsInputPortRDIRaw:
                statsType = ISPStatsTypeRDIRaw;
                break;
            case StatsInputPortRDIStats:
                statsType = ISPStatsTypeRDIStats;
                break;
            case StatsInputPortLCRHW:
                statsType = ISPStatsTypeLCRHWStats;
                break;
            case StatsInputPortBPSAWBBG:
                statsType = ISPStatsTypeBPSAWBBG;
                break;
            case StatsInputPortBPSRegYUV:
                statsType = ISPStatsTypeBPSRegYUV;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupStats, "Need to add new stats support");
                break;
        }

        return statsType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadAlgorithmLib
    ///
    /// @brief  Load algorithm library
    ///
    /// @param  pHandle             Output pointer to the loaded library's handle
    /// @param  pAlgorithmPath      Path to the algorithm library
    /// @param  pAlgorithmName      Name of algorithm
    /// @param  pFuncName           Name of the function
    ///
    /// @return custom loaded lib address
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* LoadAlgorithmLib(
        CamX::OSLIBRARYHANDLE* pHandle,
        const CHAR*            pAlgorithmPath,
        const CHAR*            pAlgorithmName,
        const CHAR*            pFuncName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDebugDataBuffer
    ///
    /// @brief  Returns the debug data associated with a given request ID
    ///
    /// @param  pDebugDataPool  Debug data property pool to get the debug data from
    /// @param  requestId       Request ID for which to obtain the debug data
    /// @param  tagId           Vendor tag associated with the debug data in interest
    /// @param  ppDebugDataOut  Debug data to be returned for the given request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetDebugDataBuffer(
        MetadataPool*       pDebugDataPool,
        UINT64              requestId,
        UINT                tagId,
        DebugData**         ppDebugDataOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsStreamInitConfig
    ///
    /// @brief  Retrieve the fast aec mode control data
    ///
    /// @param  pUsecasePool                Handle to Usecase property pool
    /// @param  pStatsStreamInitConfig      Output buffer to fill stats init config
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetStatsStreamInitConfig(
        MetadataPool*            pUsecasePool,
        StatsStreamInitConfig*   pStatsStreamInitConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRoleName
    ///
    /// @brief  return string based on role info enum
    ///
    /// @param  role information
    ///
    /// @return String containing role information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetRoleName(
        StatsAlgoRole role);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CdkResultToCamXResult
    ///
    /// @brief  Convert Cdk result to camx result
    ///
    /// @param  cdkResult cdk result
    ///
    /// @return camx result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CdkResultToCamXResult(
        CDKResult cdkResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadVendorTag
    ///
    /// @brief  Read vendor tag
    ///
    /// @param  pNode        Pointer to Node
    /// @param  pSectionName Pointer to vendor tag section name
    /// @param  pTagName     Pointer to vendor tag name
    /// @param  ppArg        Double Pointer for Value filled in tag
    ///
    /// @return camx result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReadVendorTag(
        Node*       pNode,
        const CHAR* pSectionName,
        const CHAR* pTagName,
        VOID**      ppArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteVendorTag
    ///
    /// @brief  Write vendor tag
    ///
    /// @param  pNode        Pointer to Node
    /// @param  pSectionName Pointer to vendor tag section name
    /// @param  pTagName     Pointer to vendor tag name
    /// @param  pArg         Pointer for value to write in vendor tag
    /// @param  argCount     Size of arg data
    ///
    /// @return camx result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult WriteVendorTag(
        Node*       pNode,
        const CHAR* pSectionName,
        const CHAR* pTagName,
        const VOID* pArg,
        const UINT  argCount);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements common stats Vendor Tag callback functions.
///        This is a singleton class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiStatsSession
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiStatsSession
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiStatsSession();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the class
    ///
    /// @param  pInitializeData   pointer to stats initialize data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Initialize(
        const StatsInitializeData* pInitializeData)
    {
        if (NULL != pInitializeData)
        {
            m_pNode                    = pInitializeData->pNode;
            m_pStaticPool              = pInitializeData->pStaticPool;
            m_pStatsProcessRequestData = NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStatsProcessorRequestData
    ///
    /// @brief  Sets the current processor request data
    ///
    /// @param  pStatsProcessRequestData Stats Processor current request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetStatsProcessorRequestData(
        const StatsProcessRequestData*    pStatsProcessRequestData)
    {
        if (NULL != pStatsProcessRequestData)
        {
            m_pStatsProcessRequestData = pStatsProcessRequestData;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagBase
    ///
    /// @brief  Callback function to retrieve vendor tag base
    ///
    /// @param  pVendorTagBaseInfo  Pointer to input/output parameters to retrieve vendor tag base
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult GetVendorTagBase(
        CHIVENDORTAGBASEINFO* pVendorTagBaseInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetMetadata
    ///
    /// @brief  The implementation for PFNNODEGETMETADATA defined in chinode.h
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNSetMetadata
    ///
    /// @brief  The implementation for PFNNODESETMETADATA defined in chinode.h
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNSetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryVendorTagLocation
    ///
    /// @brief  Callback function to retrieve vendor tag location
    ///
    /// @param  pSectionName    String section name
    /// @param  pTagName        String tag name
    /// @param  pTagLocation    Retrieved tagId
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult QueryVendorTagLocation(
        const CHAR* pSectionName,
        const CHAR* pTagName,
        UINT32*     pTagLocation);

private:


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDynamicMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags.
    ///         The tag can be an Android tag or a vendor tag. If the metadata information associated with the tag is not
    ///         published, Chi returns those tags as unpublished when this function returns. The node can add them in the
    ///         dependency reporting during ChiNodeProcRequest().
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetDynamicMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDynamicMetadata
    ///
    /// @brief  Sets a list of metadata information based on metadata tags.
    ///
    ///         The tag can be an Android tag or a vendor tag. When published, Chi driver will notify all other nodes that
    ///         reported these tags as dependencies.
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetDynamicMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags from static pool.
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetStaticMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStaticMetadata
    ///
    /// @brief  Sets a list of metadata information based on metadata tags into static pool.
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the node.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetStaticMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    MetadataPool*                   m_pStaticPool;              ///< Pointer to static metadata pool
    Node*                           m_pNode;                    ///< Pointer to owning Stats Node
    const StatsProcessRequestData*  m_pStatsProcessRequestData; ///< current stats processor request data

     // Do not implement the copy constructor or assignment operator
    ChiStatsSession(const ChiStatsSession&)             = delete;  ///< Disallow the copy constructor.
    ChiStatsSession& operator=(const ChiStatsSession&)  = delete;  ///< Disallow assignment operator.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements common Read/Write Property function.
///        This is a singleton class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StatsPropertyReadAndWrite
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StatsPropertyReadAndWrite
    ///
    /// @brief  Constructor
    ///
    /// @param  pInitializeData   pointer to stats initialize data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit StatsPropertyReadAndWrite(
        const StatsInitializeData* pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///  ~StatsPropertyReadAndWrite
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~StatsPropertyReadAndWrite();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetReadProperties
    ///
    /// @brief  Set Read Properties
    ///
    /// @param  pReadProperty     pointer to Read Properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetReadProperties(
        ReadProperty* pReadProperty);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetWriteProperties
    ///
    /// @brief  Set Read Properties
    ///
    /// @param  pWriteProperty    pointer to Read Properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetWriteProperties(
        WriteProperty* pWriteProperty);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetReadProperties
    ///
    /// @brief  Get Read Properties
    ///
    /// @param  count                           size of the read properties
    /// @param  requestIdOffsetFromLastFlush    offset from the last flushed frame
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetReadProperties(
        SIZE_T           count,
        UINT64           requestIdOffsetFromLastFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteProperties
    ///
    /// @brief  Write Properties
    ///
    /// @param  count     size of the write properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WriteProperties(
        SIZE_T           count);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyData
    ///
    /// @brief  Get Read Property data
    ///
    /// @param  id        index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID*  GetPropertyData(
        const UINT id)const;

    Node*                      m_pNode;                                            ///< Pointer to owning Stats Node
    VOID*                      m_pPropertyReadData[MaxReadProperties];             ///< Property Read Data

    ReadProperty*              m_pReadProperty;                                    ///< Pointer to Read property array
    WriteProperty*             m_pWriteProperty;                                   ///< Pointer to write property array
private:
    // Do not implement the copy constructor or assignment operator
    StatsPropertyReadAndWrite(const StatsPropertyReadAndWrite&)            = delete;  ///< Disallow the copy constructor.
    StatsPropertyReadAndWrite& operator=(const StatsPropertyReadAndWrite&) = delete;  ///< Disallow assignment operator.
};

CAMX_NAMESPACE_END

#endif // CAMXSTATSCOMMON_H
