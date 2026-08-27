if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang$")
    enable_language(ASM)

    set(ASM_SOURCES
        ${SOURCE_DIR}/asm/ftola.c
        ${SOURCE_DIR}/asm/snapvector.c
    )

    add_compile_options(
        -Wall -Wimplicit -Wshadow
        -Wstrict-prototypes -Wformat=2 -Wformat-security
        -Wstrict-aliasing=2 -Wmissing-format-attribute
        -Wdisabled-optimization -Werror-implicit-function-declaration
        -Wno-format-zero-length -Wno-format-nonliteral
        -fno-strict-aliasing
        -fvisibility=hidden
    )
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    include(utils/arch)

    if(ARCH MATCHES "x86|x86_64")
        enable_language(ASM_MASM)

        set(ASM_SOURCES
            ${SOURCE_DIR}/asm/snapvector.asm
            ${SOURCE_DIR}/asm/ftola.asm
        )
    endif()

    if(ARCH MATCHES "x86_64")
        list(APPEND ASM_SOURCES ${SOURCE_DIR}/asm/vm_x86_64.asm)
        set_source_files_properties(${ASM_SOURCES} PROPERTIES COMPILE_DEFINITIONS "idx64")
    endif()

    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:/W4>"
        "$<$<COMPILE_LANGUAGE:C>:/wd4267>"
        "$<$<COMPILE_LANGUAGE:C>:/wd4206>"
        "$<$<COMPILE_LANGUAGE:C>:/wd4324>"
        "$<$<COMPILE_LANGUAGE:C>:/wd4200>"
    )

    add_compile_definitions(
        __inline__=inline
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
endif()