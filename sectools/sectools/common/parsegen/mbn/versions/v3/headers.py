# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================

from sectools.common.utils.struct_base import StructBase

MBN_HDR_VERSION_3 = 3
MBN_80_MAGIC = 0x73d71034


class MbnHdr40B(StructBase):

    def _unpack_data_list(self, unpacked):
        self.image_id = unpacked[0]
        self.version = self.get_version()
        self.image_src = unpacked[2]
        self.image_dest_ptr = unpacked[3]
        self.image_size = unpacked[4]
        self.code_size = unpacked[5]
        self.sig_ptr = unpacked[6]
        self.sig_size = unpacked[7]
        self.cert_chain_ptr = unpacked[8]
        self.cert_chain_size = unpacked[9]

    def _pack_data_list(self):
        return [self.image_id,
                self.get_version(),
                self.image_src,
                self.image_dest_ptr,
                self.image_size,
                self.code_size,
                self.sig_ptr,
                self.sig_size,
                self.cert_chain_ptr,
                self.cert_chain_size]

    @classmethod
    def get_format(cls):
        return 'I'*10

    def get_format_description(self):
        return self.get_format(), ["image_id", "version", "image_src", "image_dest_ptr", "image_size", "code_size",
                                   "sig_ptr", "sig_size", "cert_chain_ptr", "cert_chain_size"]

    @staticmethod
    def get_version():
        return MBN_HDR_VERSION_3

    def get_original_size(self):
        return self.get_size()

    def validate(self):
        pass


class MbnHdr80B(MbnHdr40B):

    def _unpack_data_list(self, unpacked):
        self.codeword = unpacked[0]
        self.magic = unpacked[1]
        self.image_id = unpacked[2]
        self.reserved_1 = unpacked[3]
        self.reserved_2 = unpacked[4]
        self.image_src = unpacked[5]
        self.image_dest_ptr = unpacked[6]
        self.image_size = unpacked[7]
        self.code_size = unpacked[8]
        self.sig_ptr = unpacked[9]
        self.sig_size = unpacked[10]
        self.cert_chain_ptr = unpacked[11]
        self.cert_chain_size = unpacked[12]
        self.reserved_3 = unpacked[13]
        self.reserved_4 = unpacked[14]
        self.reserved_5 = unpacked[15]
        self.reserved_6 = unpacked[16]
        self.reserved_7 = unpacked[17]
        self.reserved_8 = unpacked[18]
        self.reserved_9 = unpacked[19]

    def _pack_data_list(self):
        return [self.codeword,
                self.magic,
                self.image_id,
                self.reserved_1,
                self.reserved_2,
                self.image_src,
                self.image_dest_ptr,
                self.image_size,
                self.code_size,
                self.sig_ptr,
                self.sig_size,
                self.cert_chain_ptr,
                self.cert_chain_size,
                self.reserved_3,
                self.reserved_4,
                self.reserved_5,
                self.reserved_6,
                self.reserved_7,
                self.reserved_8,
                self.reserved_9]

    @classmethod
    def get_format(cls):
        return 'I'*20

    def get_format_description(self):
        return self.get_format(), ["codeword", "magic", "image_id", "reserved_1", "reserved_2", "image_src",
                                   "image_dest_ptr", "image_size", "code_size", "sig_ptr", "sig_size", "cert_chain_ptr",
                                   "cert_chain_size", "reserved_3", "reserved_4", "reserved_5", "reserved_6",
                                   "reserved_7", "reserved_8", "reserved_9"]

    @staticmethod
    def get_version():
        return MBN_HDR_VERSION_3
