if(NOT USE_CODEC_MP3)
    return()
endif()

if(NOT BUILD_CLIENT)
    return()
endif()

include(utils/disable_warnings)
include(utils/find_include_dirs)

set(INTERNAL_MAD_DIR ${SOURCE_DIR}/thirdparty/libmad-0.15.1b)
list(APPEND CLIENT_DEFINITIONS USE_CODEC_MP3 FPM_DEFAULT)

file(GLOB_RECURSE MAD_SOURCES
    ${INTERNAL_MAD_DIR}/*.c
)

disable_warnings(${MAD_SOURCES})
find_include_dirs(MAD_INCLUDE_DIRS ${INTERNAL_MAD_DIR})
list(APPEND CLIENT_LIBRARY_SOURCES ${MAD_SOURCES})

list(APPEND CLIENT_INCLUDE_DIRS ${MAD_INCLUDE_DIRS})
