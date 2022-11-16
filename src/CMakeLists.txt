

add_library(imgui ${IMGUI_SRC})
add_library(scene scene.cpp scene.h 3d_prims.h voxelspace.h)
target_include_directories(scene PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(scene)

add_executable(${BIN_NAME} main.cpp scene.h octree.h voxelspace.h 3d_prims.h)
target_link_libraries(${BIN_NAME} PRIVATE imgui scene glfw Vulkan::Vulkan)
target_include_directories(${BIN_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	add_custom_command(
		TARGET ${BIN_NAME} PRE_BUILD
		COMMAND compile.bat
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
	)
else()
	add_custom_command(
		TARGET ${BIN_NAME} PRE_BUILD
		COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.sh
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
	)
endif()