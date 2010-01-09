/**************************************************************************
 *
 * Copyright 2009 Jose Fonseca
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

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <crtdbg.h>
#include <stdio.h>

static void 
YetAnotherFunction(int i)
{
   int k;
   
   assert(2+2==5);
   
#if 0
   /* Other ways to cause a GPF */
   *(int *)i = 5;  
   __asm ("int $3");
   (*((void (*)(void)) 0x12345678))();
#endif

   sscanf("12345", "%i", (int *) (k=i));
}

struct AStructType
{
   int AnArray[2];
};

enum AnEnumType { a, b, c };

static void 
MyWonderfulFunction(int AnInteger, 
                    double ADouble, 
                    int AnArray[4], 
                    char * AString, 
                    enum AnEnumType AnEnum, 
                    struct AStructType AStruct, 
                    void (*AFunction)(void))
{
   YetAnotherFunction( 8 );
}

static void 
ASimpleFunction(void) 
{
}

int 
main(int argc, char *argv[])
{
   struct AStructType AStruct = {{10, 3}};
   int AnArray[4] = {4, 3, 2, 1};
   
#if 0
   // http://msdn.microsoft.com/en-us/library/sas1dkb2.aspx
   _set_error_mode(_OUT_TO_STDERR);
   // http://msdn.microsoft.com/en-us/library/e631wekh.aspx
   //_set_abort_behavior( 0, _WRITE_ABORT_MSG);
   
   //http://msdn.microsoft.com/en-us/library/1y71x448.aspx
   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
#endif
   
   MyWonderfulFunction( 4, 5.6, AnArray, "Hello" , a, AStruct, ASimpleFunction);

   return 0;
}

/* vim:set sw=3 et: */
