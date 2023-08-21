/*-------------------------------------------------------------------
Copyright (c) 2014 - 2019 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
--------------------------------------------------------------------------*/

#include "vt_debug.h"
#include "vt_ion_allocator.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/msm_ion.h>

#undef LOG_TAG
#define LOG_TAG "VTEST_ION_ALLOCATOR"

#define SZ_4K 0x1000
#define SZ_1M 0x100000

#ifdef SLAVE_SIDE_CP
#define CP_HEAP_ID ION_SECURE_DISPLAY_HEAP_ID
#define SECURE_ALIGN SZ_1M
#else // MASTER_SIDE_CP
#define CP_HEAP_ID ION_SECURE_HEAP_ID
#define SECURE_ALIGN SZ_4K
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int vt_ion_allocate(int secure,
        void **client_ion_buffer, size_t size, int flags) {

    int rc = 0;
    struct ion_buffer_data *ion_buffer;
    *client_ion_buffer = NULL;

    if (size == 0) {
        VTEST_MSG_ERROR("buffer size passed : %zu invalid", size);
        return -EINVAL;
    }

    ion_buffer = calloc(sizeof(struct ion_buffer_data), 1);
    if (!ion_buffer) {
        VTEST_MSG_ERROR("ion_buffer structure allocation failed");
        return -ENOMEM;
    }

    ion_buffer->dev_fd = ion_open();
    if (ion_buffer->dev_fd < 0) {
        VTEST_MSG_ERROR("opening ion device failed");
        return -ENODEV;
    }

    ion_buffer->alloc_data.len = size;
    ion_buffer->alloc_data.flags = flags;
    if (secure) {
        ion_buffer->alloc_data.heap_id_mask = ION_HEAP(CP_HEAP_ID);
    } else {
        ion_buffer->alloc_data.heap_id_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
    }

    rc = ion_alloc_fd(ion_buffer->dev_fd, size, 0,
                      ion_buffer->alloc_data.heap_id_mask, flags, &ion_buffer->data_fd);
    if (rc) {
        VTEST_MSG_ERROR("Failed to allocate ion memory");
        ion_buffer->data_fd = -1;
        vt_ion_free((void *)ion_buffer);
        return -ENOMEM;
    }

    if (!secure) {
        ion_buffer->data = mmap(NULL, size,
                     PROT_READ | PROT_WRITE, MAP_SHARED, ion_buffer->data_fd, 0);
        if (ion_buffer->data == MAP_FAILED) {
            VTEST_MSG_ERROR("Failed to mmap ion memory, errno is %s", strerror(errno));
            ion_buffer->data = NULL;
            vt_ion_free((void *)ion_buffer);
            return -EINVAL;
        }
    }
    *client_ion_buffer = (void *)ion_buffer;
    return rc;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int vt_ion_free(void* client_ion_buffer) {

    int rc = 0;
    struct ion_buffer_data *ion_buffer = (struct ion_buffer_data *)client_ion_buffer;
    int nIonFd = -1;

    if (!ion_buffer) {
        VTEST_MSG_ERROR("client_ion_buffer is NULL");
        return -EINVAL;
    }

    if (ion_buffer->data) {
        rc = munmap(ion_buffer->data, ion_buffer->alloc_data.len);
        if (rc) {
            VTEST_MSG_ERROR("munmap failed for fd = %d", ion_buffer->data_fd);
        }
    }

    if (ion_buffer->dev_fd >= 0) {
        ion_close(ion_buffer->dev_fd);
        ion_buffer->dev_fd = -1;
    }
    free(ion_buffer);
    return rc;
}
