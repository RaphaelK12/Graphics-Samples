::
::    This file is a part of Jiayin's Graphics Samples.
::    Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
::

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
    )else (
        echo Unrecognized Command
        goto EOF
    )
)else if "%1" == "" (
    set BUILD_RELEASE=1
    goto EOF
)

:EOF
exit /b 0
:ERR
exit /b 1

