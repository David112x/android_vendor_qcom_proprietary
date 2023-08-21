#!/usr/bin/perl
###############################################################################################################################
# Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
###############################################################################################################################

use strict;
use warnings;
use XML::Simple;
use File::Basename;
use File::Path;
use Data::Dumper;
use feature 'state';

sub GetData
{
    my $xml  = XMLin($_[0], forcearray => [ 'Usecase', 'Pipeline', 'Target', 'Node', 'NodeProperty', 'BufferFlags', 'LinkFlags' , 'TargetFormat' , 'DstPort', 'BypassPortSrcId', 'Link', 'PrunableVariant']);
    #print Dumper($xml)."\n";
    #exit;
    my $usecases = $_[1];

    foreach my $usecase (@{$xml->{Usecase}})
    {
        my $localUsecase = {Name             => $usecase->{UsecaseName},
                            StreamConfigMode => $usecase->{StreamConfigMode},
                            targets          => [],
                            pipelines        => [] };

        if ($usecase->{Targets} && $usecase->{Targets}->{Target})
        {
            foreach my $target (@{$usecase->{Targets}->{Target}})
            {
                # Supporting old format
                $target->{MinW} = $target->{Range}->{MinW};
                $target->{MinH} = $target->{Range}->{MinH};
                $target->{MaxW} = $target->{Range}->{MaxW};
                $target->{MaxH} = $target->{Range}->{MaxH};
                push @{$localUsecase->{targets}}, $target;
            }
        }

        foreach my $pipeline (@{$usecase->{Pipeline}})
        {
            my $localPipeline = {Name      => $pipeline->{PipelineName},
                                 nodes     => [],
                                 links     => [],
                                 sinkCount => 0};

            if ($pipeline->{NodesList})
            {
                foreach my $node (@{$pipeline->{NodesList}->{Node}})
                {
                    if ($node->{NodeProperty})
                    {
                        foreach my $prop (@{$node->{NodeProperty}})
                        {
                            $prop->{Name} = 'NodeProperty';
                            push @{$node->{property}}, $prop;
                        }
                    }
                    FindVariantInfo($node);

                    push @{$localPipeline->{nodes}}, $node;
                }
            }

            if ($pipeline->{PortLinkages} && $pipeline->{PortLinkages}->{Link})
            {
                foreach my $link (@{$pipeline->{PortLinkages}->{Link}})
                {
                    if ($link->{LinkProperties} && $link->{LinkProperties}->{BatchMode})
                    {
                        $link->{BatchMode} = $link->{LinkProperties}->{BatchMode};
                    }

                    if ($link->{LinkProperties} && $link->{LinkProperties}->{LinkFlags})
                    {
                        $link->{LinkFlags} = $link->{LinkProperties}->{LinkFlags};
                    }

                    $link->{ports}  = [];
                    $link->{SrcPort}->{Name} = 'SrcPort';

                    if ($link->{SrcPort}->{NodeId} == 2 ||
                        $link->{SrcPort}->{NodeId} == 3 ||
                        $link->{SrcPort}->{NodeId} == 4)
                    {
                        $localPipeline->{sinkCount}++;
                    }

                    foreach(@{$link->{DstPort}})
                    {
                        $_->{Name} = 'DstPort';
                        if ($_->{NodeId} == 2 ||
                            $_->{NodeId} == 3 ||
                            $_->{NodeId} == 4)
                        {
                            $localPipeline->{sinkCount}++;
                        }
                        FindVariantInfo($_);
                        push @{$link->{ports}}, $_;
                    }

                    push @{$link->{ports}}, $link->{SrcPort};

                    if ($link->{BufferProperties})
                    {
                        $link->{buffer} = $link->{BufferProperties};
                    }

                    push @{$localPipeline->{links}}, $link;
                }
            }
            push @{$localUsecase->{pipelines}}, $localPipeline;
        }

        push @$usecases, $localUsecase;
    }

    return $usecases;
}

my $variantGroups = {};
my $variantTypes  = {};
sub HashRefToCEnum
{
    my $enumName = shift;
    my $rHash    = shift;
    my $prefix   = shift;
    my $result   = "// Enumerator: $enumName\nenum E$enumName\n{\n";
    my $i        = 1;
    $result .= "    Invalid$enumName = 0,\n";
    foreach my $key (sort keys %{$rHash})
    {
        $result .= "    $prefix$key = $i,\n";
        $rHash->{$key} = $i;
        $i++;
    }
    $result .= "};\n";
    $result .= "static const CHAR* g_stringMap$enumName" . "[]\n{\n";
    foreach my $key (sort keys %{$rHash})
    {
        $result .= "    \"$key\",\n";
    }
    $result .= "};\n";
    return "$result\n";
}

sub FindVariantInfo
{
    my $node = shift;
    if ($node->{PrunableVariant})
    {
        foreach my $rVariant (@{$node->{PrunableVariant}})
        {
            my $group    = $rVariant->{variantGroup};
            my $typename = $rVariant->{variantType};
            if (not defined $variantGroups->{$group})
            {
                $variantGroups->{$group} = 0;
            }
            if (not defined $variantTypes->{$typename})
            {
                $variantTypes->{$typename} = 0;
            }
        }
    }
}
sub GetVariantInfo
{
    my $node         = shift;
    my $result       = 0;
    my $pruneId      = "NULL";
    my $cVariantList = "";
    my $rVariants    = 0;
    if ($node->{PrunableVariant})
    {
        my @variants;
        my $variantMap = {};
        foreach my $rVariant (@{$node->{PrunableVariant}})
        {
            my $group    = "PruneGroup" . $rVariant->{variantGroup};
            my $typename = "PruneType"  . $rVariant->{variantType};
            if (not defined $variantMap->{$group})
            {
                $variantMap->{$group} = [];
            }
            my $cObjForm = "{$group, $typename}";
            push @{$variantMap->{$group}}, ($cObjForm);
            $result++;
        }
        foreach my $group (sort keys(%{$variantMap}))
        {
            foreach my $cObjForm (@{$variantMap->{$group}})
            {
                push @variants, $cObjForm;
            }
        }
        state $s_pruneId = 0;
        $pruneId = "s_pruneGroup" . "$s_pruneId";
        $s_pruneId++;
        $cVariantList = "{" . join(", ", @variants) . "}";
        $cVariantList = "static const PruneVariant $pruneId" . "[] = $cVariantList;\n";
        $rVariants    = \@variants;
    }
    return $result, $pruneId, $cVariantList, $rVariants;
}

sub NodeToComment {
    my $sourceNode = shift;
    return $sourceNode->{NodeName} . $sourceNode->{NodeInstanceId};
}

my ($input1, $input2, $output) = @ARGV;

if (!$input1 || !$input2)
{
    print STDERR "Usage: $0 <path to usecase XML> <optional:path to target usecase XML> <output>\n\n";
    print STDERR "Invalid: `$0 $input1 $input2 $output`\n";
    exit 1;
}

if (!-e $input1)
{
    print STDERR "Usage: $0 <path to usecase XML> <optional:path to target usecase XML> <output>\n\n";
    print STDERR "$input1 not found\n";
    exit 1;
}

if (3==@ARGV && !-e $input2)
{
    print STDERR "Usage: $0 <path to usecase XML> <optional:path to target usecase XML> <output>\n\n";
    print STDERR "$input2 not found\n";
    print STDERR "[WARNING] Will generate $output using $input1 only!\n";
}

if (2==@ARGV)
{
    #In case 1 XML file was used, use the second command line input arguement as $output
    $output = $input2;
}

my $selfUsecases = GetData($input1, ); # The second arguement is intentionally left blank
# Process the second XML file
if (3 == @ARGV && -e $input2)
{
    $selfUsecases = GetData($input2, $selfUsecases);
}
my $outFileObj = DiffFileHandle->new($output);
my $FILE = $outFileObj->GetHandle();

# Header
print $FILE "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef USECASEDEFS_H
#define USECASEDEFS_H

";
if ($input1 =~ /.*(nchi_titan_topology.xml|nchi_topology.xml|nchi_mimas_topology.xml)$/)
{
    print("INFO: generating header for native tests");
    print $FILE "\n#include \"nchicommon.h\"\n";
}
else
{
    print $FILE "\n#include \"chi.h\"\n";
}

print $FILE "// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

#ifdef __cplusplus
extern \"C\"
{
#endif // __cplusplus

/// \@brief ranges of buffer sizes supported
struct BufferDimension
{
    UINT minWidth;
    UINT minHeight;
    UINT maxWidth;
    UINT maxHeight;
};

/// \@brief Collection of information describing how a buffer is used
struct ChiTarget
{
    ChiStreamType       direction;
    BufferDimension     dimension;
    UINT                numFormats;
    ChiBufferFormat*    pBufferFormats;
    ChiStream*          pChiStream;
};

/// \@brief Information regarding where a port interacts with a buffer directly
struct ChiTargetPortDescriptor
{
    const CHAR*            pTargetName;
    ChiTarget*             pTarget;
    UINT                   numNodePorts;
    ChiLinkNodeDescriptor* pNodePort;
};

/// \@brief List of port->buffer information
struct ChiTargetPortDescriptorInfo
{
    UINT                     numTargets;
    ChiTargetPortDescriptor* pTargetPortDesc;
};

/// \@brief Combination of pipeline information with buffer information
struct ChiPipelineTargetCreateDescriptor
{
    const CHAR*                 pPipelineName;
    ChiPipelineCreateDescriptor pipelineCreateDesc;
    ChiTargetPortDescriptorInfo sinkTarget;
    ChiTargetPortDescriptorInfo sourceTarget;
};

/// \@brief Collection of information summarizing a usecase
struct ChiUsecase
{
    const CHAR*                        pUsecaseName;
    UINT                               streamConfigMode;
    UINT                               numTargets;
    ChiTarget**                        ppChiTargets;
    UINT                               numPipelines;
    ChiPipelineTargetCreateDescriptor* pPipelineTargetCreateDesc;
    const PruneSettings*               pTargetPruneSettings;
    BOOL                               isOriginalDescriptor;
};

/// \@brief Collection of usecases with matching properties (target count at this point)
struct ChiTargetUsecases
{
    UINT        numUsecases;
    ChiUsecase* pChiUsecases;
};

";
my $numUsecases = 0;
my $usecaseNames = {};
my $orderedUsecaseNames = [];

print $FILE HashRefToCEnum("VariantGroup", $variantGroups, "PruneGroup");
print $FILE HashRefToCEnum("VariantType", $variantTypes, "PruneType");

# Sort by number of targets, then by name
foreach my $usecase (sort {scalar(@{$a->{targets}}) <=> scalar(@{$b->{targets}})} sort {$a->{Name} cmp $a->{Name}} @{$selfUsecases})
{
    my $pipelines   = {};
    my $usecasename = $usecase->{Name};

    $usecaseNames->{$usecasename} = $usecase;

    push @$orderedUsecaseNames, $usecasename;

    $numUsecases++;

    print $FILE "/*==================== USECASE: $usecasename =======================*/\n\n";

    # Transform array of targets to hash for easy lookup
    my $targetMap = {};
    foreach my $target (@{$usecase->{targets}})
    {
        $targetMap->{$target->{TargetName}} = $target;
        $target->{pruningInfo}              = {};

        # Generate arrays of formats to be included in the targets themselves
        print $FILE "static ChiBufferFormat $usecasename\_$target->{TargetName}\_formats[] =\n{\n";
        foreach my $format (sort @{$target->{TargetFormat}})
        {
            print $FILE "    $format,\n";
        }
        print $FILE "};\n\n";
    }

    #struct ChiTarget
    #    ChiStreamType       direction;
    #    BufferDimension     dimension;
    #    UINT                numFormats;
    #    ChiBufferFormat*    pBufferFormats;
    #    ChiStream*          pChiStream;
    my $targetArray = [];
    foreach my $target (@{$usecase->{targets}})
    {
        $target->{TargetDirection}=~s/Target/ChiStreamType/;
        print $FILE "static ChiTarget $usecasename\_$target->{TargetName}\_target =\n".
        "{\n".
        "    $target->{TargetDirection},\n".
        "    {   // (MinW, MinH, MaxW, MaxH)\n ". 
        "       $target->{MinW}, $target->{MinH}, $target->{MaxW}, $target->{MaxH}\n".
        "    },\n".
        "    ".scalar(@{$target->{TargetFormat}}).",\n".
        "    $usecasename\_$target->{TargetName}\_formats,\n".
        "    NULL\n".
        "}; // $target->{TargetName}\n\n";
        push @$targetArray, "\&$usecasename\_$target->{TargetName}\_target";
    }

    print $FILE "static ChiTarget* $usecasename"."_Targets[] =\n{\n\t".join(",\n\t", @$targetArray)."\n};\n\n";
    my $pipelineNames  = {};
    my $pipelineEnums  = "enum ".$usecasename."PipelineIds\n{\n";
    my $pipelineId     = 0;

    # Essentially a DFS generating header stuff

    # Sort according to sinkCount, then by name
    foreach my $pipeline (sort {$a->{sinkCount} <=> $b->{sinkCount}} sort {$a->{Name} cmp $a->{Name}} @{$usecase->{pipelines}})
    {
        my $index = 0;
        my $pipelinename = $pipeline->{Name};
        while ($pipelineNames->{$pipelinename})
        {
            $index++;
            $pipelinename = $pipeline->{Name}."$index";
        }
        $pipelineNames->{$pipelinename} = 1;

        my $name = "$usecasename\_$pipelinename";

        $pipelineEnums .= "    ".$pipelinename." = ".$pipelineId.",\n";
        $pipelineId++;

        print $FILE "/*****************************Pipeline $pipelinename***************************/\n\n";

        my $isRT = 0;

        # parse out all ports for each node
        my $nodePorts        = {};
        my $nodeLookups      = {};
        my $linkNum          = 0;
        my $numLinkOutput    = [];
        my $sinkTargetList   = "";
        my $sourceTargetList = "";
        my $numSinkTargets   = 0;
        my $numSourceTargets = 0;
        my $linkcount = 0;

        # Use the links to determine the active nodes
        foreach my $link (@{$pipeline->{links}})
        {
            # While we're going through the nodes, setup the dest descriptors
            my $linkDest    = "static ChiLinkNodeDescriptor ";
            my $pruneGroups = "";
            # Setup lookup for each node containing an active src or dest port

            my $isSink   = 0;
            my $isSource = 0;
            my $isBuffer = 0;
            my $targetName;
            my $sourceNodeName;
            my $sourceNodeId;
            my $sourceNodeInstanceId;
            my $sourcePortId;
            my $numSourceTargetsCount = 0;
            my @targetNodeListarray;

            foreach my $port (@{$link->{ports}})
            {
                $isBuffer = 1 if ($port->{NodeId} == 2 || $port->{NodeId} == 4);
                $isSink   = 1 if ($port->{NodeId} == 2 || $port->{NodeId} == 3);
                $isSource = 1 if ($port->{NodeId} == 4);
                if ($port->{NodeId} == 2 || $port->{NodeId} == 4)
                {
                    if ($targetName && !($port->{PrunableVariant}))
                    {
                        die "Already have target $targetName in link trying to add $port->{PortName} from $usecasename $pipelinename\n";
                    }
                    $targetName = $port->{PortName};
                }
                if ($port->{Name} eq 'SrcPort')
                {
                    $sourceNodeName       = NodeToComment($port);
                    $sourceNodeId         = $port->{NodeId};
                    $sourceNodeInstanceId = $port->{NodeInstanceId};
                    $sourcePortId         = $port->{PortId};
                }
            }

            foreach my $port (@{$link->{ports}})
            {
                my $isSinkPort = ($port->{NodeId} == 2);
                # Note that this is a blind trust. The name must match something in the usecase's targets or the produced header will fail compilation
                if ($targetName && (($isSinkPort != 1) && $port->{NodeId} != 4))
                {
                    my $nodeId     = $port->{NodeId};
                    my $nodeIn     = $port->{NodeInstanceId};
                    my $nodePort   = $port->{PortId};

                    if ($isSource)
                    {
                        $numSourceTargetsCount++;
                        push(@targetNodeListarray, "{$nodeId, $nodeIn, $nodePort}");
                    }
                }
            }
            if($numSourceTargetsCount >= 1)
            {
               print $FILE "static ChiLinkNodeDescriptor "."$name"."_source_Link_"."$linkcount"."NodeDescriptors[] =\n{\n";
               foreach my $n (@targetNodeListarray)
               {
                  print $FILE "    $n,\n";
               }
               print $FILE "};\n";
               $sourceTargetList .= "    {\"$targetName\", \&$usecasename\_$targetName\_target, $numSourceTargetsCount, $usecasename\_$pipelinename\_source_Link_$linkcount"."NodeDescriptors}, // $targetName\n";
               $numSourceTargets++;
            }

            my $identifier = $name . "_Link" . $linkNum . "_$sourceNodeName" . "_Out" . $sourcePortId;
            $link->{identifier} = $identifier. "Desc";
            $linkDest .= $link->{identifier} . "[] =\n{\n";
            foreach my $port (@{$link->{ports}})
            {
                $port->{isSink}   = $isSink;
                $port->{isBuffer} = 0;
                my $id  = $port->{NodeId}."_".$port->{NodeInstanceId};
                my $dir = $port->{Name};   # SrcPort or DstPort
                my $isDstPort  = ($dir eq 'DstPort');
                my $isSrcPort  = ($dir eq 'SrcPort');
                my $isSinkPort = ($port->{NodeId} == 2);
                if($isBuffer eq 1) {
                    if ($isSource || $isSrcPort) {
                        $port->{isBuffer} = 1;
                    }
                    elsif ($isDstPort && $isSinkPort) {
                        $port->{isBuffer} = 1;
                    }
                }
                # Note that this is a blind trust. The name must match something in the usecase's targets or the produced header will fail compilation
                if ($targetName)
                {
                    my $nodeId     = $port->{NodeId};
                    my $nodeIn     = $port->{NodeInstanceId};
                    my $nodePort   = $port->{PortId};

                    if ($isSink && $isSinkPort)
                    {
                        $targetName    = $port->{PortName};
                        state $descId  = 0;
                        my $descName = "__$descId\_$targetName\_Link_NodeDescriptors";
                        $descId++;
                        $sinkTargetList .= "    {\"$targetName\", \&$usecasename\_$targetName\_target, 1, $descName}, // $targetName\n";
                        $numSinkTargets++;
                        print $FILE "static ChiLinkNodeDescriptor $descName\[] =\n{\n";
                        print $FILE "    {$sourceNodeId, $sourceNodeInstanceId, $sourcePortId }, \n";
                        print $FILE "};\n";
                    }
                }

                # TDEV fix for testgen tests (node 65536:IFE and node 65544:TFE)
                if ($input1 =~ /.*(nchi_titan_topology.xml|nchi_topology.xml|nchi_mimas_topology.xml)$/)
                {
                    if ($port->{NodeId} == 0 || $port->{NodeId} == 65536 || $port->{NodeId} == 65544)
                    {
                        $isRT = 1;
                    }
                }
                elsif ($port->{NodeId} == 0)
                {
                    $isRT = 1;
                }

                # Setup a node as active using the ports used
                $nodeLookups->{$port->{NodeId}}->{$port->{NodeInstanceId}} = 1;

                if (!$nodePorts->{$id}->{$dir})
                {
                    $nodePorts->{$id}->{$dir} = [];
                }
                push @{$nodePorts->{$id}->{$dir}}, $port;

                if (($isSource == 0) && (1 == $isDstPort))
                {
                    #CHILINKNODEDESCRIPTOR*  pDestNodes;            ///<  Pointer to all the dest nodes connected to the src node
                    #    UINT32                  nodeId;            ///<   Node identifier
                    #    UINT32                  nodeInstanceId;    ///<   Node instance id
                    #    UINT32                  nodePortId;        ///<   Node port id
                    #    UINT32                  portSourceTypeId;  ///<   Port source type id
                    my $portSrcTypeId = (exists $port->{PortSrcTypeId}) ? $port->{PortSrcTypeId} : "0";
                    my $comment       = "// $port->{NodeName}";

                    my $numberOfVariants = 0;
                    my $pruneId          = 0;
                    my $variants         = "";
                    my $rVariants        = 0;

                    if ($isSinkPort)
                    {
                        $comment .= " ]-> $targetName";
                    }

                    ($numberOfVariants, $pruneId, $variants, $rVariants) = GetVariantInfo($port);
                    if ($numberOfVariants > 0 && $isSinkPort)
                    {
                        my $rTargetPruneInfo = $targetMap->{$targetName}->{pruningInfo};
                        foreach my $pruneVariant (@{$rVariants})
                        {
                            if (not defined $rTargetPruneInfo->{$pruneVariant})
                            {
                                $rTargetPruneInfo->{$pruneVariant} = 0;
                            }
                        }
                    }

                    $pruneGroups .= $variants;
                    $linkDest    .= "    {$port->{NodeId}, $port->{NodeInstanceId}, $port->{PortId}, $portSrcTypeId, {$numberOfVariants, $pruneId}}, $comment\n";
                    $numLinkOutput->[$linkNum]++;
                }
            }
            $linkcount++;
            if ($isSource == 0)
            {
                if($numLinkOutput->[$linkNum])
                {
                    $linkDest .= "};\n\n";
                    print $FILE $pruneGroups;
                    print $FILE $linkDest
                }
                $linkNum++;
            }
        }

        if ($sinkTargetList)
        {
            print $FILE "static ChiTargetPortDescriptor $name\_sink_TargetDescriptors[] =\n{\n$sinkTargetList};\n\n";
        }

        if ($sourceTargetList)
        {
            print $FILE "static ChiTargetPortDescriptor $name\_source_TargetDescriptors[] =\n{\n$sourceTargetList};\n\n";
        }

        # Now that we can know what ports are used by each node, iterate through the nodes
        foreach my $nodeid (sort keys(%{$nodeLookups}))
        {
            foreach my $nodein (sort keys(%{$nodeLookups->{$nodeid}}))
            {
                my $id       = $nodeid."_".$nodein;

                if ($nodeid != 2 && $nodeid != 3 && $nodeid != 4)
                {
                    if ($nodePorts->{$id}->{DstPort} && scalar(@{$nodePorts->{$id}->{DstPort}}))
                    {
                        # generate input descriptions from each port used by node
                        #CHIINPUTPORTDESCRIPTOR* pInputPorts;             ///< Pointer to input ports
                        #    UINT32                  portId;              ///<  Input/Output port id
                        #    UINT32                  isInputStreamBuffer; ///<  Does this input port take a source buffer as
                        #                                                 ///   input
                        #    UINT32                  portSourceTypeId;    ///<  Port source type id
                        print $FILE "// " . NodeToComment($nodePorts->{$id}->{DstPort}[0]) ." Input Port Descriptors\n";
                        print $FILE "static ChiInputPortDescriptor $name"."Node$id"."InputPortDescriptors[] =\n{\n";
                        foreach my $port (@{$nodePorts->{$id}->{DstPort}})
                        {
                            my $portSrcTypeId = (exists $port->{PortSrcTypeId}) ? $port->{PortSrcTypeId} : "0";
                            print $FILE "    {$port->{PortId}, $port->{isBuffer}, $portSrcTypeId}, // $port->{PortName}\n";
                        }
                        print $FILE "};\n\n";
                    }

                    if ($nodePorts->{$id}->{SrcPort} && scalar(@{$nodePorts->{$id}->{SrcPort}}))
                    {
                        foreach my $port (@{$nodePorts->{$id}->{SrcPort}})
                        {
                            my $pMappedSourceIds = $name."Node$id"."MappedSourceIds_$port->{PortId}";
                            if (exists $port->{BypassPortSrcId})
                            {
                                $port->{mappedPortId} = $pMappedSourceIds;
                                print $FILE "static UINT32 $pMappedSourceIds"."[] =\n{\n  ";
                                print $FILE join(", ", @{$port->{BypassPortSrcId}})."\n";
                                print $FILE "};\n\n";
                            }
                        }
                        # generate output descriptions from each port used by node
                        #CHIOUTPUTPORTDESCRIPTOR* pOutputPorts;              ///<   Pointer to output ports
                        #    UINT32                   portId;                ///<    Input/Output port id
                        #    UINT32                   isSinkPort;            ///<    Sink port indicator
                        #    UINT32                   isOutputStreamBuffer;  ///<    Does the port output a stream buffer
                        #    UINT32                   portSourceTypeId;      ///<    Port source type id
                        #    UINT32                   numSourceIdsMapped;    ///<    Number of sources mapped to this output port for bypass
                        #    UINT32*                  pMappedSourceIds;      ///<    Source Ids mapped to this output port for bypass
                        print $FILE "// " . NodeToComment($nodePorts->{$id}->{SrcPort}[0]) ." Output Port Descriptors\n";
                        print $FILE "static ChiOutputPortDescriptor $name"."Node$id"."OutputPortDescriptors[] =\n{\n";
                        foreach my $port (@{$nodePorts->{$id}->{SrcPort}})
                        {
                            my $portSrcTypeId            = (exists $port->{PortSrcTypeId}) ? $port->{PortSrcTypeId} : "0";
                            my $numMappedSourceforBypass = (exists $port->{BypassPortSrcId}) ? scalar(@{$port->{BypassPortSrcId}}) : "0";
                            my $mappedSourceIds          = (exists $port->{BypassPortSrcId}) ? $port->{mappedPortId} : "NULL";
                            print $FILE "    {$port->{PortId}, $port->{isSink}, $port->{isBuffer}, $portSrcTypeId, $numMappedSourceforBypass, $mappedSourceIds}, // $port->{PortName}\n";
                        }
                        print $FILE "};\n\n";
                    }
                }
            }
        }

        #CHINODE*            pNodes;                                 ///< Pipeline nodes
        #    VOID*               pNodeProperties;                    ///<  Properties associated with the node
        #    UINT32              nodeId;                             ///<  Node identifier
        #    UINT32              nodeInstanceId;                     ///<  Node instance identifier
        #    CHINODEPORTS        nodeAllPorts;                       ///<  Information about all ports
        #        UINT32                   numInputPorts;             ///<   Number of input ports
        #        CHIINPUTPORTDESCRIPTOR*  pInputPorts;               ///<   Pointer to input ports
        #        UINT32                   numOutputPorts;            ///<   Number of output ports
        #        CHIOUTPUTPORTDESCRIPTOR* pOutputPorts;              ///<   Pointer to output ports
        my $nodeDef   = "static ChiNode $name"."Nodes[] =\n{\n";
        my $numNodes  = 0;
        my $pruneDefs = "";
        foreach my $nodeid (sort keys(%{$nodeLookups}))
        {
            if ($nodeid != 2 && $nodeid != 3 && $nodeid != 4)
            {
                foreach my $nodein (sort keys(%{$nodeLookups->{$nodeid}}))
                {
                    my $key              = $nodeid."_".$nodein;
                    my $inCount          = $nodePorts->{$key}->{DstPort} ? scalar(@{$nodePorts->{$key}->{DstPort}}) : 0;
                    my $inName           = ($inCount) ? "$name"."Node$key"."InputPortDescriptors" : "NULL";
                    my $outCount         = $nodePorts->{$key}->{SrcPort} ? scalar(@{$nodePorts->{$key}->{SrcPort}}) : 0;
                    my $outName          = ($outCount) ? "$name"."Node$key"."OutputPortDescriptors" : "NULL";
                    my $propsName        = "$name\_node$nodeid\_$nodein\_properties";
                    my $propCount        = 0;
                    my $propString       = "static ChiNodeProperty $propsName\[] =\n{\n";
                    my $numberOfVariants = 0;
                    my $pruneId          = "NULL";
                    my $rVariants        = 0;

                    foreach my $node (@{$pipeline->{nodes}})
                    {
                        if ($node->{NodeId} == $nodeid && $node->{NodeInstanceId} == $nodein)
                        {
                            my $variants;
                            die "More than one node matched for properties" if $propCount;
                            ($numberOfVariants, $pruneId, $variants, $rVariants) = GetVariantInfo($node);
                            $pruneDefs .= $variants;
                            foreach my $prop (@{$node->{property}})
                            {
                                $propCount++;
                                $propString .= "    {$prop->{NodePropertyId}, \"$prop->{NodePropertyValue}\"},\n";
                            }
                        }
                    }

                    if ("0" eq $pruneId)
                    {
                        die "[$name] Node: $nodeid:$nodein was used in a link without being declared in the <NodesList>!\n";
                    }

                    if ($propCount)
                    {
                        print $FILE "$propString};\n\n";
                    }
                    else
                    {
                        $propsName = "NULL";
                    }

                    $nodeDef .= "    {$propsName, $nodeid, $nodein, {$inCount, $inName, $outCount, $outName}, $propCount, {$numberOfVariants, $pruneId}},\n";
                    $numNodes++;
                }
            }
        }
        print $FILE $pruneDefs;
        print $FILE "$nodeDef};\n\n";

        #CHINODELINK*        pLinks;                       ///< Each link descriptor
        #    CHILINKNODEDESCRIPTOR   srcNode;              ///<  Src node in a link
        #        UINT32                  nodeId;           ///<   Node identifier
        #        UINT32                  nodeInstanceId;   ///<   Node instance id
        #        UINT32                  nodePortId;       ///<   Node port id
        #        UINT32                  portSourceTypeId; ///<   Port source type id
        #    UINT32                  numDestNodes;         ///<  Dest nodes in a link that the src node can be connected to
        #    CHILINKNODEDESCRIPTOR*  pDestNodes;           ///<  Pointer to all the dest nodes connected to the src node
        #        UINT32                  nodeId;           ///<   Node identifier
        #        UINT32                  nodeInstanceId;   ///<   Node instance id
        #        UINT32                  nodePortId;       ///<   Node port id
        #    CHILINKBUFFERPROPERTIES bufferProperties;     ///<  Buffer properties
        #        UINT32                  bufferFormat;     ///<   Buffer format
        #        UINT32                  bufferSize;       ///<   Buffer size (in case its a raw bytes buffer)
        #        UINT32                  bufferQueueDepth; ///<   Max buffers that will ever exist on the link
        #        UINT32                  bufferHeap;       ///<   Buffer heap
        #        UINT32                  bufferFlags;      ///<   Buffer flags
        #    CHILINKPROPERTIES       linkProperties;       ///<  Link properties
        #        UINT32                  isBatchedMode;    ///<   Batched mode indicator
        #        UINT32                  linkFlags;        ///<   Link flags
        print $FILE "static ChiNodeLink $name"."Links[] =\n{\n";
        $linkNum = 0;
        foreach my $link (@{$pipeline->{links}})
        {
            my $srcPort;
            foreach my $port (@{$link->{ports}})
            {
                if ($port->{Name} eq 'SrcPort')
                {
                    $srcPort = $port;
                    last;
                }
            }
            die "Could not find srcPort for link $linkNum of $name" if (!$srcPort);

            if ($srcPort->{NodeId} != 4)
            {
                my $numDestNodes = $numLinkOutput->[$linkNum];
                my $pDestNodes   = ($numDestNodes) ? $link->{identifier} : "NULL";

                my $buffer = ($link->{buffer}) ? "{$link->{buffer}->{BufferFormat}, ".
                                                ($link->{buffer}->{BufferSize} || 0).", ".
                                                "$link->{buffer}->{BufferImmediateAllocCount}, ".
                                                "$link->{buffer}->{BufferQueueDepth}, ".
                                                "BufferHeap$link->{buffer}->{BufferHeap}, ".
                                                join("|", @{$link->{buffer}->{BufferFlags}})."}" :
                                                "{0}";

                my $linkflags = ($link->{LinkProperties}->{LinkFlags}) ? join ("|", @{$link->{LinkProperties}->{LinkFlags}}) : "0";

                my $portSrcTypeId = (exists $srcPort->{PortSrcTypeId}) ? $srcPort->{PortSrcTypeId} : "0";
                print $FILE "    {{$srcPort->{NodeId}, $srcPort->{NodeInstanceId}, $srcPort->{PortId}, $portSrcTypeId}, ".
                            "$numDestNodes, ".
                            "$pDestNodes, ".
                            "$buffer, ".
                            "{".(($link->{BatchMode} && $link->{BatchMode}=~/true/i) ? "1" : "0"). ", ".                             "$linkflags". "}},\n";

                $linkNum++;
            }
        }
        print $FILE "};\n\n";

        #typedef struct ChiPipelineTargetCreateDescriptor
        #{
        #    ChiPipleineCreateDescriptor
        #        UINT32              size;                                   ///< Size of this structure
        #        UINT32              numNodes;                               ///< Number of pipeline nodes
        #        CHINODE*            pNodes;                                 ///< Pipeline nodes
        #        UINT32              numLinks;                               ///< Number of links
        #        CHINODELINK*        pLinks;                                 ///< Each link descriptor
        #        UINT32              numTargets                              ///< Number of targets
        #        CHITARGETPORTDESCRIPTOR* pTargets;                          ///< Targets with ports
        #        UINT32              isRealTime;                             ///< Is this a realtime pipeline
        #    ChiTargetDescriptor
        #        UINT32                   TargetCount
        #        CHITARGETPORTDESCRIPTOR* pTargetDescriptors
        #}
        my $sinkTargets = ($sinkTargetList) ?     "{$numSinkTargets, $name\_sink_TargetDescriptors}" : "{0, NULL}";
        my $sourceTargets = ($sourceTargetList) ? "{$numSourceTargets, $name\_source_TargetDescriptors}" : "{0, NULL}";
        my $pipelineString = "    {\"$pipelinename\", { 0, $numNodes, $name"."Nodes, $linkNum, $name"."Links, $isRT}, $sinkTargets, $sourceTargets},  // $pipeline->{Name}\n";
        if (!$pipelines->{$pipeline->{sinkCount}})
        {
            $pipelines->{$pipeline->{sinkCount}} = [];
        }
        push @{$pipelines->{$pipeline->{sinkCount}}}, $pipelineString;
    }

    $pipelineEnums .= "};\n\n";
    print $FILE $pipelineEnums;

    print $FILE "static ChiPipelineTargetCreateDescriptor $usecasename\_pipelines[] =\n{\n";
    foreach my $sinkCount (sort {$a <=> $b} keys(%$pipelines))
    {
        print $FILE @{$pipelines->{$sinkCount}};
    }
    print $FILE "};\n\n";

    my $pruneVariantInfo      = "";
    my $pruneInfoId           = $usecasename."TargetPruneInfo";
    my $outputTargetPruneInfo = "static const PruneSettings $pruneInfoId"."[] =\n{\n";
    my $shouldPrint           = 0;
    foreach my $target (@{$usecase->{targets}})
    {
        my $targetName      = $target->{TargetName};
        my $value           = "NULL";
        my $numPruneTargets = keys %{$target->{pruningInfo}};
        my $comment         = " // $targetName";

        if ($numPruneTargets > 0)
        {
            my $targetPruneId = "$usecasename" . "_$targetName" . "_PruneVariants";

            $shouldPrint = 1;
            $comment     = "";
            $value       = "$targetPruneId";

            $pruneVariantInfo .= "static const PruneVariant $targetPruneId"."[] = {\n";
            foreach my $pruneStruct (sort keys %{$target->{pruningInfo}})
            {
                $pruneVariantInfo .= "    $pruneStruct,\n";
            }
            $pruneVariantInfo .= "};\n";
        }

        $outputTargetPruneInfo .= "    {$numPruneTargets, $value},$comment\n"
    }
    $outputTargetPruneInfo .= "};\n";

    if ($shouldPrint)
    {
        $usecase->{pruneInfo} = $pruneInfoId;
        print $FILE "$pruneVariantInfo\n$outputTargetPruneInfo\n";
    }
    else
    {
        $usecase->{pruneInfo} = "NULL";
    }
}

# CHIUSECASE
#     UINT32                       streamMode                         ///< Stream mode
#     UINT32                       numTargets                         ///< Number of targets in pTargets
#     CHITARGET*                   pTargets                           ///< List of targets in the usecase
#     UINT32                       numPiplines                        ///< Number of pipelines in the usecase
#     CHIPIPELINECREATEDESCRIPTOR* pPipelines                         ///< List of pipelines in the usecase

my $targetCount = 0;
my $allUsecases = "static struct ChiTargetUsecases PerNumTargetUsecases[] =\n{";
my $usecaseEnum = "\n";

my @usecaseDefines;

while ($numUsecases > 0)
{
    $targetCount++;

    my $usecaseStructName = "Usecases".$targetCount."Target";
    my $usecasesArray     = "static ChiUsecase ".$usecaseStructName."[] =\n{\n";
    my $count             = 0;

    foreach my $usecaseName (@$orderedUsecaseNames)
    {
        my $usecase      = $usecaseNames->{$usecaseName};
        my $numPipelines = scalar(@{$usecase->{pipelines}});
        my $numTargets   = scalar(@{$usecase->{targets}});

        if ($numTargets == $targetCount)
        {
            if ($count == 0)
            {
                $usecaseEnum .= "enum UsecaseId".$targetCount."Target\n{\n";
            }
            my $usecaseEnumName = $usecaseName."Id";
            my $usecaseDefName  = "g_p".$usecaseName;

            $usecaseEnum .= "    ".$usecaseEnumName;
            push(@usecaseDefines, "#define ".$usecaseDefName." (&$usecaseStructName"."[$usecaseEnumName])");

            if ($count == 0)
            {
                $usecaseEnum .= "  = 0";
            }
            $usecaseEnum .= ",\n";

            $usecasesArray .= sprintf "    {%-70s, $usecase->{StreamConfigMode}, $numTargets, %-70s, $numPipelines, %-70s $usecase->{pruneInfo}, TRUE},\n", "\"$usecaseName\"", "$usecaseName\_Targets", "$usecaseName\_pipelines,";
            $numUsecases--;
            $count++;
        }
    }

    print $FILE "$usecasesArray};\n\n" if ($count);

    $usecaseEnum .= "};\n\n" if ($count);
    $allUsecases .= ($count) ? "\t{$count, $usecaseStructName},\n" : "\t{$count, NULL},\n";
}

$allUsecases .= "};\n\n";

print $FILE "static const UINT ChiMaxNumTargets = ".$targetCount.";\n\n";
print $FILE $allUsecases;
print $FILE $usecaseEnum;

foreach my $usecaseDef (sort(@usecaseDefines)) {
    print $FILE "$usecaseDef\n";
}

print $FILE "#ifdef __cplusplus\n".
            "}\n".
            "#endif // __cplusplus\n\n".

            "#endif // USECASEDEFS_H\n";

$outFileObj->Close();

#*******************************************************************************************************************************
#   DiffFileHandle
#
#   @brief
#       Package to manage a special filehandle that, when closed, writes the output file only if there's a change
#*******************************************************************************************************************************
package DiffFileHandle;

# After close, was the file changed?
sub WasFileChanged
{
    my ($this) = shift;

    die "Can't call WasFileChanged before Close()\n" if $this->{handle};

    return $this->{fileChange};
}

# Get the actual handle that our parent should use.
sub GetHandle
{
    my ($this) = @_;

    return $this->{handle};
}

sub GetFileName
{
    my ($this) = @_;

    return $this->{fileName};
}

sub Close
{
    my ($this) = @_;

    return if not $this->{handle};

    # If we're not writing to a memory buffer, then close the file.
    if ($this->{handleIsFile})
    {
        close $this->{handle};
        $this->{handle} = undef;
        return;
    }

    # Compare to the actual file
    close $this->{handle};   # Close the writable memory buffer
    $this->{handle} = undef;
    my ($memFH, $fileFH, $memLine);
    open($memFH, "<", \$this->{fileBuf}) or die;
    open($fileFH, "<$this->{fileName}") or die "Can't open $this->{fileName} for read\n";

    # ...Do this by comparing each line...
    while ($memLine = <$memFH>)
    {
        my $fileLine = <$fileFH>;
        if ($memLine ne $fileLine)
        {
            $this->{fileChange} = 2;
            last;
        }
    }

    close $memFH;
    close $fileFH;

    # Then write the file if necessary
    if ($this->{fileChange})
    {
        my $outFH;
        open($outFH, ">$this->{fileName}") or die "Can't open $this->{fileName} for writing\n";
        print $outFH $this->{fileBuf};
        close $outFH;
    }
}

sub new
{
    my $class = shift;

    my $this = {
                    fileName => shift,
                    handle   => undef,
                    fileBuf  => "",
                    handleIsFile => undef,
                    fileChange => 0,
               };

    bless $this, $class;

    # If file doesn't exist, life is easy
    if (not -e $this->{fileName})
    {
        $this->{handleIsFile} = 1;
        $this->{fileChange} = 1;

        # Short circuit exit. This is a new file
        open($this->{handle}, ">$this->{fileName}");
        if (!$this->{handle})
        {
            die "Can't create file: $this->{fileName}\n";
        }
        return $this;
    }

    # Make sure the file is writable
    if (not -w $this->{fileName})
    {
        die "$this->{fileName} isn't writable!\n";
    }
    if (not -r $this->{fileName})
    {
        die "$this->{fileName} isn't readable!\n";
    }

    # Open up a handle to memory
    open($this->{handle}, ">", \$this->{fileBuf}) or die;

    return $this;
}



1;
