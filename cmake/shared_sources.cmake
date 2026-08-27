include_guard(GLOBAL)

include(utils/add_git_dependency)
include(utils/disable_warnings)

file(GLOB COMMON_SOURCES
    ${SOURCE_DIR}/qcommon/*.c
)

list(FILTER COMMON_SOURCES EXCLUDE REGEX ".*/vm_javascript.*\\.c$")
list(FILTER COMMON_SOURCES EXCLUDE REGEX ".*/vm_none\\.c$")

disable_warnings(
    ${SOURCE_DIR}/qcommon/unzip.c
    ${SOURCE_DIR}/qcommon/ioapi.c
)

add_git_dependency(${SOURCE_DIR}/qcommon/common.c)

file(GLOB SYSTEM_SOURCES
    ${SOURCE_DIR}/sys/con_log.c
    ${SOURCE_DIR}/sys/sys_main.c
)
list(APPEND SYSTEM_SOURCES ${SYSTEM_PLATFORM_SOURCES})

file(GLOB SERVER_SOURCES
    ${SOURCE_DIR}/server/*.c
)

file(GLOB BOTLIB_SOURCES
    ${SOURCE_DIR}/botlib/*.c
)