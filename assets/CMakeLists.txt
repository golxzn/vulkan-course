
message(STATUS "ASSETS CREATION")

if(VC_COMPILE_SHADERS)
	message(STATUS "SHADRES COMPILATION")
	include(VCCompileShaders)
	vc_compile_shaders(DIRECTORY ${vc_assets_dir}/shaders)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/compile_commands.json)
	file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json ${vc_root}/compile_commands.json SYMBOLIC)
endif()
