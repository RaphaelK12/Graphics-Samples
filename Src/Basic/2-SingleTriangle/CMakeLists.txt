#
#  This file is a part of Jiayin's Graphics Samples.
#  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#

file(GLOB_RECURSE project_headers *.h)
file(GLOB_RECURSE project_cpps *.cpp)
file(GLOB_RECURSE project_vs_shader vs.hlsl)
file(GLOB_RECURSE project_ps_shader ps.hlsl)
set(project_shaders ${project_vs_shader} ${project_ps_shader})

set(all_files ${project_headers} ${project_cpps} ${project_shaders})
source_group_by_dir(all_files)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}/Bin")
set(CMAKE_RUNTIME_SHADER_FOLDER "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources/Shaders")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_ROOT_DIR}/Bin/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_ROOT_DIR}/Bin/Debug")

macro(RemoveDebugCXXFlag flag)
    string(REPLACE "${flag}" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endmacro()

if(PLATFORM_WIN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS-")

    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

    # Use unicode as default character set
    ADD_DEFINITIONS(-DUNICODE)

    # Somehow this will introduce an unresolved symbol error in debug mode.
    RemoveDebugCXXFlag("/RTC1")
endif()

add_executable(SingleTriangle ${all_files})

target_link_libraries( SingleTriangle "d3d12.lib" "dxgi.lib" "dxguid.lib" "d3dcompiler.lib")

# setup correct output name for different configurations
set_target_properties( SingleTriangle PROPERTIES RELEASE_OUTPUT_NAME "1_single_triangle_r" )
set_target_properties( SingleTriangle PROPERTIES DEBUG_OUTPUT_NAME "1_single_triangle_d" )

# setup working directory
set_property(TARGET SingleTriangle PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)")

# setup shader compiling steps
set_property(SOURCE ${project_shaders}      PROPERTY VS_SHADER_ENTRYPOINT   main)
set_property(SOURCE ${project_shaders}      PROPERTY VS_SHADER_MODEL        5.1)
set_property(SOURCE ${project_vs_shader}    PROPERTY VS_SHADER_TYPE         Vertex)
set_property(SOURCE ${project_ps_shader}    PROPERTY VS_SHADER_TYPE         Pixel)
set_property(SOURCE ${project_vs_shader}    PROPERTY VS_SHADER_OBJECT_FILE_NAME     "${CMAKE_RUNTIME_SHADER_FOLDER}/vso_1_single_triangle.cso")
set_property(SOURCE ${project_ps_shader}    PROPERTY VS_SHADER_OBJECT_FILE_NAME     "${CMAKE_RUNTIME_SHADER_FOLDER}/pso_1_single_triangle.cso")

# setup project folder
set_target_properties( SingleTriangle PROPERTIES FOLDER BasicSamples)