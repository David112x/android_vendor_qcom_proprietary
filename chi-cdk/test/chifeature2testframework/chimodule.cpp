////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chimodule.cpp
/// @brief Implementation for chimodule.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chimodule.h"
#include "chxutils.h"

///@brief ChiModule singleton
ChiModule* ChiModule::m_pModuleInstance = NULL;

/***************************************************************************************************************************
* ChiModule::GetInstance
*
* @brief
*     Gets the singleton instance for ChiModule
* @param
*     None
* @return
*     ChiModule singleton
***************************************************************************************************************************/
ChiModule* ChiModule::GetInstance()
{
    if (m_pModuleInstance == NULL)
    {
        m_pModuleInstance = CF2_NEW ChiModule();
        if (m_pModuleInstance->Initialize() != CDKResultSuccess)
        {
            CF2_LOG_ERROR("Failed to initialize ChiModule singleton!");
            return NULL;
        }
    }

    return m_pModuleInstance;
}

/***************************************************************************************************************************
* ChiModule::DestroyInstance
*
* @brief
*     Destroy the singleton instance of the ChiModule class
* @param
*     None
* @return
*     VOID
***************************************************************************************************************************/
VOID ChiModule::DestroyInstance()
{
    if (m_pModuleInstance != NULL)
    {
        delete m_pModuleInstance;
        m_pModuleInstance = NULL;
    }
}

/***************************************************************************************************************************
* ChiModule::ChiModule
*
* @brief
*     Constructor for ChiModule
***************************************************************************************************************************/
ChiModule::ChiModule() :
    m_camList(),
    m_pCameraInfo(NULL),
    m_pLegacyCameraInfo(NULL),
    m_pSensorInfo(NULL),
    m_numOfCameras(0),
    m_hContext(NULL),
    m_chiOps{},
    m_chiVendorTagOps{},
    m_halVendorTagOps{},
    m_fenceOps{},
    m_pCameraModule(NULL),
    hLibrary(NULL)
{
}

/***************************************************************************************************************************
* ChiModule::~ChiModule
*
* @brief
*     Destructor for ChiModule
***************************************************************************************************************************/
ChiModule::~ChiModule()
{
    if (NULL != m_hContext)
    {
        if (CloseContext() != CDKResultSuccess)
        {
            CF2_LOG_ERROR("Failed to close camera context!");
        }
    }

    for (int currCamera = 0; currCamera < m_numOfCameras; currCamera++)
    {
        if (m_pSensorInfo[currCamera])
        {
            delete[] m_pSensorInfo[currCamera];
            m_pSensorInfo[currCamera] = NULL;
        }
    }

    if (m_pLegacyCameraInfo)
    {
        delete[] m_pLegacyCameraInfo;
        m_pLegacyCameraInfo = NULL;
    }

    if (m_pSensorInfo)
    {
        delete[] m_pSensorInfo;
        m_pSensorInfo = NULL;
    }

    if (m_pCameraInfo)
    {
        delete[] m_pCameraInfo;
        m_pCameraInfo = NULL;
    }
}

/***************************************************************************************************************************
* ChiModule::Initialize
*
* @brief
*     Initialize member variables using driver calls
* @param
*     None
* @return
*     CDKResult result code
***************************************************************************************************************************/
CDKResult ChiModule::Initialize()
{
    CDKResult result = CDKResultSuccess;

    result = LoadLibraries();
    if (result != CDKResultSuccess)
    {
        return result;
    }

    //result = OpenContext();
    //if (result != CDKResultSuccess)
    //{
    //    return result;
    //}

    //m_numOfCameras = m_chiOps.pGetNumCameras(m_hContext);
    //CF2_LOG_DEBUG("Number of cameras reported by device: %d", m_numOfCameras);

    //m_pCameraInfo = CF2_NEW CHICAMERAINFO[m_numOfCameras];
    //m_pLegacyCameraInfo = CF2_NEW camera_info[m_numOfCameras];
    //m_pSensorInfo = CF2_NEW CHISENSORMODEINFO*[m_numOfCameras];

    //for (int currCamera = 0; currCamera < m_numOfCameras; currCamera++)
    //{
    //    m_pCameraInfo[currCamera].pLegacy = &m_pLegacyCameraInfo[currCamera];

    //    m_chiOps.pGetCameraInfo(m_hContext, currCamera, &m_pCameraInfo[currCamera]);

    //    uint32_t numSensorModes = m_pCameraInfo[currCamera].numSensorModes;

    //    m_pSensorInfo[currCamera] = CF2_NEW CHISENSORMODEINFO[numSensorModes];

    //    m_chiOps.pEnumerateSensorModes(m_hContext, currCamera, numSensorModes, m_pSensorInfo[currCamera]);

    //    CF2_LOG_DEBUG("Listing available sensor modes for camera %d...", currCamera)

    //    for (uint32_t sensorMode = 0; sensorMode < numSensorModes; sensorMode++)
    //    {
    //        CF2_LOG_DEBUG("Camera %d sensormode %u:, width %u, height %u, framerate %u, bpp %u",
    //            currCamera,
    //            sensorMode,
    //            m_pSensorInfo[currCamera][sensorMode].frameDimension.width,
    //            m_pSensorInfo[currCamera][sensorMode].frameDimension.height,
    //            m_pSensorInfo[currCamera][sensorMode].frameRate,
    //            m_pSensorInfo[currCamera][sensorMode].bpp);
    //    }
    //}

    // TODO Check if --camera flag set
    //uint32_t testCameraID = CmdLineParser::GetCamera();

    // If camera ID within range, add to test list
    //if (testCameraID >= 0 && testCameraID < static_cast<uint32_t>(m_numOfCameras))
    //{
    //    m_camList.push_back(testCameraID);
    //}
    //else // Else add all available cameras for test
    //{
        //for (uint32_t currCamera = 0; currCamera < static_cast<uint32_t>(m_numOfCameras); currCamera++)
        //{
        //    m_camList.push_back(currCamera);
        //}
    //}

    return result;
}

/***************************************************************************************************************************
* ChiModule::LoadLibraries
*
* @brief
*     Load chi and vendor tag operation libraries
* @param
*     None
* @return
*     CDKResult result code
***************************************************************************************************************************/
CDKResult ChiModule::LoadLibraries()
{
    PFNCHIENTRY pChiHalOpen;

    hLibrary = ChxUtils::LibMap(m_libPath);
    if (hLibrary == NULL)
    {
        CF2_LOG_ERROR("Failed to load android library");
        return CDKResultEUnableToLoad;
    }
    pChiHalOpen = reinterpret_cast<PFNCHIENTRY>(ChxUtils::LibGetAddr(hLibrary, "ChiEntry"));
    if (pChiHalOpen == NULL)
    {
        CF2_LOG_ERROR("ChiEntry missing in library");
        return CDKResultEUnableToLoad;
    }
    m_pCameraModule = reinterpret_cast<camera_module_t*>(ChxUtils::LibGetAddr(hLibrary, "HMI"));
    if (m_pCameraModule == NULL)
    {
        CF2_LOG_ERROR("CameraModule missing in library");
        return CDKResultEUnableToLoad;
    }

    (*pChiHalOpen)(&m_chiOps);

    m_pCameraModule->get_vendor_tag_ops(&m_halVendorTagOps);

    //int ret = set_camera_metadata_vendor_ops(const_cast<vendor_tag_ops_t*>(&m_halVendorTagOps));
    //if (ret != 0)
    //{
    //    LogMsg(eError, CHIMODULE, "Failed to set vendor tag ops");
    //    return CDKResultEFailed;
    //}

    return CDKResultSuccess;
}

/***************************************************************************************************************************
* ChiModule::OpenContext
*
* @brief
*     Opens a camera context
* @param
*     None
* @return
*     CDKResult result code
***************************************************************************************************************************/
CDKResult ChiModule::OpenContext()
{
    UINT32 majorVersion = m_chiOps.majorVersion;
    UINT32 minorVersion = m_chiOps.minorVersion;
    UINT32 subVersion = m_chiOps.subVersion;

    CF2_LOG_INFO("Checking CHI inteface compatibility: expected version %u.%u, queried version %u.%u",
        EXPECT_CHI_API_MAJOR_VERSION, EXPECT_CHI_API_MINOR_VERSION, majorVersion, minorVersion);

    if (majorVersion != EXPECT_CHI_API_MAJOR_VERSION)
    {
        CF2_LOG_ERROR("Major version mismatch: NativeChiTest is not compatible with this build!");
        return CDKResultEUnsupported;
    }
    if (minorVersion != EXPECT_CHI_API_MINOR_VERSION)
    {
        CF2_LOG_WARN("Minor version mismatch: NativeChiTest may need to be recompiled to work with this build.");
    }
    if (subVersion != EXPECT_CHI_API_SUB_VERSION)
    {
        CF2_LOG_WARN("Sub version mismatch: NativeChiTest may need to be recompiled to work with this build.");
    }

    CF2_LOG_INFO("Opening chi context");
    m_hContext = m_chiOps.pOpenContext();
    m_chiOps.pTagOps(&m_chiVendorTagOps);
    m_chiOps.pGetFenceOps(&m_fenceOps);
    if (m_hContext == NULL)
    {
        CF2_LOG_ERROR("Open context failed!");
        return CDKResultEFailed;
    }
    return CDKResultSuccess;
}

/***************************************************************************************************************************
* ChiModule::CloseContext
*
* @brief
*     Close camera context
* @param
*     None
* @return
*     CDKResult result code
***************************************************************************************************************************/
CDKResult ChiModule::CloseContext()
{
    CDKResult result = CDKResultSuccess;

    CF2_LOG_INFO("Closing Context: %p", m_hContext);
    if (m_hContext != NULL)
    {
        m_chiOps.pCloseContext(m_hContext);
        m_hContext = NULL;
    }
    else
    {
        CF2_LOG_ERROR("Requested context %p is not open", m_hContext);
        result = CDKResultEInvalidState;
    }
    return result;
}

/***************************************************************************************************************************
* ChiModule::GetNumCams
*
* @brief
*     Gets number of cameras reported by the driver
* @param
*     None
* @return
*     int number of cameras reported by the module
***************************************************************************************************************************/
int ChiModule::GetNumberCameras() const
{
    return m_numOfCameras;
}

/***************************************************************************************************************************
* ChiModule::GetCameraList
*
* @brief
*     Gets list of cameras reported by the driver
* @param
*     None
* @return
*     int number of cameras reported by the module
***************************************************************************************************************************/
std::vector <int> ChiModule::GetCameraList() const
{
    return m_camList;
}

/***************************************************************************************************************************
* ChiModule::GetCameraInfo
*
* @brief
*     Gets camera info for given camera Id
* @param
*     [in]  uint32_t   cameraId    cameraid associated
* @return
*     CHICAMERAINFO* camerainfo for given camera Id
***************************************************************************************************************************/
const CHICAMERAINFO* ChiModule::GetCameraInfo(uint32_t cameraId) const
{
    const CHICAMERAINFO* pCameraInfo = NULL;
    if ((m_pCameraInfo != NULL) && (&m_pCameraInfo[cameraId] != NULL))
    {
        pCameraInfo = &m_pCameraInfo[cameraId];
    }

    return pCameraInfo;
}

/***************************************************************************************************************************
* ChiModule::GetCameraSensorModeInfo
*
* @brief
*     Gets sensormode for given index
* @param
*     [in] uint32_t cameraId id of the camera
*     [in] uint32_t modeId   index of the mode to use
* @return
*     CHISENSORMODEINFO for given cameraId and index
****************************************************************************************************************************/
CHISENSORMODEINFO* ChiModule::GetCameraSensorModeInfo(uint32_t cameraId, uint32_t modeId) const
{
    CHISENSORMODEINFO* pSensorModeInfo = NULL;
    if ((m_pSensorInfo != NULL) && (&m_pSensorInfo[cameraId] != NULL))
    {
        pSensorModeInfo = &m_pSensorInfo[cameraId][modeId];
    }
    return pSensorModeInfo;
}

/***************************************************************************************************************************
* ChiModule::GetClosestSensorMode
*
* @brief
*     Chooses a sensormode closest to given resolution
* @param
*     [in] uint32_t cameraId id of the camera
*     [in] Size res          resolution used
* @return
*     CHISENSORMODEINFO for given resolution which is closest
****************************************************************************************************************************/
CHISENSORMODEINFO* ChiModule::GetClosestSensorMode(uint32_t cameraId, Size resolution) const
{
    CHISENSORMODEINFO* pSensorModeInfo = NULL;
    long long givenRes = resolution.width*resolution.height;
    long long min = LONG_MAX;
    uint32_t index = 0;
    if ((m_pSensorInfo != NULL) && (&m_pSensorInfo[cameraId] != NULL))
    {
        CHISENSORMODEINFO* pSensorModeInfo = m_pSensorInfo[cameraId];
        for(uint32_t mode = 0; mode < m_pCameraInfo[cameraId].numSensorModes; mode++)
        {
            long long currRes = pSensorModeInfo[mode].frameDimension.width*pSensorModeInfo[mode].frameDimension.height;
            if((currRes - givenRes) <= min && currRes >= givenRes)
            {
                min = currRes - givenRes;
                index = mode;
            }
        }
    }
    return &pSensorModeInfo[index];
}

/***************************************************************************************************************************
*   ChiModule::SubmitPipelineRequest
*
*   @brief
*       Submit the pipeline request (capture)
*   @param
*     [in]  CHIPIPELINEREQUEST            pRequest       pointer to the capture request
*   @return
*       CDKResult result code
****************************************************************************************************************************/
//CDKResult ChiModule::SubmitPipelineRequest(CHIPIPELINEREQUEST * pRequest) const
//{
//    for (uint32_t requestCount = 0; requestCount < pRequest->numRequests; requestCount++)
//    {
//        uint64_t frameNum = pRequest->pCaptureRequests[requestCount].frameNumber;
//        LogMsg(eDebug, CHIMODULE, "Sending pipeline request for frame number: %" PRIu64, frameNum);
//    }
//
//    return m_chiOps.pSubmitPipelineRequest(m_hContext, pRequest);
//}

/***************************************************************************************************************************
* ChiModule::GetContext
*
* @brief
*     Gets the camera context
* @param
*     None
* @return
*     CHIHANDLE  camera context
***************************************************************************************************************************/
CHIHANDLE ChiModule::GetContext() const
{
    return m_hContext;
}

/***************************************************************************************************************************
* ChiModule::GetChiOps
*
* @brief
*     Gets pointer to ChiOps
* @param
*     None
* @return
*     CHICONTEXTOPS*  pointer to chi APIs
***************************************************************************************************************************/
const CHICONTEXTOPS* ChiModule::GetChiOps() const
{
    return &m_chiOps;
}

/***************************************************************************************************************************
* ChiModule::GetFenceOps
*
* @brief
*     Gets pointer to fenceOps
* @param
*     None
* @return
*     CHIFENCEOPS*  pointer to chi fence APIs
***************************************************************************************************************************/
const CHIFENCEOPS* ChiModule::GetFenceOps() const
{
    return &m_fenceOps;
}

/***************************************************************************************************************************
* ChiModule::GetLibrary
*
* @brief
*     Gets symbols loaded through the dll/.so
* @param
*     None
* @return
*     VOID* Function pointer to library loaded
***************************************************************************************************************************/
VOID* ChiModule::GetLibrary() const
{
    return hLibrary;
}
