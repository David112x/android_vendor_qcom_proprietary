/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc
 */

#ifndef _LOADALGO_STUB_H
#define _LOADALGO_STUB_H
/// @file loadalgo.idl
///
#include "loadalgo.h"
#ifndef _QAIC_ENV_H
#define _QAIC_ENV_H

#ifdef __GNUC__
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#else
#pragma GCC diagnostic ignored "-Wpragmas"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef _ATTRIBUTE_UNUSED

#ifdef _WIN32
#define _ATTRIBUTE_UNUSED
#else
#define _ATTRIBUTE_UNUSED __attribute__ ((unused))
#endif

#endif // _ATTRIBUTE_UNUSED

#ifndef __QAIC_REMOTE
#define __QAIC_REMOTE(ff) ff
#endif //__QAIC_REMOTE

#ifndef __QAIC_HEADER
#define __QAIC_HEADER(ff) ff
#endif //__QAIC_HEADER

#ifndef __QAIC_HEADER_EXPORT
#define __QAIC_HEADER_EXPORT
#endif // __QAIC_HEADER_EXPORT

#ifndef __QAIC_HEADER_ATTRIBUTE
#define __QAIC_HEADER_ATTRIBUTE
#endif // __QAIC_HEADER_ATTRIBUTE

#ifndef __QAIC_IMPL
#define __QAIC_IMPL(ff) ff
#endif //__QAIC_IMPL

#ifndef __QAIC_IMPL_EXPORT
#define __QAIC_IMPL_EXPORT
#endif // __QAIC_IMPL_EXPORT

#ifndef __QAIC_IMPL_ATTRIBUTE
#define __QAIC_IMPL_ATTRIBUTE
#endif // __QAIC_IMPL_ATTRIBUTE

#ifndef __QAIC_STUB
#define __QAIC_STUB(ff) ff
#endif //__QAIC_STUB

#ifndef __QAIC_STUB_EXPORT
#define __QAIC_STUB_EXPORT
#endif // __QAIC_STUB_EXPORT

#ifndef __QAIC_STUB_ATTRIBUTE
#define __QAIC_STUB_ATTRIBUTE
#endif // __QAIC_STUB_ATTRIBUTE

#ifndef __QAIC_SKEL
#define __QAIC_SKEL(ff) ff
#endif //__QAIC_SKEL__

#ifndef __QAIC_SKEL_EXPORT
#define __QAIC_SKEL_EXPORT
#endif // __QAIC_SKEL_EXPORT

#ifndef __QAIC_SKEL_ATTRIBUTE
#define __QAIC_SKEL_ATTRIBUTE
#endif // __QAIC_SKEL_ATTRIBUTE

#ifdef __QAIC_DEBUG__
   #ifndef __QAIC_DBG_PRINTF__
   #include <stdio.h>
   #define __QAIC_DBG_PRINTF__( ee ) do { printf ee ; } while(0)
   #endif
#else
   #define __QAIC_DBG_PRINTF__( ee ) (void)0
#endif


#define _OFFSET(src, sof)  ((void*)(((char*)(src)) + (sof)))

#define _COPY(dst, dof, src, sof, sz)  \
   do {\
         struct __copy { \
            char ar[sz]; \
         };\
         *(struct __copy*)_OFFSET(dst, dof) = *(struct __copy*)_OFFSET(src, sof);\
   } while (0)

#define _COPYIF(dst, dof, src, sof, sz)  \
   do {\
      if(_OFFSET(dst, dof) != _OFFSET(src, sof)) {\
         _COPY(dst, dof, src, sof, sz); \
      } \
   } while (0)

_ATTRIBUTE_UNUSED
static __inline void _qaic_memmove(void* dst, void* src, int size) {
   int i;
   for(i = 0; i < size; ++i) {
      ((char*)dst)[i] = ((char*)src)[i];
   }
}

#define _MEMMOVEIF(dst, src, sz)  \
   do {\
      if(dst != src) {\
         _qaic_memmove(dst, src, sz);\
      } \
   } while (0)


#define _ASSIGN(dst, src, sof)  \
   do {\
      dst = OFFSET(src, sof); \
   } while (0)

#define _STD_STRLEN_IF(str) (str == 0 ? 0 : strlen(str))

#include "AEEStdErr.h"

#define _TRY(ee, func) \
   do { \
      if (AEE_SUCCESS != ((ee) = func)) {\
         __QAIC_DBG_PRINTF__((__FILE__ ":%d:error:%d:%s\n", __LINE__, (int)(ee),#func));\
         goto ee##bail;\
      } \
   } while (0)

#define _CATCH(exception) exception##bail: if (exception != AEE_SUCCESS)

#define _ASSERT(nErr, ff) _TRY(nErr, 0 == (ff) ? AEE_EBADPARM : AEE_SUCCESS)

#ifdef __QAIC_DEBUG__
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, _allocator_alloc(pal, __FILE_LINE__, size, alignment, (void**)&pv))
#else
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, _allocator_alloc(pal, 0, size, alignment, (void**)&pv))
#endif


#endif // _QAIC_ENV_H

#include "remote.h"
#include <string.h>
#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include <stdlib.h>
#include <stdint.h>

typedef struct _heap _heap;
struct _heap {
   _heap* pPrev;
   const char* loc;
   uint64_t buf;
};

typedef struct _allocator {
   _heap* pheap;
   uint8_t* stack;
   uint8_t* stackEnd;
   int nSize;
} _allocator;

_ATTRIBUTE_UNUSED
static __inline int _heap_alloc(_heap** ppa, const char* loc, int size, void** ppbuf) {
   _heap* pn = 0;
   pn = malloc(size + sizeof(_heap) - sizeof(uint64_t));
   if(pn != 0) {
      pn->pPrev = *ppa;
      pn->loc = loc;
      *ppa = pn;
      *ppbuf = (void*)&(pn->buf);
      return 0;
   } else {
      return -1;
   }
}
#define _ALIGN_SIZE(x, y) (((x) + (y-1)) & ~(y-1))

_ATTRIBUTE_UNUSED
static __inline int _allocator_alloc(_allocator* me,
                                    const char* loc,
                                    int size,
                                    unsigned int al,
                                    void** ppbuf) {
   if(size < 0) {
      return -1;
   } else if (size == 0) {
      *ppbuf = 0;
      return 0;
   }
   if((_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + size) < (uintptr_t)me->stack + me->nSize) {
      *ppbuf = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al);
      me->stackEnd = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + size;
      return 0;
   } else {
      return _heap_alloc(&me->pheap, loc, size, ppbuf);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_deinit(_allocator* me) {
   _heap* pa = me->pheap;
   while(pa != 0) {
      _heap* pn = pa;
      const char* loc = pn->loc;
      (void)loc;
      pa = pn->pPrev;
      free(pn);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_init(_allocator* me, uint8_t* stack, int stackSize) {
   me->stack =  stack;
   me->stackEnd =  stack + stackSize;
   me->nSize = stackSize;
   me->pheap = 0;
}


#endif // _ALLOCATOR_H

#ifndef SLIM_H
#define SLIM_H

#include <stdint.h>

//a C data structure for the idl types that can be used to implement
//static and dynamic language bindings fairly efficiently.
//
//the goal is to have a minimal ROM and RAM footprint and without
//doing too many allocations.  A good way to package these things seemed
//like the module boundary, so all the idls within  one module can share
//all the type references.


#define PARAMETER_IN       0x0
#define PARAMETER_OUT      0x1
#define PARAMETER_INOUT    0x2
#define PARAMETER_ROUT     0x3
#define PARAMETER_INROUT   0x4

//the types that we get from idl
#define TYPE_OBJECT             0x0
#define TYPE_INTERFACE          0x1
#define TYPE_PRIMITIVE          0x2
#define TYPE_ENUM               0x3
#define TYPE_STRING             0x4
#define TYPE_WSTRING            0x5
#define TYPE_STRUCTURE          0x6
#define TYPE_UNION              0x7
#define TYPE_ARRAY              0x8
#define TYPE_SEQUENCE           0x9

//these require the pack/unpack to recurse
//so it's a hint to those languages that can optimize in cases where
//recursion isn't necessary.
#define TYPE_COMPLEX_STRUCTURE  (0x10 | TYPE_STRUCTURE)
#define TYPE_COMPLEX_UNION      (0x10 | TYPE_UNION)
#define TYPE_COMPLEX_ARRAY      (0x10 | TYPE_ARRAY)
#define TYPE_COMPLEX_SEQUENCE   (0x10 | TYPE_SEQUENCE)


typedef struct Type Type;

#define INHERIT_TYPE\
   int32_t nativeSize;                /*in the simple case its the same as wire size and alignment*/\
   union {\
      struct {\
         const uintptr_t         p1;\
         const uintptr_t         p2;\
      } _cast;\
      struct {\
         uint32_t  iid;\
         uint32_t  bNotNil;\
      } object;\
      struct {\
         const Type  *arrayType;\
         int32_t      nItems;\
      } array;\
      struct {\
         const Type *seqType;\
         int32_t      nMaxLen;\
      } seqSimple; \
      struct {\
         uint32_t bFloating;\
         uint32_t bSigned;\
      } prim; \
      const SequenceType* seqComplex;\
      const UnionType  *unionType;\
      const StructType *structType;\
      int32_t         stringMaxLen;\
      uint8_t        bInterfaceNotNil;\
   } param;\
   uint8_t    type;\
   uint8_t    nativeAlignment\

typedef struct UnionType UnionType;
typedef struct StructType StructType;
typedef struct SequenceType SequenceType;
struct Type {
   INHERIT_TYPE;
};

struct SequenceType {
   const Type *         seqType;
   uint32_t               nMaxLen;
   uint32_t               inSize;
   uint32_t               routSizePrimIn;
   uint32_t               routSizePrimROut;
};

//byte offset from the start of the case values for
//this unions case value array.  it MUST be aligned
//at the alignment requrements for the descriptor
//
//if negative it means that the unions cases are
//simple enumerators, so the value read from the descriptor
//can be used directly to find the correct case
typedef union CaseValuePtr CaseValuePtr;
union CaseValuePtr {
   const uint8_t*   value8s;
   const uint16_t*  value16s;
   const uint32_t*  value32s;
   const uint64_t*  value64s;
};

//these are only used in complex cases
//so I pulled them out of the type definition as references to make
//the type smaller
struct UnionType {
   const Type           *descriptor;
   uint32_t               nCases;
   const CaseValuePtr   caseValues;
   const Type * const   *cases;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
   uint8_t                inCaseAlignment;
   uint8_t                routCaseAlignmentPrimIn;
   uint8_t                routCaseAlignmentPrimROut;
   uint8_t                nativeCaseAlignment;
   uint8_t              bDefaultCase;
};

struct StructType {
   uint32_t               nMembers;
   const Type * const   *members;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
};

typedef struct Parameter Parameter;
struct Parameter {
   INHERIT_TYPE;
   uint8_t    mode;
   uint8_t  bNotNil;
};

#define SLIM_IFPTR32(is32,is64) (sizeof(uintptr_t) == 4 ? (is32) : (is64))
#define SLIM_SCALARS_IS_DYNAMIC(u) (((u) & 0x00ffffff) == 0x00ffffff)

typedef struct Method Method;
struct Method {
   uint32_t                    uScalars;            //no method index
   int32_t                     primInSize;
   int32_t                     primROutSize;
   int                         maxArgs;
   int                         numParams;
   const Parameter * const     *params;
   uint8_t                       primInAlignment;
   uint8_t                       primROutAlignment;
};

typedef struct Interface Interface;

struct Interface {
   int                            nMethods;
   const Method  * const          *methodArray;
   int                            nIIds;
   const uint32_t                   *iids;
   const uint16_t*                  methodStringArray;
   const uint16_t*                  methodStrings;
   const char*                    strings;
};


#endif //SLIM_H


#ifndef _LOADALGO_SLIM_H
#define _LOADALGO_SLIM_H
#include "remote.h"
#include <stdint.h>

#ifndef __QAIC_SLIM
#define __QAIC_SLIM(ff) ff
#endif
#ifndef __QAIC_SLIM_EXPORT
#define __QAIC_SLIM_EXPORT
#endif

static const Parameter parameters[7] = {{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)0x0,0}}, 4,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),3,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),0,0},{0xc,{{(const uintptr_t)0x0,0}}, 32,SLIM_IFPTR32(0x4,0x8),0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)0}}, 2,0x4,0,0},{0xc,{{(const uintptr_t)0x0,0}}, 32,SLIM_IFPTR32(0x4,0x8),3,0}};
static const Parameter* const parameterArrays[13] = {(&(parameters[3])),(&(parameters[5])),(&(parameters[5])),(&(parameters[5])),(&(parameters[6])),(&(parameters[5])),(&(parameters[3])),(&(parameters[5])),(&(parameters[3])),(&(parameters[4])),(&(parameters[0])),(&(parameters[1])),(&(parameters[2]))};
static const Method methods[5] = {{REMOTE_SCALARS_MAKEX(0,0,0x2,0x0,0x0,0x1),0x4,0x0,2,2,(&(parameterArrays[10])),0x4,0x1},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x1,0x0),0x0,0x0,1,1,(&(parameterArrays[12])),0x1,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x0,0x0),0x0,0x0,0,0,0,0x0,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x1,0x0),0x4,0x0,4,2,(&(parameterArrays[8])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x2,0x1),0x14,0x0,14,8,(&(parameterArrays[0])),0x4,0x0}};
static const Method* const methodArrays[6] = {&(methods[0]),&(methods[1]),&(methods[2]),&(methods[3]),&(methods[4]),&(methods[2])};
static const char strings[120] = "Gaussian7x7u8\0static_mode\0physbuffer\0dstStride\0srcStride\0srcHeight\0srcWidth\0deinit\0close\0heap\0type\0open\0dst\0src\0din\0uri\0";
static const uint16_t methodStrings[19] = {0,108,67,57,47,104,37,89,14,26,112,94,99,116,74,83,74,76,78};
static const uint16_t methodStringsArrays[6] = {12,15,18,9,0,17};
__QAIC_SLIM_EXPORT const Interface __QAIC_SLIM(loadalgo_slim) = {6,&(methodArrays[0]),0,0,&(methodStringsArrays [0]),methodStrings,strings};
#endif //_LOADALGO_SLIM_H
__QAIC_STUB_EXPORT int __QAIC_STUB(loadalgo_skel_handle_invoke)(remote_handle64 _h, uint32_t _sc, remote_arg* _pra) __QAIC_STUB_ATTRIBUTE {
   return __QAIC_REMOTE(remote_handle64_invoke)(_h, _sc, _pra);
}
#ifdef __cplusplus
extern "C" {
#endif
extern int remote_register_dma_handle(int, uint32_t);
__QAIC_STUB_EXPORT int __QAIC_STUB(loadalgo_open)(const char* uri, remote_handle64* h) __QAIC_STUB_ATTRIBUTE {
   return __QAIC_REMOTE(remote_handle64_open)(uri, h);
}
__QAIC_STUB_EXPORT int __QAIC_STUB(loadalgo_close)(remote_handle64 h) __QAIC_STUB_ATTRIBUTE {
   return __QAIC_REMOTE(remote_handle64_close)(h);
}
static __inline int _stub_method(remote_handle64 _handle, uint32_t _mid) {
   remote_arg* _pra = 0;
   int _nErr = 0;
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 0, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(loadalgo_init)(remote_handle64 _handle) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 2;
   return _stub_method(_handle, _mid);
}
static __inline int _stub_method_1(remote_handle64 _handle, uint32_t _mid, uint32_t _in0Fd[1], uint32_t _in0Offset[1], uint32_t _in0Len[1], uint32_t _in1[1]) {
   remote_arg _pra[2];
   uint32_t _primIn[1];
   remote_arg* _praHandleIn;
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _praHandleIn = (_pra + (1 + 0));
   _COPY(&(_praHandleIn[0].dma.fd), 0, _in0Fd, 0, sizeof(uint32_t));
   _COPY(&(_praHandleIn[0].dma.offset), 0, _in0Offset, 0, sizeof(uint32_t));
   _ASSERT(_nErr, remote_register_dma_handle(*_in0Fd, *_in0Len) == 0);
   
   _COPY(_primIn, 0, _in1, 0, 4);
   _ASSERT(_nErr, 1 <= 15);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 1, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT int __QAIC_STUB(loadalgo_physbuffer)(remote_handle64 _handle, int din, uint32 dinOffset, uint32 dinLen, int type) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 3;
   return _stub_method_1(_handle, _mid, (uint32_t*)&din, (uint32_t*)&dinOffset, (uint32_t*)&dinLen, (uint32_t*)&type);
}
static __inline int _stub_method_2(remote_handle64 _handle, uint32_t _mid, uint32_t _in0Fd[1], uint32_t _in0Offset[1], uint32_t _in0Len[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _rout4Fd[1], uint32_t _rout4Offset[1], uint32_t _rout4Len[1], uint32_t _in5[1], uint32_t _in6Fd[1], uint32_t _in6Offset[1], uint32_t _in6Len[1], uint32_t _in7[1]) {
   remote_arg _pra[4];
   uint32_t _primIn[5];
   remote_arg* _praHandleIn;
   remote_arg* _praHandleROut;
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _praHandleIn = (_pra + (1 + 0));
   _COPY(&(_praHandleIn[0].dma.fd), 0, _in0Fd, 0, sizeof(uint32_t));
   _COPY(&(_praHandleIn[0].dma.offset), 0, _in0Offset, 0, sizeof(uint32_t));
   _ASSERT(_nErr, remote_register_dma_handle(*_in0Fd, *_in0Len) == 0);
   
   _COPY(_primIn, 0, _in1, 0, 4);
   _COPY(_primIn, 4, _in2, 0, 4);
   _COPY(_primIn, 8, _in3, 0, 4);
   _praHandleROut = ((_pra + (1 + 0)) + 2);
   _COPY(&(_praHandleROut[0].dma.fd), 0, _rout4Fd, 0, sizeof(uint32_t));
   _COPY(&(_praHandleROut[0].dma.offset), 0, _rout4Offset, 0, sizeof(uint32_t));
   _ASSERT(_nErr, remote_register_dma_handle(*_rout4Fd, *_rout4Len) == 0);
   _COPY(_primIn, 12, _in5, 0, 4);
   _COPY(&(_praHandleIn[1].dma.fd), 0, _in6Fd, 0, sizeof(uint32_t));
   _COPY(&(_praHandleIn[1].dma.offset), 0, _in6Offset, 0, sizeof(uint32_t));
   _ASSERT(_nErr, remote_register_dma_handle(*_in6Fd, *_in6Len) == 0);
   _COPY(_primIn, 16, _in7, 0, 4);
   _ASSERT(_nErr, 2 <= 15);
   _ASSERT(_nErr, 1 <= 15);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 2, 1), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(loadalgo_Gaussian7x7u8)(remote_handle64 _handle, int src, uint32 srcOffset, uint32 srcLen, uint32 srcWidth, uint32 srcHeight, uint32 srcStride, int dst, uint32 dstOffset, uint32 dstLen, uint32 dstStride, int heap, uint32 heapOffset, uint32 heapLen, uint32 static_mode) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 4;
   return _stub_method_2(_handle, _mid, (uint32_t*)&src, (uint32_t*)&srcOffset, (uint32_t*)&srcLen, (uint32_t*)&srcWidth, (uint32_t*)&srcHeight, (uint32_t*)&srcStride, (uint32_t*)&dst, (uint32_t*)&dstOffset, (uint32_t*)&dstLen, (uint32_t*)&dstStride, (uint32_t*)&heap, (uint32_t*)&heapOffset, (uint32_t*)&heapLen, (uint32_t*)&static_mode);
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(loadalgo_deinit)(remote_handle64 _handle) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 5;
   return _stub_method(_handle, _mid);
}
#ifdef __cplusplus
}
#endif
#endif //_LOADALGO_STUB_H
