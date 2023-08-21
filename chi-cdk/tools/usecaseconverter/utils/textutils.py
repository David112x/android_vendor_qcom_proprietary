###############################################################################################################################
#
# Copyright (c) 2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import os


def process_path(path):
    if os.name == "nt":
        slash = "\\"
        path  = path.replace("/", "\\")
    else:
        slash = "/"
    path_list = path.split(slash)
    path_stack = []
    for fd in path_list:
        if fd == "..":
            if len(path_stack) > 0 and path_stack[-1] != "..":
                path_stack.pop()
            else:
                path_stack.append(fd)
        elif fd == ".":
            continue
        else:
            path_stack.append(fd)
    return slash.join(path_stack)