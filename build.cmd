@echo off

setlocal


if not "%VS90COMNTOOLS%"=="" set generator=Visual Studio 9 2008
if not "%VS100COMNTOOLS%"=="" set generator=Visual Studio 10
if not "%VS110COMNTOOLS%"=="" set generator=Visual Studio 11


cmake -G "%generator%" -H%CD% -B%CD%\build\msvc32 -DWINDBG_SDK_DIR=%CD%\sdk
cmake --build %CD%\build\msvc32 --config RelWithDebInfo --target stackdump

cmake -G "%generator% Win64" -H%CD% -B%CD%\build\msvc64 -DWINDBG_SDK_DIR=%CD%\sdk
cmake --build %CD%\build\msvc64 --config RelWithDebInfo --target stackdump


endlocal
