/**************************************************************************
 *
 * Copyright 2009-2010 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OF OR CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/

/*
 * Simple program to dump a stack trace and produce a minidump file when
 * a child process.
 *
 * Based on many of the ideas used in the Debugging Tools for Windows SDK's
 * samples, in particular:
 * - healer
 * - dumpstk
 * - assert
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <dbgeng.h>

/**************************************************************************
 *
 * Defines
 *
 **************************************************************************/

#ifndef STATUS_FATAL_APP_EXIT
#define STATUS_FATAL_APP_EXIT 0x40000015UL
#endif

/**************************************************************************
 *
 * Globals
 *
 **************************************************************************/

static BOOL g_Verbose = FALSE;
static ULONG g_OutputMask = DEBUG_OUTPUT_DEBUGGEE;
static PCSTR g_SymbolPath = NULL;
static PCSTR g_DumpPath = NULL;
static char g_CommandLine[4096];

static IDebugClient* g_Client = NULL;
static IDebugControl* g_Control = NULL;
static IDebugSymbols* g_Symbols = NULL;

/**************************************************************************
 *
 * Utility
 *
 **************************************************************************/

static void
Cleanup(void)
{
   if (g_Control) {
      g_Control->Release();
   }

   if (g_Symbols) {
      g_Symbols->Release();
   }

   if (g_Client) {
      g_Client->EndSession(DEBUG_END_PASSIVE);
      g_Client->Release();
   }
}

static void
Abort(void)
{
   Cleanup();
   
   exit(1);
}

static HRESULT
AddBreakpoint(PCSTR expression)
{
   IDebugBreakpoint* Bp;
   HRESULT status;
   
   if (g_Verbose) {
      fprintf(stderr, "Adding breakpoing %s\n", expression);
   }

   status = g_Control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &Bp);
   if (status != S_OK) {
      fprintf(stderr, "warning: failed to add breakpoint (0x%08x)\n", status);
      return status;
   }

   status = Bp->SetOffsetExpression(expression);
   if (status != S_OK) {
      fprintf(stderr, "warning: failed to set breakpoint expression %s (0x%08x)\n", expression, status);
      return status;
   }
   
   status = Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);
   if (status != S_OK) {
      fprintf(stderr, "warning: failed to enable breakpoint %s (0x%08x)\n", expression, status);
      return status;
   }

   return S_OK;
}

static HRESULT
AddWildcardBreakpoint(PCSTR ModuleName, PCSTR SymbolName)
{
   char expression[1024];
   ULONG64  Offset;
   HRESULT status;

   _snprintf(expression, sizeof expression, "%s!%s", ModuleName, SymbolName);

   status = g_Symbols->GetOffsetByName(expression, &Offset);
   if (status != S_OK) {
      return status;
   }

   return AddBreakpoint(expression);
}

static void
DumpStack(void)
{
   HRESULT status;

   g_OutputMask = ~0;

   status = g_Control->OutputCurrentState(DEBUG_OUTCTL_ALL_CLIENTS, 
                                          DEBUG_CURRENT_SYMBOL |
                                          DEBUG_CURRENT_DISASM |
                                          DEBUG_CURRENT_REGISTERS |
                                          DEBUG_CURRENT_SOURCE_LINE);
   if (status != S_OK) {
      fprintf(stderr, "warning: failed to output current state (0x%08x)\n", status);
   }

   status = g_Control->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS, 
                                        NULL,
                                        50, 
                                        DEBUG_STACK_COLUMN_NAMES |
                                        DEBUG_STACK_FRAME_NUMBERS |
                                        DEBUG_STACK_FRAME_ADDRESSES |
                                        DEBUG_STACK_SOURCE_LINE |
                                        DEBUG_STACK_PARAMETERS);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to output a stack trace (0x%08x)\n", status);
   }

   if (g_DumpPath) {
      status = g_Client->WriteDumpFile(g_DumpPath, DEBUG_DUMP_SMALL);
      if (status != S_OK) {
         fprintf(stderr, "warning: failed to create dump file (0x%08x)\n", status);
      }
      if (g_Verbose) {
         fprintf(stderr, "%s created\n", g_DumpPath);
      }
   }
}

/**************************************************************************
 *
 * Output callbacks
 *
 **************************************************************************/

class StdioOutputCallbacks : public IDebugOutputCallbacks
{
public:
   /* IUnknown */
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID InterfaceId, PVOID* Interface);
   ULONG STDMETHODCALLTYPE AddRef(); 
   ULONG STDMETHODCALLTYPE Release();

   /* IDebugOutputCallbacks */
   HRESULT STDMETHODCALLTYPE Output(ULONG Mask, PCSTR Text);
};

HRESULT STDMETHODCALLTYPE
StdioOutputCallbacks::QueryInterface(REFIID InterfaceId, PVOID* Interface)
{
   *Interface = NULL;

   if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
       IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks))) {
      *Interface = this;
      return S_OK;
   } else {
      return E_NOINTERFACE;
   }
}

ULONG STDMETHODCALLTYPE
StdioOutputCallbacks::AddRef()
{
   return 1;
}

ULONG STDMETHODCALLTYPE
StdioOutputCallbacks::Release()
{
   return 0;
}

HRESULT STDMETHODCALLTYPE
StdioOutputCallbacks::Output(ULONG Mask, PCSTR Text)
{
   if(Mask & g_OutputMask)
      fputs(Text, stderr);
   return S_OK;
}

static StdioOutputCallbacks g_OutputCb;

/**************************************************************************
 *
 * Event callbacks
 *
 **************************************************************************/

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
   /* IUnknown */
   ULONG STDMETHODCALLTYPE AddRef();
   ULONG STDMETHODCALLTYPE Release();

   /* IDebugEventCallbacks */
   HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG Mask);
   HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT Bp);
   HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 Exception, ULONG FirstChance);
   HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64 ImageFileHandle, ULONG64 Handle, 
                                           ULONG64 BaseOffset, ULONG ModuleSize, 
                                           PCSTR ModuleName, PCSTR ImageName, 
                                           ULONG CheckSum, ULONG TimeDateStamp, 
                                           ULONG64 InitialThreadHandle, 
                                           ULONG64 ThreadDataOffset, ULONG64 StartOffset);
   HRESULT STDMETHODCALLTYPE LoadModule(ULONG64 ImageFileHandle, ULONG64 BaseOffset, 
                                        ULONG ModuleSize, PCSTR ModuleName, 
                                        PCSTR ImageName, ULONG CheckSum, 
                                        ULONG TimeDateStamp);
};

ULONG STDMETHODCALLTYPE
EventCallbacks::AddRef()
{
   return 1;
}

ULONG STDMETHODCALLTYPE 
EventCallbacks::Release()
{
   return 0;
}

HRESULT STDMETHODCALLTYPE
EventCallbacks::GetInterestMask(PULONG Mask)
{
   *Mask = DEBUG_EVENT_BREAKPOINT |
           DEBUG_EVENT_EXCEPTION |
           DEBUG_EVENT_CREATE_PROCESS |
           DEBUG_EVENT_LOAD_MODULE;
   return S_OK;
}

HRESULT STDMETHODCALLTYPE
EventCallbacks::Breakpoint(PDEBUG_BREAKPOINT Bp)
{
   DumpStack();
   Abort();
   
   return DEBUG_STATUS_GO;
}

HRESULT STDMETHODCALLTYPE
EventCallbacks::Exception(PEXCEPTION_RECORD64 Exception, ULONG FirstChance)
{
   if (g_Verbose) {
      fprintf(stderr, "uncaught exception - code %08lx (%s chance)\n", 
              Exception->ExceptionCode, FirstChance ? "first" : "second");
   }

   /*
    * Ignore first chance of non fatal or unknown exceptions to allow an
    * exception handler in the debugee to handled them as usual.
    */
   if(FirstChance) {
      switch(Exception->ExceptionCode) {

      case EXCEPTION_ACCESS_VIOLATION:
      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      case EXCEPTION_BREAKPOINT:
      case EXCEPTION_DATATYPE_MISALIGNMENT:
      case EXCEPTION_FLT_DENORMAL_OPERAND:
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      case EXCEPTION_FLT_INEXACT_RESULT:
      case EXCEPTION_FLT_INVALID_OPERATION:
      case EXCEPTION_FLT_OVERFLOW:
      case EXCEPTION_FLT_STACK_CHECK:
      case EXCEPTION_FLT_UNDERFLOW:
      case EXCEPTION_GUARD_PAGE:
      case EXCEPTION_IN_PAGE_ERROR:
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
      case EXCEPTION_INT_OVERFLOW:
      case EXCEPTION_INVALID_DISPOSITION:
      case EXCEPTION_INVALID_HANDLE:
      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      case EXCEPTION_PRIV_INSTRUCTION:
      case EXCEPTION_SINGLE_STEP:
      case EXCEPTION_STACK_OVERFLOW:
      case STATUS_FATAL_APP_EXIT:
         /* Raised in MSVCRT's abort() */
         break;

      case EXCEPTION_ILLEGAL_INSTRUCTION:
         /* Often used by applications to detect CPU capabilities (e.g. CPUID,
          * MMX, etc) */
      case DBG_CONTROL_C:
      default:
         return DEBUG_STATUS_NO_CHANGE;
      }
   }
   
   if (!g_Verbose) {
      fprintf(stderr, "uncaught exception - code %08lx (%s chance)\n", 
              Exception->ExceptionCode, FirstChance ? "first" : "second");
   }

   DumpStack();
   Abort();
   
   return DEBUG_STATUS_NO_CHANGE;
}

HRESULT STDMETHODCALLTYPE
EventCallbacks::CreateProcess(ULONG64 ImageFileHandle,
                              ULONG64 Handle,
                              ULONG64 BaseOffset,
                              ULONG ModuleSize,
                              PCSTR ModuleName,
                              PCSTR ImageName,
                              ULONG CheckSum,
                              ULONG TimeDateStamp,
                              ULONG64 InitialThreadHandle,
                              ULONG64 ThreadDataOffset,
                              ULONG64 StartOffset)
{
   UNREFERENCED_PARAMETER(ImageFileHandle);
   UNREFERENCED_PARAMETER(Handle);
   UNREFERENCED_PARAMETER(BaseOffset);
   UNREFERENCED_PARAMETER(ModuleSize);
   UNREFERENCED_PARAMETER(ModuleName);
   UNREFERENCED_PARAMETER(BaseOffset);
   UNREFERENCED_PARAMETER(CheckSum);
   UNREFERENCED_PARAMETER(TimeDateStamp);
   UNREFERENCED_PARAMETER(InitialThreadHandle);
   UNREFERENCED_PARAMETER(ThreadDataOffset);
   UNREFERENCED_PARAMETER(StartOffset);
   
   AddBreakpoint("user32!MessageBoxA");
   AddBreakpoint("user32!MessageBoxW");

   return DEBUG_STATUS_GO;
}

HRESULT STDMETHODCALLTYPE
EventCallbacks::LoadModule(ULONG64 ImageFileHandle,
                           ULONG64 BaseOffset,
                           ULONG ModuleSize,
                           PCSTR ModuleName,
                           PCSTR ImageName,
                           ULONG CheckSum,
                           ULONG TimeDateStamp)
{
   UNREFERENCED_PARAMETER(ImageFileHandle);
   UNREFERENCED_PARAMETER(BaseOffset);
   UNREFERENCED_PARAMETER(ModuleSize);
   UNREFERENCED_PARAMETER(ModuleName);
   UNREFERENCED_PARAMETER(ImageName);
   UNREFERENCED_PARAMETER(CheckSum);
   UNREFERENCED_PARAMETER(TimeDateStamp);

   AddWildcardBreakpoint(ModuleName, "_wassert");
   AddWildcardBreakpoint(ModuleName, "_assert");
   AddWildcardBreakpoint(ModuleName, "abort");

   return DEBUG_STATUS_GO;
}

static EventCallbacks g_EventCb;

/**************************************************************************
 *
 * Main function
 *
 **************************************************************************/

static void
Usage()
{
   fputs("usage: stackdump [options] <command-line>\n"
         "\n"
         "options:\n"
         "  -? displays command line help text\n"
         "  -v enables verbose output from the debugger\n"
         "  -y <symbols-path> specifies the symbol search path (same as _NT_SYMBOL_PATH)\n"
         "  -z <crash-dump-file> specifies the name of a crash dump file to create\n",
         stderr);
}

int
main(int argc, char** argv)
{
   HRESULT status;
   
   /*
    * Parse command line arguments
    */

   while (--argc > 0) {
      ++argv;

      if (!strcmp(*argv, "-?")) {
         Usage();
         return 0;
      } else if (!strcmp(*argv, "-v")) {
         g_Verbose = TRUE;
      } else if (!strcmp(*argv, "-y")) {
         if (argc < 2) {
            fprintf(stderr, "error: -y missing argument\n\n");
            Usage();
            return 1;
         }

         ++argv;
         --argc;

         g_SymbolPath = *argv;
      } else if (!strcmp(*argv, "-z")) {
         if (argc < 2) {
            fprintf(stderr, "error: -z missing argument\n\n");
            Usage();
            return 1;
         }

         ++argv;
         --argc;

         g_DumpPath = *argv;
      } else {
         break;
      }
   }
   
   /*
    * Concatenate remaining arguments into a command line
    */
   
   PSTR CommandLine = g_CommandLine;
   
   while (argc > 0) {
      ULONG Len;
      
      Len = (ULONG)strlen(*argv);
      if (Len + 3 + (CommandLine - g_CommandLine) >= sizeof(g_CommandLine)) {
         fprintf(stderr, "error: command line length exceeds %u characters\n", sizeof(g_CommandLine));
         return 1;
      }

      *CommandLine++ = '"';

      memcpy(CommandLine, *argv, Len + 1);
      CommandLine += Len;

      *CommandLine++ = '"';
      *CommandLine++ = ' ';
      
      ++argv;
      --argc;
   }

   *CommandLine = 0;

   if (strlen(g_CommandLine) == 0) {
      fprintf(stderr, "error: no command line given\n\n");
      Usage();
      return 1;
   }

   /*
    * Create interfaces
    */

   status = DebugCreate(__uuidof(IDebugClient),
                        (void**)&g_Client);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to start debugging engine (0x%08x)\n", status);
      Abort();
   }

   status = g_Client->QueryInterface(__uuidof(IDebugControl),
                                     (void**)&g_Control);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to start debugging engine (0x%08x)\n", status);
      Abort();
   }

   status = g_Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void**)&g_Symbols);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to start debugging engine (0x%08x)\n", status);
      Abort();
   }

   /*
    * Apply command line arguments
    */

   g_Control->SetEngineOptions(DEBUG_ENGOPT_ALLOW_NETWORK_PATHS);
   g_Control->SetCodeLevel(DEBUG_LEVEL_SOURCE);

   status = g_Client->SetOutputCallbacks(&g_OutputCb);
   if (status != S_OK) {
      fprintf(stderr, "warning: failed to redirect debugger output (0x%08x)\n", status);
   }

   if (g_Verbose)
      g_OutputMask = ~0;

   if (g_SymbolPath != NULL) {
      status = g_Symbols->SetSymbolPath(g_SymbolPath);
      if (status != S_OK) {
         fprintf(stderr, "warning: failed to set symbol path (0x%08x)\n", status);
      }
   }

   status = g_Client->SetEventCallbacks(&g_EventCb);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to capture debug events (0x%08x)\n", status);
      Abort();
   }
   
   status = g_Client->CreateProcess(0, g_CommandLine, DEBUG_ONLY_THIS_PROCESS);
   if (status != S_OK) {
      fprintf(stderr, "error: failed to create the process (0x%08x)\n", status);
      Abort();
   }

   /*
    * Main event loop.
    */

   for (;;) {
      status = g_Control->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
      if (status != S_OK) {
         ULONG ExecStatus;
         
         if (g_Control->GetExecutionStatus(&ExecStatus) == S_OK &&
             ExecStatus == DEBUG_STATUS_NO_DEBUGGEE) {
            break;
         }

         fprintf(stderr, "error: unexpected error (0x%0x)\n", status);
         Abort();
      }

      fprintf(stderr, "warning: ignoring unexpected event\n");
      status = g_Control->SetExecutionStatus(DEBUG_STATUS_GO_HANDLED);
      if (status != S_OK) {
         fprintf(stderr, "error: failed to proceed (0x%0x)\n", status);
         Abort();
      }
   }

   Cleanup();

   return 0;
}

/* vim:set sw=3 et: */
