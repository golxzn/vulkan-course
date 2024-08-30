cmake_minimum_required(VERSION 3.25)

set(libraries)

macro(vc_add_subdirectory name link_target)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${name} ${ARGN})
	list(APPEND _libraries ${link_target})
endmacro()


#============================ Vulkan ============================#

find_package(Vulkan REQUIRED)
list(APPEND libraries Vulkan::Vulkan)

#============================= glfw =============================#

option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
vc_add_subdirectory(glfw glfw)

#============================= glm ==============================#

# option(GLM_ENABLE_CXX_20 "Enable C++20 features" ON)
# vkl_add_subdirectory(${vkl_deps_dir}/glm glm::glm)


#================== LIBRARIES CONTAINER TARGET ==================#

add_library(vc_deps INTERFACE)
add_library(${ALIAS_PREFIX}::deps ALIAS vc_deps)
target_link_libraries(vc_deps INTERFACE ${libraries})

unset(libraries)

