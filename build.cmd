cmake -G "Visual Studio 9 2008" -H%CD% -B%CD%\build -DWINDBG_SDK_DIR=%CD%\sdk
cmake --build build --config RelWithDebInfo --target stackdump
