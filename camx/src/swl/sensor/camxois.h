////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxois.h
/// @brief API to control OIS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXOIS_H
#define CAMXOIS_H

#include "camxcmdbuffermanager.h"
#include "camxmetadatapool.h"
#include "camxpropertyblob.h"
#include "camxnode.h"
#include "camxoisdata.h"

CAMX_NAMESPACE_BEGIN

/// Forward Declarations
class OIS;
class HwContext;

/// @brief OisCreateData
struct OisCreateData
{
    OIS*                    pOis;                 ///< Output pointer
    HwContext*              pHwContext;           ///< Input pointer: get hardware context
    CmdBufferManager*       pInitPacketManager;   ///< Input pointer: Ois init packet buffer manager
    CmdBufferManager*       pInitCmdManager;      ///< Input pointer: Ois init cmd buffer manager
    CmdBufferManager*       pI2CInfoCmdManager;   ///< Input pointer: Ois I2CInfo cmd buffer manager
    CmdBufferManager*       pPowerCmdManager;     ///< Input pointer: Ois Mode cmd buffer manager
    CmdBufferManager*       pModePacketManager;   ///< Input pointer: Ois Mode packet buffer manager
    CmdBufferManager*       pModeCmdManager;      ///< Input pointer: Ois Mode cmd buffer manager
    UINT32                  cameraId;             ///< CameraId associated with it
    Node*                   pParentNode;          ///< Pointer back to the Node creating this object
    const EEPROMOTPData*    pOTPData;             ///< Pointer to OTP data
};

/// @brief Enum describing OIS configuration status
enum class OISConfigurationStatus
{
    OISConfigurationStateUninitialized, ///< OIS state uninitialized
    OISInitializationInProgress,        ///< OIS Initialization In Progress
    OISInitializationComplete,          ///< OIS Initialization Successful
    OISInitializationFailed,            ///< OIS Initialization Failed
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing Ois APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OIS final : public IPropertyPoolObserver
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create OIS object
    ///
    /// @param  pCreateData           Input and output pointers
    /// @param  oisDeviceIndex        ois index for acquiring device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        OisCreateData* pCreateData,
        INT32               oisDeviceIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Delete ois object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Process request for ois.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

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
        UINT        pipelineId)
    {
        CAMX_UNREFERENCED_PARAM(id);
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(pipelineId);
    }

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
    /// ReleaseResources
    ///
    /// @brief  As a part of ReleaseResources, OIS is disabled
    ///
    /// @param  modeBitmask Deactivate pipeline mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseResources(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OIS
    ///
    /// @brief  Constructor
    ///
    /// @param  pHwContext   HwContext pointer
    /// @param  pData        OisData pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OIS(
        HwContext* pHwContext,
        OISData* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~OIS
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~OIS();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize
    ///
    /// @param  pCreateData           Pointer to input and output data
    /// @param  oisDeviceIndex        ois index for acquiring device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        OisCreateData* pCreateData,
        INT32 oisDeviceIndex);

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
    /// @brief  Helper method to acquire Ois device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureOISMode
    ///
    /// @brief  Callback method notifying clients that a metadata update has failed for a request
    ///
    /// @param  mode  mode.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureOISMode(
        OISMode mode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalibrateOISData
    ///
    /// @brief  Calibrates the Ois data with the data obtained from EEPROM
    ///
    /// @param  pOTPData pointer to the OTP data for this cameral
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalibrateOISData(
        const EEPROMOTPData* pOTPData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCalibrationCmdBufferManager
    ///
    /// @brief  Creates the calibration cmd manager
    ///
    ///
    /// @return success/failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCalibrationCmdBufferManager();

    OIS(const OIS&) = delete;                        ///< Disallow the copy constructor
    OIS& operator=(const OIS&) = delete;             ///< Disallow assignment operator

    OISData*                           m_pOISData;                 ///< Pointer to Ois data
    UINT32                             m_cameraId;                 ///< cameraId associated with the OIS
    HwContext*                         m_pHwContext;               ///< Pointer to HW context
    CmdBufferManager*                  m_pInitializePacketManager; ///< Ois init packet buffer manager
    Packet*                            m_pPacket;                  ///< Ois init packet
    CmdBufferManager*                  m_pInitializeCmdManager;    ///< Ois init cmd buffer manager
    CmdBuffer*                         m_pInitializeCmdBuffer;     ///< Ois Init commands
    CmdBufferManager*                  m_pI2CInfoCmdManager;       ///< Ois I2CInfo cmd buffer manager
    CmdBuffer*                         m_pI2CInfoCmdBuffer;        ///< Ois I2C commands
    CmdBufferManager*                  m_pPowerCmdManager;         ///< Ois power info cmd buffer manager
    CmdBuffer*                         m_pPowerCmdBuffer;          ///< Ois Power commands
    CmdBufferManager*                  m_pCalibrateCmdManager;     ///< Ois calibration cmd buffer manager
    CmdBufferManager*                  m_pOisModePacketManager;    ///< Ois Mode packet buffer manager
    CmdBufferManager*                  m_pOisModeCmdManager;       ///< Ois Mode cmd buffer manager
    CSLDeviceHandle                    m_hOisDevice;               ///< Ois device handle
    Node*                              m_pNode;                    ///< Pointer to the owner of this class
    OISConfigurationStatus             m_OISConfigStatus;          ///< Indicates the state of OIS config
    LensOpticalStabilizationModeValues m_prevState;                ///< previous ois state
    BOOL                               m_isCalibration;            ///< calibration availability
    OISMode                            m_oisMode;                  ///< store ois mode for optmz
};

CAMX_NAMESPACE_END

#endif // CAMXOISDRIVER_H
