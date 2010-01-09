cmake_minimum_required (VERSION 2.6) 

project (STACKDUMP) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${STACKDUMP_SOURCE_DIR}")

find_package(WinDbg REQUIRED)

# http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F
foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

add_definitions( -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS )
add_definitions( -D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS )

include_directories (${WINDBG_SDK_INCLUDE_PATH}) 

add_executable (stackdump stackdump.cpp) 

target_link_libraries (stackdump "${WINDBG_SDK_DBGENG_LIBRARY}")
