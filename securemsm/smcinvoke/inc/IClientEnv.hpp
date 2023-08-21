// =======================================================================
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
// =======================================================================
#pragma once
// AUTOGENERATED FILE: DO NOT EDIT

#include <cstdint>
#include "object.h"
#include "proxy_base.hpp"
#include "IIO.hpp"

class IIClientEnv {
   public:
    virtual ~IIClientEnv() {}

    virtual int32_t open(uint32_t uid_val, ProxyBase &obj_ref) = 0;
    virtual int32_t registerLegacy(const void* credentials_ptr, size_t credentials_len, ProxyBase &clientEnv_ref) = 0;
    virtual int32_t registerAsClient(const ProxyBase &credentials_ref, ProxyBase &clientEnv_ref) = 0;
    virtual int32_t registerWithWhitelist(const ProxyBase &credentials_ref, const uint32_t* uids_ptr, size_t uids_len, ProxyBase &clientEnv_ref) = 0;

   protected:
    static const ObjectOp OP_open = 0;
    static const ObjectOp OP_registerLegacy = 1;
    static const ObjectOp OP_registerAsClient = 2;
    static const ObjectOp OP_registerWithWhitelist = 3;
};

class IClientEnv : public IIClientEnv, public ProxyBase {
   public:
    IClientEnv() {}
    IClientEnv(Object impl) : ProxyBase(impl) {}
    virtual ~IClientEnv() {}

    virtual int32_t open(uint32_t uid_val, ProxyBase &obj_ref) {
        ObjectArg a[2];
        a[0].b = (ObjectBuf) {&uid_val, sizeof(uint32_t)};

        int32_t result = invoke(OP_open, a, ObjectCounts_pack(1, 0, 0, 1));
        if (Object_OK != result) { return result; }

        obj_ref.consume(a[1].o);

        return result;
    }

    virtual int32_t registerLegacy(const void* credentials_ptr, size_t credentials_len, ProxyBase &clientEnv_ref) {
        ObjectArg a[2];
        a[0].bi = (ObjectBufIn) {credentials_ptr, credentials_len * 1};

        int32_t result = invoke(OP_registerLegacy, a, ObjectCounts_pack(1, 0, 0, 1));
        if (Object_OK != result) { return result; }

        clientEnv_ref.consume(a[1].o);

        return result;
    }

    virtual int32_t registerAsClient(const ProxyBase &credentials_ref, ProxyBase &clientEnv_ref) {
        ObjectArg a[2];
        a[0].o = credentials_ref.get();

        int32_t result = invoke(OP_registerAsClient, a, ObjectCounts_pack(0, 0, 1, 1));
        if (Object_OK != result) { return result; }

        clientEnv_ref.consume(a[1].o);

        return result;
    }

    virtual int32_t registerWithWhitelist(const ProxyBase &credentials_ref, const uint32_t* uids_ptr, size_t uids_len, ProxyBase &clientEnv_ref) {
        ObjectArg a[3];
        a[1].o = credentials_ref.get();
        a[0].bi = (ObjectBufIn) {uids_ptr, uids_len * sizeof(uint32_t)};

        int32_t result = invoke(OP_registerWithWhitelist, a, ObjectCounts_pack(1, 0, 1, 1));
        if (Object_OK != result) { return result; }

        clientEnv_ref.consume(a[2].o);

        return result;
    }

};

