////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2frameselect.h
/// @brief CHI FrameSelect feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2FRAMESELECT_H
#define CHIFEATURE2FRAMESELECT_H


#include "chifeature2base.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

// Max input ports per physical camera: RDI/Metadata
static const UINT8 MaxInputPortsPerCamera = 2;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2FrameSelect : public ChiFeature2Base
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create FrameSelect feature
    ///
    /// @param  pCreateInputInfo   Pointer to create input info for FrameSelect feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2FrameSelect* Create(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Virtual method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    /// @brief buffer information
    struct ChiFeature2PortBufferInfo
    {
        ChiFeature2PortType bufferType;             ///< Buffer type, ImageBuffer or Metadata
        union
        {
            CHISTREAMBUFFER   streamBuffer;         ///< Stream buffer that includes buffer handle and stream information
            CHIMETADATAHANDLE hMetaHandle;          ///< Metadata Handle
        } Buffer;
        ChiFeature2RequestObject* pRequestObject;   ///< Pointer to corresponding FRO
        BOOL                      isValid;          ///< True/False indicating if buffer is valid
    };

    /// @brief frame select input information
    struct ChiFeature2FrameSelectInputInfo
    {
        std::vector<ChiFeature2PortBufferInfo>  inputBufferArray[MaxInputPortsPerCamera];   ///< input buffer array
        ChiFeature2RequestObject*   pInitialFRO;        ///< pointer to initial FRO used in feature
        ChiFeature2RequestObject*   pCurrentFRO;        ///< pointer to initial FRO used in feature

        UINT8                       currentIndex;       ///< denotes current index in inputbufferArray
        UINT8                       numFramesSent;      ///< keep count of number of frames sent
        UINT8                       validCount;         ///< keep count of valid frames
        UINT8                       numRequests;        ///< keep count of total number of requests
        UINT8                       releaseStartIndex;  ///< index to start releasing from
        UINT8                       releaseEndIndex;    ///< index to end releasing at
        UINT8                       maxInputPorts;      ///< max input ports
    };

    /// @brief frame select output information
    struct ChiFeature2FrameSelectOutputInfo
    {
        std::vector<UINT8> outputOrder;                   ///< output order
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFrameSelect
    ///
    /// @brief  virtual method to do frame select.
    ///
    /// @param  pRequestObject         Feature request object instance.
    /// @param  pFrameSelectInputInfo  the pointer of input information for frame select algorithm.
    /// @param  pFrameSelectOutputInfo the pointer of output information after do frame select.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFrameSelect(
        ChiFeature2RequestObject*           pRequestObject,
        ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
        ChiFeature2FrameSelectOutputInfo*   pFrameSelectOutputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFrameValid
    ///
    /// @brief  virtual method to check if frame is valid
    ///
    /// @param  pRequestObject         Feature request object instance.
    /// @param  pFrameSelectInputInfo  the pointer of input information for frame select algorithm.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of errors
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult IsFrameValid(
        ChiFeature2RequestObject*           pRequestObject,
        ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitialize
    ///
    /// @brief  Function creates Sessions and Pipelines based on input descriptor.
    ///         Derived features can override this to create sessions and pipelines or for virtual camx impl.
    ///
    /// @param  pCreateInputInfo  Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitialize(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare request for FrameSelect feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject) const;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /// OnExecuteProcessRequest
    ///
    /// @brief  Virtual method to execute request for FrameSelect feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoCleanupRequest
    ///
    /// @brief  Virtual method to cleanup request for FrameSelect feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  Virtual method to flush request for FrameSelect feature.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInputResourcePending
    ///
    /// @brief  Use to create continuous callback for FrameSelect
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInputResourcePending(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnReleaseInputDependency
    ///
    /// @brief  Use to release target buffers, because multiple sequence changes flow of FRO_0, request 0
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    /// @param  requestId       Request id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnReleaseInputDependency(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnProcessingDependenciesComplete
    ///
    /// @brief  Check whether the request is done processing and generate a feature message callback if it is
    ///
    /// @param  pRequestObject  The FRO that the request is associated with
    /// @param  requestId       The FRO batch requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnProcessingDependenciesComplete(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2FrameSelect
    ///
    /// @brief  Deafault constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2FrameSelect() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2FrameSelect
    ///
    /// @brief  Virtual destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2FrameSelect() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleCollectInputInfo
    ///
    /// @brief  private method to collect required information for FrameSelect.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  pFrameSelectInputInfo The pointer of input information for frame select.
    /// @param  stageId               Current stage Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleCollectInputInfo(
        ChiFeature2RequestObject*           pRequestObject,
        ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
        UINT8                               stageId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildAndSubmitOutputInfo
    ///
    /// @brief  private method to build output data and notify downstream feature.
    ///
    /// @param  pRequestObject         Feature request object instance.
    /// @param  pFrameSelectInputInfo  The pointer of input information for frame select.
    /// @param  pFrameSelectOutputInfo The pointer of output information after do frame select.
    /// @param  stageId                Current stage Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult BuildAndSubmitOutputInfo(
        ChiFeature2RequestObject*           pRequestObject,
        ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
        ChiFeature2FrameSelectOutputInfo*   pFrameSelectOutputInfo,
        UINT8                               stageId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseInputDependencyResource
    ///
    /// @brief  private method to release input dependency.
    ///
    /// @param  pRequestObject        Feature request object instance.
    /// @param  dependencyIndex       dependencyIndex to release dependency on
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseInputDependencyResource(
        ChiFeature2RequestObject*   pRequestObject,
        UINT8                       dependencyIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendValidFramesDownstream
    ///
    /// @brief  Make sure there are valid frames to send to downstream feature, if so, send them.
    ///
    /// @param  pFrameSelectInputInfo  The pointer of input information for frame select.
    /// @param  stageInfo              Current Stage info of FRO
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SendValidFramesDownstream(
        ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
        ChiFeature2StageInfo                stageInfo) const;

    ChiFeature2FrameSelect(const ChiFeature2FrameSelect&)             = delete;         ///< Disallow the copy constructor
    ChiFeature2FrameSelect& operator= (const ChiFeature2FrameSelect&) = delete;         ///< Disallow assignment operator

    Mutex* m_pFrameSelectThreadMutex;   ///< Mutex for Frame Select
};

#endif // CHIFEATURE2FRAMESELECT_H
