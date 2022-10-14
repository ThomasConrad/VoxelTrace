cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(VoxelTrace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(BIN_NAME "VoxelTrace")
file(GLOB SOURCES "*.cpp")
file(GLOB IMGUI_SRC "${PROJECT_SOURCE_DIR}/include/imgui/*.cpp")
add_definitions( -DSRCDIR="${PROJECT_SOURCE_DIR}")

if(WIN32)
    # for Windows operating system in general
	if(CMAKE_CXX_COMPILER MATCHES "x64/cl.exe")
		set(__ARCH "x64")
	elseif(CMAKE_CXX_COMPILER MATCHES "x86/cl.exe")
		set(__ARCH "x86")
	else()
		set(__ARCH "xUNKNOWN")
		message(ERRRO "Can't determine target architecture")
	endif()

	#now setting triplet
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		set(VCPKG_TARGET_TRIPLET "${__ARCH}-windows-static-md" CACHE STRING "target triplet" FORCE)
		message(STATUS "Target triplet for debug is " ${VCPKG_TARGET_TRIPLET})
	else()
		set(VCPKG_TARGET_TRIPLET "${__ARCH}-windows-static" CACHE STRING "target triplet" FORCE)
		message(STATUS "Target triplet for release is " ${VCPKG_TARGET_TRIPLET})
	endif(CMAKE_BUILD_TYPE MATCHES Debug)

	include_directories("C:/VulkanSDK/1.3.224.1/Include")
	link_directories(("C:/VulkanSDK/1.3.224.1/Lib"))
	link_directories("${PROJECT_SOURCE_DIR}/include/glfw/lib-vc2022")
	include_directories("${PROJECT_SOURCE_DIR}/include/glfw/include")

elseif(UNIX AND NOT APPLE)

endif()
include_directories("${PROJECT_SOURCE_DIR}/include")

# Flags for debug build
set(
	CMAKE_CXX_FLAGS_DEBUG 
	"${CMAKE_CXX_FLAGS_DEBUG} -Wall"
)
# Flags for Release build
set(
	CMAKE_CXX_FLAGS_RELEASE 
	"${CMAKE_CXX_FLAGS_RELEASE} -Wall"
)

add_executable(${BIN_NAME} ${SOURCES} ${IMGUI_SRC})

if(MSVC)
  target_compile_options(${BIN_NAME} PRIVATE /W3 /WX)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
else()
  target_compile_options(${BIN_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
if(WIN32)
	target_link_libraries(${BIN_NAME} vulkan-1.lib)
	target_link_libraries(${BIN_NAME} glfw3.lib)
	add_custom_command(
		TARGET ${BIN_NAME} PRE_BUILD
		COMMAND compile.bat
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
	)
endif()
