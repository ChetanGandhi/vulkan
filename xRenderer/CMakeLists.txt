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
    xRenderer
    VERSION 1.0.0
    DESCRIPTION "Vulkan renderer"
    LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set CXX flags
set(CMAKE_CXX_STANDARD 17)

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

# Build
add_library(${PROJECT_NAME} SHARED "")

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE XR_EXPORTS
)

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES
    "${PROJECT_SOURCE_DIR}/lib/**/*.hxx"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/*.hxx"
)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/lib
    PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

install(
    TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/lib
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/lib
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/bin
    INCLUDES DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/include
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}/include/${PROJECT_NAME}
)

# Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
