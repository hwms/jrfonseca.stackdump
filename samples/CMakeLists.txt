cmake_minimum_required (VERSION 2.6) 

project (SAMPLES) 

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

add_executable (abort abort.c) 
add_executable (assert assert.c) 
add_executable (complex complex.c) 
add_executable (false false.c) 
add_executable (is_debugger_present is_debugger_present.c) 
add_executable (output_debug_string output_debug_string.c) 
add_executable (true true.c) 
