////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeswregistration.h
/// @brief Chi node for swregistration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODESWREGISTRATION_H
#define CAMXCHINODESWREGISTRATION_H

#include "chinode.h"
#include "camxchinodeutil.h"
#include "registration.h"

const CHAR* pswregAlgorithmLibraryName = "libswregistrationalgo";
const CHAR* pswregAlgorithmPath        = "";
const CHAR* pswregFunctionName         = "register_mf";
const INT AlgorithmFilePathSize        = 256;   ///< Maximum custom algorithm file path size


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SWRegistrationalgorithm
///
/// @brief  This function pointer to invoke SWRegistration Algo.
///
/// @param    input
/// @param    output
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*SWRegistrationalgorithm)(
    RegImage*          anchor,
    RegImage*          ref,
    RegPostprocessing* postproc,
    float              gmv[9],
    UINT32*            pConfidence);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiSWRegistrationNode
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialization required to create a node
    ///
    /// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
       CHINODECREATEINFO* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBufferInfo
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryBufferInfo(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiSWRegistrationNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiSWRegistrationNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiSWRegistrationNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiSWRegistrationNode();
private:
    ChiSWRegistrationNode(const ChiSWRegistrationNode&) = delete;               ///< Disallow the copy constructor
    ChiSWRegistrationNode& operator=(const ChiSWRegistrationNode&) = delete;    ///< Disallow assignment operator

     ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetICATransformType
    ///
    /// @brief  Get the IPE ICA capabilities from metadata Vendor Tags
    ///
    /// @return IPE ICACapability
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetICATransformType();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetaData
    ///
    /// @brief  Update the PropertyData in the pipeline
    ///
    /// @param  requestId       The request id for current request
    /// @param  motionvectors   The result of sw registraion algo
    /// @param  algoconfidence  The rsw registraion algo confidence
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMetaData(
        UINT64 requestId,
        FLOAT motionvectors[],
        UINT algoconfidence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  hInput   The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetInputBufferInfo(
        CHINODEBUFFERHANDLE hInput);

    CHIHANDLE                 m_hChiSession;       ///< The Chi session handle
    UINT32                    m_nodeId;            ///< The node's Id
    UINT32                    m_nodeCaps;          ///< The selected node caps
    CHINODEIMAGEFORMAT        m_format;            ///< The selected format
    SWRegistrationalgorithm   m_pSWRegistration;   ///< store algo registration pointer
    CHILIBRARYHANDLE          m_handle;            ///< store handle
    UINT64                    m_processedFrame;    ///< The count for processed frame
};

#endif // CAMXCHINODESWREGISTRATION_H
