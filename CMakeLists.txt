cmake_minimum_required(VERSION 3.30)

set(vc_root ${CMAKE_CURRENT_SOURCE_DIR})
include(${vc_root}/cmake/vc-prelude.cmake)

project(vulkan-course
	VERSION 0.1.2
	LANGUAGES CXX
)

include(vc-config)

#=========================== DEPENDENCIES ============================#

add_subdirectory(${vc_deps_dir})

#=========================== MAIN  TARGET ============================#

add_executable(${PROJECT_NAME})

file(GLOB_RECURSE vc_headers CONFIGURE_DEPENDS ${vc_code_dir}/*.hpp)
target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET headers TYPE HEADERS
	BASE_DIRS ${vc_code_dir} FILES ${vc_headers}
)
target_sources(${PROJECT_NAME} PUBLIC
	FILE_SET headers TYPE HEADERS
	BASE_DIRS ${vc_gen_dir} FILES ${vc_project_info_file}
)

file(GLOB_RECURSE vc_sources CONFIGURE_DEPENDS ${vc_code_dir}/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${vc_sources})

target_include_directories(${PROJECT_NAME} PRIVATE ${vc_root}/code/ ${vc_gen_dir})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ALIAS_PREFIX}::deps)

#============================= RESOURCES =============================#

add_subdirectory(${vc_assets_dir})
