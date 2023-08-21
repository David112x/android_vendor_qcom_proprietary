#!/usr/bin/perl -w

#*******************************************************************************************************************************
# Copyright (c) 2016-2017, 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#*******************************************************************************************************************************

#*******************************************************************************************************************************
#   settingsgnerator.pl
#
#   @brief  This perl script will read in a CamX settings XML file and produce header, source, and reference override text files
#           that can be used in the CamX UMD to control user settings.
#*******************************************************************************************************************************

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#   @note   If you change this file, the packed .exe version must be re-built and checked in. That requires using a licensed
#           copy of the commercial ActivePython "perlapp" from ActiveState:
#           perlapp build\infrastructure\android\settingsgenerator\settingsgenerator.pl
#
#   @note   This perl script requires XML::Simple. To install, run the following command:
#           ppm install XML::Simple
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

use strict;
use warnings;
use XML::Simple;
use File::Basename;
use File::Path;

# Acronyms for proper casing according to CamX rules
my @Acronyms =
(
    "MSM",
    "SDM"
);

#*******************************************************************************************************************************
# Main
#
# @brief    Converts a settings XML files into header files, source files, and override settings text files
#
# Usage:
#   settingsgenerator camxsettings.xml [camxhwlsettings.xml] camxoverridesettings.txt
#
# Output:
#   Using xml settings files as input produces several generated files:
#       g_camxsettings.h:                   Auto-gen’d hardware independent settings header file
#       g_camxsettings.cpp:                 Auto-gen’d hardware independent settings source file
#       g_camxhwlsettings.h:                Auto-gen’d hardware dependent settings header file
#       g_camxhwlsettings.cpp:              Auto-gen’d hardware dependent settings source file
#       camxoverridesettings.txt:           Override settings text file to transfer to target. This file contains only the
#                                           public settings and can be shared with an OEM. Values specified in this file will
#                                           override defaults in the .xml file
#       camxoverridesettingsprivate.txt:    Override settings text file to transfer to target. This file contains both the
#                                           private and public settings and must not be shared with an OEM. Values specified in
#                                           this file will override defaults in the .xml file.
#*******************************************************************************************************************************
# File variables
my $publicOverrideFilePath      = "";   # Full path to the public override settings text file (only public settings)
my $privateOverrideFilePath     = "";   # Full path to the private override settings text file (all settings)
my $fileName                    = "";   # Temporary variable for file-name only (no directory)
my $fileExtension               = "";   # Temporary variable for extension only
my $fileDirectory               = "";   # Temporary variable for directory only (no file-name)
my $settingsXMLFilePath         = "";   # Full path to the input settings XML file
my $outputHeaderFileName        = "";   # Temporary variable for output header file-name only (no directory)
my $outputHeaderFileNameAllCaps = "";   # ALL-CAPS version of the output header file-name only (no directory)
my $outputCPPFileName           = "";   # Temporary variable for output .cpp file-name only (no directory)
my $outputHeaderFilePath        = "";   # Full path to the output header file
my $outputCPPFilePath           = "";   # Full path to the output .cpp file

# Global variables for settings file generation
my $settingsXMLData             = 0;    # Parsed version of the XML file in an easily accessible data structure
my $hardwareNameUpper           = "";   # The upper camel case version of the hardware name
my $hardwareNameLower           = "";   # The lower camel case version of the hardware name
my $emitTempBOOL                = 0;    # Whether to emit the tempBOOL variable to prevent unreferenced variable warning
my %enumTypes;
my $FILE;
my $outFileObj;

# Make sure we have at least 2 arguments ($#ARGV is the index of the last element)
if ($#ARGV < 1)
{
    die "Usage: settingsgenerator camxsettings.xml [camxhwlsettings.xml] camxoverridesettings.txt\n";
}

my @mtime          = (0) x ($#ARGV+1);  # Input and output file timestamps
my $isInputUpdated = 0;                 # Temporary variable to show input file update

# Make sure the input file names are of the form "path/camx...settings.xml".
foreach my $settingsXMLFileIndex (0 .. ($#ARGV))
{
    if ($ARGV[$settingsXMLFileIndex] !~ m/^.*camx\w*setting.*$/)
    {
        {
            die "Usage: settingsgenerator camxsettings.xml [camxhwlsettings.xml] camxoverridesettings.txt\n";
        }
    }
    if (-e $ARGV[$settingsXMLFileIndex])
    {
        $mtime[$settingsXMLFileIndex] = (stat ($ARGV[$settingsXMLFileIndex]))[9];
    }
}

foreach my $settingsXMLFileIndex (0 .. ($#ARGV-1))
{
    if ($mtime[$settingsXMLFileIndex] > $mtime[$#ARGV])
    {
        $isInputUpdated = 1;
    }
}

if (not $isInputUpdated)
{
    print "No camxsettings xml updated. Skip generating camxsettings codes.\n";
    exit 0;
}
# Create the directory for the override settings text file, if it doesn't already exist
$publicOverrideFilePath = $ARGV[($#ARGV)];
($fileName, $fileDirectory, $fileExtension) = fileparse($publicOverrideFilePath,'\..*');
mkpath($fileDirectory);

# Create the private override settings text file name
$privateOverrideFilePath = $fileDirectory . $fileName . 'private' . $fileExtension;

# Process each settings XML file in order
foreach my $settingsXMLFileIndex (0 .. ($#ARGV -1))
{
    # Isolate and ensure file name is lowercase
    $settingsXMLFilePath = $ARGV[$settingsXMLFileIndex];

    if (not -e $settingsXMLFilePath)
    {
        print "Couldn't find " . $settingsXMLFilePath . "\n";
        next;
    }

    ($fileName, $fileDirectory, $fileExtension) = fileparse($settingsXMLFilePath,'\..*');
    $fileName = lc($fileName);                                          # "filename"
    $fileName =~ s/g_//;                                                # "filename" without any g_

    # Build output file names
    $outputHeaderFileName   = 'g_' . $fileName . '.h';                  # "g_filename.h"
    $outputCPPFileName      = 'g_' . $fileName . '.cpp';                # "g_filename.cpp"

    # Build destination filenames with path and extension
    $outputHeaderFilePath   = $fileDirectory . $outputHeaderFileName;   # "../g_filename.h"
    $outputCPPFilePath      = $fileDirectory . $outputCPPFileName;      # "../g_filename.cpp"

    # Build all-caps version of header file name for header file preprocessor protection
    $outputHeaderFileNameAllCaps    = 'G_' . $fileName . '_H';          # "G_filename_H"
    $outputHeaderFileNameAllCaps    = uc($outputHeaderFileNameAllCaps); # "G_FILENAME_H"

    # Load and parse the XML settings file
    $settingsXMLData = XMLin("$settingsXMLFilePath", forcearray => [ 'setting', 'enum', 'settingsSubGroup' ]);
    VerifySettings();
    IndexEnumTypes();

    # Get the settings manager class name prefix for hardware dependent settings only
    if ($settingsXMLFileIndex > 0)
    {
        $hardwareNameUpper = $settingsXMLData->{SettingsManagerClassNamePrefix};    # For example, "MSM8996" or "Titan17x"

        # Check to see if the beginning of the word is an expected acronym so that we can adjust the casing appropriately
        my $foundAcronymPrefix = 0;
        foreach my $acronym (@Acronyms)
        {
            if ($acronym eq substr($hardwareNameUpper, 0, length($acronym)))
            {
                $foundAcronymPrefix = 1;
                last;
            }
        }

        if ($foundAcronymPrefix)
        {
            $hardwareNameLower = $hardwareNameUpper;                                # For example, "MSM8996"
        }
        else
        {
            $hardwareNameLower = lcfirst($hardwareNameUpper);                       # For example, "titan17x"
        }
    }

    # The first file (index 0) must always be the hardware independent settings XML file and the others must be hardware
    # dependent settings XML files.
    CreateHeaderFile($settingsXMLFileIndex == 0);
    CreateCPPFile($settingsXMLFileIndex == 0);
    CreatePublicOverrideTextFile($settingsXMLFileIndex == 0);
    CreatePrivateOverrideTextFile($settingsXMLFileIndex == 0);
}

#*******************************************************************************************************************************
# VerifySettings
#
# @brief    Verifies settings integrity, including:
#            - Uniqueness of hashes
#
# @return   None
#*******************************************************************************************************************************
sub VerifySettings
{
    # TODO (CAMX-678): Verify enum/setting definitions are well-formed and have required fields
    # TODO (CAMX-678):  Verify Enum Hashes are unique

    my %settingStringHashes;

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            # Check to see if this is a BOOL type setting
            if ($setting->{VariableType} eq "BOOL")
            {
                $emitTempBOOL = 1;
            }

            # Check to see if we have seen the settings string hash before.
            my $settingStringHash = GetSettingsStringHashValue($setting->{VariableName});
            if (exists $settingStringHashes{$settingStringHash})
            {
                die "Error: '" . $setting->{VariableName} .
                    "' hashes to the same value as '" . $settingStringHashes{$settingStringHash} .
                    "'\n";
            }
            else
            {
                $settingStringHashes{$settingStringHash} = $setting->{VariableName};
            }
        }
    }
}

#*******************************************************************************************************************************
# IndexEnumTypes
#
# @brief    Creates a hash of enum type names to enum entries. Used to determine if a type is a defined enum type
#
# @return   None
#*******************************************************************************************************************************
sub IndexEnumTypes
{
    foreach my $enum (@{$settingsXMLData->{enum}})
    {
        $enumTypes{$enum->{Name}} = $enum;
    }
}

#*******************************************************************************************************************************
# EmitCopyright
#
# @brief    Emit a copyright notice to the file
#
# @param    File handle
# @param    Prefix for each line of the copyright notice
# @param    Line-ending character for each line of the copyright notice
#
# @return   None
#*******************************************************************************************************************************
sub EmitCopyright
{
    my $f           = shift;
    my $prefix      = shift;
    my $lineEnding  = shift;

    print $f $prefix . ("/" x (128 - length($prefix))) . $lineEnding;
    print $f $prefix . " Copyright (c) 2016-2017 Qualcomm Technologies, Inc." . $lineEnding;
    print $f $prefix . " All Rights Reserved." . $lineEnding;
    print $f $prefix . " Confidential and Proprietary - Qualcomm Technologies, Inc." . $lineEnding;
    print $f $prefix . ("/" x (128 - length($prefix))) . $lineEnding;
    print $f $lineEnding;
}

#*******************************************************************************************************************************
# EmitFileDescriptionBlock
#
# @brief    Emit a file description block to the file
#
# @param    File handle
# @param    Output file name, including extension
# @param    Description text to place after the settings group name
#
# @return   None
#*******************************************************************************************************************************
sub EmitFileDescriptionBlock
{
    my $f               = shift;
    my $outputFileName  = shift;
    my $descriptionText = shift;

    print $f ("/" x 128) . "\n";
    print $f "/// \@file  " . $outputFileName . "\n";
    print $f "/// \@brief " . $settingsXMLData->{Name} . " " . $descriptionText . ".\n";
    print $f "///\n";
    print $f "///             !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    print $f ("!" x length($fileName)) . ("!" x length($fileExtension) . "!!!!!!!!!\n");
    print $f "///             !!!!!!! AUTO-GENERATED FILE! DO NOT EDIT! Make all changes in ";
    print $f $fileName . $fileExtension . ". !!!!!!!\n";
    print $f "///             !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    print $f ("!" x length($fileName)) . ("!" x length($fileExtension) . "!!!!!!!!!\n");
    print $f "///\n";
    print $f ("/" x 128) . "\n";
    print $f "\n";
}

#*******************************************************************************************************************************
# EmitEnumerator
#
# @brief    Emit an enumerator line
#
# @param    File handle
# @param    Enum object
# @param    Enumerator object
#
# @return   None
#*******************************************************************************************************************************
sub EmitEnumerator
{
    my $f           = shift;
    my $enum        = shift;
    my $enumerator  = shift;

    my $enumeratorLine = $enum->{Prefix} . $enumerator->{Name};
    if (defined $enumerator->{Number})
    {
        $enumeratorLine .= " = $enumerator->{Number}";
    }
    $enumeratorLine .= ",";
    printf $f "    %-39s ", $enumeratorLine;
    if (defined $enumerator->{Description})
    {
        print $f "///< $enumerator->{Description}";
    }
    print $f "\n";
}

#*******************************************************************************************************************************
# EmitEnumeratorToHashMap
#
# @brief    Emit an array of enumerator to hash maps
#
# @param    File handle
# @param    Enumerator object
#
# @return   None
#*******************************************************************************************************************************
sub EmitEnumeratorToHashMapArray
{
    my $f           = shift;
    my $enum        = shift;
    my $enumerator  = shift;

    my $enumeratorName = $enum->{Prefix} . $enumerator->{Name};
    printf $f "    {%s, %-32s},", GetSettingsStringHashValue($enumeratorName), $enumeratorName;
    if (defined $enumerator->{Description})
    {
        printf $f " ///< $enumerator->{Description}";
    }
    printf $f "\n";
}

#*******************************************************************************************************************************
# EmitSettingInitializer
#
# @brief    Emit a setting initializer line
#
# @param    File handle
# @param    The setting to initialize
# @param    The setting struct name
#
# @return   None
#*******************************************************************************************************************************
sub EmitSettingInitializer
{
    my $f                   = shift;
    my $setting             = shift;
    my $settingsStructName  = shift;

    # Emit setting initializer statements based on type
    print $f "    ";
    if ($setting->{VariableType} eq 'String')
    {
        # If this is an empty string, we want to output an empty string, not a hashref
        if (ref($setting->{DefaultValue}) eq "HASH")
        {
            if (keys(%{$setting->{DefaultValue}}) > 0)
            {
                die "DefaultValue for setting '$setting->{VariableName}' contains unexpected hash values";
            }
            $setting->{DefaultValue} = "";
        }

        print $f "OsUtils::StrLCpy(" . $settingsStructName . $setting->{VariableName} . ", \"";
        print $f $setting->{DefaultValue};
        print $f "\", sizeof(" . $settingsStructName . $setting->{VariableName} ."))";
    }
    elsif (defined $enumTypes{$setting->{VariableType}})
    {
        my $enum = $enumTypes{$setting->{VariableType}};
        printf $f "%s%-30s = ", $settingsStructName, $setting->{VariableName};
        print $f $enum->{Prefix} . $setting->{DefaultValue};
    }
    else
    {
        printf $f "%s%-30s = ", $settingsStructName, $setting->{VariableName};
        print $f $setting->{DefaultValue};
        if ($setting->{VariableType} eq "FLOAT")
        {
            print $f "F";       # Explicitly making const FLOAT avoids MSVC warning
        }
    }
    print $f ";\n";
}

#*******************************************************************************************************************************
# EmitCommentedEnumeratorDescription
#
# @brief    Emit a commented enumerator description including hashed value, name, number, and description
#
# @param    File handle
# @param    The enum this enumerator is a part of
# @param    The enumerator to describe
#
# @return   None
#*******************************************************************************************************************************
sub EmitCommentedEnumeratorDescription
{
    my $f           = shift;
    my $enum        = shift;
    my $enumerator  = shift;

    my $enumeratorName = $enum->{Prefix} . $enumerator->{Name};
    print $f ";   " . GetSettingsStringHashValue($enumeratorName);
    if (defined $enumerator->{Number})
    {
        $enumeratorName .= " = $enumerator->{Number}";
    }
    printf $f "  %-35s ", $enumeratorName;
    if (defined $enumerator->{Description})
    {
        print $f "///< $enumerator->{Description}";
    }
    print $f "\n";
}

#*******************************************************************************************************************************
# EmitCommentedSetting
#
# @brief    Emit a commented setting block
#
# @param    File handle
# @param    The setting to emit
#
# @return   None
#*******************************************************************************************************************************
sub EmitCommentedSetting
{
    my $f       = shift;
    my $setting = shift;

    my $helpFormatted = $setting->{Help};
    my $defaultValue  = $setting->{DefaultValue};
    my $defaultHashed = $setting->{DefaultValue};

    # Ensure multi-line help text has a ';' at the start of each line and strip any leading whitespace. Global replace of LF
    # followed by zero or more spaces with a LF, a ';', and a space.
    $helpFormatted =~ s/^\n\s*//;
    $helpFormatted =~ s/\n\s*$//;
    $helpFormatted =~ s/\n\s*/\n; /g;

    print $f (";" x 128) . "\n";
    print $f "; $setting->{Name}\n";
    print $f ";\n";
    print $f "; $helpFormatted\n";
    print $f ";\n";
    if ($setting->{VariableType} eq "BOOL")
    {
        print $f "; Type: BOOL\n";
    }
    elsif ($setting->{VariableType} eq "FLOAT")
    {
        print $f "; Type: FLOAT\n";
    }
    elsif ($setting->{VariableType} eq "INT")
    {
        print $f "; Type: INT\n";
    }
    elsif ($setting->{VariableType} eq "UINT")
    {
        print $f "; Type: UINT\n";
    }
    elsif ($setting->{VariableType} eq "String")
    {
        print $f "; Type: String\n";
    }
    elsif (defined $enumTypes{$setting->{VariableType}})
    {
        my $enum = $enumTypes{$setting->{VariableType}};
        print $f "; Type: Enum\n";
        if (ref($enum->{enumerator}) eq "ARRAY")
        {
            foreach my $enumerator (@{$enum->{enumerator}})
            {
                EmitCommentedEnumeratorDescription(*$f, $enum, $enumerator);
            }
        }
        else
        {
            my $enumerator = $enum->{enumerator};
            EmitCommentedEnumeratorDescription(*$f, $enum, $enumerator);
        }
        # Print the default value as a hash
        $defaultValue = $enum->{Prefix} . $setting->{DefaultValue};
        $defaultHashed = GetSettingsStringHashValue($defaultValue);
    }
    else
    {
        die "Error: Invalid VariableType '$setting->{VariableType}' in $settingsXMLFilePath";
    }
    print $f ";\n";
    print $f "; " . GetSettingsStringHashValue($setting->{VariableName}) . "=" . $defaultHashed . "\n";
    print $f "; " . $setting->{VariableName} . "=" . $defaultValue . "\n";
    print $f (";" x 128) . "\n";
    print $f "\n";
}
#*******************************************************************************************************************************
# CleanLine
#
# @brief    Removes newlines, returns, and collapses spaces in an input string. Used to prevent errors in strings specified
#           across multiple lines in an XML config file.
#
# @param    The text to clean
#
# @return   String without line-breaks or a series of spaces
#*******************************************************************************************************************************
sub CleanLine
{
    my $text = shift;

    $text =~ s/\n//g;   # Remove newlines
    $text =~ s/\r//g;   # Remove carriage returns
    $text =~ s/\s+/ /g; # Collapse a series of spaces to a single space.
    $text =~ s/"/'/g;   # Removes the double quotes from the strings so they can be loaded in the C++ file.
    return $text;
}

#*******************************************************************************************************************************
# GetSettingsStringHashValue
#
# @brief    Creates a hash for the settings string passed in for the purpose of obfuscation. Case independent.
#
# @note     Must match OverrideSettingsFile::GetSettingsStringHashValue() in driver code
#
# @param    The unencrypted setting string
#
# @return   The setting string hash value as an eight digit hex string with a 0x prefix
#*******************************************************************************************************************************
sub GetSettingsStringHashValue
{
    # Using integer math here. Assuming at least 32 bits.
    use integer;

    my $unencryptedName = shift;
    my $highOrder       = 0;
    my $hash            = 0;
    my $hashHexString   = "0x00000000";

    # For every character in the string...
    for ($unencryptedName=~/./g)
    {
        # ...add to the hash. Character is in $_
        $highOrder = $hash & 0xF8000000;                    # Swizzle previous bits
        $hash      = ($hash << 5) & 0xffffffff;
        $hash      = $hash ^ (($highOrder >> 27) & 0x1f);   # Mask off the MSBs in case we sign extended (32bit perl)
        $hash      = $hash ^ ord(lc($_));                   # XOR in current character (lower case)
    }

    # Format result as an eight digit hex string with a 0x prefix
    $hashHexString = sprintf "0x%08X", $hash;
    return $hashHexString;
}

#*******************************************************************************************************************************
# GetSettingStringHashName
#
# @brief    Determines the macro used to alias the hash for the setting name
#
# @param    The setting name
#
# @return   None
#*******************************************************************************************************************************
sub GetSettingStringHashName
{
    my $settingName = shift;

    return "StringHash" . ucfirst($settingName);
}

#*******************************************************************************************************************************
# IsHex
#
# @brief    Determines if a string is a hexadecimal value.
#
# @param    String to analyze for hex formatting
#
# @return   1 if the string is a hex value, 0 otherwise.
#*******************************************************************************************************************************
sub IsHex
{
    my $inputVal = shift;

    $inputVal =~ s/^\s+//;  # Remove whitespace at the beginning of the line

    return ($inputVal and
            substr($inputVal, 0, 1) eq '0' and
            (substr($inputVal, 1, 1) eq 'x' or substr($inputVal, 1, 1) eq 'X'));
}

#*******************************************************************************************************************************
# CreateHeaderFile
#
# @brief    Generates the settings header file from the XML data.
#
# @param    A boolean indicating whether the settings are hardware independent or hardware dependent
#
# @return   None
#*******************************************************************************************************************************
sub CreateHeaderFile
{
    my $isHardwareIndependent = shift;

    $outFileObj = DiffFileHandle->new($outputHeaderFilePath);
    $FILE = $outFileObj->GetHandle();

    EmitCopyright(*$FILE, "//", "\n");
    EmitFileDescriptionBlock(*$FILE, $outputHeaderFileName, "definitions");

    # Emit whiner suppressions, header pre-processor protection, #includes, and namespace
    print $FILE "#ifndef " . $outputHeaderFileNameAllCaps . "\n";
    print $FILE "#define " . $outputHeaderFileNameAllCaps . "\n";
    print $FILE "\n";
    print $FILE "#include \"camxdefs.h\"\n";
    print $FILE "#include \"camxtypes.h\"\n";
    if (!$isHardwareIndependent)
    {
        print $FILE "#include \"g_camxsettings.h\"\n";
    }
    print $FILE "\n";

    print $FILE "CAMX_NAMESPACE_BEGIN\n";
    print $FILE "\n";

    # Emit separate for constant definitions
    print $FILE ("/" x 128) . "\n";
    print $FILE "// Constant Definitions\n";
    print $FILE ("/" x 128) . "\n";

    if ($isHardwareIndependent)
    {
        # Emit anything needed at the top of the hardware independent file
        print $FILE "static const UINT32 MaxStringLength                     = 512;";
        print $FILE "          ///< Maximum value string length\n";
        print $FILE "\n";
    }

    # Emit setting string hash constant definitions
    print $FILE "// Setting string hash constant definitions\n";
    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            my $LHS = "static const UINT32 " . GetSettingStringHashName($setting->{VariableName}) . " ";
            printf $FILE "%-55s ", $LHS;
            print $FILE "= " . GetSettingsStringHashValue($setting->{VariableName}) . ";";
            print $FILE "   ///< " . $setting->{VariableName} . "\n";
            if ($setting->{SetpropKey})
            {
                $LHS = "static const UINT32 Prop" . GetSettingStringHashName($setting->{VariableName}) . " ";
                printf $FILE "%-55s ", $LHS;
                print $FILE "= " . GetSettingsStringHashValue($setting->{SetpropKey}) . ";";
                print $FILE "   ///< " . $setting->{VariableName} . "\n";
            }
        }
    }
    print $FILE "\n";

    # Emit separate for type definitions
    print $FILE ("/" x 128) . "\n";
    print $FILE "// Type Definitions\n";
    print $FILE ("/" x 128) . "\n";

    # Emit enumeration structures
    foreach my $enum (@{$settingsXMLData->{enum}})
    {
        next if $enum->{NoDeclare};

        print $FILE "/// $enum->{Description}\n";
        print $FILE "enum $enum->{Name}\n";
        print $FILE "{\n";
        if (ref($enum->{enumerator}) eq "ARRAY")
        {
            foreach my $enumerator (@{$enum->{enumerator}})
            {
                EmitEnumerator(*$FILE, $enum, $enumerator);
            }
        }
        else
        {
            my $enumerator = $enum->{enumerator};
            EmitEnumerator(*$FILE, $enum, $enumerator);
        }
        print $FILE "};\n\n";
    }

    # Emit EnumeratorToHashMap definition
    if ($isHardwareIndependent)
    {
        print $FILE "/// \@brief Maps an enumerator to its hash value\n";
        print $FILE "struct EnumeratorToHashMap\n";
        print $FILE "{\n";
        print $FILE "    UINT32  enumeratorStringHash;   ///< The hash value used for obfuscation\n";
        print $FILE "    INT     enumerator;             ///< The numerical value of the enumerator\n";
        print $FILE "};\n";
        print $FILE "\n";
    }

    print $FILE "/// \@brief Encapsulates all ${hardwareNameUpper} static settings values\n";
    print $FILE "struct ${hardwareNameUpper}StaticSettings";
    if ($isHardwareIndependent)
    {
        print $FILE "\n";
    }
    else
    {
        print $FILE " : public StaticSettings\n";
    }
    print $FILE "{\n";

    # Generate a list of settings in the structure. We want to group BOOLs together so we can use bitfields. Therefore, generate
    # a list as an array so that it can be sorted alphabetically.
    my @settingsList;
    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            my $settingLine;

            # BOOLs are stored as a bitfield
            my $variableType    = $setting->{VariableType};
            my $suffix          = "";

            if ($variableType eq "BOOL")
            {
                $variableType   = "BIT";
                $suffix         = ":1";
            }
            elsif ($variableType eq "String")
            {
                $variableType   = "CHAR";
                $suffix         = "[MaxStringLength]";
            }

            # Ensure multi-line help text is indented properly and has a '///<' before each comment. Global replace of LF
            # followed by zero or more spaces with a LF, spaces, and '///<'.
            my $helpFormatted = $setting->{Help};
            $helpFormatted =~ s/^\n\s*//;
            $helpFormatted =~ s/\n\s*$//;
            $helpFormatted =~ s/\n\s*/\n                                                                \/\/\/< /g;

            $settingLine = sprintf "    %-23s %s%s;", $variableType, $setting->{VariableName}, $suffix;
            $settingLine = sprintf "%-63s ///< %s", $settingLine, $helpFormatted;
            $settingLine .= "\n";

            # Add to settings list for later alphabetical sorting
            push @settingsList, $settingLine;
        }
    }

    # Sort the settings lines. Since the type is at the start of the string it gets sorted by type first, then by variable name.
    @settingsList = sort @settingsList;

    print $FILE @settingsList;

    print $FILE "};\n";
    print $FILE "\n";
    print $FILE "CAMX_NAMESPACE_END\n";
    print $FILE "\n";
    print $FILE "#endif // " . $outputHeaderFileNameAllCaps . "\n";
    $outFileObj->Close();
}

#*******************************************************************************************************************************
# CreateCPPFile
#
# @brief    Generates the settings CPP source file from the XML data.
#
# @param    A boolean indicating whether the settings are hardware independent or hardware dependent
#
# @return   None
#*******************************************************************************************************************************
sub CreateCPPFile
{
    my $isHardwareIndependent = shift;

    $outFileObj = DiffFileHandle->new($outputCPPFilePath);
    $FILE = $outFileObj->GetHandle();

    EmitCopyright(*$FILE, "//", "\n");
    EmitFileDescriptionBlock(*$FILE, $outputCPPFileName, "implementation");

    # Emit whiner suppressions, header pre-processor protection, #includes, and namespace
    #print $FILE "// NOWHINE FILE GR002: Comments may exceed max line length\n";
    print $FILE "#include \"camxincs.h\"\n";
    print $FILE "#include \"camx" . lc($hardwareNameLower) . "settingsmanager.h\"\n";
    print $FILE "\n";
    print $FILE "#include \"" . $outputHeaderFileName . "\"\n";
    print $FILE "\n";

    print $FILE "CAMX_NAMESPACE_BEGIN\n";
    print $FILE "\n";

    # Emit separate for type definitions
    print $FILE ("/" x 128) . "\n";
    print $FILE "// Type Definitions\n";
    print $FILE ("/" x 128) . "\n";

    # Emit arrays containing the mapping between the enumerator and its hashed value
    foreach my $enum (@{$settingsXMLData->{enum}})
    {
        next if $enum->{NoDeclare};

        print $FILE "EnumeratorToHashMap $enum->{Name}EnumeratorToHashMap\[\] =\n";
        print $FILE "{\n";
        if (ref($enum->{enumerator}) eq "ARRAY")
        {
            foreach my $enumerator (@{$enum->{enumerator}})
            {
                EmitEnumeratorToHashMapArray(*$FILE, $enum, $enumerator)
            }
        }
        else
        {
            my $enumerator = $enum->{enumerator};
            EmitEnumeratorToHashMapArray(*$FILE, $enum, $enumerator)
        }
        print $FILE "};\n\n";
    }

    my $settingsTypePrefix  = "";
    my $settingsStructName  = "";
    my $hardwarePrefix      = "";
    my $tempBOOL            = "tempBOOL";
    my $argumentIndent      = 8;

    if ($isHardwareIndependent)
    {
        $settingsTypePrefix = "in";
        $settingsStructName = "m_pStaticSettings->";
        $hardwarePrefix     = "";
    }
    else
    {
        $settingsTypePrefix  = "";
        $settingsStructName  = "m_${hardwareNameLower}StaticSettings\.";
        $hardwarePrefix     = "Hw";
    }

    print $FILE ("/" x 128) . "\n";
    print $FILE "// ${hardwareNameUpper}SettingsManager Method Definitions\n";
    print $FILE ("/" x 128) . "\n";
    print $FILE "\n";

    # Begin InitializeDefaultSettings
    print $FILE ("/" x 128) . "\n";
    print $FILE "/// ${hardwareNameUpper}SettingsManager::${hardwarePrefix}InitializeDefaultSettings\n";
    print $FILE "///\n";
    print $FILE "/// \@note  This method is part of the SettingsManager implementation. See camxsettingsmanager.h.\n";
    print $FILE ("/" x 128) . "\n";

    print $FILE "VOID ${hardwareNameUpper}SettingsManager::${hardwarePrefix}InitializeDefaultSettings()\n";
    print $FILE "{\n";

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        if (defined($settingsSubGroup->{Name}) && $settingsSubGroup->{Name} !~ m/Debug Settings/)
        {
            foreach my $setting (@{$settingsSubGroup->{setting}})
            {
                EmitSettingInitializer(*$FILE, $setting, $settingsStructName);
            }
        }
    }

    if ($isHardwareIndependent)
    {
        print $FILE "\n";
        print $FILE "    // Now initialize the hardware dependent default settings values\n";
        print $FILE "    HwInitializeDefaultSettings();\n";
    }

    print $FILE "}\n";
    print $FILE "\n";
    # End InitializeDefaultSettings

    # Begin InitializeDefaultDebugSettings
    print $FILE ("/" x 128) . "\n";
    print $FILE "/// ${hardwareNameUpper}SettingsManager::${hardwarePrefix}InitializeDefaultDebugSettings\n";
    print $FILE "///\n";
    print $FILE "/// \@note  This method is part of the SettingsManager implementation. See camxsettingsmanager.h.\n";
    print $FILE ("/" x 128) . "\n";

    print $FILE "VOID ${hardwareNameUpper}SettingsManager::${hardwarePrefix}InitializeDefaultDebugSettings()\n";
    print $FILE "{\n";

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        if (defined($settingsSubGroup->{Name}) && $settingsSubGroup->{Name} =~ m/Debug Settings/)
        {
            foreach my $setting (@{$settingsSubGroup->{setting}})
            {
                EmitSettingInitializer(*$FILE, $setting, $settingsStructName);
            }
        }
    }

    if ($isHardwareIndependent)
    {
        print $FILE "\n";
        print $FILE "    // Now initialize the hardware dependent default debug settings values\n";
        print $FILE "    HwInitializeDefaultDebugSettings();\n";
    }

    print $FILE "}\n";
    print $FILE "\n";
    # End InitializeDefaultDebugSettings

    # Begin LoadOverrideSettings
    print $FILE ("/" x 128) . "\n";
    print $FILE "/// ${hardwareNameUpper}SettingsManager::${hardwarePrefix}LoadOverrideSettings\n";
    print $FILE "///\n";
    print $FILE "/// \@note  This method is part of the SettingsManager implementation. See camxsettingsmanager.h.\n";
    print $FILE ("/" x 128) . "\n";

    print $FILE "CamxResult ${hardwareNameUpper}SettingsManager::${hardwarePrefix}LoadOverrideSettings(\n";
    print $FILE "    IOverrideSettingsStore* pOverrideSettingsStore)\n";
    print $FILE "{\n";
    print $FILE "    CamxResult  result      = CamxResultSuccess;\n";
    if ($emitTempBOOL)
    {
        print $FILE "    BOOL        ${tempBOOL}    = FALSE;\n";
    }
    print $FILE "\n";

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            my $castTo;
            my $readFunctionCall    = "";

            # Treat BOOLs differently since we are storing BOOLs in a bitfield. We use a temporary variable to hold the initial
            # value from the settings structure.
            if ($setting->{VariableType} eq "BOOL")
            {
                print $FILE "    " . $tempBOOL . " = (";
                print $FILE $settingsStructName . $setting->{VariableName} . " == 1) ? TRUE : FALSE;\n";
            }

            $readFunctionCall = "    pOverrideSettingsStore->ReadSetting";

            if ($setting->{VariableType} eq "BOOL")
            {
                $readFunctionCall .= "BOOL(";
            }
            elsif ($setting->{VariableType} eq "FLOAT")
            {
                $readFunctionCall .= "FLOAT(";
            }
            elsif ($setting->{VariableType} eq "INT")
            {
                $readFunctionCall .= "INT(";
            }
            elsif ($setting->{VariableType} eq "UINT")
            {
                $readFunctionCall .= "UINT(";
            }
            elsif ($setting->{VariableType} eq "String")
            {
                $readFunctionCall .= "String(";
            }
            elsif (defined $enumTypes{$setting->{VariableType}})
            {
                $castTo = "INT";
                $readFunctionCall .= "Enum(";
            }
            else
            {
                die "Error: Invalid VariableType '$setting->{VariableType}' in $settingsXMLFilePath";
            }

            print $FILE $readFunctionCall . "\n";

            # Emit the setting string hash constant
            my $settingStringHashName = GetSettingStringHashName($setting->{VariableName});
            print $FILE (" " x $argumentIndent);
            print $FILE $settingStringHashName . ",\n";

            # Emit the address of the setting to be updated based on type
            my $settingVariable = $settingsStructName . $setting->{VariableName};
            print $FILE (" " x $argumentIndent);
            if ($setting->{VariableType} eq "BOOL")
            {
                print $FILE "&";
                $settingVariable = $tempBOOL;
            }
            elsif ($setting->{VariableType} eq 'String')
            {
                # Do nothing
            }
            elsif (defined $castTo) # User-define enum type
            {
                print $FILE "reinterpret_cast<$castTo*>(&";
            }
            else
            {
                print $FILE "&";
            }
            print $FILE $settingVariable;

            # Emit additional arguments, based on type
            if ($setting->{VariableType} eq 'String')
            {
                # If we are processing the string, also pass the size of the string
                print $FILE ",\n";
                print $FILE (" " x $argumentIndent);
                print $FILE "sizeof(" . $settingsStructName . $setting->{VariableName} . ")";
            }
            elsif (defined $castTo)
            {
                # If we are processing a user-defined enum type (i.e. castTo is defined), we also need to pass the corresponding
                # enumerator to hash map array and the number of elements in it.
                print $FILE "),\n";
                print $FILE (" " x $argumentIndent);
                print $FILE "$enumTypes{$setting->{VariableType}}->{Name}EnumeratorToHashMap,\n";
                print $FILE (" " x $argumentIndent);
                print $FILE
                       "static_cast<UINT>(CAMX_ARRAY_SIZE($enumTypes{$setting->{VariableType}}->{Name}EnumeratorToHashMap))";
            }
            print $FILE ");\n";

            # Treat BOOLs differently since we are storing BOOLs in a bitfield. We used a temporary variable to hold the
            # override value from the file, so copy back to the struct.
            if ($setting->{VariableType} eq "BOOL")
            {
                print $FILE "    " . $settingsStructName . $setting->{VariableName} . " = (";
                print $FILE $tempBOOL . " == TRUE) ? 1 : 0;\n";
            }

            print $FILE "\n";
        }
    }

    if ($isHardwareIndependent)
    {
        print $FILE "    if (CamxResultSuccess == result)\n";
        print $FILE "    {\n";
        print $FILE "        // Now load the hardware dependent override settings\n";
        print $FILE "        result = HwLoadOverrideSettings(pOverrideSettingsStore);\n";
        print $FILE "    }\n";
        print $FILE "\n";
    }

    print $FILE "    return result;\n";
    print $FILE "}\n";
    print $FILE "\n";
    # End LoadOverrideSettings

    # Begin LoadOverrideProperties
    print $FILE ("/" x 128) . "\n";
    print $FILE "/// ${hardwareNameUpper}SettingsManager::${hardwarePrefix}LoadOverrideProperties\n";
    print $FILE "///\n";
    print $FILE "/// \@note  This method is part of the SettingsManager implementation. See camxsettingsmanager.h.\n";
    print $FILE ("/" x 128) . "\n";

    print $FILE "CamxResult ${hardwareNameUpper}SettingsManager::${hardwarePrefix}LoadOverrideProperties(\n";
    print $FILE "    IOverrideSettingsStore* pOverrideSettingsStore,\n";
    print $FILE "    BOOL                    updateStatic)\n";
    print $FILE "{\n";
    print $FILE "    CamxResult  result      = CamxResultSuccess;\n";
    if ($emitTempBOOL)
    {
        print $FILE "    BOOL        ${tempBOOL}    = FALSE;\n";
    }
    print $FILE "\n";

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            if ($setting->{SetpropKey})
            {
                my $castTo;
                my $readFunctionCall    = "";
                my $extraSpaces = "";
                if ($setting->{Dynamic} eq "FALSE")
                {
                    print $FILE "    if (TRUE == updateStatic)\n    {\n";
                    $extraSpaces = "    ";
                }
                # Treat BOOLs differently since we are storing BOOLs in a bitfield. We use a temporary variable to hold the
                # initial value from the settings structure.
                if ($setting->{VariableType} eq "BOOL")
                {
                    print $FILE "    " . $extraSpaces .  $tempBOOL . " = (";
                    print $FILE $settingsStructName . $setting->{VariableName} . " == 1) ? TRUE : FALSE;\n";
                }

                $readFunctionCall = "    " . $extraSpaces . "pOverrideSettingsStore->ReadSetting";

                if ($setting->{VariableType} eq "BOOL")
                {
                    $readFunctionCall .= "BOOL(";
                }
                elsif ($setting->{VariableType} eq "FLOAT")
                {
                    $readFunctionCall .= "FLOAT(";
                }
                elsif ($setting->{VariableType} eq "INT")
                {
                    $readFunctionCall .= "INT(";
                }
                elsif ($setting->{VariableType} eq "UINT")
                {
                    $readFunctionCall .= "UINT(";
                }
                elsif ($setting->{VariableType} eq "String")
                {
                    $readFunctionCall .= "String(";
                }
                elsif (defined $enumTypes{$setting->{VariableType}})
                {
                    $castTo = "INT";
                    $readFunctionCall .= "Enum(";
                }
                else
                {
                    die "Error: Invalid VariableType '$setting->{VariableType}' in $settingsXMLFilePath";
                }

                print $FILE $readFunctionCall . "\n";

                # Emit the setting string hash constant
                my $settingStringHashName = "Prop" . GetSettingStringHashName($setting->{VariableName});
                print $FILE ($extraSpaces . " " x $argumentIndent);
                print $FILE $settingStringHashName . ",\n";

                # Emit the address of the setting to be updated based on type
                my $settingVariable = $settingsStructName . $setting->{VariableName};
                print $FILE ($extraSpaces . " " x $argumentIndent);
                if ($setting->{VariableType} eq "BOOL")
                {
                    print $FILE "&";
                    $settingVariable = $tempBOOL;
                }
                elsif ($setting->{VariableType} eq 'String')
                {
                    # Do nothing
                }
                elsif (defined $castTo) # User-define enum type
                {
                    print $FILE "reinterpret_cast<$castTo*>(&";
                }
                else
                {
                    print $FILE "&";
                }
                print $FILE $settingVariable;

                # Emit additional arguments, based on type
                if ($setting->{VariableType} eq 'String')
                {
                    # If we are processing the string, also pass the size of the string
                    print $FILE ",\n";
                    print $FILE ($extraSpaces . " " x $argumentIndent);
                    print $FILE "sizeof(" . $settingsStructName . $setting->{VariableName} . ")";
                }
                elsif (defined $castTo)
                {
                    # If we are processing a user-defined enum type (i.e. castTo is defined), we also need to
                    # pass the corresponding enumerator to hash map array and the number of elements in it.
                    print $FILE "),\n";
                    print $FILE ($extraSpaces .  " " x $argumentIndent);
                    print $FILE "$enumTypes{$setting->{VariableType}}->{Name}EnumeratorToHashMap,\n";
                    print $FILE ($extraSpaces . " " x $argumentIndent);
                    print $FILE
                          "static_cast<UINT>(CAMX_ARRAY_SIZE($enumTypes{$setting->{VariableType}}->{Name}EnumeratorToHashMap))";
                }
                print $FILE ");\n";

                # Treat BOOLs differently since we are storing BOOLs in a bitfield. We used a temporary variable to hold the
                # override value from the file, so copy back to the struct.
                if ($setting->{VariableType} eq "BOOL")
                {
                    print $FILE "    " . $extraSpaces . $settingsStructName . $setting->{VariableName} . " = (";
                    print $FILE $tempBOOL . " == TRUE) ? 1 : 0;\n";
                }

                print $FILE "\n";

                if ($setting->{Dynamic} eq "FALSE")
                {
                    print $FILE "    }\n\n";
                }
            }
        }
    }

    if ($isHardwareIndependent)
    {
        print $FILE "    if (CamxResultSuccess == result)\n";
        print $FILE "    {\n";
        print $FILE "        // Now load the hardware dependent override properties\n";
        print $FILE "        result = HwLoadOverrideProperties(pOverrideSettingsStore, updateStatic);\n";
        print $FILE "    }\n";
        print $FILE "\n";
    }

    print $FILE "    return result;\n";
    print $FILE "}\n";
    print $FILE "\n";
    # End LoadOverrideProperties

    # Begin DumpSettings
    print $FILE "#if SETTINGS_DUMP_ENABLE\n";
    print $FILE ("/" x 128) . "\n";
    print $FILE "/// ${hardwareNameUpper}SettingsManager::${hardwarePrefix}DumpSettings\n";
    print $FILE "///\n";
    print $FILE "/// \@note  This method is part of the SettingsManager implementation. See camxsettingsmanager.h.\n";
    print $FILE ("/" x 128) . "\n";
    print $FILE "VOID ${hardwareNameUpper}SettingsManager::${hardwarePrefix}DumpSettings() const\n";
    print $FILE "{\n";
    print $FILE "    CAMX_LOG_VERBOSE(CamxLogGroupCore, \"=============== BEGIN DUMP OF CURRENT";
    if (!$isHardwareIndependent)
    {
        print $FILE " " . uc($hardwareNameUpper);
    }
    print $FILE " HARDWARE-" . uc($settingsTypePrefix) . "DEPENDENT SETTINGS VALUES ===============\");\n";
    print $FILE "    CAMX_LOG_VERBOSE(CamxLogGroupCore, \"<Setting> (<Hash>) = <Value>\");\n";

    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            print $FILE "    CAMX_LOG_VERBOSE(CamxLogGroupCore, \"" . $setting->{VariableName} . " (";
            print $FILE GetSettingsStringHashValue($setting->{VariableName}) . ") = %";

            my $format = "";
            if (($setting->{VariableType} eq "BOOL") ||
                ($setting->{VariableType} eq "INT")  ||
                ($setting->{VariableType} eq "UINT") ||
                (defined $enumTypes{$setting->{VariableType}}))
            {
                $format = "d";
            }
            elsif ($setting->{VariableType} eq "FLOAT")
            {
                $format = "f";
            }
            elsif ($setting->{VariableType} eq "String")
            {
                $format = "s";
            }
            else
            {
                die "Error: Invalid VariableType '$setting->{VariableType}' in $settingsXMLFilePath";
            }
            print $FILE $format . "\", " . $settingsStructName . $setting->{VariableName} . ");\n";
        }
    }

    print $FILE "    CAMX_LOG_VERBOSE(CamxLogGroupCore, \"================ END DUMP OF CURRENT";
    if (!$isHardwareIndependent)
    {
        print $FILE " " . uc($hardwareNameUpper);
    }
    print $FILE " HARDWARE-" . uc($settingsTypePrefix) . "DEPENDENT SETTINGS VALUES ================\");\n";

    if ($isHardwareIndependent)
    {
        print $FILE "\n";
        print $FILE "    HwDumpSettings();\n";
    }

    print $FILE "}\n";
    print $FILE "#endif // SETTINGS_DUMP_ENABLE\n";
    #end DumpSettings

    print $FILE "\n";
    print $FILE "CAMX_NAMESPACE_END\n";

    $outFileObj->Close();
}

#*******************************************************************************************************************************
# CreatePublicOverrideTextFile
#
# @brief    Generates a fully commented out override settings text file with public settings only.
#
# @return   None
#*******************************************************************************************************************************
sub CreatePublicOverrideTextFile
{
    my $isHardwareIndependent = shift;

    if ($isHardwareIndependent)
    {
        # Hardware independent settings so create the files
        open ($FILE, '>', $publicOverrideFilePath) or die $!;

        EmitCopyright(*$FILE, ";", "\n");

        print $FILE "\n\n";
        print $FILE ";#################################################################################################\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#!!!!!!! AUTO-GENERATED FILE! DO NOT CHECKIN! This file may be modified and distributed. !!!!!!!#\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#################################################################################################\n";
        print $FILE "\n\n\n";
    }
    else
    {
        # Hardware dependent settings so append the files
        open ($FILE, '>>', $publicOverrideFilePath) or die $!;
    }

    print $FILE ";" . ("/" x 127) . "\n";
    print $FILE "; $settingsXMLData->{Name} Overrides\n";
    print $FILE ";" . ("/" x 127) . "\n";
    print $FILE "\n";

    # Emit each setting name, help text, type, hashed name and default value, and variable name and default value
    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            # Only emit to the public file if setting is specified as public
            if (defined($setting->{Public}) && ($setting->{Public} eq "TRUE"))
            {
                EmitCommentedSetting(*$FILE, $setting);
            }
        }
    }

    close $FILE;
}

#*******************************************************************************************************************************
# CreatePrivateOverrideTextFile
#
# @brief    Generates a fully commented out override settings text file with all settings (private and public).
#
# @return   None
#*******************************************************************************************************************************
sub CreatePrivateOverrideTextFile
{
    my $isHardwareIndependent = shift;

    if ($isHardwareIndependent)
    {
        # Hardware independent settings so create the files
        open ($FILE, '>', $privateOverrideFilePath) or die $!;

        EmitCopyright(*$FILE, ";", "\n");

        print $FILE "\n\n";
        print $FILE ";#############################################################################\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#!!!!!!! DO NOT DISTRIBUTE! This file contains proprietary settings. !!!!!!!#\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#############################################################################\n";
        print $FILE "\n\n\n";
        print $FILE ";#####################################################################################################\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#!!!!!!! AUTO-GENERATED FILE! DO NOT CHECKIN! This file may be modified and used internally. !!!!!!!#\n";
        print $FILE ";#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#\n";
        print $FILE ";#####################################################################################################\n";
        print $FILE "\n\n\n";
    }
    else
    {
        # Hardware dependent settings so append the files
        open ($FILE, '>>', $privateOverrideFilePath) or die $!;
    }

    print $FILE ";" . ("/" x 127) . "\n";
    print $FILE "; $settingsXMLData->{Name} Overrides\n";
    print $FILE ";" . ("/" x 127) . "\n";
    print $FILE "\n";

    # Emit each setting name, help text, type, hashed name and default value, and variable name and default value
    foreach my $settingsSubGroup (@{$settingsXMLData->{settingsSubGroup}})
    {
        foreach my $setting (@{$settingsSubGroup->{setting}})
        {
            EmitCommentedSetting(*$FILE, $setting);
        }
    }

    close $FILE;
}

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
