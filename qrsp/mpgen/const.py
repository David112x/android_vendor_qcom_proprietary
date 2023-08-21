'''
Copyright (c) 2017 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

Created on May 1, 2017

@author: amavrin
'''
import os
import hashlib
import re

#*** CONSTANTS ***
VERSION_BASE_VER = '2.7'
NAME = 'MPGen'


def get_all_files(path, exclude_folders = None):
	"get all files including hidden"
	for root, dirnames, files in os.walk(path):
		if exclude_folders:
			banned = set(dirnames) & set(exclude_folders)  # s.intersection(t)
			for dirname in banned:
				dirnames.remove(dirname)  # tell walk not to go there

		for filename in files:
			yield os.path.join(root, filename)


# generate MPGen version
mpgen_path = os.path.dirname(os.path.realpath(__file__))
# mpgen_files = os.listdir(mpgen_path)
mpgen_files = get_all_files(mpgen_path, exclude_folders = ['.git'])  # --
# filter out hidden and .pyc
mpgen_files = [fl for fl in mpgen_files if not fl.startswith('.')]
mpgen_files = [fl for fl in mpgen_files if not fl.endswith('.pyc')]
# calculate MPGen hash
m = hashlib.sha256()
for fl in sorted(mpgen_files):
	with open(fl, "rb") as f:
		m.update(f.read())
# parse out MPGen interface version from the rtic_mp_header.template.h
with open(os.path.join(mpgen_path, 'templates', 'rtic_mp_header.template.h'), 'rb') as f:
	mpgen_interface_version = re.findall(r'RTIC_MPGEN_INTERFACE_VERSION.*?(\d+)', f.read(), flags = 0)[0]

VERSION = "%s.%s.%s" % (VERSION_BASE_VER, m.hexdigest()[:6], mpgen_interface_version)
