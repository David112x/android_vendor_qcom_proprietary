# User Variables
set (SUPPORT_FUNCTION_CALL_TRACE 1)

# NO DATA FOR SECTION: Custom additions (begin)

# Common Include
include (${CAMX_CDK_PATH}/../linuxembedded/common.cmake)

# Files and Build Type
add_library( com.qti.stats.hafoverride SHARED
    ../../src/camxhafoverride.cpp
    ../../src/camxhafoverridepdaf.cpp
    ../../src/camxhafoverridepdaftofmixer.cpp
    ../../src/camxhafoverridetofdecorator.cpp
)

# Libraries to compile
#target_link_libraries (com.qti.stats.hafoverride cutils)
#target_link_libraries (com.qti.stats.hafoverride log)
#target_link_libraries (com.qti.stats.hafoverride sync)
#target_link_libraries (com.qti.stats.hafoverride camxutils)
#target_link_libraries (com.qti.stats.hafoverride camxosutils)

target_link_libraries (com.qti.stats.hafoverride camxosutils)
target_link_libraries (com.qti.stats.hafoverride camxutils)
target_link_libraries (com.qti.stats.hafoverride camxgenerated)
target_link_libraries (com.qti.stats.hafoverride dl)
target_link_libraries (com.qti.stats.hafoverride cutils)
target_link_libraries (com.qti.stats.hafoverride log)
target_link_libraries (com.qti.stats.hafoverride sync)

# Include Paths
include_directories (${CAMX_C_INCLUDES})
include_directories (../../inc)
include_directories (../../inc)
include_directories (${CAMX_CDK_PATH}/stats)
include_directories (${CAMX_CDK_PATH}/fd)
include_directories (${CAMX_CDK_PATH})


if(IQSETTING EQUAL "OEM1")
        include_directories (${CAMX_OEM1IQ_PATH}/chromatix/g_chromatix)
        include_directories (${CAMX_OEM1IQ_PATH}/chromatix/g_parser)
else()
        include_directories (${CAMX_CDK_PATH}/generated/g_chromatix)
        include_directories (${CAMX_CDK_PATH}/generated/g_parser)
endif()

# Dependencies


# Definitions
add_definitions (${CAMX_CFLAGS})
add_definitions (${CAMX_CPPFLAGS})
# NO DATA FOR SECTION: Custom additions (end)

# Auto-Gen
#add_dependencies(com.qti.stats.hafoverride autogen)
install(TARGETS com.qti.stats.hafoverride LIBRARY DESTINATION lib/${CAMX_LIB_OUTPUT_PATH})

if (CONFIG STREQUAL "Debug")
    add_definitions( -g )
endif()

