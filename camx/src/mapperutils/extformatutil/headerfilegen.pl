#!/usr/bin/perl
###############################################################################################################################
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
###############################################################################################################################

use strict;
use warnings;
use XML::Simple;
use File::Basename;
use File::Path;
use Data::Dumper;

sub GetData
{
    #print STDERR "GetData is called, in perl\n\n";
    my $xml  = XMLin($_[0], forcearray => [ 'MapperUtils', 'FormatProperties']);
    #print Dumper($xml)."\n";
    #exit;
    my $mapperutils = $_[1];

    foreach my $mapperEntry (@{$xml->{MapperUtils}})
    {
        my $localMapperEntry = {formatProperties          => [] };

        if ($mapperEntry->{FormatProperties})
        {
            foreach my $FormatEntry (@{$mapperEntry->{FormatProperties}})
            {
                # Supporting old format
                $FormatEntry->{BufferFormat}         = $FormatEntry->{BufferFormat};
                $FormatEntry->{BufferStrideAlign}    = $FormatEntry->{BufferStrideAlign};
                $FormatEntry->{BufferScanLineAlign}  = $FormatEntry->{BufferScanLineAlign};
                $FormatEntry->{BufferPlaneAlign}     = $FormatEntry->{BufferPlaneAlign};
                $FormatEntry->{PrivateFormat}        = $FormatEntry->{PrivateFormat};
                $FormatEntry->{BatchCount}           = $FormatEntry->{BatchCount};
                push @{$localMapperEntry->{formatProperties}}, $FormatEntry;
            }
        }

        push @$mapperutils, $localMapperEntry;
    }

    return $mapperutils;
}

my ($input1, $input2, $output) = @ARGV;

if (!$input1 || !$input2 )
{
    print STDERR "Usage: $0 <path to common xml> <path to format mapper util XML> <output>\n\n";
    print STDERR "Invalid: `$0 $input1 $input2 $output`\n";
    exit 1;
}

if (!-e $input1)
{
    print STDERR "Usage: $0 <path to common xml> <path to format mapper util XML> <output>\n\n";
    print STDERR "$input1 not found\n";
    exit 1;
}

if (3 == @ARGV && !-e $input2)
{
    print STDERR "Usage: $0 <path to common xml> <path to format mapper util XML> <output>\n\n";
    print STDERR "$input2 not found\n";
    print STDERR "[WARNING] Will generate $output using $input1 only!\n";
    # exit 1;
}

if (2==@ARGV)
{
    #In case 1 XML file was used, use the second command line input arguement as $output
    $output = $input2;
}

my $fomatmapperutilentries = GetData($input1, ); # The second arguement is intentionally left blank
# Process the second XML file
if (3 == @ARGV && -e $input2)
{
    $fomatmapperutilentries = GetData($input2, $fomatmapperutilentries);
}
my $outFileObj = DiffFileHandle->new($output);
my $FILE = $outFileObj->GetHandle();
print STDERR "Output header is opened to write, name: $output\n\n";

# Header
print $FILE "////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMERA_EXT_FORMAT_H
#define CAMERA_EXT_FORMAT_H

#include \"camxformatutilexternal.h\"

// NOWHINE ENTIRE FILE - Bypassing for autogen header files

#ifdef __cplusplus
extern \"C\"
{
#endif // __cplusplus

/// \@brief Camera External format info
struct CameraExternalImageFormat
{
    CamxPixelFormat format;           ///< Format type
    UINT            strideAlign;      ///< stride Align value
    UINT            scanLineAlign;    ///< scanLine align value
    UINT            planeAlign;       ///< plane size Align value
    UINT            isPrivateFormat;  ///< is this Camera specific private format
                                      ///< If 1, alignment values given above will be used
                                      ///< If 0, camx will get alignment values from Venus
    UINT            batchCount;       ///< format batch Count value
};

";
my $mapperentrycount = 0;
my $usecaseNames = {};
my $orderedUsecaseNames = [];

# Sort by number of targets, then by name
foreach my $mapperEntry (@{$fomatmapperutilentries})
{
    my $formatentries   = {};

    $mapperentrycount++;

    print $FILE "/*==================== CameraExternalImageFormat array =======================*/\n\n";
    # Generate arrays of formats to be included in the headerfile themselves
    print $FILE "static CameraExternalImageFormat mapperUtil_formats[] =\n{\n";


    #struct CameraExternalImageFormat
    #    UINT format;
    #    UINT strideAlign;
    #    UINT scanLineAlign;
    #    UINT planeAlign;
    #    UINT isPrivateFormat;
    #    UINT batchCount;
    my $formatArray = [];
    foreach my $formatentry (@{$mapperEntry->{formatProperties}})
    {
        print $FILE " {$formatentry->{BufferFormat}, $formatentry->{BufferStrideAlign}, $formatentry->{BufferScanLineAlign},";
        print $FILE " $formatentry->{BufferPlaneAlign}, $formatentry->{PrivateFormat}, $formatentry->{BatchCount} }";
        if (1 == $formatentry->{PrivateFormat})
        {
            print $FILE ", // alignment info given for this entry will be used\n"
        }
        else
        {
            print $FILE ", // Venus defined alignment will be used\n"
        }

    }
    print $FILE "}; // mapperutil entries array\n\n";

}

print $FILE "#ifdef __cplusplus\n".
            "}\n".
            "#endif // __cplusplus\n\n".

            "#endif // CAMERA_EXT_FORMAT_H\n";

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
