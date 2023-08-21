// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "icautils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::CalcMantissaExp4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils::CalcMantissaExp4(
    INT32  sa1,
    UINT32 ua2,
    UINT32 ub1,
    UINT32 ub2)
{
    return ConvertFloatToRegister(ConvertRatioToFloat(sa1, ua2, ub1, ub2));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::CountLeadingZeros
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils::CountLeadingZeros(
    UINT32 x)
{
    UINT32 n = 0;

    if (x == 0)
    {
        return 32;
    }

    if ((x & 0xFFFF0000) == 0)
    {
        n = n + 16; x = x << 16;
    }
    if ((x & 0xFF000000) == 0)
    {
        n = n + 8; x = x << 8;
    }
    if ((x & 0xF0000000) == 0)
    {
        n = n + 4; x = x << 4;
    }
    if ((x & 0xC0000000) == 0)
    {
        n = n + 2; x = x << 2;
    }
    if ((x & 0x80000000) == 0)
    {
        n = n + 1;
    }

    return n;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::SaturateSigned
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::SaturateSigned(
    INT32 val,
    UINT32 sat)
{
    INT32 ret = val;
    INT32 low, high;

    if (sat >= 1 && sat <= 32)
    {
        low = -(1 << (sat - 1));
        if (val < low)
        {
            ret = low;
        }
        high = (1 << (sat - 1)) - 1;
        if (val > high)
        {
            ret = high;
        }
    }
    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ConvertFloatToRegister
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils::ConvertFloatToRegister(
    FPV fpv)
{
    if (!fpv.wasError)
    {
        return (fpv.mantissa & CFPMantissaMask17x) |
            ((fpv.exponent & CFPExponentMask) << CFPMantissaBits17x);
    }
    return ICAFpError;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::Log2Floor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::Log2Floor(
    UINT32 x)
{
    return 31 - CountLeadingZeros(x);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::Log2Ceil
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::Log2Ceil(
    UINT32 x)
{
    INT b, m;

    if (x == 0)
    {
        return -1;
    }

    b = Log2Floor(x);
    m = (1 << b) - 1;

    if ((x & m) != 0)
    {
        b = b + 1;
    }

    return b;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::DivCeil32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils:: DivCeil32(
    UINT32 a,
    UINT32 b)
{
    return (a + b - 1) / b;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::DivCeil64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils::DivCeil64(
    UINT64 a,
    UINT64 b)
{
    return static_cast<UINT32>((a + b - 1) / b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::Abs64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 ICAUtils::Abs64(
    INT64 a)
{
    return (a >= 0) ? a : -a;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::Abs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ICAUtils::Abs(
    INT32 a)
{
    return (a >= 0) ? a : -a;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ConvertRatioToFloatImp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 FPV ICAUtils::ConvertRatioToFloatImp(
    INT32 sa1,
    UINT32 ua2,
    UINT32 ub1,
    UINT32 ub2)
{
    INT32 m, e, nshft;

    if (sa1 == 0 || ua2 == 0)
    {
        return fpCreate(0, 0);
    }
    else
    {
        // Test for signed/unsigned overflows
        INT64 sa64 = static_cast<INT64>(sa1 * ua2);
        INT32 sa32 = static_cast<INT32>(sa64);
        UINT64 ub64 = static_cast<UINT64>(ub1 * ub2);
        UINT32 ub32 = static_cast<UINT32>(ub64);
        INT64 nom, denom;

        if (sa64 == sa32 && ub64 == ub32)
        {
            UINT32 ua = Abs(sa32);
            if (ua > ub32)
            {
                e = Log2Floor(ua / ub32) + 1;
            }
            else
            {
                e = 1 - Log2Ceil(DivCeil32(ub32, ua));
            }
        }
        else // overflow
        {
            UINT64 ua = Abs64(sa64);
            if (ua > ub64)
            {
                e = Log2Floor(static_cast<UINT32>(ua / ub64)) + 1;
            }
            else
            {
                e = 1 - Log2Ceil(DivCeil64(ub64, ua));
            }
        }

        if (e >= 0)
        {
            nshft = (CFPMantissaBits17x - 1);
            denom = static_cast<INT64>(ub64) << e;
            if (ub64 != (static_cast<UINT64>(denom) >> e))
            {
                return FPError; // overflow
            }
        }
        else
        {
            nshft = (CFPMantissaBits17x - 1 - e);
            denom = static_cast<INT64>(ub64);
        }

        nom = (sa64 << nshft);
        if (sa64 != (nom >> nshft))
        {
            return FPError; // overflow
        }

        if (sa64 >= 0)
        {
            m = static_cast<INT32>((nom + (denom >> 1)) / denom);
        }
        else
        {
            m = static_cast<INT32>((nom - (denom >> 1)) / denom);
        }

        m = SaturateSigned(m, CFPMantissaBits17x);
        if (m == -(1 << (CFPMantissaBits17x - 1))) // Mantissa must be symmetric
        {
            e += 1;
            m >>= 1;
        }

        if (e < static_cast<INT32>(CFPExponentMin) || e >  static_cast<INT32>(CFPExponentMax))
        {
            // HL_DBG_ERR(LOG_TAG, "Exponent %d cannot be represented in 6 bits", e);
            return FPError;
        }

        e = SaturateSigned(e, CFPExpBits);

        return fpCreate(m, e);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ConvertRatioToFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FPV ICAUtils::ConvertRatioToFloat(
    INT32 sa1,
    UINT32 ua2,
    UINT32 ub1,
    UINT32 ub2)
{
    FPV fpv = ConvertRatioToFloatImp(sa1, ua2, ub1, ub2);

    if (fpv.wasError) // overflow, fallback to floating-point math
    {
        FPV fp1, fp2;

        fp1 = ConvertRatioToFloatImp(sa1, 1, ub1, 1);
        if (fp1.wasError)
        {
            return fp1; // nothing more to do ...
        }

        fp2 = ConvertRatioToFloatImp(1, ua2, 1, ub2);
        if (fp2.wasError)
        {
            return fp2; // nothing more to do ...
        }

        fpv = fpMult32(fp1, fp2);
    }

    return fpv;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ExtendSign32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::ExtendSign32(
    INT32 v,
    INT32 nbits)
{
    return (((v) << (32 - (nbits))) >> (32 - (nbits)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ShiftRightBySignedVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::ShiftRightBySignedVal(
    INT32 v,
    INT sr)
{
    INT32 ret = 0;
    if (sr > 0)
    {
        ret = ((v + (1 << (sr - 1))) >> sr);
    }
    else
    {
        ret = (v << (-sr)); // fill with zeros
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::TreatLowExponent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICAUtils::TreatLowExponent(
    FPV *ret)
{
    if (ret->exponent < -(1 << (CFPExpBits - 1)))
    {
        ret->mantissa = ret->exponent = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::CheckError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICAUtils::CheckError(
    FPV *ret)
{
    INT sgn;
    if (ret->exponent >= (1 << (CFPExpBits - 1)))
    {
        ret->wasError = TRUE; // global error indication in stage.
        sgn = (ret->mantissa >= 0) ? 1 : -1;
        ret->mantissa = sgn*((1 << (CFPMantissaBits17x - 1)) - 1);
        ret->exponent = (1 << (CFPExpBits - 1)) - 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::HandleCornerCase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ICAUtils::HandleCornerCase(
    FPV *ret)
{
    // ret->mantissa is 17 bits
    if (Abs(ret->mantissa) == (1 << (CFPMantissaBits17x - 1)))
    {
        ret->mantissa >>= 1; // ret->mantissa is 16 bits
        ret->exponent += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::fpCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FPV ICAUtils::fpCreate(
    INT32 mantissa,
    INT32 exponent)
{
    FPV fpv;
    fpv.mantissa = ExtendSign32(mantissa, CFPMantissaBits17x);
    fpv.exponent = ExtendSign32(exponent, CFPExpBits);
    fpv.wasError = false;
    return fpv;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::Log2Floor64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::Log2Floor64(
    UINT64 x)
{
    INT32 ret = 0;
    if ((x & 0xFFFFFFFF00000000) == 0)
    {
        ret = 31 - CountLeadingZeros(static_cast<uint32_t>(x));
    }
    else
    {
        ret = 63 - CountLeadingZeros(static_cast<uint32_t>(x >> 32));
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::ShiftRightBySignedVal64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ICAUtils::ShiftRightBySignedVal64(
    INT64 v, INT sr)
{
    INT32 ret = 0;
    if (sr > 0)
    {
        ret = static_cast<INT32>((v + static_cast<INT64>(1 << (sr - 1))) >> sr);
    }
    else {
        ret = static_cast<INT32>(v << (-sr)); // fill with zeros
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::fpMult64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FPV ICAUtils::fpMult64(
    FPV v1,
    FPV v2)
{
    FPV ret;
    INT32 dExp;
    INT32 eNew;
    INT32 sgn;
    INT64 mNew;
    UINT64 mNewAbs;

    if ((TRUE == v1.wasError) || (TRUE == v2.wasError))
    {
        ret = FPError;
    }
    else
    {
        mNew = static_cast<INT64>(v1.mantissa) * static_cast<INT64>(v2.mantissa);
        eNew = v1.exponent + v2.exponent;
        if (0 == mNew)
        {
            ret = fpCreate(0, 0);
        }
        else
        {
            mNewAbs = Abs64(mNew);
            sgn = (mNew >= 0) ? 1 : -1;
            dExp = Log2Floor64(mNewAbs);//FindFirst1FromLeft(mNewAbs,31)
            dExp -= CFPMantissaBits480 - 2;
            ret.mantissa = sgn* ShiftRightBySignedVal64(mNewAbs, dExp); // 17 bits
            ret.exponent = eNew + dExp - CFPMantissaBits480 + 1;
            HandleCornerCase(&ret);
            TreatLowExponent(&ret);
            CheckError(&ret);
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICAUtils::fpMult32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FPV ICAUtils::fpMult32(
    FPV v1,
    FPV v2)
{
    FPV ret;
    INT32 dExp;
    INT32 eNew;
    INT32 sgn;
    INT32 mNew;
    UINT32 mNewAbs;

    if ((TRUE == v1.wasError) || (TRUE == v2.wasError))
    {
        ret =  FPError;
    }
    else
    {
        mNew = static_cast<INT64>(v1.mantissa) * static_cast<INT64>(v2.mantissa);
        eNew = v1.exponent + v2.exponent;
        if (0 == mNew)
        {
            ret = fpCreate(0, 0);
        }
        else
        {
            mNewAbs = Abs(mNew);
            sgn = (mNew >= 0) ? 1 : -1;
            dExp = Log2Floor(mNewAbs);//FindFirst1FromLeft(mNewAbs,31)
            dExp -= CFPMantissaBits17x - 2;
            ret.mantissa = sgn* ShiftRightBySignedVal(mNewAbs, dExp); // 17 bits
            ret.exponent = eNew + dExp - CFPMantissaBits17x + 1;
            HandleCornerCase(&ret);
            TreatLowExponent(&ret);
            CheckError(&ret);
        }
    }
    return ret;
}
