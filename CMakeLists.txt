#
#  This file is a part of Jiayin's Graphics Samples.
#  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#

cmake_minimum_required (VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# this will avoid generate ZERO_CHECK project
# set(CMAKE_SUPPRESS_REGENERATION true)

# define platform
if (WIN32)
    set(PLATFORM_WIN true)
    set(PLATFORM_MAC false)
    set(PLATFORM_LINUX false)
    set(PLATFORM_NAME "Windows" )
elseif(APPLE)
    set(PLATFORM_WIN false)
    set(PLATFORM_MAC true)
    set(PLATFORM_LINUX false)
    set(PLATFORM_NAME "Mac OS" )
elseif(UNIX)
    set(PLATFORM_WIN false)
    set(PLATFORM_MAC false)
    set(PLATFORM_LINUX true)
    set(PLATFORM_NAME "Linux" )
endif()

# hide the options that I never use
MARK_AS_ADVANCED(CMAKE_INSTALL_PREFIX)
if(PLATFORM_MAC)
    MARK_AS_ADVANCED(CMAKE_OSX_ARCHITECTURES)
    MARK_AS_ADVANCED(CMAKE_OSX_DEPLOYMENT_TARGET)
    MARK_AS_ADVANCED(CMAKE_OSX_SYSROOT)
    MARK_AS_ADVANCED(CMAKE_EXECUTABLE_FORMAT)
endif()

# default build type is release
set( DEFUALT_BULID_TYPE "Release")

# Options to choose configuration type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFUALT_BULID_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFUALT_BULID_TYPE}" CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set( CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "" FORCE )
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release" "Debug")
endif()

# A helper macro to be shared by other CMakes
macro(source_group_by_dir source_files)
    set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
    foreach(sgbd_file ${${source_files}})
        string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
        string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
        string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
        string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
        if(sgbd_nogroup)
            set(sgbd_group_name "\\")
        endif(sgbd_nogroup)
        source_group(${sgbd_group_name} FILES ${sgbd_file})
    endforeach(sgbd_file)
endmacro(source_group_by_dir)

# a helper macro to keep track of the root dir
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# find the executable to generate shader header files
find_program(GLSLANG_VALIDATOR NAMES glslangValidator HINTS ${PROJECT_ROOT_DIR}/Dependencies )
# find the python script to generate spirv headers
find_program(SPIRV_GENERATE_SCRIPT NAMES generate_spirv.py HINTS ${PROJECT_ROOT_DIR}/Scripts )

# fine python
find_package(Python REQUIRED)

# enable groupping projects in folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Graphics samples project
project (GraphicsSamples)

if(PLATFORM_WIN)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_WIN=1")
elseif(PLATFORM_MAC)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_MAC=1")
elseif(PLATFORM_LINUX)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPLATFORM_LINUX=1")
endif()

# the basic samples
add_subdirectory(Src/Basic)