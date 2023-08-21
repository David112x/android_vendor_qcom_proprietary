////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2testcase.h
/// @brief Test suite parent class for Feature2 testing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FEATURE2TESTCASE_H
#define FEATURE2TESTCASE_H

#include "chifeature2test.h"
#include "feature2buffermanager.h"
#include "camera3.h"
#include "chifeature2interface.h"
#include "chimetadatautil.h"
#include "camxcdktypes.h"

static const UINT32 m_DefaultNumMetadataBuffers = 16;     ///< Default number of metadata buffers

class Feature2TestCase : public ChiFeature2Test
{
public:

    virtual VOID Setup();
    virtual VOID Teardown();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessage
    ///
    /// @brief  Feature base class calls this method so that the test may process a message.
    ///
    /// @param  pFeatureRequestObj  Feature request object containing context
    /// @param  pMessages           The message to process
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessMessage(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages);

    ChiFeature2Interface* GetFeature2Interface();

    // Use setup and teardown functions instead
    Feature2TestCase() = default;
    ~Feature2TestCase() = default;

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunFeature2Test
    ///
    /// @param Feature2Type    Feature for which this test case need to be initialized
    ///
    /// @brief main business logic to run the test
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RunFeature2Test();

    CDKResult InitializeInputMetaBufferPool(int cameraId,
                                            CHISTREAMCONFIGINFO* pStreamConfig,
                                            const char* inputMetaName,
                                            bool multiFrame = FALSE);
    CDKResult InitializeBufferManagers(int cameraId,
                                       CHISTREAMCONFIGINFO* pStreamConfig,
                                       const char* inputImage,
                                       bool multiFrame = FALSE);
    CDKResult PatchingMetadata(const CHAR* pFileName, bool multiFrame = FALSE);
    CDKResult PatchingMetadata(const CHAR* pSection, const CHAR* pTag, const VOID* pData, UINT32 count);
    CDKResult PatchingStats(const CHAR* pStatsVendorTagSection);
    CDKResult DeleteTag(const CHAR* pSection, const CHAR* pTag);
    CDKResult PatchingTuningMode();
    CDKResult PatchingMetaConfig();
    CDKResult PatchingStreamConfig(CHISTREAMCONFIGINFO* pStreamConfigInfo);

    UINT32 GetMetadataClientId() const;
    UINT32 GetNumOutputStreams() const;
    ChiMetadataManager* GetMetadataManager() const;
    CHIThreadManager*   GetThreadManager() const;

    ChiFeature2Interface    m_feature2Interface;
    GenericBufferManager**  m_ppBufferManagers;
    int                     m_numStreams;
    int                     m_numOutputStreams;
    std::map<NativeChiStream*, GenericBufferManager*> m_streamBufferMap;

    static UINT32           m_frameNumber;

private:

    virtual VOID SetFeature2Interface() = 0;
    VOID FillDefaultMetadata(ChiMetadata* pMetaData);

    CDKResult VerifyFeature2Interface();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMetadataManager
    ///
    /// @brief create metadata manager for metadata operations
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateMetadataManager();

    ChiMetadataManager*         m_pMetadataManager;                 ///< metadata manager for metadata operations
    UINT32                      m_genericMetadataClientId;          ///< client id returned while creating metadata manager
    Mutex*                      m_pFeature2RequestStateMutex;       ///< App Result mutex
    Condition*                  m_pFeature2RequestStateComplete;
    ChiFeature2RequestObject*   m_pFeature2RequestObject;
    CHIThreadManager*           m_pThreadManager;                   ///< thread manager

    /* Camera members */
    camera_info_t               m_cameraInfo;
    int                         m_selectCam = 0;

    /* CHI interface members */
    ChiModule* m_pChiModule;

    /* Metadata members */
    ChiMetadataUtil* m_pMetadataUtil;

    CDKResult SetupCamera();
    CDKResult LoadChiOps();

};

#endif // FEATURE2TESTCASE_H
