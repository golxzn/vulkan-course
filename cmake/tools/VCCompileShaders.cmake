macro(msg)
	if(NOT CS_SILENT)
		message(${ARGN})
	endif()
endmacro()

# Function: vc_compile_shaders
#
# This function compiles shader files using Vulkan's glslc compiler. It takes in a directory to scan for shader files,
# an output directory to place the compiled shaders, and a list of file extensions to consider as shader files.
# If no output directory is specified, the input directory is used as the output directory.
# If no extensions are specified, the function defaults to ".vert", ".frag", ".geom", ".tesc", ".tese", and ".comp".
#
# Parameters:
#   SILENT - If specified, suppresses all non-error messages.
#   DIRECTORY - The directory to scan for shader files. This argument is required.
#   OUTPUT_DIRECTORY - The directory to place the compiled shaders. If not specified, the input directory is used.
#   EXTENSIONS - A list of file extensions to consider as shader files. If not specified, defaults to ".vert", ".frag", ".geom", ".tesc", ".tese", and ".comp".
#
# Usage:
#   vc_compile_shaders(DIRECTORY input_dir OUTPUT_DIRECTORY output_dir EXTENSIONS ".vert" ".frag")
#
# After calling this function, all shader files in the input directory (and its subdirectories) with the specified extensions
# will be compiled using Vulkan's glslc compiler, and the compiled shaders will be placed in the output directory.
function(vc_compile_shaders)
	include(CMakeParseArguments)

	cmake_parse_arguments(CS "SILENT" "DIRECTORY;OUTPUT_DIRECTORY" "EXTENSIONS" ${ARGN})

	if(NOT CS_DIRECTORY)
		message(FATAL_ERROR "compile_shaders: DIRECTORY not specified")
	endif()
	cmake_path(ABSOLUTE_PATH CS_DIRECTORY NORMALIZE)

	if(NOT CS_OUTPUT_DIRECTORY)
		msg(STATUS "compile_shaders: OUTPUT_DIRECTORY not specified, using ${CS_DIRECTORY}")
		set(CS_OUTPUT_DIRECTORY ${CS_DIRECTORY})
	endif()

	find_package(Vulkan REQUIRED QUIET COMPONENTS glslc)
	if (NOT Vulkan_glslc_FOUND)
		message(FATAL_ERROR "compile_shaders: Vulkan glslc not found")
	endif()
	msg(STATUS "compile_shaders: Found Vulkan glslc at ${Vulkan_GLSLC_EXECUTABLE}")

	set(_extensions ".vert" ".frag" ".geom" ".tesc" ".tese" ".comp")
	list(APPEND _extension ${CS_EXTENSIONS})

	set(_shaders)
	foreach(extension IN LISTS _extensions)
		file(GLOB_RECURSE tmp CONFIGURE_DEPENDS ${CS_DIRECTORY}/*${extension})
		list(APPEND _shaders ${tmp})
	endforeach()

	list(LENGTH _shaders _shaders_count)
	msg(STATUS "compile_shaders: Compiling ${_shaders_count} shaders from ${CS_DIRECTORY} to ${CS_OUTPUT_DIRECTORY}")

	set(_log)
	foreach(shader IN LISTS _shaders)
		set(output_file ${shader}.spv)
		get_filename_component(filename ${shader} NAME)

		if(NOT EXISTS ${output_file} OR ${shader} IS_NEWER_THAN ${output_file})
			# msg(STATUS "compile_shaders: Compiling ${shader} to ${output_file}")
			execute_process(
				COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${shader} -o ${output_file}
				OUTPUT_VARIABLE _output
				RESULT_VARIABLE result
			)
			if(result)
				list(APPEND _log "compile_shaders: ❌ ${filename} -> ${filename}.spv (${result})")
				message(WARNING ${_output})
			else()
				list(APPEND _log "compile_shaders: ✅ ${filename} -> ${filename}.spv")
			endif()
		else()
			list(APPEND _log "compile_shaders: ♻️ ${filename} -> ${filename}.spv (up to date)")
		endif()
	endforeach()

	if (CS_SILENT)
		return()
	endif()

	foreach(line IN LISTS _log)
		message(STATUS ${line})
	endforeach()

endfunction()
