if(BUILD_SERVER)
    find_package(Bullet REQUIRED COMPONENTS
        Dynamics
        Collision
        LinearMath
    )
    list(APPEND SERVER_INCLUDE_DIRS ${BULLET_INCLUDE_DIRS})
    list(APPEND SERVER_LIBRARIES
        BulletDynamics
        BulletCollision
        LinearMath
    )
endif()

if(USE_HTTP AND BUILD_CLIENT)
    find_package(CURL REQUIRED)
    list(APPEND CLIENT_LIBRARIES CURL::libcurl)
endif()

if(USE_FREETYPE AND BUILD_RENDERER)
    find_package(Freetype REQUIRED)
    list(APPEND RENDERER_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
    list(APPEND RENDERER_LIBRARIES ${FREETYPE_LIBRARIES})
endif()

if(BUILD_RENDERER)
    find_package(JPEG REQUIRED)
    list(APPEND RENDERER_LIBRARIES JPEG::JPEG)
    list(APPEND RENDERER_INCLUDE_DIRS ${JPEG_INCLUDE_DIRS})
    list(APPEND RENDERER_DEFINITIONS ${JPEG_DEFINITIONS})
endif()

if(USE_CODEC_VORBIS AND BUILD_CLIENT)
    find_package(Ogg CONFIG REQUIRED)
    list(APPEND CLIENT_LIBRARIES Ogg::ogg)
endif()

if(USE_OPENAL AND BUILD_CLIENT)
    find_path(OPENAL_INCLUDE_DIR
        NAMES al.h
        PATH_SUFFIXES AL
        REQUIRED
    )
    find_library(OPENAL_LIBRARY
        NAMES OpenAL32 openal
        REQUIRED
    )
    list(APPEND CLIENT_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
    list(APPEND CLIENT_DEFINITIONS USE_OPENAL)

    if(USE_OPENAL_DLOPEN)
        list(APPEND CLIENT_DEFINITIONS USE_OPENAL_DLOPEN)
    else()
        find_package(Threads REQUIRED)
        list(APPEND CLIENT_LIBRARIES
            Threads::Threads
            ${OPENAL_LIBRARY}
        )
    endif()
endif()

if(USE_CODEC_OPUS AND BUILD_CLIENT)
    find_package(Opus CONFIG REQUIRED)
    find_package(OpusFile CONFIG REQUIRED)
    list(APPEND CLIENT_LIBRARIES
        Opus::opus
        OpusFile::opusfile
    )
    list(APPEND CLIENT_DEFINITIONS USE_CODEC_OPUS)
endif()

if(BUILD_CLIENT)
    if(EMSCRIPTEN)
        list(APPEND CLIENT_COMPILE_OPTIONS -sUSE_SDL=2)
        list(APPEND CLIENT_LINK_OPTIONS -sUSE_SDL=2)
    else()
        find_package(SDL2 CONFIG REQUIRED)
        list(APPEND CLIENT_LIBRARIES
            SDL2::SDL2
            SDL2::SDL2main
        )
        list(APPEND CLIENT_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
        list(APPEND RENDERER_LIBRARIES SDL2::SDL2)
        list(APPEND RENDERER_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
    endif()
endif()

if(USE_CODEC_VORBIS AND BUILD_CLIENT)
    find_package(Vorbis CONFIG REQUIRED)
    list(APPEND CLIENT_LIBRARIES
        Vorbis::vorbis
        Vorbis::vorbisfile
    )
    list(APPEND CLIENT_DEFINITIONS USE_CODEC_VORBIS)
endif()

find_package(ZLIB REQUIRED)
list(APPEND SERVER_LIBRARIES ZLIB::ZLIB)
list(APPEND CLIENT_LIBRARIES ZLIB::ZLIB)