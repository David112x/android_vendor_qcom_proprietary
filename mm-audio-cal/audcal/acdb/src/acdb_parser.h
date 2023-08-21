#ifndef __ACDB_PARSER_H__
#define __ACDB_PARSER_H__
/*===========================================================================
@file acdb_parser.h

The interface of the Acdb Parse project.

This file will handle the parsing of an ACDB file. It will issue callbacks
when encountering useful client information in the ACDB file.

Copyright (c) 2012-2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_parser.h#14 $ */

/*===========================================================================
EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order. Please
use ISO format for dates.

$Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_parser.h#14 $

when who what, where, why
---------- --- -----------------------------------------------------
2013-06-07 avi Support Voice Volume boost feature

========================================================================== */

/* ---------------------------------------------------------------------------
* Include Files
*--------------------------------------------------------------------------- */
#include "acdb_os_includes.h"

/* ---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
*--------------------------------------------------------------------------- */

#define ACDB_PARSE_COMPRESSED 1
#define ACDB_PARSE_SUCCESS 0
#define ACDB_PARSE_FAILURE -1
#define ACDB_PARSE_CHUNK_NOT_FOUND -2
#define ACDB_PARSE_INVALID_FILE -3

#define ACDB_CHUNKID_DATE 0x4445494649444f4dULL // MODIFIED
#define ACDB_CHUNKID_SWNAME 0x20454d414e505753ULL // SWPNAME
#define ACDB_CHUNKID_SWVERS 0x2053524556505753ULL // SWPVERS
#define ACDB_CHUNKID_OEMINFO 0x204f464e494d454fULL // OEMINFO
#define ACDB_CHUNKID_DEVCAT 0x4e49544143564544ULL // DEVCATIN
#define ACDB_CHUNKID_AUDPROCLUT 0x3054554c49474341ULL // ACGILUT0
#define ACDB_CHUNKID_AUDPROCDFT 0x5446444349474341ULL // ACGICDFT
#define ACDB_CHUNKID_AUDPROCDOT 0x544f444349474341ULL // ACGICDOT
#define ACDB_CHUNKID_DATAPOOL 0x4c4f4f5041544144ULL // DATAPOOL
#define ACDB_CHUNKID_VOCPROCLUT 0x3054554c49474356ULL // VCGILUT0
#define ACDB_CHUNKID_VOCPROCDFT 0x5446444349474356ULL // VCGICDFT
#define ACDB_CHUNKID_VOCPROCDOT 0x544f444349474356ULL // VCGICDOT
#define ACDB_CHUNKID_AUDPROGAINDEPCLUT 0x3054554c44474341ULL // ACGDLUT0
#define ACDB_CHUNKID_AUDPROGAINDEPCDFT 0x5446444344474341ULL // ACGDCDFT
#define ACDB_CHUNKID_AUDPROGAINDEPCDOT 0x544f444344474341ULL // ACGDCDOT
#define ACDB_CHUNKID_AUDVOLCLUT 0x3054554c4c4f5641ULL // AVOLLUT0
#define ACDB_CHUNKID_AUDVOLCDFT 0x544644434c4f5641ULL // AVOLCDFT
#define ACDB_CHUNKID_AUDVOLCDOT 0x544f44434c4f5641ULL // AVOLCDOT
#define ACDB_CHUNKID_VOCVOLCLUT 0x3054554c44474356ULL // VCGDLUT0
#define ACDB_CHUNKID_VOCVOLCDFT 0x5446444344474356ULL // VCGDCDFT
#define ACDB_CHUNKID_VOCVOLCDOT 0x544f444344474356ULL // VCGDCDOT
#define ACDB_CHUNKID_AFECLUT 0x3054554c20454641ULL // AFE LUT0
#define ACDB_CHUNKID_AFECDFT 0x5446444320454641ULL // AFE CDFT
#define ACDB_CHUNKID_AFECDOT 0x544f444320454641ULL // AFE CDOT
#define ACDB_CHUNKID_AFECMNCLUT 0x3054554c43454641ULL // AFECLUT0
#define ACDB_CHUNKID_AFECMNCDFT 0x5446444343454641ULL // AFECCDFT
#define ACDB_CHUNKID_AFECMNCDOT 0x544f444343454641ULL // AFECCDOT
#define ACDB_CHUNKID_AUDSTREAMCLUT 0x3054554c4d545341ULL // ASTMLUT0
#define ACDB_CHUNKID_AUDSTREAMCDFT 0x544644434d545341ULL // ASTMCDFT
#define ACDB_CHUNKID_AUDSTREAMCDOT 0x544f44434d545341ULL // ASTMCDOT
#define ACDB_CHUNKID_VOCSTREAMCLUT 0x3054554c4d545356ULL // VSTMLUT0
#define ACDB_CHUNKID_VOCSTREAMCDFT 0x544644434d545356ULL // VSTMCDFT
#define ACDB_CHUNKID_VOCSTREAMCDOT 0x544f44434d545356ULL // VSTMCDOT

#define ACDB_CHUNKID_ADIEANCLUT 0x3054554c20434e41ULL // ANC LUT0
#define ACDB_CHUNKID_ADIELUT 0x203054554c434443ULL // CDCLUT0
#define ACDB_CHUNKID_DEVPROPLUT 0x54554c504f525044ULL // DPROPLUT
#define ACDB_CHUNKID_GLOBALPROPLUT 0x54554c504f525047ULL // GPROPLUT
#define ACDB_CHUNKID_GLOBALLUT 0x2054554c4c424c47ULL // GLBLLUT

//pervocoder changes Need to revisit
#define ACDB_CHUNKID_VOCPROCDYNLUT 0x3054554c59445056ULL // VCGILUT0
#define ACDB_CHUNKID_VOCPROCDYNLUTCVD 0x3044564359445056 // VCGILUT0
#define ACDB_CHUNKID_VOCPROCDYNLUTOFFSET 0x5453464f59445056 // VCGILUT0
#define ACDB_CHUNKID_VOCPROCSTATLUT 0x3054554c54535056ULL // VCGILUT0
#define ACDB_CHUNKID_VOCPROCSTATLUTCVD 0x3044564354535056 // VCGILUT0
#define ACDB_CHUNKID_VOCPROCSTATLUTOFFSET 0x5453464f54535056 // VCGILUT0
#define ACDB_CHUNKID_VOCPROCDYNDFT 0x5446444359445056ULL // VCGICDFT
#define ACDB_CHUNKID_VOCPROCSTATDOT 0x544f444354535056ULL // VCGICDOT
#define ACDB_CHUNKID_VOCPROCSTATDFT 0x5446444354535056ULL // VCGICDFT
#define ACDB_CHUNKID_VOCPROCDYNDOT 0x544f444359445056ULL // VCGICDOT
#define ACDB_CHUNKID_VOCSTREAM2CLUT 0x3054554c32545356ULL // VSTMLUT0
#define ACDB_CHUNKID_VOCSTREAM2CDFT 0x5446444332545356ULL // VSTMCDFT
#define ACDB_CHUNKID_VOCSTREAM2CDOT 0x544f444332545356ULL // VSTMCDOT

#define ACDB_CHUNKID_DEVPAIRCFGCLUT 0x3054554c49504456ULL // VDPILUT0
#define ACDB_CHUNKID_DEVPAIRCFGCDFT 0x5446444349504456ULL // VDPICDFT
#define ACDB_CHUNKID_DEVPAIRCFGCDOT 0x544f444349504456ULL // VDPICDOT

#define ACDB_CHUNKID_LSMLUT 0x3054554c434d534cULL // LSMCLUT0
#define ACDB_CHUNKID_LSMCDFT 0x54464443434d534cULL // LSMCCDFT
#define ACDB_CHUNKID_LSMCDOT 0x544f4443434d534cULL // LSMCCDOT
#define ACDB_CHUNKID_CDCFEATUREDATALUT 0x3054554c53464443ULL // CDFSLUT0

#define ACDB_CHUNKID_ADSTLUT 0x3054554c54534441ULL // ADSTCLUT0
#define ACDB_CHUNKID_ADSTCDFT 0x5446444354534441ULL // ADSTCCDFT
#define ACDB_CHUNKID_ADSTCDOT 0x544f444354534441ULL // ADSTCCDOT

#define ACDB_CHUNKID_AANCLUT 0x3054554c434e4141ULL // AANCLUT0
#define ACDB_CHUNKID_AANCCDFT 0x54464443434e4141ULL // AANCCDFT
#define ACDB_CHUNKID_AANCCDOT 0x544f4443434e4141ULL // AANCCDOT

#define ACDB_CHUNKID_ACSWVERS 0x5352455657534341ULL // ACSWVERS
#define ACDB_CHUNKID_ADSPVERS 0x5352455650534441ULL // ADSPVERS
#define ACDB_CHUNKID_ACSWVER2 0x3252455657534341ULL // ACSWVER2

#define ACDB_CHUNKID_VOCVOL2CLUT 0x3054554c32444756ULL // VCD2LUT0
#define ACDB_CHUNKID_VOCVOL2CDFT 0x5446444332444356ULL // VCD2CDFT
#define ACDB_CHUNKID_VOCVOL2CDOT 0x544f444332444356ULL // VCD2CDOT

#define ACDB_CHUNKID_VOICEVP3CLUT 0x3054554c33505656ULL // VVP3LUT0
#define ACDB_CHUNKID_VOICEVP3CDFT 0x5446444333505656ULL // VVP3CDFT
#define ACDB_CHUNKID_VOICEVP3CDOT 0x544f444333505656ULL // VVP3CDOT

#define ACDB_CHUNKID_AUDIORECVP3CLUT 0x3054554c33505641ULL // AVP3LUT0
#define ACDB_CHUNKID_AUDIORECVP3CDFT 0x5446444333505641ULL // AVP3CDFT
#define ACDB_CHUNKID_AUDIORECVP3CDOT 0x544f444333505641ULL // AVP3CDOT

#define ACDB_CHUNKID_AUDIORECECVP3CLUT 0x3054554c33505645ULL // EVP3LUT0
#define ACDB_CHUNKID_AUDIORECECVP3CDFT 0x5446444333505645ULL // EVP3CDFT
#define ACDB_CHUNKID_AUDIORECECVP3CDOT 0x544f444333505645ULL // EVP3CDOT

#define ACDB_CHUNKID_MINFOLUT 0x54554c4f464e494dULL // MINFOLUT

//WCD9340 Graphite changes
#define ACDB_WDSP_CHUNKID_MODULECALBLUT 0x3054554c41434d57ULL // WMCALUT0
#define ACDB_WDSP_CHUNKID_MODULECALBCDFT 0x5446444341434d57ULL // WMCACDFT
#define ACDB_WDSP_CHUNKID_MODULECALBCDOT 0x544f444341434d57ULL // WMCACDOT

#define ACDB_WDSP_CHUNKID_MODULECONNLUT 0x3054554c4e434d57ULL // WMCNLUT0
#define ACDB_WDSP_CHUNKID_MODULECONNCDFT 0x544644434e434d57ULL // WMCNCDFT
#define ACDB_WDSP_CHUNKID_MODULECONNCDOT 0x544f44434e434d57ULL // WMCNCDOT

#define ACDB_WDSP_CHUNKID_SUBGRAPHCONNLUT 0x3054554c43475357ULL // WSGCLUT0
#define ACDB_WDSP_CHUNKID_SUBGRAPHCONNCDFT 0x5446444343475357ULL // WSGCCDFT
#define ACDB_WDSP_CHUNKID_SUBGRAPHCONNCDOT 0x544f444343475357ULL // WSGCCDOT

#define ACDB_WDSP_CHUNKID_DYNAMICMODULEINFOLUT 0x3054554c494d4457ULL // WDMILUT0
#define ACDB_WDSP_CHUNKID_DYNAMICMODULEINFOCDFT 0x54464443494d4457ULL // WDMICDFT
#define ACDB_WDSP_CHUNKID_DYNAMICMODULEINFOCDOT 0x544f4443494d4457ULL // WDMICDOT

#define ACDB_WDSP_CHUNKID_MODULEINFOLUT 0x3054554c44494d57ULL // WMIDLUT0
#define ACDB_WDSP_CHUNKID_MODULEINFOCDFT 0x5446444344494d57ULL // WMIDCDFT
#define ACDB_WDSP_CHUNKID_MODULEINFOCDOT 0x544f444344494d57ULL // WMIDCDOT

#define ACDB_CHUNKID_WDSP_DEVPROPLUT 0x54554c5052504457ULL // WDPRPLUT
#define ACDB_CHUNKID_WDSP_GLOBALPROPLUT 0x54554c5052504757ULL // WDPRPLUT

#define ACDB_CHUNKID_AUDPROCINSTLUTCHUNKID 0x3054554C49494741ULL //AGIILUT0
#define ACDB_CHUNKID_AUDPROCINSTCDFTCHUNKID 0x5446444349494741ULL //AGIICDFT
#define ACDB_CHUNKID_AUDPROCINSTCDOTCHUNKID 0x544f444349494741ULL //AGIICDOT

#define ACDB_CHUNKID_AUDVOLINSTCLUTCHUNKID 0x3054554C49495641ULL //AVIILUT0
#define ACDB_CHUNKID_AUDVOLINSTCDFTCHUNKID 0x5446444349495641ULL //AVIICDFT
#define ACDB_CHUNKID_AUDVOLINSTCDOTCHUNKID 0x544f444349495641ULL //AVIICDOT

#define ACDB_CHUNKID_AFECMNINSTCLUTCHUNKID 0x3054554C49454641ULL //AFEILUT0
#define ACDB_CHUNKID_AFECMNINSTCDFTCHUNKID 0x5446444349454641ULL //AFEICDFT
#define ACDB_CHUNKID_AFECMNINSTCDOTCHUNKID 0x544f444349454641ULL //AFEICDOT

#define ACDB_CHUNKID_LSMINSTCLUTCHUNKID 0x3054554C494D534CULL //LSMILUT0
#define ACDB_CHUNKID_LSMINSTCDFTCHUNKID 0x54464443494D534CULL //LSMICDFT
#define ACDB_CHUNKID_LSMINSTCDOTCHUNKID 0x544f4443494D534CULL //LSMICDOT

#define ACDB_CHUNKID_AUDSTREAMINSTCLUTCHUNKID 0x3054554C49545341ULL //ASTILUT0
#define ACDB_CHUNKID_AUDSTREAMINSTCDFTCHUNKID 0x5446444349545341ULL //ASTICDFT
#define ACDB_CHUNKID_AUDSTREAMINSTCDOTCHUNKID 0x544f444349545341ULL //ASTICDOT

#define ACDB_CHUNKID_VOCSTREAM2INSTCLUTCHUNKID 0x3054554C49325356ULL //VS2ILUT0
#define ACDB_CHUNKID_VOCSTREAM2INSTCDFTCHUNKID 0x5446444349325356ULL //VS2ICDFT
#define ACDB_CHUNKID_VOCSTREAM2INSTCDOTCHUNKID 0x544f444349325356ULL //VS2ICDOT

#define ACDB_CHUNKID_VOCPROCDYNINSTCLUTCHUNKID 0x3054554C49594456ULL //VDYILUT0
#define ACDB_CHUNKID_VOCPROCDYNINSTOFSTCHUNKID 0x5453464F49594456ULL //VDYIOFST
#define ACDB_CHUNKID_VOCPROCDYNINSTCDFTCHUNKID 0x5446444349594456ULL //VDYICDFT
#define ACDB_CHUNKID_VOCPROCDYNINSTCDOTCHUNKID 0x544f444349594456ULL //VDYICDOT
#define ACDB_CHUNKID_VOCPROCDYNINSTCVDCHUNKID 0x3044564349594456ULL //VDYICVD0

#define ACDB_CHUNKID_VOCPROCSTATINSTCLUTCHUNKID 0x3054554C49545356ULL //VSTILUT0
#define ACDB_CHUNKID_VOCPROCSTATINSTOFSTCHUNKID 0x5453464F49545356ULL //VSTIOFST
#define ACDB_CHUNKID_VOCPROCSTATINSTCDFTCHUNKID 0x5446444349545356ULL //VSTICDFT
#define ACDB_CHUNKID_VOCPROCSTATINSTCDOTCHUNKID 0x544f444349545356ULL //VSTICDOT
#define ACDB_CHUNKID_VOCPROCSTATINSTCVDCHUNKID 0x3044564349545356ULL //VSTICVD0

#define ACDB_CHUNKID_CODECGAINDEPLUT			0x3054554C56434443  //CDCVLUT0
#define ACDB_CHUNKID_CODECGAINDEPCDFT			0x5446444356434443  //CDCVCDFT
#define ACDB_CHUNKID_CODECGAINDEPCDOT			0x544F444356434443  //CDCVCDOT

#define ACDB_CHUNKID_CODECCOMMONLUT				0x3054554C43434443  //CDCCLUT0
#define ACDB_CHUNKID_CODECCOMMONCDFT			0x5446444343434443  //CDCCCDFT
#define ACDB_CHUNKID_CODECCOMMONCDOT			0x544F444343434443  //CDCCCDOT
/* ---------------------------------------------------------------------------
* Type Declarations
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Class Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Function Declarations and Documentation
*--------------------------------------------------------------------------- */

int32_t AcdbFileGetChunkData(const uint8_t *pFileBuf,const uint32_t nFileBufLen,uint64_t chunkID,uint8_t **pChkBuf,uint32_t *pChkLen);
int32_t IsAcdbFileValid(const uint8_t *pFileBuf,const uint32_t nFileSize);
int32_t IsAcdbFileZipped(const uint8_t *pFileBuf,const uint32_t nFileSize);
int32_t AcdbFileGetVersion(const uint8_t *pFileBuf,const uint32_t nFileSize,uint32_t *pMajVer,uint32_t *pMinVer, uint32_t *pRevVer);
int32_t IsCodecFileType(const uint8_t *pFileBuf,const uint32_t nFileSize);
int32_t IsGlobalFileType(const uint8_t *pFileBuf,const uint32_t nFileSize);
int32_t IsAVFileType(const uint8_t *pFileBuf,const uint32_t nFileSize);
int32_t AcdbFileGetSWVersion(const uint8_t *pFileBuf,const uint32_t nFileSize,uint32_t *pMajVer,uint32_t *pMinVer, uint32_t *pRevVer, uint32_t *pCplInfo);
int32_t IsAcdbAMDBFileValid(const uint8_t *pFileBuf,const uint32_t nFileSize);

#endif /* __ACDB_PARSER_H__ */
