/*========================================================================*/
/**
\file npu_api.h

    Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
    All rights reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*========================================================================*/

#ifndef NPU_API_H
#define NPU_API_H

#include <stdint.h>
#include "npu_sts.h"

/*========================================================================
 Includes
 ========================================================================*/

/*========================================================================
 Defines
 ========================================================================*/

#define MAJOR_NPU_DRIVER_VER        2   // 16 bit
#define MINOR_NPU_DRIVER_VER        19  // 8  bit
#define PATCH_NPU_DRIVER_VER        0   // 8  bit

#define NPU_DRIVER_VERSION  ((MAJOR_NPU_DRIVER_VER << 16) | \
                             (MINOR_NPU_DRIVER_VER << 8)  | \
                             (PATCH_NPU_DRIVER_VER))

// for hw version returned for npu property: NPU_SYS_HW_VERSION
#define NPU_HW_VERSION_2   0x20000000
#define NPU_HW_VERSION_2_1 0x20010000
#define NPU_HW_VERSION_1_1 0x10010000
#define NPU_HW_VERSION_1_2 0x10020000


#define NPU_HW_MAJOR_VERSION_MASK   0xF0000000
#define NPU_HW_MAJOR_VERSION_SHIFT  28

#define NPU_MAX_INPUT_BUFFERS       10
#define NPU_MAX_OUTPUT_BUFFERS      4
#define NPU_MAX_INPUT_DIM           3
#define NPU_MAX_OUTPUT_DIM          NPU_MAX_INPUT_DIM

#define NPU_MAX_BUF_DIM_NUM         16
#define NPU_MAX_BUF_PLANE_NUM       4
#define NPU_MAX_LAYER_NAME_CHARS    128

#define NPU_PROP_PARAM_MAX_SIZE         8

// for returned for npu property: NPU_SYS_NNC_CAPS
#define NPU_PROP_ONLINE_COMPILER         0x00000001
#define NPU_PROP_LEGACY_COMPILER         0x00000002

// for returned for npu property: NPU_SYS_BLOB_ID
#define NPU_LEGACY_BLOB_ID         0x00000001
#define NPU_ONLINE_BLOB_ID         0x00000002

#define MAX_RESIZE_DIMS             4


#define NPU_NETWORK_PRIORITY_MASK            0x000000FF
#define NPU_NETWORK_LAYER_PREEMPT_MASK       0x00FF0000
#define NPU_NETWORK_LAYER_PREEMPT_SHIFT      16

#define NPU_NETWORK_PRIORITY_NORMAL_MIN       0      // Normal priority minimum level - 0
#define NPU_NETWORK_PRIORITY_NORMAL_MAX       0x7F   // Normal priority maximum level - 127

#define NPU_NETWORK_PRIORITY_RT_MIN           0x80   // Real time priority minimum level - 128
#define NPU_NETWORK_PRIORITY_RT_MAX           0xFF   // Real time priority maximum level - 255

/*========================================================================
 Flags
 ========================================================================*/
// Flags used in npu_load_network_param_t

/* Load flags for V1/V2 params */
#define NPU_LOAD_FLAGS_V1        0x00100000
#define NPU_LOAD_FLAGS_V2        0x00200000

#define NPU_LOAD_FLAGS_DLBC_ENABLE  0x00000010
#define NPU_LOAD_FLAGS_DLBC_DISABLE 0x00000020

// Execute Flags used in npu_execute_network_param_t

/* The following flags are exclusive */
/* Basic Flags  */
#define NPU_EXE_FLAGS_STATS_BASIC 0x000000001

/* Per Layer Stats
 * Provides basic layer stats plus the per layer stats
*/
#define NPU_EXE_FLAGS_STATS_LAYER 0x000000002

// Alloc flags used in npu_alloc_param_v2_t
/* A pre-filled descriptor is provided
*/
#define NPU_ALLOC_FLAGS_PRE_FILLED_DESCRIPTOR 0x00000001

#define NPU_MAX_FW_CAPS 8


/*========================================================================
 Data Types
 ========================================================================*/
typedef void* npu_handle_t;
typedef uint32_t npu_network_handle_t;
typedef uint32_t npu_buffer_handle_t;
typedef void(*func_cb) (npu_network_handle_t hNetwork, npu_sts_t npuStatus, void *pUserData);

typedef struct _npu_info_t
{
    uint32_t npuVersion;
    uint32_t npuFwVersion;
} npu_info_t;

typedef enum _quantize_type_t
{
    NPU_QUANTIZE_TYPE_NONE  = 0,
    NPU_QUANTIZE_TYPE_TF8   = 1,
    NPU_QUANTIZE_TYPE_QMN8  = 2,
    NPU_QUANTIZE_TYPE_QMN16 = 3,
    NPU_QUANTIZE_TYPE_FLOAT = 4,
    NPU_QUANTIZE_TYPE_TF32  = 5,
    NPU_QUANTIZE_TYPE_TF16  = 6,
    NPU_QUANTIZE_TYPE_NUM,
    NPU_QUANTIZE_TYPE_FORCE_32BIT = 0x7FFFFFFF
} quantize_type_t;

typedef enum _npu_pixel_format_t
{
    // Linear data, e.g. bias and weight
    NPU_PIXEL_FORMAT_LINEAR = 0,

    /// The RGB format consists of 3 bytes per pixel: one byte for
    /// Red, one for Green and one for Blue. The byte ordering is
    /// endian independent and is always RGB byte order.
    NPU_PIXEL_FORMAT_RGB    = 1,

    /// The ARGB32 format consists of 4 bytes per pixel: one byte for
    /// Red, one for Green, one for Blue and one for the alpha channel.
    /// The alpha channel is ignored.
    NPU_PIXEL_FORMAT_ARGB32 = 2,

    /// The RGBA format consists of 4 bytes per pixel: one byte for
    /// Red, one for Green, one for Blue and one for the alpha channel.
    /// The alpha channel is ignored.
    NPU_PIXEL_FORMAT_RGBA   = 3,

    /// NV21 is the Android version of YUV. The Chrominance is down
    /// sampled and has a sub sampling ratio of 4:2:0. Note that this
    /// image format has 3 channels, but the U and V channels
    /// are subsampled. For every four Y pixels there is one U and one V pixel.
    NPU_PIXEL_FORMAT_NV21    = 4,

    /// The BGR format consists of 3 bytes per pixel: one byte for Blue,
    /// one for Green and one for Red.
    NPU_PIXEL_FORMAT_BGR     = 5,

    // NPU native
    NPU_PIXEL_FORMAT_B_G_R_PLANAR = 6,

    // NPU network specific format
    // used for split networks or intermediate data mainly
    NPU_PIXEL_FORMAT_NETWORK  = 7,

    NPU_PIXEL_FORMAT_FORCE_32BIT = 0x7FFFFFFF,
} npu_pixel_format_t;

typedef struct _quantize_cfg_t
{
    quantize_type_t quanType;
    float           min;
    float           max;
    float           offset;
    float           delta;
    uint32_t        fracBitNum;
    uint32_t        intBitNum;
    uint32_t        isSigned;
} quantize_cfg_t;

typedef enum _npu_dim_type_t
{
    NPU_DIM_X     = 0,
    NPU_DIM_Y     = 1,
    NPU_DIM_Z     = 2,      //Ni
    NPU_DIM_Batch = 3,
    NPU_DIM_Max = 0x7FFFFFFF
} npu_dim_type_t;

typedef struct _npu_dim_desc_t
{
    npu_dim_type_t type;
    int32_t length;         //total size of dimension (bytes for 1st dimension, lines for second and planes for third, etc)
    int32_t offset;         //offset from start of dimension
    int32_t elementSize;    //size of each element( in either bytes, lines, planes, etc)
    int32_t numElements;    //number of elements in this dimension - remainder is padding
    int32_t alignment;      //in - prefered by caller; 0 for don't care, out - alignment required by npu hardware
    int32_t blockSize;      //needed for 4d blocks. tells size of each block in Z. Others set to 1
    int32_t padding;        //padding in this dimension (bytes for 1st dimension, lines for second and planes for third, etc)
    int32_t stride;         //stride in bytes
    int32_t flags;          //for future use. ie. potential change direction of blocksize.
} npu_dim_desc_t;

typedef struct _npu_plane_desc_t
{
    int32_t offset; //offset from buffer start point
    int32_t stride;
} npu_plane_desc_t;

/*
    Examples:
    1. weights / biases
        pixelFmt = NPU_PIXEL_FORMAT_LINEAR;
        dimNum = 1;
        dimSizes[0] = length;
        planeNum = 1;
        planeDescs[0].offset = 0;
    2. B_G_R planar 2d video input
        pixelFmt = NPU_PIXEL_FORMAT_B_G_R_PLANAR;
        dimNum = 2;
        dimSizes[0] = width;
        dimSizes[1] = height;
        planeNum = 3;
        planeDescs[0].offset = 0; //plane for B
        planeDescs[0].stride = stride;
        planeDescs[1].offset = offset1; // plane for G, stride x height + padding
        planeDescs[1].stride = stride;
        planeDescs[2].offset = offset2; // plane for R, offset1 + (stride x height + padding)
        planeDescs[2].stride = stride;
    3. Binary data
        all are 0;
*/
typedef struct _npu_buffer_desc_t
{
    // dimension number, it usually is 2 for video, 1 for weights or biases.
    int32_t dimNum;
    // data length for each dimension
    int32_t dimSizes[NPU_MAX_BUF_DIM_NUM];

    // plane number, defined by pixel format, e.g. B_G_R_PLANAR has 3 planes, BGR has only 1 plane.
    int32_t planeNum;
    // In normal case, planeDescs[0].offset is 0, planeDescs[planeNum - 1].offset can be ignored.
    npu_plane_desc_t planeDescs[NPU_MAX_BUF_PLANE_NUM];

    npu_pixel_format_t pixelFmt;
    quantize_cfg_t quanCfg;
} npu_buffer_desc_t;

typedef struct _npu_buffer_t
{
    // npu buffer desriptor
    npu_buffer_desc_t bufDesc;
    int32_t bufSize;
    /* host accessbile data pointer */
    void *pData;
    /* npu buffer handle */
    npu_buffer_handle_t hBuf;
    /* ion fd */
    int32_t ionFd;
} npu_buffer_t;

#define NPU_INPUT_BUF_ID_DEFAULT 0x80000000
#define NPU_OUTPUT_BUF_ID_DEFAULT 0x40000000

typedef struct _npu_buffer_desc_v2_t
{
    /* dimension number, it usually is 2 for video, 1 for weights or biases. */
    int32_t dimNum;

    /* describe each dimension */
    npu_dim_desc_t dimDescs[NPU_MAX_BUF_DIM_NUM];

    npu_pixel_format_t pixelFmt;
    quantize_cfg_t quanCfg;

    /* Unique identified for this buffer within the context of the network */
    uint32_t bufferId;

} npu_buffer_desc_v2_t;

typedef struct _npu_memory_t
{
    int32_t bufSize;

    /* host accessbile data pointer */
    /* single buffer for all dimensions */
    union
    {
        void *pData;
        uint64_t hostVirtualAddress;
    } address;

    uint64_t dspVirtualAddress;

    /* npu buffer handle */
    npu_buffer_handle_t hBuf;
    /* ion fd */
    int32_t ionFd;
    /* flags */
    uint32_t flags;
} npu_memory_t;

typedef struct _npu_buffer_v2_t
{
    /* npu alloc structure */
    npu_memory_t memory;

    /* npu buffer descriptor */
    npu_buffer_desc_v2_t bufDesc;

    /* flags */
    uint32_t flags;
} npu_buffer_v2_t;

typedef enum _npu_layer_type_t
{
    NPU_LAYER_NULL          = 0,
    NPU_LAYER_INPUT         = 1,
    NPU_LAYER_ACTIVATION    = 2,
    NPU_LAYER_BATCH_NORM    = 3,
    NPU_LAYER_CONCAT        = 4,
    NPU_LAYER_CONVOLUTION   = 5,
    NPU_LAYER_DECONVOLUTION = 6,
    NPU_LAYER_DROPOUT       = 7,
    NPU_LAYER_FC            = 8,
    NPU_LAYER_LRN           = 9,
    NPU_LAYER_POOLING       = 10,
    NPU_LAYER_PRELU         = 11,
    NPU_LAYER_SOFTMAX       = 12,
    NPU_LAYER_CROP          = 13,
    NPU_LAYER_ELEMENTWISEOP = 14,
    NPU_LAYER_SUBTRACTMEAN  = 15,
    NPU_LAYER_RESIZE        = 16,
    NPU_LAYER_SLICE         = 17,
    NPU_LAYER_CONST         = 18,
    NPU_LAYER_MAX,
    NPU_LAYER_FORCE_32BIT   = 0x7FFFFFFF  // Ensure this enum takes 32 bits.
} npu_layer_type_t;

typedef enum _npu_perf_mode_t
{
    NPU_MODE_DEFAULT = 0,
    NPU_MODE_1       = 1,
    NPU_MODE_2       = 2,
    NPU_MODE_3       = 3,
    NPU_MODE_4       = 4,
    NPU_MODE_5       = 5,
    NPU_MODE_6       = 6,
    NPU_MODE_MAX,
    NPU_MODE_FORCE_32BIT = 0x7FFFFFFF,
} npu_perf_mode_t;

typedef enum _npu_dcvs_mode_t
{
    NPU_DCVS_MODE_OFF = 0,
    NPU_DCVS_MODE_1   = 1,
    NPU_DCVS_MODE_2   = 2,
    NPU_DCVS_MODE_3   = 3,
    NPU_DCVS_MODE_4   = 4,
    NPU_DCVS_MODE_MAX,
    NPU_DCVS_MODE_FORCE_32BIT = 0x7FFFFFFF,
} npu_dcvs_mode_t;

typedef enum _npu_property_mode_t
{
    NPU_SYS_FW_STATE =        0,
    NPU_SYS_PERF_MODE =       1,
    NPU_NET_PERF_MODE =       2,
    NPU_SYS_DCVS_MODE =       3,
    NPU_NET_DCVS_MODE =       4,
    NPU_SYS_CLK_GATING_MODE = 5,
    NPU_SYS_PERF_MODE_MAX =   6,
    NPU_SYS_DCVS_MODE_MAX =   7,
    NPU_SYS_HW_VERSION =      8,
    NPU_SYS_FW_VERSION =      9,
    NPU_SYS_DRV_VERSION =     10,
    NPU_NET_BLOB_ID =         11, // Deprecated - currently has the same NPU_SYS_BLOB_ID value for backward compatibility
    NPU_SYS_BLOB_ID =         11,
    NPU_SYS_NNC_CAPS =        12,
    NPU_SYS_NNC_VERSION =     13,
    NPU_SYS_FW_CAPS =         14, // FW capabilites. Use npu_sys_prop_fwcaps_t
    NPU_SYS_DRV_CAPS =        15, // Driver capabilites Use npu_sys_prop_t
    NPU_SYS_PROP_NUM,
    NPU_PROP_MODE_FORCE_32BIT = 0x7FFFFFFF,  // Ensure this enum takes 32 bits.
} npu_property_mode_t;

// Property structures
typedef struct _npu_sys_prop_t
{
    uint32_t param;
} npu_sys_prop_t;

typedef struct _npu_net_prop_t
{
    npu_network_handle_t hNetwork;
    uint32_t param;
} npu_net_prop_t;

// use for prop ID: NPU_SYS_FW_CAPS
typedef struct _npu_sys_prop_fwcaps_t
{
    uint32_t caps[NPU_MAX_FW_CAPS];     // bit encoded caps
    uint32_t flags;                     // reserved for future use
} npu_sys_prop_fwcaps_t;


// Convolution
typedef struct _conv_layer_parms_t
{
    int32_t                     numFilters;
    int32_t                     sizeX;
    int32_t                     sizeY;
    int32_t                     paddingX;
    int32_t                     paddingY;
    int32_t                     strideX;
    int32_t                     strideY;
    int32_t                     groups;
    npu_buffer_t                weights;
    npu_buffer_t                biases;
} conv_layer_parms_t;

typedef struct _input_layer_parms_t
{
    npu_buffer_desc_t  inputDesc;
    npu_buffer_desc_t  outputDesc;
} input_layer_parms_t;

// Crop layer
typedef struct _crop_layer_parms_t {
    int32_t      numOffsets;
    int32_t      numInputStrides;
    int32_t      numOutputStrides;
    uint32_t     offsets[NPU_MAX_INPUT_DIM];
    uint32_t     inputStrides[NPU_MAX_INPUT_DIM];
    uint32_t     outputStrides[NPU_MAX_OUTPUT_DIM];
} crop_layer_parms_t;

typedef struct _de_conv_layer_parms_t {
    int32_t                     numFilters;
    int32_t                     sizeX;
    int32_t                     sizeY;
    int32_t                     paddingX;
    int32_t                     paddingY;
    int32_t                     strideX;
    int32_t                     strideY;
    int32_t                     groups;
    npu_buffer_t                weights;
    npu_buffer_t                biases;
} de_conv_layer_parms_t;

// Const layer
typedef struct _const_layer_parms_t {
    npu_buffer_t                data;
} const_layer_parms_t;

typedef struct _softmax_layer_parms_t {
    int32_t depth;
    int32_t size;
} softmax_layer_parms_t;

typedef struct _dropout_layer_parms_t {
    float keep;
} dropout_layer_parms_t;

typedef struct _prelu_layer_parms_t {
    npu_buffer_t biases;
} prelu_layer_parms_t;

// Lrn
// Data fields from NormParms class.
typedef struct _lrn_layer_parms_t {
    int32_t   size;
    float     k;
    float     alpha;
    float     beta;
} lrn_layer_parms_t;

// BatchNorm
typedef struct _batch_norm_layer_parms_t {
    npu_buffer_t        weights;
    npu_buffer_t        biases;
} batch_norm_layer_parms_t;

// Pooling
typedef enum _pooling_type_t {
    POOLING_TYPE_MAX = 0, // Used for creating the max pooling layer
    POOLING_TYPE_AVG = 1, // Used for creating the average pooling layer
    POOLING_TYPE_MAX_REMEMBER_POSITIONS = 2
} pooling_type_t;

// From PoolingParms class
typedef struct _pooling_layer_parms_t {
    /// @brief width & height of pooling area
    int32_t sizeX;
    int32_t sizeY;
    /// @brief step in the x and y direction for each pool
    int32_t strideX;
    int32_t strideY;
    /// @brief starting x position for each row. value < 0 starts to the left of
    ///  the input.
    int32_t startX;
    /// @brief starting y position for each column. value < 0 starts above
    //   the input
    int32_t startY;
    /// @brief Type of pooling layer to create
    pooling_type_t poolingType;
    /// @brief If true, fixes the filter size to m_Size even at the edges where not all input pixels are available.
    uint32_t poolRegionIncludePadding;
} pooling_layer_parms_t;

typedef struct _concat_layer_parms_t {
    uint32_t dummy;
} concat_layer_parms_t;

typedef struct _slice_layer_parms_t {
    int32_t     axis;
    int32_t     dimension;
    uint32_t    numOfSlices;
    uint32_t    slicePoints[NPU_MAX_OUTPUT_BUFFERS - 1];
} slice_layer_parms_t;

typedef struct _fc_layer_parms_t {
    npu_buffer_t        weights;
    npu_buffer_t        biases;
} fc_layer_parms_t;

typedef enum _activation_function_t {
    ACTIVATE_FUNC_NONE = 0,
    ACTIVATE_FUNC_RELU = 1,
    ACTIVATE_FUNC_TANH = 2,
    ACTIVATE_FUNC_LOGISTIC = 3
} activation_function_t;

typedef struct _activation_layer_parms_t {
    activation_function_t activation;
    float a;
    float b;
} activation_layer_parms_t;

// Resize
typedef struct _resize_layer_parms_t {
    float         scaleH;
    float         scaleW;
    float         padValue;
    uint32_t      maintainAspectRatio;
} resize_layer_parms_t;

typedef enum _elt_wise_op_type_t {
    ELT_WISE_SUM       = 0,
    ELT_WISE_PRODUCT   = 1,
    ELT_WISE_MAX       = 2,
    ELT_WISE_NONE      = 3
} elt_wise_op_type_t;

typedef struct _elt_wise_op_layer_params_t {
    elt_wise_op_type_t    op;
    uint32_t              numCoeffs;
    quantize_cfg_t        coeff;
} elt_wise_op_layer_params_t;

typedef struct _subtract_mean_layer_parms_t {
    uint32_t numMeans;
    quantize_cfg_t quantizeCfg;
} subtract_mean_layer_parms_t;

typedef struct _static_min_max_t
{
    uint32_t      valid;
    float         max;
    float         min;
} static_min_max_t;

typedef struct _npu_resize_param_t
{
    // input dimensions
    uint32_t inputDims[MAX_RESIZE_DIMS];

    // original blob information (this is the non-serialized blob)
    void     *pBlobData;          // original blob data pointer
    uint32_t  blobSize;           // original blob size

    // resized blob allocated by npu_resize_network_init and released by npu_resize_network_deinit
    void     *pResizedBlobData;  // pointer to new memory containing resized blob.
                                 // Note: the memory for this data is allocated within the driver
                                 // and needs to be freed by caller. It can be freed once npu_load_network()
                                 // is completed
    uint32_t resizedBlobSize;    // new resized blob size
    uint32_t flags;              // flags for internal functionality
} npu_resize_param_t;

typedef struct _npu_layer_common_t
{
    /// @@brief For int m_Id in FxpLayer
    int32_t id;
    /// @brief for input buffer IDs
    uint32_t numInputBuffers;
    uint64_t inputBufferIds[NPU_MAX_INPUT_BUFFERS];
    /// @brief for output buffer IDs
    uint64_t outputBufferIds[NPU_MAX_OUTPUT_BUFFERS];
    uint32_t numOutputBuffers;
    /// @brief for output dims
    // TODO: support multiple sets of output dims
    uint32_t num_output_dims[NPU_MAX_OUTPUT_BUFFERS];
    uint32_t output_dims[NPU_MAX_OUTPUT_BUFFERS][NPU_MAX_OUTPUT_DIM];
    // whether float output is required: 0: false, non-zero: true
    uint32_t outputFloats;
    static_min_max_t staticMinMax;
} npu_layer_common_t;

// Header for each layer
typedef struct _npu_layer_descriptor_t
{
    uint32_t               length; // Length, in bytes of a particular descriptor.
    char                   layerName[NPU_MAX_LAYER_NAME_CHARS];
    npu_layer_type_t       layerType;
    npu_layer_common_t     common;
    union
    {
        input_layer_parms_t         inputParms;
        conv_layer_parms_t          convParms;
        de_conv_layer_parms_t       deconvParms;
        prelu_layer_parms_t         preluParms;
        lrn_layer_parms_t           lrnParms;
        pooling_layer_parms_t       poolingParms;
        batch_norm_layer_parms_t    bnParms;
        concat_layer_parms_t        concatParms;
        fc_layer_parms_t            fcParms;
        activation_layer_parms_t    actParms;
        dropout_layer_parms_t       dropoutParms;
        softmax_layer_parms_t       softmaxParms;
        crop_layer_parms_t          cropParms;
        elt_wise_op_layer_params_t  eltWiseOpParms;
        subtract_mean_layer_parms_t subtractMeanParms;
        resize_layer_parms_t        resizeParms;
        slice_layer_parms_t         sliceLayerOpParms;
        const_layer_parms_t         constParms;
    } params;
} npu_layer_descriptor_t;

typedef struct _npu_load_network_param_t
{
    uint32_t networkDataSize; //current test input: to be removed
    void *pNetworkData; //current test input: to be removed

    uint32_t blobDataSize;
    void *pBlobData;

    uint32_t layerNum;
    npu_layer_descriptor_t *pLayerDesc;

    npu_perf_mode_t perfMode;

    // Priority field indicates the network's prioritization in relation to other networks. Its a bit encoded field described as follows:
    // Bits 0-7 indicate priority.
    //    Setting in the range NPU_NETWORK_PRIORITY_NORMAL_MIN - NPU_NETWORK_PRIORITY_NORMAL_MAX (0-127) indicates normal service priority. These
    //    networks will be preempted when a network with higher priority (128-255) needs to execute.
    //    Setting in the range of NPU_NETWORK_PRIORITY_RT_MIN - NPU_NETWORK_PRIORITY_RT_MAX (128-255) indicate real-time service priority. The networks will be prioritized and executed as soon as possible
    //    and will execute before networks with normal priority.
    //
    //    Real-time networks will only preempt normal priority networks, not other real-time ones.
    //    All priorities within each range 0-127 and 128-255 are currently treated equally.
    // Bits 8-15: Reserved for future use
    // Bits 16-23: indicate at which layer number multiple the network is preemptable at.
    //    Setting to 0 (default) will indicate the network is preemptable on a per layer basis. (Only applicable to networks with priority 0-127 currently)
    uint32_t priority;
    uint32_t flags;
} npu_load_network_param_t;

typedef struct _npu_buffer_patch_info_t
{
    uint32_t bufferId;
    npu_memory_t mem;
} npu_buffer_patch_info_t;

typedef struct _npu_load_network_param_v2_t
{
    // contains transformed w/b
    npu_buffer_patch_info_t networkBuffer;

    // intermediate buffers
    npu_buffer_patch_info_t intermediateBuffer;

    // aco
    npu_memory_t acoBuffer;

    // number input buffer descriptors
    uint32_t numInputBuff;

    npu_buffer_v2_t *pInBuffDesc;

    // number output buffer descriptors
    uint32_t numOutputBuff;

    npu_buffer_v2_t *pOutBuffDesc;

    // need for stats buffer allocation + handling.
    uint32_t layerNum;

    // performance requirement
    npu_perf_mode_t perfMode;
    uint32_t priority;
    uint32_t flags;
} npu_load_network_param_v2_t;

typedef struct _npu_layer_profile_t
{
    uint64_t start_usec;
    uint64_t stop_usec;
} npu_layer_profile_t;

typedef struct _npu_execute_network_param_t
{
    npu_network_handle_t hNetwork;
    int32_t layerNum;
    npu_layer_profile_t *pLayerProfile;

    int32_t inputBufNum;
    npu_buffer_t *pInputBuf;
    int32_t outputBufNum;
    npu_buffer_t *pOutputBuf;

    func_cb pfnCallback;
    void *pCallbackUserData;

    uint32_t flags;
} npu_execute_network_param_t;

typedef struct _npu_alloc_param_t
{
    npu_network_handle_t hNetwork;

    npu_buffer_t *pBuffer;
    uint32_t bufferId;

    uint32_t flags;
} npu_alloc_param_t;

typedef struct _npu_execute_network_param_v2_t
{
    npu_network_handle_t hNetwork;
    int32_t layerNum;
    npu_layer_profile_t *pLayerProfile;

    int32_t inputBufNum;
    npu_buffer_v2_t *pInputBuf;
    int32_t outputBufNum;
    npu_buffer_v2_t *pOutputBuf;

    func_cb pfnCallback;
    void *pCallbackUserData;

    uint32_t flags;
} npu_execute_network_param_v2_t;

typedef struct _npu_alloc_param_v2_t
{
    npu_network_handle_t hNetwork;
    npu_buffer_v2_t *pBuffer;
    uint32_t flags;                    // If flags set NPU_ALLOC_FLAGS_PRE_FILLED_DESCRIPTOR,
                                       // this uses the pre-filled descriptor for size
} npu_alloc_param_v2_t;

typedef struct _npu_translate_stats_t
{
    // network handle
    npu_network_handle_t hNetwork;

    // number of layer stats expected to filled
    // if execute was called with only basic stats (ie end to end time) set to 1.
    // if execute was called with  layer stats (basic + layer stats) set to layer num (unsquahed layers) + 1
    uint32_t numLayer;

    // inout paramater - pointer to layer info array to be filled in
    // Caller is responsible for allocating layerNum (unsquashed layers) + 1 x npu_layer_profile_t bytes of data
    // (*** See above for note on setting layerNum)

    // Returned Data:
    // Index 0  contains the end to end execution time
    // Indexes 1 .. layer num contain the induvidual layer execution times.
    npu_layer_profile_t *pDstLayerProfile;

    // in param - pointer to source stats buffer returned from the execute_network function
    // Layer times will be translated from squashed NPU layers to caller original unsquashed layers
    npu_layer_profile_t *pSrcLayerProfile;
    // flags
    // Set to same stats flags as the execute call
    uint32_t flags;
} npu_translate_stats_t;

/* Logging Masks */
#define NPU_LOG_MASK_FATAL   (1 << 0)
#define NPU_LOG_MASK_ERROR   (1 << 1)
#define NPU_LOG_MASK_WARNING (1 << 2)
#define NPU_LOG_MASK_INFO    (1 << 3)
#define NPU_LOG_MASK_DEBUG   (1 << 4)
#define NPU_LOG_MASK_TRACE   (1 << 5)

/*========================================================================
 Function Prototypes
 ========================================================================*/

 /*!
 \fn npu_sts_t npu_read_info
 \brief Retrieve npu infomation
 \param[out] pInfo               - npu version info
 \retval 0     Successful operation.
 \retval non-0 Unsuccessful.
 */
npu_sts_t npu_read_info(npu_info_t *pInfo);

/*!
\fn npu_sts_t npu_init
\brief  Initiate NPU boot up sequence.
\param[in] flags
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_init(uint32_t flags);

/*!
\fn npu_sts_t npu_deinit
\brief Execute NPU termination sequence
\param[in] flags
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_deinit(uint32_t flags);

/*!
\fn npu_sts_t npu_set_property
\brief Set certain property
\param[in] propID
           propSize
           pProp
           flags
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_set_property (uint32_t propId, uint32_t propSize, uint32_t *pProp, uint32_t flags);

/*!
\fn npu_sts_t npu_get_property
\brief Get certain property
\param[in] propID
           propSize
           pProp
           flags
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_get_property (uint32_t propId, uint32_t propSize, uint32_t *pProp, uint32_t flags);

/*!
\fn npu_sts_t npu_load_network
\brief Load a given network.  Return a network handle on success
\param[in] pLoadNetworkParam - Pointer to the network loading parameters
\param[out] pNetworkHandle   - Pointer to network handle to be returned
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_load_network(npu_load_network_param_t *pLoadNetworkParam, npu_network_handle_t *pNetworkHandle);


/*!
\fn npu_sts_t npu_load_network_v2
\brief Load a given network.  Return a network handle on success
\param[in] pLoadNetworkParam - Pointer to the network loading parameters (npu_load_network_param_v2_t)
\param[out] pNetworkHandle   - Pointer to network handle to be returned
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_load_network_v2(npu_load_network_param_v2_t *pLoadNetworkParam, npu_network_handle_t *pNetworkHandle);

/*!
\fn npu_sts_t npu_unload_network
\brief Unload the specified network
\param[in] hNetwork         - Handle to the network to be unloaded
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_unload_network(npu_network_handle_t hNetwork);

/*!
\fn npu_sts_t npu_execute_network
\brief Execute network(forward propogation)
\param[in] *pExecuteParam   - Pointer to the network execution parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_execute_network(npu_execute_network_param_t *pExecuteNetworkParam);

/*!
\fn npu_sts_t npu_alloc_buffer
\brief allocate buffer for npu access
\param[in] *pMemoryAllocParam   - Pointer to the allocation parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_alloc_buffer(npu_alloc_param_t *pMemoryAllocParam);

/*!
\fn npu_sts_t npu_free_buffer
\brief free buffer previously allocated for npu access
\param[in] *pMemoryFreeParam   - Pointer to the free parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_free_buffer(npu_alloc_param_t *pMemoryFreeParam);


/*!
\fn npu_sts_t npu_execute_network
\brief Execute network(forward propogation)
\param[in] *pExecuteParam   - Pointer to the network execution parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_execute_network_v2(npu_execute_network_param_v2_t *pExecuteNetworkParam);

/*!
\fn npu_sts_t npu_alloc_buffer
\brief allocate buffer for npu access
\param[in] *pMemoryAllocParam   - Pointer to the allocation parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_alloc_buffer_v2(npu_alloc_param_v2_t *pMemoryAllocParam);

/*!
\fn npu_sts_t npu_free_buffer
\brief free buffer previously allocated for npu access
\param[in] *pMemoryFreeParam   - Pointer to the free parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_free_buffer_v2(npu_alloc_param_v2_t *pMemoryFreeParam);


/*!
\fn npu_sts_t npu_set_log_mask
\brief Sets the logging level
\param[in] mask - Logging mask
\param[in] flags -
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_set_log_mask(uint32_t mask, uint32_t flags);

/*!
\fn npu_sts_t npu_get_log_mask
\brief Queries the logging level
\retval npu_log_level_t - Current logging mask
*/
uint32_t npu_get_log_mask();

/*!
\fn npu_sts_t npu_translate_stats
\brief Converts stats data from npu format to caller format
\param[inout] pStatsInfo
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/

npu_sts_t npu_translate_stats(npu_translate_stats_t *pStatsInfo);

/*!
\fn npu_sts_t npu_get_num_buffers
\brief  Return total number of external buffers
\param[in] hNetwork - the network handle
\param[in] *pNumBuffersInput - output uint32_t of number of external input buffers, if pointer is NULL, then will not fill out
\param[in] *pNumBuffersOutput - output uint32_t of number of external output buffers, if pointer is NULL, then will not fill out
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_get_num_buffers(npu_network_handle_t hNetwork, uint32_t *pNumBuffersInput, uint32_t *pNumBuffersOutput);

/*!
\fn npu_sts_t npu_get_buffer_ids
\brief  Return ID numbers of external buffers
\ param[in] *pInputBufferIDs - output uint32_t of number of external input buffers, if pointer is NULL, then will not fill out
\ param[in] numOfInputIDs - this is to help against a size check, it will specify how many ids can be contained by pInputBufferIDs
\ param[in] *pOutputBufferIDs - output uint32_t of number of external output buffers, if pointer is NULL, then will not fill out
\ param[in] numOfOutputIDs - this is to help against a size check, it will specify how many ids can be contained by pOutputBufferIDs
\   Other notes: in the case of v1 meta data return will be:
\   NPU_INPUT_BUF_ID_DEFAULT 0x80000000
\   NPU_OUTPUT_BUF_ID_DEFAULT 0x40000000
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
//npu_sts_t get_buffer_ids(npu_network_handle_t hNetwork, uint32_t *pOutIDs, uint32_t numOfIDs);
npu_sts_t npu_get_buffer_ids(npu_network_handle_t hNetwork,
                             uint32_t *pInputBufferIDs,
                             uint32_t numOfInputIDs,
                             uint32_t *pOutputBufferIDs,
                             uint32_t numOfOutputIDs);

/*!
\fn npu_sts_t npu_get_buffer_desc
\brief  Get V2 buffer descriptor
\ param[in] id - the input ID
\ param[in] pOutBufferDesc - output buffer descriptor to be filled in
\ param[in] pBufSize - output size requirement to be filled in
\   Other notes: in the case of v1 meta data, the user can specify the following IDs:
\   NPU_INPUT_BUF_ID_DEFAULT 0x80000000
\   NPU_OUTPUT_BUF_ID_DEFAULT 0x40000000
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_get_buffer_desc(npu_network_handle_t hNetwork, uint32_t id, npu_buffer_desc_v2_t* pOutBufferDesc, int32_t* pBufSize);

/*!
\fn npu_sts_t npu_get_stats_buffer_info
\brief  Get the stats buffer size and layer numbers required for this network.
\ Caller must allocate one stats buffer of this size per execute submitted.
\ param[in] hNetwork - the network handle
\ param[in] pSize - stats buffer size requirement to be filled in
\ param[in] pLayerNum - stats layer numbers requirement to be filled in
\   Other notes:
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
\retval returns NPU_STS_LAYER_STATS_NOT_SUPPORTED in the case of layer stats not supported.
\SNPE shall need to allocate the size(returned by pSize) for BASIC profiling in the case of layer stats not supported.
\Profiling level OFF doesnt require the size to be allocated
\retval non-zero value other than above : Unsuccessful. psize should not be used by SNPE in this case
*/
npu_sts_t npu_get_stats_buffer_info(npu_network_handle_t hNetwork, uint32_t *pSize, uint32_t *pLayerNum);

/*!
\fn npu_sts_t npu_get_serialize_network_size
\brief  Get the buffer size required for serialization of this network.
\ Caller must allocate buffer of this size if serialization is called.
\ Caller must call this function once npu_load_network_v2 finished with no error.
\ param[in] hNetwork - the network handle
\ param[in] *pSize - output uint32_t of required buffer size, if pointer is NULL, then will not fill out
\   Other notes:
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
\retval returns NPU_STS_INVALID_PARAM when provided wrong parameters.
\SNPE shall need to allocate the size(returned by pSize) every time when serializaton (caching) required.
\retval non-zero value other than above : Unsuccessful. psize should not be used by SNPE in this case
*/
npu_sts_t npu_get_serialize_network_size(npu_network_handle_t hNetwork, uint32_t *pSize);

/*!
\fn npu_sts_t npu_serialize_network
\brief  Serialize network data into provided buffer.
\ Caller must call this function once npu_load_network_v2 finished with no error.
\ param[in] pLoadNetworkParam - Pointer to the network loading parameters (npu_load_network_param_v2_t)
\ param[in] hNetwork - the network handle
\ param[in] pBuff - pointer to the buffer to be filled with serialized data
\ param[in] size - size of allocated buffer
\   Other notes:
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
\retval returns NPU_STS_INVALID_PARAM when provided wrong parameters.
\SNPE shall need to allocate the size(returned by pSize) every time when serializaton (caching) required.
\retval non-zero value other than above : Unsuccessful. psize should not be used by SNPE in this case
*/
npu_sts_t npu_serialize_network(npu_load_network_param_t *pLoadNetworkParam, npu_network_handle_t hNetwork, void *pBuff, uint32_t size);

/*!
\fn npu_sts_t npu_validate_serialized_network_hdr
\brief  Check validity of serialized network header.
\ Caller should call this function before npu_load_network_v2 to know what blob to be used.
\ param[in] pSerBuff - pointer to the buffer to be filled with serialized data
\ param[in] pBlob -    pointer to the buffer to be filled current blob data
\   Other notes:
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_validate_serialized_network_hdr(void *pSerBuff, void *pBlob);

/*!
\fn npu_sts_t npu_resize_network_init
\brief Allocate resources and Resize a given network.
\param[in|out] pResizeParams - Pointer to the resize parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_resize_network_init(npu_resize_param_t *pResizeParams);

/*!
\fn npu_sts_t npu_resize_network_deinit
\brief Release allocated by npu_resize_network_init resouces.
\param[in|out] pResizeParams - Pointer to the resize parameters
\retval 0     Successful operation.
\retval non-0 Unsuccessful.
*/
npu_sts_t npu_resize_network_deinit(npu_resize_param_t *pResizeParams);

#endif // NPU_API_H
