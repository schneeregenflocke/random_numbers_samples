cmake_minimum_required(VERSION 3.16)

include(CMakePrintHelpers)

project(random_numbers_samples)

add_executable(${PROJECT_NAME})


target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
get_target_property(${PROJECT_NAME}_TARGET_CXX_STANDARD ${PROJECT_NAME} CXX_STANDARD)
cmake_print_variables(${PROJECT_NAME}_TARGET_CXX_STANDARD)

cmake_print_variables(CMAKE_SYSTEM_NAME CMAKE_CXX_COMPILER_ID PROJECT_NAME CMAKE_PROJECT_VERSION CMAKE_CURRENT_SOURCE_DIR CMAKE_CURRENT_BINARY_DIR CMAKE_CFG_INTDIR)


if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	target_compile_options(${PROJECT_NAME} PRIVATE -mrdrnd)
endif()


#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/script.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_samples.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/rect4.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/transform.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_numbers.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/random_data_table.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/plot.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/histogram.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/glfw_include.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/file_io.h
)


# libepoxy

find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)
target_include_directories(${PROJECT_NAME} PRIVATE ${EPOXY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${EPOXY_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<epoxy/gl.h>)


# glm

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)


# glfw

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)


# imgui
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_USER_CONFIG=<custom_imconfig.h>)

target_sources(${PROJECT_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui_demo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui_draw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui_internal.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imgui_widgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imstb_rectpack.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imstb_textedit.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/imstb_truetype.h

	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_glfw.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_glfw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_opengl3.h
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/backends)


# licenses

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/submodules/embed-resource)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/embed-resource)

list(APPEND LICENSE_FILES_PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt random_samples
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/glfw/LICENSE.md glfw
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui/LICENSE.txt imgui
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm/copying.txt glm
	${CMAKE_CURRENT_SOURCE_DIR}/submodules/embed-resource/LICENSE.txt embed-resource
)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	list(APPEND LICENSE_FILES_PATHS ${Boost_INCLUDE_DIRS}/LICENSE_1_0.txt boost)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	list(APPEND LICENSE_FILES_PATHS /usr/share/licenses/boost/LICENSE_1_0.txt boost)
endif()

embed_resources(RESOURCES ${LICENSE_FILES_PATHS})

target_sources(${PROJECT_NAME} PRIVATE ${RESOURCES})
