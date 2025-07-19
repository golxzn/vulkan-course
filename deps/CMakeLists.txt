
set(BUILD_SHARED_LIBS OFF)

set(libraries)

macro(vc_add_subdirectory name link_target)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${name} ${ARGN})
	list(APPEND libraries ${link_target})
endmacro()


# >-------------------------| Vulkan |-------------------------< #

find_package(Vulkan REQUIRED)
list(APPEND libraries Vulkan::Vulkan Vulkan::Headers)

# >--------------------------| glfw |--------------------------< #

option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL    "Generate installation target" OFF)
vc_add_subdirectory(glfw glfw)

# >--------------------------| glm |---------------------------< #

option(GLM_ENABLE_CXX_20 "Enable C++20 features" ON)
option(GLM_ENABLE_SIMD_AVX "Enable AVX optimizations" ON)
option(GLM_ENABLE_SIMD_AVX2 "Enable AVX2 optimizations" ON)
vc_add_subdirectory(glm glm::glm)
target_compile_definitions(glm PUBLIC
	GLM_FORCE_RADIANS
	GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# >--------------------------| fmt |---------------------------< #

vc_add_subdirectory(fmt fmt::fmt)

# >------------------------------------------------------------< #

target_link_libraries(${PROJECT_NAME} PUBLIC ${libraries})

unset(libraries)

