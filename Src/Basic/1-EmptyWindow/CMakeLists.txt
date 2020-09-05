#
#  This file is a part of Jiayin's Graphics Samples.
#  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#

file(GLOB_RECURSE project_headers *.h)
file(GLOB_RECURSE project_cpps *.cpp)
file(GLOB_RECURSE project_cs *.c)

set(all_files ${project_headers} ${project_cpps} ${project_cs} ${project_ccs} ${project_headers})
source_group_by_dir(all_files)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}/Bin")
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

add_executable(EmptyWindow ${all_files})

target_link_libraries( EmptyWindow "d3d12.lib" "dxgi.lib" "dxguid.lib" )

# setup correct output name for different configurations
set_target_properties( EmptyWindow PROPERTIES RELEASE_OUTPUT_NAME "1_empty_window_r" )
set_target_properties( EmptyWindow PROPERTIES DEBUG_OUTPUT_NAME "1_empty_window_d" )

# setup project folder
set_target_properties (EmptyWindow PROPERTIES FOLDER BasicSamples)