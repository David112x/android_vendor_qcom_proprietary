#!/usr/bin/python2.7
"""
ModuleName:        BuildAndGenerateBins

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

DESCRIPTION:

    Build and genarate the binaries required for tuning binaries

"""

# ==============================================================================
# Import python libraries
import argparse
import os
import platform
import shutil
import subprocess
import sys
import yaml
import fnmatch

# =============================================================================
# Import local modules

# =============================================================================
# Static Variables
fileDescription = 'Runs all auto-generate steps for bin file generation'
version         = '2.1.0'

# =============================================================================


# =============================================================================

def isExisted(targets):
    for t in targets:
        if not os.path.exists(t):
            return False
    return True
# end of isExisted(targets):

def getTargetTimestamp(targets, debug=False):
    assert len(targets) > 0 , "No target is set."
    ts = os.stat(targets[0]).st_mtime
    for t in targets[1:]:
        cur = os.stat(t).st_mtime
        if debug:
            print "target: {}, timestamp: {}".format(t, cur)
        if cur < ts:
            ts = cur
    return ts
# end of getTargetTimestamp(targets):

def checkDependency(deps, target_ts):
    for d in deps:
        if not os.path.exists(d):
            return True
        if os.stat(d).st_mtime > target_ts:
            return True
    return  False
# end of checkDependency(deps, target_ts):

class EasyMake(object):
    def __init__(self, force=False,
                       check_deps_only=False,
                       verbose=False,
                       debug=False):
        self._Force          = force;
        self._CheckDepsOnly  = check_deps_only
        self._Verbose        = verbose
        self._Debug          = debug

        self._commandQueue = []
        self._commandInput = []
    # end of __init__

    def addTarget(self, targets, command, dependency, message):
        if isinstance(targets, basestring):
            targets = [targets]
        if isinstance(command, basestring):
            command = [command]
        if isinstance(dependency, basestring):
            dependency = [dependency]
        self._commandInput.append({
                'tgt': targets,
                'cmd': command,
                'dep': dependency,
                'msg': message})
    # end of addSingleTarget

    def _isNeeded(self, rule):
        if not isExisted(rule['tgt']):
            return True

        mtime = getTargetTimestamp(rule['tgt'], self._Debug)
        return checkDependency(rule['dep'], mtime)
    # end of _isNeeded

    def _runCmdHelper(self, cmds):
        for cmd in cmds:
            if self._Debug:
                print
                print "[Command]: {}".format(cmd)
            if cmd.lower().startswith("$(cp)"):
                cmdArr = cmd.split()
                assert len(cmdArr) == 3, "Wrong command: " + cmd
                if os.path.exists(cmdArr[1]):
                    shutil.copy(cmdArr[1], cmdArr[2])
                else:
                    print
                    print "The original path is not existed: " + cmdArr[1]
                    print
            elif cmd.lower().startswith("$(mkdir)"):
                cmdArr = cmd.split()
                assert len(cmdArr) == 2, "Wrong command: " + cmd
                assert not os.path.isfile(cmdArr[1]), "The path is a file: " + cmdArr[1]
                if not os.path.exists(cmdArr[1]):
                    os.makedirs(cmdArr[1])
            elif cmd.lower().startswith("$(rm)"):
                cmdArr = cmd.split()
                for f in cmdArr[1:]:
                    if os.path.isfile(f):
                        os.remove(f)
                    elif os.path.isdir(f):
                        shutil.rmtree(f)
            else:
                proc = subprocess.Popen(cmd.split(),
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
                stdoutdata, stderrdata = proc.communicate()
                if self._Verbose:
                    print
                    print stdoutdata
                    print stderrdata
                    print
                else:
                    if len(stderrdata) > 0:
                        print
                        print stderrdata
                if (proc.returncode != 0) and (proc.returncode != 100):
                    print
                    print stdoutdata
                    print stderrdata
                    print
                    err_msg = "***** PROCESS EXITED WITH CODE: %d *****\n" \
                                % proc.returncode
                    err_msg += "Arguments: %s" % cmd
                    sys.exit(err_msg)
    # end of runCommand(cmds):

    def _runCmd(self):
        numTask = len(self._commandQueue)
        if numTask == 0:
            print "All targets are up-to-date."
            return
        else:
            for idx, rule in enumerate(self._commandQueue):
                print "( {}/{} ) Build {} ... ".format(idx+1, numTask, rule['msg']),

                if self._CheckDepsOnly:
                    print "Need to be updated (NOT updated because argument "\
                        "\"--check-deps-only\" is given)."
                else:
                    self._runCmdHelper(rule['cmd'])
                    print "Done"
    # end of _runCmd

    def process(self):
        self._commandQueue = []
        if self._Debug:
            print "There are {} targets.".format(len(self._commandInput))

        if self._Force:
            self._commandQueue = self._commandInput
        else:
            for rule in self._commandInput:
                if self._isNeeded(rule):
                    self._commandQueue.append(rule)
            # if the dependency of a rule is updated as the target of others,
            # then this rule need to be updated too
            while True:
                additionalRule = []
                for rule in self._commandInput:
                    if rule in self._commandQueue:
                        # avoid the run same rule twice
                        continue
                    found = False
                    for cq in self._commandQueue:
                        for t in cq['tgt']:
                            if t in rule['dep']:
                                found = True
                                break
                        if found:
                            break
                    if found:
                        additionalRule.append(rule)
                if len(additionalRule) > 0:
                    self._commandQueue.extend(additionalRule)
                else:
                    break
        self._runCmd()
        self._commandQueue = []
    # end of process

    def clean(self):
        for rule in self._commandInput:
            print "Clean {} ... ".format(rule['msg']),
            for t in rule['tgt']:
                if self._Verbose:
                    print "Remove {}".format(t)
                if os.path.isfile(t):
                    os.remove(t)
                elif os.path.isdir(t):
                    shutil.rmtree(t)
            print "Done"
    # end of clean

# end of class EasyMake(object):

# =============================================================================

def getParserPath(camx_chicdk_path):
    if 'Linux' == platform.system():
        progDir = "linux64"
        exeName = "ParameterParser"
    else:
        progDir = "win32"
        exeName = "ParameterParser.exe"

    return os.path.join(camx_chicdk_path, 'tools', 'buildbins', progDir, exeName)
# end of getParserPath


def getDirContent(indir, ext=".*", contentType="df", recursive=False,  \
                  includeInput=False, excludePath=None):
    res = []
    if includeInput:
        res.append(indir)

    if (not os.path.exists(indir)) or (not os.path.isdir(indir)):
        return res

    if ext == ".*" or ext == "*":
        ext = ""

    for child in os.listdir(indir):
        if excludePath:
            if isinstance(excludePath, basestring):
                excludePath = [excludePath]
            found = False
            for ep in excludePath:
                if fnmatch.fnmatch(child, ep):
                    found = True
                    break
            if found:
                continue
        cur = os.path.join(indir, child)
        if os.path.isdir(cur):
            if 'd' in contentType:
                res.append(cur)
            if recursive:
                res.extend( \
                    getDirContent(cur, ext, contentType, recursive,
                                  False, excludePath))
        elif os.path.isfile(cur):
            if 'f' in contentType and cur.endswith(ext):
                res.append(cur)
    return res
# end of getDirContent

def readMK(mkfile):
    fid = open(mkfile, 'r')
    lines = []

    curLine = ""
    for l in fid.readlines():
        curLine += l.rstrip()
        if curLine.endswith("\\"):
            curLine = curLine.rstrip("\\")
        elif len(curLine) > 0:
            lines.append(curLine)
            curLine = ""
    fid.close()

    res = {}
    for l in lines:
        firstArr = l.split(":=")
        if len(firstArr) != 2:
            sys.exit("Unknown content: "+l)
        k = firstArr[0].rstrip()
        v = firstArr[1].lstrip()
        files = v.split()
        res[k] = files
    return res
# end of readMK

def getCodeNames(mkfile, prefix):
    mkContent = readMK(mkfile)
    res = []
    for v in mkContent.itervalues():
        res.extend(v)
    res = [os.path.normpath(os.path.join(prefix, x)) for x in res]
    return res
# end of getCodeNames



# =================================================================================
# Todo: Make a base class with logging etc?
class CBuildCodeAndBinary(object):
    """

    Makes auto-generated code and binary files for CamX


    """

    # --------------------------------------------------------------------------
    def __init__(self, chicdkroot, args):
        """
        Initializes the CBuildCodeAndBinary object.

        CBuildCodeAndBinary(r'/my/path/to/camx')

        Possible Exceptions Thrown:

        """
        self._ChiCdkRoot = chicdkroot

        # Extract command line arguments related to this class
        self._Force          = args.force;
        self._CheckDepsOnly  = args.check_deps_only
        self._Verbose        = args.verbose
        self._OutputPaths    = args.bin_path
        self._Debug          = args.debug
        self._DefaultTuning  = args.default_tuning_project
        self._Clean          = args.clean
        self._YamlFileName   = args.yaml_file_name
        self._GenCodeDir     = args.gen_code_dir

        if self._Debug:
            self._Verbose = True


        if self._ChiCdkRoot is None or not isinstance(self._ChiCdkRoot, basestring):
            raise ValueError("Chi-CDK directory must not be None and must be of type \ "
                             "basestring. type found (%s)" % (type(self._ChiCdkRoot)))

        if not os.path.isdir(self._ChiCdkRoot):
            raise ValueError("Chi-CDK directory must be a valid directory accessible by the host system.")


        self.camx_api_path          = os.path.join(self._ChiCdkRoot, "api")

        if 'Linux' == platform.system():
            progDir = "linux64"
            exeName = "ParameterParser"
        else:
            progDir = "win32"
            exeName = "ParameterParser.exe"

        self.camx_parser = os.path.join(self._ChiCdkRoot, 'tools', 'buildbins', \
                                        progDir, exeName)

        # camx command: parser output subcommand [input(s)] [argument(s)]
        if self._Verbose:
            self.camx_cmd = self.camx_parser + " {} {} {} {}"
        else:
            self.camx_cmd = self.camx_parser + " {} {} {} {} -q"

        self.camx_fd_xsd_dir = os.path.join(self.camx_api_path, "fd")
        self.camx_sensor_xsd_dir = os.path.join(self.camx_api_path, "sensor")
        self.camx_chromatix_xsd_dir = os.path.join(self.camx_api_path, \
                                                   "chromatix","XSD")
        self.camx_tuningtypes_xsd = os.path.join(self.camx_chromatix_xsd_dir, \
                                          "ParameterTuningTypes.xsd")

        if self._GenCodeDir:
            self.camx_generated_path = self._GenCodeDir
            self.camx_generated_la_path = self.camx_generated_path
            assert not os.path.isfile(self.camx_generated_path), "The path is a file: " + self.camx_generated_path
            if not os.path.exists(self.camx_generated_path):
                os.makedirs(self.camx_generated_path)
        else:
            self.camx_generated_path = os.path.join(self.camx_api_path, "generated")
            self.camx_generated_la_path = os.path.join(self.camx_generated_path, \
                                                       "build", "android")

        self.camx_fd_mk = os.path.join(self.camx_generated_la_path, \
                                       "g_fd.mk")
        self.camx_sensor_mk         = os.path.join(self.camx_generated_la_path, \
                                          "g_sensor.mk")
        self.camx_chromatix_mk      = os.path.join(self.camx_generated_la_path, \
                                          "g_chromatix.mk")
        self.camx_parser_mk         = os.path.join(self.camx_generated_la_path, \
                                          "g_parser.mk")


        self.camx_fd_dir        = os.path.join(self.camx_generated_path, "g_facedetection")
        self.camx_sensor_dir    = os.path.join(self.camx_generated_path, "g_sensor")
        self.camx_chromatix_dir = os.path.join(self.camx_generated_path, "g_chromatix")
        self.camx_parser_dir    = os.path.join(self.camx_generated_path, "g_parser")


        self.camx_fd_xsd          = getDirContent(self.camx_fd_xsd_dir, ext=".xsd",        \
                                         includeInput=True)
        self.camx_sensor_xsd      = getDirContent(self.camx_sensor_xsd_dir, ext=".xsd",    \
                                         includeInput=True)
        self.camx_chromatix_xsd   = getDirContent(self.camx_chromatix_xsd_dir, ext=".xsd", \
                                         recursive=True, includeInput=True)

        self.camx_global_deps     = [self.camx_parser]
        self.camx_sensor_bin_deps = self.camx_sensor_xsd
        self.camx_tuning_bin_deps = self.camx_chromatix_xsd
        self.camx_fd_bin_deps     = self.camx_fd_xsd

        self.camx_oemqcom_path     = os.path.join(self._ChiCdkRoot, "oem", "qcom")
        self.camx_sample_tuning_proj = os.path.join(self.camx_oemqcom_path,
                                                    "tuning",
                                                    self._DefaultTuning)
        self.camx_default_tuning_proj = os.path.join(self.camx_oemqcom_path,
                                                     "tuning",
                                                     "default_c7project")

        self.mk_conf = {
            "g_fd_mk": {
                "target":     self.camx_fd_mk,
                "command":    self.camx_cmd.format(self.camx_fd_mk, \
                                    "m", self.camx_fd_xsd_dir, \
                                     "-nfacedetection -pfd -l"),
                "dependency": self.camx_global_deps + self.camx_fd_xsd
                    },
            "g_sensor_mk": {
                "target":     self.camx_sensor_mk,
                "command":    self.camx_cmd.format(self.camx_sensor_mk, \
                                    "m", self.camx_sensor_xsd_dir, \
                                    "-nsensor -pImageSensorModule -l"),
                "dependency": self.camx_global_deps + self.camx_sensor_xsd
                    },
            "g_chromatix_mk": {
                "target":     self.camx_chromatix_mk,
                "command":    self.camx_cmd.format(self.camx_chromatix_mk, \
                                    "m", self.camx_chromatix_xsd_dir, \
                                    "-nchromatix -pTuning -ls"),
                "dependency": self.camx_global_deps + self.camx_chromatix_xsd
                    },
            "g_parser_mk": {
                "target":     self.camx_parser_mk,
                "command":    self.camx_cmd.format(self.camx_parser_mk, \
                                    "m", "", \
                                    "-pparser -lu"),
                "dependency": self.camx_global_deps
                    }
        }


        # The targets in code_conf will be extended when the g_*.mk files
        # are generated
        self.code_conf = {
            'FaceDetection': {
                "target":     [self.camx_fd_dir],
                "command":    ["$(rm) " + self.camx_fd_dir, \
                               "$(mkdir) " + self.camx_fd_dir, \
                               self.camx_cmd.format(self.camx_fd_dir, "s", \
                                       self.camx_fd_xsd_dir, \
                                       "-pfd -l")
                               ],
                "dependency": self.camx_global_deps + self.camx_fd_xsd
            },
            'SensorXSD': {
                "target":     [self.camx_sensor_dir],
                "command":    ["$(rm) " + self.camx_sensor_dir, \
                               "$(mkdir) " + self.camx_sensor_dir, \
                               self.camx_cmd.format(self.camx_sensor_dir, "s", \
                                           self.camx_sensor_xsd_dir, \
                                           "-pImageSensorModule -nCamX -l") ],
                "dependency": self.camx_global_deps + self.camx_sensor_xsd
            },
            'ChromatixXSD': {
                 "target":    [self.camx_chromatix_dir],
                "command":    ["$(rm) " + self.camx_chromatix_dir, \
                               "$(mkdir) " + self.camx_chromatix_dir, \
                               self.camx_cmd.format(self.camx_chromatix_dir, "s", \
                                              self.camx_chromatix_xsd_dir, \
                                              "-ls") ],
                "dependency": self.camx_global_deps + self.camx_chromatix_xsd
           },
            'ParserUtils': {
                 "target":    [self.camx_parser_dir],
                "command":    ["$(rm) " + self.camx_parser_dir, \
                               "$(mkdir) " + self.camx_parser_dir, \
                               self.camx_cmd.format(self.camx_parser_dir, "u", \
                                           self.camx_tuningtypes_xsd, \
                                           "-l") ],
                "dependency": self.camx_global_deps + [self.camx_tuningtypes_xsd]
           },
        }


        yamlPath = os.path.join(self._ChiCdkRoot, 'tools', 'buildbins', self._YamlFileName)

        if not os.path.exists(yamlPath):
            print "yaml file: " + self._YamlFileName + " Not found default to use buildbins.yaml"
            self._YamlFileName = "buildbins.yaml"
            yamlPath = os.path.join(self._ChiCdkRoot, 'tools', 'buildbins', self._YamlFileName)

        with open(yamlPath, 'r') as stream:
            self.bin_conf = yaml.load(stream)

        if self._OutputPaths:
            outputPathList = [op.strip() for op in self._OutputPaths.split(',')]
        else:
            outputPathList = [os.path.join(self._ChiCdkRoot, 'oem', 'qcom', 'bin')]

        self._OutputPathList = outputPathList;

    # end of __init__

    def genCode(self):

        buildMK = EasyMake(force           = self._Force,
                           check_deps_only = self._CheckDepsOnly,
                           verbose         = self._Verbose,
                           debug           = self._Debug)
        for k,v in self.mk_conf.items():
            buildMK.addTarget(v['target'], v['command'], v['dependency'], k)
        
        if self._Clean:
            print "Cleaning makefiles for the auto-generated code."
            buildMK.clean()
            print
        else:
            print "Generating makefiles for the auto-generated code."
            buildMK.process()
            print

        if not self._Clean:
            camx_fd_code = getCodeNames(self.camx_fd_mk, self.camx_generated_path)
            camx_sensor_code = getCodeNames(self.camx_sensor_mk, self.camx_generated_path)
            camx_chromatix_code = getCodeNames(self.camx_chromatix_mk, self.camx_generated_path)
            camx_parser_code = getCodeNames(self.camx_parser_mk, self.camx_generated_path)

            self.code_conf["FaceDetection"]["target"].extend(camx_fd_code)
            self.code_conf["SensorXSD"]["target"].extend(camx_sensor_code)
            self.code_conf["ChromatixXSD"]["target"].extend(camx_chromatix_code)
            self.code_conf["ParserUtils"]["target"].extend(camx_parser_code)

        buildCode = EasyMake(force           = self._Force, 
                             check_deps_only = self._CheckDepsOnly, 
                             verbose         = self._Verbose,
                             debug           = self._Debug)
        for k,v in self.code_conf.items():
            buildCode.addTarget(v['target'], v['command'], v['dependency'], k)

        if self._Clean:
            print "Cleaning auto-generated code."
            buildCode.clean()
            print
        else:
            print "Generating auto-generated code."
            buildCode.process()
            print
    # end of genCode(self):

    def genDefaultTuningProj(self):
        camx_sample_tuning_files = getDirContent(self.camx_sample_tuning_proj,
                                                 contentType="f",
                                                 recursive=True,  
                                                 excludePath=["Sensor.*", 
                                                              "Simulation"])

        camx_sample_tuning_dirs = getDirContent(self.camx_sample_tuning_proj,
                                                 contentType="d",
                                                 recursive=True,
                                                 includeInput=True,
                                                 excludePath=["Sensor.*", 
                                                              "Simulation"])

        pathLength = len(self.camx_sample_tuning_proj)

        camx_default_tuning_files = [self.camx_default_tuning_proj +
                                     x[pathLength:] for x in camx_sample_tuning_files]
        
        camx_default_tuning_dirs  = [self.camx_default_tuning_proj +
                                     x[pathLength:] for x in camx_sample_tuning_dirs]

            
        proj_cmd = ['$(rm) {}'.format(self.camx_default_tuning_proj)]
        for dir1 in camx_default_tuning_dirs:
            proj_cmd.append("$(mkdir) {}".format(dir1))
        for idx, f1 in enumerate(camx_sample_tuning_files):
            f2 = camx_default_tuning_files[idx]
            proj_cmd.append("$(cp) {} {}".format(f1, f2))

        buildProj = EasyMake(force           = self._Force, 
                             check_deps_only = self._CheckDepsOnly, 
                             verbose         = self._Verbose,
                             debug           = self._Debug)
        buildProj.addTarget(camx_default_tuning_dirs + camx_default_tuning_files,
                            proj_cmd,
                            camx_sample_tuning_dirs + camx_sample_tuning_files,
                            "DefaultTuningProject")

        if self._Clean:
            print "Cleaning default tuning project."
            buildProj.clean()
            print
        else:
            print "Generating default tuning project."
            buildProj.process()
            print
    # end of genDefaultTuningProj

    def genBinaries(self):

        # create binary output folders first
        buildBinDir = EasyMake(force           = self._Force, 
                               check_deps_only = self._CheckDepsOnly, 
                               verbose         = self._Verbose,
                               debug           = self._Debug)
        for op in self._OutputPathList:
            buildBinDir.addTarget(op, "$(mkdir) {}".format(op), [],
                                  "OutputDirs")
        if not self._Clean:
            print "Generating output folders."
            buildBinDir.process()
            print

        options = ""
        # create binaries
        buildBin = EasyMake(force           = self._Force, 
                            check_deps_only = self._CheckDepsOnly,
                            verbose         = self._Verbose,
                            debug           = self._Debug)

        for binType, bins in self.bin_conf.items():
            for items in bins:
                for name, files in items.items():
                    output = os.path.realpath(os.path.join(self._OutputPathList[0], name + ".bin"))
                    files = [os.path.join(self._ChiCdkRoot, "oem", "qcom", f) for f in files]

                    input = " ".join(files)

                    if "chromatix_projects" == binType:
                        command = "p"
                        local_depends = self.camx_tuning_bin_deps
                        input_dep = getDirContent(input, contentType="df", 
                                                  recursive=True,
                                                  includeInput=True)
                        
                    elif "facedetection_binaries" == binType:
                        command = "b"
                        local_depends = self.camx_fd_bin_deps
                        input_dep = files
                    elif "sensor_drivers" == binType:
                        command = "b"
                        local_depends = self.camx_sensor_bin_deps
                        input_dep = files
                    else:
                        command = "b"
                        local_depends = []
                        input_dep = files

                    cmd = ["$(rm) {}".format(output),
                           self.camx_cmd.format(output, command, input, options)]
                    deps = self.camx_global_deps + local_depends + input_dep
                    buildBin.addTarget(output, cmd, deps, name)
                    # make copy of binaries 
                    for outputPath in self._OutputPathList[1:]:
                        copy = os.path.realpath(os.path.join(outputPath, name + ".bin"))
                        cmd = ["$(cp) {} {}".format(output, copy)]
                        buildBin.addTarget(copy, cmd, output, "copy of {}".format(name))
        if self._Clean:
            print "Cleaning binaries."
            buildBin.clean()
            print
        else:
            print "Generating binaries."
            buildBin.process()
            print
    # end of genBinaries(self):
# end of class CBuildCodeAndBinary

if __name__ == '__main__':
    # Flush STDOUT
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    chiCdkRoot, filename = os.path.split(os.path.realpath(__file__))
    for i in range (2):
        chiCdkRoot = os.path.join(chiCdkRoot, '..')
    chiCdkRoot = os.path.normpath(chiCdkRoot)

    parser = argparse.ArgumentParser(description='Generate tuning binaries and their C++ parsers')

    parser.add_argument('--debug', help='Show debugging output (you want --verbose?)',
                        action='store_const', const=True)

    parser.add_argument('--verbose', help='Show verbose output',
                        action='store_const', const=True)

    parser.add_argument('--force', help='Ignore timestamps & always force regeneration',
                        action='store_const', const=1)

    parser.add_argument('--check-deps-only', help='Do not generate anyting. For makefile dep gen',
                         action='store_const', const=1)

    parser.add_argument('--bin-path', help='Comma separated list containing paths where compiled .bin files go',
                        action='store')

    parser.add_argument('--default-tuning-project', help='A tuning poject folder, which will be used to create default tuning binary',
                        action='store', default='sm8250_sunny_imx519')

    parser.add_argument('--target', help='Set the target will be generated by the script',
                        action='store', default='all',
                        choices=['all', 'code', 'bin'])

    parser.add_argument('--clean', help='Clean outputs',
                        action='store_const', const=True)

    parser.add_argument('--yaml-file-name', help='yaml file name update for multiple yaml files usage',
                        default='buildbins.yaml', action='store')

    parser.add_argument('--gen-code-dir', help='Integration directory where the generated sources and mk files are outputed',
                        action='store')

    args = parser.parse_args()

    buildCode = CBuildCodeAndBinary(chiCdkRoot, args)

    if args.target == 'all':
        buildCode.genCode()
        buildCode.genDefaultTuningProj()
        buildCode.genBinaries()
    elif args.target == 'code':
        buildCode.genCode()
    elif args.target == 'bin':
        buildCode.genDefaultTuningProj()
        buildCode.genBinaries()
