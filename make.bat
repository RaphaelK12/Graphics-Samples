::
::  This file is a part of Jiayin's Graphics Samples.
::  Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
::

@echo off
set PROJECT_ROOT_DIR=%~dp0

rem reset all variables first
set BUILD_RELEASE=
set BUILD_HYBRID=
set BUILD_DEBUG=
set CLEAN=
set UPDATE=
set UPDATE_DEP=
set FORCE_UPDATE_DEP=

rem parse arguments
:argv_loop
if NOT "%1" == "" (
    if "%1" == "clean" (
        set CLEAN=1
        goto EOF
    )else if "%1" == "update" (
        set UPDATE=1
        goto EOF
    )else if "%1" == "release" (
        set BUILD_RELEASE=1
        goto EOF
    )else if "%1" == "debug" (
        set BUILD_DEBUG=1
        goto EOF
    )else if "%1" == "update_dep" (
        set UPDATE_DEP=1
        goto EOF
    )else if "%1" == "force_update_dep" (
        set FORCE_UPDATE_DEP=1
        goto EOF
    )else (
        echo Unrecognized Command
        goto EOF
    )
)else if "%1" == "" (
    set BUILD_RELEASE=1
    goto EOF
)

:EOF

if "%BUILD_RELEASE%" == "1" (
    echo [33mBuilding release[0m

    :: sync dependencies if needed
    py ./Scripts/get_dependencies.py

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

    :: sync dependencies if needed
    py ./Scripts/get_dependencies.py

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

	py ./Scripts/get_dependencies.py

    goto BUILD_SUCCEEED
)

if "%FORCE_UPDATE_DEP%" == "1" (
    echo [33mDownloading dependencies[0m

	py ./Scripts/get_dependencies.py TRUE

    goto BUILD_SUCCEEED
)

:BUILD_SUCCEEED
exit /b 0
:BUILD_ERR
exit /b 1