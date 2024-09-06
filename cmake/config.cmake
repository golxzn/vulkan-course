
#======================================= Information ========================================#

cmake_host_system_information(RESULT vc_platform QUERY OS_NAME)
string(TOUPPER ${vc_platform} vc_upper_platform)
string(TOLOWER ${vc_platform} vc_lower_platform)

#========================================== Options =========================================#

set(ALIAS_PREFIX vc)

option(VC_COMPILE_SHADERS             "Compile the shaders"                ON)

#======================================== Directories ========================================#

set(vc_assets_dir   ${vc_root}/assets               CACHE PATH "Path to the assets directory")
set(vc_deps_dir     ${vc_root}/deps                 CACHE PATH "Path to the dependencies directory")
set(vc_code_dir     ${vc_root}/code                 CACHE PATH "Path to the application directory")
set(vc_platform_dir ${vc_code_dir}/platform/${vc_lower_platform} CACHE PATH "Path to the platform directory")

#====================================== Configurations ======================================#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH ${vc_root}/cmake/tools)

add_compile_definitions(
	VC_${vc_upper_platform}
	VC_$<UPPER_CASE:$<CONFIG>>
)

set(project_info_file ${vc_code_dir}/core/info/project.hpp)

set(APPLICATION_NAME "Vulkan Corpse")

configure_file(${project_info_file}.in ${project_info_file} @ONLY NEWLINE_STYLE UNIX)
