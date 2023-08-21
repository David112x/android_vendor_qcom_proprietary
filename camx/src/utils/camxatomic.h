////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxatomic.h
/// @brief Abstract atomic operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CF032: This is a C++ header file (Whiner thinks it is C)

#ifndef CAMXATOMIC_H
#define CAMXATOMIC_H

#include "camxdefs.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicInc
///
/// @brief  Atomic increment (INT) operation
///
/// @param  pVar Value to atomically increment
///
/// @return Incremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CamxAtomicInc(
    volatile INT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicIncU
///
/// @brief  Atomic increment (UINT) operation
///
/// @param  pVar Value to atomically increment
///
/// @return Incremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CamxAtomicIncU(
    volatile UINT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicIncU32
///
/// @brief  Atomic increment (UINT) operation
///
/// @param  pVar Value to atomically increment
/// @param  val  How much to increment by
///
/// @return Incremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CamxAtomicIncU32(
    volatile UINT32* pVar,
    UINT32           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicDec
///
/// @brief  Atomic decrement (INT) operation
///
/// @param  pVar Value to atomically decrement
///
/// @return Decremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CamxAtomicDec(
    volatile INT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicDecU
///
/// @brief  Atomic decrement (UINT) operation
///
/// @param  pVar Value to atomically decrement
///
/// @return Decremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CamxAtomicDecU(
    volatile UINT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicDecU32
///
/// @brief  Atomic decrement (UINT) operation
///
/// @param  pVar Value to atomically decrement
/// @param  val  How much to decrement by
///
/// @return Decremented value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CamxAtomicDecU32(
    volatile UINT32* pVar,
    UINT32           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAdd
///
/// @brief  Atomic add (INT) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CamxAtomicAdd(
    volatile INT*   pVar,
    INT             val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAdd8
///
/// @brief  Atomic add (INT8) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT8 CamxAtomicAdd8(
    volatile INT8*  pVar,
    INT8            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAdd32
///
/// @brief  Atomic add (INT32) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CamxAtomicAdd32(
    volatile INT32* pVar,
    INT32           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAdd64
///
/// @brief  Atomic add (INT64) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 CamxAtomicAdd64(
    volatile INT64* pVar,
    INT64           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAddU
///
/// @brief  Atomic add (UINT) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CamxAtomicAddU(
    volatile UINT*  pVar,
    UINT            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAddU8
///
/// @brief  Atomic add (UINT8) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 CamxAtomicAddU8(
    volatile UINT8* pVar,
    UINT8           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAddU32
///
/// @brief  Atomic add (UINT32) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CamxAtomicAddU32(
    volatile UINT32*    pVar,
    UINT32              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicAddU64
///
/// @brief  Atomic add (UINT64) operation
///
/// @param  pVar    Value to atomically add
/// @param  val     Value to atomically add
///
/// @return Sum of two input values
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 CamxAtomicAddU64(
    volatile UINT64*    pVar,
    UINT64              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSub
///
/// @brief  Atomic subtract (INT8) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CamxAtomicSub(
    volatile INT*   pVar,
    INT             val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSub8
///
/// @brief  Atomic subtract (INT8) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT8 CamxAtomicSub8(
    volatile INT8*  pVar,
    INT8            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSub32
///
/// @brief  Atomic subtract (INT32) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CamxAtomicSub32(
    volatile INT32* pVar,
    INT32           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSub64
///
/// @brief  Atomic subtract (INT64) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 CamxAtomicSub64(
    volatile INT64* pVar,
    INT64           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSubU
///
/// @brief  Atomic subtract (UINT) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CamxAtomicSubU(
    volatile UINT*  pVar,
    UINT            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSubU8
///
/// @brief  Atomic subtract (UINT8) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 CamxAtomicSubU8(
    volatile UINT8* pVar,
    UINT8           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSubU32
///
/// @brief  Atomic subtract (UINT32) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CamxAtomicSubU32(
    volatile UINT32*    pVar,
    UINT32              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicSubU64
///
/// @brief  Atomic subtract (UINT64) operation
///
/// @param  pVar    Value to atomically subtract from
/// @param  val     Value to atomically subtract
///
/// @return Result of pVar - value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 CamxAtomicSubU64(
    volatile UINT64*    pVar,
    UINT64              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStore
///
/// @brief  Atomic store (INT) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStore(
    volatile INT*   pVar,
    INT             val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStore8
///
/// @brief  Atomic store (INT8) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStore8(
    volatile INT8   pVar,
    INT8            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStore32
///
/// @brief  Atomic store (INT32) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStore32(
    volatile INT32* pVar,
    INT32           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStore64
///
/// @brief  Atomic store (INT64) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStore64(
    volatile INT64* pVar,
    INT64           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStoreU
///
/// @brief  Atomic store (UINT) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStoreU(
    volatile UINT*  pVar,
    UINT            val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStoreU8
///
/// @brief  Atomic store (UINT8) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStoreU8(
    volatile UINT8* pVar,
    UINT8           val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStoreU32
///
/// @brief  Atomic store (UINT32) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStoreU32(
    volatile UINT32*    pVar,
    UINT32              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStoreU64
///
/// @brief  Atomic store (UINT64) operation
///
/// @param  pVar    Pointer to location at which to store value
/// @param  val     Value to store
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStoreU64(
    volatile UINT64*    pVar,
    UINT64              val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicStoreP
///
/// @brief  Atomic store pointer operation
///
/// @param  ppDst   Pointer to location at which to store value
/// @param  pSrc    Pointer location from where to get the value to store into Dest
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxAtomicStoreP(
    VOID** ppDst,
    VOID*  pSrc);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoad
///
/// @brief  Atomic load (INT) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CamxAtomicLoad(
    volatile INT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoad8
///
/// @brief  Atomic load (INT8) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT8 CamxAtomicLoad8(
    volatile INT8* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoad32
///
/// @brief  Atomic load (INT32) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CamxAtomicLoad32(
    volatile INT32* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoad64
///
/// @brief  Atomic load (INT64) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT64 CamxAtomicLoad64(
    volatile INT64* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoadU
///
/// @brief  Atomic load (UINT) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CamxAtomicLoadU(
    volatile UINT* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoadU8
///
/// @brief  Atomic load (UINT8) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 CamxAtomicLoadU8(
    volatile UINT8* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoadU32
///
/// @brief  Atomic load (UINT32) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CamxAtomicLoadU32(
    volatile UINT32* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoadU64
///
/// @brief  Atomic load (UINT64) operation
///
/// @param  pVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 CamxAtomicLoadU64(
    volatile UINT64* pVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicLoadP
///
/// @brief  Atomic load pointer operation
///
/// @param  ppVar Pointer to location from which to load the value
///
/// @return The value loaded from pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CamxAtomicLoadP(
    VOID** ppVar);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicCompareExchange
///
/// @brief  Atomic compare and exchange (INT). If the current value of *pVar is oldValue, then write newValue into *pVar.
///
/// @param  pVar        Pointer to location of value to compare with oldValue
/// @param  oldVal      Value to compare to *pVar
/// @param  newVal      Value to write to *pVar if *pVar and oldValue are equal.
///
/// @return TRUE if the comparison is successful and newValue was written to *pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CamxAtomicCompareExchange(
    volatile INT*   pVar,
    INT             oldVal,
    INT             newVal);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxAtomicCompareExchangeU
///
/// @brief  Atomic compare and exchange (UINT). If the current value of *pVar is oldValue, then write newValue into *pVar.
///
/// @param  pVar        Pointer to location of value to compare with oldValue
/// @param  oldVal      Value to compare to *pVar
/// @param  newVal      Value to write to *pVar if *pVar and oldValue are equal.
///
/// @return TRUE if the comparison is successful and newValue was written to *pVar
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CamxAtomicCompareExchangeU(
    volatile UINT*  pVar,
    UINT            oldVal,
    UINT            newVal);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFence
///
/// @brief  Issues memory barrier operation
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CamxFence();

CAMX_NAMESPACE_END

#endif // CAMXATOMIC_H
