/*========================================================================*/
/**
\file hta_hexagon_api.h

    Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
    All rights reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*========================================================================*/

/*
 * Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _HTA_HEXAGON_API_H_
#define _HTA_HEXAGON_API_H_

#include "hta_hexagon_nn_ops.h"
#include <stdint.h>
#include <stdarg.h>

typedef int32_t hexagon_hta_nn_nn_id;

struct input {
    uint32_t src_id;
    uint32_t output_idx;
};

#define NODE_ID_RESERVED_CONSTANT 0


#define MAX_DIMENSIONS 8
struct output {
    uint32_t rank; // dimensions in the tensor
    uint32_t max_sizes[MAX_DIMENSIONS]; // max num elements in each dimension
    uint32_t elementsize; // size of each element
    int32_t zero_offset; // 0 for float / integer values
    float stepsize; // 0 for float/integer values
};

struct perfinfo {
    uint32_t node_id;
    uint32_t executions;
    union {
        uint64_t counter;
        struct {
            uint32_t counter_lo;
            uint32_t counter_hi;
        };
    };
};

typedef struct input hexagon_hta_nn_input;
typedef struct output hexagon_hta_nn_output;
typedef struct perfinfo hexagon_hta_nn_perfinfo;

typedef int32_t hexagon_hta_nn_padding_type;

typedef enum hexagon_hta_nn_config_enum {
    HTA_NN_CONFIG_CREATE_STATIC_NODES = 0,
    HTA_NN_CONFIG_PREALLOC_BUF_GROUP_COUNT,
    HTA_NN_CONFIG_ENABLE_VIA_CDSP,
    HTA_NN_CONFIG_BANDWIDTH_COMPRESSION,
    HTA_NN_CONFIG_POWER_LEVEL,
    HTA_NN_CONFIG_DCVS,
    HTA_NN_CONFIG_API_LOG,
    HTA_NN_CONFIG_PERFORMANCE_LOG, // Deprecated
    HTA_NN_CONFIG_LOG_CALLBACK,
    HTA_NN_CONFIG_MAX = 0x7FFFFFFF
}hexagon_hta_nn_config_enum;

typedef enum padding_type_enum {
    HTA_NN_PAD_NA = 0,
    HTA_NN_PAD_SAME,
    HTA_NN_PAD_VALID,
    HTA_NN_PAD_MIRROR_REFLECT,
    HTA_NN_PAD_MIRROR_SYMMETRIC,
    HTA_NN_PAD_SAME_CAFFE,
} hta_padding_type;

/* HTA NN Error Codes */
/* Using #define to be consistent with the existing APIs */
#define HTA_NN_ERROR_NONE                               0
#define HTA_NN_ERROR_GRAPH_INVALID_ID                  -1
#define HTA_NN_ERROR_GRAPH_STARTUP                     -2
#define HTA_NN_ERROR_GRAPH_NOT_INITIALZIED             -3
#define HTA_NN_ERROR_GRAPH_NOT_CONSTRUCTED             -4
#define HTA_NN_ERROR_GRAPH_NOT_PREPARED                -5
#define HTA_NN_ERROR_GRAPH_NOT_COMPILED                -6
#define HTA_NN_ERROR_FAILURE                           -7
#define HTA_NN_ERROR_CONFIG_ID_UNSUPPORTED             -8
#define HTA_NN_ERROR_INVALID_STATE                     -9
#define HTA_NN_ERROR_SET_POWER_LEVEL                   -10
#define HTA_NN_ERROR_LOAD_NETWORK                      -11
#define HTA_NN_ERROR_BUFF_DESC                         -12
#define HTA_NN_ERROR_INVALID_PARAM                     -13
#define HTA_NN_ERROR_DESERIALIZE_INCORRECT_VERSION     -14
#define HTA_NN_ERROR_ALLOC_MEM                         -15
#define HTA_NN_ERROR_COMPILATION_FAILURE               -16

/* HTA CDSP Flags */
#define HTA_NN_CDSP_FLAGS_DISABLE_POWER_SET             0x1
#define HTA_NN_CDSP_FLAGS_USE_UNSIGNED_PD               0x2

typedef struct {
    uint32_t batches;
    uint32_t height;
    uint32_t width;
    uint32_t depth;
    uint8_t *data;
    uint32_t dataLen;        /* For input and output */
    uint32_t data_valid_len; /* for output only */
    uint32_t unused;
} hexagon_hta_nn_tensordef;

/* Actual functions in the interface */
/* Returns 0 on success, nonzero on error unless otherwise noted */
/* Configure the hardware and software environment.  Should be called once before doing anything */
int hexagon_hta_nn_config( void );

/* Allow for configuring the network */
int hexagon_hta_nn_set_config_params(hexagon_hta_nn_nn_id id, hexagon_hta_nn_config_enum config_id, void *param, uint32_t param_len );

/* Initialize a new graph, returns a new nn_id or -1 on error */
int hexagon_hta_nn_init(hexagon_hta_nn_nn_id *g);

/* Set debug verbosity.  Default is 0, higher values are more verbose */
int hexagon_hta_nn_set_debug_level(hexagon_hta_nn_nn_id id, int level);

/* Append a node to the graph.  Nodes are executed in the appended order. */
int hexagon_hta_nn_append_node(
    hexagon_hta_nn_nn_id id,
    uint32_t node_id,
    hta_op_type operation,
    hta_padding_type padding,
    const struct input *inputs,
    uint32_t num_inputs,
    const struct output *outputs,
    uint32_t num_outputs);

/*
 * Append a const node into the graph.  The data is copied locally during this
 * call, the caller does not need it to persist.
 */
int hexagon_hta_nn_append_const_node(
    hexagon_hta_nn_nn_id id,
    uint32_t node_id,
    uint32_t batches,
    uint32_t height,
    uint32_t width,
    uint32_t depth,
    const uint8_t *data,
    uint32_t data_len);

/*
 * Append an empty const node into the graph.
 */
int hexagon_hta_nn_append_empty_const_node(
    hexagon_hta_nn_nn_id id,
    uint32_t node_id,
    uint32_t batches,
    uint32_t height,
    uint32_t width,
    uint32_t depth,
    uint32_t data_len);

/*
 * Prepare a graph for execution.  Must be done before attempting to execute the graph.
 */
int hexagon_hta_nn_prepare(hexagon_hta_nn_nn_id id);

/*
 * Prepare a model that was generated offline for execution.
 * Must be done before attempting to execute the graph and is used instead of hexagon_hta_nn_prepare
 *
 */
int hexagon_hta_nn_prepare_from_offline_model(
    hexagon_hta_nn_nn_id id,
    const uint8_t *data_in,
    uint32_t data_len_in);

/* Execute the graph with a single input and a single output. */
int hexagon_hta_nn_execute(
    hexagon_hta_nn_nn_id id,
    uint32_t batches_in,
    uint32_t height_in,
    uint32_t width_in,
    uint32_t depth_in,
    const uint8_t *data_in,
    uint32_t data_len_in,
    uint32_t *batches_out,
    uint32_t *height_out,
    uint32_t *width_out,
    uint32_t *depth_out,
    uint8_t *data_out,
    uint32_t data_out_max,
    uint32_t *data_out_size);

/* Tear down a graph, destroying it and freeing resources.  */
int hexagon_hta_nn_teardown(hexagon_hta_nn_nn_id id);

/* Get the version of the library */
int hexagon_hta_nn_version(int *ver);

/* Execute the graph with a multiple input and a multiple output. */
int hexagon_hta_nn_execute_new(
    hexagon_hta_nn_nn_id id,
    const hexagon_hta_nn_tensordef *inputs,
    uint32_t n_inputs,
    hexagon_hta_nn_tensordef *outputs,
    uint32_t n_outputs);

int hexagon_hta_nn_serialize_size(hexagon_hta_nn_nn_id id, uint32_t *serialized_obj_size_out);
int hexagon_hta_nn_serialize(hexagon_hta_nn_nn_id id, void *buf, uint32_t buf_len);
int hexagon_hta_nn_deserialize(void *buf, uint32_t len, hexagon_hta_nn_nn_id *g);

/* -------------------------------------------------------------------------- */
/* Extension APIs                                                             */
/* -------------------------------------------------------------------------- */

typedef enum _hexagon_hta_hw_layout
{
    HEXAGON_HTA_HW_FORMAT_D32 = 1,
    HEXAGON_HTA_HW_FORMAT_DEPTH_FIRST = 2, // channel major
    HEXAGON_HTA_HW_FORMAT_PLANAR = 3,      // image major
    HEXAGON_HTA_HW_FORMAT_MAX
} hexagon_hta_hw_layout;

typedef struct _hexagon_hta_tensor_dim_layout
{
    // Values are in shape
    uint32_t lpadding;
    uint32_t valid;
    uint32_t length;
} hexagon_hta_tensor_dim_layout;

typedef struct _hexagon_hta_nn_hw_tensordef
{
    hexagon_hta_hw_layout format;
    uint32_t elementSize;                              // element size in bytes
    uint32_t numDims;                                  // number of dimensions
    hexagon_hta_tensor_dim_layout dim[MAX_DIMENSIONS];
    uint32_t batchStride;                              // in bytes

    uint32_t dataLen;                                  // in bytes
    int32_t  fd;
    uint64_t htaHWHandle;
    uint64_t flags;
} hexagon_hta_nn_hw_tensordef;

typedef struct _hexagon_hta_nn_tensordef_quant_params
{
    float min;
    float max;
    float offset;
    float delta;
} hexagon_hta_nn_tensordef_quant_params;

typedef void (*hta_hexagon_callback_fn)(void* callbackUserContext, int ret);

typedef void (*hta_hexagon_api_log_callback_fn)(hexagon_hta_nn_nn_id id, const char *api_op, const char *const format, ...);

typedef void (*hta_hexagon_log_callback_fn)(int logLevel, uint32_t networkHandle, uint32_t threadID, const char *const format, ...);

int hexagon_hta_nn_get_memory_layout(hexagon_hta_nn_nn_id id, uint8_t isOutput, uint32_t idx, hexagon_hta_nn_hw_tensordef* pTensor, hexagon_hta_nn_tensordef_quant_params* pTensorQuantParams);
int hexagon_hta_nn_register_tensor(hexagon_hta_nn_nn_id id, hexagon_hta_nn_hw_tensordef* pTensor);
int hexagon_hta_nn_deregister_tensor(hexagon_hta_nn_nn_id id, hexagon_hta_nn_hw_tensordef* pTensor);

/* CPU and DSP API */
int hexagon_hta_nn_execute_hw(hexagon_hta_nn_nn_id id,
                              hexagon_hta_nn_hw_tensordef* pInputs,
                              uint32_t n_inputs,
                              hexagon_hta_nn_hw_tensordef* pOutputs,
                              uint32_t n_outputs,
                              hta_hexagon_callback_fn callbackFn,
                              void* callbackUserContext);

#endif //_HTA_HEXAGON_API_H_
