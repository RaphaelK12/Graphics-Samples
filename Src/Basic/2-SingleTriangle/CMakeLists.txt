#
#  This file is a part of Jiayin's Graphics Samples.
#  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#

file(GLOB_RECURSE project_headers *.h)
file(GLOB_RECURSE project_cpps *.cpp)
file(GLOB_RECURSE project_hlsl_vs_shader vs.hlsl)
file(GLOB_RECURSE project_hlsl_ps_shader ps.hlsl)
set(project_hlsl_shaders ${project_hlsl_vs_shader} ${project_hlsl_ps_shader})
file(GLOB_RECURSE project_glsl_vs_shader vs.vert.glsl)
file(GLOB_RECURSE project_glsl_ps_shader ps.frag.glsl)
set(project_glsl_shaders ${project_glsl_vs_shader} ${project_glsl_ps_shader})

set(generated_hlsl_headers ${CMAKE_CURRENT_SOURCE_DIR}/d3d12/shaders/generated_vs.h ${CMAKE_CURRENT_SOURCE_DIR}/d3d12/shaders/generated_ps.h)
set(generate_spirv_headers ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_vs.h ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_ps.h)

# The header files won't be generated until compiling, this is just a workaround to indicate CMake that these files will be generated.
# Ideally, if there is a way to locate fxc, I can also generate the header file here, which is a lot better.
add_custom_command( OUTPUT ${generated_hlsl_headers}
                    COMMAND call >> generated_vs.h | call >> generated_ps.h
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/d3d12/shaders/)

# I will find time to clean this later
add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_vs.h
                    COMMAND ${Python_EXECUTABLE} ${SPIRV_GENERATE_SCRIPT} ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/vs.vert.glsl ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_vs.h ${GLSLANG_VALIDATOR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    DEPENDS ${PROJECT_ROOT_DIR}/Scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/vs.vert.glsl ${GLSLANG_VALIDATOR})

add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_ps.h
                    COMMAND ${Python_EXECUTABLE} ${SPIRV_GENERATE_SCRIPT} ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/ps.frag.glsl ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/generated_ps.h ${GLSLANG_VALIDATOR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    DEPENDS ${PROJECT_ROOT_DIR}/Scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/ps.frag.glsl ${GLSLANG_VALIDATOR})

set(all_files ${project_headers} ${project_cpps} ${project_hlsl_shaders} ${project_glsl_shaders} ${generated_hlsl_headers} ${generate_spirv_headers})
source_group_by_dir(all_files)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}/Bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_ROOT_DIR}/Bin/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_ROOT_DIR}/Bin/Debug")

# include directories
include_directories( "${VULKAN_SDK_DIR}/Include" )

# library folder to link from
link_directories( "${VULKAN_SDK_DIR}/Lib" )

macro(RemoveDebugCXXFlag flag)
    string(REPLACE "${flag}" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endmacro()

if(PLATFORM_WIN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS-")

    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

    # Use unicode as default character set
    ADD_DEFINITIONS(-DUNICODE -DVK_USE_PLATFORM_WIN32_KHR)

    # Somehow this will introduce an unresolved symbol error in debug mode.
    RemoveDebugCXXFlag("/RTC1")
endif()

add_executable(SingleTriangle ${all_files})

target_link_libraries( SingleTriangle "d3d12.lib" "dxgi.lib" "dxguid.lib" "d3dcompiler.lib" "vulkan-1.lib")

# setup correct output name for different configurations
set_target_properties( SingleTriangle PROPERTIES RELEASE_OUTPUT_NAME "2_single_triangle_r" )
set_target_properties( SingleTriangle PROPERTIES DEBUG_OUTPUT_NAME "2_single_triangle_d" )

# setup working directory
set_property(TARGET SingleTriangle PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)")

# setup shader compiling steps
set_property(SOURCE ${project_hlsl_shaders}         PROPERTY VS_SHADER_ENTRYPOINT   main)
set_property(SOURCE ${project_hlsl_shaders}         PROPERTY VS_SHADER_MODEL        5.1)
set_property(SOURCE ${project_hlsl_vs_shader}       PROPERTY VS_SHADER_TYPE         Vertex)
set_property(SOURCE ${project_hlsl_ps_shader}       PROPERTY VS_SHADER_TYPE         Pixel)
set_property(SOURCE ${project_hlsl_vs_shader}       PROPERTY VS_SHADER_OUTPUT_HEADER_FILE     "${CMAKE_CURRENT_SOURCE_DIR}/d3d12/shaders/generated_vs.h")
set_property(SOURCE ${project_hlsl_vs_shader}       PROPERTY VS_SHADER_VARIABLE_NAME          "g_shader_vs")
set_property(SOURCE ${project_hlsl_ps_shader}       PROPERTY VS_SHADER_OUTPUT_HEADER_FILE     "${CMAKE_CURRENT_SOURCE_DIR}/d3d12/shaders/generated_ps.h")
set_property(SOURCE ${project_hlsl_ps_shader}       PROPERTY VS_SHADER_VARIABLE_NAME          "g_shader_ps")

# setup project folder
set_target_properties( SingleTriangle PROPERTIES FOLDER BasicSamples)