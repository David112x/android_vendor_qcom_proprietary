////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeswcac.h
/// @brief Chi node for SW CAC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODESWCAC_H
#define CAMXCHINODESWCAC_H

#include <cutils/properties.h>

#include "chinode.h"
#include "camxchinodeutil.h"

#ifndef ION_SYSTEM_HEAP_ID
#define ION_SYSTEM_HEAP_ID    25
#endif

struct Cac3Argruments
{
    uint32_t      cac3EnableFlag;       // 0 - no cac, 1 - cac
    uint32_t      rnrEnableFlag;
    uint32_t      skinRnrEnableFlag;    // 0 -no Skin RNR but RNR, 1- both SkinRNR and RNR
    uint32_t      ldsEnableFlag;
    uint32_t      cdsEnableFlag;
    uint32_t      yuv420Toyuv422;       // YUV420 to YUV422 flag - set if CDS on, LDS on
    uint32_t      yuyv422Toyuv420;      // YUYV422 to YUV420 flag - 0:disable, 1:enable

    uint8_t*      pIonVirtualAddr;      // ION buffer's virtual address
    uint8_t*      pLumaData;            // Pointer to start of luma   data
    uint8_t*      pChromaData;          // Pointer to start of chroma data
    int           fd;
    int           isCached;             // 1=cached ION buffer, 0=uncached ION buffer
    int           ionHeapId;
    uint32_t      imageFormat;
    uint32_t      yWidth;
    uint32_t      yHeight;
    uint32_t      yStride;
    uint32_t      uvWidth;
    uint32_t      uvHeight;
    uint32_t      uvStride;
    uint32_t      yOutStride;           // Used only when YUYV422 to YUV420 flag = 1 (on)
    uint32_t      uvOutStride;          // Used only when YUYV422 to YUV420 flag = 1 (on)

    // parameters for CAC tunning
    int32_t       detection_TH1;
    int32_t       detection_TH2;
    int32_t       verification_TH1;
    int32_t       verification_TH2;

    // RNR arguments
    uint8_t       samplingFactor;
    float*        sigmaLut;
    int           lutSize;
    float         scaleFactor;
    float         centerNoiseSigma;
    float         centerNoiseWeight;
    float         weightOrder;

    // Skin RNR arguments
    float*        skinSigmaLut;
    int           skinSigmaLutSize;
    float         skinStrength;         // skin_slope = 1/skin_strength
    uint8_t       skinCr;
    uint8_t       skinCb;
    uint8_t       skinYMin;
    uint8_t       skinYMax;
    float         skinScaler;           // originally float but we convert to Q10 and store as uint16_t
    float         skinSigmaScale;
    float         skinThreshold;
    uint8_t       dsUsSkinDetection;

    // Face Detection arguments
    uint8_t       numFaces;

    // Userdata passed from the client
    void          *userData;
    int(*cac3_cb) (void *userData);
};

typedef enum
{
    InvalidImageFormat = -1,
    YCbCr,
    YCrCb
} CacImageFormat;

typedef UINT32 (*CHICAC3PROCESS)(Cac3Argruments* pCac3Argruments);
typedef UINT32 (*CHICAC3INIT)(int IonHeapId);
typedef UINT32 (*CHICAC3DEINIT)();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Cac interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiSwCacNode
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
    /// SetBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo);

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
    /// ChiSwCacNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiSwCacNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiSwCacNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiSwCacNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFuncPtrs
    ///
    /// @brief  Open cac lib and find cac3_init, cac3_deinit and cac3_process function.
    ///
    /// @return CDKResultSuccess on success and CDKResultEUnableToLoad in failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeFuncPtrs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeInitializeFunPtrs
    ///
    /// @brief  Unmap cac lib and deinit cac3_init, cac3_deinit and cac3_process function members.
    ///
    /// @return CDKResultSuccess on success and CDKResultEUnableToLoad in failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DeInitializeFunPtrs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ApplyCacAlgo
    ///
    /// @brief  Initialize cac variable and apply cac algo.
    ///
    /// @param  input buffer handle
    ///
    /// @return CDKResultSuccess on success and CDKResultEUnableToLoad in failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ApplyCacAlgo(
       CHINODEBUFFERHANDLE* phBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertCamxImageFormatToCacImageFormat
    ///
    /// @brief  Convert CamX image format to Cac lib compatiable format.
    ///
    /// @param  input buffer handle
    ///
    /// @return InvalidImageFormat in failure otherwise cac compatiable format.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CacImageFormat ConvertCamxImageFormatToCacImageFormat(
        ChiFormat  chiformat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCacState
    ///
    /// @brief  Read metadata and get Cac state.
    ///
    /// @param  request id
    ///
    /// @return 0 if cac disabled else enabled value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetCacState(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsByssableNode
    ///
    /// @brief  Method to query if node is bypassable
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitCacArgument
    ///
    /// @brief  Method to initialize cac arguments.
    ///
    /// @return CDKResultSuccess on success and CDKResultEUnableToLoad in failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitCacArgument(
        CHINODEBUFFERHANDLE* phBuffer);

private:
    ChiSwCacNode(const ChiSwCacNode&) = delete;               ///< Disallow the copy constructor
    ChiSwCacNode& operator=(const ChiSwCacNode&) = delete;    ///< Disallow assignment operator

    CHIHANDLE              m_hChiSession;                     ///< The Chi session handle
    UINT32                 m_nodeId;                          ///< The node's Id
    UINT32                 m_nodeCaps;                        ///< The selected node caps
    CHINODEFLAGS           m_nodeFlags;                       ///< Node flags
    CHINODEIMAGEFORMAT     m_format;                          ///< The selected format
    UINT8                  m_enableSwCacAlways;               ///< SwCAC enable state

    UINT64                 m_processedFrame;                  ///< The count for processed frame
    CHILIBRARYHANDLE       m_hCac3Lib;                        ///< handle for cac3 library.

    CHICAC3PROCESS         m_hCac3Process;
    CHICAC3INIT            m_hCac3Init;
    CHICAC3DEINIT          m_hCac3Deinit;
    Cac3Argruments         m_cac3Argruments;
    static BOOL            m_bIsCacInitDone;                  ///< Tells cac_init status.

};
#endif // CAMXCHINODESWCAC_H
