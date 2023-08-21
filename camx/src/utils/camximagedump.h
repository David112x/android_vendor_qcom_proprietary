////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximagedump.h
/// @brief  Utility functions for dumping images to files.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGEDUMP_H
#define CAMXIMAGEDUMP_H

#include "camxformats.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

// forward decls
class ImageBuffer;

static const UINT32 PatternGridLength      = 20;   ///< Length of the grid in a segment
static const UINT32 PatternGridsPerSegment = 3;    ///< Number of grids per segment
static const UINT32 PatternSegmentSize     = PatternGridLength * PatternGridLength * PatternGridsPerSegment; ///< Size of
                                                                                                             ///  segment
static const UINT32 PatternSegmentCount    = 5;    ///< Number of segments per pattern
static const UINT32 PatternCount           = 10;   ///< Number of different patterns

/// @brief Watermark pattern structure
struct WatermarkPattern
{
    BYTE   pattern[PatternCount][PatternSegmentCount * PatternSegmentSize]; ///< Watermark pattern of the 0-9 digits each made
                                                                            ///  up of 5 segments of 3 grid pieces
    UINT32 watermarkOffsetX;                                                ///< X directional offset to place the watermark
    UINT32 watermarkOffsetY;                                                ///< Y directional offset to place the watermark
};

/// @brief details for dumping the image
struct ImageDumpInfo
{
    const CHAR*         pPipelineName;      ///< Pipeline name (string)
    UINT32              requestId;          ///< Session-based request ID
    const CHAR*         pNodeName;          ///< Node name (string)
    UINT32              nodeInstance;       ///< Node instance
    UINT32              portId;             ///< port ID (Input/Output)
    UINT32              batchId;            ///< Batch index (0 if no batching)
    UINT32              numFramesInBatch;   ///< Batch size
    const ImageFormat*  pFormat;            ///< Pointer to image format
    UINT32              width;              ///< width in pixels for the format
    UINT32              height;             ///< height in pixels for the format
    const BYTE*         pBaseAddr;          ///< CPU accessible base pointer to image data
    WatermarkPattern*   pWatermarkPattern;  ///< If watermarking, then this pattern will be used
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Static utility class for dumping images to file.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageDump
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  Initialize format params.  Note, buffers must have been allocated with the appropriate CPU usage flag set for
    ///         this function to work properly
    ///
    /// @param  pDumpInfo   The image details
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID Dump(
        const ImageDumpInfo* pDumpInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeWatermarkPattern
    ///
    /// @brief  Initialize the Watermark pattern data.
    ///
    /// @param  pPattern The pattern to initialize
    ///
    /// @return CamxResult return CamxResultSuccess if successful initialization
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult InitializeWatermarkPattern(
        WatermarkPattern* pPattern);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Watermark
    ///
    /// @brief  Watermark requestId onto frame.  Note, buffers must have been allocated with the appropriate CPU usage flag
    ///         set for this function to work properly
    ///
    /// @param  pDumpInfo   The image details
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID Watermark(
        const ImageDumpInfo* pDumpInfo);

private:

    ImageDump()                                    = delete;
    ImageDump(const ImageDump& rOther)             = delete;
    ImageDump& operator=(const ImageDump& rOther)  = delete;
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGEDUMP_H
