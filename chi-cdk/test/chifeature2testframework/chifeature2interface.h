////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2interface.h
/// @brief Interface for implementing a Feature2 class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2INTERFACE_H
#define CHIFEATURE2INTERFACE_H

#include "chifeature2types.h"
#include "chifeature2base.h"
#include "chifeature2requestobject.h"
#include "chicommon.h"

//extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;
//
//enum Feature2Type
//{
//    GENERIC
//    //MFXR
//    //HDR
//};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETFEATURE2DESCRIPTOR
///
/// @brief  Gets a feature descriptor required for creating feature.
///
/// @param  pFeature2DescriptorOut   Pointer to a structure that holds feature descriptor.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETFEATURE2DESCRIPTOR)(ChiFeature2CreateInputInfo* pFeature2DescriptorOut);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNPROCESSMESSAGE
///
/// @brief  Process result message.
///
/// @param  pFeature2MessageObject   Result message from feature.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNPROCESSMESSAGE)(
    ChiFeature2RequestObject*   pFeatureRequestObj,
    ChiFeature2Messages*        pMessages);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETFEATURE2REQUESTOBJECT
///
/// @brief  Gets a feature request object.
///
/// @param  pFeature2DescriptorOut   Output feature descriptor.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETFEATURE2REQUESTOBJECT)(
    ChiFeature2Base*            pFeature2Base,
    ChiMetadata*                pMetadata,
    ChiFeature2RequestObject**  ppFeature2RequestObjectOut,
    VOID*                       pPrivateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNPROCESSPARTIALRESULT
///
/// @brief  Gets a feature request object.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @param  ChiFeature2ResultObject  Partial result from feature.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNPROCESSPARTIALRESULT)(
    ChiFeature2Base*            pFeature2Base);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNINIALIZEFEATURE2TEST
///
/// @brief  Initialize feature2 test.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @param  ChiFeature2ResultObject  Partial result from feature.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNINIALIZEFEATURE2TEST)();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETINPUTFORPORT
///
/// @brief  Update input for feature.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @param  ChiFeature2RequestObject Feature Request Object.
///
/// @return VOID if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETINPUTFORPORT)(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*    pFeature2ResultObject);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNUPDATEINPUTMETADATA
///
/// @brief  Update input for feature.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @param  ChiFeature2RequestObject Feature Request Object.
///
/// @return VOID if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNUPDATEINPUTMETADATA)(
    ChiFeature2Base*            pFeature2Base,
    ChiFeature2RequestObject*    pFeature2ResultObject);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCREATEFEATURE2
///
/// @brief  Update input for feature.
///
/// @param  pFeature2Base            Pointer to derived feature class.
///
/// @param  ChiFeature2RequestObject Feature Request Object.
///
/// @return VOID if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef ChiFeature2Base* (*PFNCREATEFEATURE2)(
    ChiFeature2CreateInputInfo* pFeature2CreateInputInfo);

/// @brief Interface for features to implement.
typedef struct ChiFeature2Interface
{
    PFNINIALIZEFEATURE2TEST         pInitializeFeature2Test;        ///< Intialize the test
    PFNGETFEATURE2DESCRIPTOR        pGetFeature2Descriptor;         ///< Get feature descriptor
    PFNGETFEATURE2REQUESTOBJECT     pGetInputFeature2RequestObject; ///< Get FRO
    PFNPROCESSMESSAGE               pProcessMessage;                ///< Process a message callback from feature
    PFNPROCESSPARTIALRESULT         pProcessPartialResult;          ///< Process a partial metadata result
    PFNGETINPUTFORPORT              pGetInputForPort;               ///< Get input for port
    PFNUPDATEINPUTMETADATA          pUpdateInputMetadata;           ///< Upadte input metadata
    PFNCREATEFEATURE2               pCreateFeature2;                ///< Create a feature
} CHIFEATURE2INTERFACE;


#endif // CHIFEATURE2INTERFACE_H