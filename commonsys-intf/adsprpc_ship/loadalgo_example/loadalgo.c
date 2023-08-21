/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <utils/Log.h>
#include "QSEEComAPI.h"
#include "common_log.h"
#include <sys/mman.h>
#include <sys/time.h>
#include <getopt.h>
#include <dlfcn.h>
#include "loadalgo.h"
#include "secure_memory.h"
#include "remote.h"
#include "AEEStdErr.h"

/** adb log */
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LoadAlgo"
#ifdef LOG_NDDEBUG
#undef LOG_NDDEBUG
#endif
#define LOG_NDDEBUG 0 //Define to enable LOGD
#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#endif
#define LOG_NDEBUG  0 //Define to enable LOGV

#define DEFAULT_DYNAMIC_HEAP_SIZE 0x2800000 //40MB secure memory added to amss heap in securePD.
#define DEFAULT_HLOS_PHYSPOOL_SIZE 0x800000 //8MB of secure memory added to HLOS Physical Pool in securePD.
#define ALIGNMENT   0x1000
#define KM_SB_LENGTH (4096 * 2)
#define ERRNO (errno == 0 ? -1 : errno)

#define WIDTH 1920
#define HEIGHT 1080
#define LOOPS 1

#define MAX_NUMBER_PATHS 3
static char* search_paths[MAX_NUMBER_PATHS] = {"/vendor/firmware_mnt/image", "/vendor/firmware/", "/firmware/image/"};
static unsigned dynamic_heap_size = DEFAULT_DYNAMIC_HEAP_SIZE;
static unsigned hlos_physpool_size = DEFAULT_HLOS_PHYSPOOL_SIZE;

/* Structure of the packets that are sent to the trusted app via QSEECom from HLOS client */
typedef struct send_cmd_struct {
    uint32_t  cmd_id;           /* command ID */
    uint64_t  secureBuf;        /* Secure Buffer Address */
    uint32_t  secureBufsize;    /* Secure Buffer Size */
    uint32_t  Type;             /* Secure Buffer Type */
    uint64_t  nonSecureBuf;     /* Non Secure Buffer Address */
    uint32_t  nonSecureBufsize; /* Non Secure Buffer Size */
}__attribute__ ((packed)) send_cmd_t;

/* Types of Secure Buffers */
typedef enum buffer_type {
    MODEL = 1,                 /* Model elf */
    AUTH = 2,                  /* Face Auth elf*/
    CAMERA = 3,                /* Camera buffer */
    ALGO = 4,                  /* Algo elf */
    HEAP = 5,                  /* Heap buffer */
    POOL = 6,                  /* Pool buffer */
    DATA = 7                   /* Data buffer */
} buf_type_t;

/* Different commands that can be sent to TA*/
enum commands {
    REGISTER_BUFFER = 0,       /* Register the given secure buffer */
    DEREGISTER_BUFFER,         /* Deregister the given secure buffer */
    DECRYPT_BUFFER,            /* Decrypt the given non secure buffer and copy over the contents to secure buffer */
    COPYNONSECURE_TO_SECURE,   /* Copy contents from non secure buffer to secure buffer */
    SEC_MATCH_RESULT,          /* Match the result computed by TA with that of securePD */
    SEC_INPUT_BUFFER_INIT      /* Initialize secure buffer */
};

/* Error codes used by loadalgo app */
enum loadalgo_error_codes {
    LOAD_ERROR = 500,
    ALLOCATE_ERROR,
    FILEOPS_ERROR,
    INVALID_STATE,
    INVALID_INPUT
};

/* State of the secure channel between securePD and TA */
typedef enum state_type {
    RESET = 0,
    INIT,
    LOADED,
    DONE,
    PASS,
    ERROR,
} state_type_t;

#pragma weak  remote_register_dma_handle_attr

struct keymaster_handle
{
    struct QSEECom_handle *qseecom;
    void *libhandle;
    void *librpchandle;
    remote_handle64 cdsp;
    int (*QSEECom_start_app)(struct QSEECom_handle ** handle, const char* path,
                          const char* appname, uint32_t size);
    int (*QSEECom_shutdown_app)(struct QSEECom_handle **handle);
    int (*QSEECom_send_cmd)(struct QSEECom_handle* handle, void *cbuf,
                          uint32_t clen, void *rbuf, uint32_t rlen);
    int (*QSEECom_send_modified_cmd)(struct QSEECom_handle* handle, void *cbuf,
                          uint32_t clen, void *rbuf, uint32_t rlen,
                          struct QSEECom_ion_fd_info *ihandle);
    int (*QSEECom_send_modified_cmd_64)(struct QSEECom_handle *handle, void *send_buf,
                          uint32_t sbuf_len, void *resp_buf, uint32_t rbuf_len,
                          struct QSEECom_ion_fd_info  *ifd_data);
    int (*QSEECom_set_bandwidth)(struct QSEECom_handle* handle, bool high);
    int (*loadalgo_open)(const char*, remote_handle64*);
    int (*loadalgo_init)(remote_handle64);
    int (*loadalgo_physbuffer)(remote_handle64, int din, uint32 dinOffset, uint32 dinLen, int type);
    int (*loadalgo_Gaussian7x7u8)(remote_handle64 h, int srcFd, uint32 srcOffset, uint32 srcLen, uint32 srcWidth,
                                    uint32 srcHeight, uint32 srcStride, int dstFd, uint32 dstOffset,
                                    uint32 dstLen, uint32 dstStride, int heap, uint32 heapOffset, uint32 heapLen, uint32 modeStatic);
    int (*loadalgo_deinit)(remote_handle64);
    int (*loadalgo_close)(remote_handle64);
};
typedef struct keymaster_handle keymaster_handle_t;
static keymaster_handle_t* km_handle;

static struct option example_options[] =
{
    {"loadalgo", 1, NULL, 'l'},
    {"rungaussian", 1, NULL, 'r'},
    {"rungaussian_and_exit", 1, NULL, 'e'},
    {"help", no_argument, NULL, 'h'},
    {"rungaussian_static", no_argument, NULL, 's'},
    {NULL, 0, NULL, 0},
    {NULL, 0, NULL, 0},
};

static mem_handle nonsecureHandle, secureHandle, heapHandle, poolHandle;
static int loadalgoInit, proxyInit, TAinit;

unsigned long long GetTime( void )
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static int km_get_rpclib_sym(keymaster_handle_t* km_handle, const char *rpclib)
{
    int nErr = 0;
    km_handle->librpchandle =  dlopen(rpclib, RTLD_NOW);
    if (km_handle->librpchandle)
    {
        *(void **)(&km_handle->loadalgo_open) =
                               dlsym(km_handle->librpchandle,"loadalgo_open");
        if (km_handle->loadalgo_open == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_open\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->loadalgo_init) =
                                dlsym(km_handle->librpchandle,"loadalgo_init");
        if (km_handle->loadalgo_init == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_init\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->loadalgo_physbuffer) =
                                dlsym(km_handle->librpchandle,"loadalgo_physbuffer");
        if (km_handle->loadalgo_physbuffer == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_physbuffer\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->loadalgo_Gaussian7x7u8) =
                                dlsym(km_handle->librpchandle,"loadalgo_Gaussian7x7u8");
        if (km_handle->loadalgo_Gaussian7x7u8 == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_Gaussian7x7u8\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->loadalgo_deinit) =
                                dlsym(km_handle->librpchandle,"loadalgo_deinit");
        if (km_handle->loadalgo_deinit == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_deinit\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->loadalgo_close) =
                                dlsym(km_handle->librpchandle,"loadalgo_close");
        if (km_handle->loadalgo_close == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : dlsym failed to find loadalgo_close\n", nErr, __func__);
            goto bail;
        }
    }
    else {
        nErr = LOAD_ERROR;
        ALOGE("%s: Failed to load loadalgo_stub shared object with nErr: 0x%x\n", __func__, nErr);
        return nErr;
    }
bail:
    if (nErr != 0) {
        dlclose(km_handle->librpchandle );
        km_handle->librpchandle  = NULL;
    }
    return nErr;
}

static int km_get_lib_sym(keymaster_handle_t* km_handle, const char *qseecomlib)
{
    int nErr = 0;
    km_handle->libhandle = dlopen(qseecomlib, RTLD_NOW);
    if (km_handle->libhandle) {
        *(void **)(&km_handle->QSEECom_start_app) =
                               dlsym(km_handle->libhandle,"QSEECom_start_app");
        if (km_handle->QSEECom_start_app == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_start_app\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->QSEECom_shutdown_app) =
                                dlsym(km_handle->libhandle,"QSEECom_shutdown_app");
        if (km_handle->QSEECom_shutdown_app == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_shutdown_app\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->QSEECom_send_cmd) =
                                dlsym(km_handle->libhandle,"QSEECom_send_cmd");
        if (km_handle->QSEECom_send_cmd == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_send_cmd\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->QSEECom_send_modified_cmd) =
                                dlsym(km_handle->libhandle,"QSEECom_send_modified_cmd");
        if (km_handle->QSEECom_send_modified_cmd == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_send_modified_cmd\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->QSEECom_send_modified_cmd_64) =
                                dlsym(km_handle->libhandle,"QSEECom_send_modified_cmd_64");
        if (km_handle->QSEECom_send_modified_cmd_64 == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_send_modified_cmd_64\n", nErr, __func__);
            goto bail;
        }
        *(void **)(&km_handle->QSEECom_set_bandwidth) =
                                dlsym(km_handle->libhandle,"QSEECom_set_bandwidth");
        if (km_handle->QSEECom_set_bandwidth == NULL) {
            nErr = LOAD_ERROR;
            ALOGE("Error: 0x%x : %s : Failed to find QSEECom_set_bandwidth\n", nErr, __func__);
            goto bail;
        }
    } else {
        nErr = LOAD_ERROR;
        ALOGE("%s: Failed to load qseecom library with nErr:0x%x\n", __func__, nErr);
        return nErr;
    }
bail:
    if (nErr != 0) {
        dlclose(km_handle->libhandle );
        km_handle->libhandle  = NULL;
    }
    return nErr;
}

/**
  * @brief : Send different commands to trusted application from hlos client
  * @param cmd: Command type.
  * @param secBufFd: Secure Buffer fd.
  * @param secBufLen: Secure Buffer length.
  * @param nonsecBufFd: Non-Secure Buffer fd.
  * @param nonsecBufLen: Non-Secure Buffer length.
  * @param type: Secure Buffer Type.
  * @param state: Secure Channel State.
  */
static int send_sec_dsp_cmd_to_ta(uint32_t cmd, uint32_t secBufFd, uint32_t secBufLen,
                                  uint32_t nonsecBufFd, uint32_t nonsecBufLen,
                                  buf_type_t type, uint64_t* state)
{
    int nErr = 0;
    struct QSEECom_handle *handle = NULL;
    send_cmd_t *shared_buf_req;
    uint64_t   *shared_buf_rsp;
    uint32_t   req_buf_len;
    uint32_t   rsp_buf_len;
    struct QSEECom_ion_fd_info ion_fd_info;

    handle = (struct QSEECom_handle *)(km_handle->qseecom);
    if (!handle) {
        nErr = INVALID_INPUT;
        ALOGE("%s: qseecom handle is NULL, nErr:%d\n", __func__, nErr);
        goto bail;
    }

    if (secBufFd == 0 || secBufLen == 0) {
        nErr = INVALID_INPUT;
        ALOGE("%s: Secure buffer fd/length is NULL nErr:%d\n", __func__, nErr);
        goto bail;
    }

    shared_buf_req = (send_cmd_t *)(handle->ion_sbuffer);
    shared_buf_req->cmd_id = cmd;
    shared_buf_req->secureBufsize = secBufLen;
    shared_buf_req->nonSecureBufsize = nonsecBufLen;
    shared_buf_req->Type = (uint32_t)type;
    req_buf_len = sizeof(*shared_buf_req);
    rsp_buf_len = sizeof(*shared_buf_rsp);

    // align buffers
    if (req_buf_len & QSEECOM_ALIGN_MASK) {
        req_buf_len = QSEECOM_ALIGN(req_buf_len);
    }
    if (rsp_buf_len & QSEECOM_ALIGN_MASK) {
        rsp_buf_len = QSEECOM_ALIGN(rsp_buf_len);
    }

    shared_buf_rsp = (uint64_t *)((uint64_t)handle->ion_sbuffer + (uint64_t)req_buf_len);
    memset(shared_buf_rsp, 0, rsp_buf_len);

    memset(&ion_fd_info, 0, sizeof(ion_fd_info));
    ion_fd_info.data[0].fd = secBufFd;
    ion_fd_info.data[0].cmd_buf_offset = (uint32_t)((uint8_t *)&shared_buf_req->secureBuf - (uint8_t *)shared_buf_req);
    ion_fd_info.data[1].fd = nonsecBufFd;
    ion_fd_info.data[1].cmd_buf_offset = (uint32_t)((uint8_t *)&shared_buf_req->nonSecureBuf - (uint8_t *)shared_buf_req);
    ALOGD("Send modified command: fd %d offset 0x%x", ion_fd_info.data[0].fd, ion_fd_info.data[0].cmd_buf_offset);

    nErr = (*km_handle->QSEECom_send_modified_cmd_64)(handle, shared_buf_req, req_buf_len, shared_buf_rsp, rsp_buf_len, &ion_fd_info);
    ALOGD("shared_buf_rsp is %lu", *shared_buf_rsp);
    if (state != NULL) {
      *state = *shared_buf_rsp;
    }
    if (nErr) {
        ALOGE("%s: Failed to send command to TA with nErr:%d\n", __func__, nErr);
        goto bail;
    }
bail:
    return nErr;
}

/**
  * @brief: Load the trusted application.
  * @param km_handle: Handle associated with the HLOS client app.
  * @retval : 0 in case of success.
  */
static int start_ta(keymaster_handle_t* km_handle){
    int nErr, i = 0;

    for(i = 0; i < MAX_NUMBER_PATHS; i++) {
        nErr = (*km_handle->QSEECom_start_app)((struct QSEECom_handle **)&km_handle->qseecom,
                        search_paths[i], "loadalgota64", KM_SB_LENGTH);
        if (nErr == 0) {
            break;
        }
    }
    if (!nErr) {
        TAinit = 1;
        printf("%s done with QSEECom handle %p\n", __func__, km_handle->qseecom);
    }
    return nErr;
}

/**
  * @brief: Open a remote handle and start the proxyPD on QDSP.
  * @param km_handle: Handle associated with the HLOS client app.
  * @retval : 0 in case of success.
  */
static int start_proxy(keymaster_handle_t* km_handle) {
    int nErr = 0;

    nErr = (*km_handle->loadalgo_open)(loadalgo_URI "&_dom=cdsp", &km_handle->cdsp);
    if (nErr) {
        ALOGE("%s: Failed to open cdsp domain with nErr: 0x%x\n", __func__, nErr);
        return nErr;
    }
    proxyInit = 1;
    printf("%s: created proxy PD on CDSP with handle 0x%x\n", __func__, (int)km_handle->cdsp);

    nErr = (*km_handle->loadalgo_init)(km_handle->cdsp);
    if (nErr) {
        ALOGE("%s: Failed to create proxy with nErr: 0x%x\n", __func__, nErr);
        return nErr;
    }
    printf("%s: initialized proxy PD on CDSP\n", __func__);

    return nErr;
}

/**
  * @brief: Setup the securePD QDSP, like adding heap and pool memory.
  * @param km_handle: Handle associated with the HLOS client app.
  * @retval : 0 in case of success.
  */
static int initialize_securepd(keymaster_handle_t* km_handle) {
    int nErr = 0;
    uint64_t state = 0;

    nErr = secure_mem_alloc(&heapHandle, dynamic_heap_size, ALIGNMENT, TRUE);  //Allocate heap memory
    if (nErr) {
        ALOGE("%s: Failed to allocate heap buffer of size:0x%x\n", __func__, dynamic_heap_size);
        printf("%s: Allocation failed for size:0x%x\n --- Read cdsp_secure_heap node from target dtsi file to find out the memory configured for secure heap usecase.\n", __func__, dynamic_heap_size);
        goto bail;
    }
    nErr = remote_register_dma_handle_attr(heapHandle.fd, heapHandle.size, FASTRPC_ATTR_NOMAP);
    if (nErr) {
        ALOGE("%s: Failed to register dma handle for fd:%d with nErr:%d\n", __func__, heapHandle.fd, nErr);
        goto bail;
    }
    printf("%s: allocated and registered secure HEAP fd %d of size 0x%x\n", __func__, heapHandle.fd, dynamic_heap_size);

    nErr = secure_mem_alloc(&poolHandle, hlos_physpool_size, ALIGNMENT, TRUE);  //Allocate secure memory
    if (nErr) {
        ALOGE("%s: Failed to allocate pool buffer of size:0x%x\n", __func__, hlos_physpool_size);
        printf("%s: Allocation failed for size:0x%x\n --- Read cdsp_secure_heap node from target dtsi file to find out the memory configured for secure heap usecase.\n", __func__, hlos_physpool_size);
        goto bail;
    }
    nErr = remote_register_dma_handle_attr(poolHandle.fd, poolHandle.size, FASTRPC_ATTR_NOMAP);
    if (nErr) {
        ALOGE("%s: Failed to register dma handle for fd:%d with nErr:%d\n", __func__, poolHandle.fd, nErr);
        goto bail;
    }
    printf("%s: allocated and registered secure POOL fd %d of size 0x%x\n", __func__, poolHandle.fd, hlos_physpool_size);

    nErr = (*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, true);
    if (nErr) {
        ALOGE("%s: aes enc command failed (unable to enable clks) nErr=%d", __func__, nErr);
        goto bail;
    }
    nErr = send_sec_dsp_cmd_to_ta(REGISTER_BUFFER, heapHandle.fd, heapHandle.size, 0, 0, HEAP, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to register secure buffer:%d  with nErr:%d state:0x%llx\n", __func__, heapHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: registered secure HEAP buffer with TA\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(REGISTER_BUFFER, poolHandle.fd, poolHandle.size, 0, 0, POOL, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to register secure buffer:%d  with nErr:%d state:0x%llx\n", __func__, poolHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: registered secure POOL buffer with TA\n", __func__);

    nErr = (*km_handle->loadalgo_physbuffer)(km_handle->cdsp, heapHandle.fd, 0, heapHandle.size, HEAP);
    if (nErr) {
        ALOGE("%s: Failed to add heap buffer to the securePD with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    printf("%s: added HEAP buffer to securePD over proxy\n", __func__);

    nErr = (*km_handle->loadalgo_physbuffer)(km_handle->cdsp, poolHandle.fd, 0, poolHandle.size, POOL);
     if (nErr) {
       ALOGE("%s: Failed to add secure buffer to HLOS_PHYSPOOL with nErr:%d\n", __func__, nErr);
       goto bail;
    }
    printf("%s: added POOL buffer to securePD over proxy\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(DEREGISTER_BUFFER, heapHandle.fd, heapHandle.size, 0, 0, HEAP, &state); //TODO: Do we need to deregister here??
    if (nErr || state) {
        ALOGE("%s: Failed to deregister heap buffer fd:%d  with nErr:%d state:0x%llx\n", __func__, heapHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: deregistered HEAP buffer from TA\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(DEREGISTER_BUFFER, poolHandle.fd, poolHandle.size, 0, 0, POOL, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to deregister pool buffer fd:%d  with nErr:%d state:0x%llx\n", __func__, poolHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: deregistered POOL buffer from TA\n", __func__);

bail:
    if((*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, false)) {
        ALOGE("%s: aes enc command: (unable to disable clks)\n", __func__);
    }
    if (nErr != 0) {
        if (heapHandle.fd) {
            secure_mem_free(&heapHandle);
        }
        if (poolHandle.fd) {
            secure_mem_free(&poolHandle);
        }
    }
    return nErr;
}

/**
  * @brief: Resolve the symbols in loadalgo_stub, qseecom static
  *         library , load the TA, create the proxyPD and setup
  *         the securePD.
  * @retval : 0 in case of success
  */
static int km_proxy_open()
{
    int nErr = 0;
    const char qseecomlib[] = "libQSEEComAPI.so";
    const char rpclib[] = "libloadalgo_stub.so";

    if (!loadalgoInit) {
        km_handle = (keymaster_handle_t *)malloc(sizeof(keymaster_handle_t));
        if (km_handle == NULL) {
            nErr = ALLOCATE_ERROR; //mem failed
            ALOGE("%s: Failed to allocate memory for keymaster handle with nErr:%d\n", __func__, nErr);
            goto bail;
        }
        km_handle->qseecom = NULL;
        km_handle->libhandle = NULL;
        nErr = km_get_lib_sym(km_handle, qseecomlib);
        if (nErr) {
            ALOGE("%s: Failed to get the symbols in %s with nErr %d\n", __func__, qseecomlib, nErr);
            goto bail;
        }
        printf("%s: loaded symbols from %s\n", __func__, qseecomlib);

        nErr = km_get_rpclib_sym(km_handle, rpclib);
        if (nErr) {
            ALOGE("%s: Failed to get the symbols in %s with nErr %d\n", __func__, rpclib, nErr);
            goto bail;
        }
        printf("%s: loaded symbols from %s\n", __func__, rpclib);

        nErr = start_ta(km_handle);
        if (nErr) {
            ALOGE("%s: Failed to load keymaster app with nErr:%d\n", __func__, nErr);
            goto bail;
        }
        printf("%s: started loadalgo TA\n", __func__);

        nErr = start_proxy(km_handle);
        if (nErr) {
            ALOGE("%s: Failed to start proxy with nErr:%d\n", __func__, nErr);
            goto bail;
        }
        printf("%s: proxy PD launched on CDSP\n", __func__);

        nErr = initialize_securepd(km_handle);
        if (nErr) {
           ALOGE("%s: Failed to initialize secure pd with nErr:%d\n", __func__, nErr);
           goto bail;
        }
        loadalgoInit = 1;
        printf("%s: securePD initialization done\n", __func__);
    }
bail:
    if (nErr) {
        if (km_handle && !proxyInit && !TAinit) {
            // In case of failures after TA/proxy init, need to clean them up before freeing handle
            free(km_handle);
            km_handle = 0;
        }
    }
    return nErr;
}

/**
  * @brief: Unload the TA and deinitialize the proxyPD on QDSP.
  */
static void km_proxy_close()
{
    int nErr = 0;
    if (km_handle) {
        if (TAinit) {
            if (km_handle->qseecom) {
                printf("%s: shutting down TA with QSEECom handle %p...\n", __func__, km_handle->qseecom);
                nErr = (*km_handle->QSEECom_shutdown_app)((struct QSEECom_handle **)&km_handle->qseecom);
                if (nErr) {
                    ALOGE("%s: Failed to shutdown TA with nErr %d\n", __func__, nErr);
                }
                printf("%s: shut down TA\n", __func__);
            } else {
                ALOGE("%s: Qseecom handle is NULL\n", __func__);
            }
        }

        if (proxyInit) {
            printf("%s: cleaning up the proxy session on CDSP\n", __func__);
            nErr = (*km_handle->loadalgo_deinit)(km_handle->cdsp);
            if (nErr) {
                ALOGE("%s: Failed to deinitialize proxy with nErr 0x%x\n", __func__, nErr);
            }
            printf("%s: deinitialized proxy PD\n", __func__);

            nErr = (*km_handle->loadalgo_close)(km_handle->cdsp);
            if (nErr) {
                ALOGE("%s: Failed to close domain in proxy with nErr 0x%x\n", __func__, nErr);
            }
            printf("%s: closed proxy PD session with handle 0x%x\n", __func__, (int)km_handle->cdsp);
        }

        if (heapHandle.fd) {
            secure_mem_free(&heapHandle);
        }
        if (poolHandle.fd) {
            secure_mem_free(&poolHandle);
        }
        printf("%s: freed HEAP and POOL buffers\n", __func__);

        free(km_handle);
        km_handle = NULL;
    }
    loadalgoInit = 0;
    return;
}

/**
  * @brief: Display the various options supported by the HLOS client app.
  */
static void example_usage(void)
{
 printf("\n"
   " \t\t------------------------------------------------------------\n"
   " \t\t -d <size in hex>       : dynamic heap memory size (default 0x%x)\n"
   " \t\t -p <size in hex>       : hlos physpool size (default 0x%x)\n"
   " \t\t -l <path to algo elf>  : load algo elf in secure pd\n"
   " \t\t -r <path to algo elf>  : load and run algo elf in secure pd\n"
   " \t\t -e <path to algo elf>  : load and run algo elf in secure pd and exit\n"
   " \t\t -s                     : run static gaussian in secure pd\n"
   " \t\t -h or --help           : print this help message\n\n"
   " \t\t Examples : \n"
   " \t\t adb shell /vendor/bin/loadalgo -r \"/vendor/bin/example_image.so\"\n"
   " \t\t adb shell /vendor/bin/loadalgo -d A00000 -p 800000 -s\n"
   " \t\t--------------------------------------------------------------\n\n\n",
   DEFAULT_DYNAMIC_HEAP_SIZE, DEFAULT_HLOS_PHYSPOOL_SIZE);
}

/**
  * @brief: Parse the signed and encrypted algorithm elf.
  * @param stream: Associated stream with the opened file.
  * @param memsz : Size of the file in bytes.
  * @retval : 0 in case of success.
  */
static int parse_algo_elf(FILE* stream, size_t* memsz)
{
    int nErr = 0;
    int fd = -1;
    struct stat st;
    size_t sz;

    fd = fileno(stream);
    if (fd == -1) {
        nErr = FILEOPS_ERROR;
        ALOGE("%s: Couldn't retrieve the fd due to errno: %s nErr:%d\n", __func__, strerror(ERRNO), nErr);
        goto bail;
    }
    nErr = fstat(fd, &st);
    if (nErr == -1) {
        ALOGE("%s: Couldn't retrieve information of file:%d , errno:%s nErr:%d\n", __func__, fd, strerror(ERRNO), nErr);
        goto bail;
    }
    sz = (size_t)st.st_size;
    *memsz = sz;
bail:
    return nErr;
}

/**
  * @brief: Copy contents of algo elf to non-secure buffer.
  * @param stream: Associated stream with the opened file.
  * @param nonsecureHandle: Handle associated with non-secure buffer.
  * @param memsz : Size of the file in bytes.
  * @param numItems: Number of bytes read from the algo elf.
  * @retval : 0 in case of success
  */
static int copy_algoelf_to_buffer(FILE* stream, mem_handle* nonsecureHandle, size_t memsz, size_t* numItems)
{
    int nErr = 0;
    size_t numRead = 0;
    int fd = -1;

    fd = fileno(stream);
    nErr = fseek(stream, 0, SEEK_SET);
    if (nErr) {
        ALOGE("%s: Failed to seek to the start of the file:%d, errno:%s\n nErr:%d", __func__, fd, strerror(ERRNO), nErr);
        goto bail;
    }
    numRead = fread((char*)nonsecureHandle->addr, 1, memsz, stream);
    if (numRead != memsz) {
        nErr = FILEOPS_ERROR;
        ALOGE("%s: Failed to read size:%zu bytes from fd:%d nErr:%d\n", __func__, memsz, fd, nErr);
    }
    *numItems = numRead;
bail:
    return nErr;
}

/**
  * @brief: Load the algo elf in securePD on QDSP.
  * @param path: Path of the signed and encrypted algo elf.
  * @retval: 0 in case of success.
  */
static int start_load_algo_elf(char* path)
{
    int nErr = 0;
    size_t memsz, numItems = 0;
    FILE* stream = NULL;
    uint64_t state = 0;

    printf("%s starting\n", __func__);
    nErr = km_proxy_open();
    if (nErr) {
        ALOGE("%s: Failed to start keymaster/proxy with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    printf("%s: proxy, TA and securePD init completed\n", __func__);

    stream = fopen(path, "r");
    if (!stream) {
        nErr = FILEOPS_ERROR; // File operation failed
        ALOGE("%s: Failed to open algorithm elf:%s, errno: %s nErr:%d\n", __func__, path, strerror(ERRNO), nErr);
        goto bail;
    }
    nErr = parse_algo_elf(stream, &memsz);
    if (nErr) {
        ALOGE("%s: Failed to parse the alogorithm elf with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    printf("%s: opened and parsed algo elf %s\n", __func__, path);

    nErr = secure_mem_alloc(&nonsecureHandle, memsz, ALIGNMENT, FALSE); //Allocate non secure memory
    if (nErr) {
        ALOGE("%s: Failed to allocate non secure buffer of size:%zu\n", __func__, memsz);
        goto bail;
    }
    printf("%s: allocated non-secure ELF buffer with fd %d of size %zu\n", __func__, nonsecureHandle.fd, memsz);

    nErr = copy_algoelf_to_buffer(stream, &nonsecureHandle, memsz, &numItems);
    if (nErr) {
        ALOGE("%s: Failed to copy size:%zu bytes from fd:%d\n", __func__, memsz, fileno(stream));
        goto bail;
    }
    printf("%s: copied numItems %zu into non-secure buffer\n", __func__, numItems);

    nErr = secure_mem_alloc(&secureHandle, memsz, ALIGNMENT, TRUE);  //Allocate secure memory
    if (nErr) {
        ALOGE("%s: Failed to allocate secure buffer of size:%zu\n", __func__, memsz);
        goto bail;
    }
    nErr = remote_register_dma_handle_attr(secureHandle.fd, secureHandle.size, FASTRPC_ATTR_NOMAP);
    if (nErr) {
        ALOGE("%s: Failed to register dma handle for fd:%d with nErr:%d\n", __func__, secureHandle.fd, nErr);
        goto bail;
    }
    printf("%s: allocated and registered secure ALGO buffer with fd %d of size %zu\n", __func__, secureHandle.fd, memsz);

    nErr = (*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, true);
    if (nErr) {
        ALOGE("%s: aes enc command failed (unable to enable clks) nErr=%d\n", __func__, nErr);
        goto bail;
    }
    nErr = send_sec_dsp_cmd_to_ta(REGISTER_BUFFER, secureHandle.fd, secureHandle.size, 0, 0, ALGO, &state);  //REGISTER SECURE BUFFER
    if (nErr || state) {
        ALOGE("%s: Failed to register secure buffer:%d  with nErr:%d state:0x%llx\n", __func__, secureHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: registered secure ALGO buffer with TA\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(DECRYPT_BUFFER, secureHandle.fd, secureHandle.size, nonsecureHandle.fd, nonsecureHandle.size, ALGO, &state);  //DECRYPT AND COPY NONSECURE TO SECURE BUFFER
    if (nErr || state) {
       ALOGE("%s: Failed to copy contents from nonsecure buffer:%d to securebuffer:%d with nErr:%d state:0x%llx\n", __func__, nonsecureHandle.fd, secureHandle.fd, nErr, state);
       nErr = nErr || state;
       goto bail;
    }
    printf("%s: decrypted and copied ELF contents from non-secure buffer (fd %d, size %u) to secure buffer (fd %d, size %u)\n",
            __func__, nonsecureHandle.fd, nonsecureHandle.size, secureHandle.fd, secureHandle.size);

    //send fastrpc command to proxy
    nErr = (*km_handle->loadalgo_physbuffer)(km_handle->cdsp, secureHandle.fd, 0, secureHandle.size, ALGO);
    if (nErr) {
       ALOGE("%s: Failed to load algorithm elf in securePD with nErr:%d\n", __func__, nErr);
       goto bail;
    }
    printf("%s: dynamically loaded secure algo on securePD using ALGO buffer\n", __func__);

    //Deregister secure buffer
    nErr = send_sec_dsp_cmd_to_ta(DEREGISTER_BUFFER, secureHandle.fd, secureHandle.size, 0, 0, ALGO, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to deregister secure buffer fd:%d  with nErr:%d state:0x%llx\n", __func__, secureHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: deregistered ALGO buffer from TA\n", __func__);

bail:
    if (km_handle) {
        if((*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, false)) {
            ALOGE("%s: aes enc command: (unable to disable clks)\n", __func__);
        }
    }
    //Free non-secure and secure buffer
    if (secureHandle.fd) {
        secure_mem_free(&secureHandle);
    }
    if (nonsecureHandle.fd) {
        secure_mem_free(&nonsecureHandle);
    }
    printf("%s: freed secure and non-secure buffers\n", __func__);

    if (stream) {
        fclose(stream);
    }
    //TODO: handle error scenarios, currently deregister of  buffers is not happening in bail.
    if (nErr != 0) {
        printf("Error 0x%x: %s: cleaning up TA and proxy\n", nErr, __func__);
        km_proxy_close();
    }
    return nErr;
}

/**
  * @brief: Run the gaussian algorithm in securePD on QDSP.
  * @param path: Path to signed and encrypted algo elf.
  * @retval: Returns 0 on success.
  */
int run_gaussian_algo(char* path)
{
    int nErr, i = 0;
    mem_handle srcMemHandle = {0};
    mem_handle dstMemHandle = {0};
    uint64_t state = 0;
    uint32_t srcWidth = WIDTH;
    uint32_t srcHeight = HEIGHT;
    uint32_t srcStride = srcWidth;    // keep aligned to 128 bytes!
    uint32_t dstHeight = srcHeight;
    uint32_t dstStride = srcStride;
    int srcSize = srcStride * srcHeight;
    int dstSize = dstStride * dstHeight;
    uint32_t gaussian_static = 0;         // Dynamic loading of gaussian algorithm by default

    if (NULL != path) {
        printf("%s: User entered path %s to algo elf\n", __func__, path);
        nErr = start_load_algo_elf(path);
        if (nErr != 0) {
            ALOGE("%s: Failed to load %s in secure PD with nErr:%d\n", __func__, path, nErr);
            goto bail;
        }
        printf("%s: loaded encrypted shared object on securePD\n", __func__);
    } else {
        printf("%s: user wants to run the static gaussian\n", __func__);
        gaussian_static = 1;

        nErr = km_proxy_open();
        if (nErr) {
            ALOGE("%s: Failed to start keymaster/proxy with nErr:%d\n", __func__, nErr);
            goto bail;
        }
        printf("%s: proxy, TA and securePD init completed\n", __func__);
    }

    nErr = secure_mem_alloc(&srcMemHandle, srcSize, ALIGNMENT, TRUE);
    if (nErr != 0) {
       ALOGE("%s: Failed to allocate source data buffer with nErr:%d\n", __func__, nErr);
       goto bail;
    }
    nErr = remote_register_dma_handle_attr(srcMemHandle.fd, srcMemHandle.size, FASTRPC_ATTR_NOMAP);
    if (nErr) {
        ALOGE("%s: Failed to register dma handle for fd:%d with nErr:%d\n", __func__, srcMemHandle.fd, nErr);
        goto bail;
    }
    printf("%s: allocated and registered secure SRC buffer with fd %d of size %d\n", __func__, srcMemHandle.fd, srcSize);

    nErr = secure_mem_alloc(&dstMemHandle, dstSize, ALIGNMENT, TRUE);
    if (nErr) {
        ALOGE("%s: Failed to allocate destination data buffer with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    nErr = remote_register_dma_handle_attr(dstMemHandle.fd, dstMemHandle.size, FASTRPC_ATTR_NOMAP);
    if (nErr) {
        ALOGE("%s: Failed to register dma handle for fd:%d with nErr:%d\n", __func__, dstMemHandle.fd, nErr);
    }
    printf("%s: allocated and registered secure DST buffer with fd %d of size %d\n", __func__, dstMemHandle.fd, dstSize);

    nErr = (*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, true);
    if (nErr) {
        ALOGE("%s: aes enc command failed (unable to enable clks) nErr=%d", __func__, nErr);
        goto bail;
    }
    nErr = send_sec_dsp_cmd_to_ta(REGISTER_BUFFER, srcMemHandle.fd, srcMemHandle.size, 0, 0, DATA, &state);  //REGISTER SECURE BUFFER
    if (nErr || state) {
        ALOGE("%s: Failed to register src buffer:%d  with nErr:%d state:0x%llx\n", __func__, srcMemHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: registered secure SRC buffer with TA\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(REGISTER_BUFFER, dstMemHandle.fd, dstMemHandle.size, 0, 0, DATA, &state);  //REGISTER SECURE BUFFER
    if (nErr || state) {
        ALOGE("%s: Failed to register secure buffer:%d  with nErr:%d state:0x%llx\n", __func__, dstMemHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: registered secure DST buffer with TA\n", __func__);

    //Call TA to initialize secure buffer with secure contents
    nErr = send_sec_dsp_cmd_to_ta(SEC_INPUT_BUFFER_INIT, srcMemHandle.fd, srcMemHandle.size, 0, 0, DATA, &state);
    if (nErr || state) {
        ALOGE("%s: SEC_INPUT_BUFFER_INIT failed for source buffer fd:%d with nErr:%d state:0x%llx\n", __func__, srcMemHandle.fd, nErr, state);
        goto bail;
    }
    printf("%s: initialized SRC buffer using TA\n", __func__);

    //Call DSP SecurePD with secure buffer
    printf("%s: calling Gaussian on a %ux%u image...\n", __func__, srcWidth, srcHeight);
    unsigned long long t1 = GetTime();
    for (i = 0; i < LOOPS; i++)
    {
        // For HVX case, note that src, srcStride, dst, dstStride all must be multiples of 128 bytes.
        // The HVX code for this example function does not handle unaligned inputs.
        nErr = (*km_handle->loadalgo_Gaussian7x7u8)(km_handle->cdsp, srcMemHandle.fd, 0, srcMemHandle.size, srcWidth,
                                           srcHeight, srcStride, dstMemHandle.fd, 0, dstMemHandle.size, dstStride,
                                           heapHandle.fd, 0, heapHandle.size, gaussian_static);
    }
    unsigned long long t2 = GetTime();
    printf("%s: return value from Gaussian %d (0x%x)\n", __func__, nErr, nErr);
    if (nErr) {
        ALOGE("%s: Failed to send data buffers to securePD with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    printf("%s: Gaussian run-time: %llu microseconds for %d iteration(s)\n", __func__, t2-t1, LOOPS);

    //Call TA to validate result
    //Please be aware that this is for demo only, we should not trust any data on HLOS side
    nErr = send_sec_dsp_cmd_to_ta(SEC_MATCH_RESULT, dstMemHandle.fd, dstMemHandle.size, 0, 0, DATA, &state);
    if (nErr)  {
        ALOGE("%s: send_sec_cmd_to_ta for SEC_MATCH_RESULT failed with nErr:%d\n", __func__, nErr);
        goto bail;
    }
    printf("%s: matched DST and REF Gaussian contents using TA\n", __func__);

    if (state != PASS) {
        nErr = INVALID_STATE;
        ALOGE("%s: Test failed as state is not PASS, state:%lu nErr:%d\n", __func__, state, nErr);
        printf("%s: Test failed as state is not PASS, state:%lu nErr:%d\n", __func__, state, nErr);
    }
    nErr = send_sec_dsp_cmd_to_ta(DEREGISTER_BUFFER, srcMemHandle.fd, srcMemHandle.size, 0, 0, DATA, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to deregister secure buffer fd:%d  with nErr:%d state:0x%llx\n", __func__, srcMemHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: deregistered SRC buffer from TA\n", __func__);

    nErr = send_sec_dsp_cmd_to_ta(DEREGISTER_BUFFER, dstMemHandle.fd, dstMemHandle.size, 0, 0, DATA, &state);
    if (nErr || state) {
        ALOGE("%s: Failed to deregister secure buffer fd:%d  with nErr:%d state:0x%llx\n", __func__, dstMemHandle.fd, nErr, state);
        nErr = nErr || state;
        goto bail;
    }
    printf("%s: deregistered DST buffer from TA\n", __func__);
bail:
    if (km_handle) {
        if((*km_handle->QSEECom_set_bandwidth)(km_handle->qseecom, false)) {
            ALOGE("%s: aes enc command: (unable to disable clks)\n", __func__);
        }
    }
    if (srcMemHandle.fd) {
       secure_mem_free(&srcMemHandle);
    }
    if (dstMemHandle.fd) {
       secure_mem_free(&dstMemHandle);
    }
    ALOGD("%s: algo returning with nErr:%d\n", __func__, nErr);
    return nErr;
}

/**
  * @brief: Restart the app in case of SSR.
  * @param algo_ret_val: Return value from QDSP.
  * @retval: Returns TRUE in case of SSR, otherwise FALSE.
  */
static bool retry_sequence_after_ssr(int algo_ret_val) {
    bool restart = false;

    switch (algo_ret_val) {
        case AEE_ECONNRESET:
            restart = true;
            km_proxy_close();
            printf("%s: CDSP is down, waiting for 5 seconds before retrial\n", __func__);
            sleep(5);
            break;
        case AEE_SUCCESS:
            restart = false;
            printf("%s: SecurePD test Success\n", __func__);
            break;
        default:
            restart = false;
            printf("%s: SecurePD test failed with retval 0x%x\n", __func__, algo_ret_val);
            break;
    }
    return restart;
}

/**
  * @brief: Restart the app based on user prompt.
  * @retval: Returns TRUE in case user enters 'Y' or 'y', otherwise FASLE.
  */
static bool retry_sequence_after_user_request() {
    bool restart = false;
    char c;

    fflush(stdout);
    printf("%s: Do you want to rerun the example? (Y/N)\n", __func__);
    fflush(stdout);
    c = getchar();
    getchar(); //For capturing enter

    if (c == 'Y' || c == 'y') {
        km_proxy_close();
        restart = true;
        printf("%s: HLOS app restarting %d\n", __func__, restart);
        ALOGD("%s: HLOS app restarting %d\n", __func__, restart);
    }
    return restart;
}

/**
  *@brief: Entry point for the HLOS client app.
  */
int main(int argc, char *argv[])
{
    int command = 0;
    int retVal = 0;
    int nErr = 0;
    bool restart = false;
    int size = 0;

    if (argc <= 1) {
        example_usage();
        nErr = INVALID_INPUT;
        goto bail;
    }

    while ((command = getopt_long(argc, argv, "d:p:l:r:e:h:s", example_options, NULL)) != -1) {
        switch (command) {
            case 'd':
               size = (int)strtol(optarg, NULL, 16);
               if (size <= 0) {
                   printf("%s: ERROR: Invalid dynamic heap size 0x%x\n", __func__, size);
                   nErr = AEE_EBADPARM;
               } else {
                   dynamic_heap_size = size;
                   printf("%s: dynamic heap size 0x%x\n", __func__, dynamic_heap_size);
               }
               break;
            case 'p':
               size = (int)strtol(optarg, NULL, 16);
               if (size <= 0) {
                   printf("%s: ERROR: Invalid hlos physpool size 0x%x\n", __func__, size);
                   nErr = AEE_EBADPARM;
               } else {
                   hlos_physpool_size = size;
                   printf("%s: hlos physpool size 0x%x\n", __func__, hlos_physpool_size);
               }
               break;
            case 'l':
                while (1) {
                   printf("%s: loading gaussian algo elf\n", __func__);
                   retVal = start_load_algo_elf(optarg);
                   printf("%s: load_algo_elf returned 0x%x (%d)\n", __func__, retVal, retVal);
                   restart = retry_sequence_after_ssr(retVal);
                   if (restart) {
                       continue;
                   }
                   restart = retry_sequence_after_user_request();
                   if (restart) {
                       continue;
                   } else {
                       break;
                   }
                }
                break;
            case 'r':
               while (1) {
                   printf("%s: running gaussian algo elf\n", __func__);
                   retVal = run_gaussian_algo(optarg);
                   printf("%s: run_gaussian_algo returned 0x%x (%d)\n", __func__, retVal, retVal);
                   restart = retry_sequence_after_ssr(retVal);
                   if (restart) {
                       continue;
                   }
                   restart = retry_sequence_after_user_request();
                   if (restart) {
                       continue;
                   } else {
                       break;
                   }
               }
               break;
            case 'e':
                printf("%s: running gaussian algo elf and exiting after run\n", __func__);
                retVal = run_gaussian_algo (optarg);
                printf("%s: run_gaussian_algo returned 0x%x (%d)\n", __func__, retVal, retVal);
                restart = retry_sequence_after_ssr(retVal);
                break;
           case 's':
                printf("%s: running static gaussian\n", __func__);
                retVal = run_gaussian_algo (NULL);
                printf("%s: run_gaussian_algo static returned 0x%x (%d)\n", __func__, retVal, retVal);
                restart = retry_sequence_after_ssr(retVal);
                break;
           case 'h':
                example_usage();
                break;
           default:
                example_usage();
                nErr = INVALID_INPUT;
                break;
        }
    }
bail:
    printf("%s: HLOS securePD test app exiting with nErr 0x%x\n", __func__, nErr);
    return nErr;
}
