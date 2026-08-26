if(NOT BUILD_SERVER)
    return()
endif()

include(utils/disable_warnings)
include(utils/find_include_dirs)

set(INTERNAL_BULLET_DIR ${SOURCE_DIR}/thirdparty/bullet3-3.25)

file(GLOB_RECURSE BULLET_SOURCES
    ${INTERNAL_BULLET_DIR}/*.cpp
    ${INTERNAL_BULLET_DIR}/*.c
)

disable_warnings(${BULLET_SOURCES})
find_include_dirs(BULLET_INCLUDE_DIRS ${INTERNAL_BULLET_DIR})
list(APPEND SERVER_LIBRARY_SOURCES ${BULLET_SOURCES})

list(APPEND SERVER_INCLUDE_DIRS ${BULLET_INCLUDE_DIRS})