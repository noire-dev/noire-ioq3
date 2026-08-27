if(UNIX)
    list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/sys_unix.c)
    list(APPEND SYSTEM_PLATFORM_SOURCES
        $<IF:$<BOOL:${EMSCRIPTEN}>,${SOURCE_DIR}/sys/con_passive.c,${SOURCE_DIR}/sys/con_tty.c>)
    list(APPEND COMMON_LIBRARIES ${CMAKE_DL_LIBS} m)

    if(USE_HTTP)
        list(APPEND CLIENT_PLATFORM_SOURCES ${SOURCE_DIR}/client/cl_http_curl.c)
    endif()
endif()

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".wasm")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

    foreach(opt BUILD_SERVER USE_RENDERER_DLOPEN
                USE_OPENAL_DLOPEN BUILD_GAME_LIBRARIES USE_HTTP)
        set(${opt} OFF CACHE INTERNAL "")
    endforeach()

    list(APPEND CLIENT_LINK_OPTIONS
        -sTOTAL_MEMORY=256MB -sSTACK_SIZE=5MB
        -sMIN_WEBGL_VERSION=1 -sMAX_WEBGL_VERSION=2
        -sEXPORTED_RUNTIME_METHODS=FS,addRunDependency,removeRunDependency
        -sEXIT_RUNTIME=1 -sEXPORT_ES6 -sEXPORT_NAME=${CLIENT_NAME}
    )

    option(EMSCRIPTEN_PRELOAD_FILE "Preload game files into .data file" OFF)
    if(EMSCRIPTEN_PRELOAD_FILE)
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${BASEGAME}")
            message(FATAL_ERROR "No files in '${BASEGAME}' for emscripten preload")
        endif()
        list(APPEND CLIENT_LINK_OPTIONS --preload-file "${BASEGAME}")
    endif()

    list(APPEND POST_CONFIGURE_FUNCTIONS deploy_shell_files)
    function(deploy_shell_files)
        configure_file(${SOURCE_DIR}/web/client.html.in
            ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CLIENT_NAME}.html @ONLY)
        if(NOT EMSCRIPTEN_PRELOAD_FILE)
            configure_file(${SOURCE_DIR}/web/client-config.json
                ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CLIENT_NAME}-config.json COPYONLY)
        endif()
    endfunction()
endif()

if(UNIX AND NOT APPLE)
    list(APPEND CLIENT_DEFINITIONS USE_ICON)
    list(APPEND RENDERER_DEFINITIONS USE_ICON)

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX /opt/quake3 CACHE PATH "" FORCE)
    endif()

    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

if(APPLE)
    option(BUILD_MACOS_APP "Deploy as a macOS .app" ON)
    enable_language(OBJC)

    list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/sys_osx.m)
    list(APPEND COMMON_LIBRARIES "-framework Cocoa")
    list(APPEND CLIENT_LIBRARIES "-framework IOKit")
    list(APPEND RENDERER_LIBRARIES "-framework OpenGL")

    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)

    if(BUILD_MACOS_APP)
        set(CLIENT_EXECUTABLE_OPTIONS MACOSX_BUNDLE)
        list(APPEND POST_CONFIGURE_FUNCTIONS finish_macos_app)
    endif()

    function(finish_macos_app)
        get_filename_component(MACOS_ICON_FILE ${MACOS_ICON_PATH} NAME)

        set(MACOS_APP_BUNDLE_NAME ${CLIENT_NAME})
        set(MACOS_APP_EXECUTABLE_NAME ${CLIENT_BINARY})
        set(MACOS_APP_GUI_IDENTIFIER ${MACOS_BUNDLE_ID})
        set(MACOS_APP_ICON_FILE ${MACOS_ICON_FILE})
        set(MACOS_APP_SHORT_VERSION_STRING ${PRODUCT_VERSION})
        set(MACOS_APP_BUNDLE_VERSION ${PRODUCT_VERSION})
        set(MACOS_APP_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
        set(MACOS_APP_COPYRIGHT ${COPYRIGHT})

        if(PROTOCOL_HANDLER_SCHEME)
            set(MACOS_APP_PLIST_URL_TYPES
                "<key>CFBundleURLTypes</key>
                <array><dict>
                    <key>CFBundleURLName</key><string>${MACOS_APP_BUNDLE_NAME}</string>
                    <key>CFBundleURLSchemes</key>
                    <array><string>${PROTOCOL_HANDLER_SCHEME}</string></array>
                </dict></array>")
        else()
            set(MACOS_APP_PLIST_URL_TYPES "")
        endif()

        configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
            ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
        set_target_properties(${CLIENT_BINARY} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist)

        set(RESOURCES_DIR $<TARGET_FILE_DIR:${CLIENT_BINARY}>/../Resources)
        add_custom_command(TARGET ${CLIENT_BINARY} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${MACOS_ICON_PATH} ${RESOURCES_DIR})

        if(USE_RENDERER_DLOPEN)
            set(MACOS_APP_BINARY_DIR ${CLIENT_BINARY}.app/Contents/MacOS)
            if(BUILD_RENDERER)
                set_output_dirs(${RENDERER_BINARY} SUBDIRECTORY ${MACOS_APP_BINARY_DIR})
                add_dependencies(${CLIENT_BINARY} ${RENDERER_BINARY})
            endif()
        endif()
    endfunction()

    if(DEFINED ENV{APPLE_CERTIFICATE_ID})
        list(APPEND POST_CONFIGURE_FUNCTIONS codesign)
        function(codesign)
            get_directory_property(INSTALL_TARGETS DIRECTORY ${CMAKE_SOURCE_DIR} BUILDSYSTEM_TARGETS)
            foreach(TARGET IN LISTS INSTALL_TARGETS)
                get_target_property(DESTINATION ${TARGET} INSTALL_DESTINATION)
                if(NOT DESTINATION)
                    continue()
                endif()
                add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND codesign --force --deep --options runtime
                        --entitlements ${CMAKE_SOURCE_DIR}/cmake/entitlements.plist
                        --sign "$ENV{APPLE_CERTIFICATE_ID}"
                        "$<TARGET_FILE:${TARGET}>"
                    COMMENT "Code Signing: $<TARGET_FILE_BASE_NAME:${TARGET}>")
            endforeach()
        endfunction()
    endif()

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} Installer")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/misc/macos/macos-dmg-background.png")
    set(CPACK_DMG_SUBDIRECTORY "${CLIENT_NAME}")
    configure_file(
        "${CMAKE_SOURCE_DIR}/misc/macos/macos-dmg-setup.applescript.in"
        "${CMAKE_BINARY_DIR}/macos-dmg-setup.applescript" @ONLY)
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/macos-dmg-setup.applescript")
endif()

if(WIN32)
    list(APPEND SYSTEM_PLATFORM_SOURCES
        ${SOURCE_DIR}/sys/sys_win32.c
        ${SOURCE_DIR}/sys/win_resource.rc
    )
    list(APPEND CLIENT_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_passive.c)
    list(APPEND SERVER_PLATFORM_SOURCES ${SOURCE_DIR}/sys/con_win32.c)
    list(APPEND CLIENT_DEFINITIONS USE_ICON)
    list(APPEND COMMON_LIBRARIES ws2_32 winmm psapi)

    if(MINGW)
        list(APPEND COMMON_LIBRARIES mingw32)
    endif()

    set_source_files_properties(${SOURCE_DIR}/sys/win_resource.rc
        PROPERTIES COMPILE_DEFINITIONS WINDOWS_ICON_PATH="${WINDOWS_ICON_PATH}")

    if(MSVC)
        list(APPEND SERVER_LINK_OPTIONS "/MANIFEST:NO")
        list(APPEND CLIENT_LINK_OPTIONS "/MANIFEST:NO")
    endif()

    set(CLIENT_EXECUTABLE_OPTIONS WIN32)
    set(HOST_EXECUTABLE_SUFFIX ${CMAKE_EXECUTABLE_SUFFIX})

    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON ${WINDOWS_ICON_PATH})
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
endif()