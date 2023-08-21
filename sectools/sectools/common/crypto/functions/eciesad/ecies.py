# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================

from sectools.common.crypto.functions.eciesad import EciesBase
from sectools.common.utils.c_process import run_command


class EciesImpl(EciesBase):
    def __init__(self, module):
        super(EciesImpl, self).__init__(module)
        self.ecies = module

    def encrypt(self, key, aad, message):
            cmd = [self.ecies,
                   '--encrypt',
                   '--key', key,
                   '--aad', aad,
                   '--msg', message]
            return run_command(cmd, log=False)

    def decrypt(self, key, cryptogram):
            cmd = [self.ecies,
                   '--decrypt',
                   '--key', key,
                   '--cryptogram', cryptogram]
            return run_command(cmd, log=False)
