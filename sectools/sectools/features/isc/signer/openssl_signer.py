# ===============================================================================
#
#  Copyright (c) 2013-2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================================

import os

from sectools.common import crypto
from sectools.common.utils import c_path
from sectools.common.utils.c_attribute import Attribute
from sectools.common.utils.c_logging import logger
from sectools.features.isc import data_provision_enquirer as openssl_fetch_module
from sectools.features.isc.hasher.hasher import Hasher
from sectools.features.isc.signer import signerutils
from sectools.features.isc.signer.base_signer import ishmac
from sectools.features.isc.signer.local_v2_signer import LocalV2Signer
from sectools.features.isc.signer.signer_errors import ConfigError
from sectools.features.isc.signer.signer_output import SignerOutput
from sectools.features.isc.signer.signerutils.certconfigparser import CertData
from sectools.features.isc.signer.signerutils.certificate import Certificate


class OpenSSLSigner(LocalV2Signer):
    '''
    classdocs
    '''
    def __init__(self, config):
        super(OpenSSLSigner, self).__init__(config)
        self.openssl_info = openssl_fetch_module.OpenSSLPathsObject(config)

    @classmethod
    def is_plugin(cls):
        return True

    @classmethod
    def signer_id(cls):
        return 'local'

    def sign_hash(self, hash_to_sign, imageinfo, binary_to_sign=None,
                  debug_dir=None, sha_algo=None, binary_to_sign_len=None):
        # Check that openssl is available
        try:
            crypto.are_available([crypto.modules.MOD_OPENSSL])
        except Exception as e:
            raise RuntimeError('Cannot sign: ' + str(e))

        # abstract some of the image information
        signing_attributes = imageinfo.signing_attributes
        general_properties = imageinfo.general_properties

        # GET OPENSSL DATA
        openssl_configfile = self.openssl_info.openssl_config
        openssl_attest_ca_xts = self.openssl_info.attest_ca_xts
        openssl_ca_cert_xts = self.openssl_info.ca_cert_xts

        # GET SIGNING ATTRIBUTE DATA
        debug_val = int(signing_attributes.debug, 16)
        oem_id = int(signing_attributes.oem_id, 16) & 0xFFFF
        model_id = int(signing_attributes.model_id, 16) & 0xFFFF
        num_certs_in_certchain = general_properties.num_certs_in_certchain
        crash_dump = int(signing_attributes.crash_dump, 16)
        rot_en = int(signing_attributes.rot_en, 16)
        in_use_soc_hw_version = signing_attributes.in_use_soc_hw_version
        use_serial_number_in_signing = signing_attributes.use_serial_number_in_signing
        oem_id_independent = signing_attributes.oem_id_independent

        multi_serial_numbers = signing_attributes.multi_serial_numbers.serial if signing_attributes.multi_serial_numbers is not None else []
        app_id = int(signing_attributes.app_id, 16) if signing_attributes.app_id is not None else None
        mask_soc_hw_version = int(signing_attributes.mask_soc_hw_version, 16) if signing_attributes.mask_soc_hw_version is not None else None

        # GET CERTIFICATE INFORMATION
        cert_dict = {}
        cert_dict['id'] = imageinfo.cert_config
        cert_dict['chip'] = self.config.metadata.chipset
        cert_dict['keysize'] = general_properties.key_size
        cert_dict['exponent'] = general_properties.exponent
        cert_dict['mrc_index'] = general_properties.mrc_index

        #TODO: Padding is not supported with ECDSA curves
        cert_dict['padding']= general_properties.rsa_padding.upper() if general_properties.rsa_padding else 'PKCS'
        cert_dict['hash_algorithm'] = general_properties.hash_algorithm
        # Can't use imageinfo.data_prov_basepath because MockImage can't use it
        cert_dict['dp_path'] = self.config.data_provisioning.base_path
        cert_dict['dsa_type'] = general_properties.dsa_type
        cert_dict['ecdsa_curve'] = general_properties.ecdsa_curve
        self.cert_data_object = CertData(cert_dict)
        crypto_params_dict = self.cert_data_object.get_crypto_params()

        # Create the attestation_certificate_key_pair
        attestation_certificate_key_pair = None

        # check if the root cert is using pss
        # if so and we are creating a new attest cert, the attest cert should use pss
        #TODO: how do we deal with this in ECDSA?
        use_pss = general_properties.rsa_padding and general_properties.rsa_padding.lower() == 'pss'
        hash_algorithm = general_properties.hash_algorithm


        logger.debug("Using " + ('PSS' if use_pss else 'PKCS') + " RSA padding")

        root_certificate_params = crypto_params_dict['root_certificate_properties']
        root_certificate_params_is_valid, generate_new_root_certificate = self._validate_certificate_params_dict(root_certificate_params)
        if root_certificate_params_is_valid:
            if generate_new_root_certificate:
                logger.debug('Generating new Root certificate and a random key')
                priv_key, pub_key = crypto.rsa.gen_keys(signing_attributes.exponent, general_properties.key_size)
                root_key_pair = {'private_key': priv_key, 'public_key': pub_key}
                root_cert = crypto.cert.create_cert(priv_key, subject_params=root_certificate_params,
                                                    config=openssl_configfile, self_sign=True,
                                                    hash_algo=hash_algorithm, days=7300, serial=1,
                                                    sign_algo=crypto.cert.SIGN_ALGO_RSA_PSS if use_pss else crypto.cert.SIGN_ALGO_RSA_PKCS,
                                                    pad_hash_algo=hash_algorithm)
            else:
                logger.debug('Using a predefined Root certificate and a predefined key')
                logger.debug('Key Used: ' + root_certificate_params['private_key_path'])
                logger.debug('Certificate Used: ' + root_certificate_params['certificate_path'])
                root_cert, root_key_pair = self._get_certificate_and_key_pair_from_files(root_certificate_params)
        else:
            logger.error("Root certificate params are invalid! Please check config file.")
            raise RuntimeError("Root certificate params are invalid! Please check config file.")

        if num_certs_in_certchain > 2:
            logger.debug("Generating Attestation CA certificate, since certchain size is greater than 2")
            attestation_ca_certificate_params = crypto_params_dict['attest_ca_certificate_properties']
            attestation_ca_params_is_valid, generate_new_attestation_ca = self._validate_certificate_params_dict(attestation_ca_certificate_params)
            if attestation_ca_params_is_valid:
                if generate_new_attestation_ca:
                    logger.debug('Generating new Attestation CA certificate and a random key')
                    priv_key, pub_key = crypto.rsa.gen_keys(signing_attributes.exponent, general_properties.key_size)
                    attestation_ca_certificate_key_pair = {'private_key': priv_key, 'public_key': pub_key}

                    attestation_ca_certificate_req = crypto.cert.create_cert(priv_key,
                                                                             subject_params=attestation_ca_certificate_params,
                                                                             config=openssl_configfile)

                    attestation_ca_certificate = crypto.cert.sign_cert(attestation_ca_certificate_req,
                                                                       root_cert, root_key_pair['private_key'],
                                                                       extfile=openssl_ca_cert_xts,
                                                                       hash_algo=hash_algorithm, days=7300, serial=1,
                                                                       sign_algo=crypto.cert.SIGN_ALGO_RSA_PSS if use_pss else crypto.cert.SIGN_ALGO_RSA_PKCS,
                                                                       pad_hash_algo=hash_algorithm)

                else:
                    logger.debug('Using a predefined Attestation CA certificate and a predefined key')
                    logger.debug('Key Used: ' + attestation_ca_certificate_params['private_key_path'])
                    logger.debug('Certificate Used: ' + attestation_ca_certificate_params['certificate_path'])
                    attestation_ca_certificate, attestation_ca_certificate_key_pair = self._get_certificate_and_key_pair_from_files(attestation_ca_certificate_params)
            else:
                logger.error("Attestation CA certificate params are invalid! Please check config file.")
                raise RuntimeError("Attestation CA certificate params are invalid! Please check config file.")

        attestation_certificate_params = crypto_params_dict['attest_certificate_properties']
        attestation_certificate_params_is_valid, generate_new_attestation_certificate = self._validate_certificate_params_dict(attestation_certificate_params)

        if attestation_certificate_params_is_valid:
            if generate_new_attestation_certificate:
                # TCG support
                if self._is_oid_supported(signing_attributes) is True:
                    if self.validate_oid_from_config(attestation_ca_certificate_params['certificate_path'], signing_attributes) is False:
                        raise ConfigError("{0} min and max are not set correctly in configuration."\
                                          "Signing will not continue.".format(signing_attributes.object_id.name)
                                          )
                    attestation_certificate_extensions_path = self._generate_attestation_certificate_extensions(
                                                                    openssl_attest_ca_xts,
                                                                    signing_attributes.object_id.name,
                                                                    signing_attributes.object_id.min,
                                                                    signing_attributes.object_id.max)
                else:
                    attestation_certificate_extensions_path = openssl_attest_ca_xts

                # Get the binary to sign length
                if binary_to_sign_len is None:
                    if binary_to_sign is not None:
                        binary_to_sign_len = len(binary_to_sign)
                    else:
                        raise RuntimeError('Length of binary could not be computed')

                logger.debug('Generating new Attestation certificate and a random key')
                hmac_params = signerutils.get_hmac_params_from_config(signing_attributes)
                certificate_ou_sw_id = "01 " + hmac_params.sw_id_str + " SW_ID"
                certificate_ou_hw_id = "02 " + hmac_params.msm_id_str + " HW_ID"
                certificate_ou_oem_id = "04 " + "%0.4X" % oem_id + " OEM_ID"
                certificate_ou_sw_size = "05 " + "%0.8X" % binary_to_sign_len + " SW_SIZE"
                certificate_ou_model_id = "06 " + "%0.4X" % model_id + " MODEL_ID"
                certificate_hash_alg = self.SHA_OU_MAP[signing_attributes.hash_algorithm]
                certificate_ou_debug_id = "03 " + "%0.16X" % debug_val + " DEBUG"

                certificate_ou = [certificate_ou_sw_id,
                                  certificate_ou_hw_id,
                                  certificate_ou_oem_id,
                                  certificate_ou_sw_size,
                                  certificate_ou_model_id,
                                  certificate_hash_alg,
                                  certificate_ou_debug_id]

                # Optional attributes
                if app_id:
                    certificate_app_id = "08 " + "%0.16X" % app_id + " APP_ID"
                    certificate_ou.append(certificate_app_id)
                if crash_dump:
                    certificate_crash_dump = "09 " + "%0.16X" % crash_dump + " CRASH_DUMP"
                    certificate_ou.append(certificate_crash_dump)
                if rot_en:
                    certificate_rot_en = "10 " + "%0.16X" % rot_en + " ROT_EN"
                    certificate_ou.append(certificate_rot_en)
                if mask_soc_hw_version:
                    certificate_mask_soc_hw_version = "12 " + "%0.4X" % mask_soc_hw_version + " MASK_SOC_HW_VERSION"
                    certificate_ou.append(certificate_mask_soc_hw_version)
                if in_use_soc_hw_version == 1:
                    certificate_in_use_soc_hw_version = "13 " + "%0.4X" % in_use_soc_hw_version + " IN_USE_SOC_HW_VERSION"
                    certificate_ou.append(certificate_in_use_soc_hw_version)
                if use_serial_number_in_signing == 1:
                    certificate_use_serial_number_in_signing = "14 " + "%0.4X" % use_serial_number_in_signing + " USE_SERIAL_NUMBER_IN_SIGNING"
                    certificate_ou.append(certificate_use_serial_number_in_signing)
                if oem_id_independent == 1:
                    certificate_oem_id_independent = "15 " + "%0.4X" % oem_id_independent + " OEM_ID_INDEPENDENT"
                    certificate_ou.append(certificate_oem_id_independent)
                certificate_ou_sn_list = []
                for index in xrange(0, len(multi_serial_numbers), 6):
                    serial_sublist = multi_serial_numbers[index: index+6]
                    certificate_sn = "16"
                    for serial in serial_sublist:
                        certificate_sn += " " + serial[2:]

                    # fill last SN OU field with zeros
                    zeros = " 00000000" * (6 - len(serial_sublist))
                    certificate_sn += zeros

                    certificate_sn += " SN"
                    certificate_ou_sn_list.append(certificate_sn)
                certificate_ou_sn_list.reverse()
                certificate_ou.extend(certificate_ou_sn_list)
                if 'OU' in attestation_certificate_params.keys():
                    if type(attestation_certificate_params['OU']) == list:
                        for item in attestation_certificate_params['OU']:
                            certificate_ou.append(item)
                    else:
                        certificate_ou.append(attestation_certificate_params['OU'])

                attestation_certificate_params['OU'] = certificate_ou

                if attestation_certificate_key_pair is None:
                    priv_key, pub_key = crypto.rsa.gen_keys(signing_attributes.exponent, general_properties.key_size)
                    attestation_certificate_key_pair = {'private_key': priv_key, 'public_key': pub_key}

                # Create the request
                attestation_certificate_req = crypto.cert.create_cert(attestation_certificate_key_pair['private_key'],
                                                                      subject_params=attestation_certificate_params,
                                                                      config=openssl_configfile)
                ca_cert, ca_priv_key = ((attestation_ca_certificate, attestation_ca_certificate_key_pair['private_key'])
                                        if num_certs_in_certchain > 2 else (root_cert, root_key_pair['private_key']))

                attestation_certificate = crypto.cert.sign_cert(attestation_certificate_req,
                                                                ca_cert, ca_priv_key,
                                                                extfile=attestation_certificate_extensions_path,
                                                                hash_algo=hash_algorithm, days=7300, serial=1,
                                                                sign_algo=crypto.cert.SIGN_ALGO_RSA_PSS if use_pss else crypto.cert.SIGN_ALGO_RSA_PKCS,
                                                                pad_hash_algo=hash_algorithm)

                attestation_certificate = crypto.cert.get_cert_in_format(attestation_certificate, crypto.utils.FORMAT_DER)

                # Clean temp file
                if self._is_oid_supported(signing_attributes) is True:
                    c_path.clean_file(attestation_certificate_extensions_path)

            else:  # generate_new_attestation_certificate == False
                logger.debug('Using a predefined Attestation certificate and a predefined key')
                logger.debug('Key Used: ' + attestation_certificate_params['private_key_path'])
                logger.debug('Certificate Used: ' + attestation_certificate_params['certificate_path'])
                attestation_certificate, attestation_certificate_key_pair = self._get_certificate_and_key_pair_from_files(attestation_certificate_params)
                attestation_certificate = crypto.cert.get_cert_in_format(attestation_certificate, crypto.utils.FORMAT_DER)

                # Since the get_hmac_params_from_certificate_chain always works with the first cert in the cert chain,
                # this function will work for a single der certificate as well.
                is_hmac = ishmac(imageinfo)

                logger.debug('Using ' + ('QTI HMAC' if is_hmac else hash_algorithm) + ' for hash segment')

                hmacParams = self.get_hmac_params_from_cert(crypto.cert.get_text(attestation_certificate))
                hash_to_sign = Hasher().get_hash(binary_to_sign,
                                               hmac_params=hmacParams if is_hmac else None,
                                               sha_algo=sha_algo)

            signature = crypto.rsa.sign(hash_to_sign, attestation_certificate_key_pair['private_key'],
                                        padding=crypto.rsa.RSA_PAD_PSS if use_pss else crypto.rsa.RSA_PAD_PKCS,
                                        hash_algo=hash_algorithm)
        else:
            logger.error("Attestation certificate params are invalid! Please check config file.")
            raise RuntimeError("Attestation certificate params are invalid! Please check config file.")

        if num_certs_in_certchain > 2:
            attestation_ca_certificate = crypto.cert.get_cert_in_format(attestation_ca_certificate, crypto.utils.FORMAT_DER)
        else:
            attestation_ca_certificate = None

        root_cert = crypto.cert.get_cert_in_format(root_cert, crypto.utils.FORMAT_DER)

        root_cert_list = self.cert_data_object.get_rootcerts(general_properties.num_root_certs)

        certificate_list = self._get_certificate_list(general_properties.num_root_certs,
                                                      num_certs_in_certchain,
                                                      attestation_certificate,
                                                      attestation_ca_certificate,
                                                      root_cert,
                                                      root_cert_list)

        cert_chain = crypto.cert.create_cert_chain_bin(certificate_list)

        signer_output = SignerOutput()
        signer_output.root_cert = root_cert
        signer_output.attestation_ca_cert = attestation_ca_certificate
        signer_output.attestation_cert = attestation_certificate
        signer_output.signature = signature
        signer_output.cert_chain = cert_chain
        signer_output.root_cert_list = root_cert_list
        signer_output.root_key = root_key_pair['private_key']
        # Make sure the variable is defined
        try:
            attestation_ca_certificate_key_pair
        except Exception: pass
        else:
            signer_output.attestation_ca_key = attestation_ca_certificate_key_pair['private_key']
        signer_output.attestation_key = attestation_certificate_key_pair['private_key']
        signer_output.hash_to_sign = hash_to_sign

        return signer_output

    def sign(self, binary_to_sign, imageinfo, debug_dir=None, is_hash=False, parsegen=None):
        '''
        This function returns a SignerOutput object which has all the security assets generated
        by the signer.
        '''
        self.validate_config(imageinfo)
        sha_algo = signerutils.get_sha_algo_from_config(imageinfo.signing_attributes)

        if is_hash:
            hash_to_sign = binary_to_sign
            binary_to_sign = None
        else:
            is_hmac = ishmac(imageinfo)

            logger.debug('Using ' + ('QTI HMAC' if is_hmac else 'SHA') + ' for hash segment')

            hmacParams = signerutils.get_hmac_params_from_config(imageinfo.signing_attributes)
            hash_to_sign = Hasher().get_hash(binary_to_sign,
                                             hmac_params=hmacParams if is_hmac else None,
                                             sha_algo=sha_algo)

        signer_output = self.sign_hash(hash_to_sign, imageinfo, binary_to_sign, debug_dir, sha_algo)

        # print certificate properties
        attestation_cert_obj = Certificate(signer_output.attestation_cert)
        logger.info('\nAttestation Certificate Properties:\n' + str(attestation_cert_obj))

        return signer_output

    def validate_config(self, imageinfo):

        self._validate_config(imageinfo.cert_config, imageinfo.general_properties)

        self._validate_oid_values(imageinfo.signing_attributes,
                                    imageinfo.general_properties)

    def _get_certificate_list(self, num_root_certs,
                              num_certs_in_certchain,
                              attestation_certificate,
                              attestation_ca_certificate,
                              root_cert,
                              root_cert_list):
        certificate_list = [attestation_certificate]
        if num_certs_in_certchain > 2:
            certificate_list.append(attestation_ca_certificate)
        if num_root_certs == 1:
            certificate_list.append(root_cert)
        else:
            # multirootcert case
            if num_root_certs > len(root_cert_list):
                raise RuntimeError("num_root_certs \"{0}\" exceeds number of available root certificates \"{1}\""
                                   "\nEnsure that selected_cert_config is configured properly".format(num_root_certs, len(root_cert_list)))
            for i in range(0, num_root_certs):
                certificate_list.append(root_cert_list[i])

        return certificate_list

    def _get_certificate_and_key_pair_from_files(self, certificate_params_dict):
        key_pair = {}
        with open(certificate_params_dict['private_key_path'], 'rb') as private_key_file:
            private_key = private_key_file.read()
            private_key = crypto.rsa.get_key_in_format(private_key, crypto.utils.FORMAT_PEM)
            public_key = crypto.rsa.get_public_key_from_private(private_key)
            key_pair['public_key'] = public_key
            key_pair['private_key'] = private_key

        with open(certificate_params_dict['certificate_path'], 'rb') as cert_file:
            cert = crypto.cert.get_cert_in_format(cert_file.read(), crypto.utils.FORMAT_PEM)

        return cert, key_pair

    def _validate_certificate_params_dict(self, certificate_params_dict):
        certificate_params_is_valid = False
        generate_new_certificate = False
        for key in certificate_params_dict:
            if key not in ['C', 'CN', 'L', 'O', 'ST', 'OU', 'emailAddress']:
                if key not in ['private_key_path', 'certificate_path']:
                    logger.error("Invalid Key is being passed in configuration!" + repr(key))
                    raise RuntimeError("Invalid Key is being passed in configuration!")
                else:
                    # pre-generated cert/key, check if exist
                    if os.path.exists(certificate_params_dict['private_key_path']) is False:
                        err_str = "private_key_path does not exist: {0}!".format(certificate_params_dict['private_key_path'])
                        logger.error(err_str)
                        certificate_params_is_valid = False
                        raise RuntimeError(err_str)

                    if os.path.exists(certificate_params_dict['certificate_path']) is False:
                        err_str = "certificate_path does not exist: {0}!".format(certificate_params_dict['certificate_path'])
                        logger.error(err_str)
                        certificate_params_is_valid = False
                        raise RuntimeError(err_str)

                    cert_version = crypto.cert.get_version(crypto.cert.get_text(c_path.load_data_from_file(certificate_params_dict['certificate_path'])))
                    if cert_version == crypto.cert.CERT_V3:
                        generate_new_certificate = False
                        certificate_params_is_valid = True
                    else:
                        logger.critical('Certificate version is incorrect: ' + str(cert_version))
                        raise RuntimeError('Invalid certificate: ' + certificate_params_dict['certificate_path'])

            else: # generate new cert/key
                certificate_params_is_valid = True
                generate_new_certificate = True

        return certificate_params_is_valid, generate_new_certificate

    def _validate_config(self, cert_config, general_properties):

        if (general_properties.num_root_certs == 0):
            raise ConfigError("Number of root certificates cannot be set zero")

        if (general_properties.num_root_certs > 16):
            raise ConfigError("Number of root certificates cannot be more than 16")

        if (general_properties.mrc_index and
            general_properties.mrc_index >= general_properties.num_root_certs):
            raise ConfigError("Multirootcert index {0} must be smaller than the number of root certs {1}"
                              .format(general_properties.mrc_index, general_properties.num_root_certs))

    def _generate_oid_config(self, oid_name, min_str, max_str):
        min_attr = Attribute.init(num_bits=32, string=min_str)
        max_attr = Attribute.init(num_bits=32, string=max_str)

        oid_str = "%.8X%.8X" % (min_attr.value, max_attr.value)
        oid_cfg = "\n%s=DER:%s:%s:%s:%s:%s:%s:%s:%s" % \
                (Certificate.GetOIDByName(oid_name), oid_str[0:2], oid_str[2:4], oid_str[4:6], oid_str[6:8], \
                 oid_str[8:10], oid_str[10:12], oid_str[12:14], oid_str[14:16])

        return oid_cfg
