# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================

from ..v3.headers import MbnHdr40B

MBN_HDR_VERSION_5 = 5


class MbnHdr40BV5(MbnHdr40B):
    """ MBN v5 header has a fixed size of 40 bytes """
    def _unpack_data_list(self, unpacked):
        self.image_id = unpacked[0]
        self.version = self.get_version()
        self.sig_size_qti = unpacked[2]
        self.cert_chain_size_qti = unpacked[3]
        self.image_size = unpacked[4]
        self.code_size = unpacked[5]
        self.sig_ptr = unpacked[6]
        self.sig_size = unpacked[7]
        self.cert_chain_ptr = unpacked[8]
        self.cert_chain_size = unpacked[9]

    def _pack_data_list(self):
        pack_order = [self.image_id,
                      self.get_version(),
                      self.sig_size_qti,
                      self.cert_chain_size_qti,
                      self.image_size,
                      self.code_size,
                      self.sig_ptr,
                      self.sig_size,
                      self.cert_chain_ptr,
                      self.cert_chain_size
                      ]
        return pack_order

    def get_format_description(self):
        return self.get_format(), ["image_id", "version", "sig_size_qti", "cert_chain_size_qti", "image_size",
                                   "code_size", "sig_ptr", "sig_size", "cert_chain_ptr", "cert_chain_size"]

    @staticmethod
    def supports_qti_signing():
        return True

    @staticmethod
    def get_version():
        return MBN_HDR_VERSION_5
