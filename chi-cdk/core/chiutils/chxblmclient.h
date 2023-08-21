////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxblmclient.h
/// @brief CHX BWLIMITER class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXBLMCLIENT_H
#define CHXBLMCLIENT_H

#include "chxincs.h"
#include "g_blmclientconfig.h"

/// @brief thermal bandwidth request
typedef INT32(*setUsecaseHintFunc)(CHAR* client_name, INT32 req_data);

/// @brief thermal client release request
typedef VOID(*cancelUsecaseHintFunc)(CHAR* client_name);

/// @brief BLM Client structure for BLM supported Operations
struct ChiBLMClientOps
{
    setUsecaseHintFunc         setUsecaseHint;     ///< Sent BW level to BWL API
    cancelUsecaseHintFunc      cancelUsecaseHint; ///< Release function to BWL API
};

/// @brief BLM Client structure to identify usecase
struct ChiBLMParams
{
    INT                 numcamera;           ///< Number of Camera
    UINT32              height;              ///< Stream Height
    UINT32              width;               ///< Stream Width
    INT                 FPS;                 ///< Usecase FPS
    UsecaseId           selectedusecaseId;   ///< Selected usecase type
    INT32               socId;               ///< Target SoC ID
    BOOL                EISEnable;           ///< EIS Flag
    UINT                logicalCameraType;   ///< Logical Camera type
    BOOL                isVideoMode;         ///< Encode cases
};

/// @brief Usecases enum need to be common among Arch, Camera and System team
enum  ChiBLMClientUseCase
{
    ///< Preview/Snapshot cases
    ChiBLMClientUseCaseZSLPreview = 0x100,          ///< ZSL Preview usecase
    ChiBLMClientUseCaseMFNR,                        ///< MFNR usecase
    ChiBLMClientUseCaseQuadCFA,                     ///< QuadCFA usecase
    ChiBLMCLientUsecaseSATPreview,                  ///< SAT Preview usecase
    ChiBLMClientUsecaseRTBSnapshot,                 ///< RTB Snapshot usecase
    ChiBLMClientUsecaseBurstShot,                   ///< Burst shot

    // HFR Usecases
    ChiBLMClientUseCaseHFR           = 0x1000,         ///< HFR usecases
    ChiBLMClientUseCaseHFR720p480,                     ///< 720P  480 FPS usecase
    ChiBLMClientUseCaseHFR1080p240,                    ///< 1080P 240 FPS usecase
    ChiBLMClientUsecaseHFR720p120,                     ///< 720P  120 FPS usecase
    ChiBLMClientUsecaseHFR1080p60,                     ///< 1080P 60  FPS usecase
    ChiBLMClientUseCaseHFR1080p120,                    ///< 1080P 120 FPS usecase
    ChiBLMClientUsecaseDualCamHFR1080p60,              ///< DualCam FHD 60

    // 30 Encode FPS cases
    ChiBLMClientUseCaseEncode       = 0x2000,       ///< Encode usecases
    ChiBLMClientUsecase1080p30,                     ///< 1080P   30  FPS usecase
    ChiBLMClientUseCaseUHD30,                       ///< UHD     30  FPS usecase
    ChiBLMClientUseCaseTriCam1080p30,               ///< TriCam 1080p 30 FPS
};

/// @brief enum to define bw level
enum  ChiBLMBWLevel
{
    ChiBLMBWLevelLow = 0x0,                    ///< Low  level
    ChiBLMBWLevelMid,                          ///< Mid  level
    ChiBLMBWLevelHigh,                         ///< High level
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BLM Client API implementation
///
/// Allows system performance characteristics to be customized based on the use case needs.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE FILE CP017: Private constructor Destructor
class CHXBLMClient final
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of Bandwidth limiter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHXBLMClient* Create();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of Bandwidth limiter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUsecaseBwLevel
    ///
    /// @brief  Method to send perf level to BLM API
    ///
    /// @param  blmparam to identify usecase and decide level
    ///
    /// @return CDKResult Success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetUsecaseBwLevel(
        ChiBLMParams blmparam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CancelUsecaseHint
    ///
    /// @brief  Method to cancel perf level to BLM API
    ///
    /// @return CDKResult Success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CancelUsecaseHint();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CHXBLMClient
    ///
    /// @brief  Default constructor for Bandwidth limiter object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHXBLMClient() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CHXBLMClient
    ///
    /// @brief  Default destructor for Bandwidth limiter object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CHXBLMClient() = default;

    CHXBLMClient& operator=(const CHXBLMClient&) = delete;              ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize a chxBLMclientobject
    ///
    /// @return CDKResult if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapUsecaseFromBwParams
    ///
    /// @brief  map to camera usecaseID
    ///
    /// @param  blmParams Parameter to identify case
    ///
    /// @return usecase ID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32     MapUsecaseFromBwParams(
        ChiBLMParams blmParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBLMLevel
    ///
    /// @brief  Get bw level
    ///
    /// @param  cameraUsecaseId to map to bw level
    /// @param  targetIdx       target index
    ///
    /// @return bw level
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT      QueryBLMLevel(
        INT32 cameraUsecaseId,
        INT   targetIdx);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBLMClientBW
    ///
    /// @brief  Get CPU bw level Parsed from XML
    ///
    /// @param  blmUsecaseID to get CPU BW
    ///
    /// @return CPU bw level
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT    GetBLMClientBW(
        UINT blmUsecaseID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagetIdx
    ///
    /// @brief  Get target index in BLMinfo array
    ///
    /// @param  socId target socid
    ///
    /// @return Target idx
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT    GetTagetIdx(
        INT32 socId);

    Mutex*                  m_pMutex;                                   ///< Mutex object
    INT32                   m_handle;                                   ///< bwl  handle
    ChiBLMClientOps         m_pBLMOps;                                  ///< BWL  API entry
};


#endif // CHXBLMCLIENT_H
