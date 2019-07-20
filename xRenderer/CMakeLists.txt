cmake_minimum_required(VERSION 3.10)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(
        FATAL_ERROR
        "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles."
    )
endif()

project(
    XRenderer
    VERSION 1.0.0
    DESCRIPTION "Vulkan renderer"
    LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-lm")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE STRING "LunarG Vulkan SDK path")
if(VULKAN_SDK)
    set(ENV{VULKAN_SDK} ${VULKAN_SDK})
endif()

message(STATUS "VULKAN_SDK: $ENV{VULKAN_SDK}")

# Be aware that system libraries have priority on SDK in the finding.
find_package(Vulkan REQUIRED)

# Build

# file(GLOB_RECURSE SOURCES )

add_library(
    ${PROJECT_NAME} SHARED
    src/logger.cpp
    src/utils.cpp
)

target_include_directories(
        ${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/lib
        ${Vulkan_INCLUDE_DIRS}
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
