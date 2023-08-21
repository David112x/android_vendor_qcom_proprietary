# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================


# Name & version of the tool
SECTOOLS_TOOL_NAME = 'SecTools'
SECTOOLS_TOOL_VERSION = '5.27'

import os
import sys

# Add any external modules to the path
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.abspath(os.path.join(CUR_DIR, '..', 'ext')))

# c_misc needs to be imported to ensure subprocess.check_output method exists
from sectools.common.utils import c_misc


class Paths(object):
    """ Contains common paths used throughout the tool """
    TopLevel = os.path.dirname(CUR_DIR)
    SecToolsPy = os.path.join(os.path.dirname(CUR_DIR), "sectools.py")
