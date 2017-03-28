/*
 *  Debugging and error handling.
 */

/*****************************************************************************\
* Copyright (c) 2017, Elliott Forney, http://www.elliottforney.com            *
* All rights reserved.                                                        *
*                                                                             *
* Redistribution and use in source and binary forms, with or without          *
* modification, are permitted provided that the following conditions are met: *
*                                                                             *
* 1. Redistributions of source code must retain the above copyright notice,   *
*    this list of conditions and the following disclaimer.                    *
*                                                                             *
* 2. Redistributions in binary form must reproduce the above copyright        *
*    notice, this list of conditions and the following disclaimer in the      *
*    documentation and/or other materials provided with the distribution.     *
*                                                                             *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE   *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR         *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        *
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN     *
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)     *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  *
* POSSIBILITY OF SUCH DAMAGE.                                                 *
\*****************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

/* current debug level */
unsigned debug = 1;

/* set debug level */
void debug_set(unsigned level)
{
    debug = level;
}

/* print debug message to stdout if debug > level */
void debug_print(unsigned level, char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (debug >= level) {
        vfprintf(stdout, format, arglist);
        fprintf(stdout, ".\n");
    }

    va_end(arglist);
}

/* print warning message with file and line number to stderr */
void _debug_warn(char *file, unsigned line, char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (debug > 0) {
        fprintf(stderr, "%s:%d: ", file, line);
        vfprintf(stderr, format, arglist);
        fprintf(stderr, ".\n");
    }

    va_end(arglist);
}

/* print error message with file and line number to stderr */
void _debug_warn_errno(char *file, unsigned line, char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (debug > 0) {
        fprintf(stderr, "%s:%d: ", file, line);
        vfprintf(stderr, format, arglist);
        fprintf(stderr, ": %s.\n", strerror(errno));
    }

    va_end(arglist);
}

/* print error message with file and line number to stderr then exit abnormally */
void _debug_fail(char *file, unsigned line, char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (debug > 0) {
        fprintf(stderr, "%s:%d: ", file, line);
        vfprintf(stderr, format, arglist);
        fprintf(stderr, ". Aborting.\n");
    }

    va_end(arglist);
    exit(EXIT_FAILURE);
}

/* print error message and errno message with file and line number to
   stderr then exit abnormally */
void _debug_fail_errno(char *file, unsigned line, char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (debug > 0) {
        fprintf(stderr, "%s:%d: ", file, line);
        vfprintf(stderr, format, arglist);
        fprintf(stderr, ": %s. Aborting.\n", strerror(errno));
    }

    va_end(arglist);
    exit(EXIT_FAILURE);
}
