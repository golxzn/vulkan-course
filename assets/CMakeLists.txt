cmake_minimum_required(VERSION 3.25)

message(STATUS "ASSETS CREATION")

if(VC_COMPILE_SHADERS)
	message(STATUS "SHADRES COMPILATION")
	include(VCCompileShaders)
	vc_compile_shaders(DIRECTORY ${vc_assets_dir}/shaders)
endif()

