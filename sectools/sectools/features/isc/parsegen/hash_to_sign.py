# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================
'''
Created on Apr 27, 2015
@author: hraghav
'''

from sectools.features.isc.parsegen.bin_to_sign import SecParseGenBinToSign


# Hash to sign file type
HASH_TO_SIGN_FILE_TYPE = 'hash_to_sign'


class SecParseGenHashToSign(SecParseGenBinToSign):

    def __init__(self, data, imageinfo=None,
                 hash_to_sign_properties=None,
                 general_properties=None,
                 encdec=None,
                 debug_dir=None,
                 debug_prefix=None,
                 debug_suffix=None,
                 validating=False,
                 signing=False,
                 parsegens=None,
                 sign_attr=False):

        super(SecParseGenHashToSign, self).__init__(data,
                                                    imageinfo=imageinfo,
                                                    bin_to_sign_properties=None,
                                                    general_properties=general_properties,
                                                    encdec=encdec,
                                                    debug_dir=debug_dir,
                                                    debug_prefix=debug_prefix,
                                                    debug_suffix=debug_suffix,
                                                    validating=validating,
                                                    signing=signing,
                                                    parsegens=parsegens,
                                                    sign_attr=sign_attr)

    #--------------------------------------------------------------------------
    # Mandatory Properties overloaded from the base class
    #--------------------------------------------------------------------------
    @classmethod
    def is_plugin(cls):
        return True

    @classmethod
    def file_type(cls):
        return HASH_TO_SIGN_FILE_TYPE

    @property
    def is_data_hash(self):
        return True
