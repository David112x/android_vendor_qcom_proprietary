/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "LogCollectorCommandBuilder.h"

using namespace std;


LogCollectorCommandBuilder::LogCollectorCommandBuilder()
{
    SetOpcodeGenerators();
}

void LogCollectorCommandBuilder::SetOpcodeGenerators()
{
    RegisterOpcodeGenerators("get_state", unique_ptr<OpCodeGeneratorBase>(new LCGetStateGenerator()));
    RegisterOpcodeGenerators("get_config", unique_ptr<OpCodeGeneratorBase>(new LCGetConfigGenerator()));
    RegisterOpcodeGenerators("set_config", unique_ptr<OpCodeGeneratorBase>(new LCSetConfigGenerator()));
    RegisterOpcodeGenerators("reset_config", unique_ptr<OpCodeGeneratorBase>(new LCReSetConfigGenerator()));
    RegisterOpcodeGenerators("set_verbosity", unique_ptr<OpCodeGeneratorBase>(new LCSetVerbosityGenerator()));
    RegisterOpcodeGenerators("get_verbosity", unique_ptr<OpCodeGeneratorBase>(new LCGetVerbosityGenerator()));
    RegisterOpcodeGenerators("start_recording", unique_ptr<OpCodeGeneratorBase>(new LCStartRecordingGenerator()));
    RegisterOpcodeGenerators("stop_recording", unique_ptr<OpCodeGeneratorBase>(new LCStopRecordingGenerator()));
    RegisterOpcodeGenerators("split_recording", unique_ptr<OpCodeGeneratorBase>(new LCSplitRecordingGenerator()));
    RegisterOpcodeGenerators("start_deferred_recording", unique_ptr<OpCodeGeneratorBase>(new LCStartDeferredRecordingGenerator()));
    RegisterOpcodeGenerators("stop_deferred_recording", unique_ptr<OpCodeGeneratorBase>(new LCStopDeferredRecordingGenerator()));
}

LCGetStateGenerator::LCGetStateGenerator():
    OpCodeGeneratorBase("Get Recording and Publishing state of host manager ")
{
    SetOpcodeCommonParams();
}

LCGetConfigGenerator::LCGetConfigGenerator() :
    OpCodeGeneratorBase("Get log collector configuration.")
{
    SetOpcodeCommonParams();
}

LCStartRecordingGenerator::LCStartRecordingGenerator() :
    OpCodeGeneratorBase("Start log recording according to provided parameters")
{
    SetOpcodeCommonParams();
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("CpuType", false, "Select specific cpu.", "fw / ucode")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("RecordingType", false, "Select a recording type.", "raw(default) / txt")));
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("Compress", false, "Compress fw/ucode logs after collection")));
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("Upload", false, "Compress fw/ucode logs after collection")));

}

LCStopRecordingGenerator::LCStopRecordingGenerator() :
    OpCodeGeneratorBase("Stop log recording according to provided parameters")
{
    SetOpcodeCommonParams();
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("CpuType", false, "Select specific cpu.", "fw / ucode")));
}

LCSplitRecordingGenerator::LCSplitRecordingGenerator() :
    OpCodeGeneratorBase("Split all recordings")
{
}


LCStartDeferredRecordingGenerator::LCStartDeferredRecordingGenerator():
    OpCodeGeneratorBase("Start log recording for a new discovered device")
{
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("RecordingType", false, "Select a recording type.", "raw(default) / txt")));
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("Compress", false, "Compress fw/ucode logs after collection")));
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("Upload", false, "Compress fw/ucode logs after collection")));
}


LCStopDeferredRecordingGenerator::LCStopDeferredRecordingGenerator() :
    OpCodeGeneratorBase("Stop log recording for a new discovered device")
{
}


LCSetConfigGenerator::LCSetConfigGenerator() :
    OpCodeGeneratorBase("Set log collector configuration- \npolling interval, log file maximal size, file suffix etc..")
{
    RegisterParam(unique_ptr<ParameterObject>(new IntParameterObject("PollingIntervalMs", false, "Set the polling interval for log collection")));
    RegisterParam(unique_ptr<ParameterObject>(new IntParameterObject("MaxSingleFileSizeMb", false, "Set max size in MB for a single file")));
    RegisterParam(unique_ptr<ParameterObject>(new IntParameterObject("MaxNumberOfLogFiles", false, "Set max number of files created before overriding")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("ConversionFilePath", false, "Set the path to strings conversion files (both ucode and fw)")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("LogFilesDirectory", false, "Set the path for the log recording directory")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("TargetServer", false, "Set the IP or host name of the server to upload logs to")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("UserName", false, "Set the user name for the upload server")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("RemotePath", false, "Set the remote path on the upload server")));
}

LCReSetConfigGenerator::LCReSetConfigGenerator():
    OpCodeGeneratorBase("ReSet default log collector configuration")
{
}

LCSetVerbosityGenerator::LCSetVerbosityGenerator() :
    OpCodeGeneratorBase("Set FW / UCODE module verbosity. \nV-verbose, I-information, E-error, W-warning")
{
    const string moduleParamDescription = "Set specific module verbosity - Advanced";
    const string moduleParamValues = "Any combination of {V, I, E, W}, e.g V, VI, IE .. or empty(\"\")";
    SetOpcodeCommonParams();
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("Persist", false, "If set to true will save verbosity to persistant configuration")));

    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("CpuType", false, "Select specific cpu.", "fw / ucode")));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("DefaultVerbosity", false, "Set Verbosity level for all modules", moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module0", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module1", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module2", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module3", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module4", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module5", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module6", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module7", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module8", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module9", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module10", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module11", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module12", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module13", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module14", false, moduleParamDescription, moduleParamValues, true)));
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("Module15", false, moduleParamDescription, moduleParamValues, true)));
}

LCGetVerbosityGenerator::LCGetVerbosityGenerator() :
    OpCodeGeneratorBase("Get FW / UCODE module verbosity. \nV-verbose, I-information, E-error, W-warning")
{
    SetOpcodeCommonParams();
    RegisterParam(unique_ptr<ParameterObject>(new StringParameterObject("CpuType", true, "Select specific cpu.", "fw / ucode")));
    RegisterParam(unique_ptr<ParameterObject>(new BoolParameterObject("FromDevice", false, "false/empty for verbosity from configuration, true for verbosity from device")));
}


