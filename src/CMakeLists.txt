

add_library(imgui ${IMGUI_SRC})
add_library(scene scene.cpp scene.h helpers.hpp voxelspace.h)
target_include_directories(scene PRIVATE "${PROJECT_SOURCE_DIR}/include")

add_library(iohandler IOHandler.hpp IOHandler.cpp)
add_executable(${BIN_NAME} main.cpp scene.h octree.h voxelspace.h helpers.hpp )
target_link_libraries(${BIN_NAME} PRIVATE iohandler imgui scene glfw Vulkan::Vulkan)
target_include_directories(${BIN_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")

add_executable(octest octest.cpp octree.h)


file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/shaders")
if(MSVC)
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	add_custom_command(
		TARGET ${BIN_NAME} PRE_BUILD
		COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.sh ${PROJECT_SOURCE_DIR}/shaders
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
        DEPENDS "shaders/voxeltrace.frag" "shaders/voxeltrace.frag"
	)
else()
	add_custom_target(
		compileShaders
		COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.sh ${PROJECT_SOURCE_DIR}/shaders
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/shaders"
	)
    #add_dependencies(compileShaders "shaders/voxeltrace.frag" "shaders/voxeltrace.frag")
	add_dependencies(${BIN_NAME} compileShaders)
endif()