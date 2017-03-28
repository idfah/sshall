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

#ifndef ioredir_h
    #define ioredir_h

    /* denotes whether we are redircting the
       standard input, output or error streams */
    typedef enum {
        ioredir_in,
        ioredir_out,
        ioredir_err,
    } ioredir_kind;

    /* stores a file descriptor and stream type
       for restoring an IO stream */
    typedef struct {
        int orig;
        ioredir_kind k;
    } ioredir_desc;

    /* shorthand for setting input, output and error streams */
    #define ioredir_set_in(fd)  ioredir_set(ioredir_in, fd)
    #define ioredir_set_out(fd) ioredir_set(ioredir_out, fd)
    #define ioredir_set_err(fd) ioredir_set(ioredir_err, fd)

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
    ioredir_desc ioredir_set(const ioredir_kind k, const int fd);

    /*  Restore IO stream to configuration stored in d.
        If (fd <= -1), then simply store current configuration.

        Args:
            d:  ioredir_desc containing configuration to resture.
    */
    void ioredir_restore(ioredir_desc d);

#endif
