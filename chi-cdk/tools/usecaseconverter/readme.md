########################################################################################################################
#######
# Copyright (c) 2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
########################################################################################################################
#######

# Usecase Composer

`usecasecomposer.py` adds semantics similiar to C's `#include "someFile"`
when creating Usecase xml files. This enables:

1) Splitting Usecase definitions into different files
2) Defining pipelines in terms of reusable, target independent segments

While Pipelines can include other files directly, they may also include
segments. From a Pipeline's perspective, a segment refers to a logical unit such
as: the realtime segment or the jpeg encoding segment. The concrete
implementations may be defined as common implementation or as a target specific
implementation.

## Definitions

- segment: A named, reusable Pipeline segment
- common: Hardware independent segments should be defined in common
- target: This refers to the hw target

## XML Syntax

The `<PipelineSegment>` tag is nearly identical to a `<Pipeline>` tag. The name
of the PipelineSegment **must** be provided as the attribute `name`. A
`<PipelineSegment>` **must** have a `<NodesList>` and `<PortLinkages>` as
child elements. Their contents may be left empty if not applicable to
defining the PipelineSegment. The composing script appends the `<NodesList>`
and `<PortLinkages>` of the segment to the `<NodesList>` and `<PortLinkages>`
of the Pipeline including the segment.

```xml
<PipelineSegment name="segmentName">
  <NodesList>
    ...
  </NodesList>
  <PortLinkages>
    ...
  </PortLinkages>
</PipelineSegment>
```

CamxInclude allows for the inclusion of arbitrary xml files and segments.

```xml
<CamxInclude href="relative/path/to/include"/>
```

The href attribute refers to the relative path of the file to include from
the context of the current file.

```xml
<CamxInclude segment="segmentName"/>
```

The segment attribute is the name of the Segment to include. The attribute is currently
only valid within the context of a pipeline xml file (found within a usecase's
pipelines folder).

When including a segment into a pipeline, a common segment definition **must**
exist. Target-specific segments will override common segments
if a segment of an identical name is defined in the target segment directory.

After segment inclusions are processed, generated pipelines reside in the 
usecase's g_pipelines folder. These generated pipelines will then be included
into a master g_{target}_usecase.xml file containing all usecase pipelines.

```perl
ROOT_TOPOLOGY_DIRECTORY
|_
  |- usecases
  |  |_
  |    |- usecaseA
  |    |  |_
  |    |    |- pipelines
  |    |    |- g_pipelines
            ...
  |    |- usecaseZ
  |       |_
  |         |- pipelines
  |         |- g_pipelines
  |- segments
       |
       |- common
       |  |_
       |    |- segment1.xml
       |    |- segment2.xml
                  ...
       |    |- segmentN.xml
       |
       |- target
          |_
            |- segment1.xml
            |- segment2.xml
```
