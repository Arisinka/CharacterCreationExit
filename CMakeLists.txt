cmake_minimum_required(VERSION 3.28)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

project(CharacterCreationExit VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(COMMONLIBSSE_TAG "v16.9.0")
# Default comes from the environment, so every project shares one checkout
set(COMMONLIB_SOURCE_DIR "$ENV{COMMONLIB_SOURCE_DIR}" CACHE PATH "Shared CommonLibSSE-NG checkout")
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)

if(COMMONLIB_SOURCE_DIR)
    if(NOT EXISTS "${COMMONLIB_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "COMMONLIB_SOURCE_DIR is not a CommonLibSSE-NG checkout.")
    endif()
    set(commonlib_source "${COMMONLIB_SOURCE_DIR}")
else()
    FetchContent_Declare(
        CommonLibSSE-NG
        GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
        GIT_TAG "${COMMONLIBSSE_TAG}"
        GIT_SHALLOW TRUE # If GIT_SHALLOW is enabled then GIT_TAG works only with branch names and tags. A commit hash is not allowed.
        GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(CommonLibSSE-NG)
    FetchContent_GetProperties(CommonLibSSE-NG SOURCE_DIR commonlib_source)
endif()

# Makes it obvious whether the shared checkout or the FetchContent fallback is used
message(STATUS "CommonLibSSE-NG source: ${commonlib_source}")

if(COMMONLIB_SOURCE_DIR)
    add_subdirectory("${commonlib_source}" "${CMAKE_CURRENT_BINARY_DIR}/commonlib")
endif()
include("${commonlib_source}/cmake/CommonLibSSE.cmake")
add_commonlibsse_plugin(${PROJECT_NAME}
    NAME "${PROJECT_NAME}"
    AUTHOR "Arisinka"
    VERSION "${PROJECT_VERSION}"
    SOURCES src/main.cpp)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
    target_link_options(${PROJECT_NAME} PRIVATE /DEBUG:FULL)
endif()
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

set(MOD_PACKAGE_DATA_DIR "" CACHE PATH "Optional staging Data directory")
if(MOD_PACKAGE_DATA_DIR)
    set(plugin_dir "${MOD_PACKAGE_DATA_DIR}/SKSE/Plugins")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${plugin_dir}"
        PDB_OUTPUT_DIRECTORY "${plugin_dir}")
    foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${config}" "${plugin_dir}"
            "PDB_OUTPUT_DIRECTORY_${config}" "${plugin_dir}")
    endforeach()
endif()
