# - This module looks for Microsoft Debugging Tools for Windows SDK
# It defines:
#   WINDBG_SDK_DIR               : full path to SDK root dir
#   WINDBG_SDK_INCLUDE_PATH      : include path to the API (dbgeng.h)
#   WINDBG_SDK_DBGENG_LIBRARY    : full path to the library (dbgeng.lib)
#   WINDBG_SDK_DBGHELP_LIBRARY   : full path to the library (dbghelp.lib)
#

if(WIN32)

  find_path(WINDBG_SDK_DIR 
    inc/dbgeng.h
    paths
      "$ENV{PROGRAMFILES}/Debugging Tools for Windows/sdk"
      "$ENV{PROGRAMFILES}/Debugging Tools for Windows (x86)/sdk"
      "$ENV{PROGRAMFILES}/Debugging Tools for Windows (x64)/sdk"
    doc "Microsoft Debugging Tools for Windows SDK directory")

  find_path(WINDBG_SDK_INCLUDE_PATH 
    dbgeng.h 
    paths "${WINDBG_SDK_DIR}/inc"
    )

  if(CMAKE_CL_64)
    set(WINDBG_SDK_LIBRARY_PATHS 
      "${WINDBG_SDK_DIR}/lib/amd64")
  else(CMAKE_CL_64)
    set(WINDBG_SDK_LIBRARY_PATHS 
      "${WINDBG_SDK_DIR}/lib/i386"
      "${WINDBG_SDK_DIR}/lib")
  endif(CMAKE_CL_64)

  find_library(WINDBG_SDK_DBGENG_LIBRARY 
    dbgeng
    ${WINDBG_SDK_LIBRARY_PATHS})

  find_library(WINDBG_SDK_DBGHELP_LIBRARY 
    dbghelp
    ${WINDBG_SDK_LIBRARY_PATHS})

  mark_as_advanced(
    WINDBG_SDK_INCLUDE_PATH
    WINDBG_SDK_DBGENG_LIBRARY
    WINDBG_SDK_DBGHELP_LIBRARY
    )

endif(WIN32)

if (WINDBG_SDK_DIR)
  set( WINDBG_SDK_FOUND 1)
else (WINDBG_SDK_DIR)
  set( WINDBG_SDK_FOUND 0)
endif (WINDBG_SDK_DIR)
mark_as_advanced( WINDBG_SDK_FOUND )

# vim:set sw=2 et:
