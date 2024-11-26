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
        -fms-extensions
        -fdeclspec

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
##  Dependencies
## ============================================================================
include(FetchContent)

set(ICU_FIND_COMPONENTS uc i18n data)
include(FindICU)
if (NOT ICU_FOUND)
    message(FATAL_ERROR "ICU not found. Required components are: uc, i18n, data.")
endif ()

add_definitions(-DLIBASSERT_STATIC_DEFINE)
FetchContent_Declare(
    libassert
    GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
    GIT_TAG        v2.1.0
)

FetchContent_Declare(
    clopts
    GIT_REPOSITORY https://github.com/Sirraide/clopts.git
    GIT_TAG        master
    SOURCE_DIR     "${CMAKE_CURRENT_BINARY_DIR}/clopts"
)

FetchContent_MakeAvailable(libassert clopts)
target_include_directories(libbase PUBLIC "${clopts_SOURCE_DIR}/include")
target_link_libraries(libbase
    PUBLIC
    libassert::assert

    PRIVATE
    ICU::i18n
    ICU::uc
    ICU::data
)

## ============================================================================
##  Testing
## ============================================================================
if (DEFINED BUILD_TESTS)
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
    target_compile_options(tests PRIVATE -fms-extensions -fdeclspec)
    target_compile_definitions(tests PRIVATE
        "LIBBASE_TESTING_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
        "LIBBASE_IS_BUILDING_TESTS"
    )
    catch_discover_tests(tests)
endif()
