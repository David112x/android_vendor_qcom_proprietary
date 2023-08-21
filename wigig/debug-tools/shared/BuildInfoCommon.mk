BUILD_DATE       := $(shell date +%F)
LAST_CHANGE_ID   := $(shell cd $(REPOSITORY_LOCATION); git log --format="%h" -n1 2>/dev/null)
LAST_CHANGE_DATE := $(shell cd $(REPOSITORY_LOCATION); git log --date=short --format="%cd" -n1 2>/dev/null)

VERSION_INFO_FILE := $(REPOSITORY_LOCATION)/../shared/SharedVersionInfo.h
$(info Repository Location: $(REPOSITORY_LOCATION))

VERSION_MAJOR  := $(shell grep -nr "define TOOL_VERSION_MAJOR"  $(VERSION_INFO_FILE) | tr -s ' ' | cut -d ' ' -f3)
VERSION_MINOR  := $(shell grep -nr "define TOOL_VERSION_MINOR"  $(VERSION_INFO_FILE) | tr -s ' ' | cut -d ' ' -f3)
VERSION_MAINT  := $(shell grep -nr "define TOOL_VERSION_MAINT"  $(VERSION_INFO_FILE) | tr -s ' ' | cut -d ' ' -f3)
VERSION_INTERM := $(shell grep -nr "define TOOL_VERSION_INTERM" $(VERSION_INFO_FILE) | tr -s ' ' | cut -d ' ' -f3)

HOST_TOOLS_VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MAINT).$(VERSION_INTERM)

MIN_DMTOOLS_VERSION := $(shell grep -nr "define MIN_DMTOOLS_VERSION" $(VERSION_INFO_FILE) | tr -s ' ' | cut -d ' ' -f3 | cut -d '"' -f2)

ifneq (,$(EXT_LAST_CHANGE_ID))
	LAST_CHANGE_ID := $(EXT_LAST_CHANGE_ID)
endif

ifneq (,$(EXT_LAST_CHANGE_DATE))
	LAST_CHANGE_DATE := $(EXT_LAST_CHANGE_DATE)
endif

ifeq (,$(BUILD_ID))
    ifneq (,$(EXT_BUILD_ID))
        BUILD_ID := $(EXT_BUILD_ID)
    endif
endif

CPPFLAGS += \
	-DBUILD_ID=\"$(BUILD_ID)\" \
	-DBUILD_DATE=\"$(BUILD_DATE)\" \
	-DLAST_CHANGE_ID=\"$(LAST_CHANGE_ID)\" \
	-DLAST_CHANGE_DATE=\"$(LAST_CHANGE_DATE)\" \


BUILD_INFO_FILE := $(BUILD_DIR)/build_info.txt
$(info Build info file: $(BUILD_INFO_FILE))

$(shell echo Build Information:                          >  $(BUILD_INFO_FILE))
$(shell echo ==================                          >> $(BUILD_INFO_FILE))
$(shell echo Build ID:            $(BUILD_ID)            >> $(BUILD_INFO_FILE))
$(shell echo Build date:          $(BUILD_DATE)          >> $(BUILD_INFO_FILE))
$(shell echo Host Tools Version:  $(HOST_TOOLS_VERSION)  >> $(BUILD_INFO_FILE))
$(shell echo Last change:         $(LAST_CHANGE_ID)      >> $(BUILD_INFO_FILE))
$(shell echo Last change date:    $(LAST_CHANGE_DATE)    >> $(BUILD_INFO_FILE))
$(shell echo Min DmTools Version: $(MIN_DMTOOLS_VERSION) >> $(BUILD_INFO_FILE))

$(info ***** Build ID:            $(BUILD_ID))
$(info ***** Build date:          $(BUILD_DATE))
$(info ***** Host Tools Version:  $(HOST_TOOLS_VERSION))
$(info ***** Last change:         $(LAST_CHANGE_ID))
$(info ***** Last change date:    $(LAST_CHANGE_DATE))
$(info ***** Min DmTools Version: $(MIN_DMTOOLS_VERSION))