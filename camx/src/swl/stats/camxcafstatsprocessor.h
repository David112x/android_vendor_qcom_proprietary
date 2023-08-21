////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafstatsprocessor.h
/// @brief The class that implements IStatsProcessor for AF.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCAFSTATSPROCESSOR_H
#define CAMXCAFSTATSPROCESSOR_H

#include "chiafinterface.h"
#include "chistatsproperty.h"

#include "camxafstatemachine.h"
#include "camxcafalgorithmhandler.h"
#include "camxcafioutil.h"
#include "camxistatsprocessor.h"
#include "camxlist.h"
#include "camxstatsparser.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

/// @brief This structure holds AF property dependency List
struct AFPropertyDependency
{
    UINT64      slotNum;    ///< The request id for the statistics
    PropertyID  propertyID; ///< Property dependencies in this unit
};

/// @brief This structure holds AF property dependency List
struct AFPropertyDependencyList
{
    INT32                   propertyCount;                  ///< Number of properties in this unit
    AFPropertyDependency    properties[MaxStatsProperties]; ///< Property dependencies in this unit
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements IStatsProcessor for AF.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAFStatsProcessor final : public IStatsProcessor
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create the object for CAFStatsProcessor.
    ///
    /// @param  ppAFStatsProcessor Pointer to CAFStatsProcessor
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IStatsProcessor** ppAFStatsProcessor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Used to initialize the class.
    ///
    /// @param  pInitializeData Pointer to initial settings
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        const StatsInitializeData* pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Executes the algorithm for a request.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependencies
    ///
    /// @brief  Get the the list of dependencies from all stats processors.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pStatsDependency                Pointer to stats dependencies
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetDependencies(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        StatsDependency*                pStatsDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPublishList
    ///
    /// @brief  Get the the list of tags published by all stats processors.
    ///
    /// @param  maxTagArraySize  Maximum size of pTagArray
    /// @param  pTagArray        Array of tags that are published by the stats processor.
    /// @param  pTagCount        Number of tags published by the stats processor
    /// @param  pPartialTagCount Number of Partialtags published by the stats processor
    ///
    /// @return CamxResultSuccess if successful, return failure if the number of tags to be published exceeds maxTagArraySize
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetPublishList(
        const UINT32    maxTagArraySize,
        UINT32*         pTagArray,
        UINT32*         pTagCount,
        UINT32*         pPartialTagCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOEMDependencies
    ///
    /// @brief  Get the the list of OEM dependencies from all stats processors.
    ///
    /// @param  pAlgoGetParam Pointer to input param given to the AFGetParam
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetOEMDependencies(
        AFAlgoGetParam* pAlgoGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDependenciesSatisfied
    ///
    /// @brief  Checks if the dependencies is satisfied for a family.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pIsSatisfied                    Pointer to  Is dependencies satisfied
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult IsDependenciesSatisfied(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        BOOL*                           pIsSatisfied);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAFAlgorithmHandler
    ///
    /// @brief  Set AFAlgoHandler Object.
    ///
    /// @param  pAlgoHandler    Pointer to AFAlgoHandler  Object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAFAlgorithmHandler(
        CAFAlgorithmHandler* pAlgoHandler);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAFIOUtil
    ///
    /// @brief  Set AutoFocus Input/Output Object.
    ///
    /// @param  pAFIOUtil    Pointer to Input/Output Object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAFIOUtil(
        CAFIOUtil* pAFIOUtil);;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAFPDAFInformation
    ///
    /// @brief  Set PDAF Enable flag.
    ///
    /// @param  pPDAFEnablementConditions   pointer to the structure which holds required conditions to enable
    ///                                     pdaf/dualPDHW/SparsePDHW.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAFPDAFInformation(
        PDAFEnablementConditions* pPDAFEnablementConditions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFixedFocus
    ///
    /// @brief  Set isFixedFocus flag.
    ///
    /// @param  isFixedFocus    Fixed focus flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetFixedFocus(
        BOOL isFixedFocus)
    {
        m_isFixedFocus = isFixedFocus;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessUpdatePeerFocusInfo
    ///
    /// @brief   Update the Peer infromation;
    ///
    /// @return  None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessUpdatePeerFocusInfo();

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAFStatsProcessor
    ///
    /// @brief  destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CAFStatsProcessor();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAFStatsProcessor
    ///
    /// @brief  Constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAFStatsProcessor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareInputParams
    ///
    /// @brief  Prepare the input params required for core algorithm
    ///
    /// @param  pBFStats    Pointer to BF Stats
    /// @param  pBGStats    Pointer to BG Stats
    /// @param  pCameraInfo Pointer to camera information
    /// @param  pInputList  Input list to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareInputParams(
        AFBAFStatistics* pBFStats,
        StatsBayerGrid*  pBGStats,
        StatsCameraInfo* pCameraInfo,
        AFAlgoInputList* pInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessSetParams
    ///
    /// @brief  Sends the required set parameters for the core algorithm
    ///
    /// @param  pInputTuningModeData         Pointer to Chi tuning data
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessSetParams(
        ChiTuningModeParameter*        pInputTuningModeData,
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateInputParam
    ///
    /// @brief  Function to set input into algorithm input handle
    ///
    /// @param  inputType  input parameter type
    /// @param  inputSize  size of input parameter
    /// @param  pValue     payload of the input parameter
    /// @param  inputIndex index of input parameter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID UpdateInputParam(
        AFAlgoInputType inputType,
        UINT32          inputSize,
        VOID*           pValue,
        SIZE_T          inputIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverwriteAutoFocusOutput
    ///
    /// @brief  Over the algorithm output from the algorithm
    ///
    /// @param  pOutput         Pointer to core algorithm's output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OverwriteAutoFocusOutput(
        AFAlgoOutputList*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAFSettings
    ///
    /// @brief  Function to update AF settings
    ///
    /// @param  initialUpdate    If we need to read all properties or just dynamic properties
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateAFSettings(
        BOOL initialUpdate);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapControlAFTriggerToAFStateTransition
    ///
    /// @brief  Function to map AF Trigger to AF state Transition
    ///
    /// @param  pAFTriggerValue         pointer to control AF trigger value
    /// @param  pAFStateTransitionType  pointer to AF State transition
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MapControlAFTriggerToAFStateTransition(
        ControlAFTriggerValues* pAFTriggerValue,
        AFStateTransitionType*  pAFStateTransitionType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateFocusControlCommand
    ///
    /// @brief  Function to generate focus control command
    ///
    /// @param  pFocusMode              pointer to AF focus mode
    /// @param  pCameraOperationalMode  pointer to Camera operational mode
    ///
    /// @return AF control command
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFControlCommand GenerateFocusControlCommand(
        AFFocusMode*    pFocusMode,
        AFRunMode*      pCameraOperationalMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildHALOutput
    ///
    /// @brief  Function to process and generate HAL output
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  pointer to HAL output list
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BuildHALOutput(
        const AFAlgoOutputList* pOutput,
        AFHALOutputList*        pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateControlAFState
    ///
    /// @brief  Function to generate control AF state for HAL
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  pointer to HAL output list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateControlAFState(
        const AFAlgoOutputList* pOutput,
        AFHALOutputList*        pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateControlLensDistance
    ///
    /// @brief  Function to generate Lens distance for HAL
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  pointer to HAL output list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateControlLensDistance(
        const AFAlgoOutputList* pOutput,
        AFHALOutputList*        pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFocusMode
    ///
    /// @brief  Function to generate control Lens state for HAL
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  pointer to HAL output list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateFocusMode(
        const AFAlgoOutputList* pOutput,
        AFHALOutputList*        pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFocusValue
    ///
    /// @brief  Function to retrive the focus value from the algorithm
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  pointer to HAL output list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateFocusValue(
        const AFAlgoOutputList* pOutput,
        AFHALOutputList*        pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CombinePropertyDependencLists
    ///
    /// @brief  Function to which combine AF property lists to one list
    ///
    /// @param  pPropertyDependencyList Pointer to list of all stat dependencies including vendor tag
    /// @param  pStatsDependency        Pointer to list of stat dependencies
    /// @param  pAlgoGetParam           pointer to vendor tag list
    /// @param  requestId               Describes metadata slot number to query input parameters from MetadataPool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CombinePropertyDependencLists(
        AFPropertyDependencyList*   pPropertyDependencyList,
        StatsDependency*            pStatsDependency,
        AFAlgoGetParam*             pAlgoGetParam,
        UINT64                      requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddPropertyDependencies
    ///
    /// @brief  Add list of properties dependencies
    ///
    /// @param  pUnpublishedPropertyDependencyList  Pointer to list of unpublished stat dependencies
    /// @param  pStatsDependency                    Pointer to stat dependency structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddPropertyDependencies(
        AFPropertyDependencyList*   pUnpublishedPropertyDependencyList,
        StatsDependency*            pStatsDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VendorTagListAllocation
    ///
    /// @brief  Function to fetch vendor tag list and allocate memory for them
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult VendorTagListAllocation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDefaultConfig
    ///
    /// @brief  Set Default AF configuration
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetDefaultConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultConfigFromAlgo
    ///
    /// @brief  Get Default AF configuration from Algo
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDefaultConfigFromAlgo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDefaultSensorResolution
    ///
    /// @brief  Set Default sensor resolution for first frame.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetDefaultSensorResolution();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSingleParamToAlgorithm
    ///
    /// @brief  This function wraps the logic when only need set one single parameter to algorithm
    ///
    /// @param  paramType   The AFAlgoSetParamType of parameter will be set to algorithm
    /// @param  pParam      The pointer to the parameter will be set
    /// @param  paramSize   The size of the parameter
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetSingleParamToAlgorithm(
        AFAlgoSetParamType paramType,
        VOID*               pParam,
        UINT32              paramSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetHardwareCapability
    ///
    /// @brief  Set Hardware capibility.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetHardwareCapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoChromatix
    ///
    /// @brief  Sets the required inputs parameters for the core algorithm
    ///
    /// @param  pInputTuningModeData Pointer to Chi Tuning mode
    /// @param  pTuningData          Pointer to o/p data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoChromatix(
        ChiTuningModeParameter* pInputTuningModeData,
        StatsTuningData*        pTuningData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeCameraInfo
    ///
    /// @brief  Initialize Dual Camera Information
    ///
    /// @param  pInitializeData   processor related initialization data
    /// @param  pStatsCameraInfo  camera Information related to this pipeline
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeCameraInfo(
        const StatsInitializeData* pInitializeData,
        StatsCameraInfo* pStatsCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadHAFOverrideCreateFunction
    ///
    /// @brief  Load HAF Override Create function from library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LoadHAFOverrideCreateFunction();

    CAFStatsProcessor(const CAFStatsProcessor&) = delete;                ///< Do not implement copy constructor
    CAFStatsProcessor& operator=(const CAFStatsProcessor&) = delete;     ///< Do not implement assignment operator

    AFStateMachine          m_AFStateMachine;                    ///< Auto Focus State
    CHIAFAlgorithm*         m_pAFAlgorithm;                      ///< Pointer to the instance of AF core interface
    CAFAlgorithmHandler*    m_pAFAlgorithmHandler;               ///< Pointer to the instance of AF Algo handler
    CAFIOUtil*              m_pAFIOUtil;                         ///< Pointer to AF IO Util handler
    AFAlgoInput             m_inputArray[AFInputTypeLastIndex];  ///< Structure to the input parameter for the
                                                                 ///  interface
    AFAlgoSettingInfo       m_settingsInfo;                      ///< Settings retrieved from Settings Manager
    CamX::OSLIBRARYHANDLE   m_hHandle;                           ///< handle for custom algo.
    CamX::OSLIBRARYHANDLE   m_hHAFHandle;                        ///< handle for custom HAF algo.
    BOOL                    m_isStatsNodeAvailable;              ///< flag to indicate statsNode is available in
                                                                 ///  topology
    BOOL                    m_isPDAFEnabled;                     ///< flag to indicate pdaf is enabled for the session
    CREATEAF                m_pCreate;                           ///< Pointer to Algo entry function
    CREATEHAFALGORITHM      m_pHAFCreate;                        ///< Pointer to HAF algorithm entry function
    StatsVendorTagList      m_pInputVendorTagList;               ///< List of dependent input vendortag
    UINT64                  m_requestId;                         ///< Holds the current request ID
    VOID*                   m_pTuningDataManager;                ///< Tuning Data Manager
    BOOL                    m_isFixedFocus;                      ///< Flag to indicate is fixed focus or not
    StatsCameraInfo         m_cameraInfo;                        ///< Holds the camera Info
    PDLibSensorType         m_PDAFType;                          ///< specifies type of PDAF sensor
    Node*                   m_pNode;                             ///< Pointer to owning StatsProcessor node
    StatsVendorTagInfoList* m_pOutputVendorTagInfoList;          ///< List of Algo's output vendor tags
};

CAMX_NAMESPACE_END

#endif // CAMXCAFSTATSPROCESSOR_H
