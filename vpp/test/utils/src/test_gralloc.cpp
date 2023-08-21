/*!
 * @file test_gralloc.cpp
 *
 * @cr
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <errno.h>

#include <gr_priv_handle.h>
#include "vpp_dbg.h"
#include "vpp.h"
#include "test_gralloc.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/

/************************************************************************
 * Global Functions
 ***********************************************************************/
void* vpVppTestGralloc_Alloc(int fdPx, int fdMeta)
{
    struct private_handle_t *pstHdl;

    pstHdl = new private_handle_t(fdPx, fdMeta, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!pstHdl)
    {
        LOGE("Cannot allocate memory for private_handle_t");
        return NULL;
    }

    return pstHdl;
}

uint32_t u32VppTestGralloc_Free(void *pvGralloc)
{
    VPP_RET_IF_NULL(pvGralloc, VPP_ERR_PARAM);

    if (pvGralloc)
    {
        delete((struct private_handle_t *)(pvGralloc));
    }

    return VPP_OK;
}
