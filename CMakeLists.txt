cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(VoxelTrace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(BIN_NAME "VoxelTrace")
file(GLOB IMGUI_SRC "${PROJECT_SOURCE_DIR}/include/imgui/*.cpp")
add_definitions( -DSRCDIR="${PROJECT_SOURCE_DIR}")

if(WIN32)
    include("C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()


find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(src)

