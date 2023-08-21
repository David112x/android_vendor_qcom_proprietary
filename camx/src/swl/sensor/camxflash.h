// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxflash.h
/// @brief API to control flash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFLASH_H
#define CAMXFLASH_H

#include "camxcmdbuffermanager.h"
#include "camxflashdata.h"
#include "camxmetadatapool.h"
#include "camxnode.h"
#include "camxpropertyblob.h"
#include "rertuning.h"

CAMX_NAMESPACE_BEGIN

static const UINT FlashOffDelay       = 0;   ///< Delay to turn off main flash
static const UINT MaxLEDTriggers      = 3;   ///< Maximum number of LEDs
static const UINT NumberOfFlash       = 1;   ///< Number of LED in this camera module
static const UINT RERCycles           = 3;   ///< Number of RER cycles
static const UINT FlashDefaultCurrent = 300; ///< Default flash current in mA
static const UINT FlashCmdCount       = 5;   ///< Number of command buffers in flash command
static const UINT REROnOffTime        = 40;  ///< RER On/Off in milli seconds


/// Forward Declarations
class Flash;
class HwContext;

/// @brief FlashCreateData
struct FlashCreateData
{
    Flash*                 pFlash;                   ///< Output pointer: Flash object
    HwContext*             pHwContext;               ///< Input pointer: HwConext
    Node*                  pParentNode;              ///< Node creating flash instance
    CmdBufferManager*      pPacketManager;           ///< Input pointer: Flash packet buffer manager
    CmdBufferManager*      pI2CCmdManager  ;         ///< Input pointer: Flash i2c cmd buffer manager
    CmdBufferManager*      pInitializeCmdManager;    ///< Input pointer: Flash initialize cmd buffer manager
    CmdBufferManager*      pFlashPowerCmdManager;    ///< Input pointer: Flash power cmd buffer manager
    CmdBufferManager*      pI2CInitCmdManager;       ///< Input pointer: Flash i2c init setting cmd buffer manager
    CmdBufferManager*      pFireCmdManager;          ///< Input pointer: Flash fire cmd buffer manager
    CmdBufferManager*      pI2CFireCmdManager;       ///< Input pointer: Flash i2c fire cmd buffer manager
    CmdBufferManager*      pRERCmdManager;           ///< Input pointer: RER cmd buffer manager
    CmdBufferManager*      pQueryCmdManager;         ///< Input pointer: Query cmd buffer manager
    UINT32                 cameraId;                 ///< CameraID corresponding to this sensor node
    CSLDeviceOperationMode operationMode;            ///< Set device as Real Time or Non-Real Time device
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing flash APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Flash
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Flash object
    ///
    /// @param  pCreateData       Input and output pointers
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        FlashCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Delete Flash object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Process request for flash.
    ///
    /// @param  pExecuteProcessRequestData   Process request data
    /// @param  flashType                    Indicates if it is pre flash or main flash
    /// @param  LEDCurrents                  LED Currents for the given request
    /// @param  pTuningManager               Tuning Manager referenced by the parent node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        AECFlashInfoType            flashType,
        UINT32                      LEDCurrents[LEDSettingCount],
        TuningDataManager*          pTuningManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOff
    ///
    /// @brief  Method to that will be called before streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any preparation. This is generally called on every Deactivate Pipeline.
    ///         Nodes may use this to release things that are not required at the end of streaming. For exa, any resources
    ///         that are not needed after stream-on can be released here. Make sure to do light weight operations here as
    ///         releasing here may result in re-allocating resources in OnStreamOn.
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OnStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Fire
    ///
    /// @brief  Submit packet to fire flash with high / low current, or turn it off.
    ///
    /// @param  operationType    Flash operation type
    /// @param  requestId        Request Id
    /// @param  pShouldSendNop   OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Fire(
        FlashOperation   operationType,
        UINT64           requestId,
        BOOL*            pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryCurrent
    ///
    /// @brief  Submit packet to query max battery current.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryCurrent();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flash
    ///
    /// @brief  Constructor
    ///
    /// @param  pHwContext   HwContext pointer
    /// @param  pData        FlashData pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit Flash(
        HwContext* pHwContext,
        FlashData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Flash
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~Flash();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize
    ///
    /// @param  pCreateData       Pointer to input and output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        FlashCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateInitializePacket
    ///
    /// @brief  Create initialize packet and initialize HW
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateInitializePacket();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFlashStateMetadata
    ///
    /// @brief  UpdateFlashStateMetadata
    ///
    /// @param  operationType    Flash operation type
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateFlashStateMetadata(
        FlashOperation operationType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FireRERSequence
    ///
    /// @brief  Submit packet to fire red eye reduction sequence.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FireRERSequence();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FirePMIC
    ///
    /// @brief  Submit packet to fire flash with high / low current, or turn it off.
    ///
    /// @param  operationType    Flash operation type
    /// @param  requestId        Request Id
    /// @param  pShouldSendNop   OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FirePMIC(
        FlashOperation   operationType,
        UINT64           requestId,
        BOOL*            pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FireI2C
    ///
    /// @brief  Submit packet to fire flash with high / low current, or turn it off.
    ///
    /// @param  operationType    Flash operation type
    /// @param  requestId        Request Id
    /// @param  pShouldSendNop   OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FireI2C(
        FlashOperation   operationType,
        UINT64           requestId,
        BOOL*            pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandlePreFlash
    ///
    /// @brief  Handle preflash sequence
    ///
    /// @param  requestId        Request Id
    /// @param  pShouldSendNop   OUT: Flag that indicates whether to send Nop Packet or not
    /// @param  aeMode           aeModeValue
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandlePreFlash(
        UINT64 requestId,
        BOOL*  pShouldSendNop,
        ControlAEModeValues aeMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleCalibarationFlash
    ///
    /// @brief  Handle preflash sequence
    ///
    /// @param  requestId        Request Id
    /// @param  pShouldSendNop   OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandleCalibarationFlash(
        UINT64 requestId,
        BOOL*  pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleTorch
    ///
    /// @brief  Handle Torch light
    ///
    /// @param  flashMode       Flash mode input from HAL
    /// @param  requestId       Request Id
    /// @param  pShouldSendNop  OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandleTorch(
        FlashModeValues flashMode,
        UINT64          requestId,
        BOOL*           pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleMainFlash
    ///
    /// @brief  Handle Main Flash
    ///
    /// @param  requestId       Request Id
    /// @param  flashMode       Flash mode input from HAL
    /// @param  captureIntent   Capture intent value input from HAL
    /// @param  pShouldSendNop  OUT: Flag that indicates whether to send Nop Packet or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandleMainFlash(
        UINT64                     requestId,
        FlashModeValues            flashMode,
        ControlCaptureIntentValues captureIntent,
        BOOL*                      pShouldSendNop);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Nop
    ///
    /// @brief  Send Nop command to kernel.
    ///
    /// @param  requestId    Request Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Nop(
        UINT64 requestId);


    Flash(const Flash&) = delete;               ///< Disallow the copy constructor
    Flash& operator=(const Flash&) = delete;    ///< Disallow assignment operator

    UINT32                               m_cameraId;                     ///< cameraId associated with the flash
    FlashData*                           m_pFlashData;                  ///< Pointer to flash data
    HwContext*                           m_pHwContext;                  ///< Pointer to HW context
    CmdBufferManager*                    m_pPacketManager;              ///< Flash packet buffer manager
    CmdBufferManager*                    m_pInitializeCmdManager;       ///< Flash initialize cmd buffer manager
    CmdBufferManager*                    m_pI2CCmdManager;              ///< Flash i2c info cmd buffer manager
    CmdBufferManager*                    m_pPowerCmdManager;            ///< Flash power info cmd buffer manager
    CmdBufferManager*                    m_pI2CInitCmdManager;          ///< Flash i2c init settings cmd buffer manager
    CmdBufferManager*                    m_pFireCmdManager;             ///< Fire flash cmd buffer manager
    CmdBufferManager*                    m_pI2CFireCmdManager;          ///< Fire flash I2C cmd buffer manager
    CmdBufferManager*                    m_pRERCmdManager;              ///< Red eye reduction sequence cmd buffer manager
    CmdBufferManager*                    m_pQueryCurrentCmdManager;     ///< Query current cmd buffer manager
    CSLDeviceHandle                      m_hFlashDevice;                ///< Flash device handle
    Node*                                m_pParentNode;                 ///< Parent node
    UINT16                               m_numberOfFlash;               ///< Number of flash
    UINT                                 m_highCurrent[MaxLEDTriggers]; ///< Current used for main flash
    UINT                                 m_lowCurrent[MaxLEDTriggers];  ///< Current used for preflash, torch
    UINT16                               m_RERIteration;                ///< RER iteration number
    UINT32                               m_REROnDelayMillisecond;       ///< RER flash on delay
    UINT32                               m_REROffDelayMillisecond;      ///< RER flash off delay
    BOOL                                 m_isMainflashRequired;         ///< Indicates if main flash required for next capture
    PreFlashState                        m_preFlashState;               ///< Store current PreFlash state
    CalibrationFlashState                m_calibFlashState;             ///< Store current CalibrationFlash state
    INT8                                 m_flashOffDelayCounter;        ///< Flash off delay counter
    FlashModeValues                      m_lastFlashMode;               ///< Last flash mode
    FlashOperation                       m_operationType;               ///< Present Operation Type
    FlashDriverType                      m_flashType;                   ///< Flash Driver type
    CSLDeviceOperationMode               m_operationMode;               ///< Device is real time or non-real time
    BOOL                                 m_initialConfigPending;        ///< Flag to track flash initial config
    BOOL                                 m_torchStateOn;                ///< Store torch on state
    BOOL                                 m_RERCompleted;                ///< Flag to notify AEC when RER completed
    rerTuning::ChromatixRedEyeReduction* m_pRERTuningData;              ///< Struct to store TuningData
    FlashOperation                       m_lastFlashOperation;          ///< Last flash operation
};

CAMX_NAMESPACE_END

#endif // CAMXFLASH_H
