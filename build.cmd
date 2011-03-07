cmake -G "Visual Studio 9 2008" -H%CD% -B%CD%\build\msvc32 -DWINDBG_SDK_DIR=%CD%\sdk
cmake --build %CD%\build\msvc32 --config RelWithDebInfo --target stackdump

cmake -G "Visual Studio 9 2008 Win64" -H%CD% -B%CD%\build\msvc64 -DWINDBG_SDK_DIR=%CD%\sdk
cmake --build %CD%\build\msvc64 --config RelWithDebInfo --target stackdump
