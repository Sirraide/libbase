cmake_minimum_required(VERSION 3.28)
project(libbase VERSION 0.1.0 LANGUAGES CXX)

## ============================================================================
##  Global CMake Variables.
## ============================================================================
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## ============================================================================
##  Main library.
## ============================================================================
file(GLOB_RECURSE _libbase_headers include/*.hh)
file(GLOB_RECURSE _libbase_sources src/*.cc)
add_library(libbase STATIC ${_libbase_sources})
target_sources(libbase PUBLIC
    FILE_SET HEADERS FILES ${_libbase_headers}
)
set_target_properties(libbase PROPERTIES PREFIX "")
target_include_directories(libbase PUBLIC include)

## ============================================================================
##  Compiler options.
## ============================================================================
## Turn on diagnostics colours.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(libbase PUBLIC -fdiagnostics-color=always)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(libbase PUBLIC -fcolor-diagnostics)
endif ()

## Use mold as the default linker, if it exists.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(MOLD_LINKER "mold")
    if (MOLD_LINKER)
        target_link_options(libbase PUBLIC -fuse-ld=mold)
    endif ()
endif ()

## Flags for Clang and GCC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(libbase PRIVATE
        ## Warnings.
        -Wall -Wextra     # Enable ‘all’ warnings.
        -Wundef           # Invalid #undef or undefined macro in #if.
        -Wcast-align      # Casting that changes alignment.
        -Wconversion      # Implicit conversions.
        -Wsign-conversion # Implicit sign conversions.
        -Wformat=2        # Stricter format checking.

        ## Disabled warnings.
        -Wno-unused-function
        -Wno-unused-local-typedefs

        ## NULL Errors.
        -Werror=nonnull # Passing NULL to nonnull parameter.

        ## Memory Errors.
        -Werror=address              # Suspicious use of addresses.
        -Werror=init-self            # Initialization of a variable with itself.
        -Werror=uninitialized

        ## Return type.
        -Werror=return-type
        -Wmissing-noreturn

        ## C/C++.
        -Werror=implicit-fallthrough
        -Werror=pointer-arith        # Disallow void* and function pointer arithmetic.
        -Werror=string-compare       # Nonsensical string comparisons.
        -Werror=switch               # Missing switch cases.
        # -Werror=switch-enum          # Switch on enum (even if there is a default case).
        -Werror=write-strings        # Strings in C should be const char*.

        ## C++.
        -Werror=missing-field-initializers
        -Werror=non-virtual-dtor
        -Werror=pessimizing-move
    )
endif ()

## Additional flags for GCC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(libbase PRIVATE
        -Wlogical-op      # Duplicate or unintended logical operators.
        -Werror=invalid-memory-model # For atomics.
        -Werror=maybe-uninitialized
        -Werror=missing-requires
        -Werror=return-local-addr
    )
endif ()

## Additional flags for Clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(libbase PRIVATE
        -fms-extensions
        -fdeclspec
        -Werror=dangling
        -Werror=return-stack-address
    )
endif ()

## Flags for MSVC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(libbase PRIVATE
        /W4 # Enable ‘all’ warnings.

        # Allow unnamed structs/unions.
        /wd4201

        # Source code is UTF-8.
        /utf-8
    )
endif ()

## On Windows, don’t suggest the _s nonsense functions.
if (WIN32)
    target_compile_definitions(libbase PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS_GLOBALS
        _CRT_NONSTDC_NO_WARNINGS
    )
endif ()

## Address Sanitiser.
if (ENABLE_ASAN)
    target_compile_options(libbase PUBLIC -fsanitize=address)
    target_link_options(libbase PUBLIC -fsanitize=address)
endif ()

## Debug/Release flags.
if (NOT MSVC)
    target_compile_options(libbase PRIVATE
        $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3>
        $<$<CONFIG:RELEASE>:-O3 -march=native>
    )
    target_link_options(libbase PRIVATE
        $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3 -rdynamic>
        $<$<CONFIG:RELEASE>:-O3 -march=native>
    )
else ()
    target_compile_options(libbase PRIVATE
        $<$<CONFIG:DEBUG>:/Od>
        $<$<CONFIG:RELEASE>:/O2>
    )
endif ()

## Finally, apply user-specified flags.
if (DEFINED LIBBASE_CXXFLAGS)
    target_compile_options(libbase PRIVATE ${LIBBASE_CXXFLAGS})
endif()

## ============================================================================
##  Options
## ============================================================================
option(LIBBASE_LITE "Enable lite mode" OFF)
option(LIBBASE_ENABLE_PCRE2 "Enable PCRE2 support" ON)
option(LIBBASE_ENABLE_ZSTD "Enable ZSTD support" ON)
option(LIBBASE_USE_LIBASSERT "Use libassert" ON)
option(LIBBASE_ENABLE_UNICODE_SUPPORT "Enable Unicode support" ON)
option(LIBBASE_LLVM_BINDINGS "Enable LLVM bindings" OFF)

if (LIBBASE_LITE)
    set(LIBBASE_ENABLE_PCRE2 OFF)
    set(LIBBASE_ENABLE_ZSTD OFF)
    set(LIBBASE_USE_LIBASSERT OFF)
    set(LIBBASE_ENABLE_UNICODE_SUPPORT OFF)
endif()

if (LIBBASE_LLVM_BINDINGS)
    target_compile_definitions(libbase PUBLIC -DLIBBASE_ENABLE_LLVM_BINDINGS=1)
endif()

## ============================================================================
##  Dependencies
## ============================================================================
include(FetchContent)

## Add ICU for utf 8<->32 conversion and other unicode-related features.
if (LIBBASE_ENABLE_UNICODE_SUPPORT)
    set(ICU_FIND_COMPONENTS uc i18n data)
    include(FindICU)
    if (NOT ICU_FOUND)
        message(FATAL_ERROR "ICU not found. Required components are: uc, i18n, data.")
    endif ()

    message(STATUS "ICU_INCLUDE_DIRS: ${ICU_INCLUDE_DIRS}")
    message(STATUS "ICU_LIBRARIES: ${ICU_LIBRARIES}")
    message(STATUS "ICU_VERSION: ${ICU_VERSION}")

    target_compile_definitions(libbase PUBLIC LIBBASE_ENABLE_UNICODE_SUPPORT)
    target_include_directories(libbase PRIVATE ${ICU_INCLUDE_DIRS})
    target_link_libraries(libbase PRIVATE
        ICU::i18n
        ICU::uc
        ICU::data
    )
endif()

## Add libassert for better assertions and stack traces.
if (LIBBASE_USE_LIBASSERT)
    set(LIBASSERT_USE_ENCHANTUM ON)
    add_definitions(-DLIBASSERT_STATIC_DEFINE)
    FetchContent_Declare(
        libassert
        GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
        GIT_TAG        v2.2.0
    )
    FetchContent_MakeAvailable(libassert)

    target_compile_definitions(libbase PUBLIC LIBBASE_USE_LIBASSERT)
    target_link_libraries(libbase PUBLIC libassert::assert)
endif()

## Add 'zstd' for compression.
if (LIBBASE_ENABLE_ZSTD)
    set(ZSTD_BUILD_PROGRAMS OFF)
    set(ZSTD_BUILD_CONTRIB OFF)
    set(ZSTD_BUILD_TESTS OFF)
    set(ZSTD_BUILD_STATIC ON)
    set(ZSTD_BUILD_SHARED OFF)
    set(ZSTD_LEGACY_SUPPORT OFF)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd
        GIT_TAG        v1.5.7
    )
    FetchContent_MakeAvailable(zstd)
    target_compile_definitions(libbase PUBLIC LIBBASE_ENABLE_ZSTD)
    target_link_libraries(libbase PUBLIC libzstd_static)
endif()

## Add a proper regex engine.
if (LIBBASE_ENABLE_PCRE2)
    set(PCRE2_BUILD_PCRE2GREP OFF)
    set(PCRE2_BUILD_TESTS OFF)
    set(PCRE2_BUILD_PCRE2_8 ON)
    set(PCRE2_BUILD_PCRE2_16 ON)
    set(PCRE2_BUILD_PCRE2_32 ON)
    FetchContent_Declare(
        pcre2
        GIT_REPOSITORY https://github.com/PCRE2Project/pcre2.git
        GIT_TAG        pcre2-10.45
    )
    FetchContent_MakeAvailable(pcre2)

    target_compile_definitions(libbase PUBLIC LIBBASE_ENABLE_PCRE2)
    target_link_libraries(libbase PRIVATE pcre2-8 pcre2-16 pcre2-32)
endif ()

## Add the command-line options library in any case.
FetchContent_Declare(
    clopts
    GIT_REPOSITORY https://github.com/Sirraide/clopts.git
    GIT_TAG        master
    SOURCE_DIR     "${CMAKE_CURRENT_BINARY_DIR}/clopts"
)

FetchContent_MakeAvailable(clopts)
target_include_directories(libbase PUBLIC "${clopts_SOURCE_DIR}/include")

## ============================================================================
##  Testing
## ============================================================================
if (LIBBASE_BUILD_TESTS)
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.6.0
    )
    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)

    file(GLOB_RECURSE test_sources test/*.cc)
    add_executable(tests ${test_sources})
    target_link_libraries(tests PRIVATE libbase Catch2::Catch2WithMain)
    target_compile_options(tests PRIVATE -fno-access-control)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(tests PRIVATE -fms-extensions -fdeclspec)
    endif()

    target_compile_definitions(tests PRIVATE
        "LIBBASE_TESTING_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
        "LIBBASE_TEST_DIR=\"${PROJECT_SOURCE_DIR}/test\""
        "LIBBASE_IS_BUILDING_TESTS"
    )

    catch_discover_tests(tests)
endif()
