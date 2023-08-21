# Minimal supported compiler version is gcc 4.8.3
CXX_VERSION_INFO = $(shell $(CXX) --version)
$(info ***** Using cross-compiler version: $(CXX_VERSION_INFO))

REPOSITORY_LOCATION := $(shell pwd)
BUILD_DIR := $(shell pwd)

include ../shared/BuildInfoCommon.mk
