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
    app
    VERSION 1.0.0
    DESCRIPTION "Vulkan application"
    LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Set CXX flags
set(CMAKE_CXX_STANDARD 11)

# Set debug build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

# Check for Vulkan package
# Be aware that system libraries have priority on SDK in the finding.
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS ${Vulkan_LIBRARY})
endif()

find_package(glm REQUIRED)
if(NOT glm_FOUND)
	message(FATAL_ERROR "Could not find GLM library!")
else()
	message(STATUS ${glm_LIBRARY})
endif()

if(NOT WIN32)
    find_package(XCB REQUIRED)
    if(NOT XCB_FOUND)
    	message(FATAL_ERROR "Could not find XCB library!")
    else()
        message(STATUS ${XCB_LIBRARY})
    endif()
endif()

find_library(
    xRenderer_LIBRARIES
    NAMES xRenderer
    HINTS "$ENV{XRENDERER_PATH}/lib"
)
if(NOT xRenderer_LIBRARIES)
    message(FATAL_ERROR "Could not find xRenderer library!")
else()
    message(STATUS ${xRenderer_LIBRARIES})
endif()

# Build
link_directories("$ENV{XRENDERER_PATH}/lib")
add_executable(${PROJECT_NAME} WIN32 "")

target_sources(
        ${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/win32Window.cpp
        ${PROJECT_SOURCE_DIR}/xWindow.cpp
        ${PROJECT_SOURCE_DIR}/resource.h
        ${PROJECT_SOURCE_DIR}/resource.rc
        ${PROJECT_SOURCE_DIR}/lib/stb/stb_image.h
        ${PROJECT_SOURCE_DIR}/lib/tinyobj/tiny_obj_loader.h
)

target_include_directories(
       ${PROJECT_NAME}
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        $ENV{XRENDERER_PATH}/include
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} ${glm_LIBRARIES} ${xRenderer_LIBRARIES})
else(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ENV{XRENDERER_PATH}/lib'")
    target_link_libraries(${PROJECT_NAME} ${XCB_LIBRARIES} ${Vulkan_LIBRARY} ${glm_LIBRARIES} ${xRenderer_LIBRARIES})
endif(WIN32)

install(
    TARGETS ${PROJECT_NAME} RUNTIME
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/bin
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
    DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.spv"
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/resources
    DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}
)

# Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
