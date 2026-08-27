include_guard(GLOBAL)

if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER)
    return()
endif()

include(utils/set_output_dirs)

file(GLOB RENDERER_COMMON_SOURCES
    ${SOURCE_DIR}/renderercommon/*.c
)

file(GLOB SDL_RENDERER_SOURCES
    ${SOURCE_DIR}/sdl/sdl_gamma.c
    ${SOURCE_DIR}/sdl/sdl_glimp.c
)

file(GLOB DYNAMIC_RENDERER_SOURCES
    ${SOURCE_DIR}/renderercommon/tr_subs.c
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/q_math.c
)

if(USE_FREETYPE)
    list(APPEND RENDERER_DEFINITIONS BUILD_FREETYPE)
endif()

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_DEFINITIONS USE_RENDERER_DLOPEN)
endif()

list(APPEND RENDERER_LIBRARIES ${COMMON_LIBRARIES})

file(GLOB RENDERER_SOURCES
    ${SOURCE_DIR}/renderer/*.c
)

file(GLOB RENDERER_SHADER_SOURCES
    ${SOURCE_DIR}/renderer/glsl/*.glsl
)

set(SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders.dir)
file(MAKE_DIRECTORY ${SHADERS_DIR})

foreach(SHADER_FILE IN LISTS RENDERER_SHADER_SOURCES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_C_FILE ${SHADERS_DIR}/${SHADER_NAME}.c)

    string(REPLACE "${CMAKE_BINARY_DIR}/" "" SHADER_C_FILE_COMMENT ${SHADER_C_FILE})

    add_custom_command(
        OUTPUT ${SHADER_C_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${SHADER_FILE}
            -DOUTPUT_FILE=${SHADER_C_FILE}
            -DSHADER_NAME=${SHADER_NAME}
            -P ${CMAKE_SOURCE_DIR}/cmake/utils/stringify_shader.cmake
        DEPENDS ${SHADER_FILE}
        COMMENT "Stringify shader ${SHADER_C_FILE_COMMENT}"
    )

    list(APPEND RENDERER_SHADER_C_SOURCES ${SHADER_C_FILE})
endforeach()

set(RENDERER_BASENAME renderer_main)
set(RENDERER_BINARY ${RENDERER_BASENAME})

list(APPEND RENDERER_BINARY_SOURCES
    ${RENDERER_COMMON_SOURCES}
    ${RENDERER_SOURCES}
    ${RENDERER_SHADER_C_SOURCES}
    ${SDL_RENDERER_SOURCES}
    ${RENDERER_LIBRARY_SOURCES}
)

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_BINARY_SOURCES
        ${DYNAMIC_RENDERER_SOURCES}
    )

    add_library(${RENDERER_BINARY} SHARED
        ${RENDERER_BINARY_SOURCES}
    )

    target_link_libraries(${RENDERER_BINARY}
        PRIVATE ${RENDERER_LIBRARIES}
    )

    target_include_directories(${RENDERER_BINARY}
        PRIVATE ${RENDERER_INCLUDE_DIRS}
    )

    target_compile_definitions(${RENDERER_BINARY}
        PRIVATE ${RENDERER_DEFINITIONS}
    )

    target_compile_options(${RENDERER_BINARY}
        PRIVATE ${RENDERER_COMPILE_OPTIONS}
    )

    target_link_options(${RENDERER_BINARY}
        PRIVATE ${RENDERER_LINK_OPTIONS}
    )

    set_output_dirs(${RENDERER_BINARY})
endif()