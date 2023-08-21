////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017,2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpacketbuilder.cpp
/// @brief PacketBuilder class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxcmdbuffer.h"
#include "camxcslsensordefs.h"
#include "camxincs.h"
#include "camxpacketbuilder.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteRegRangeSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteRegRangeSizeInDwords(
    UINT32  numVals)
{
    return cdm_required_size_reg_continuous(numVals);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteInterleavedRegsSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteInterleavedRegsSizeInDwords(
    UINT32 numRegVals)
{
    return cdm_required_size_reg_random(numRegVals);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteDMISizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteDMISizeInDwords()
{
    return cdm_required_size_dmi();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteIndirectSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteIndirectSizeInDwords()
{
    return cdm_required_size_indirect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteChangeRegBaseSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteChangeRegBaseSizeInDwords()
{
    return cdm_required_size_changebase();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteRegRange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteRegRange(
    CmdBuffer*  pCmdBuffer,
    UINT32      reg,
    UINT32      numVals,
    UINT32*     pVals)
{
    CamxResult  result                  = CamxResultSuccess;

    if ((NULL == pCmdBuffer) || (numVals <= 0) || (NULL == pVals))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
        result = CamxResultEInvalidArg;
    }
    else
    {
        UINT32  requiredSizeInDwords    = RequiredWriteRegRangeSizeInDwords(numVals);
        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            VOID* pCmd = cdm_write_regcontinuous(static_cast<UINT32*>(pCurrentCmdAddr), reg, numVals, pVals);

            CAMX_ASSERT(pCmd == Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32)));
        }

        if (CamxResultSuccess == result)
        {
            result = pCmdBuffer->CommitCommands();
        }
        else
        {
            pCmdBuffer->CancelCommands();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteInterleavedRegs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteInterleavedRegs(
    CmdBuffer*  pCmdBuffer,
    UINT32      numRegVals,
    UINT32*     pRegVals)
{
    CamxResult  result                  = CamxResultSuccess;

    if ((NULL == pCmdBuffer) || (numRegVals == 0) || (NULL == pRegVals))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
        result = CamxResultEInvalidArg;
    }
    else
    {
        UINT32  requiredSizeInDwords    = RequiredWriteInterleavedRegsSizeInDwords(numRegVals);
        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            VOID* pCmd = cdm_write_regrandom(static_cast<UINT32*>(pCurrentCmdAddr),
                                             numRegVals,
                                             pRegVals);

            CAMX_ASSERT(pCmd == Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32)));
        }

        if (CamxResultSuccess == result)
        {
            result = pCmdBuffer->CommitCommands();
        }
        else
        {
            pCmdBuffer->CancelCommands();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteDMI(
    CmdBuffer*  pCmdBuffer,
    UINT32      DMIAddr,
    UINT8       DMISel,
    CmdBuffer*  pDMIBuf,
    UINT32      offset,
    UINT32      size)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((0 == offset) || (0 != size));

    if ((NULL == pCmdBuffer) || (NULL == pDMIBuf))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args pCmdBuffer %p pDMIBuf %p ", pCmdBuffer, pDMIBuf);
    }
    else
    {
        UINT32  requiredSizeInDwords = RequiredWriteDMISizeInDwords();

        if (TRUE == pCmdBuffer->MustInlineIndirectBuffers())
        {
            requiredSizeInDwords += (0 == size) ? (pDMIBuf->GetResourceUsedDwords() * sizeof(UINT32)) : size / 4;
        }

        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);
        UINT32  addrOffset              = (pCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)) + cdm_offsetof_dmi_addr();

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            VOID*   pCmd        = NULL;
            UINT8   DMICmd      = CDMCmdDMI;
            UINT32  DMILength   = (0 == size) ? (pDMIBuf->GetResourceUsedDwords() * sizeof(UINT32)) : size;
            VOID*   pDMISrcAddr = CamX::Utils::VoidPtrInc(pDMIBuf->GetHostAddr(), offset);

            switch (pDMIBuf->GetType())
            {
                case CmdType::CDMDMI:
                    DMICmd = CDMCmdDMI;
                    break;
                case CmdType::CDMDMI32:
                    DMICmd = (TRUE == pCmdBuffer->MustInlineIndirectBuffers()) ? SwCmdDMI32
                                                                               : CDMCmdDMI32;
                    break;
                case CmdType::CDMDMI64:
                    DMICmd = (TRUE == pCmdBuffer->MustInlineIndirectBuffers()) ? SwCmdDMI64
                                                                               : CDMCmdDMI64;
                    break;
                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Unknown CDM DMI Type: %d", pDMIBuf->GetType());
                    break;
            }

            pCmd = cdm_write_dmi(static_cast<UINT32*>(pCurrentCmdAddr),
                                 DMICmd,
                                 DMIAddr,
                                 DMISel,
                                 /// @note: Since device address is not available to UMD, the below
                                 ///        address is not undefined on HW target (and will be patched instead)
                                 ///        but valid in presil.
                                 pDMIBuf->GetDeviceAddr(),
                                 DMILength);

            if (TRUE == pCmdBuffer->MustInlineIndirectBuffers())
            {
                Utils::Memcpy(pCmd, pDMISrcAddr, DMILength);
                pCmd = Utils::VoidPtrInc(pCmd, DMILength);
            }

            pCurrentCmdAddr = Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32));
            if (pCurrentCmdAddr != pCmd)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "The Written sizes doesn't match pCurrentCmdAdr = %p pCmd %p",
                               pCurrentCmdAddr, pCmd);
            }

            if (CamxResultSuccess == result)
            {
                result = pCmdBuffer->CommitCommands();
            }
            else
            {
                pCmdBuffer->CancelCommands();
            }

            if (CamxResultSuccess == result)
            {
                if (TRUE == pCmdBuffer->IsPatchingEnabled())
                {
                    result = pCmdBuffer->AddNestedCmdBufferInfo(addrOffset, pDMIBuf, offset);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteIndirect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteIndirect(
    CmdBuffer*  pCmdBuffer,
    CmdBuffer*  pIndirectCmdBuffer,
    UINT32      offset,
    UINT32      size)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((0 == offset) || (0 != size));

    if ((NULL == pCmdBuffer) || (NULL == pIndirectCmdBuffer))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        UINT32 requiredSizeInDwords = RequiredWriteIndirectSizeInDwords();

        if (TRUE == pCmdBuffer->MustInlineIndirectBuffers())
        {
            // In inlining mode, the whole indirect buffer is written in the command buffer without any header.
            requiredSizeInDwords = pIndirectCmdBuffer->GetResourceUsedDwords();
        }

        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);
        UINT32  length                  = (0 == size) ? (pIndirectCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)) : size;
        VOID*   pSrcAddr                = CamX::Utils::VoidPtrInc(pIndirectCmdBuffer->GetHostAddr(), offset);
        UINT32  addrOffset              =
            (pCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)) + cdm_offsetof_indirect_addr();

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            VOID* pCmd = pCurrentCmdAddr;

            if (TRUE == pCmdBuffer->MustInlineIndirectBuffers())
            {
                Utils::Memcpy(pCmd, pSrcAddr, length);
                pCmd = Utils::VoidPtrInc(pCmd, length);
            }
            else
            {
                pCmd = cdm_write_indirect(static_cast<UINT32*>(pCurrentCmdAddr),
                                          /// @note: Since device address is not available to UMD, the below
                                          ///        address is not undefined on HW target (and will be patched instead)
                                          ///        but valid in presil.
                                          pIndirectCmdBuffer->GetDeviceAddr(),
                                          length);
            }

            CAMX_ASSERT(Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32)) == pCmd);

            if (CamxResultSuccess == result)
            {
                result = pCmdBuffer->CommitCommands();
            }
            else
            {
                pCmdBuffer->CancelCommands();
            }

            if (CamxResultSuccess == result)
            {
                if (TRUE == pCmdBuffer->IsPatchingEnabled())
                {
                    pCmdBuffer->AddNestedCmdBufferInfo(addrOffset, pIndirectCmdBuffer, offset);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteChangeRegBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteChangeRegBase(
    CmdBuffer*  pCmdBuffer,
    UINT32      base)
{
    CamxResult  result = CamxResultSuccess;

    if (NULL == pCmdBuffer)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
    }
    else
    {
        UINT32  requiredSizeInDwords    = RequiredWriteChangeRegBaseSizeInDwords();
        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            VOID* pCmd = cdm_write_changebase(static_cast<UINT32*>(pCurrentCmdAddr),
                                              base);

            CAMX_ASSERT(Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32)) == pCmd);
        }

        if (CamxResultSuccess == result)
        {
            result = pCmdBuffer->CommitCommands();

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Failed to commit commands.");
            }
        }
        else
        {
            pCmdBuffer->CancelCommands();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteI2CInterleavedRegs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteI2CInterleavedRegs(
    CmdBuffer*  pCmdBuffer,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      numRegVals,
    UINT32*     pRegVals)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(numRegVals);
    CAMX_UNREFERENCED_PARAM(pRegVals);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteI2CInterleavedRegsSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteI2CInterleavedRegsSizeInDwords(
    UINT32  numRegVals)
{
    CAMX_UNREFERENCED_PARAM(numRegVals);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteI2CContinuousRegs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteI2CContinuousRegs(
    CmdBuffer*  pCmdBuffer,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      reg,
    UINT32      numVals,
    UINT32*     pVals)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(reg);
    CAMX_UNREFERENCED_PARAM(numVals);
    CAMX_UNREFERENCED_PARAM(pVals);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteI2CContinuousRegsSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteI2CContinuousRegsSizeInDwords(
    UINT32  numVals)
{
    CAMX_UNREFERENCED_PARAM(numVals);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::ReadI2CInterleavedRegs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::ReadI2CInterleavedRegs(
    CmdBuffer*  pCmdBuffer,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      numRegs,
    UINT32*     pRegs)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(numRegs);
    CAMX_UNREFERENCED_PARAM(pRegs);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredReadI2CInterleavedRegsSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredReadI2CInterleavedRegsSizeInDwords(
    UINT32  numRegs)
{
    CAMX_UNREFERENCED_PARAM(numRegs);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::ReadI2CContinuousRegs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::ReadI2CContinuousRegs(
    CmdBuffer*  pCmdBuffer,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      reg,
    UINT32      numRegs)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(reg);
    CAMX_UNREFERENCED_PARAM(numRegs);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredReadI2CContinuousRegsSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredReadI2CContinuousRegsSizeInDwords(
    UINT32  numRegs)
{
    CAMX_UNREFERENCED_PARAM(numRegs);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteSensorConditionalWait
///
/// @brief  Writes a command into the command buffer that, when executed, will poll the given register until its value
///         becomes equal to the provided (masked) value.
///
/// @param  pCmdBuffer  Address of the target command buffer
/// @param  addrType    Address type of I2C operation
/// @param  dataType    Data type of I2C operation
/// @param  reg         Address of the register to poll
/// @param  val         Expected value
/// @param  mask        Mask that should be applied to the value
/// @param  timeout     Timeout after which the wait ends
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteSensorConditionalWait(
    CmdBuffer*  pCmdBuffer,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      reg,
    UINT32      val,
    UINT32      mask,
    UINT32      timeout)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(reg);
    CAMX_UNREFERENCED_PARAM(val);
    CAMX_UNREFERENCED_PARAM(mask);
    CAMX_UNREFERENCED_PARAM(timeout);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteSensorConditionalWaitSizeInDwords
///
/// @brief  Calculates the required size of a WriteSensorConditionalWait command in dwords.
///
/// @param  numRegs Number of registers that will be read
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteSensorConditionalWaitSizeInDwords()
{
    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteSensorWait
///
/// @brief  Writes a command into the command buffer that, when executed, will wait for the given amount of time
///
/// @param  pCmdBuffer  Address of the target command buffer
/// @param  delay       Delay after which the wait ends
/// @param  useHW       Specifies whether the wait should happen in HW
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteSensorWait(
    CmdBuffer*  pCmdBuffer,
    UINT32      delay,
    BOOL        useHW)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(delay);
    CAMX_UNREFERENCED_PARAM(useHW);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteSensorWaitSizeInDwords
///
/// @brief  Calculates the required size of a WriteSensorWait command in dwords.
///
/// @param  numRegs Number of registers that will be read
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteSensorWaitSizeInDwords()
{
    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WritePowerSequence
///
/// @brief  Writes a command into the command buffer that, when executed, will execute the given power sequence
///
/// @param  pCmdBuffer  Address of the target command buffer
/// @param  numSettings Number of power settings to be written
/// @param  pSettings   Array of power settings
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WritePowerSequence(
    CmdBuffer*              pCmdBuffer,
    UINT32                  numSettings,
    CSLSensorPowerSetting*  pSettings)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(numSettings);
    CAMX_UNREFERENCED_PARAM(pSettings);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWritePowerSequenceSizeInDwords
///
/// @brief  Calculates the required size of a WritePowerSequence command in dwords.
///
/// @param  numSettings Number of power settings that will be written
///
/// @return Size in dwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWritePowerSequenceSizeInDwords(
    UINT32  numSettings)
{
    CAMX_UNREFERENCED_PARAM(numSettings);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteSensorProbe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteSensorProbe(
    CmdBuffer*  pCmdBuffer,
    UINT16      sensorId,
    UINT8       addrType,
    UINT8       dataType,
    UINT32      reg,
    UINT32      val,
    UINT32      mask)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);
    CAMX_UNREFERENCED_PARAM(sensorId);
    CAMX_UNREFERENCED_PARAM(addrType);
    CAMX_UNREFERENCED_PARAM(dataType);
    CAMX_UNREFERENCED_PARAM(reg);
    CAMX_UNREFERENCED_PARAM(val);
    CAMX_UNREFERENCED_PARAM(mask);

    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredWriteSensorProbeSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredWriteSensorProbeSizeInDwords()
{
    CamxResult  result = CamxResultSuccess;
    CAMX_NOT_IMPLEMENTED();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::RequiredGenericBlobDataSizeInDwords
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 PacketBuilder::RequiredGenericBlobDataSizeInDwords(
    UINT32  blobSize)
{
    return  CSLGenericBlobHeaderSizeInDwords +                  // header (type + size)
            ((blobSize + sizeof(UINT32) - 1) / sizeof(UINT32)); // Actual data + dword alignment
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PacketBuilder::WriteGenericBlobData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PacketBuilder::WriteGenericBlobData(
    CmdBuffer*  pCmdBuffer,
    UINT32      blobType,
    UINT32      blobSize,
    BYTE*       pBlobData)
{
    CamxResult  result  = CamxResultSuccess;

    if ((NULL == pCmdBuffer) || (blobSize == 0) || (NULL == pBlobData))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid input args");
        result = CamxResultEInvalidArg;
    }
    else if (((blobSize >> (32 - CSLGenericBlobCmdSizeShift)) > 0) || (((blobType & (!CSLGenericBlobCmdTypeMask)) > 0)))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid blob params type %d, size %d", blobType, blobSize);
        result = CamxResultEInvalidArg;
    }
    else
    {
        UINT32  requiredSizeInDwords    = RequiredGenericBlobDataSizeInDwords(blobSize);
        VOID*   pCurrentCmdAddr         = pCmdBuffer->BeginCommands(requiredSizeInDwords);

        if (NULL == pCurrentCmdAddr)
        {
            result = CamxResultENoMore;
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of command buffer space");
        }
        else
        {
            UINT32  blobSizeWithPaddingInBytes  = ((blobSize + sizeof(UINT32) - 1) / sizeof(UINT32)) * sizeof(UINT32);
            UINT32* pCmd                        = static_cast<UINT32*>(pCurrentCmdAddr);

            // Always insert actual blob size in the header
            *pCmd++ = ((blobSize << CSLGenericBlobCmdSizeShift) & CSLGenericBlobCmdSizeMask) |
                      ((blobType << CSLGenericBlobCmdTypeShift) & CSLGenericBlobCmdTypeMask);

            Utils::Memcpy(reinterpret_cast<BYTE*>(pCmd), pBlobData, blobSize);
            pCmd = static_cast<UINT32*>(Utils::VoidPtrInc(static_cast<VOID*>(pCmd), blobSizeWithPaddingInBytes));

            CAMX_ASSERT(static_cast<VOID*>(pCmd) == Utils::VoidPtrInc(pCurrentCmdAddr, requiredSizeInDwords * sizeof(UINT32)));

            result = pCmdBuffer->CommitCommands();

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Error in Commit command, blob=%d, result=%d", blobType, result);
            }
        }
    }

    return result;
}

CAMX_NAMESPACE_END
