////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxactuator.h
/// @brief API to control actuator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXACTUATOR_H
#define CAMXACTUATOR_H

#include "camxactuatordata.h"
#include "camxcmdbuffermanager.h"
#include "camxmetadatapool.h"
#include "camxpropertyblob.h"
#include "camxnode.h"

CAMX_NAMESPACE_BEGIN

/// Forward Declarations
class Actuator;
class HwContext;

/// @brief ActuatorCreateData
struct ActuatorCreateData
{
    Actuator*               pActuator;                 ///< Output pointer
    HwContext*              pHwContext;                ///< Input pointer
    MetadataPool*           pMainPool;                 ///< Input pointer to per frame pool
    MetadataPool*           pPerUsecasePool;           ///< Input pointer to use case pool
    CmdBufferManager*       pPacketManager;            ///< Input pointer: Actuator packet buffer manager
    CmdBufferManager*       pInitCmdManager;           ///< Input pointer: Actuator init cmd buffer manager
    CmdBufferManager*       pI2CInfoCmdManager;        ///< Input pointer: Actuator I2CInfo cmd buffer manager
    CmdBufferManager*       pPowerCmdManager;          ///< Input pointer: Actuator power cmd buffer manager
    CmdBufferManager*       pMoveFocusCmdManager;      ///< Input pointer: Actuator move focus cmd buffer manager
    UINT32                  cameraId;                  ///< CameraId associated with it
    Node*                   pParentNode;               ///< Parent node
    const EEPROMOTPData*    pOTPData;                  ///< Pointer to OTP data
    UINT32                  requestQueueDepth;         ///< Request Queue Depth
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing actuator APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Actuator final : public IPropertyPoolObserver
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Actuator object
    ///
    /// @param  pCreateData           Input and output pointers
    /// @param  actuatorDeviceIndex   actuator index for acquiring device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        ActuatorCreateData* pCreateData,
        INT32               actuatorDeviceIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Delete Actuator object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentPosition
    ///
    /// @brief  Get current lens position
    ///
    /// @param  unit Specify position in DAC or Step
    ///
    /// @return Lens Postion
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT GetCurrentPosition(
        PositionUnit unit);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensitivity
    ///
    /// @brief  Get actuator sensitivity, i.e. how much lens moved for each digit of DAC value change.
    ///
    /// @return Actuator sensitivity in step per DAC
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT GetSensitivity();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // IPropertyPoolObserver Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPropertyUpdate
    ///
    /// @brief  Callback method notifying clients of an available property. The OnPropertyUpdate method is called
    ///         according to the subscribed properties to notify client that an update is available.
    ///
    /// @param  id          The property that has been updated.
    /// @param  requestId   The frame the property applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnPropertyUpdate(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId);

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
    /// OnMetadataUpdate
    ///
    /// @brief  Callback method notifying clients of an available metadata. The OnMetadataUpdate method is called
    ///         according to the subscribed metadata to notify client that an update is available.
    ///
    /// @param  tag         The metadata that has been updated.
    /// @param  requestId   The frame the metadata applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnMetadataUpdate(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId)
    {
        CAMX_UNREFERENCED_PARAM(tag);
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(pipelineId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPropertyFailure
    ///
    /// @brief  Callback method notifying clients that a property update has failed for a request
    ///
    /// @param  id          The property update that has failed
    /// @param  requestId   The frame the property applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnPropertyFailure(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId)
    {
        CAMX_UNREFERENCED_PARAM(id);
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(pipelineId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnMetadataFailure
    ///
    /// @brief  Callback method notifying clients that a metadata update has failed for a request
    ///
    /// @param  tag         The metadata that has been updated.
    /// @param  requestId   The frame the metadata applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnMetadataFailure(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId)
    {
        CAMX_UNREFERENCED_PARAM(tag);
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(pipelineId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LockForPublish
    ///
    /// @brief  Perform locking necessary to ensure coherency for updating subscriptions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID LockForPublish()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnlockAfterPublish
    ///
    /// @brief  Perform unlock after publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UnlockAfterPublish()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendNOP
    ///
    /// @brief  Send no operation command to actuator driver
    ///
    /// @param  requestId   Request ID
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SendNOP(
        UINT64  requestId);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Actuator
    ///
    /// @brief  Constructor
    ///
    /// @param  pHwContext   HwContext pointer
    /// @param  pData        ActuatorData pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Actuator(
        HwContext*    pHwContext,
        ActuatorData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Actuator
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~Actuator();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize
    ///
    /// @param  pCreateData           Pointer to input and output data
    /// @param  actuatorDeviceIndex   actuator index for acquiring device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        ActuatorCreateData* pCreateData,
        INT32               actuatorDeviceIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateInitializePacket
    ///
    /// @brief  Create initialize packet and initialize HW
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateInitializePacket();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire actuator device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParkLens
    ///
    /// @brief  Move lens to initial position
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ParkLens();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MoveFocus
    ///
    /// @brief  Send command to actuator to move lens
    ///
    /// @param  targetPosition     Target position to move
    /// @param  unit               Specify position in DAC or Step
    /// @param  requestId          Request ID
    /// @param  additionalDelay    Additional delay besides those specified in driver
    /// @param  isManualMode       specifies whether actuator needs to be moved immediatly or real time with request id
    ///
    /// @return CamxResultSuccess, if SUCCESS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MoveFocus(
        INT          targetPosition,
        PositionUnit unit,
        UINT64       requestId,
        UINT16       additionalDelay,
        BOOL         isManualMode);

    Actuator(const Actuator&) = delete;               ///< Disallow the copy constructor
    Actuator& operator=(const Actuator&) = delete;    ///< Disallow assignment operator

    UINT32            m_cameraId;                     ///< cameraId associated with this actuator
    ActuatorData*     m_pActuatorData;                ///< Pointer to actuator data
    HwContext*        m_pHwContext;                   ///< Pointer to HW context
    CmdBufferManager* m_pPacketManager;               ///< Actuator packet buffer manager
    CmdBufferManager* m_pInitializeCmdManager;        ///< Actuator init cmd buffer manager
    CmdBufferManager* m_pI2CInfoCmdManager;           ///< Actuator I2CInfo cmd buffer manager
    CmdBufferManager* m_pPowerCmdManager;             ///< Actuator power cmd buffer manager
    CmdBufferManager* m_pMoveFocusCmdManager;         ///< Move focus cmd buffer manager
    CSLDeviceHandle   m_hActuatorDevice;              ///< Actuator device handle
    BOOL              m_deviceAcquired;               ///< Device acquire state
    Node*             m_pParentNode;                  ///< Parent node
    BOOL              m_initialConfigPending;         ///< Flag to track flash initial config
    UINT32            m_requestQueueDepth;            ///< Request Queue Depth
};

CAMX_NAMESPACE_END

#endif // CAMXACTUATORDRIVER_H
