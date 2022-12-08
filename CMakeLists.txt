
cmake_minimum_required(VERSION 3.1)
project(GraphicsProject)

#add path for textures etc
add_definitions( -DSRCDIR="${PROJECT_SOURCE_DIR}")
# Set C++17
set (CMAKE_CXX_STANDARD 17)
# Set the name of the ouput binary
set(
	BIN_NAME
	"GraphicsProject"
)

# Add all files from a src folder

include_directories("include")
include_directories("datastructures")
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)
IF(OPENMP_FOUND)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()
find_package(OpenImageDenoise CONFIG REQUIRED)

add_library(scene scene.cpp scene.h)

add_library(tracer raytracer.cpp 
				   raytracer.h
				   scene.h)
target_link_libraries(tracer PRIVATE scene OpenImageDenoise)

# Setup binary output
add_executable(${BIN_NAME} main.cpp raytracer.h)
target_link_libraries(${BIN_NAME} PRIVATE tracer glfw Vulkan::Vulkan)
