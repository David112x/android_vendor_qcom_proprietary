#ifndef _WIFIDISPLAY_CLIENT_H_
#define _WIFIDISPLAY_CLIENT_H_
/*==============================================================================
*       WiFiDisplayClient.h
*
*  DESCRIPTION:
*       Class declaration WiFiDisplayClient
*
*
*  Copyright (c) 2014-2017, 2019 Qualcomm Technologies, Inc.
*  All Rights Reserved.
*  Confidential and Proprietary - Qualcomm Technologies, Inc.
*===============================================================================
*/

#include <sys/types.h>
#include <jni.h>
#include <vector>

struct WfdDevice;
struct WFD_uibc_event_t;
struct uibc_Fields;
struct ANativeWindow;

enum HIDDataType : int;

typedef void (*stringarray_cb)(const char* eName,
    int numObjects, char strArray[][256],const jobject&);

typedef bool (*uibc_send_event_cb)(WFD_uibc_event_t* const,
    const void * const pClientData, const jobject&, const jobject&);

typedef bool (*uibc_hid_event_cb)(const uint8_t* const HIDPacket, uint8_t packetLen,
    const HIDDataType& dataType,const jobject&);

namespace android {
    class InputEvent;
    class KeyEvent;
}

class WiFiDisplayClient
{
public:

    typedef struct screenDimensions{
        uint16_t height;
        uint16_t width;
        uint16_t Orientation;
        float aspectRatio;
        char* dump();
        screenDimensions() = default;

    private:
        static const int NUM_DIG_UINT16_t = 5;
        char dumpStr[4*NUM_DIG_UINT16_t + 30];
    } screenDimensions;

    static WiFiDisplayClient* create(bool sink,
                               stringarray_cb,uibc_send_event_cb,
                               uibc_hid_event_cb);

    static WiFiDisplayClient* create(bool sink,
                               stringarray_cb,uibc_send_event_cb,
                               uibc_hid_event_cb,JNIEnv*,const jobject&);

    virtual ~WiFiDisplayClient() {}

    virtual int createWfdSession (WfdDevice *localDevice, WfdDevice *peerDevice) = 0;

    virtual int stopWfdSession (int sessId, JNIEnv* env) = 0;

    virtual int play_rtsp (int sessId, bool secureFlag) = 0;

    virtual int pause_rtsp (int sessId, bool secureFlag) = 0;

    virtual int standby_rtsp(int sessId) = 0;

    virtual int teardown_rtsp(int sessId, bool isRTSP) = 0;

    virtual int setRtpTransport(int transportType) = 0;

    virtual int queryTCPTransportSupport() = 0;

    virtual int setDecoderLatency(int latency) = 0;

    virtual int tcpPlaybackControl(int cmdType, int cmdVal) = 0;

    virtual int negotiateRtpTransport(int TransportType,int BufferLenMs,
                                      int portNum) = 0;

    virtual int executeRuntimeCommand(int cmd) = 0;

    virtual int setSurface(ANativeWindow* surface) = 0;

    virtual int getConfigItems(std::vector<std::string> &configItems) = 0;

    virtual uibc_Fields* getUIBCField () const = 0;

    virtual int setUIBC(int sessId) = 0;

    virtual int enableUIBC(int sessId) = 0;

    virtual int disableUIBC(int sessId) = 0;

    virtual int startUIBC() = 0;//For clients oblivious of JNI

    virtual int startUIBC(JNIEnv*, jfieldID, const jobject&, const jobject&) = 0;

    virtual int stopUIBC() = 0;

    virtual int sendUIBCEvent(android::InputEvent* ie, WFD_uibc_event_t* ev) = 0;

    virtual int setAVPlaybackMode(int mode) = 0;

    //! @param codec    - 0 for H.264, 1 for H.265
    //! @param profile  - H.264 0-CBP 1-RHP 2-RHP2 3-BP 4-MP 5-HP / H.265 - 0 Main
    //! @param level    - 0 - level 3.1, 1- 3.2, 2 -4, 3 -4.1, 4 - 4.2, 5 - 5.0, 6 - 5.1, 7 - 5.2
    //! @param formatType - 3 - CEA, 4 - VESA, 5 - HH
    //! @param value    - 0x1, 0x2, 0x4 and so on
    //! @param resParams - {width, height, framerate}
    //! @param len - length of resParams
    //!
    //! @return int
    virtual int sendAvFormatChange(int codec, int profile, int level,
                                   int formatType, int value, int* resParams, int len) = 0;

    virtual int setBitrate(int value) = 0;

    virtual int setResolution (int formatType, int value, int* resParams, int len) = 0;

    virtual int getResolution(int32_t* width, int32_t* height) = 0;

    virtual int getCommonResolution(uint32_t** bitmap, int32_t* numProf) = 0;

    virtual int getNegotiatedResolutionBitmap(std::vector<uint64_t>& bitmap) = 0;

    virtual int getUIBCSupportCategory() = 0;

    virtual void setSurfaceProp(const jint*, jsize, const jint*, jsize) = 0;

    virtual void sendIDRRequest() = 0;

    virtual const screenDimensions& screenDims() const = 0;
};

#endif // _WIFIDISPLAY_CLIENT_H_
