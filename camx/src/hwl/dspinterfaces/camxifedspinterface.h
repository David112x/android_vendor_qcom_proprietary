////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedspinterface.h
/// @brief ife HVX lass declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEDSPINTERFACE_H
#define CAMXIFEDSPINTERFACE_H

// Camx includes
#include "camxformats.h"
#include "camxdefs.h"
#include "chiisphvxdefs.h"
#include "dsp_streamer.h"
#include "dsp_streamer_callback.h"

CAMX_NAMESPACE_BEGIN

/// @brief HVX stub events
enum HVXStubEventType
{
    HVX_STUB_EVENT_QUERY_CAPS,           ///< Query cap event
    HVX_STUB_EVENT_OPEN,                 ///< OPEN event
    HVX_STUB_EVENT_STATIC_CONFIG,        ///< STATIC CONFIG cap event
    HVX_STUB_EVENT_DYNAMIC_CONFIG,       ///< DYNAMIC CONFIG event
    HVX_STUB_EVENT_START,                ///< START event
    HVX_STUB_EVENT_UPDATE,               ///< UPDATE event
    HVX_STUB_EVENT_RESET,                ///< RESET event
    HVX_STUB_EVENT_STOP,                 ///< STOP event
    HVX_STUB_EVENT_EXCEPTION_STOP,       ///< Exception stop event
    HVX_STUB_EVENT_CLOSE,                ///< Close event
    HVX_STUB_EVENT_MAX,                  ///< MAX event
};

/// @brief HVX stub VFE Type
enum HVXStubVFEType
{
    HVX_STUB_VFE_NULL,      ///< No Vfe
    HVX_STUB_VFE0,          ///< VFE0
    HVX_STUB_VFE1,          ///< VFE1
    HVX_STUB_VFE_BOTH,      ///< Dual VFE
    HVX_STUB_VFE_MAX,       ///< Max
};

/// @brief HVX stub Pixel format
enum HVXStubPixelFormat
{
    HVX_STUB_BAYER_RGGB,     ///< RGGB
    HVX_STUB_BAYER_BGGR,     ///< BGGR
    HVX_STUB_BAYER_GRBG,     ///< GRBG
    HVX_STUB_BAYER_GBRG,     ///< GBRG
    HVX_STUB_BAYER_UYVY,     ///< UyVY
    HVX_STUB_BAYER_VYUY,     ///< VYUY
    HVX_STUB_BAYER_YUYV,     ///< YUYU
    HVX_STUB_BAYER_YVYU,     ///< YVYU
    HVX_STUB_BAYER_MAX,      ///< MAX
};

/// @brief HVX stub Vector Mode
enum HVXStubVectorMode
{
    HVX_STUB_VECTOR_NULL,  ///< Vector NULL
    HVX_STUB_VECTOR_128,   ///< Vector 128
    HVX_STUB_VECTOR_MAX,   ///< Vector Max
};

/// @brief HVX stub QueryCaps
struct HVXStubQueryCaps
{
    HVXStubVectorMode hvxStubVectorMode;  ///< Hvx vector mode
    INT               maxHvxUnit;         ///< Max Units
    INT               maxL2Size;          ///< Max L2 Cache size
};

/// @brief HVX stub Open
struct HVXStubOpen
{
    CHAR name[32];             ///< Algo name
    HVXStubVFEType IFEType;    ///< IFE id
    INT dspClock;              ///< dsp clock
    INT busClock;              ///< bus clock,
    INT dspLatency;            ///< dsp latency
    INT handle;                ///< handle
};

/// @brief HVX stub Callback Functions
struct HVXStubSetCallbackFunc
{
    INT(*callbackdata)(INT handle, HVXIFEType stubVfeType,
        UCHAR bufLabel);
    INT(*ErrorCallback)(INT handle, enum HVXStubVFEType stubVfeType,
        const CHAR* errorMsg, INT errorMsgLen, INT frameId);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE DSP Interface Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEDSPInterface
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCallbackFunctions
    ///
    /// @brief  function for Set callback
    ///
    /// @param  pData Pointer to the data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetCallbackFunctions(
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HVXStubCallbackData
    ///
    /// @brief  function for callback data
    ///
    /// @param  handle    DSP handle
    /// @param  vfeType   Which VFE
    /// @param  bufLabel  Buffer label
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HVXStubCallbackData(
        INT    handle,
        INT    vfeType,
        UCHAR  bufLabel);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HVXStubCallbackError
    ///
    /// @brief  function for callback data
    ///
    /// @param  handle       DSP handle
    /// @param  vfeType      Which VFE
    /// @param  pErrorMsg    Error message
    /// @param  errorMsgLen  Error Message Length
    /// @param  frameId      Request Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult HVXStubCallbackError(
        INT         handle,
        UINT32      vfeType,
        const CHAR* pErrorMsg,
        INT         errorMsgLen,
        INT         frameId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSPOpen
    ///
    /// @brief  Open the DSP Library
    ///
    /// @param  phDSPHandle   DSP handle
    /// @param  pHvxOpen      pointer to the Open structure
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSPOpen(
        INT*             phDSPHandle,
        HVXStubOpen*     pHvxOpen);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSPClose
    ///
    /// @brief  Close the DSP library
    ///
    /// @param  phDSPHandle pointer to the DSP handle
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSPClose(
        INT*  phDSPHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSPReset
    ///
    /// @brief  Reset DSP streamer
    ///
    /// @param  phDSPHandle pointer to DSP handle
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSPReset(
        INT*  phDSPHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSPStart
    ///
    /// @brief  Start DSP streaming
    ///
    /// @param  phDSPHandle pointer to the DSP handle
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSPStart(
        INT*  phDSPHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DSPStop
    ///
    /// @brief  Stop DSP streaming
    ///
    /// @param  phDSPHandle pointer to the DSP handle
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DSPStop(
        INT*  phDSPHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryCapabilities
    ///
    /// @brief  Query Capabilities from DSP
    ///
    /// @param  handle         DSP handle
    /// @param  pHvxQueryCaps  pointer to the QueryCaps structure
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult QueryCapabilities(
        INT               handle,
        HVXStubQueryCaps* pHvxQueryCaps);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StaticConfig
    ///
    /// @brief  Exexcute Static Config
    ///
    /// @param  phDSPHandle    pointer to DSP handle
    /// @param  pStaticConfig  pointer to the Static config structure
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult StaticConfig(
        INT*                 phDSPHandle,
        HVXStubStaticConfig* pStaticConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DynamicConfig
    ///
    /// @brief  Execute process capture request to configure module
    ///
    /// @param  phDSPHandle  pointer to DSP handle
    /// @param  pData        Data for the Dynamic configuration
    /// @param  dataLength   length of the data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DynamicConfig(
        INT*  phDSPHandle,
        VOID* pData,
        UINT  dataLength);

    static remote_handle64               s_hFastRpcHandle;     ///< fast RPC Handle
    static struct HVXStubSetCallbackFunc s_callback;           ///< Callback functions

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEDSPInterface
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEDSPInterface();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEDSPInterface
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEDSPInterface();

private:

    IFEDSPInterface(const IFEDSPInterface&) = delete;       ///< Disallow the copy constructor
    IFEDSPInterface& operator=(const IFEDSPInterface&) = delete;       ///< Disallow assignment operator

};

CAMX_NAMESPACE_END

#endif // CAMXIFEWB12_H
