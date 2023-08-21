/*===========================================================================
    FILE:           acdb_utility.c

    OVERVIEW:       This file contains the acdb init utility functions
                    implemented specifically in the win32 environment.

    DEPENDENCIES:   None

                    Copyright (c) 2010-2018 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */

/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_utility.c#6 $

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2010-07-08  vmn     Initial revision.

========================================================================== */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

#include "acdb_utility.h"
#include "acdb_datainfo.h"
#include "acdb_private.h"

int32_t AcdbDataCompareIndices(uint32_t* lookup,uint32_t *key,
                                           int num_params)
{
   int32_t i = 0;
   uint32_t* lookupVal = NULL;
   uint32_t* keyVal = NULL;

   // Assuming that the indices are all uint32_t.
   for(i = 0;i<num_params; i++)
      {
          lookupVal = lookup+i;
          keyVal = key+i;

          if(*lookupVal > *keyVal)
          {
              return 1;
          }
          else if(*lookupVal < *keyVal)
          {
              return -1;
          }
      }
   return 0;
}
int32_t AcdbDataBinarySearch(void *voidLookUpArray, int32_t max,int32_t indexCount,
            void *pCmd, int32_t nNoOfIndsCount,uint32_t *index)
{
   int32_t result = SEARCH_ERROR;
   int32_t min = 0;
   int32_t mid = 0;
   int32_t compareResult = 0;

   uint32_t *lookUpArray = (uint32_t *)voidLookUpArray;

   while(max >= min)
   {
       mid = (min + max)/2;

       compareResult = AcdbDataCompareIndices(&lookUpArray[indexCount * mid],
                                   (uint32_t *)pCmd,nNoOfIndsCount);

       if(compareResult > 0) // search upper array
       {
           max = mid - 1;
       }
       else if(compareResult < 0) // search lower array
       {
           min = mid + 1;
       }
       else
       {
		   // If its a partial search then the found index could no be the very first item
		   // Find the first occurence of this element by going backward
		   while(0 == AcdbDataCompareIndices(&lookUpArray[indexCount * (mid-1)],
                                   (uint32_t *)pCmd,nNoOfIndsCount))
		   {
			   mid = mid - 1;
		   }
		   *index = (uint32_t)mid;
           result = SEARCH_SUCCESS;
           break;
       }
   }

   return result;
}

uint16_t GetUint16IID(uint32_t iid)
{
	uint16_t new_iid = iid & 0x0000FFFF;
	return new_iid;
}

uint32_t GetUint32IID(uint16_t iid)
{
      const uint32_t lim=0xFFFF;
      uint32_t new_iid=iid & lim;
      return new_iid;
}

int32_t Get_table_indices_count(uint32_t tblId, uint32_t *noOfTableIndices, uint32_t *nonModuleTblFound, uint32_t *noOfCdftIndices, uint32_t *noOfCmdIndices)
{
	switch(tblId)
	{
	   case AUDPROC_GAIN_INDP_TBL:
		*noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
		*noOfCdftIndices = AUDPROCTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDPROC_CMD_INDICES_COUNT;
		   break;
	   case AUDPROC_COPP_GAIN_DEP_TBL:
		*noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
		*noOfCdftIndices = AUDPROCTBL_GAIN_DEP_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDPROC_GAIN_DEP_CMD_INDICES_COUNT;
		   break;
	   case AUDPROC_AUD_VOL_TBL:
		*noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
		*noOfCdftIndices = AUDPROCTBL_VOL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDPROC_VOL_CMD_INDICES_COUNT;
		   break;
	   case AUD_STREAM_TBL:
		*noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
		*noOfCdftIndices = AUDSTREAMTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDSTREAM_CMD_INDICES_COUNT;
		   break;
	   case VOCPROC_GAIN_INDP_TBL:
		*noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROC_CMD_INDICES_COUNT;
		   break;
	   case VOCPROC_COPP_GAIN_DEP_TBL:
		*noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCTBL_VOL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROC_VOL_CMD_INDICES_COUNT;
		   break;
	   case VOC_STREAM_TBL:
		*noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
		*noOfCdftIndices = VOCSTREAMTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCSTREAM_CMD_INDICES_COUNT;
		   break;
	   case AFE_TBL:
		*noOfTableIndices = AFETBL_INDICES_COUNT;
		*noOfCdftIndices = AFETBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AFE_CMD_INDICES_COUNT;
		   break;
	   case AFE_CMN_TBL:
		*noOfTableIndices = AFECMNTBL_INDICES_COUNT;
		*noOfCdftIndices = AFECMNTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AFECMN_CMD_INDICES_COUNT;
		   break;
	   case VOCPROC_DEV_CFG_TBL:
		*noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCDEVCFGTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROCDEVCFG_CMD_INDICES_COUNT;
		   break;
	   case ADIE_ANC_TBL:
		*noOfTableIndices = ANCTBL_INDICES_COUNT;
		*nonModuleTblFound = 1;
		*noOfCdftIndices = ANCTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = ANC_CMD_INDICES_COUNT;
		   break;
	   case ADIE_CODEC_TBL:
		*noOfTableIndices = ADIETBL_INDICES_COUNT;
		*nonModuleTblFound = 1;
		*noOfCdftIndices = ADIETBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = ADIE_CMD_INDICES_COUNT;
		   break;
	   case GLOBAL_DATA_TBL:
		*noOfTableIndices = GLOBALTBL_INDICES_COUNT;
		*nonModuleTblFound = 1;
		*noOfCdftIndices = GLOBALTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = GLOBAL_CMD_INDICES_COUNT;
		   break;
	   case LSM_TBL:
		*noOfTableIndices = LSM_INDICES_COUNT;
		*noOfCdftIndices = LSM_CDFT_INDICES_COUNT;
		*noOfCmdIndices = LSM_CMD_INDICES_COUNT;
		   break;
	   case CDC_FEATURES_TBL:
		*noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
		*nonModuleTblFound = 1;
		*noOfCdftIndices = CDC_FEATURES_DATA_CDFT_INDICES_COUNT;
		*noOfCmdIndices = CDC_FEATURES_DATA_CMD_INDICES_COUNT;
		   break;
	   case ADIE_SIDETONE_TBL:
		*noOfTableIndices = ADST_INDICES_COUNT;
		*noOfCdftIndices = ADST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = ADST_CMD_INDICES_COUNT;
		   break;
	   case AANC_CFG_TBL:
		*noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
		*noOfCdftIndices = AANC_CFG_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AANC_CFG_CMD_INDICES_COUNT;
		   break;
      case VOCPROC_COPP_GAIN_DEP_V2_TBL:
		*noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCTBL_VOL_V2_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROC_VOL_V2_CMD_INDICES_COUNT;
         break;
      case VOICE_VP3_TBL:
		*noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
		*noOfCdftIndices = VOICE_VP3TBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOICE_VP3_CMD_INDICES_COUNT;
         break;
      case AUDIO_REC_VP3_TBL:
		*noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
		*noOfCdftIndices = AUDREC_VP3TBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDREC_VP3_CMD_INDICES_COUNT;
         break;
      case AUDIO_REC_EC_VP3_TBL:
		*noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
		*noOfCdftIndices = AUDREC_EC_VP3TBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDREC_EC_VP3_CMD_INDICES_COUNT;
         break;
      case METAINFO_LOOKUP_TBL:
		*noOfTableIndices = MINFOTBL_INDICES_COUNT;
		*nonModuleTblFound = 1;
		*noOfCdftIndices = MINFOTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = MINFO_CMD_INDICES_COUNT;
         break;
       case VOCPROC_DYNAMIC_TBL:
		*noOfTableIndices = VOCPROCDYNTBL_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCDYNTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROCDYN_CMD_INDICES_COUNT;
		   break;
	   case VOCPROC_STATIC_TBL:
		*noOfTableIndices = VOCPROCSTATTBL_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCSTATTBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROCSTAT_CMD_INDICES_COUNT;
		   break;
	   case VOC_STREAM2_TBL:
		*noOfTableIndices = VOCSTREAM2TBL_INDICES_COUNT;
		*noOfCdftIndices = VOCSTREAM2TBL_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCSTREAM2_CMD_INDICES_COUNT;
		   break;
	case VOCPROC_DYN_INST_TBL_ID:
		*noOfTableIndices = VOCPROCDYNTBL_INSTANCE_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCDYN_INSTANCE_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROCDYN_INSTANCE_CMD_INDICES_COUNT;
		break;
	case VOCPROC_STAT_INST_TBL_ID:
		*noOfTableIndices = VOCPROCSTATTBL_INSTANCE_INDICES_COUNT;
		*noOfCdftIndices = VOCPROCSTAT_INSTANCE_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCPROCSTAT_INSTANCE_CMD_INDICES_COUNT;
		break;
	case VOCSTRM_INST_TBL_ID:
		*noOfTableIndices = VOCSTREAM2TBL_INST_INDICES_COUNT;
		*noOfCdftIndices = VOCSTREAM2TBL_INST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = VOCSTREAM2_INST_CMD_INDICES_COUNT;
		break;
	case AUDPROC_INST_TBL_ID:
		*noOfTableIndices = AUDPROCTBL_INST_INDICES_COUNT;
		*noOfCdftIndices = AUDPROCTBL_INST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDPROC_INST_CMD_INDICES_COUNT;
		break;
	case AUDPROCVOL_INST_TBL_ID:
		*noOfTableIndices = AUDPROCTBL_INST__GAIN_DEP_INDICES_COUNT;
		*noOfCdftIndices = AUDPROCTBL_INST_GAIN_DEP_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDPROC_INST_GAIN_DEP_CMD_INDICES_COUNT;
		break;
	case AFECMN_INST_TBL_ID:
		*noOfTableIndices = AFECMNTBL_INST_INDICES_COUNT;
		*noOfCdftIndices = AFECMNTBL_INST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AFECMN_INTS_CMD_INDICES_COUNT;
		break;
	case AUDSTRM_INST_TBL_ID:
		*noOfTableIndices = AUDSTREAMTBL_INST_INDICES_COUNT;
		*noOfCdftIndices = AUDSTREAMTBL_INST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = AUDSTREAM_INST_CMD_INDICES_COUNT;
		break;
	case LSM_INST_TBL_ID:
		*noOfTableIndices = LSM_INST_INDICES_COUNT;
		*noOfCdftIndices = LSM_INST_CDFT_INDICES_COUNT;
		*noOfCmdIndices = LSM_INST_CMD_INDICES_COUNT;
		break;
	case CODEC_COMMON_TBL:
		*noOfTableIndices = CODEC_PP_COMMON_TABLE_INDICES_COUNT;
		*noOfCdftIndices = CODEC_PP_COMMON_CDFT_INDICES_COUNT;
		*noOfCmdIndices = CODEC_PP_COMMON_CMD_INDICES_COUNT;
			break;
	case CODEC_GAIN_TBL:
		*noOfTableIndices = CODEC_PP_GAIN_TABLE_INDICES_COUNT;
		*noOfCdftIndices = CODEC_PP_GAIN_CDFT_INDICES_COUNT;
		*noOfCmdIndices = CODEC_PP_GAIN_CMD_INDICES_COUNT;
			break;
      default:
		ACDB_DEBUG_LOG("Requested indices count for unknown Table ID : %u",tblId);
         return -1;
	}
	return ACDB_SUCCESS;
}