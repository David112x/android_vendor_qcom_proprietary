/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef KEYMASTER_4_0_KEYMASTERHALDEVICEEXT_H_
#define KEYMASTER_4_0_KEYMASTERHALDEVICEEXT_H_

#include <KeymasterHalDevice.h>

namespace keymaster {
static const keymaster_key_format_t KM_KEY_FORMAT_SHARED
                                    = static_cast<keymaster_key_format_t>(7);
static const keymaster_tag_t KM_TAG_SHARED_KEY
                                    = static_cast<keymaster_tag_t>(KM_BOOL | 16301);
static const keymaster_tag_t KM_TAG_SHARED_VM1
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16302);
static const keymaster_tag_t KM_TAG_SHARED_VM2
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16303);
static const keymaster_tag_t KM_TAG_SHARED_VM3
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16304);
static const keymaster_tag_t KM_TAG_SHARED_VM4
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16305);
static const keymaster_tag_t KM_TAG_SHARED_VM5
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16306);
static const keymaster_tag_t KM_TAG_SHARED_VM6
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16307);
static const keymaster_tag_t KM_TAG_SHARED_VM7
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16308);
static const keymaster_tag_t KM_TAG_SHARED_VM8
                                    = static_cast<keymaster_tag_t>(KM_UINT | 16309);
static const keymaster_tag_t KM_TAG_PURPOSE_VM1
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16311);
static const keymaster_tag_t KM_TAG_PURPOSE_VM2
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16312);
static const keymaster_tag_t KM_TAG_PURPOSE_VM3
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16313);
static const keymaster_tag_t KM_TAG_PURPOSE_VM4
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16314);
static const keymaster_tag_t KM_TAG_PURPOSE_VM5
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16315);
static const keymaster_tag_t KM_TAG_PURPOSE_VM6
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16316);
static const keymaster_tag_t KM_TAG_PURPOSE_VM7
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16317);
static const keymaster_tag_t KM_TAG_PURPOSE_VM8
                                    = static_cast<keymaster_tag_t>(KM_ENUM_REP | 16318);

DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_SHARED_KEY);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM1);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM2);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM3);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM4);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM5);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM6);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM7);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_SHARED_VM8);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM1);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM2);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM3);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM4);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM5);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM6);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM7);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_PURPOSE_VM8);

} // namespace keymaster

namespace keymasterdevice {
class KeymasterHalDeviceExt : public KeymasterHalDevice {
    public:
        KeymasterHalDeviceExt(keymaster_security_level_t security_level) :
                                   KeymasterHalDevice(security_level) {}
        virtual ~KeymasterHalDeviceExt() {}

        keymaster_error_t delete_key_ext(const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params);
        keymaster_error_t delete_shared_key(const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params);
        keymaster_error_t delete_all_shared_keys(void);

    };

}

#endif /* KEYMASTER_4_0_KEYMASTERHALDEVICEEXT_H_ */

