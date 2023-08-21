////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxthermalmanager.h
/// @brief Thermal mitigation for camera and camcorder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTHERMALMANAGER_H
#define CAMXTHERMALMANAGER_H

#include "camxcommontypes.h"
#include "camxdefs.h"
#include "camxhal3module.h"
#include "camxhaldevice.h"
#include "camxlist.h"
#include "camxtypes.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HALDevice;

/// @brief Interface for registering a client with the Thermal Engine
typedef INT(*CAMX_RegisterThermalFunc)(CHAR* name, INT(*ThermalCallback)(INT, VOID* pUserdata, VOID* pData), VOID* pData);

/// @brief Interface for unregistering a client from the Thermal Engine
typedef VOID(*CAMX_UnregisterThermalFunc)(INT handle);

/// @brief Thermal level matching with levels received from thermal engine
enum CameraThermalLevel
{
    ThermalLevelNoAdjustments  = 0,       ///< levels 0-9: No mitigation action from the camera stack
    ThermalLevelShutDown       = 10       ///< Trigger shutdown of use case since temperature is too high
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the thermal mitigation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ThermalManager final
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ThermalManager
    ///
    /// @brief  ThermalManager constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ThermalManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ThermalManager
    ///
    /// @brief  ThermalManager destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ThermalManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create thermalManager singleton object
    ///
    /// @return thermalmanager object , NULL otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ThermalManager* Create();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize and register with the thermal engine
    ///
    /// @return CamxResultSuccess on success, error status otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterHALDevice
    ///
    /// @brief  Register a HALDevice, which will be notified if thermal engine requires mitigation.
    ///
    /// @param  pHALDevice : HALDevice to register
    ///
    /// @return CamxResultSuccess on success, error status otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RegisterHALDevice(
       HALDevice* pHALDevice);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterHALDevice
    ///
    /// @brief  Unregister a HALDevice this is about to be closed.
    ///
    /// @param  pHALDevice : HALDevice to unregister
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UnregisterHALDevice(
       HALDevice* pHALDevice);

private:

    // Do not implement the copy constructor or assignment operator
    ThermalManager(const ThermalManager&             rQCameracopyThermalManager) = delete;
    ThermalManager& operator= (const ThermalManager& rQCameracopyThermalManager) = delete;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ThermalCallback
    ///
    /// @brief  Callback registered with the system ThermalManager. Will be invoked by thermal engine when there are changes
    ///         in measured thermal levels.
    ///
    /// @param  level     : level to be set
    /// @param  pUserdata : Userdata that was passed to the thermal engine during registration
    /// @param  pData     : Reserved
    ///
    /// @return 0 on success, -1 otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT ThermalCallback(
        INT   level,
        VOID* pUserdata,
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ThermalCallbackHandler
    ///
    /// @brief  Handler for thermal events.
    ///
    /// @param  level : level to be set
    ///
    /// @return 0 on success, -1 otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT ThermalCallbackHandler(
       INT level);


    static CHAR                      s_clientName[];                   ///< Name string to use when registering with the
                                                                       ///  thermal engine
    OSLIBRARYHANDLE                  m_hThermalEngineLib;              ///< Thermal engine library handle
    INT                              m_hThermalHandle;                 ///< Handle provided by thermal engine for this client
                                                                       ///  instance
    LightweightDoublyLinkedList      m_HALDevices;                     ///< List of all active HALDevice sessions
    CameraThermalLevel               m_currentLevel;                   ///< Current mitigation level
    static Mutex*                    s_pThermalLock;                   ///< Lock for synchronizing devices in queue
};


CAMX_NAMESPACE_END


#endif /* CAMXTHERMALMANAGER_H */
