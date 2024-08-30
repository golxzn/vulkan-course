cmake_minimum_required(VERSION 3.25)

set(vc_root ${CMAKE_CURRENT_SOURCE_DIR})

project(vulkan-course LANGUAGES CXX)

include(${vc_root}/cmake/config.cmake)

#=========================== DEPENDENCIES ============================#

add_subdirectory(${vc_root}/deps)

#=========================== MAIN  TARGET ============================#

file(GLOB_RECURSE vc_sources CONFIGURE_DEPENDS ${vc_root}/code/*.cpp)
file(GLOB_RECURSE vc_headers CONFIGURE_DEPENDS ${vc_root}/code/*.hpp)

add_executable(${PROJECT_NAME} ${vc_sources} ${vc_headers})
target_include_directories(${PROJECT_NAME} PRIVATE ${vc_root}/code/)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ALIAS_PREFIX}::deps)

