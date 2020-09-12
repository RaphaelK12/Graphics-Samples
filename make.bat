::
::  This file is a part of Jiayin's Graphics Samples.
::  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
::

@echo off
set PROJECT_ROOT_DIR=%~dp0

rem reset all variables first
call "%PROJECT_ROOT_DIR%\BuildFiles\Win\reset_arguments.cmd"

rem parse arguments
call "%PROJECT_ROOT_DIR%\BuildFiles\Win\parse_arguments.cmd" %*
if errorlevel 1 goto EOF

if "%BUILD_RELEASE%" == "1" (
    echo [33mBuilding release[0m

    :: create the resource folder since fxc will fail to write files in that folder
    powershell New-Item -Force -ItemType directory -Path Bin/Resources

    powershell New-Item -Force -ItemType directory -Path Temp/Release
	cd Temp/Release
	cmake -A x64 ../..
	msbuild /p:Configuration=Release GraphicsSamples.sln

	:: catch msbuild error
	if ERRORLEVEL 1 ( 
		goto BUILD_ERR
	)

	cd ../..
)

if "%BUILD_DEBUG%" == "1" (
    echo [33mBuilding debug[0m

    :: create the resource folder since fxc will fail to write files in that folder
    powershell New-Item -Force -ItemType directory -Path Bin/Resources

    powershell New-Item -Force -ItemType directory -Path Temp/Debug
	cd Temp/Debug
	cmake -A x64 ../..
	msbuild /p:Configuration=Debug GraphicsSamples.sln

	:: catch msbuild error
	if ERRORLEVEL 1 ( 
		goto BUILD_ERR
	)
	
	cd ../..
)

if "%CLEAN%" == "1" (
    echo [33mCleaning all temporary file[0m
	powershell Remove-Item -path ./Bin -recurse -ErrorAction Ignore
	powershell Remove-Item -path ./Temp -recurse -ErrorAction Ignore
)

if "%UPDATE%" == "1" (
	echo [33mSycning latest code[0m
	git pull
)

if "%UPDATE_DEP%" == "1" (
	echo [33mDownloading dependencies[0m
	powershell .\BuildFiles\win\getdep.ps1
)

:BUILD_SUCCEEED
exit /b 0
:BUILD_ERR
exit /b 1