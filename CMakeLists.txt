cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(VoxelTrace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(BIN_NAME "VoxelTrace")
file(GLOB IMGUI_SRC "${PROJECT_SOURCE_DIR}/include/imgui/*.cpp")
add_definitions( -DSRCDIR="${PROJECT_SOURCE_DIR}")

if(WIN32)
    #include("C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_TARGET_TRIPLET "x64-windows")
    message(STATUS "Target triplet for is " ${VCPKG_TARGET_TRIPLET})
    find_package(glm CONFIG REQUIRED)
endif()


find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(src)

