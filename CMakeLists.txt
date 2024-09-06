cmake_minimum_required(VERSION 3.25)

set(vc_root ${CMAKE_CURRENT_SOURCE_DIR})

project(vulkan-course
	VERSION 0.1.0
	LANGUAGES CXX
)

include(${vc_root}/cmake/config.cmake)

#=========================== DEPENDENCIES ============================#

add_subdirectory(${vc_deps_dir})

#=========================== MAIN  TARGET ============================#

file(GLOB_RECURSE vc_sources CONFIGURE_DEPENDS ${vc_code_dir}/*.cpp)
file(GLOB_RECURSE vc_headers CONFIGURE_DEPENDS ${vc_code_dir}/*.hpp)

add_executable(${PROJECT_NAME} ${vc_sources} ${vc_headers})
target_include_directories(${PROJECT_NAME} PRIVATE ${vc_root}/code/)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ALIAS_PREFIX}::deps)

#============================= RESOURCES =============================#

add_subdirectory(${vc_assets_dir})
