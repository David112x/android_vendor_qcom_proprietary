{#
/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Content of this file should not be changed! Only extended without modifying existing defs.
 * Otherwise backwards compatibility may be broken. E.g. if structure needs to be changed, create a new one and append this with version - mp_ou_attributes_t -> mp_ou_attributes_v2_t
 *
 */
 #}

#ifndef RTIC_MP_HEADER_DYNAMIC_FEATURES_H_
#define RTIC_MP_HEADER_DYNAMIC_FEATURES_H_

#define RTIC_MPGEN_INTERFACE_VERSION_MINOR	(/*!!!*/ 3 /*!!!*/) /* Should be incremented every time this file has changed in a minor way */

/**
 * OEM Unlock attributes
 */
typedef struct {
	unsigned long long int const attributes; // control attributes e.g. MP_SECTION_ATTRIBUTE_NRTIC
}__attribute__ ((packed)) mp_ou_attributes_t;

/**
 * QHEE Report attributes
 */
typedef struct {
	unsigned long long int const attributes; // control attributes e.g. MP_SECTION_ATTRIBUTE_NRTIC
}__attribute__ ((packed)) mp_qr_attributes_t;

/**
 * Kernel task_struct offsets (set to -1 if unusable)
 */
typedef struct {
	int const offset_state; // offset to state field in task_struct
	int const offset_pid; // offset to pid field in task_struct
	int const offset_parent; // offset to parent field in task_struct
	int const offset_comm; // offset to comm field in task_struct
}__attribute__ ((__packed__)) mp_kernel_task_struct_offsets_t;

/**
 * The content of the DTS payload used in RTIC DTB.
 */
typedef struct {
	unsigned long long int mp_va_addr; /* MP data VA adress */
	unsigned long long int mp_offset; /* offset of MP data from .head.text */
	unsigned int mp_size; /* size of MP data */
	unsigned char sha256[32]; /* hash */
} __attribute__((__packed__)) dts_payload_t;

#endif /* RTIC_MP_HEADER_DYNAMIC_FEATURES_H_ */
