cmake_minimum_required(VERSION 3.10)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(
        FATAL_ERROR
        "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles."
    )
endif()

# Define the project
project(
    XRenderer
    VERSION 1.0.0
    DESCRIPTION "Vulkan renderer"
    LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set CXX flags
set(CMAKE_CXX_STANDARD 11)

# Set debug build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_definitions(-DDEBUG)
endif()

# Check for Vulkan package
# Be aware that system libraries have priority on SDK in the finding.
find_package(Vulkan REQUIRED)
find_package(GLM REQUIRED)

# Build
add_library(${PROJECT_NAME} SHARED)

target_sources(
        ${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/logger.cpp
        ${PROJECT_SOURCE_DIR}/src/utils.cpp
        ${PROJECT_SOURCE_DIR}/include/buildParam.h
        ${PROJECT_SOURCE_DIR}/include/common.h
        ${PROJECT_SOURCE_DIR}/include/logger.h
        ${PROJECT_SOURCE_DIR}/include/platform.h
        ${PROJECT_SOURCE_DIR}/include/utils.h
        ${PROJECT_SOURCE_DIR}/include/vertex.h
        ${PROJECT_SOURCE_DIR}/include/vulkanWindow.h
        ${PROJECT_SOURCE_DIR}/lib/stb/stb_image.h
        ${PROJECT_SOURCE_DIR}/lib/tinyobj/tiny_obj_loader.h
)

target_include_directories(
        ${PROJECT_NAME}
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/lib
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} ${glm_LIBRARIES})

install(
    TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
