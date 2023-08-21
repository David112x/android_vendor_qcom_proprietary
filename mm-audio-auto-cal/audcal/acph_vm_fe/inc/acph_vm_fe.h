#ifndef _AUDCAL_ACPH_VM_FE_H_
#define _AUDCAL_ACPH_VM_FE_H_
/*===========================================================================
    @file   acph_vm_fe.h

    The interface of acph vm frontend module to be export to acph module.

    This file will provide interface of acph vm frontend module to acph module.

                    Copyright (c) 2017 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */
/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2017-08-29  ct      Enable ACPH VM Frontend for GVM Calibration.
===========================================================================*/

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */
#define ACPH_VM_SUCCESS 0
#define ACPH_VM_FAILURE -1

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */
typedef void (*pfn_Callback)(uint8_t*, uint32_t, uint8_t**, uint32_t*);

int32_t acph_vm_fe_init (pfn_Callback callback);

int32_t acph_vm_fe_deinit ( void );

#endif //_AUDCAL_ACPH_VM_FE_H_

