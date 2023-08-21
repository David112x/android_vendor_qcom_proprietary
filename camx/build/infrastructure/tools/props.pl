#!/usr/bin/perl

################################################################################################################################
# Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
################################################################################################################################

# constructs property IDs, blob, and offsets

use strict;
use warnings;
use XML::Simple;

my ($input, $output) = @ARGV;

if (!$input || !$output)
{
    print STDERR "Usage: $0 <path to usecase XML> <output basepath/basename>\n\nInvalid: `$0 $input $output`";
    exit 1;
}

if (!-e $input)
{
    print STDERR "Usage: $0 <path to usecase XML> <output basepath/basename>\n\n$input not found\n";
    exit 1;
}

my $outFileH = DiffFileHandle->new("$output.h");
my $FILEH    = $outFileH->GetHandle();

my $outFileC = DiffFileHandle->new("$output.cpp");
my $FILEC    = $outFileC->GetHandle();

my $xml      = XMLin($input, forcearray=>['Property']);
my $sections = [qw(MainProperty InternalProperty UsecaseProperty DebugDataProperty)];
my $begins   = {MainProperty      => "PropertyIDPerFrameResultBegin",
                InternalProperty  => "PropertyIDPerFrameInternalBegin",
                UsecaseProperty   => "PropertyIDUsecaseBegin",
                DebugDataProperty => "PropertyIDPerFrameDebugDataBegin"};
my $nodeCompleteCount = 32;
my $linkMetadataCount = 20;

print $FILEC
"////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \@file g_camxproperties.cpp
/// \@brief Define Qualcomm Technologies, Inc. Property IDs and structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include \"camxincs.h\"
#include \"camxpropertyblob.h\"

CAMX_NAMESPACE_BEGIN

";


print $FILEH
"////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \@file g_camxproperties.h
/// \@brief Define Qualcomm Technologies, Inc. Property IDs and structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef G_CAMXPROPERTIES_H
#define G_CAMXPROPERTIES_H

#include \"camxdefs.h\"

CAMX_NAMESPACE_BEGIN



// $nodeCompleteCount Properties reserved for tracking dependencies on the previous request
// Use Node->GetNodeCompleteProperty() to identify the prop
static const UINT NodeCompleteCount = $nodeCompleteCount;

// $linkMetadataCount Properties reserved for tracking link based metadata
static const UINT LinkMetadataCount = $linkMetadataCount;
";


# Create Node complete properties at the end
foreach my $node (0..$nodeCompleteCount-1)
{
    push @{$xml->{MainProperty}->{Property}}, {Name=>"NodeComplete$node", Type=>"BOOL"};
}

# Create Link meta properties at the end
foreach my $lbm (0..$linkMetadataCount-1)
{
    push @{$xml->{MainProperty}->{Property}}, {Name=>"LinkMetadata$lbm", Type=>"INT"};
}

foreach my $section (@$sections)
{
    print $FILEH "\n/* Beginning of $section */\n\n";

    my $offset   = 0;
    my $blobname = $section."Blob";
    my $proplut  = "\n/// \@brief LUT of each member in the property blob\n".
                   "static const PropertyID $section"."LinearLUT[] =\n".
                   "{\n";
    my $blob     = "\n/// \@brief Collection of properties in a struct\n".
                   "struct $blobname\n".
                   "{\n";
    my $offsets  = "\n/// \@brief Offsets of each member in the property blob\n".
                   "static const SIZE_T $section"."Offsets[] =\n".
                   "{\n";
    my $sizes    = "\n/// \@brief Sizes of each member in the property blob\n".
                   "static const SIZE_T $section"."Sizes[] =\n".
                   "{\n";

    my $end = $begins->{$section};
    $end=~s/Begin/End/g;

    printf $FILEC "/// \@brief LUT that allows using the propertyID to determine a string name for the propertyID\n";
    printf $FILEC "///        Indexed with (id & ~PropertyIDPerFrameDebugDataBegin)\n";
    printf $FILEC "const CHAR* p$section"."Strings[%d] =\n{\n", scalar(@{$xml->{$section}->{Property}});

    foreach my $prop (@{$xml->{$section}->{Property}})
    {
        my $propName = "m$prop->{Name}";

        if ($prop->{Type}=~/\*/)
        {
            $propName = "p$prop->{Name}";
        }

        printf $FILEH "static const PropertyID PropertyID%-35s = $begins->{$section} + 0x%02X;\n", $prop->{Name}, $offset++;

        print  $FILEC "    \"PropertyID$prop->{Name}\",\n";

        $proplut .= sprintf "    PropertyID%-s,\n", $prop->{Name};
        $blob    .= sprintf "    %-35s %-35s///< %s\n", $prop->{Type} , "$propName;", $prop->{Name};
        $offsets .= "    offsetof($blobname, $propName),\n";
        $sizes   .= "    sizeof($prop->{Type}),\n";
    }

    print $FILEC "};\n\n";

    printf $FILEH "\nextern const CHAR* p$section"."Strings[%d];\n", scalar(@{$xml->{$section}->{Property}});

    printf $FILEH "\nstatic const PropertyID $end = $begins->{$section} + 0x%02X;\n", $offset - 1;
    print $FILEH "$proplut};\n";
    print $FILEH "$blob};\n";
    print $FILEH "$offsets};\n";
    print $FILEH "$sizes};\n";

    print $FILEH "\n/* End of $section */\n\n";
}

print $FILEH "CAMX_NAMESPACE_END\n".
             "#endif // G_CAMXPROPERTIES_H\n";

print $FILEC "CAMX_NAMESPACE_END\n";

$outFileH->Close();
$outFileC->Close();

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
        if (!$fileLine || $memLine ne $fileLine)
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



