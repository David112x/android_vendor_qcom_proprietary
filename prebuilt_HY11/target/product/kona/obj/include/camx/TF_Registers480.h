// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __TF_REGISTERS480_H__
#define __TF_REGISTERS480_H__

#include <stdint.h>


struct IPETFRegCmd480;

// Validates the settings of the registers
// Returns 0 if success
// Else, returns the number of invalid settings
int32_t ValidatePacked_TF20_REG( IPETFRegCmd480* regStruct );
// Set the default values of struct
// This function set the internal struct fields and not the packed register
// Therefore, the function PackREG should be called before using the final value of the registers
void SetDefaultValPacked_TF20_REG( IPETFRegCmd480* regStruct );

#endif //__TF_REGISTERS480_H__

