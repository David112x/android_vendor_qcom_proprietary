////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodegpu.cpp
/// @brief Chi node for GPU functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE PR008: Error: - Can't include library (bracketed) includes after quoted includes

#include <CL/cl_ext_qcom.h>
#include <system/camera_metadata.h>
#include "camxchinodegpu.h"
#include "camxutils.h"
#include "inttypes.h"

#undef LOG_TAG
#define LOG_TAG "CHIGPU"

/// @todo (CAMX-2282) Create CDK Utils and OS Utils that can be used from CDK/Chi nodes

#if defined (_LINUX)

const CHAR* pDefaultOpenCLLibraryName = "libOpenCL";

#elif defined (_WIN32)

const CHAR* pDefaultOpenCLLibraryName = "OpenCL";

#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32)

const CHAR* pOpenCLProgramBinName = "ChiNodeGPU.cl.bin";

static const UINT32 maxGPUWidth  = 16000;
static const UINT32 maxGPUHeight = 16000;

// =============================================================================================================================
// OpenCL Stuff
// =============================================================================================================================

// Supported OpenCL image formats
static const CLImageFormatInfo SupportedCLImageFormatInfo[] =
{
    {
        CL_QCOM_NV12,               CL_QCOM_NV12_Y,               CL_QCOM_NV12_UV,
        CL_UNORM_INT8,              CL_UNORM_INT8,                CL_UNORM_INT8
    }, // CL_QCOM_NV12
    {
        CL_QCOM_TP10,               CL_QCOM_TP10_Y,               CL_QCOM_TP10_UV,
        CL_QCOM_UNORM_INT10,        CL_QCOM_UNORM_INT10,          CL_QCOM_UNORM_INT10
    }, // CL_QCOM_TP10
    {
        CL_QCOM_P010,               CL_QCOM_P010_Y,               CL_QCOM_P010_UV,
        CL_QCOM_UNORM_INT10,        CL_QCOM_UNORM_INT10,          CL_QCOM_UNORM_INT10
    }, // CL_QCOM_P010
    {
        CL_QCOM_COMPRESSED_NV12,    CL_QCOM_COMPRESSED_NV12_Y,    CL_QCOM_COMPRESSED_NV12_UV,
        CL_UNORM_INT8,              CL_UNORM_INT8,                CL_UNORM_INT8
    }, // CL_QCOM_COMPRESSED_NV12
    {
        CL_QCOM_COMPRESSED_NV12_4R, CL_QCOM_COMPRESSED_NV12_4R_Y, CL_QCOM_COMPRESSED_NV12_4R_UV,
        CL_UNORM_INT8,              CL_UNORM_INT8,                CL_UNORM_INT8
    }, // CL_QCOM_COMPRESSED_NV12_4R
    {
        CL_QCOM_COMPRESSED_TP10,    CL_QCOM_COMPRESSED_TP10_Y,    CL_QCOM_COMPRESSED_TP10_UV,
        CL_QCOM_UNORM_INT10,        CL_QCOM_UNORM_INT10,          CL_QCOM_UNORM_INT10
    }  // CL_QCOM_COMPRESSED_TP10
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::GPUOpenCL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GPUOpenCL::GPUOpenCL()
    : m_initStatus(CLInitInvalid)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::~GPUOpenCL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GPUOpenCL::~GPUOpenCL()
{
    if (m_bIsBufferMappingEnabled == TRUE)
    {
        ReleaseYUVImagePlanes();
    }
    Uninitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeFuncPtrs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::InitializeFuncPtrs()
{
    CDKResult   result          = CDKResultSuccess;
    INT         numCharWritten  = 0;
    CHAR        libFilePath[FILENAME_MAX];

    numCharWritten = ChiNodeUtils::SNPrintF(libFilePath,
                                            FILENAME_MAX,
                                            "%s%s%s.%s",
                                            VendorLibPath,
                                            PathSeparator,
                                            pDefaultOpenCLLibraryName,
                                            SharedLibraryExtension);

    m_hOpenCLLib = ChiNodeUtils::LibMapFullName(libFilePath);

    if (NULL == m_hOpenCLLib)
    {
        result = CDKResultEUnableToLoad;
    }
    else
    {
        m_pfnCLGetPlatformIDs          = reinterpret_cast<PFNCLGETPLATFORMIDS>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clGetPlatformIDs"));
        m_pfnCLGetDeviceIDs            = reinterpret_cast<PFNCLGETDEVICEIDS>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clGetDeviceIDs"));
        m_pfnCLCreateContext           = reinterpret_cast<PFNCLCREATECONTEXT>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateContext"));
        m_pfnCLReleaseContext          = reinterpret_cast<PFNCLRELEASECONTEXT>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseContext"));
        m_pfnCLCreateCommandQueue      = reinterpret_cast<PFNCLCREATECOMMANDQUEUE>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateCommandQueue"));
        m_pfnCLReleaseCommandQueue     = reinterpret_cast<PFNCLRELEASECOMMANDQUEUE>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseCommandQueue"));
        m_pfnCLCreateImage2D           = reinterpret_cast<PFNCLCREATEIMAGE2D>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateImage2D"));
        m_pfnCLCreateImage             = reinterpret_cast<PFNCLCREATEIMAGE>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateImage"));
        m_pfnCLReleaseMemObject        = reinterpret_cast<PFNCLRELEASEMEMOBJECT>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseMemObject"));
        m_pfnCLCreateProgramWithSource = reinterpret_cast<PFNCLCREATEPROGRAMWITHSOURCE>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateProgramWithSource"));
        m_pfnCLCreateProgramWithBinary = reinterpret_cast<PFNCLCREATEPROGRAMWITHBINARY>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateProgramWithBinary"));
        m_pfnCLBuildProgram            = reinterpret_cast<PFNCLBUILDPROGRAM>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clBuildProgram"));
        m_pfnCLReleaseProgram          = reinterpret_cast<PFNCLRELEASEPROGRAM>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseProgram"));
        m_pfnCLGetProgramInfo          = reinterpret_cast<PFNCLGETPROGRAMINFO>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clGetProgramInfo"));
        m_pfnCLGetProgramBuildInfo     = reinterpret_cast<PFNCLGETPROGRAMBUILDINFO>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clGetProgramBuildInfo"));
        m_pfnCLCreateKernel            = reinterpret_cast<PFNCLCREATEKERNEL>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateKernel"));
        m_pfnCLReleaseKernel           = reinterpret_cast<PFNRELEASEKERNEL>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseKernel"));
        m_pfnCLSetKernelArg            = reinterpret_cast<PFNCLSETKERNELARG>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clSetKernelArg"));
        m_pfnCLEnqueueNDRangeKernel    = reinterpret_cast<PFNCLENQUEUENDRANGEKERNEL>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clEnqueueNDRangeKernel"));
        m_pfnCLFlush                   = reinterpret_cast<PFNCLFLUSH>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clFlush"));
        m_pfnCLFinish                  = reinterpret_cast<PFNCLFINISH>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clFinish"));
        m_pfnCLGetDeviceInfo           = reinterpret_cast<PFNCLGETDEVICEINFO>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clGetDeviceInfo"));
        m_pfnCLCreateBuffer            = reinterpret_cast<PFNCLCREATEBUFFER>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateBuffer"));
        m_pfnCLCreateSampler           = reinterpret_cast<PFNCLCREATESAMPLER>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clCreateSampler"));
        m_pfnCLReleaseSampler          = reinterpret_cast<PFNCLRELEASESAMPLER>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clReleaseSampler"));
        m_pfnCLEnqueueCopyBuffer       = reinterpret_cast<PFNCLENQUEUECOPYBUFFER>(
                                            ChiNodeUtils::LibGetAddr(m_hOpenCLLib, "clEnqueueCopyBuffer"));
    }

    if ((NULL == m_pfnCLGetPlatformIDs)             ||
        (NULL == m_pfnCLGetDeviceIDs)               ||
        (NULL == m_pfnCLCreateContext)              ||
        (NULL == m_pfnCLReleaseContext)             ||
        (NULL == m_pfnCLCreateCommandQueue)         ||
        (NULL == m_pfnCLReleaseCommandQueue)        ||
        (NULL == m_pfnCLCreateImage)                ||
        (NULL == m_pfnCLCreateImage2D)              ||
        (NULL == m_pfnCLReleaseMemObject)           ||
        (NULL == m_pfnCLCreateProgramWithSource)    ||
        (NULL == m_pfnCLCreateProgramWithBinary)    ||
        (NULL == m_pfnCLBuildProgram)               ||
        (NULL == m_pfnCLReleaseProgram)             ||
        (NULL == m_pfnCLGetProgramInfo)             ||
        (NULL == m_pfnCLGetProgramBuildInfo)        ||
        (NULL == m_pfnCLCreateKernel)               ||
        (NULL == m_pfnCLReleaseKernel)              ||
        (NULL == m_pfnCLSetKernelArg)               ||
        (NULL == m_pfnCLEnqueueNDRangeKernel)       ||
        (NULL == m_pfnCLFlush)                      ||
        (NULL == m_pfnCLFinish)                     ||
        (NULL == m_pfnCLGetDeviceInfo)              ||
        (NULL == m_pfnCLCreateBuffer)               ||
        (NULL == m_pfnCLCreateSampler)              ||
        (NULL == m_pfnCLReleaseSampler))
    {
        LOG_ERROR(CamxLogGroupChi,
                  "Error Initializing one or more function pointers in Library: %s",
                  libFilePath);

        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::Initialize()
{

    m_initStatus = CLInitRunning;

    CDKResult       result          = CDKResultSuccess;
    cl_platform_id  platform        = NULL;
    cl_int          error           = CL_SUCCESS;
    cl_uint         numPlatforms    = 0;

    result = InitializeFuncPtrs();

    if (CDKResultSuccess == result)
    {
        // Get the platform ID
        error = m_pfnCLGetPlatformIDs(1, &platform, &numPlatforms);

        if (CL_SUCCESS != error)
        {
            LOG_ERROR(CamxLogGroupChi, "Error getting the platform ID: %d", error);
            result = CDKResultEFailed;
        }

        // Get the requested device
        if (CDKResultSuccess == result)
        {
            error = m_pfnCLGetDeviceIDs(platform,  CL_DEVICE_TYPE_GPU, 1, &m_device, NULL);
            if (CL_SUCCESS != error)
            {
                LOG_ERROR(CamxLogGroupChi, "Error getting the requested Device: %d", error);
                result = CDKResultEFailed;
            }
        }

        if (CDKResultSuccess == result)
        {
            cl_context_properties properties[] = { CL_CONTEXT_PERF_HINT_QCOM,  CL_PERF_HINT_NORMAL_QCOM, 0};
            m_context = m_pfnCLCreateContext(properties, 1, &m_device, NULL, NULL, &error);

            if ((NULL == m_context) || (CL_SUCCESS != error))
            {
                LOG_ERROR(CamxLogGroupChi, "Error creating an OpenCL context: %d", error);
                result = CDKResultEUnsupported;
            }
        }

        if (CDKResultSuccess == result)
        {
            m_queue = m_pfnCLCreateCommandQueue(m_context, m_device, 0, &error);

            if ((NULL == m_queue) || (CL_SUCCESS != error))
            {
                LOG_ERROR(CamxLogGroupChi, "Error creating the OpenCL Command Queue: %d", error);
                result = CDKResultEUnsupported;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        result = CreateProgram();
    }

    if (CDKResultSuccess == result)
    {
        result = InitializeKernel();
    }

    if (CDKResultSuccess == result)
    {
        result = InitializeResources();
    }

    if (CDKResultSuccess == result)
    {
        m_pOpenCLMutex = CamX::Mutex::Create("OpenCLObject");

        if (NULL == m_pOpenCLMutex)
        {
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_initStatus = CLInitDone;
    }
    else
    {
        m_initStatus = CLInitInvalid;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::InitializeResources()
{
    cl_weight_image_desc_qcom weightDesc;
    cl_image_format           weightFmt;
    cl_half                   ds4Weights[2][8];
    cl_mem_ion_host_ptr       ionMem                 = { { 0 } };
    cl_image_format           input_1x1_image_format = { CL_RGBA, CL_UNSIGNED_INT32 };
    cl_image_desc             image_desc             = { 0 };
    CDKResult                 result                 = CDKResultSuccess;
    cl_int                    error                  = CL_FALSE;

    weightFmt.image_channel_data_type = CL_HALF_FLOAT;
    weightFmt.image_channel_order     = CL_R;

    memset(&weightDesc, 0, sizeof(cl_weight_image_desc_qcom));
    weightDesc.image_desc.image_type = CL_MEM_OBJECT_WEIGHT_IMAGE_QCOM;

    weightDesc.image_desc.image_width      = 8;
    weightDesc.image_desc.image_height     = 8;
    weightDesc.image_desc.image_array_size = 1;
    // Specify separable filter, default (flags=0) is 2D convolution filter
    weightDesc.weight_desc.flags           = CL_WEIGHT_IMAGE_SEPARABLE_QCOM;
    weightDesc.weight_desc.center_coord_x  = 3;
    weightDesc.weight_desc.center_coord_y  = 3;

    // Initialize weights
    static float ds4WeightsFloat[] = {
        (125.f / 1024.f),
        (91.f / 1024.f),
        (144.f / 1024.f),
        (152.f / 1024.f),
        (152.f / 1024.f),
        (144.f / 1024.f),
        (91.f / 1024.f),
        (125.f / 1024.f)
    };

    for (int i = 0; i < 8; i++)
    {
        ds4Weights[0][i] = (cl_half)ChiNodeUtils::FloatToHalf(ds4WeightsFloat[i]);
        ds4Weights[1][i] = (cl_half)ChiNodeUtils::FloatToHalf(ds4WeightsFloat[i]);
    }

    m_ds4WeightsImage = m_pfnCLCreateImage(m_context,
                                           CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                           &weightFmt,
                                           (cl_image_desc*)&weightDesc,
                                           (void*)ds4Weights,
                                           &error);

    if (CL_SUCCESS != error)
    {
        LOG_ERROR(CamxLogGroupChi, "Failed to create ds4 weights image with error %d", error);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // Create sampler
        m_ds4Sampler = m_pfnCLCreateSampler(m_context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &error);

        if (CL_SUCCESS != error)
        {
            LOG_ERROR(CamxLogGroupChi, "Failed to create ds4 sampler with error %d", error);
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CreateProgram
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::CreateProgram()
{
    CDKResult   result         = CDKResultSuccess;
    cl_int      err            = CL_FALSE;
    INT         numCharWritten = 0;
    CHAR        binFilePath[FILENAME_MAX];

    /// @todo (CAMX-4183) Check FileDumpPath read/write permission
    numCharWritten = ChiNodeUtils::SNPrintF(binFilePath,
                                            FILENAME_MAX,
                                            "%s%s%s",
                                            FileDumpPath,
                                            PathSeparator,
                                            pOpenCLProgramBinName);

    m_program = CreateProgramFromBinary(m_context, m_device, binFilePath);

    if (NULL == m_program)
    {
        // Create program from source
        LOG_VERBOSE(CamxLogGroupChi, "Fail to load program binary, create Program from source.");
        m_program = m_pfnCLCreateProgramWithSource(m_context, 1, reinterpret_cast<const CHAR**>(&m_pProgramSource), NULL, &err);

        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "clCreateProgramWithSource failed: error: %d", err);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            err = m_pfnCLBuildProgram(m_program, 1, &m_device, NULL, NULL, NULL);

            if (CL_SUCCESS != err)
            {
                CHAR log[512] = "\0";
                m_pfnCLGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
                LOG_ERROR(CamxLogGroupChi, "clBuildProgram failed: error: %d, Log: %s", err, log);
                result = CDKResultEFailed;
            }
        }

        if (CDKResultSuccess == result)
        {
            if (CDKResultSuccess != SaveProgramBinary(m_program, m_device, binFilePath))
            {
                // Not a fatal error
                LOG_WARN(CamxLogGroupChi, "Failed to write program binary");
            }
            else
            {
                LOG_VERBOSE(CamxLogGroupChi, "Save program binary successful %s", binFilePath);
            }
        }
    }
    else
    {
        LOG_VERBOSE(CamxLogGroupChi, "Load program binary %s successful!", binFilePath);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::SaveProgramBinary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::SaveProgramBinary(
    cl_program   program,
    cl_device_id device,
    const CHAR*  pFileName)
{
    CDKResult     result              = CDKResultSuccess;
    cl_uint       numDevices          = 0;
    cl_int        errNum;
    cl_device_id* pDevices            = NULL;
    size_t*       pProgramBinarySizes = NULL;
    UCHAR**       ppProgramBinaries   = NULL;

    // 1 - Query for number of devices attached to program
    errNum = m_pfnCLGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &numDevices, NULL);
    if (errNum != CL_SUCCESS)
    {
        LOG_ERROR(CamxLogGroupChi, "Error querying for number of devices. errNum = %d", errNum);
        result = CDKResultEFailed;
    }

    // 2 - Get all of the Device IDs
    if (CDKResultSuccess == result)
    {
        pDevices = reinterpret_cast<cl_device_id*>(CAMX_CALLOC(sizeof(cl_device_id) * numDevices));
        if (NULL == pDevices)
        {
            result = CDKResultENoMemory;
        }
        else
        {
            errNum = m_pfnCLGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id) * numDevices, pDevices, NULL);
            if (errNum != CL_SUCCESS)
            {
                LOG_ERROR(CamxLogGroupChi, "Error querying for devices. errNum = %d", errNum);
                result = CDKResultEFailed;
            }
        }
    }

    // 3 - Determine the size of each program binary
    if (CDKResultSuccess == result)
    {
        pProgramBinarySizes = reinterpret_cast<size_t*>(CAMX_CALLOC(sizeof(size_t) * numDevices));
        if (NULL == pProgramBinarySizes)
        {
            result = CDKResultENoMemory;
        }
        else
        {
            errNum = m_pfnCLGetProgramInfo(program,
                                           CL_PROGRAM_BINARY_SIZES,
                                           sizeof(size_t) * numDevices,
                                           pProgramBinarySizes,
                                           NULL);
            if (errNum != CL_SUCCESS)
            {
                LOG_ERROR(CamxLogGroupChi, "Error querying for program binary sizes. errNum = %d", errNum);
                result = CDKResultEFailed;
            }
            else
            {
                ppProgramBinaries = reinterpret_cast<UCHAR**>(CAMX_CALLOC(sizeof(UCHAR*) * numDevices));
                if (NULL == ppProgramBinaries)
                {
                    result = CDKResultENoMemory;
                }
                else
                {
                    for (cl_uint i = 0; i < numDevices; i++)
                    {
                        ppProgramBinaries[i] = reinterpret_cast<UCHAR*>(CAMX_CALLOC(sizeof(CHAR) * pProgramBinarySizes[i]));
                        if (NULL == ppProgramBinaries[i])
                        {
                            result = CDKResultENoMemory;
                            break;
                        }
                    }
                }
            }
        }
    }

    // 4 - Get all of the program binaries
    if (CDKResultSuccess == result)
    {
        errNum = m_pfnCLGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(UCHAR*) * numDevices, ppProgramBinaries, NULL);
        if (errNum != CL_SUCCESS)
        {
            LOG_ERROR(CamxLogGroupChi, "Error querying for program binaries. errNum = %d", errNum);
            result = CDKResultEFailed;
        }
    }

    // 5 - Finally store the binaries for the device requested out to disk for future reading.
    if (CDKResultSuccess == result)
    {
        for (cl_uint i = 0; i < numDevices; i++)
        {
            // Store the binary just for the device requested.  In a scenario where
            // multiple devices were being used you would save all of the binaries out here.
            if (pDevices[i] == device)
            {
                FILE* hProgBin = ChiNodeUtils::FOpen(pFileName, "wb");

                if (NULL != hProgBin)
                {
                    SIZE_T elemWritten = ChiNodeUtils::FWrite(ppProgramBinaries[i], 1, pProgramBinarySizes[i], hProgBin);

                    if (elemWritten != pProgramBinarySizes[i])
                    {
                        LOG_VERBOSE(CamxLogGroupChi,
                                    "FWrite failed to write %" PRIu64 ,
                                    static_cast<UINT64>(pProgramBinarySizes[i]));
                        result = CDKResultEFailed;
                    }
                    ChiNodeUtils::FClose(hProgBin);
                }
                else
                {
                    result = CDKResultEFailed;
                    LOG_VERBOSE(CamxLogGroupChi, "Fail to open file %s for device id = %d", pFileName, i);
                }
                break;
            }
        }
    }

    // Cleanup
    if (NULL != pDevices)
    {
        CAMX_FREE(pDevices);
    }
    if (NULL != pProgramBinarySizes)
    {
        CAMX_FREE(pProgramBinarySizes);
    }
    if (NULL != ppProgramBinaries)
    {
        for (cl_uint i = 0; i < numDevices; i++)
        {
            if (NULL != ppProgramBinaries[i])
            {
                CAMX_FREE(ppProgramBinaries[i]);
            }
        }
        CAMX_FREE(ppProgramBinaries);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CreateProgramFromBinary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cl_program GPUOpenCL::CreateProgramFromBinary(
    cl_context   context,
    cl_device_id device,
    const CHAR*  pFileName)
{
    cl_program program        = NULL;
    cl_int     err            = CL_SUCCESS;
    FILE*      hprogramBinary = ChiNodeUtils::FOpen(pFileName, "rb");

    if (NULL != hprogramBinary)
    {
        // Determine the size of the binary
        size_t binarySize;

        ChiNodeUtils::FSeek(hprogramBinary, 0, SEEK_END);
        binarySize = ChiNodeUtils::FTell(hprogramBinary);
        ChiNodeUtils::FSeek(hprogramBinary, 0, SEEK_SET);

        UCHAR* pProgramBinary = reinterpret_cast<UCHAR*>(CAMX_CALLOC(binarySize));
        if (NULL != pProgramBinary)
        {
            ChiNodeUtils::FRead (pProgramBinary, binarySize, binarySize, 1, hprogramBinary);

            cl_int     binaryStatus;
            program = m_pfnCLCreateProgramWithBinary(context,
                                                     1,
                                                     &device,
                                                     &binarySize,
                                                     (const UCHAR**)&pProgramBinary,
                                                     &binaryStatus,
                                                     &err);
            CAMX_FREE(pProgramBinary);

            if ((err != CL_SUCCESS) || (CL_SUCCESS != binaryStatus) || (NULL == program))
            {
                LOG_ERROR(CamxLogGroupChi, "clCreateProgramWithBinary failed: error: %d, status: %d", err, binaryStatus);
                program = NULL;
            }
            else
            {
                err = m_pfnCLBuildProgram(program, 0, NULL, NULL, NULL, NULL);
                if (err != CL_SUCCESS)
                {
                    // Determine the reason for the error
                    CHAR log[512] = "\0";

                    m_pfnCLGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
                    LOG_ERROR(CamxLogGroupChi, "clBuildProgram failed: error: %d, Log: %s", err, log);

                    m_pfnCLReleaseProgram(program);
                    program = NULL;
                }
            }
        }
        else
        {
            err = CL_MEM_OBJECT_ALLOCATION_FAILURE;
            LOG_ERROR(CamxLogGroupChi, "clCreateProgramWithBinary failed: error: %d", err);
        }
        ChiNodeUtils::FClose(hprogramBinary);
    }

    return program;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeKernel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::InitializeKernel()
{
    CDKResult   result  = CDKResultSuccess;
    cl_int      err     = CL_FALSE;


    // Create the copy kernel
    m_copyImageKernel = m_pfnCLCreateKernel(m_program, "copyImage", &err);

    if (CL_SUCCESS != err)
    {
        if (NULL != m_copyImageKernel)
        {
            m_pfnCLReleaseKernel(m_copyImageKernel);
            m_copyImageKernel = NULL;
        }

        LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the CopyImage Kernel failed: error: %d", err);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // Create the rotation kernel
        m_rotateImageKernel = m_pfnCLCreateKernel(m_program, "rotateImage", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_rotateImageKernel)
            {
                m_pfnCLReleaseKernel(m_rotateImageKernel);
                m_rotateImageKernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the RotateImage Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }


    if (CDKResultSuccess == result)
    {
        // Create the flip kernel
        m_flipImageKernel = m_pfnCLCreateKernel(m_program, "flipImage", &err);


        if (CL_SUCCESS != err)
        {
            if (NULL != m_flipImageKernel)
            {
                m_pfnCLReleaseKernel(m_flipImageKernel);
                m_flipImageKernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the FlipImage Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Create the ds4 kernel
        m_ds4SinglePlaneKernel = m_pfnCLCreateKernel(m_program, "ds4_single_plane", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_ds4SinglePlaneKernel)
            {
                m_pfnCLReleaseKernel(m_ds4SinglePlaneKernel);
                m_ds4SinglePlaneKernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the DS4SinglePlane Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }


    if (CDKResultSuccess == result)
    {
        // Create the box filter kernel
        m_boxFilterSinglePlaneKernel = m_pfnCLCreateKernel(m_program, "boxfilter_single_plane", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_boxFilterSinglePlaneKernel)
            {
                m_pfnCLReleaseKernel(m_boxFilterSinglePlaneKernel);
                m_boxFilterSinglePlaneKernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the BoxFilterSinglePlane Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Create the P010 to PD10 convert kernel
        m_p010topd10Kernel = m_pfnCLCreateKernel(m_program, "p010_to_pd10", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_p010topd10Kernel)
            {
                m_pfnCLReleaseKernel(m_p010topd10Kernel);
                m_p010topd10Kernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the p010_to_pd10 Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Create the P010 to UBWCTP10 convert kernel
        m_p010totp10Kernel = m_pfnCLCreateKernel(m_program, "p010_to_tp10", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_p010totp10Kernel)
            {
                m_pfnCLReleaseKernel(m_p010totp10Kernel);
                m_p010totp10Kernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the p010_to_tp10 Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Create the UBWCTP10 to UBWCTP10 copy kernel
        m_tp10totp10Kernel = m_pfnCLCreateKernel(m_program, "tp10_to_tp10", &err);

        if (CL_SUCCESS != err)
        {
            if (NULL != m_tp10totp10Kernel)
            {
                m_pfnCLReleaseKernel(m_tp10totp10Kernel);
                m_tp10totp10Kernel = NULL;
            }

            LOG_ERROR(CamxLogGroupChi, "clCreateKernel for the tp10_to_tp10 Kernel failed: error: %d", err);
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::FindEntryinMapTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GPUOpenCL::FindEntryinMapTable(
    INT      fd,
    VOID*    pAddr,
    cl_mem*  pYUVImage,
    cl_mem*  pYImage,
    cl_mem*  pUVImage)
{
    MapList                              foundMapList  = { 0 };
    BOOL                                 bIsEntryFound = FALSE;
    std::map<UINT32, MapList> ::iterator it;

    it = m_mapList.find(fd);
    if (it != m_mapList.end())
    {
        foundMapList = it->second;
        if (foundMapList.pAddr == pAddr)
        {
            *pYUVImage    = foundMapList.YUVImage;
            *pYImage      = foundMapList.YImage;
            *pUVImage     = foundMapList.UVImage;
            bIsEntryFound = TRUE;
            LOG_VERBOSE(CamxLogGroupChi, "fd %d already maaped YUVImage=%p YImage=%p UVImage=%p pAddr=%p",
                                         fd, *pYUVImage, *pYImage, *pUVImage, pAddr);
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi, "fd %d already maaped but pAddr %p mismatched", fd, pAddr);
            ReleaseSeparateYUVImagePlanes(foundMapList.YUVImage, foundMapList.YImage, foundMapList.UVImage);
            m_mapList.erase(fd);
        }
    }
    else
    {

        LOG_VERBOSE(CamxLogGroupChi, "Entry %d not found", fd);
    }

    return bIsEntryFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::AddEntryinMapTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GPUOpenCL::AddEntryinMapTable(
    INT      fd,
    VOID*    pAddr,
    cl_mem*  pYUVImage,
    cl_mem*  pYImage,
    cl_mem*  pUVImage)
{
    MapList                              addMapList = { 0 };
    std::map<UINT32, MapList> ::iterator it;

    it = m_mapList.find(fd);
    while (it != m_mapList.end())
    {
        ++it;
    }
    addMapList.YUVImage = *pYUVImage;
    addMapList.YImage   = *pYImage;
    addMapList.UVImage  = *pUVImage;
    addMapList.fd       = fd;
    addMapList.pAddr    = pAddr;
    m_mapList[fd]       = addMapList;
    LOG_VERBOSE(CamxLogGroupChi, "Mapped fd %d YUVImage=%p YImage=%p UVImage=%p pAddr=%p",
                              fd, *pYUVImage, *pYImage, *pUVImage, pAddr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CreateSeparateYUVImagePlanes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::CreateSeparateYUVImagePlanes(
    CHINODEBUFFERHANDLE hBuffer,
    UINT                imageIndex,
    cl_mem_flags        memoryAccessFlags,
    cl_mem*             pYUVImage,
    cl_mem*             pYImage,
    cl_mem*             pUVImage)
{
    CDKResult      result         = CDKResultSuccess;
    cl_int         err            = CL_SUCCESS;
    CLImageFormats format         = ConvertChiFormat(hBuffer->format.format);
    INT            fd             = hBuffer->pImageList[imageIndex].fd[0];
    VOID*          pAddr          = hBuffer->pImageList[imageIndex].pAddr[0];
    BOOL           bMapEntryFound = FALSE;

    LOG_INFO(CamxLogGroupChi,
             " Create Separate Image input params"
             " yuvFormat[0].height       = %d"
             " yuvFormat[0].width        = %d"
             " yuvFormat[0].planeStride  = %d"
             " yuvFormat[0].sliceHeight  = %d"
             " yuvFormat[1].height       = %d"
             " yuvFormat[1].width        = %d"
             " yuvFormat[1].planeStride  = %d"
             " yuvFormat[1].sliceHeight  = %d",
             hBuffer->format.formatParams.yuvFormat[0].height,
             hBuffer->format.formatParams.yuvFormat[0].width,
             hBuffer->format.formatParams.yuvFormat[0].planeStride,
             hBuffer->format.formatParams.yuvFormat[0].sliceHeight,
             hBuffer->format.formatParams.yuvFormat[1].height,
             hBuffer->format.formatParams.yuvFormat[1].width,
             hBuffer->format.formatParams.yuvFormat[1].planeStride,
             hBuffer->format.formatParams.yuvFormat[1].sliceHeight);

    if (m_bIsBufferMappingEnabled == TRUE)
    {
        bMapEntryFound = FindEntryinMapTable(fd, pAddr, pYUVImage, pYImage, pUVImage);
        if (bMapEntryFound == FALSE)
        {
            memoryAccessFlags = CL_MEM_READ_WRITE;
        }
    }

    if (bMapEntryFound == FALSE)
    {

        cl_image_desc       imageDesc = { 0 };
        cl_mem_ion_host_ptr ionMem    = { {0} };

        ionMem.ext_host_ptr.allocation_type   = CL_MEM_ION_HOST_PTR_QCOM;
        ionMem.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
        ionMem.ion_filedesc                   = hBuffer->pImageList[imageIndex].fd[0];
        ionMem.ion_hostptr                    = hBuffer->pImageList[imageIndex].pAddr[0];

        /// @note Passing in actual height and using image_slice_pitch should be the right way; we need to double check with CL
        ///       team
        imageDesc.image_type        = CL_MEM_OBJECT_IMAGE2D;
        imageDesc.image_width       = hBuffer->format.width;
        imageDesc.image_height      = hBuffer->format.formatParams.yuvFormat[0].height;
        imageDesc.image_row_pitch   = 0;
        imageDesc.image_slice_pitch = 0;

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Add support for UBWC, TP10 etc
        cl_image_format   imageFormat =
        {
            SupportedCLImageFormatInfo[format].channelOrder,
            SupportedCLImageFormatInfo[format].channelDataType
        };
        cl_image_format   imageYFormat =
        {
            SupportedCLImageFormatInfo[format].channelOrderY,
            SupportedCLImageFormatInfo[format].channelDataTypeY
        };
        cl_image_format   imageUVFormat =
        {
            SupportedCLImageFormatInfo[format].channelOrderUV,
            SupportedCLImageFormatInfo[format].channelDataTypeUV
        };

        *pYUVImage = m_pfnCLCreateImage(m_context,
            (CL_MEM_EXT_HOST_PTR_QCOM | CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR),
            &imageFormat,
            &imageDesc,
            reinterpret_cast<VOID*>(&ionMem),
            &err);

        if ((CL_SUCCESS == err) && (NULL != *pYUVImage))
        {
            imageDesc.image_type        = CL_MEM_OBJECT_IMAGE2D;
            imageDesc.image_width       = hBuffer->format.width;
            imageDesc.image_height      = hBuffer->format.formatParams.yuvFormat[0].height;
            imageDesc.image_array_size  = 1;
            imageDesc.image_row_pitch   = 0;
            imageDesc.image_slice_pitch = 0;
            imageDesc.mem_object = (cl_mem)(*pYUVImage);

            *pYImage = m_pfnCLCreateImage(m_context, memoryAccessFlags, &imageYFormat, &imageDesc, NULL, &err);

            if (CL_SUCCESS == err)
            {
                *pUVImage = m_pfnCLCreateImage(m_context, memoryAccessFlags, &imageUVFormat, &imageDesc, NULL, &err);
            }
        }

        if ((NULL == *pYUVImage) || (NULL == *pYImage) || (NULL == *pUVImage))
        {
            LOG_ERROR(CamxLogGroupChi, "clCreateImage failed: error: %d", err);
            result = CDKResultEFailed;
        }
        else if (m_bIsBufferMappingEnabled == TRUE)
        {
            AddEntryinMapTable(fd, pAddr, pYUVImage, pYImage, pUVImage);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ReleaseYUVImagePlanes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GPUOpenCL::ReleaseYUVImagePlanes()
{
    std::map<UINT32, MapList> ::iterator it;
    MapList                              deleteMapEntery = { 0 };
    it = m_mapList.begin();

    while (it != m_mapList.end())
    {
        deleteMapEntery = it->second;
        LOG_INFO(CamxLogGroupChi, "Deleting fd %d YUVImage=%p YImage=%p UVImage=%p pAddr=%p",
            deleteMapEntery.fd,
            deleteMapEntery.YUVImage,
            deleteMapEntery.YImage,
            deleteMapEntery.UVImage,
            deleteMapEntery.pAddr);
        ReleaseSeparateYUVImagePlanes(deleteMapEntery.YUVImage, deleteMapEntery.YImage, deleteMapEntery.UVImage);
        m_mapList.erase(it);
        ++it;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ReleaseSeparateYUVImagePlanes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ReleaseSeparateYUVImagePlanes(
    cl_mem yuvImage,
    cl_mem yImage,
    cl_mem uvImage)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != yImage)
    {
        m_pfnCLReleaseMemObject(yImage);
        yImage = NULL;
    }

    if (NULL != uvImage)
    {
        m_pfnCLReleaseMemObject(uvImage);
        uvImage = NULL;
    }

    if (NULL != yuvImage)
    {
        m_pfnCLReleaseMemObject(yuvImage);
        yuvImage = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ConvertChiFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLImageFormats GPUOpenCL::ConvertChiFormat(
    ChiFormat chiFormat)
{
    CLImageFormats openCLFormat = CLNV12Linear;

    switch (chiFormat)
    {
        case ChiFormat::YUV420NV12:
        case ChiFormat::YUV420NV21:
            openCLFormat = CLNV12Linear;
            break;
        case ChiFormat::P010:
            openCLFormat = CLP010Linear;
            break;
        case ChiFormat::YUV420NV12TP10:
            openCLFormat = CLTP10Linear;
            break;
        case ChiFormat::UBWCNV12:
            openCLFormat = CLNV12UBWC;
            break;
        case ChiFormat::UBWCNV124R:
            openCLFormat = CLNV124RUBWC;
            break;
        case ChiFormat::UBWCTP10:
            openCLFormat = CLTP10UBWC;
            break;
        default:
            LOG_ERROR(CamxLogGroupChi, "Unsupported Chi Format for OpenCL: %d, falling back to NV12", chiFormat);
            break;
    }

    return openCLFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::IsCompressedFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GPUOpenCL::IsCompressedFormat(
    CLImageFormats format)
{
    BOOL isCompressed = FALSE;

    switch (format)
    {
        case CLImageFormats::CLNV12UBWC:
        case CLImageFormats::CLNV124RUBWC:
        case CLImageFormats::CLTP10UBWC:
            isCompressed = TRUE;
            break;
        case CLImageFormats::CLNV12Linear:
        case CLImageFormats::CLTP10Linear:
        case CLImageFormats::CLP010Linear:
            isCompressed = FALSE;
            break;
        default:
            LOG_ERROR(CamxLogGroupChi, "Unknown format type: %d", format);
            break;
    }

    return isCompressed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteCopyImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteCopyImage(
    cl_mem  dst,
    cl_mem  src,
    UINT32  width,
    UINT32  height)
{
    CDKResult   result          = CDKResultSuccess;
    cl_int      err             = CL_SUCCESS;
    size_t      globalSize[2]   = {width, height};

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_copyImageKernel, 0, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_copyImageKernel, 1, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_copyImageKernel, 2, NULL, &globalSize[0], NULL, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteCopyImage failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteConvertP010ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteConvertP010ImageToUBWCTP10(
    cl_mem  dstY,
    cl_mem  dstUV,
    cl_mem  srcYUV,
    UINT32  width,
    UINT32  height)
{
    auto work_units = [](size_t x, size_t r)->size_t { return (x + r - 1) / r; };

    CDKResult   result        = CDKResultSuccess;
    cl_int      err           = CL_SUCCESS;
    UINT        argIdx        = 0;
    size_t      globalSize[2] = { work_units(width, 6), height };

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010totp10Kernel, argIdx++, sizeof(cl_mem), &srcYUV);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010totp10Kernel, argIdx++, sizeof(cl_mem), &dstY);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010totp10Kernel, argIdx++, sizeof(cl_mem), &dstUV);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010totp10Kernel, argIdx++, sizeof(cl_sampler), &m_ds4Sampler);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_p010totp10Kernel, 2, NULL, &globalSize[0], NULL, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteConvertP010ImageToUBWCTP10 failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteCopyUBWCTP10ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteCopyUBWCTP10ImageToUBWCTP10(
    cl_mem  dstY,
    cl_mem  dstUV,
    cl_mem  srcYUV,
    UINT32  width,
    UINT32  height)
{
    auto work_units = [](size_t x, size_t r)->size_t { return (x + r - 1) / r; };

    CDKResult   result        = CDKResultSuccess;
    cl_int      err           = CL_SUCCESS;
    UINT        argIdx        = 0;
    size_t      globalSize[2] = { work_units(width, 6), height };

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_tp10totp10Kernel, argIdx++, sizeof(cl_mem), &srcYUV);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_tp10totp10Kernel, argIdx++, sizeof(cl_mem), &dstY);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_tp10totp10Kernel, argIdx++, sizeof(cl_mem), &dstUV);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_tp10totp10Kernel, argIdx++, sizeof(cl_sampler), &m_ds4Sampler);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_tp10totp10Kernel, 2, NULL, &globalSize[0], NULL, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteCopyUBWCTP10ImageToUBWCTP10 failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteRotateImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteRotateImage(
    cl_mem          dst,
    cl_mem          src,
    UINT32          dstWidth,
    UINT32          dstHeight,
    UINT32          srcWidth,
    UINT32          srcHeight,
    RotationAngle   rotation)
{
    CDKResult   result        = CDKResultSuccess;
    cl_int      err           = CL_SUCCESS;
    size_t      globalSize[2] = { dstWidth, dstHeight };
    FLOAT       sinTheta;
    FLOAT       cosTheta;

    switch (rotation)
    {
    case RotationAngle::Rotate0Degrees:
        sinTheta = 0.0f;
        cosTheta = 1.0f;
        break;
    case RotationAngle::Rotate90Degrees:
        sinTheta = 1.0f;
        cosTheta = 0.0f;
        break;
    case RotationAngle::Rotate180Degrees:
        sinTheta = 0.0f;
        cosTheta = -1.0f;
        break;
    case RotationAngle::Rotate270Degrees:
        sinTheta = -1.0f;
        cosTheta = 0.0f;
        break;
    default:
        LOG_ERROR(CamxLogGroupChi, "Unknown rotation angle %d", rotation);
        err = CL_INVALID_VALUE;
        break;
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 0, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 1, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 2, sizeof(UINT32), &dstWidth);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 3, sizeof(UINT32), &dstHeight);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 4, sizeof(UINT32), &srcWidth);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 5, sizeof(UINT32), &srcHeight);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 6, sizeof(FLOAT), &sinTheta);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_rotateImageKernel, 7, sizeof(FLOAT), &cosTheta);
    }

    size_t lws_y[2] = { ChiNodeUtils::DivideAndCeil(dstWidth, 64), 8 };
    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_rotateImageKernel, 2, NULL, &globalSize[0], lws_y, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteRotateImage failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteFlipImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteFlipImage(
    cl_mem        dst,
    cl_mem        src,
    UINT32        width,
    UINT32        height,
    UINT32        inStride,
    UINT32        outStride,
    UINT32        inUVOffset,
    UINT32        outUVOffset,
    FlipDirection direction)
{
    CDKResult   result          = CDKResultSuccess;
    cl_int      err             = CL_SUCCESS;
    size_t      globalSize[2]   = {width, height};

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 0, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 1, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 2, sizeof(UINT32), &width);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 3, sizeof(UINT32), &height);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 4, sizeof(UINT32), &inStride);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 5, sizeof(UINT32), &outStride);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 6, sizeof(UINT32), &inUVOffset);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 7, sizeof(UINT32), &outUVOffset);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 8, sizeof(UINT32), &direction);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_flipImageKernel, 2, NULL, &globalSize[0], NULL, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteFlipImage failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteConvertP010ImageToPD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteConvertP010ImageToPD10(
    cl_mem        dst,
    cl_mem        src,
    UINT32        width,
    UINT32        height,
    UINT32        inStride,
    UINT32        inScanlines,
    UINT32        outStride)
{
    CDKResult   result          = CDKResultSuccess;
    cl_int      err             = CL_SUCCESS;
    size_t      globalSize[2]   = {width/2, height/2};

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010topd10Kernel, 0, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010topd10Kernel, 1, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010topd10Kernel, 2, sizeof(UINT32), &inStride);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010topd10Kernel, 3, sizeof(UINT32), &inScanlines);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_p010topd10Kernel, 4, sizeof(UINT32), &outStride);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_p010topd10Kernel, 2, NULL, &globalSize[0], NULL, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteConvertP010ImageToPD10 failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CopyBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::CopyBuffer(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult result = CDKResultSuccess;

    cl_int   err = CL_SUCCESS;

    LOG_WARN(CamxLogGroupChi, "GPUOpenCL::CopyBuffer : NOT TESTED");

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstBuffer = NULL;
        cl_mem  srcBuffer = NULL;

        UINT32  width           = hInput->format.width;
        UINT32  height          = hInput->format.height;
        UINT32  inStride        = hInput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  outStride       = hOutput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  inHeightStride  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
        UINT32  outHeightStride = hInput->format.formatParams.yuvFormat[0].sliceHeight;

        UINT32 totalSize = 0;
        for (UINT32 j = 0; j < hOutput->numberOfPlanes; j++)
        {
            totalSize += hOutput->planeSize[j];
        }

        // Create 1x1 Buffer for input on top of the original ion allocation
        cl_mem_ion_host_ptr ionMem              = { {0} };
        ionMem.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionMem.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionMem.ion_filedesc                     = hInput->pImageList[i].fd[0];
        ionMem.ion_hostptr                      = hInput->pImageList[i].pAddr[0];

        srcBuffer = m_pfnCLCreateBuffer(m_context,
                                        (CL_MEM_READ_WRITE | CL_MEM_EXT_HOST_PTR_QCOM),
                                        totalSize,
                                        reinterpret_cast<VOID*>(&ionMem),
                                        &err);

        if (CL_SUCCESS == err)
        {
            // Create 1x1 Buffer for output on top of the original ion allocation
            ionMem.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
            ionMem.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
            ionMem.ion_filedesc                     = hOutput->pImageList[i].fd[0];
            ionMem.ion_hostptr                      = hOutput->pImageList[i].pAddr[0];

            dstBuffer = m_pfnCLCreateBuffer(m_context,
                                            (CL_MEM_READ_WRITE | CL_MEM_EXT_HOST_PTR_QCOM),
                                            totalSize,
                                            reinterpret_cast<VOID*>(&ionMem),
                                            &err);
        }

        if (CL_SUCCESS == err)
        {
             err = m_pfnCLEnqueueCopyBuffer(m_queue, srcBuffer, dstBuffer, 0, 0, totalSize, 0, NULL, NULL);
        }

        if (CL_SUCCESS == err)
        {
            err = m_pfnCLFinish(m_queue);
        }

        if (NULL != srcBuffer)
        {
            m_pfnCLReleaseMemObject(srcBuffer);
            srcBuffer = NULL;
        }

        if (NULL != dstBuffer)
        {
            m_pfnCLReleaseMemObject(dstBuffer);
            dstBuffer = NULL;
        }

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CopyImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::CopyImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result  = CDKResultSuccess;
    cl_int      err     = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    // CAMX_ASSERT(hInput->formatParams.format.format == YUV420NV12);
    // CAMX_ASSERT(hInput->numberOfPlanes == 2);
    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYUVImage = NULL;
        cl_mem  srcYUVImage = NULL;
        cl_mem  dstYImage   = NULL;
        cl_mem  srcYImage   = NULL;
        cl_mem  dstUVImage  = NULL;
        cl_mem  srcUVImage  = NULL;
        UINT32  width       = hOutput->format.width;
        UINT32  height      = hOutput->format.height;
        // UINT32  sliceHeightY  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
        // UINT32  sliceHeightUV = hInput->format.formatParams.yuvFormat[1].sliceHeight;

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_WRITE_ONLY, &dstYUVImage, &dstYImage, &dstUVImage);

        if (CDKResultSuccess == result)
        {
            result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_ONLY, &srcYUVImage, &srcYImage, &srcUVImage);
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
           if ((NULL != dstYImage) && (NULL != srcYImage))
           {
               result = ExecuteCopyImage(dstYImage, srcYImage, width, height);
           }

           if ((NULL != dstUVImage) && (NULL != srcUVImage))
           {
               UINT32 uvPlaneWidth = (hInput->format.format == P010) ? width : (width >> 1);
               result = ExecuteCopyImage(dstUVImage, srcUVImage, uvPlaneWidth, height >> 1);
           }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }
        m_pOpenCLMutex->Unlock();

        if (m_bIsBufferMappingEnabled == FALSE)
        {
            ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
            ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::CopyImage failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::RotateImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::RotateImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput,
    RotationAngle       targetRotation)
{
    CDKResult   result = CDKResultSuccess;
    cl_int      err = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    // CAMX_ASSERT(hInput->formatParams.format.format == YUV420NV12);
    // CAMX_ASSERT(hInput->numberOfPlanes == 2);
    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYUVImage = NULL;
        cl_mem  srcYUVImage = NULL;
        cl_mem  dstYImage   = NULL;
        cl_mem  srcYImage   = NULL;
        cl_mem  dstUVImage  = NULL;
        cl_mem  srcUVImage  = NULL;
        UINT32  dstWidth    = 0;
        UINT32  dstHeight   = 0;
        UINT32  srcWidth    = hInput->format.width;
        UINT32  srcHeight   = hInput->format.height;

        // If rotated, output buffer rotation needs to have width and height swapped
        if ((RotationAngle::Rotate90Degrees == targetRotation) ||
            (RotationAngle::Rotate270Degrees == targetRotation))
        {
            dstWidth = srcHeight;
            dstHeight = srcWidth;
        }
        else
        {
            dstWidth = srcWidth;
            dstHeight = srcHeight;
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_WRITE_ONLY, &dstYUVImage, &dstYImage, &dstUVImage);

        if (CDKResultSuccess == result)
        {
            result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_ONLY, &srcYUVImage, &srcYImage, &srcUVImage);
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
            if ((NULL != dstYImage) && (NULL != srcYImage))
            {
                result = ExecuteRotateImage(dstYImage,
                                            srcYImage,
                                            dstWidth,
                                            dstHeight,
                                            srcWidth,
                                            srcHeight,
                                            targetRotation);
            }
        }

        if (CDKResultSuccess == result)
        {
            if ((NULL != dstUVImage) && (NULL != srcUVImage))
            {
                result = ExecuteRotateImage(dstUVImage,
                                            srcUVImage,
                                            (dstWidth >> 1),
                                            (dstHeight >> 1),
                                            (srcWidth >> 1),
                                            (srcHeight >> 1),
                                            targetRotation);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }
        m_pOpenCLMutex->Unlock();

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }

        if (m_bIsBufferMappingEnabled == FALSE)
        {
            ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
            ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::CopyImage failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::FlipImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::FlipImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput,
    FlipDirection       direction)
{
    CDKResult   result  = CDKResultSuccess;
    cl_int      err     = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    // CAMX_ASSERT(hInput->formatParams.format.format == YUV420NV12);
    // CAMX_ASSERT(hInput->numberOfPlanes == 2);
    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYBuffer  = 0;
        cl_mem  srcYBuffer  = 0;
        cl_mem  dstUVBuffer = 0;
        cl_mem  srcUVBuffer = 0;
        UINT32  width       = hInput->format.width;
        UINT32  height      = hInput->format.height;
        UINT32  inStride    = hInput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  outStride   = hOutput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  inUVOffset  = 0;
        UINT32  outUVOffset = 0;

        LOG_ERROR(CamxLogGroupChi, "Input width: %d, inStride is %d, output stride is %d ", width, inStride, outStride);


        cl_mem_ion_host_ptr ionmemDstY              = {{0}};
        ionmemDstY.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemDstY.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemDstY.ion_filedesc                     = hOutput->pImageList[i].fd[0];
        ionmemDstY.ion_hostptr                      = hOutput->pImageList[i].pAddr[0];

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        dstYBuffer = m_pfnCLCreateBuffer(m_context,
                                         (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                         (inStride * height),
                                         reinterpret_cast<VOID*>(&ionmemDstY),
                                         &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::FlipImage failed to create clBuffer dstY: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemDstUV              = {{0}};
        ionmemDstUV.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemDstUV.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemDstUV.ion_filedesc                     = hOutput->pImageList[i].fd[1];
        ionmemDstUV.ion_hostptr                      = hOutput->pImageList[i].pAddr[1];

        dstUVBuffer = m_pfnCLCreateBuffer(m_context,
                                          (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                          (inStride * height / 2),
                                          reinterpret_cast<VOID*>(&ionmemDstUV),
                                          &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::FlipImage failed to create clBuffer dstUV: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemSrcY              = {{0}};
        ionmemSrcY.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemSrcY.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemSrcY.ion_filedesc                     = hInput->pImageList[i].fd[0];
        ionmemSrcY.ion_hostptr                      = hInput->pImageList[i].pAddr[0];

        srcYBuffer = m_pfnCLCreateBuffer(m_context,
                                         (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                         (inStride * height),
                                         reinterpret_cast<VOID*>(&ionmemSrcY),
                                         &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::FlipImage failed to create clBuffer srcY: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemSrcUV              = {{0}};
        ionmemSrcUV.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemSrcUV.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemSrcUV.ion_filedesc                     = hInput->pImageList[i].fd[1];
        ionmemSrcUV.ion_hostptr                      = hInput->pImageList[i].pAddr[1];

        srcUVBuffer = m_pfnCLCreateBuffer(m_context,
                                          (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                          (inStride * height / 2),
                                          reinterpret_cast<VOID*>(&ionmemSrcUV),
                                          &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::FlipImage failed to create clBuffer srcUV: error: %d", err);
            result  = CDKResultEFailed;
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
            if ((NULL != dstYBuffer) && (NULL != srcYBuffer))
            {
                result = ExecuteFlipImage(dstYBuffer,
                                        srcYBuffer,
                                        width,
                                        height,
                                        inStride,
                                        outStride,
                                        0,
                                        0,
                                        direction);
            }
        }

        if (CDKResultSuccess == result)
        {
            if ((NULL != dstUVBuffer) && (NULL != srcUVBuffer))
            {
                inUVOffset  = hInput->pImageList[i].pAddr[1] - hInput->pImageList[i].pAddr[0];
                outUVOffset = hOutput->pImageList[i].pAddr[1] - hOutput->pImageList[i].pAddr[0];
                result      = ExecuteFlipImage(dstUVBuffer,
                                            srcUVBuffer,
                                            (width >> 1),
                                            (height >> 1),
                                            inStride,
                                            outStride,
                                            inUVOffset,
                                            outUVOffset,
                                            direction);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }
        m_pOpenCLMutex->Unlock();

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }

        if (0 != dstYBuffer)
        {
            m_pfnCLReleaseMemObject(dstYBuffer);
        }
        if (0 != dstUVBuffer)
        {
            m_pfnCLReleaseMemObject(dstUVBuffer);
        }
        if (0 != srcYBuffer)
        {
            m_pfnCLReleaseMemObject(srcYBuffer);
        }
        if (0 != srcUVBuffer)
        {
            m_pfnCLReleaseMemObject(srcUVBuffer);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::FlipImage failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::DownscaleImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::DownscaleBy4Image(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result = CDKResultSuccess;
    cl_int      err = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    // CAMX_ASSERT(hInput->formatParams.format.format == YUV420NV12);
    // CAMX_ASSERT(hInput->numberOfPlanes == 2);
    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYUVImage;
        cl_mem  srcYUVImage;
        cl_mem  dstYImage;
        cl_mem  srcYImage;
        cl_mem  dstUVImage;
        cl_mem  srcUVImage;
        UINT32  srcWidth  = hInput->format.width;
        UINT32  srcHeight = hInput->format.height;
        UINT32  dstWidth  = ChiNodeUtils::DivideAndCeil(srcWidth, 4) * 2;
        UINT32  dstHeight = ChiNodeUtils::DivideAndCeil(srcHeight, 4) * 2;

        // 20.0f is roughly large value not to use srcYMax in CL Kernel
        FLOAT   srcYMax   = hInput->format.height + 20.f;

        // Slice height being passed in the format is bogus, calculating manually until that is fixed. (this was due to a packing issue)
        UINT32  srcSliceHeightY  = hInput->format.formatParams.yuvFormat[0].sliceHeight; // ChiNodeUtils::AlignGeneric32(srcHeight, 32);
        UINT32  srcSliceHeightUV = hInput->format.formatParams.yuvFormat[1].sliceHeight; // ChiNodeUtils::AlignGeneric32((srcHeight / 2), 16);

        UINT32  dstSliceHeightY  = hOutput->format.formatParams.yuvFormat[0].sliceHeight;
        UINT32  dstSliceHeightUV = hOutput->format.formatParams.yuvFormat[1].sliceHeight;

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_WRITE_ONLY, &dstYUVImage, &dstYImage, &dstUVImage);
        // CAMX_ASSERT(CDKResultSuccess == result);
        result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_ONLY, &srcYUVImage, &srcYImage, &srcUVImage);
        // CAMX_ASSERT(CDKResultSuccess == result);

        m_pOpenCLMutex->Lock();

        if ((CDKResultSuccess == result) && (NULL != dstYImage) && (NULL != srcYImage))
        {
            result = ExecuteDownscaleBy4SinglePlane(dstYImage,
                                                    srcYImage,
                                                    dstWidth,
                                                    dstHeight,
                                                    srcYMax);
            // CAMX_ASSERT(CDKResultSuccess == result);
        }

        if ((CDKResultSuccess == result) && (NULL != dstUVImage) && (NULL != srcUVImage))
        {
            result = ExecuteBoxFilterSinglePlane(dstUVImage,
                                                 srcUVImage,
                                                 4.0f,
                                                 4.0f,
                                                 (dstWidth  >> 1),
                                                 (dstHeight >> 1),
                                                 srcYMax);
            // CAMX_ASSERT(CDKResultSuccess == result);
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }

        m_pOpenCLMutex->Unlock();

        if (m_bIsBufferMappingEnabled == FALSE)
        {
            ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
            ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
        }

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess != result)
        {
            // Don't continue trying to execute if anything has failed.
            break;
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::DownscaleBy4Image failed: result: %d, error: %d", result, err);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::DownscaleImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::DownscaleImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result = CDKResultSuccess;
    cl_int      err = CL_SUCCESS;

    if (2 == hInput->numberOfPlanes)
    {
        for (UINT i = 0; i < hOutput->imageCount; i++)
        {
            cl_mem  dstYUVImage = NULL;
            cl_mem  srcYUVImage = NULL;
            cl_mem  dstYImage   = NULL;
            cl_mem  srcYImage   = NULL;
            cl_mem  dstUVImage  = NULL;
            cl_mem  srcUVImage  = NULL;
            UINT32  srcWidth    = hInput->format.width;
            UINT32  srcHeight   = hInput->format.height;
            UINT32  dstWidth    = hOutput->format.width;
            UINT32  dstHeight   = hOutput->format.height;
            FLOAT   widthScale  = static_cast<FLOAT>(srcWidth) / static_cast<FLOAT>(dstWidth);
            FLOAT   heightScale = static_cast<FLOAT>(srcHeight) / static_cast<FLOAT>(dstHeight);

            // 20.0f is roughly large value not to use srcYMax in CL Kernel
            FLOAT   srcYMax   = hInput->format.height + 20.f;

            UINT32  srcSliceHeightY  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
            UINT32  srcSliceHeightUV = hInput->format.formatParams.yuvFormat[1].sliceHeight;

            UINT32  dstSliceHeightY  = hOutput->format.formatParams.yuvFormat[0].sliceHeight;
            UINT32  dstSliceHeightUV = hOutput->format.formatParams.yuvFormat[1].sliceHeight;

            /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
            ///                   ION buffers per-process request
            result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_READ_WRITE, &dstYUVImage, &dstYImage, &dstUVImage);

            if (CDKResultSuccess == result)
            {
                result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_WRITE, &srcYUVImage, &srcYImage, &srcUVImage);
            }

            m_pOpenCLMutex->Lock();

            if ((CDKResultSuccess == result) && (NULL != dstYImage) && (NULL != srcYImage))
            {
                result = ExecuteBoxFilterSinglePlane(dstYImage,
                                                     srcYImage,
                                                     widthScale,
                                                     heightScale,
                                                     dstWidth,
                                                     dstHeight,
                                                     srcYMax);
                if (CDKResultSuccess != result)
                {
                    LOG_ERROR(CamxLogGroupChi, "Failed to downscale the first plane");
                }
            }

            if ((CDKResultSuccess == result) && (NULL != dstUVImage) && (NULL != srcUVImage))
            {
                result = ExecuteBoxFilterSinglePlane(dstUVImage,
                                                     srcUVImage,
                                                     widthScale,
                                                     heightScale,
                                                     (dstWidth  >> 1),
                                                     (dstHeight >> 1),
                                                     srcYMax);
                if (CDKResultSuccess != result)
                {
                    LOG_ERROR(CamxLogGroupChi, "Failed to downscale the second plane");
                }
            }

            /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to
            ///                   use events and spawn a thread to wait on those events, and then signal the buffer to unblock
            ///                   the next node in the pipeline.
            if (CDKResultSuccess == result)
            {
                err = m_pfnCLFinish(m_queue);
            }

            m_pOpenCLMutex->Unlock();

            if (m_bIsBufferMappingEnabled == FALSE)
            {
                ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
                ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
            }

            if (CL_SUCCESS != err)
            {
                result = CDKResultEFailed;
            }

            if (CDKResultSuccess != result)
            {
                // Don't continue trying to execute if anything has failed.
                break;
            }
        }
    }
    else
    {
        result = CDKResultEInvalidArg;
        LOG_ERROR(CamxLogGroupChi, "Unable to downscale image with current format");
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::DownscaleImage failed: result: %d, error: %d", result, err);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteDownscaleBy4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteDownscaleBy4SinglePlane(
    cl_mem          dst,
    cl_mem          src,
    UINT32          dstWidth,
    UINT32          dstHeight,
    FLOAT           srcYMax)
{
    CDKResult   result        = CDKResultSuccess;
    cl_int      err           = CL_SUCCESS;
    UINT        argIdx        = 0;
    // Set the global work size to the 4x1 scale
    size_t      globalSize[2] = { dstWidth, dstHeight };

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_ds4SinglePlaneKernel, argIdx++, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_ds4SinglePlaneKernel, argIdx++, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_ds4SinglePlaneKernel, argIdx++, sizeof(cl_mem), &m_ds4WeightsImage);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_ds4SinglePlaneKernel, argIdx++, sizeof(cl_sampler), &m_ds4Sampler);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_ds4SinglePlaneKernel, argIdx++, sizeof(FLOAT), &srcYMax);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteDownscaleBy4SinglePlane failed setting arg %u with err %d", argIdx, err);
    }

    size_t lws_y[2] = { ChiNodeUtils::DivideAndCeil(dstWidth, 64), 8 };
    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_ds4SinglePlaneKernel, 2, NULL, &globalSize[0], lws_y, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteDownscaleBy4SinglePlane failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteBoxFilterSinglePlane
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ExecuteBoxFilterSinglePlane(
    cl_mem          dst,
    cl_mem          src,
    FLOAT           scaleX,
    FLOAT           scaleY,
    UINT32          dstWidth,
    UINT32          dstHeight,
    FLOAT           srcYMax)
{
    CDKResult   result        = CDKResultSuccess;
    cl_int      err           = CL_SUCCESS;
    UINT        argIdx        = 0;
    // Set the global work size to the 4x1 scale
    size_t      globalSize[2] = { dstWidth, dstHeight };

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(cl_sampler), &m_ds4Sampler);
    }

    if (CL_SUCCESS == err)
    {
        cl_box_filter_size_qcom scaleFactors;

        scaleFactors.box_filter_width  = scaleX;
        scaleFactors.box_filter_height = scaleY;

        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(cl_box_filter_size_qcom), &scaleFactors);
    }

    if (CL_SUCCESS == err)
    {
        cl_float2 unpackedScales;

        unpackedScales.s[0] = scaleX;
        unpackedScales.s[1] = scaleY;

        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(cl_float2), &unpackedScales);
    }

    if (CL_SUCCESS == err)
    {
        err = m_pfnCLSetKernelArg(m_boxFilterSinglePlaneKernel, argIdx++, sizeof(FLOAT), &srcYMax);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteBoxFilterSinglePlane failed setting arg %u with err %d", argIdx, err);
    }

    size_t lws_y[2] = { ChiNodeUtils::DivideAndCeil(dstWidth, 64), 8 };
    if (CL_SUCCESS == err)
    {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_boxFilterSinglePlaneKernel, 2, NULL, &globalSize[0], lws_y, 0, NULL, NULL);
    }

    if (CL_SUCCESS != err)
    {
        LOG_ERROR(CamxLogGroupChi, "ExecuteBoxFilterSinglePlane failed: error: %d", err);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ConvertP010ImageToPD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ConvertP010ImageToPD10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result  = CDKResultSuccess;
    cl_int      err     = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    // CAMX_ASSERT(hInput->formatParams.format.format == YUV420NV12);
    // CAMX_ASSERT(hInput->numberOfPlanes == 2);
    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstBuffer    = 0;
        cl_mem  srcBuffer    = 0;
        UINT32  width        = hOutput->format.width;
        UINT32  height       = hOutput->format.height;
        UINT32  inStride     = hInput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  inScanlines  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
        UINT32  outStride    = hOutput->format.formatParams.yuvFormat[0].planeStride;
        UINT32  outScanlines = hOutput->format.formatParams.yuvFormat[0].sliceHeight;

        LOG_ERROR(CamxLogGroupChi, "Input width: %d, inStride is %d, output stride is %d ", width, inStride, outStride);


        cl_mem_ion_host_ptr ionmemDstY              = {{0}};
        ionmemDstY.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemDstY.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemDstY.ion_filedesc                     = hOutput->pImageList[i].fd[0];
        ionmemDstY.ion_hostptr                      = hOutput->pImageList[i].pAddr[0];

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        dstBuffer = m_pfnCLCreateBuffer(m_context,
                                        (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                        (outStride * outScanlines),
                                        reinterpret_cast<VOID*>(&ionmemDstY),
                                        &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::ConvertP010ImageToPD10 failed to create clBuffer dst: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemSrcY              = {{0}};
        ionmemSrcY.ext_host_ptr.allocation_type     = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemSrcY.ext_host_ptr.host_cache_policy   = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemSrcY.ion_filedesc                     = hInput->pImageList[i].fd[0];
        ionmemSrcY.ion_hostptr                      = hInput->pImageList[i].pAddr[0];

        srcBuffer = m_pfnCLCreateBuffer(m_context,
                                        (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                        (inStride * inScanlines),
                                        reinterpret_cast<VOID*>(&ionmemSrcY),
                                        &err);
        if (CL_SUCCESS != err)
        {
            LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::ConvertP010ImageToPD10 failed to create clBuffer src: error: %d", err);
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
            if ((NULL != dstBuffer) && (NULL != srcBuffer))
            {
                result = ExecuteConvertP010ImageToPD10(dstBuffer,
                                                       srcBuffer,
                                                       width,
                                                       height,
                                                       inStride,
                                                       inScanlines,
                                                       outStride);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }
        m_pOpenCLMutex->Unlock();

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }

        if (0 != dstBuffer)
        {
            m_pfnCLReleaseMemObject(dstBuffer);
        }
        if (0 != srcBuffer)
        {
            m_pfnCLReleaseMemObject(srcBuffer);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::ConvertP010ImageToPD10 failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ConvertP010ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::ConvertP010ImageToUBWCTP10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result = CDKResultSuccess;
    cl_int      err = CL_SUCCESS;

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYUVImage = NULL;
        cl_mem  srcYUVImage = NULL;
        cl_mem  dstYImage   = NULL;
        cl_mem  srcYImage   = NULL;
        cl_mem  dstUVImage  = NULL;
        cl_mem  srcUVImage  = NULL;
        UINT32  width       = hOutput->format.width;
        UINT32  height      = hOutput->format.height;
        // UINT32  sliceHeightY  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
        // UINT32  sliceHeightUV = hInput->format.formatParams.yuvFormat[1].sliceHeight;

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_READ_WRITE, &dstYUVImage, &dstYImage, &dstUVImage);

        if (CDKResultSuccess == result)
        {
            result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_ONLY, &srcYUVImage, &srcYImage, &srcUVImage);
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
            if ((NULL != dstYImage) && (NULL != srcYImage) && (NULL != dstUVImage) && (NULL != srcUVImage))
            {
                result = ExecuteConvertP010ImageToUBWCTP10(dstYImage, dstUVImage, srcYUVImage, width, height);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }
        m_pOpenCLMutex->Unlock();

        if (m_bIsBufferMappingEnabled == FALSE)
        {
            ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
            ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::ConvertP010ImageToUBWCTP10 failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::CopyUBWCTP10ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::CopyUBWCTP10ImageToUBWCTP10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    CDKResult   result = CDKResultSuccess;
    cl_int      err = CL_SUCCESS;

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        cl_mem  dstYUVImage = NULL;
        cl_mem  srcYUVImage = NULL;
        cl_mem  dstYImage   = NULL;
        cl_mem  srcYImage   = NULL;
        cl_mem  dstUVImage  = NULL;
        cl_mem  srcUVImage  = NULL;
        UINT32  width       = hOutput->format.width;
        UINT32  height      = hOutput->format.height;
        // UINT32  sliceHeightY  = hInput->format.formatParams.yuvFormat[0].sliceHeight;
        // UINT32  sliceHeightUV = hInput->format.formatParams.yuvFormat[1].sliceHeight;

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and Release
        ///                   ION buffers per-process request
        result = CreateSeparateYUVImagePlanes(hOutput, i, CL_MEM_READ_WRITE, &dstYUVImage, &dstYImage, &dstUVImage);

        if (CDKResultSuccess == result)
        {
            result = CreateSeparateYUVImagePlanes(hInput, i, CL_MEM_READ_ONLY, &srcYUVImage, &srcYImage, &srcUVImage);
        }

        m_pOpenCLMutex->Lock();
        if (CDKResultSuccess == result)
        {
            if ((NULL != dstYImage) && (NULL != srcYImage) && (NULL != dstUVImage) && (NULL != srcUVImage))
            {
                result = ExecuteCopyUBWCTP10ImageToUBWCTP10(dstYImage, dstUVImage, srcYUVImage, width, height);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the buffer to unblock the
        ///                   next node in the pipeline.
        if (CDKResultSuccess == result)
        {
            err = m_pfnCLFinish(m_queue);
        }

        if (CL_SUCCESS != err)
        {
            result = CDKResultEFailed;
        }
        m_pOpenCLMutex->Unlock();

        if (m_bIsBufferMappingEnabled == FALSE)
        {
            ReleaseSeparateYUVImagePlanes(srcYUVImage, srcYImage, srcUVImage);
            ReleaseSeparateYUVImagePlanes(dstYUVImage, dstYImage, dstUVImage);
        }
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::CopyUBWCTP10ImageToUBWCTP10 failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GPUOpenCL::Uninitialize()
{
    CDKResult result = CDKResultSuccess;

    cl_int err = CL_SUCCESS;

    if (NULL != m_pOpenCLMutex)
    {
        m_pOpenCLMutex->Destroy();
        m_pOpenCLMutex = NULL;
    }

    if (NULL != m_queue)
    {
        err = m_pfnCLReleaseCommandQueue(m_queue);
        LOG_VERBOSE(CamxLogGroupChi, "GPUOpenCL ReleaseCommandQueue error -0 means success-: %d", err);
        m_queue = NULL;
    }

    if (NULL != m_context)
    {
        err = m_pfnCLReleaseContext(m_context);
        LOG_VERBOSE(CamxLogGroupChi, "GPUOpenCL ReleaseContext error -0 means success-: %d", err);
        m_context = NULL;
    }

    if (NULL != m_copyImageKernel)
    {
        m_pfnCLReleaseKernel(m_copyImageKernel);
        m_copyImageKernel = NULL;
    }

    if (NULL != m_rotateImageKernel)
    {
        m_pfnCLReleaseKernel(m_rotateImageKernel);
        m_rotateImageKernel = NULL;
    }

    if (NULL != m_flipImageKernel)
    {
        m_pfnCLReleaseKernel(m_flipImageKernel);
        m_flipImageKernel = NULL;
    }

    if (NULL != m_ds4SinglePlaneKernel)
    {
        m_pfnCLReleaseKernel(m_ds4SinglePlaneKernel);
        m_ds4SinglePlaneKernel = NULL;
    }

    if (NULL != m_boxFilterSinglePlaneKernel)
    {
        m_pfnCLReleaseKernel(m_boxFilterSinglePlaneKernel);
        m_boxFilterSinglePlaneKernel = NULL;
    }

    if (NULL != m_p010topd10Kernel)
    {
        m_pfnCLReleaseKernel(m_p010topd10Kernel);
        m_p010topd10Kernel = NULL;
    }

    if (NULL != m_p010totp10Kernel)
    {
        m_pfnCLReleaseKernel(m_p010totp10Kernel);
        m_p010totp10Kernel = NULL;
    }

    if (NULL != m_tp10totp10Kernel)
    {
        m_pfnCLReleaseKernel(m_tp10totp10Kernel);
        m_tp10totp10Kernel = NULL;
    }

    if (NULL != m_ds4WeightsImage)
    {
        m_pfnCLReleaseMemObject(m_ds4WeightsImage);
        m_ds4WeightsImage = NULL;
    }

    if (NULL != m_ds4Sampler)
    {
        m_pfnCLReleaseSampler(m_ds4Sampler);
        m_ds4Sampler = NULL;
    }

    if (NULL != m_program)
    {
        m_pfnCLReleaseProgram(m_program);
        m_program = NULL;
    }

    if (NULL != m_hOpenCLLib)
    {
        result = ChiNodeUtils::LibUnmap(m_hOpenCLLib);
        m_hOpenCLLib = 0;
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "GPUOpenCL::Uninitialize Failed to unmap lib: %d", result);
    }

    m_initStatus = CLInitInvalid;

    return result;
}

// =============================================================================================================================
// END OpenCL Stuff
// =============================================================================================================================

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads

ChiNodeInterface g_ChiNodeInterface;             ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase          = 0;   ///< Chi assigned runtime vendor tag base for the node
UINT32           g_bypassCameraIdTagBase  = 0;   ///< Chi assigned runtime vendor tag base for the node

/// @todo (CAMX-1854) Need to get the major / minor verion from Chi
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of Chi interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of Chi interface

static const CHAR   GpuNodeSectionName[]         = "com.qti.node.gpu";  ///< The section name for node
static const CHAR   GpuNodeInputSectionName[]    = "com.qti.node.gpuinput";
static const CHAR   StreamTypePresent[]          = "org.quic.camera.streamTypePresent";
static const CHAR   GPUSkipbasedonFDSkip[]       = "org.quic.camera.skipgpuprocessingbasedonfd";

static const UINT32 GpuNodeTagBase               = 0;                   ///< Tag base
static const UINT32 GpuNodeTagSupportedFeature   = GpuNodeTagBase + 0;  ///< Tag for supported features
static const UINT32 GpuNodeTagCurrentMode        = GpuNodeTagBase + 1;  ///< Tag to indicated current operation mode
static const UINT32 GpuNodeTagProcessedFrameNum  = GpuNodeTagBase + 2;  ///< Tag to show processed frame's count
static const UINT32 GpuNodeTagFrameDimension     = GpuNodeTagBase + 3;  ///< Tag to show frame dimensions

static const UINT32 GpuNodeNumTags               = 4;

// Reminder to change the constant and array entries below.

///< Supported vendor tag list, it shall align with the definition in g_VendorTagSectionGpuNode
static const UINT32 g_VendorTagList[] =
{
    GpuNodeTagSupportedFeature,
    GpuNodeTagCurrentMode,
    GpuNodeTagProcessedFrameNum,
    GpuNodeTagFrameDimension,
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionGpuNode[] =
{
    { "SupportedFeature",     TYPE_INT32, 1 },
    { "CurrentMode",          TYPE_BYTE,  1 },
    { "ProcessedFrameNumber", TYPE_INT64, 1 },
    { "FrameDimension",       TYPE_INT32, 2 },
    { "BypassCameraId",       TYPE_INT32, 1 },
};


///< This is an array for vendor tag which tells stream buffer availability
static CHIVENDORTAGDATA g_VendorTagSectionstreamTypePresent[] =
{
    { "preview", 0, 1 },
};

static CHIVENDORTAGDATA g_VendorTagSectionSkipGPUprocessingbasedonFD[] =
{
    { "SkipGPUprocessingbasedonFD", 0, sizeof(UINT32) }
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagGpuNodeSection[] =
{
    {
        GpuNodeSectionName,  0,
        sizeof(g_VendorTagSectionGpuNode) / sizeof(g_VendorTagSectionGpuNode[0]), g_VendorTagSectionGpuNode,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoGpuNode[] =
{
    {
        &g_VendorTagGpuNodeSection[0],
        sizeof(g_VendorTagGpuNodeSection) / sizeof(g_VendorTagGpuNodeSection[0])
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pCapsInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODECAPSINFO) <= pCapsInfo->size)
        {
            pCapsInfo->nodeCapsMask = ChiNodeCapsScale | ChiNodeCapsGpuMemcpy |
                                      ChiNodeCapsGPURotate | ChiNodeCapsGPUDownscale | ChiNodeCapsGPUFlip;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODECAPSINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag  Pointer to a structure that defines the run-time Chi assigned vendor tag base.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    if (NULL == pQueryVendorTag)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalide argument: pQueryVendorTag is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryVendorTag->size >= sizeof(CHIQUERYVENDORTAG))
        {
            pQueryVendorTag->pVendorTagInfo = g_VendorTagInfoGpuNode;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "pQueryVendorTag is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeCreate
///
/// @brief  GPU Node Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult   result  = CDKResultSuccess;
    ChiGPUNode* pNode   = NULL;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pTagTypeInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODECREATEINFO) > pCreateInfo->size)
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        pNode = new ChiGPUNode;
        if (NULL == pNode)
        {
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        result = pNode->Initialize(pCreateInfo);
    }

    if (CDKResultSuccess == result)
    {
        pCreateInfo->phNodeSession = reinterpret_cast<CHIHANDLE*>(pNode);
    }

    if (CDKResultSuccess != result)
    {
        if (NULL != pNode)
        {
            delete pNode;
            pNode = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeDestroy(
    CHINODEDESTROYINFO* pDestroyInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pDestroyInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pDestroyInfo is NULL");
    }
    if ((CDKResultSuccess == result) && (NULL == pDestroyInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pDestroyInfo->hNodeSession is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODEDESTROYINFO) <= pDestroyInfo->size)
        {
            ChiGPUNode* pNode = static_cast<ChiGPUNode*>(pDestroyInfo->hNodeSession);
            // Guaranteed to be not null at this point (playing safe due threating in future)
            if (NULL != pNode)
            {
                delete pNode;
                pNode = NULL;
            }

            pDestroyInfo->hNodeSession  = NULL;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEDESTROYINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeQueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pQueryBufferInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pQueryBufferInfo argument");
    }

    if ((CDKResultSuccess == result) && (NULL == pQueryBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pQueryBufferInfo->hNodeSessionn argument");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODEQUERYBUFFERINFO) <= pQueryBufferInfo->size)
        {
            ChiGPUNode* pNode   = static_cast<ChiGPUNode*>(pQueryBufferInfo->hNodeSession);
            result              = pNode->QueryBufferInfo(pQueryBufferInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEQUERYBUFFERINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeSetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pSetBufferInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pSetBufferInfo argument");
    }

    if ((CDKResultSuccess == result) && (NULL == pSetBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pSetBufferInfo->hNodeSession argument");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODESETBUFFERPROPERTIESINFO) <= pSetBufferInfo->size)
        {
            ChiGPUNode* pNode   = static_cast<ChiGPUNode*>(pSetBufferInfo->hNodeSession);
            result              = pNode->SetBufferInfo(pSetBufferInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODESETBUFFERPROPERTIESINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeProcRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pProcessRequestInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pProcessRequestInfo argument");
    }

    if ((CDKResultSuccess == result) && (NULL == pProcessRequestInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid pProcessRequestInfo->hNodeSession argument");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(CHINODEPROCESSREQUESTINFO) <= pProcessRequestInfo->size)
        {
            ChiGPUNode* pNode   = static_cast<ChiGPUNode*>(pProcessRequestInfo->hNodeSession);
            result              = pNode->ProcessRequest(pProcessRequestInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCESSREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodePostPipelineCreate
///
/// @brief  Implementation of PFNPOSTPIPELINECREATE defined in chinode.h
///
/// @param  pNodeSession node session pointer
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult GpuNodePostPipelineCreate(
    VOID* pNodeSession)
{
    if (NULL == pNodeSession)
    {
        LOG_ERROR(CamxLogGroupChi, "Null GpuNodePostPipelineCreate pNodeSession");
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID GpuNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pNodeInterface)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pNodeInterface is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (sizeof(ChiNodeInterface) <= pNodeInterface->size)
        {
            memcpy(&g_ChiNodeInterface, pNodeInterface, sizeof(ChiNodeInterface));

            // get the vendor tag base
            auto GetVendorTagBase = [] (const CHAR* pSectionName, const CHAR* pTagName)
            {
                return ChiNodeUtils::GetVendorTagBase(pSectionName, pTagName, &g_ChiNodeInterface);
            };

            g_vendorTagBase         = GetVendorTagBase(GpuNodeSectionName, NULL);
            g_bypassCameraIdTagBase = GetVendorTagBase(GpuNodeSectionName, "BypassCameraId");

        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeFlushRequest
///
/// @brief  Implementation of PFNNODEFLUSH defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeFlushRequest(
    CHINODEFLUSHREQUESTINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pInfo = %p", pInfo);
    }

    if ((CDKResultSuccess == result) && (NULL == pInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: session handle = %p", pInfo->hNodeSession);
    }

    if (CDKResultSuccess == result)
    {
        if (pInfo->size >= sizeof(CHINODEFLUSHREQUESTINFO))
        {
            ChiGPUNode* pNode = static_cast<ChiGPUNode*>(pInfo->hNodeSession);

            result = pNode->FlushRequest(pInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                "CHINODEFLUSHREQUESTINFO is smaller than expected: pInfo->size = %d, sizeof(CHINODEFLUSHREQUESTINFO) = %d",
                pInfo->size, static_cast<INT>(sizeof(CHINODEFLUSHREQUESTINFO)));

            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GpuNodeGetFlushResponse
///
/// @brief  Implementation of PFNNODEFLUSHRESPONSEINFO defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult GpuNodeGetFlushResponse(
    CHINODERESPONSEINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pInfo = %p", pInfo);
    }

    if ((CDKResultSuccess == result) && (NULL == pInfo->hChiSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: session handle = %p", pInfo->hChiSession);
    }

    if (CDKResultSuccess == result)
    {
        if (pInfo->size == sizeof(CHINODERESPONSEINFO))
        {
            ChiGPUNode* pNode = static_cast<ChiGPUNode*>(pInfo->hChiSession);

            result = pNode->ComputeResponseTime(pInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi,
                "CHINODERESPONSEINFO is smaller than expected: pInfo->size = %d, sizeof(CHINODEFLUSHREQUESTINFO) = %d",
                pInfo->size, static_cast<INT>(sizeof(CHINODEFLUSHREQUESTINFO)));

            result = CDKResultEFailed;
        }
    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeEntry
///
/// @brief  Entry point called by the Chi driver to initialize the custom node.
///
/// This function must be exported by every <library>.so in order for driver to initialize the Node. This function is called
/// during the camera server initialization, which occurs during HAL process start. In addition to communicating the necessary
/// function pointers between Chi and external nodes, this function allows a node to do any initialization work that it
/// would typically do at process init. Anything done here should not be specific to a session, and any variables stored in
/// the node must be protected against multiple sessions accessing it at the same time.
///
/// @param pNodeCallbacks  Pointer to a structure that defines callbacks that the Chi driver sends to the node.
///                        The node must fill in these function pointers.
///
/// @return VOID.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiNodeEntry(
    CHINODECALLBACKS* pNodeCallbacks)
{
    if (NULL != pNodeCallbacks)
    {
        if ((ChiNodeMajorVersion == pNodeCallbacks->majorVersion) &&
            (sizeof(CHINODECALLBACKS) <= pNodeCallbacks->size))
        {
            pNodeCallbacks->majorVersion                = ChiNodeMajorVersion;
            pNodeCallbacks->minorVersion                = ChiNodeMinorVersion;
            pNodeCallbacks->pGetCapabilities            = GpuNodeGetCaps;
            pNodeCallbacks->pQueryVendorTag             = GpuNodeQueryVendorTag;
            pNodeCallbacks->pCreate                     = GpuNodeCreate;
            pNodeCallbacks->pDestroy                    = GpuNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo            = GpuNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo              = GpuNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest             = GpuNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface    = GpuNodeSetNodeInterface;
            pNodeCallbacks->pPostPipelineCreate         = GpuNodePostPipelineCreate;
            pNodeCallbacks->pFlushRequest               = GpuNodeFlushRequest;
            pNodeCallbacks->pGetFlushResponse           = GpuNodeGetFlushResponse;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Chi API major version doesn't match (%d:%d) vs (%d:%d)",
                      pNodeCallbacks->majorVersion,
                      pNodeCallbacks->minorVersion,
                      ChiNodeMajorVersion,
                      ChiNodeMinorVersion);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid Argument: %p", pNodeCallbacks);
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::InitializeDownscaleBypassMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::InitializeDownscaleBypassMap()
{
    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
    vendorTagBase.size           = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName = "com.qti.chi.cameraconfiguration";
    vendorTagBase.pTagName       = "PhysicalCameraInputConfig";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    ChiPhysicalCameraConfig* pCameraConfig = NULL;

    VOID* pData = NULL;
    pData       = ChiNodeUtils::GetMetaData(0, vendorTagBase.vendorTagBase | UsecaseMetadataSectionMask, ChiMetadataDynamic,
                                            &g_ChiNodeInterface, m_hChiSession);

    pCameraConfig = static_cast<ChiPhysicalCameraConfig*>(pData);
    if (pCameraConfig != NULL)
    {
        LOG_INFO(CamxLogGroupCore,
                 "[%p] - InstanceId: %u ReqId: %u pConfig: %p - numConfigurations: %u",
                 this, m_nodeId, ((UINT32) 0), pCameraConfig, pCameraConfig->numConfigurations);

        UINT numDS4Mapped  = 0;
        UINT numDS16Mapped = 0;
        for (UINT i = 0; i < pCameraConfig->numConfigurations; i++)
        {
            auto*             pInputConfig      = &pCameraConfig->configuration[i];
            auto*             pNodeDescriptor   = &pInputConfig->nodeDescriptor;
            CameraBypassInfo* pCameraBypassInfo = NULL;

            if ((m_nodeId != pNodeDescriptor->nodeId) || (m_nodeInstanceId != pNodeDescriptor->nodeInstanceId))
            {
                continue; // Skip nodes that aren't this node
            }

            switch(pInputConfig->bufferDownscaleType)
            {
                case BUFFER_DS4:
                    pCameraBypassInfo = &m_portMapDS4[numDS4Mapped++];
                    break;
                case BUFFER_DS16:
                    pCameraBypassInfo = &m_portMapDS16[numDS16Mapped++];
                    break;
                default:
                    LOG_ERROR(CamxLogGroupChi, "Cannot map: %u", pInputConfig->bufferDownscaleType);
                    break;
            }

            if (NULL != pCameraBypassInfo)
            {
                pCameraBypassInfo->cameraId    = pInputConfig->physicalCameraId;
                pCameraBypassInfo->inputPortId = pNodeDescriptor->nodePortId;
            }

            LOG_VERBOSE(CamxLogGroupChi, "Physical Camera: %u -> Node: %u:%u @ InputPort: %u | Type: %u",
                        pInputConfig->physicalCameraId,
                        pNodeDescriptor->nodeId,
                        pNodeDescriptor->nodeInstanceId,
                        pNodeDescriptor->nodePortId,
                        pInputConfig->bufferDownscaleType);
        }

        if (TRUE == (numDS16Mapped == numDS4Mapped))
        {
            m_numPortMapping = numDS4Mapped;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Mismatching number of DS4(%u) <-> DS16(%u) inputs",
                      numDS4Mapped, numDS16Mapped);
        }
    }
    else
    {
        LOG_WARN(CamxLogGroupChi, "pCameraConfig: NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult            result                  = CDKResultSuccess;
    CHIVENDORTAGBASEINFO vendorTagBase           = { 0 };
    BOOL                 bIsBufferMappingEnabled = FALSE;

    /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
    m_hChiSession                   = pCreateInfo->hChiSession;
    m_nodeId                        = pCreateInfo->nodeId;
    m_nodeInstanceId                = pCreateInfo->nodeInstanceId;
    m_nodeCaps                      = pCreateInfo->nodeCaps.nodeCapsMask;
    m_nodeFlags                     = pCreateInfo->nodeFlags;
    m_processedFrame                = 0;
    m_vendorTagPreviewStreamPresent = 0;
    m_bIsSkipProcessing             = FALSE;

    if (m_nodeCaps & ChiNodeCapsGPUSkipProcessing)
    {
        LOG_INFO(CamxLogGroupChi, "Before m_nodeFlags 0x%x", m_nodeCaps);
        // As of GPU node can perform only operation at a time, so resetting the caps value so that existing
        // functioality do not break.
        m_nodeCaps                        &= ~(ChiNodeCapsGPUSkipProcessing);
        pCreateInfo->nodeCaps.nodeCapsMask = m_nodeCaps;
        m_bIsSkipProcessing                = TRUE;
        LOG_INFO(CamxLogGroupChi, "After m_nodeFlags 0x%x m_bIsSkipProcessing %d", m_nodeCaps, m_bIsSkipProcessing);
    }
    if (m_nodeCaps & ChiNodeCapsGPUEnableMapping)
    {
        LOG_INFO(CamxLogGroupChi, "Before m_nodeFlags 0x%x", m_nodeCaps);
        // As of now, GPU node can perform only one operation at a time, so resetting the caps value so that existing
        // functioality do not break.
        m_nodeCaps                        &= ~(ChiNodeCapsGPUEnableMapping);
        pCreateInfo->nodeCaps.nodeCapsMask = m_nodeCaps;
        bIsBufferMappingEnabled            = TRUE;
        LOG_INFO(CamxLogGroupChi, "After m_nodeFlags 0x%x bIsBufferMappingEnabled %d", m_nodeCaps, bIsBufferMappingEnabled);
    }

    pCreateInfo->nodeFlags.canSetInputBufferDependency = TRUE;

    LOG_INFO(CamxLogGroupChi, "bypass set %d, inplace set %d", m_nodeFlags.isBypassable, m_nodeFlags.isInplace);

    m_pGpuNodeMutex                      = CamX::Mutex::Create("CHIGPUNODE");
    m_pOpenCL                            = &m_openCL;
    m_pOpenCL->m_bIsBufferMappingEnabled = bIsBufferMappingEnabled;

    m_openCL.Initialize();

    if(ChiNodeCapsGPUDownscale == m_nodeCaps)
    {
        InitializeDownscaleBypassMap();
    }

    result = ChiNodeUtils::GetVendorTagBase(StreamTypePresent,
                                            g_VendorTagSectionstreamTypePresent[0].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        m_vendorTagPreviewStreamPresent = vendorTagBase.vendorTagBase;
        LOG_INFO(CamxLogGroupChi, "StreamTypePresent");
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get StreamTypePresent Vendor Tag Id %d", result);
    }

    result = ChiNodeUtils::GetVendorTagBase(GPUSkipbasedonFDSkip,
                                            g_VendorTagSectionSkipGPUprocessingbasedonFD[0].pVendorTagName,
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        // Save Current Process skip Tag Id
        m_vendorTagSkipGPUprocessingbasedonFD = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get FD based process skip Tag Id");
    }
    if ((NULL == m_pOpenCL) || (NULL == m_pGpuNodeMutex))
    {
        result = CDKResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::SetDependencies(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CHIDEPENDENCYINFO* pDependencyInfo     = pProcessRequestInfo->pDependency;
    pDependencyInfo->inputBufferFenceCount = 0;

    for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
    {
        if ((NULL != pProcessRequestInfo->phInputBuffer) &&
            (NULL != pProcessRequestInfo->phInputBuffer[i]) &&
            (NULL != pProcessRequestInfo->phInputBuffer[i]->pfenceHandle) &&
            (NULL != pProcessRequestInfo->phInputBuffer[i]->pIsFenceSignaled))
        {
            pDependencyInfo->pInputBufferFence[pDependencyInfo->inputBufferFenceCount] =
                pProcessRequestInfo->phInputBuffer[i]->pfenceHandle;
            pDependencyInfo->pInputBufferFenceIsSignaled[pDependencyInfo->inputBufferFenceCount] =
                pProcessRequestInfo->phInputBuffer[i]->pIsFenceSignaled;

            pDependencyInfo->inputBufferFenceCount++;
        }
    }

    pDependencyInfo->hNodeSession                      = m_hChiSession;
    pDependencyInfo->processSequenceId                 = 1;
    pDependencyInfo->hasIOBufferAvailabilityDependency = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    UINT32                     numOutputPorts         = pQueryBufferInfo->numOutputPorts;
    CDKResult                  result                 = CDKResultSuccess;
    CHINODEBUFFERREQUIREMENT** ppOutputRequirementMax = new CHINODEBUFFERREQUIREMENT *[numOutputPorts];
    if (NULL == ppOutputRequirementMax)
    {
        result = CDKResultENoMemory;
    }

    LOG_INFO(CamxLogGroupChi, "numOutputPorts = %d, numInputPorts = %d",
             pQueryBufferInfo->numOutputPorts,
             pQueryBufferInfo->numInputPorts);

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < numOutputPorts; i++)
        {
            INT                       strideAlignment    = 0;
            INT                       scanlineAlignment  = 0;

            CHINODEBUFFERREQUIREMENT* pOutputOptions     = &pQueryBufferInfo->pOutputPortQueryInfo[i].outputBufferOption;
            CHINODEBUFFERREQUIREMENT* pOutputRequirement = &pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[0];
            ChiFormat                 format             = pQueryBufferInfo->pOutputPortQueryInfo[i].outputBufferOption.format;

            ppOutputRequirementMax[i] = &pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[0];

            for (UINT32 num = 0; num < pQueryBufferInfo->pOutputPortQueryInfo[i].numConnectedInputPorts; num++)
            {
                if ((pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[num].optimalH >
                         ppOutputRequirementMax[i]->optimalH) &&
                    (pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[num].optimalW >
                         ppOutputRequirementMax[i]->optimalW))
                {
                    ppOutputRequirementMax[i] = &pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[num];
                }
            }

            pOutputRequirement = ppOutputRequirementMax[i];

            LOG_VERBOSE(CamxLogGroupChi, "output port id = %d, format = %d",
                        pQueryBufferInfo->pOutputPortQueryInfo[i].outputPortId,
                        pQueryBufferInfo->pOutputPortQueryInfo[i].outputBufferOption.format);

            // set the nodeCaps later
            switch (m_nodeCaps)
            {
                case ChiNodeCapsGpuMemcpy:
                    pOutputOptions->minW     = pOutputRequirement->minW;
                    pOutputOptions->minH     = pOutputRequirement->minH;
                    pOutputOptions->maxW     = pOutputRequirement->maxW;
                    pOutputOptions->maxH     = pOutputRequirement->maxH;
                    pOutputOptions->optimalW = pOutputRequirement->optimalW;
                    pOutputOptions->optimalH = pOutputRequirement->optimalH;

                    for (INT planeId = 0; planeId < 2; planeId++)
                    {
                        ChiNodeUtils::GetAlignment(pOutputOptions->optimalW,
                                                   pOutputOptions->optimalH,
                                                   planeId,
                                                   format,
                                                   &strideAlignment,
                                                   &scanlineAlignment);

                        pOutputOptions->planeAlignment[planeId].strideAlignment   = strideAlignment;
                        pOutputOptions->planeAlignment[planeId].scanlineAlignment = scanlineAlignment;
                    }
                    break;

                case ChiNodeCapsGPUGrayscale:
                case ChiNodeCapsGPURotate:
                    pOutputOptions->minW     = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, pOutputRequirement->minH);
                    pOutputOptions->minH     = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, pOutputRequirement->minH);
                    pOutputOptions->maxW     = ChiNodeUtils::MaxUINT32(pOutputRequirement->maxW, pOutputRequirement->maxH);
                    pOutputOptions->maxH     = ChiNodeUtils::MaxUINT32(pOutputRequirement->maxW, pOutputRequirement->maxH);
                    pOutputOptions->optimalW =
                        ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, pOutputRequirement->optimalH);
                    pOutputOptions->optimalH =
                        ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, pOutputRequirement->optimalH);

                    for (INT planeId = 0; planeId < 2; planeId++)
                    {
                        ChiNodeUtils::GetAlignment(pOutputOptions->optimalW,
                                                   pOutputOptions->optimalH,
                                                   planeId,
                                                   format,
                                                   &strideAlignment,
                                                   &scanlineAlignment);

                        pOutputOptions->planeAlignment[planeId].strideAlignment   = strideAlignment;
                        pOutputOptions->planeAlignment[planeId].scanlineAlignment = scanlineAlignment;
                    }
                    break;

                case ChiNodeCapsGPUDownscale:
                    pOutputOptions->minW     = pOutputRequirement->minW;
                    pOutputOptions->minH     = pOutputRequirement->minH;

                    //Check if only arbitrary downscaling ports are enabled.
                    //If so attempt to adjust maxW and maxH.
                    if (TRUE == IsArbitraryDownscalerOnly(pQueryBufferInfo))
                    {
                        pOutputOptions->maxW = ChiNodeUtils::MaxUINT32(maxGPUWidth,  pOutputRequirement->maxW);
                        pOutputOptions->maxH = ChiNodeUtils::MaxUINT32(maxGPUHeight, pOutputRequirement->maxH);
                    }
                    else
                    {
                        pOutputOptions->maxW     = pOutputRequirement->maxW;
                        pOutputOptions->maxH     = pOutputRequirement->maxH;
                    }

                    pOutputOptions->optimalW = pOutputRequirement->optimalW;
                    pOutputOptions->optimalH = pOutputRequirement->optimalH;

                    for (INT planeId = 0; planeId < 2; planeId++)
                    {
                        ChiNodeUtils::GetAlignment(pOutputOptions->optimalW,
                                                   pOutputOptions->optimalH,
                                                   planeId,
                                                   format,
                                                   &strideAlignment,
                                                   &scanlineAlignment);

                        pOutputOptions->planeAlignment[planeId].strideAlignment   = strideAlignment;
                        pOutputOptions->planeAlignment[planeId].scanlineAlignment = scanlineAlignment;
                    }
                    break;
                case ChiNodeCapsGPUFlip:
                    // Expected that the pipeline will properly set the resolutions,
                    // therefore keep it the same as the requirement.
                default:
                    /// @todo (CAMX-1806) Node caps not set, default case is used
                    pOutputOptions->minW     = pOutputRequirement->minW;
                    pOutputOptions->minH     = pOutputRequirement->minH;
                    pOutputOptions->maxW     = pOutputRequirement->maxW;
                    pOutputOptions->maxH     = pOutputRequirement->maxH;
                    pOutputOptions->optimalW = pOutputRequirement->optimalW;
                    pOutputOptions->optimalH = pOutputRequirement->optimalH;

                    for (INT planeId = 0; planeId < 2; planeId++)
                    {
                        ChiNodeUtils::GetAlignment(pOutputOptions->optimalW,
                                                   pOutputOptions->optimalH,
                                                   planeId,
                                                   format,
                                                   &strideAlignment,
                                                   &scanlineAlignment);

                        pOutputOptions->planeAlignment[planeId].strideAlignment   = strideAlignment;
                        pOutputOptions->planeAlignment[planeId].scanlineAlignment = scanlineAlignment;
                    }
                    break;
            }
        }

        for (UINT32 i = 0; i < pQueryBufferInfo->numInputPorts; i++)
        {
            INT  strideAlignment    = 0;
            INT  scanlineAlignment  = 0;
            // During DS, we may have inputs > outputs
            UINT outIdx             = (ChiNodeCapsGPUDownscale == m_nodeCaps) ? 0 : i;

            CHINODEBUFFERREQUIREMENT* pInputOptions      = &pQueryBufferInfo->pInputOptions[i].inputBufferOption;
            CHINODEBUFFERREQUIREMENT* pOutputRequirement =
                &pQueryBufferInfo->pOutputPortQueryInfo[outIdx].pBufferRequirement[0];
            ChiFormat                 format             = pQueryBufferInfo->pInputOptions[i].inputBufferOption.format;

            if (numOutputPorts > outIdx)
            {
                pOutputRequirement = ppOutputRequirementMax[outIdx];
            }
            else
            {
                LOG_ERROR(CamxLogGroupChi, " outIdx: %d exceed numOutputPorts: %d", outIdx, numOutputPorts);
            }

            LOG_INFO(CamxLogGroupChi, "Input port id = %d, Format = %d - outIdx: %u m_nodeCaps: %u",
                     pQueryBufferInfo->pInputOptions[i].inputPortId,
                     pQueryBufferInfo->pInputOptions[i].inputBufferOption.format,
                     outIdx, m_nodeCaps);

            pInputOptions->minW     = pOutputRequirement->minW;
            pInputOptions->minH     = pOutputRequirement->minH;
           if (ChiNodeCapsGPUDownscale == m_nodeCaps )
            {
                pInputOptions->maxW     = pOutputRequirement->maxW * 8;
                pInputOptions->maxH     = pOutputRequirement->maxH * 8;
            }
            else
            {
                pInputOptions->maxW     = pOutputRequirement->maxW;
                pInputOptions->maxH     = pOutputRequirement->maxH;
            }
            pInputOptions->optimalW = pOutputRequirement->optimalW;
            pInputOptions->optimalH = pOutputRequirement->optimalH;

            for (INT planeId = 0; planeId < 2; planeId++)
            {
                ChiNodeUtils::GetAlignment(pInputOptions->optimalW,
                                           pInputOptions->optimalH,
                                           planeId,
                                           format,
                                           &strideAlignment,
                                           &scanlineAlignment);

                pInputOptions->planeAlignment[planeId].strideAlignment   = strideAlignment;
                pInputOptions->planeAlignment[planeId].scanlineAlignment = scanlineAlignment;
            }
        }

        delete[] ppOutputRequirementMax;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::InitializeDownscaleBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::InitializeDownscaleBufferManagers(
    UINT previewWidth,
    UINT previewHeight)
{
    CHINodeBufferManagerCreateData createData;
    if ((NULL == m_hBufferManagerDS16) || (NULL == m_hBufferManagerDS4))
    {
        LOG_INFO(CamxLogGroupChi, "[E] - Preview Dimensions: (%u x %u)", previewWidth, previewHeight);

        createData                      = {0};
        createData.format               = ChiStreamFormatP010;
        createData.consumerFlags        = GrallocUsageSwReadOften;
        createData.producerFlags        = createData.consumerFlags | GrallocUsageSwWriteOften;
        createData.maxBufferCount       = 1;
        createData.immediateBufferCount = 1;
        createData.bufferHeap           = BufferHeapDefault;
    }

    auto GetBufferManagerHandle = [&] (const CHAR*          pName,
                                       UINT                 downscaleFactor,
                                       CHINODEBUFFERHANDLE& hBuffer) -> CHIBUFFERMANAGERHANDLE
    {
        createData.width  = DownscaleDivide(previewWidth, downscaleFactor);
        createData.height = DownscaleDivide(previewHeight, downscaleFactor);

        CHIBUFFERMANAGERHANDLE hBufferManager = g_ChiNodeInterface.pCreateBufferManager(pName, &createData);

        if (NULL == hBufferManager)
        {
            LOG_ERROR(CamxLogGroupChi, "Could not create: %s", pName);
        }
        else
        {
            hBuffer = g_ChiNodeInterface.pBufferManagerGetImageBuffer(hBufferManager);
            if (NULL == hBuffer)
            {
                LOG_ERROR(CamxLogGroupChi, "Could not obtain buffer from: %s", pName);
            }
        }
        return hBufferManager;
    };

    if (NULL == m_hBufferManagerDS4)
    {
        m_hBufferManagerDS4 = GetBufferManagerHandle("m_hBufferManagerDS4", 4, m_hTempBufferDS4);
    }
    if (NULL == m_hBufferManagerDS16)
    {
        m_hBufferManagerDS16 = GetBufferManagerHandle("m_hBufferManagerDS16", 16, m_hTempBufferDS16);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    m_format.width            = pSetBufferInfo->pFormat->width;
    m_format.height           = pSetBufferInfo->pFormat->height;
    m_fullOutputDimensions[0] = pSetBufferInfo->pFormat->width;
    m_fullOutputDimensions[1] = pSetBufferInfo->pFormat->height;
    LOG_INFO(CamxLogGroupChi, "OutputPortId: %u - (%u x %u)",
              pSetBufferInfo->portId,
              pSetBufferInfo->pFormat->width,
              pSetBufferInfo->pFormat->height);

    return CDKResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ProcessDownscaleRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::ProcessDownscaleRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    const UINT INVALID_INDEX = 0xFFFFFFFF;

    CDKResult                  result            = CDKResultSuccess;
    BOOL                       shouldBypass      = FALSE;
    UINT                       fullInputIndex    = 0;
    UINT32*                    pBypassCameraId   = NULL;

    UINT                       idxOutputFull     = INVALID_INDEX;
    UINT                       idxOutputDS4      = INVALID_INDEX;
    UINT                       idxOutputDS16     = INVALID_INDEX;
    UINT                       idxOutputDS       = INVALID_INDEX;
    UINT                       idxOutputDSb      = INVALID_INDEX;
    UINT                       idxOutputDS64     = INVALID_INDEX;

    CHINODEBUFFERHANDLE        hOutputDS4        = NULL;
    CHINODEBUFFERHANDLE        hOutputDS16       = NULL;
    CHINODEBUFFERHANDLE        hOutputDS64       = NULL;
    CHINODEBUFFERHANDLE        hOutputDS         = NULL;
    CHINODEBUFFERHANDLE        hOutputDSb        = NULL;
    CHINODEBUFFERHANDLE        hInputFull        = pProcessRequestInfo->phInputBuffer[fullInputIndex];
    BOOL                       isFDFrameskip     = TRUE;

    ReadSkipVendorTag(pProcessRequestInfo->frameNum, &isFDFrameskip);

    pBypassCameraId = static_cast<UINT32*>(GetMetaData(pProcessRequestInfo->frameNum, g_bypassCameraIdTagBase));

    shouldBypass = ((NULL != pBypassCameraId) && (pProcessRequestInfo->inputNum > 1)) ? TRUE : FALSE;


    // Find the output port indexes for this request
    for (UINT32 i = 0; i < pProcessRequestInfo->outputNum; i++)
    {
        CHINODEBUFFERHANDLE hOutputBuffer = pProcessRequestInfo->phOutputBuffer[i];
        switch(hOutputBuffer->portId)
        {
            case    FULL:
                idxOutputFull  = i;
                break;
            case    DS4:
                idxOutputDS4   = i;
                hOutputDS4     = hOutputBuffer;
                break;
            case    DS16:
                idxOutputDS16  = i;
                hOutputDS16    = hOutputBuffer;
                break;
            case    DS64:
                idxOutputDS64 = i;
                hOutputDS64   = hOutputBuffer;
                break;
            case    DS:
                idxOutputDS   = i;
                hOutputDS     = hOutputBuffer;
                break;
            case    DSb:
                idxOutputDSb = i;
                hOutputDSb   = hOutputBuffer;
                break;
            default:
                idxOutputDS    = i;
                break;
        }
        LOG_VERBOSE(CamxLogGroupChi, "Request: %" PRIu64 " Bypass: %u Output Port: %u Fmt: %u - (%u x %u)",
                    pProcessRequestInfo->frameNum, shouldBypass, hOutputBuffer->portId,
                    hOutputBuffer->format.format, hOutputBuffer->format.width, hOutputBuffer->format.height);
    }

    // Pointer to member function of GPU node that takes two CHINODEBUFFERHANDLE and returns CDKResult
    typedef CDKResult (ChiGPUNode::*DownscaleFunc)(CHINODEBUFFERHANDLE, CHINODEBUFFERHANDLE);

    // Closure to downscale hInput into the output buffer at idxOutput
    // - Handles P010 -> PD10 conversion for any DownscaleFunc pDownscale
    auto ApplyDownscale = [&](DownscaleFunc       pDownscale,
                              UINT                idxOutput,
                              CHINODEBUFFERHANDLE hInput,
                              CHINODEBUFFERHANDLE hTemp) -> VOID
    {
        if (INVALID_INDEX != idxOutput)
        {
            BOOL                formatError      = TRUE;
            CHINODEBUFFERHANDLE hOutput          = pProcessRequestInfo->phOutputBuffer[idxOutput];
            const BOOL&&        isSupportedInput = IsSupportedFormat(hInput->format.format) ? TRUE : FALSE;

            if (TRUE == isSupportedInput)
            {
                if ((PD10 == hOutput->format.format) && (NULL != hTemp))
                {
                    formatError = FALSE;
                    result      = (this->*pDownscale)(hTemp, hInput);
                    ConvertP010ImageToPD10(hOutput, hTemp);
                }
                else if (TRUE == IsSupportedFormat(hOutput->format.format))
                {
                    formatError = FALSE;
                    result      = (this->*pDownscale)(hOutput, hInput);
                }
            }

            if (TRUE == formatError)
            {
                LOG_ERROR(CamxLogGroupChi,
                          "Unsupported downscale format from Input (Port: %u, Format: %u) to Output (Port: %u, Format: %u)",
                          hInput->portId, hInput->format.format, hOutput->portId, hOutput->format.format);
            }
        }
    };

    // Closure to bypass idxInput to idxOutput
    // - If the node supports bypass, then do a real bypass
    // - Otherwise, copy input buffer to output buffer
    auto BypassInput = [&] (UINT idxInput, UINT idxOutput) -> VOID
    {
        if (INVALID_INDEX != idxOutput)
        {
            CHINODEBUFFERHANDLE hInput = pProcessRequestInfo->phInputBuffer[idxInput];
            CHINODEBUFFERHANDLE hOutput = pProcessRequestInfo->phOutputBuffer[idxOutput];
            if (TRUE == IsBypassableNode())
            {
                pProcessRequestInfo->pBypassData[idxOutput].isBypassNeeded = TRUE;
                pProcessRequestInfo->pBypassData[idxOutput].selectedInputPortIndex = idxInput;
            }
            else if (FALSE == m_nodeFlags.isInplace)
            {
                result = CopyImage(hOutput, hInput);
            }
        }
    };

    if (TRUE == shouldBypass)
    {
        UINT bypassCameraId = *pBypassCameraId;
        UINT ds4InputId     = 0;
        UINT ds16InputId    = 0;

        // Find the downscaling input IDs
        for (UINT j = 0; j < m_numPortMapping; j++)
        {
            if (bypassCameraId == m_portMapDS4[j].cameraId)
            {
                ds4InputId = m_portMapDS4[j].inputPortId;
            }
            if (bypassCameraId == m_portMapDS16[j].cameraId)
            {
                ds16InputId = m_portMapDS16[j].inputPortId;
            }
        }
        // Apply bypass mapping
        for (UINT idxInput = 0; idxInput < pProcessRequestInfo->inputNum; idxInput++)
        {
            CHINODEBUFFERHANDLE phInputBuffer = pProcessRequestInfo->phInputBuffer[idxInput];

            if (phInputBuffer->portId == ds4InputId)
            {
                BypassInput(idxInput, idxOutputDS4);
            }
            else if (phInputBuffer->portId == ds16InputId)
            {
                BypassInput(idxInput, idxOutputDS16);
            }
        }
        LOG_VERBOSE(CamxLogGroupChi, "Master Camera Id: %u ds4Id: %u ds16Id: %u",
                    bypassCameraId, ds4InputId, ds16InputId);
    }
    else // Non-Bypass Case: Apply downscaling
    {
        if (TRUE == IsBypassableNode()) // Clear BypassData if needed
        {
            for (UINT32 i = 0; i < pProcessRequestInfo->outputNum; i++)
            {
                pProcessRequestInfo->pBypassData[i].isBypassNeeded = FALSE;
            }
        }

        InitializeDownscaleBufferManagers(hInputFull->format.formatParams.yuvFormat[0].width,
                                          hInputFull->format.formatParams.yuvFormat[0].height);

        // Calculate DS4 Output
        // Use full-sized Input
        ApplyDownscale(&ChiGPUNode::DownscaleBy4Image, idxOutputDS4, hInputFull, m_hTempBufferDS4);

        // Calculate DS16 Output
        // If no DS4 Output was produced, then use full-sized Input
        // - Image quality suffers in this case
        if (INVALID_INDEX == idxOutputDS4)
        {
            ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDS16, hInputFull, m_hTempBufferDS16);
        }
        // Otherwise, use DS4 output to produce DS16
        else
        {
            CHINODEBUFFERHANDLE hDSResult = (PD10 == hOutputDS4->format.format) ? m_hTempBufferDS4 : hOutputDS4;
            ApplyDownscale(&ChiGPUNode::DownscaleBy4Image, idxOutputDS16, hDSResult, m_hTempBufferDS16);
        }

        if ((INVALID_INDEX != idxOutputDSb) && (INVALID_INDEX != idxOutputDS) && (FALSE == m_bIsSkipProcessing))
        {
            if ((NULL != pProcessRequestInfo->phOutputBuffer[idxOutputDS]) &&
                (NULL != pProcessRequestInfo->phOutputBuffer[idxOutputDSb]))
            {
                CHINODEBUFFERHANDLE ds_hOutput = pProcessRequestInfo->phOutputBuffer[idxOutputDS];
                CHINODEBUFFERHANDLE dsb_hOutput = pProcessRequestInfo->phOutputBuffer[idxOutputDSb];

                FLOAT   ds_scale = static_cast<FLOAT>(hInputFull->format.width) / static_cast<FLOAT>(ds_hOutput->format.width);
                FLOAT   dsb_scale = static_cast<FLOAT>(hInputFull->format.width) / static_cast<FLOAT>(dsb_hOutput->format.width);

                if (ds_scale <= dsb_scale)
                {
                    ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDS, hInputFull, NULL);
                    ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDSb, hOutputDS, NULL);
                }
                else
                {
                    ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDSb, hInputFull, NULL);
                    ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDS, hOutputDSb, NULL);
                }
            }
        }
        else
        {
            if(TRUE == m_bIsSkipProcessing)
            {
                if (FALSE == isFDFrameskip)
                {
                    ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDS, hInputFull, NULL);
                }
                else
                {
                    LOG_INFO(CamxLogGroupChi, "Skipping frame %llu from DS for FD",
                        (long long int)pProcessRequestInfo->frameNum);
                }
            }
            else
            {
                ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDS, hInputFull, NULL);
            }
            // Calculate arbitrary DS Output
            // Use full-sized Input
            ApplyDownscale(&ChiGPUNode::DownscaleImage, idxOutputDSb, hInputFull, NULL);
        }
    }

    // Always bypass full input if a full output exists
    if (INVALID_INDEX != idxOutputFull)
    {
        BypassInput(fullInputIndex, idxOutputFull);
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ReadSkipVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::ReadSkipVendorTag(
    UINT64 requestId,
    BOOL* bIsSkipForFDFromProperty)
{

    //reading vendortag published by FD to skip frame
    if (m_vendorTagSkipGPUprocessingbasedonFD != 0)
    {
        BOOL        bIsSkipProcess  = FALSE;
        VOID* pSkipData = ChiNodeUtils::GetMetaData(requestId,
                                                    m_vendorTagSkipGPUprocessingbasedonFD,
                                                    ChiMetadataDynamic,
                                                    &g_ChiNodeInterface,
                                                    m_hChiSession);
        if (pSkipData != NULL)
        {
            bIsSkipProcess = *static_cast<CHAR*>(pSkipData);
            if (TRUE == bIsSkipProcess)
            {
                LOG_INFO(CamxLogGroupChi, "This is time to skip request %llu as FD skipped",
                    (long long int)requestId);
                *bIsSkipForFDFromProperty = TRUE;
            }
            else
            {
                *bIsSkipForFDFromProperty = FALSE;
            }
        }
        else
        {
            *bIsSkipForFDFromProperty = FALSE;
            LOG_ERROR(CamxLogGroupChi,"Couldnt read vendor tag and not skipping frame for FD");
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result              = CDKResultSuccess;
    UINT16    sequenceNumber      = 0;
    BOOL      bIsProcessingNeeded = TRUE;

    sequenceNumber = pProcessRequestInfo->pDependency->processSequenceId;

    LOG_INFO(CamxLogGroupChi, "ProcessRequest Entry m_nodeCaps %u Frame Num %" PRIu64 " Seq number %d numInputs: %u",
             m_nodeCaps, pProcessRequestInfo->frameNum, sequenceNumber, pProcessRequestInfo->inputNum);

    if (m_vendorTagPreviewStreamPresent != 0)
    {
        VOID* pData                    = NULL;
        BOOL  bIsPreviewStreamPresent  = FALSE;

        pData = ChiNodeUtils::GetMetaData(pProcessRequestInfo->frameNum,
                                          m_vendorTagPreviewStreamPresent,
                                          ChiMetadataDynamic,
                                          &g_ChiNodeInterface, m_hChiSession);
        if (NULL != pData)
        {
            bIsPreviewStreamPresent = *static_cast<CHAR*>(pData);
            LOG_VERBOSE(CamxLogGroupChi, "bIsPreviewStreamPresent %d, request %llu ", bIsPreviewStreamPresent,
                (long long int)pProcessRequestInfo->frameNum);
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi, "Unable to read endorTag IsPreviewStreamPrasent request %llu",
                (long long int)pProcessRequestInfo->frameNum);
        }

        if ((m_bIsSkipProcessing == TRUE) && (FALSE == bIsPreviewStreamPresent))
        {
            LOG_INFO(CamxLogGroupChi, " skip request %llu since no preview  stream is present",
                (long long int)pProcessRequestInfo->frameNum);
            bIsProcessingNeeded = FALSE;
        }
    }

    if (m_vendorTagSkipGPUprocessingbasedonFD != 0)
    {
        if ((m_bIsSkipProcessing == TRUE) && (ChiNodeCapsGPUDownscale == m_nodeCaps))
        {
            bIsProcessingNeeded = TRUE;
            LOG_INFO(CamxLogGroupChi, " Process request %llu based on DS, FD vendor tag and skip property",
                (long long int)pProcessRequestInfo->frameNum);
        }
    }

    if ((0 == sequenceNumber) && (bIsProcessingNeeded == TRUE))
    {
        SetDependencies(pProcessRequestInfo);

        m_pGpuNodeMutex->Lock();

        switch (m_nodeCaps)
        {
            case ChiNodeCapsGPURotate:
                for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
                {
                    UpdateOrientation(pProcessRequestInfo->frameNum);

                    // Update the width/height dimensions to be passed as metadata
                    if ((Rotate90Degrees == m_currentRotation) || (Rotate270Degrees == m_currentRotation))
                    {
                        // Output is currently rotated, swap the width and height values
                        m_fullOutputDimensions[0] = pProcessRequestInfo->phInputBuffer[i]->format.height;
                        m_fullOutputDimensions[1] = pProcessRequestInfo->phInputBuffer[i]->format.width;
                    }
                    else
                    {
                        m_fullOutputDimensions[0] = pProcessRequestInfo->phInputBuffer[i]->format.width;
                        m_fullOutputDimensions[1] = pProcessRequestInfo->phInputBuffer[i]->format.height;
                    }
                }
                break;
            default:
                break;
        }

        m_processedFrame++;
        UpdateMetaData(pProcessRequestInfo->frameNum);

        CHINODEPROCESSMETADATADONEINFO metadataDoneInfo;
        metadataDoneInfo.size        = sizeof(metadataDoneInfo);
        metadataDoneInfo.hChiSession = m_hChiSession;
        metadataDoneInfo.frameNum    = pProcessRequestInfo->frameNum;
        metadataDoneInfo.result      = CDKResultSuccess;

        g_ChiNodeInterface.pProcessMetadataDone(&metadataDoneInfo);
        pProcessRequestInfo->isEarlyMetadataDone = TRUE;

        m_pGpuNodeMutex->Unlock();
    }

    if ((1 == sequenceNumber) && (bIsProcessingNeeded == TRUE))
    {
        m_pGpuNodeMutex->Lock();

        switch (m_nodeCaps)
        {
        case ChiNodeCapsGpuMemcpy:
            /// @todo (CAMX-1854) get the metadata from the camera system and only run the logic in preview/snapshot "Copy" Mode
            for (UINT32 i = 0; (i < pProcessRequestInfo->inputNum) && (CDKResultSuccess == result); i++)
            {
                if ((PD10 == pProcessRequestInfo->phOutputBuffer[i]->format.format) &&
                    (P010 == pProcessRequestInfo->phInputBuffer[i]->format.format))
                {
                    // Use memcpy capability to convert from P010 format to PD10
                    result = ConvertP010ImageToPD10(pProcessRequestInfo->phOutputBuffer[i], pProcessRequestInfo->phInputBuffer[i]);
                }
                else if ((UBWCTP10 == pProcessRequestInfo->phOutputBuffer[i]->format.format) &&
                         (P010 == pProcessRequestInfo->phInputBuffer[i]->format.format))
                {
                    // Conversion P010 to UBWCTP10
                    result = ConvertP010ImageToUBWCTP10(pProcessRequestInfo->phOutputBuffer[i],
                                                        pProcessRequestInfo->phInputBuffer[i]);
                }
                else if ((UBWCTP10 == pProcessRequestInfo->phOutputBuffer[i]->format.format) &&
                        (UBWCTP10 == pProcessRequestInfo->phInputBuffer[i]->format.format))
                {
                    // Copy UBWCTP10 to UBWCTP10
                    result = CopyUBWCTP10ImageToUBWCTP10(pProcessRequestInfo->phOutputBuffer[i],
                                                         pProcessRequestInfo->phInputBuffer[i]);
                }

                else
                {
                    result = CopyImage(pProcessRequestInfo->phOutputBuffer[i], pProcessRequestInfo->phInputBuffer[i]);
                }
            }
            break;

        case ChiNodeCapsGPURotate:
            for (UINT32 i = 0; (i < pProcessRequestInfo->inputNum) && (CDKResultSuccess == result); i++)
            {
                UpdateOrientation(pProcessRequestInfo->frameNum);
                if (TRUE == IsBypassableNode())
                {
                    if (m_currentRotation == Rotate0Degrees)
                    {
                        LOG_INFO(CamxLogGroupChi,
                            "[BYPASS] ProcessRequest GPU Rotation as rotation = %d",
                            m_currentRotation);
                        pProcessRequestInfo->pBypassData[i].isBypassNeeded         = TRUE;
                        pProcessRequestInfo->pBypassData[i].selectedInputPortIndex = i;
                    }
                    else
                    {
                        LOG_INFO(CamxLogGroupChi,
                            "[NoBYPASS] ProcessRequest GPU Rotation rotation = %d",
                            m_currentRotation);
                        pProcessRequestInfo->pBypassData[i].isBypassNeeded = FALSE;
                        result = RotateImage(pProcessRequestInfo->phOutputBuffer[i],
                                             pProcessRequestInfo->phInputBuffer[i], m_currentRotation);
                    }
                }
                else
                {
                    LOG_INFO(CamxLogGroupChi,
                        "[NoBYPASS] ProcessRequest GPU Rotation rotation = %d",
                        m_currentRotation);
                    result = RotateImage(pProcessRequestInfo->phOutputBuffer[i],
                                         pProcessRequestInfo->phInputBuffer[i], m_currentRotation);
                }
            }
            break;
        case ChiNodeCapsGPUDownscale:
            ProcessDownscaleRequest(pProcessRequestInfo);
            break;
        case ChiNodeCapsGPUFlip:
            UpdateOrientation(pProcessRequestInfo->frameNum);
            UpdateFlip(pProcessRequestInfo->frameNum);

            /// @todo (CAMX-1854) get the metadata from the camera system and only run the logic in preview/snapshot "Copy" Mode
            for (UINT32 i = 0; (i < pProcessRequestInfo->inputNum) && (CDKResultSuccess == result); i++)
            {
                if (TRUE == IsBypassableNode())
                {
                    if (m_currentFlip == FlipNone)
                    {
                        LOG_ERROR(CamxLogGroupChi,
                            "[BYPASS] ProcessRequest GPU NOFLIP  %d , rotation = %d",
                            m_currentFlip,
                            m_currentRotation);
                        pProcessRequestInfo->pBypassData[i].isBypassNeeded         = TRUE;
                        pProcessRequestInfo->pBypassData[i].selectedInputPortIndex = i;
                    }
                    else
                    {
                        LOG_ERROR(CamxLogGroupChi,
                            "[NoBYPASS] ProcessRequest GPU Flip = %d , rotation = %d",
                            m_currentFlip,
                            m_currentRotation);
                        pProcessRequestInfo->pBypassData[i].isBypassNeeded = FALSE;
                        result = FlipImage(pProcessRequestInfo->phOutputBuffer[i],
                            pProcessRequestInfo->phInputBuffer[i],
                            m_currentFlip,
                            m_currentRotation);
                    }
                }
                else
                {
                    LOG_ERROR(CamxLogGroupChi,
                        "[NoBYPASS] ProcessRequest GPU Flip = %d , rotation = %d",
                        m_currentFlip,
                        m_currentRotation);
                    result = FlipImage(pProcessRequestInfo->phOutputBuffer[i],
                        pProcessRequestInfo->phInputBuffer[i],
                        m_currentFlip,
                        m_currentRotation);
                }
            }
            break;
        }

        // metadata Done has been handled already, the wrapper does not need to handle this for us
        pProcessRequestInfo->isEarlyMetadataDone = TRUE;
        m_pGpuNodeMutex->Unlock();
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "ProcessRequest failed for request = %" PRIu64 "Seq number %d",
            pProcessRequestInfo->frameNum, sequenceNumber);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ChiGPUNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiGPUNode::ChiGPUNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_processedFrame(0)
    , m_pGpuNodeMutex(NULL)
{
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::~ChiGPUNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiGPUNode::~ChiGPUNode()
{
    m_pOpenCL = NULL;

    if (NULL != m_pGpuNodeMutex)
    {
        m_pGpuNodeMutex->Destroy();
        m_pGpuNodeMutex = NULL;
    }

    if (NULL != m_hBufferManagerDS16)
    {
        g_ChiNodeInterface.pBufferManagerReleaseReference(m_hBufferManagerDS16, m_hTempBufferDS16);
        g_ChiNodeInterface.pDestroyBufferManager(m_hBufferManagerDS16);
        m_hBufferManagerDS16 = NULL;
    }

    if (NULL != m_hBufferManagerDS4)
    {
        g_ChiNodeInterface.pBufferManagerReleaseReference(m_hBufferManagerDS4, m_hTempBufferDS4);
        g_ChiNodeInterface.pDestroyBufferManager(m_hBufferManagerDS4);
        m_hBufferManagerDS4 = NULL;
    }

    m_hChiSession = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::CopyBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::CopyBuffer(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->CopyBuffer(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::CopyImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::CopyImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->CopyImage(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::RotateImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::RotateImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput,
    RotationAngle       targetRotation)
{
    return m_pOpenCL->RotateImage(hOutput, hInput, targetRotation);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::FlipImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::FlipImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput,
    FlipDirection       targetFlip,
    RotationAngle       currentOrientation)
{
    FlipDirection direction = targetFlip;
    if ((Rotate90Degrees == currentOrientation) || (Rotate270Degrees == currentOrientation))
    {
        if (FlipLeftRight == targetFlip)
        {
            direction = FlipTopBottom;
        }
        else if (FlipTopBottom == targetFlip)
        {
            direction = FlipLeftRight;
        }
    }

    return m_pOpenCL->FlipImage(hOutput, hInput, direction);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::DownscaleIBy4Image
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::DownscaleBy4Image(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->DownscaleBy4Image(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::DownscaleImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::DownscaleImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->DownscaleImage(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ConvertP010ImageToPD10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::ConvertP010ImageToPD10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->ConvertP010ImageToPD10(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ConvertP010ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::ConvertP010ImageToUBWCTP10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->ConvertP010ImageToUBWCTP10(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::CopyUBWCTP10ImageToUBWCTP10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::CopyUBWCTP10ImageToUBWCTP10(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    return m_pOpenCL->CopyUBWCTP10ImageToUBWCTP10(hOutput, hInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::UpdateFlip
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::UpdateFlip(
    UINT64 requestId)
{
    CDK_UNUSED_PARAM(requestId);
    // Currently have it always set to only flip the image from left to right
    m_currentFlip = FlipLeftRight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::UpdateOrientation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::UpdateOrientation(
    UINT64 requestId)
{
    VOID* pData = NULL;
    INT32 orientation = 0;
    pData = ChiNodeUtils::GetMetaData(requestId,
                                      (ANDROID_JPEG_ORIENTATION | InputMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);

    if (NULL != pData)
    {
        orientation = *static_cast<INT32 *>(pData);
    }

    // Convert Android's Jpeg Orientation metadata
    switch (orientation)
    {
    case 0:
        m_currentRotation = Rotate0Degrees;
        break;
    case 90:
        m_currentRotation = Rotate90Degrees;
        break;
    case 180:
        m_currentRotation = Rotate180Degrees;
        break;
    case 270:
        m_currentRotation = Rotate270Degrees;
        break;
    default:
        m_currentRotation = Rotate0Degrees;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiGPUNode::UpdateMetaData(
    UINT64 requestId)
{
    CHIMETADATAINFO metadataInfo     = {0};
    const UINT32    tagSize          = sizeof(g_VendorTagSectionGpuNode) / sizeof(g_VendorTagSectionGpuNode[0]);
    CHITAGDATA      tagData[tagSize] = { {0} };
    UINT32          tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;

    UINT32  supportedFeature    = ChiNodeCapsGpuMemcpy;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &supportedFeature;
    tagData[index].dataSize     = g_VendorTagSectionGpuNode[index].numUnits;
    index++;

    UINT32  currentMode         = m_nodeCaps;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &currentMode;
    tagData[index].dataSize     = g_VendorTagSectionGpuNode[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_processedFrame;
    tagData[index].dataSize     = g_VendorTagSectionGpuNode[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_fullOutputDimensions[0];
    tagData[index].dataSize     = g_VendorTagSectionGpuNode[index].numUnits;

    metadataInfo.tagNum = (index + 1);
    g_ChiNodeInterface.pSetMetadata(&metadataInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::GetMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiGPUNode::GetMetaData(
    UINT64 requestId,
    UINT32 tagId)
{
    CHIMETADATAINFO metadataInfo = {0};
    const UINT32    tagSize      = 1;
    CHITAGDATA      tagData      = {0};
    UINT32          tagList      = tagId;

    tagData.requestId         = requestId;
    tagData.offset            = 0;
    tagData.negate            = FALSE;

    metadataInfo.size         = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession   = m_hChiSession;
    metadataInfo.tagNum       = tagSize;
    metadataInfo.pTagList     = &tagList;
    metadataInfo.pTagData     = &tagData;
    metadataInfo.metadataType = ChiMetadataDynamic;

    g_ChiNodeInterface.pGetMetadata(&metadataInfo);
    return tagData.pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::FlushRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::FlushRequest(
    CHINODEFLUSHREQUESTINFO* pFlushRequestInfo)
{
    // any internal resouces reserved for the request can be freed up here
    LOG_VERBOSE(CamxLogGroupChi, "Flush request Id %" PRIu64 " from node", pFlushRequestInfo->frameNum);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGPUNode::ComputeResponseTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiGPUNode::ComputeResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    pInfo->responseTimeInMillisec = DEFAULT_FLUSH_RESPONSE_TIME;

    return CDKResultSuccess;
}
