/*
* Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef __HDR_INTERFACE_H__
#define __HDR_INTERFACE_H__

#include <string>
#include <stdint.h>
#include <bitset>
#include <memory>
#include <tuple>
#include <vector>
#include <array>
#include <functional>
#include <color_metadata.h>
#include <display_color_processing.h>

// To preserve precision of luminance values.
#define LUMINANCE_FACTOR 10000.0f

namespace sdm {
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::pair;
using std::tuple;
using std::make_tuple;
using std::array;

struct HdrIntfPropertyPayload;

enum HdrOps {
    kProcessLayers = 1,
    kOpsMax = 0xffff
};

class HdrInterface {
protected:
    virtual ~HdrInterface() {}
public:
    virtual int Init() = 0;
    virtual int Deinit() = 0;
    virtual int SetParams(const HdrIntfPropertyPayload &payload) = 0;
    virtual int GetParams(HdrIntfPropertyPayload *payload) = 0;
    virtual int CallOps(HdrOps, const HdrIntfPropertyPayload &input,
                      HdrIntfPropertyPayload *output) = 0;
};

extern "C" HdrInterface* GetHdrInterface();

struct HdrColorInfo {
    ColorPrimaries gamut;
    GammaTransfer gamma;
};

enum HdrToneMapperHwBlock {
    kDspp,
    kVigSspp,
    kGpu,
    kDmaSspp,
    kToneMapperHwMax = 0xffff,
};

enum HdrToneMapAlgorithm {
    kTetraHedral10bit,
    kTetraHedral12bit,
    kTrilinear,
    k1d3d1d,
    kAlgoMax = 0x3f
};

struct HdrToneMapSupport {
    HdrColorInfo src;
    HdrColorInfo dest;
    HdrToneMapperHwBlock block;
    uint64_t block_caps;
};

struct HdrToneMapsCap {
    vector<HdrToneMapSupport> caps;
};

struct HdrPanelinfo {
    Primaries primaries;
    // Client will multiply with LUMINANCE_FACTOR to preserve precision.
    uint32_t panel_brightness;  // unit: cd/m^2. 0 means no data is provided.
    // Client will multiply with LUMINANCE_FACTOR to preserve precision.
    uint32_t panel_blackLevel;  // unit: cd/m^2. 0 means no data is provided.
    ColorPrimaries gamut = ColorPrimaries_DCIP3;
    GammaTransfer gamma = Transfer_Gamma2_2;
};

enum HdrProperty {
    kHdrCapabilityProperty = 0x1,
    kHdrVersionProperty,
    kHdrPanelProperty,
    kHdrBrightnessProperty,
    kHdrBlobProperty,
    kHdrInputParam,
    kHdrOutputParam,
    kHdrNeedsLutupdate,
    kHdrValidDynMetaData,
    kHdrGenerateVsif,
    kHdrGameBlobVersionProperty,
    kHdrGameBlobProperty,
    kHdrDppsIntfProperty,
    kHdrGameVersionProperty,
    kHdrDisableConstantALSProperty,
    kHdrPropertyMax = 0xffff
};

enum ContentType {
    kContentUnknown = 0,
    kContentVideo,
    kContentGraphics,
    kContentGame,
    kContentMax = 0xffff
};

enum HdrLutTypes {
    kTriLinear,
    kTetraHedral12Bit,
    k1DCsc1D,
    kTetraHedral10Bit,
    kLutTypesMax,
};

struct HdrLayerInputParams {
    ContentType type;
    ColorMetaData layer_meta;
    // HW block intended to use for tone-mapping
    HdrToneMapperHwBlock hw_info;
    // LUT that client wants to use for tone-mapping
    // TODO: Currently this field is ignored. Will be used in future
    // Client will call GetParams with kHdrCapabilityProperty, HDR interface
    // will return the combination of HW & LUT that is supported.
    // Client can choose hw_info/lut_type based on the Platform capabilties.
    HdrLutTypes lut_type;
};

struct HdrInputParams {
    ColorPrimaries gamut;
    GammaTransfer gamma;
    vector<shared_ptr<HdrLayerInputParams>> layers;
    void Add(shared_ptr<HdrLayerInputParams> a) {
        layers.push_back(a);
    }
};

struct HdrDmaCfg {
    bool validigc;
    bool validcsc;
    bool validgc;
    int64_t csc_matrix[3][3];
    uint32_t igc_checksum;
    uint32_t gc_checksum;
    lut1d_info gc;
    lut1d_info igc;
};

struct HdrVigCfg {
    bool validigc;
    bool valid3dlut;
    lut1d_info igc;
    lut3d_info tetrahedral;
};

struct HdrLayerOutParams {
    // LUT type returned by Algorithm
    // Currently interface chooses based on hw_info
    // TODO: Check the lut_type in HdrLayerInputParams & select the LUT based
    // on clients request
    HdrLutTypes lut_type;
    uint32_t checksum;
    // HW block intended to use for tone-mapping, will be passed from HdrLayerInputParams
    HdrToneMapperHwBlock hw_info;
    union HdrLuts {
        uint64_t padding;
        Lut3d trilinear;
        lut3d_info tetrahedral;
        HdrDmaCfg dmacfg;
        HdrVigCfg vigcfg;
    } luts;
    // Game configuration, vector size should be zero if implementation is
    // not returning the game cfg.
    std::vector<uint8_t> game_cfg;
};

struct HdrOutputParams {
    vector<shared_ptr<HdrLayerOutParams>> layers;
    void Add(shared_ptr<HdrLayerOutParams> a) {
        layers.push_back(a);
    };
};

struct HdrValidDynMetaData {
public:
    bool valid;
    const ColorMetaData &reference;
    HdrValidDynMetaData(const ColorMetaData &r) : reference(r) {
    };
};

struct HdrVsifData {
public:
    const ColorMetaData &reference;
    HdrVsifData(const ColorMetaData &r) : reference(r) {
    };
    std::vector<uint8_t> vsifdata;
    // Client should set what layers are part of the current composition round.
    // This info is needed to support dynamic metadata for external panels.
    std::vector<ContentType> layer_content_type;
};

struct HdrIntfPropertyPayload {
public:
    HdrIntfPropertyPayload() = default;
    HdrProperty GetPropertyInfo() const {
        return property;
    }

    template <typename T>
    int GetPayloadinfo(T *&output, uint32_t &size) const {
        int ret = -EINVAL;
        output = NULL;
        size = 0;

        if (!payload)
            return ret;

        switch (property) {
        case kHdrPanelProperty:
            if (std::is_same<HdrPanelinfo, T>::value)
                ret = 0;
        break;
        case kHdrGameBlobProperty:
        case kHdrBlobProperty:
            if (std::is_same<char, T>::value)
                ret = 0;
        break;
        case kHdrGameBlobVersionProperty:
        case kHdrGameVersionProperty:
        case kHdrBrightnessProperty:
            if (std::is_same<uint32_t, T>::value)
                ret = 0;
        break;
        case kHdrVersionProperty:
            if (std::is_same<std::string, T>::value &&
                sz == sizeof(std::string))
                ret = 0;
        break;
        case kHdrCapabilityProperty:
        if (std::is_same<HdrToneMapsCap, T>::value &&
                sz == sizeof(HdrToneMapsCap))
            ret = 0;
        break;
        case kHdrInputParam:
            if (std::is_same<HdrInputParams, T>::value &&
                    sz == sizeof(HdrInputParams))
                ret = 0;
        break;
        case kHdrOutputParam:
            if (std::is_same<HdrOutputParams, T>::value &&
                    sz == sizeof(HdrOutputParams))
                ret = 0;
        break;
        case kHdrNeedsLutupdate:
            if (std::is_same<bool, T>::value)
                ret = 0;
        break;
        case kHdrValidDynMetaData:
            if (std::is_same<HdrValidDynMetaData, T>::value)
                ret = 0;
        break;
        case kHdrGenerateVsif:
            if (std::is_same<HdrVsifData, T>::value)
                ret = 0;
        break;
        case kHdrDppsIntfProperty:
            if (std::is_same<uint64_t, T>::value)
                ret = 0;
        break;
        default:
        break;
        }
        if (ret)
            return ret;

        output = reinterpret_cast<T *>(payload);
        size = sz;
        return 0;
    }

    template <typename T, typename Y>
    int CreatePayload(HdrProperty prop, const Y &ref, T *&output) {
        int ret = -EINVAL;
        switch (prop) {
        case kHdrValidDynMetaData:
            if (std::is_same<HdrValidDynMetaData, T>::value
                && std::is_same<ColorMetaData, Y>::value)
                ret = 0;
        break;
        case kHdrGenerateVsif:
            if (std::is_same<HdrVsifData, T>::value &&
                std::is_same<ColorMetaData, Y>::value)
                ret = 0;
        break;
        default:
        break;
        }
        if (ret)
            return ret;
        property = prop;
        output = (new T(ref));
        if (!output)
            return -ENOMEM;
        payload = reinterpret_cast<uint8_t *>(output);
        sz = sizeof(T);
        release = std::function<void(void)>([output]() -> void {delete output;});
        return ret;
    }

    template <typename T>
    int CreatePayload(T *&output, HdrProperty prop, uint32_t sz) {
        int ret = -EINVAL;

        switch (prop) {
        case kHdrGameBlobProperty:
        case kHdrBlobProperty:
        if (std::is_same<char, T>::value)
            ret = 0;
        break;
        default:
        break;
        }
        if (ret)
            return -EINVAL;
        payload = new uint8_t[sizeof(T) * sz]();
        if (!payload) {
            output = NULL;
            return -ENOMEM;
        }
        this->sz = sizeof(T) * sz;
        output = reinterpret_cast<T *>(payload);
        property = prop;
        release = std::function<void(void)>([output]() -> void {delete [] output;});
        return 0;
    }

    template <typename T>
    int CreatePayload(T *&output, HdrProperty prop) {
        int ret = -EINVAL;
        switch (prop) {
        case kHdrInputParam:
            if (std::is_same<HdrInputParams, T>::value)
                ret = 0;
        break;
        case kHdrOutputParam:
            if (std::is_same<HdrOutputParams, T>::value)
                 ret = 0;
        break;
        case kHdrCapabilityProperty:
            if (std::is_same<HdrToneMapsCap, T>::value)
                ret = 0;
        break;
        case kHdrGameBlobVersionProperty:
        case kHdrGameVersionProperty:
        case kHdrBrightnessProperty:
            if (std::is_same<uint32_t, T>::value)
                ret = 0;
         break;
        case kHdrPanelProperty:
            if (std::is_same<HdrPanelinfo, T>::value)
                ret = 0;
        break;
        case kHdrVersionProperty:
            if (std::is_same<std::string, T>::value)
                ret = 0;
         break;
        case kHdrNeedsLutupdate:
        case kHdrDisableConstantALSProperty:
            if (std::is_same<bool, T>::value)
                ret = 0;
        break;
        case kHdrDppsIntfProperty:
            if (std::is_same<uint64_t, T>::value)
                ret = 0;
        break;
        default:
        break;
        }
        if (ret)
            return ret;
        property = prop;
        T *tmp = new T();
        payload = reinterpret_cast<uint8_t *>(tmp);
        if (!payload) {
            output = NULL;
            return -ENOMEM;
        }
        this->sz = sizeof(T);
        output = reinterpret_cast<T *>(payload);
        release = std::function<void(void)>([tmp]() -> void {delete tmp;});
        return ret;
    }

    inline void DestroyPayload() {
        if (payload)
            release();
        payload = NULL;
    }

    ~HdrIntfPropertyPayload() {
        DestroyPayload();
    }

private:
    uint32_t sz = 0;
    uint8_t *payload = NULL;
    HdrProperty property = kHdrPropertyMax;
    std::function<void(void)> release;
};
} // name space sdm
#endif // __HDR_INTERFACE_H__
