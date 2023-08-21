////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chibinarylog.h
/// @brief Binary logging related definitions and prototypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIBINARYLOG_H
#define CHIBINARYLOG_H

// NOWHINE FILE PR008: Custom compiler provided library next to its type declarations within a scoped preprocessor guard.
// NOWHINE FILE CP011: namespace is used to hide implementations for templated functions that need to be in a header file
// NOWHINE FILE PR007b: standard library (cstring) needed for memcpy.
// NOWHINE FILE CP007: Templates needed for the feature.
// NOWHINE FILE NC009: Its a chi file.

#include <cstring>

#if defined (_LINUX)
#include "cdkutils.h"
#endif
#include "camxchiofflinelogger.h"

// The Events we will be interested in logging
enum class LogEvent
{
    HAL3_Open,
    HAL3_ConfigSetup,
    HAL3_StreamInfo,
    HAL3_ProcessCaptureRequest,
    HAL3_ProcessCaptureResult,
    HAL3_BufferInfo,
    HAL3_FlushInfo,
    HAL3_Notify,
    FT2_Base_SubmitSessionRequest,
    FT2_Base_ProcessMessage,
    FT2_Base_ProcessResult,
    FT2_Base_SubmitRequest,
    FT2_FRO_Init,
    FT2_FRO_StateInfo,
    FT2_FRO_RequestInfo,
    FT2_FRO_ProcessSequenceInfo,
    FT2_FRO_PortDescriptorInfo,
    FT2_FRO_BufferInfo,
    FT2_FRO_OutputNotifiedPortInfo,
    FT2_FRO_ReleaseAckPortInfo,
    FT2_FRO_OutputReleaseInfo,
    FT2_FRO_Destroy,
    FT2_Graph_Init,
    FT2_Graph_FeatureInit,
    FT2_Graph_UpstreamFeatureRequest,
    FT2_Graph_ProcessMessage,
    FT2_Graph_ResultMetadataInfo,
    FT2_Graph_ExtSinkWalk,
    FT2_URO_StateInfo,
    FT2_URO_Init,
    ReqMap_AppFrameNum_to_CHXFrameNum,
    ReqMap_CHXFrameNum_to_pURO,
    ReqMap_pURO_to_pFRO,
    ReqMap_pFRO_to_CamX,
    ReqMap_CamXInfo,
    FenceCB_UnkownNode,
    FenceCB_Processed,
    Node_Initialize,
    Node_ProcessRequest,
    Node_EarlyMetadataDone,
    Node_PartialMetadataDone,
    Node_MetadataDone,
    Node_RequestIdDone,
    Node_SetupRequestOutputPort,
    Node_SetupRequestInputPort,
    CamXChi_CreateFence,
    Session_ProcessResult,
    Pipeline_PartialMetadataDone,
    Pipeline_MetadataDone,
    Pipeline_RequestIdDone,
    FT2_Pool_CreateInstance,
    ChiFenceCB_Processed,
    Pipeline_Initialize
};

// get some tools to work around the compiler intrinsic limitation (C-Style variadic arguments limitations)
#if (__snapdragon_llvm__) && (__clang_major__ >= 8)
#include <qc_binary_log.h>
#define BINARY_LOG_COMPILING_WITH_SDLLVM
#define BINARY_LOG_TYPE_WRAPPER       __binary_log_struct
#define BINARY_LOG_WRAP_OBJECT(obj)   __binary_log_record(&obj)
#define BINARY_LOG_DUMP_PREAMBLE      __builtin_binary_log
// We are not compiling with SDLLVM, we need to provide our own version of qc_binary_log.h
#else

namespace details_binarylog {

    // An object wrapper to wrap non-trivial types (such as ones with a vtable) into a simple used defined type.
    template <typename T>
    struct TypeWrapper { T* x; };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WrapObject
    ///
    /// @brief  Return an instance of the pointer wrapper in a trivial type wrapper.
    ///
    /// @tparam T   The type that is wrapped in BINARY_LOG_TYPE_WRAPPER.
    ///
    /// @param ptr  The location of the wrapper object in memory.
    ///
    /// @return TypeWrapper<T> instance pointing to the object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    TypeWrapper<T> WrapObject(T* ptr) {
        TypeWrapper<T> wrapped;
        wrapped.x = ptr;
        return wrapped;
    };
}

#define BINARY_LOG_TYPE_WRAPPER     TypeWrapper
#define BINARY_LOG_WRAP_OBJECT(obj) WrapObject(&obj)
#define BINARY_LOG_DUMP_PREAMBLE(...)

#endif // __snapdragon_llvm__

namespace details_binarylog {

    // Max size a binary log buffer can be in bytes.
    constexpr std::size_t MaxBinLogMsgSize = 256000;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyMem
    ///
    /// @brief  Perform a memcpy of the object wrapped in a BINARY_LOG_TYPE_WRAPPER<T>.
    ///
    /// @tparam T   The type that is wrapped in BINARY_LOG_TYPE_WRAPPER.
    ///
    /// @param out  The buffer we want to write the argument h to.
    /// @param h    The argument we want to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr void CopyMem(CHAR* out, const BINARY_LOG_TYPE_WRAPPER<T>& h) {
        memcpy(out, h.x, sizeof(T));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyMem
    ///
    /// @brief  Perform a memcpy of the type given, applies to all types not wrapped in BINARY_LOG_TYPE_WRAPPER<T>.
    ///
    /// @tparam T   The type that we are writing to the stack buffer.
    ///
    /// @param out  The buffer we want to write the argument h to.
    /// @param h    The argument we want to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr void CopyMem(CHAR* out, const T& h) {
        memcpy(out, &h, sizeof(T));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump Type
    ///
    /// @brief Given a type T, dump T with the BinaryLog__IGNORE event so the parsing scripts will know you are not declaring
    ///        an actual event
    ///
    /// @tparam T   The type that we are trying to dump a type descriptor for
    ///
    /// @param  w   The wrapper we want to wrap the type with
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr void DumpType(BINARY_LOG_TYPE_WRAPPER<T>&& t) {
#ifdef BINARY_LOG_COMPILING_WITH_SDLLVM
        enum CAMX_IGNORE_ENUM { BinaryLog__IGNORE };
        auto& x = t->x;
        BINARY_LOG_DUMP_PREAMBLE(CAMX_IGNORE_ENUM::BinaryLog__IGNORE, BINARY_LOG_WRAP_OBJECT(&x));
#else
        CAMX_UNREFERENCED_PARAM(t);
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump Type
    ///
    /// @brief Given a type T, dump T with the BinaryLog__IGNORE event so the parsing scripts will know you are not declaring
    ///        an actual event
    ///
    /// @tparam T   The type that we are trying to dump a type descriptor for
    ///
    /// @param  w   The object to use for type deduction
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr void DumpType(T&& x) {
#ifdef BINARY_LOG_COMPILING_WITH_SDLLVM
        enum CAMX_IGNORE_ENUM { BinaryLog__IGNORE };
        BINARY_LOG_DUMP_PREAMBLE(CAMX_IGNORE_ENUM::BinaryLog__IGNORE, x);
#else
        CAMX_UNREFERENCED_PARAM(x);
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExtractBaseType
    ///
    /// @brief Identity function to return the object it was passed. Used as a base case for the type recursion used to remove
    ///        the arrays from the type.
    ///
    /// @tparam T   The type of obj
    ///
    /// @param  obj The object we want to return
    ///
    /// @return obj
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr auto ExtractBaseType(T obj) {
        return obj;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExtractBaseType
    ///
    /// @brief Given a T*, this will return a T. Helpful in cases of array to ptr decay.
    ///
    /// Given T = int[1][2], f = ExtractBaseType.   f(T) = int
    ///
    /// @tparam T    The type that pObj points to
    ///
    /// @param  pObj The object we want to return
    ///
    /// @return obj
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr auto ExtractBaseType(T* pArr) {
        return ExtractBaseType(*pArr);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpArray
    ///
    /// @brief  Tell the compiler to dump out the type descriptor for the elements of the array as well
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T, int N>
    constexpr void DumpArray(T(&arr)[N]) {
        using TT = decltype(ExtractBaseType(arr));
        TT* pDummy = nullptr;
        DumpType(*pDummy);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpArray
    ///
    /// @brief We have no arrays here so we dont need to do anything special
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr void DumpArray(T&& a) {
        CAMX_UNREFERENCED_PARAM(a);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogSize
    ///
    /// @brief  Perform a sizeof of the type given, applies to all types not wrapped in BINARY_LOG_TYPE_WRAPPER<T>.
    ///
    /// @tparam T   The type that we are getting the size for.
    ///
    /// @return size of the type T.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct LogSize {
        static constexpr auto value = sizeof(T);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogSize
    ///
    /// @brief  Perform a sizeof of the type wrapped in BINARY_LOG_TYPE_WRAPPER<T>.
    ///
    /// @tparam T   The type that we are getting the size for.
    ///
    /// @return size of the type wrapped in a binary log struct.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct LogSize<BINARY_LOG_TYPE_WRAPPER<T> > {
        static constexpr auto value = sizeof(T);
    };

    // Compiler error message shown when the max size is exceeded
#define MAX_BIN_LOG_MSG_SIZE_EXCEEDED_ERR "Maximum size of a log message (including metadata) is 256 Kilobytes"

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TotalSize
    ///
    /// @brief  Calculates the size of a list of types.
    ///
    /// @tparam Head    The type that we are reading the size of currently.
    /// @tparam Tail    A paramater pack containing the rest of arguments we want to get the size for.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename Head, typename ...Tail>
    struct TotalSize {
        static constexpr std::size_t value = LogSize<Head>::value + TotalSize<Tail...>::value;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TotalSize
    ///
    /// @brief  Calculates the size of a type.
    ///
    /// @tparam Head    The type that we are reading the size of currently.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename Head>
    struct TotalSize<Head> {
        static constexpr std::size_t value = LogSize<Head>::value;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBuffer
    ///
    /// @brief  Adds an element to the stack allocated buffer in WriteLog with the proper offset.
    ///
    /// @tparam Head    The type that we are writing to the stack buffer.
    ///
    /// @param out  The buffer we want to write the argument h to.
    /// @param h    The argument we want to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename Head>
    constexpr void FillBuffer(CHAR* out, Head&& h) {
        DumpArray(std::forward<Head>(h));
        CopyMem(out, std::forward<Head>(h));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBuffer
    ///
    /// @brief  Adds an element to the stack allocated buffer in WriteLog with the proper offset and calculates the offset for
    ///         the next one.
    ///
    /// @tparam Head    The type that we are writing to the stack buffer.
    /// @tparam Tail    The rest of the types we want to log.
    ///
    /// @param out  The buffer we want to write the argument h to.
    /// @param h    The argument we want to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename Head, typename ...Tail>
    constexpr void FillBuffer(CHAR* out, Head&& h, Tail&&... args)
    {
        DumpArray(std::forward<Head>(h));
        CopyMem(out, std::forward<Head>(h));
        FillBuffer(out + LogSize<Head>::value, args...);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteLog
    ///
    /// @brief Stack allocate a buffer of an approperiate size, fill it with the arguments passed, then pass it to the logging
    /// infrastructure.
    ///
    /// @tparam Args  A parameter pack of the types we want to log.
    ///
    /// @param args   The argument we want to log.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename ...Args>
    constexpr void WriteLog(Args&&... args)
    {
        constexpr auto logMessageSize = TotalSize<Args...>::value;
        constexpr auto logBufferSize = logMessageSize + LogSize<std::size_t>::value;
        static_assert(logBufferSize < MaxBinLogMsgSize, MAX_BIN_LOG_MSG_SIZE_EXCEEDED_ERR);
        CHAR buffer[logBufferSize];
        FillBuffer(buffer, logMessageSize, std::forward<Args>(args)...);
        CamX::OfflineLogger::GetInstance(OfflineLoggerType::BINARY)->AddLog(buffer, logBufferSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BinaryLog Logs a message to the binary logging framework
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined (_LINUX)
#define BINARY_LOG(evt, ...) \
{   \
    using namespace details_binarylog; \
    auto timestamp_nanoseconds = CdkUtils::GetNanosecondsSinceEpoch(); \
    auto threadID              = CdkUtils::GetThreadID(); \
    WriteLog(static_cast<UINT16>(evt), timestamp_nanoseconds, threadID, __VA_ARGS__); \
    BINARY_LOG_DUMP_PREAMBLE(evt, timestamp_nanoseconds, threadID, __VA_ARGS__); \
}
#else
#define BINARY_LOG(evt, ...) \
{} // Do nothing for windows
#endif

// This is not needed outside of the details logic above.
#undef BINARY_LOG_TYPE_WRAPPER

#endif // CHIXBINARYLOG_H
