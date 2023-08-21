# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================
"""
Created on Sep 16, 2015

@author: coryf
"""

import os

from sectools.common import crypto
from sectools.common.crypto.functions.rsa.openssl import RsaOpenSSLImpl
from sectools.common.utils import c_path, c_misc
from sectools.features.isc.signer import signerutils
from sectools.features.isc.signer.base_remote_client_signer import BaseRemoteClientSigner, \
    BaseRemoteClientSigningRequest
from sectools.features.isc.signer.cass.cass_connector import CassConnector
from sectools.features.isc.signer.cass.signature_package import SignaturePackage
from sectools.features.isc.signer.cass.signing_package import SigningPackage
from sectools.features.isc.signer.cass.signing_request_oid import OidSigningRequest
from sectools.features.isc.signer.signer_errors import ConfigError
from sectools.features.isc.signer.signer_errors import ExternalSignerError
from sectools.features.isc.signer.signer_output import SignerOutput
from sectools.features.isc.signer.signerutils.attestation_cert_base import get_ecdsa_curve_from_text


class CassSigningRequest(BaseRemoteClientSigningRequest):
    def __init__(self, signer, hash_to_sign, data_to_sign, imageinfo, oid_supported):
        BaseRemoteClientSigningRequest.__init__(self, signer, hash_to_sign, data_to_sign, imageinfo)
        self.oid_supported = oid_supported

    def execute(self):
        self._initialize()
        self._sign()

    def _initialize(self):
        self.cass_signer_attributes = self.signer.config.signing.signer_attributes.cass_signer_attributes
        self.signer.signing_package = SigningPackage(self.hash_to_sign,
                                                     self.data_to_sign,
                                                     self.imageinfo.dest_image.image_path,
                                                     self.signer.signing_package_file_name,
                                                     self.imageinfo.signing_attributes)

    def _sign(self):
        self.signer.connector_response = CassConnector(self.cass_signer_attributes,
                                                       self.signer.signing_package_file_name,
                                                       self.imageinfo.dest_image.image_dir).sign()


class CassSigner(BaseRemoteClientSigner):
    MESG_INVALID_OEM_ID = "oem_id \"{0}\" is invalid for CASS signing.\n"
    MESG_INVALID_SIG = "CASS Signer selected. Signature validation failed for {0} \n"
    MESG_INVALID_PADDING = "CASS Signer selected. CASS server signed using RSA{0} padding but RSA{1} padding was expected. Ensure that:\n" \
                           "\t1) cass_capability is correctly configured\n" \
                           "\t2) rsa_padding is correctly configured"
    MESG_MISSING_CASS_CONFIG = "CASS Signer selected. Cass signer attributes are missing in the user config file.\n"
    SIGNING_PACKAGE_FILENAME = "signingpackage.xml"
    SIGNATURE_PACKAGE_FILENAME = "signaturepackage.xml"
    SIGNATURE_RESULT_FILENAME = "results.zip"

    def __init__(self, config):
        BaseRemoteClientSigner.__init__(self, config)
        self.signing_package = None
        self.signature_package = None
        self.cass_output_dir = None
        self.signing_package_file_name = None
        self.connector_response = None
        self.signature_result_file_name = None

    @classmethod
    def is_plugin(cls):
        return True

    @classmethod
    def signer_id(cls):
        return 'cass'

    @classmethod
    def is_prod_signer(cls):
        return True

    def create_signing_request(self, hash_to_sign, data_to_sign, imageinfo):

        if imageinfo.signing_attributes.cass_capability not in OidSigningRequest.get_supported_capabilities() and \
                        int(imageinfo.signing_attributes.oem_id, 16) == 0 and \
                        imageinfo.signing_attributes.use_serial_number_in_signing != 1:
            raise RuntimeError(CassSigner.MESG_INVALID_OEM_ID.format(imageinfo.signing_attributes.oem_id))

        if self.debug_dir is not None:
            self.cass_output_dir = self.debug_dir
        else:
            self.cass_output_dir = imageinfo.dest_image.image_dir

        self.signing_package_file_name = os.path.join(self.cass_output_dir, self.SIGNING_PACKAGE_FILENAME)
        self.signature_result_file_name = os.path.join(self.cass_output_dir, self.SIGNATURE_RESULT_FILENAME)

        cass_signing_request = CassSigningRequest(self, hash_to_sign, data_to_sign, imageinfo,
                                                  self._is_oid_supported(self.signing_attributes))

        return cass_signing_request

    def process_signature_response(self):
        self.signature_package = SignaturePackage(self.connector_response, self.signing_package.get_digest().lower(),
                                                  self.signature_result_file_name)

        [signature, cert_chain_list] = signerutils.readSigFromZip(self.signature_result_file_name)
        cert_text = crypto.cert.get_text(cert_chain_list[0])
        # Extract ecdsa_curve from attestation certificate output
        ecdsa_curve = get_ecdsa_curve_from_text(cert_text)
        if ecdsa_curve is None:
            # Validate padding
            use_pss = crypto.cert.get_sign_algo(cert_text) == crypto.cert.SIGN_ALGO_RSA_PSS
            cass_padding = self.PAD_PSS if use_pss else self.PAD_PKCS
            if cass_padding != self.padding:
                raise ExternalSignerError(self.MESG_INVALID_PADDING.format(cass_padding.upper(), self.padding.upper()))
            # Add signature padding
            signature = RsaOpenSSLImpl.add_sig_padding(signature)
        else:
            signature = crypto.ecdsa.add_sig_padding(signature, ecdsa_curve)

        if not self.validate_sig_using_hash(self.hash_to_sign, signature, cert_chain_list):
            raise ExternalSignerError(self.MESG_INVALID_SIG.format(self.signature_result_file_name))

        # Clean up/save signing package and signature response
        if self.debug_dir is None:
            c_path.clean_file(self.signing_package_file_name)
            c_path.clean_file(self.signature_result_file_name)

        c_misc.store_debug_data_to_file(self.SIGNATURE_PACKAGE_FILENAME, self.connector_response, self.debug_dir)

        # Extract certificates to create signer output
        out_root_cert = None
        out_attest_ca_cert = None
        out_attest_cert = cert_chain_list[0]
        if len(cert_chain_list) == 3:
            out_attest_ca_cert = cert_chain_list[1]
            out_root_cert = cert_chain_list[2]
        elif len(cert_chain_list) == 2:
            out_root_cert = cert_chain_list[1]

        signer_output = SignerOutput()
        signer_output.root_cert = out_root_cert
        signer_output.attestation_ca_cert = out_attest_ca_cert
        signer_output.attestation_cert = out_attest_cert
        signer_output.signature = signature

        return signer_output

    def validate_config(self, imageinfo):
        cass_signer_attributes = self.config.signing.signer_attributes.cass_signer_attributes
        self._validate_config(cass_signer_attributes)

        self._validate_oid_values(imageinfo.signing_attributes,
                                    imageinfo.general_properties,
                                    mandatory=False)

    def _validate_config(self, cass_signer_attributes):
        if not cass_signer_attributes:
            raise ConfigError(self.MESG_MISSING_CASS_CONFIG)
