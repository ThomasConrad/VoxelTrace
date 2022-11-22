

add_library(imgui ${IMGUI_SRC})
add_library(scene scene.cpp scene.h helpers.hpp voxelspace.h)
target_include_directories(scene PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(scene)

add_executable(${BIN_NAME} main.cpp scene.h octree.h voxelspace.h helpers.hpp)
target_link_libraries(${BIN_NAME} PRIVATE imgui scene glfw Vulkan::Vulkan)
target_include_directories(${BIN_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")

add_executable(octest octest.cpp octree.h)


if(MSVC)
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/shaders")
	add_custom_command(
		TARGET ${BIN_NAME} PRE_BUILD
		COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.bat ${PROJECT_SOURCE_DIR}/shaders
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
	)
else()
	file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/shaders")
	add_custom_target(
		compileShaders
		COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.sh ${PROJECT_SOURCE_DIR}/shaders
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
	)
	add_dependencies(${BIN_NAME} compileShaders)
endif()