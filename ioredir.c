/*
 *  Set and restore Input/Output redirection.
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ioredir.h"
#include "debug.h"


/*  Redirect the standard IO stream denoted by k to the file
    descriptor fd and return an ioredir_desc that can be used
    to restore the stream later.  if (fd <= -1), then simply
    store current configuration.

    Args:
        k:  kind of stream to redirect, ioredir_in (input),
            ioredir_out (output), ioredir_err (error).

        fd: file descriptor of new stream.

    Returns:
        ioredir_desc containing the state of the original
        configuration.  This can be restored later using
        ioredir_restore.
*/
ioredir_desc ioredir_set(const ioredir_kind k, const int fd)
{
    ioredir_desc d;
    d.k = k;

    // redirect input stream
    if (k == ioredir_in) {
        // flush the current stream
        fflush(stdin);

        // duplicate stdin and store in d
        if ((d.orig = dup(STDIN_FILENO)) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDIN_FILENO);

        // duplicate fd and set as stdin
        if ((fd > -1) && (dup2(fd, STDIN_FILENO) < 0))
            debug_fail_errno("Failed to duplicate file descriptor %d", STDIN_FILENO);
    }

    // redirect output stream
    else if (k == ioredir_out) {
        // flush the current stream
        fflush(stdout);

        // duplicate stdout and store in d
        if ((d.orig = dup(STDOUT_FILENO)) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDOUT_FILENO);

        // duplicate fd and set as stdout
        if ((fd > -1) && (dup2(fd, STDOUT_FILENO) < 0))
            debug_fail_errno("Failed to duplicate file descriptor %d", STDOUT_FILENO);
    }

    // redirect error stream
    else if (k == ioredir_err) {
        // flush the current stream
        fflush(stderr);

        // duplicate stderr and store in d
        if ((d.orig = dup(STDERR_FILENO)) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDERR_FILENO);

        // duplicate fd and set as stderr
        if ((fd > -1) && (dup2(fd, STDERR_FILENO) < 0))
            debug_fail_errno("Failed to duplicate file descriptor %d", STDERR_FILENO);
    }
    else
        debug_fail("Invalid ioredir_kind %d", k);

    return d;
}

/*  Restore IO stream to configuration stored in d.
    If (fd <= -1), then simply store current configuration.

    Args:
        d:  ioredir_desc containing configuration to resture.
*/
void ioredir_restore(ioredir_desc d)
{
    // if stdin
    if (d.k == ioredir_in) {
        // flush the current stream
        fflush(stdin);

        // duplicate stored fd back to stdin
        if (dup2(d.orig, STDIN_FILENO) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDIN_FILENO);
    }

    // if stdin
    else if (d.k == ioredir_out) {
        // flush the current stream
        fflush(stdout);

        // duplicate stored fd back to stdout
        if (dup2(d.orig, STDOUT_FILENO) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDOUT_FILENO);
    }

    // if stderr
    else if (d.k == ioredir_err) {
        // flush the current stream
        fflush(stderr);

        // duplicate stored fd back to stderr
        if (dup2(d.orig, STDERR_FILENO) < 0)
            debug_fail_errno("Failed to duplicate file descriptor %d", STDERR_FILENO);
    }
    else
        debug_fail("Invalid ioredir_kind %d", d.k);

    // close the stored file descriptor
    close(d.orig);
}
