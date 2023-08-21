# Copyright (c) 2017 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

from __future__ import print_function
from __future__ import absolute_import
import mpgen_mpdb
import sys


class Entry():


	def __init__(self, mp_type, name, attrs, value = None, section_or_symbol = mpgen_mpdb.SYMBOL, split_step = 0, split_reserve_size = 0, writer = ''):
		self.mp_type = mp_type
		self.name = name
		self.attrs = attrs
		self.value = value
		self.section_or_symbol = section_or_symbol
		self.split_step = split_step  # currently used by WU split feature - split large assets to smaller chanks
		self.split_reserve_size = split_reserve_size  # currently used by WU split feature - sometimes the size of the symbol in vmlinux.o may be different than in the final vmlinux. If set this tells to split feature to reserve at least this number of bytes.
		self.writer = writer  # authorized writers string
		return

	def dump(self):
		print('self.mp_type == {0}'.format(self.mp_type))
		print('self.name == {0}'.format(self.name))
		print('self.section_or_symbol == {0}'.format(self.section_or_symbol))
		return

class Catalog(list):
	def __init__(self):
		list.__init__(self)
		self.mp_warning_msg = ""  # the warning that will be passed to the TA from the MP
		self.is_checkable_kernel = 1  # the flag that will be passed to the TA and HT
		self.ou_attributes = '0x0'  # OEM Unlock attributes
		self.qr_attributes = '0x0'  # QHEE Report attributes
		return

	def ro_section(self, section_name, attrs = '0x0'):
		el = Entry(mpgen_mpdb.RD_ONLY, section_name, attrs, None, mpgen_mpdb.SECTION)
		self.append(el)
		return

	def wu_section(self, section_name, attrs = '0x0'):
		el = Entry(mpgen_mpdb.WR_ONCE_UNKNOWN, section_name, attrs, None, mpgen_mpdb.SECTION)
		self.append(el)
		return

	def wu_section_split(self, section_name, writer = '', attrs = '0x0', split_step = 1024, split_reserve_size = 0x0):
		el = Entry(mpgen_mpdb.WR_ONCE_UNKNOWN_SPLIT, section_name, attrs, None, mpgen_mpdb.SECTION, split_step = split_step, split_reserve_size = split_reserve_size, writer = writer)
		self.append(el)
		return

	def ro_symbol(self, symbol_name, attrs = '0x0'):
		el = Entry(mpgen_mpdb.RD_ONLY, symbol_name, attrs)
		self.append(el)
		return

	def write_once_known(self, symbol_name, value = None, writer = '', attrs = '0x0'):
		el = Entry(mpgen_mpdb.WR_ONCE_KNOWN, symbol_name, attrs, value, writer = writer)
		self.append(el)
		return

	def write_once_unknown(self, symbol_name, writer = '', attrs = '0x0'):
		el = Entry(mpgen_mpdb.WR_ONCE_UNKNOWN, symbol_name, attrs, writer = writer)
		self.append(el)
		return

	def auth_writer(self, symbol_name, writer, attrs = '0x0'):
		el = Entry(mpgen_mpdb.WR_AUTH_WRITER, symbol_name, attrs, writer = writer)
		self.append(el)
		return

	def kconfig(self, name, attrs):
		el = Entry(mpgen_mpdb.KCONFIG, name, attrs)
		self.append(el)
		return

	def mp_warning(self, msg):
		""" 
			Prints the warning to stderr and adds this to MP. New line character will separate the TA messages
			
			The MP structure capable of holding a short warning/error message
			This message is printed during the TA startup to QSEE log
			
			Note: try to keep it short as this var is about 239b long 
		"""
		print("WARNING: ", msg, file = sys.stderr)  # print to stderr
		# escape special C characters but not /n
		tmp = ""
		for c in msg + "\n":
			if c in ('\\', '"', "%") or not (32 <= ord(c) < 127):
				tmp += '\\x%02x' % ord(c)
			else:
				tmp += c
		self.mp_warning_msg += tmp
		self.mp_warning_msg = self.mp_warning_msg[0:238]  # slice it just in case

	def ou_attributes_set(self, attr):
		"""
			Sets OEM Unlock attributes
		"""
		self.ou_attributes = attr

	def qr_attributes_set(self, attr):
		"""
			Sets QHEE report attributes
		"""
		self.qr_attributes = attr

	def mark_kernel_non_checkable(self, mp_msg = None):
		""" 
			Sets is_checkable_kernel in the MP template header
			Calling to this function disables QHEE protection, TA will ignore this flag and continue
			(RTIC report will contain the warning message as well as the is checkable kernel flag)
		"""
		self.is_checkable_kernel = 0
		if mp_msg:
			self.mp_warning(mp_msg)


def dynamic_ftrace(config):
	"""
	Dynamic Ftrace depends on kprobes that set's ftrace dynamic events (kprobe event)
	However Kprobes seems to be enabled in the secondary boot build and on the production
	Returns the list of configs enabling dynamic ftrace
	"""

	params_of_interest = list(config.get_parameters_regex(".*?CONFIG_DYNAMIC_FTRACE.*?"))
	print ("Kprobes (params of interest): ", [(n.name, n.value) for n in params_of_interest])
	params_of_interest_enabled = [n.name for n in params_of_interest if n.value == 'y']
	return params_of_interest_enabled


def errata(config):

	params_of_interest = list(config.get_parameters_regex(".*?ERRATUM.*?"))
	print ("ARM Errata (params of interest): ", [(n.name, n.value) for n in params_of_interest])
	params_of_interest_enabled = [n.name for n in params_of_interest if n.value == 'y']
	return params_of_interest_enabled


def get_catalog(config):
	cat = Catalog()

	# TEST CATALOG
	# For production catalog contact Qualcomm Technologies, Inc.

	# ---------------------------------------------
	# 1. Generic assets (arranged by an asset name)
	# ---------------------------------------------

	# OEM Unlock and QHEE Report attestation features
	cat.ou_attributes_set("")  # OEM Unlock flags
	cat.qr_attributes_set("")  # QHEE Report flags

	# == kernel banner == (linux_banner, linux_proc_banner)
	cat.ro_symbol('linux_banner', attrs = '0x0')  # RTIC + QHEE
	cat.ro_symbol('linux_proc_banner', attrs = '0x0')  # RTIC + QHEE

	# == selinux_enforcing ==
	cat.write_once_unknown('selinux_enforcing', writer = 'selinux', attrs = 'MP_SECTION_ATTRIBUTE_ALLOW_ONE')  # RTIC + QHEE
	# Using RO instead cat.auth_writer('selinux_enforcing', 'selinux', attrs = 'MP_SECTION_ATTRIBUTE_ENFORCE')
	# cat.ro_symbol('selinux_enforcing', attrs = 'MP_SECTION_ATTRIBUTE_NRTIC')  # QHEE

	# == selinux_hooks ==
	# cat.write_once_unknown('selinux_hooks', attrs = 'MP_SECTION_ATTRIBUTE_NQHEE | MP_SECTION_ATTRIBUTE_NONSTRICT')  # MP_SECTION_ATTRIBUTE_NONSTRICT - pointers affected by kaslr
	cat.ro_symbol('selinux_hooks', attrs = 'MP_SECTION_ATTRIBUTE_NRTIC')  # QHEE only (RTIC pointers affected by KASLR)

	# == ss_initialized ==
	# cat.write_once_unknown('ss_initialized')  # Write Once Unknown - TA + QHEE asset
	# cat.auth_writer('ss_initialized', 'security_load_policy', attrs = 'MP_SECTION_ATTRIBUTE_ENFORCE')  # Authorized Writer - QHEE asset
	# cat.ro_symbol('ss_initialized', attrs = 'MP_SECTION_ATTRIBUTE_NRTIC')  # QHEE only
	cat.write_once_known('ss_initialized', b'\x01\x00\x00\x00', writer = "security_load_policy")  # RTIC + QHEE

	# (OBSOLETE) selinux_ops
	# Note: selinux_ops may not present on recent kernels (i.g. 4.9) cat.write_once_unknown('selinux_ops', attrs = 'MP_SECTION_ATTRIBUTE_ENFORCE | MP_SECTION_ATTRIBUTE_NONSTRICT')

	# Read Only sections - depend on kernel config
	dynamic_ftrace_configs = dynamic_ftrace(config)
	if (dynamic_ftrace_configs):
		# Ftrace or Kprobes enabled
		cat.mp_warning('ftrace on: ' + ", ".join(dynamic_ftrace_configs))

		# Jun 4 18, considering this to be a checkable case
		# Monitoring generic assets above  cat.mark_kernel_non_checkable("non-checkable (ERR)!")

		# Boot up performance will be severely affected if we enable the code sections with FTRACE enabled.
		# cat.auth_writer('.text', '__apply_alternatives __arch_copy_to_user', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
		# cat.auth_writer('.head.text', '__apply_alternatives __arch_copy_to_user', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
		# cat.auth_writer('.init.text', '__apply_alternatives __arch_copy_to_user', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
	elif (errata(config)):
		# No Dynamic Ftrace, no Kprobes, ARM Errata enabled (production variant)
		cat.mp_warning('errata on: ok')
		cat.wu_section('.head.text', attrs = 'MP_SECTION_ATTRIBUTE_NQHEE')
		# cat.wu_section_split('.text', attrs = 'MP_SECTION_ATTRIBUTE_NQHEE', split_step = 1 * 1024 * 1024, split_reserve_size = 0x00ea75e8)
		# May 22 18 Marked as non strict. It looks like HLOS unloads this region.
		# cat.wu_section('.init.text', attrs = 'MP_SECTION_ATTRIBUTE_NQHEE | MP_SECTION_ATTRIBUTE_NONSTRICT')

		# Boot up performance will be severely affected if we enable the code sections with Dynamic FTrace enabled.
		# cat.wu_section_split('.text', writer = '__apply_alternatives', attrs = 'MP_SECTION_ATTRIBUTE_CODE | MP_SECTION_ATTRIBUTE_NRTIC', split_step = 1024 * 1024)

		# TODO: Jun 26, commenting out below as per "memory is overlapping with SharedVM".
		# cat.auth_writer('.text', '__apply_alternatives', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
		# cat.auth_writer('.head.text', '__apply_alternatives', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
		# cat.auth_writer('.init.text', '__apply_alternatives', attrs = 'MP_SECTION_ATTRIBUTE_CODE')
	else :
		# No ARM Errata, no Ftrace, no Kprobes (unlikely variant due to errata should be enabled anyways)
		cat.mp_warning('prod: ok')
		cat.ro_section('.head.text', attrs = 0)
		cat.ro_section('.text', attrs = "MP_SECTION_ATTRIBUTE_NRTIC")
		# May 22 18 Marked as non strict. It looks like HLOS unloads this region.
		# cat.ro_section('.init.text', attrs = 'MP_SECTION_ATTRIBUTE_NONSTRICT')
		# cat.ro_section('populate_all_ro_sections')

	# Kconfig warnings
	# if any of the below configs are enabled this something to warn about
	warn_if_configs_enabled = ['CONFIG_KPROBES', 'CONFIG_FTRACE', 'CONFIG_ARM64_ICACHE_DISABLE', 'CONFIG_ARM64_DCACHE_DISABLE',
				'CONFIG_ARM64','CONFIG_SYSTEM_TRUSTED_KEYS','CONFIG_DM_VERITY','CONFIG_DM_ANDROID_VERITY',
				'CONFIG_DM_VERITY_FEC','CONFIG_DM_VERITY_AVB','CONFIG_IMA','CONFIG_SECURITY',
				'CONFIG_INTEGRITY', 'CONFIG_SECURITYFS', 'CONFIG_EVM', 'CONFIG_MODULES', 'CONFIG_', ]
	params = (config.get_parameter(wc) for wc in warn_if_configs_enabled if config.get_parameter(wc))
	wenabled = [n.name for n in params if n.value == 'y']
	if (len(wenabled)):
		cat.mp_warning('Kconfigs enabled: ' + ", ".join(wenabled))
	
	return cat

