/*
 *  Simple but flexible tool for executing
 *  remote commands across multiple hosts
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

// requires gnu compatibility
#define _GNU_SOURCE

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "colorset.h"
#include "ioredir.h"

#ifdef RSH
    #define rcmd "rsh"
    #define cmd_args "-n" // XXX idfah
#else
    #define rcmd "ssh"
    #define cmd_args "-o ConnectTimeout=2", "-o StrictHostkeyChecking=no", "-o ForwardX11=no"
#endif

#define rbuff_isize    3     // size of host read buffer
#define npar_default   10    // default number of commands to run in parallel

// default arguments to rcmd, should be able to configure in environment var or something XXX - idfah
/*char *cmd_args[] =
        {"-o ConnectTimeout=2",
         "-o StrictHostkeyChecking=no",
         "-o ForwardX11=no"};
int ncmd_args = 3;*/

// when to display colors
typedef enum {
    color_always,
    color_never,
    color_auto
} color;

// color definitions
#define coltx_host 1        // host text color
#define colfg_host 31       // host foreground color
#define colbg_host 0        // host background color
#define coltx_dash 1        // dash text color
#define colfg_dash 36       // dash foreground color
#define colbg_dash 0        // dash background color
#define coltx_err  1        // error text color
#define colfg_err  37       // error foregroud color
#define colbg_err  41       // error background color


//
const int lockfile_flags = O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC;

char     *prog_name;       // name of this program
char     *command = NULL;  // command to execute remotely
int       input   = -1;    // input file descriptor
unsigned  npar    = 0;     // number of commands to run in parallel
bool      interac = false; //
bool      async   = true; //
color     colstat = color_auto; // weather or not to use color
bool      usecol  = false;
struct timespec delay = {.tv_sec=0, .tv_nsec=0}; // delay between hosts

/*
 *  Function bodies
 */

/*  Print program usage to standard out.
*/
void print_usage()
{
    printf("Usage: %s [OPTIONS] command\n", prog_name);
    printf("    -c, --color\n"
            "    -d, --delay\n"
            "    -f, --file\n"
            "    -h, --help\n"
            "    -h, --version\n"
            "    -i, --interactive\n"
            "    -p, --parallel\n"
            "    -q, --quiet\n"
            "    -u, --user\n"
            "    -v, --verbose\n");
}

/*  Parse command line arguments and
    setup variables accordingly.
    */
void parse_args(int narg, char *arg[])
{
    int i;

    // long options
    const struct option longopts[] = {
        { "color",       optional_argument, NULL, 'c' },
        { "delay",       required_argument, NULL, 'd' },
        { "file",        required_argument, NULL, 'f' },
        { "help",        no_argument,       NULL, 'h' },
        { "interactive", no_argument,       NULL, 'i' },
        { "parallel",    optional_argument, NULL, 'p' },
        { "quiet",       no_argument,       NULL, 'q' },
        { "verbose",     no_argument,       NULL, 'v' }
    };

    // option string 
    const char optstring[] = "+c::d:f:hip::qv";

    // for each command-line argument
    while ((i = getopt_long(narg, arg, optstring, longopts, NULL)) != -1) {
        if (i == 'c') {
            if (optarg) {
                unsigned i = 0;
                do optarg[i] = tolower(optarg[i]);
                while (optarg[++i] != '\0');

                if (strcmp(optarg, "always") == 0) {
                    colstat = color_always;
                    debug_print(2, "color: always");
                }
                else if (strcmp(optarg, "auto") == 0) {
                    colstat = color_auto;
                    debug_print(2, "color: auto");
                }
                else if (strcmp(optarg, "never") == 0) {
                    colstat = color_never;
                    debug_print(2, "color: never");
                }
                else {
                    fprintf(stderr, "Invalid color mode: %s\n", optarg);
                    print_usage();
                    exit(EXIT_FAILURE);
                }
            }
            else {
                colstat = color_always;
                debug_print(2, "color: always");
            }
        }

        // set delay between remote commands
        else if (i == 'd') {
            // convert to double
            errno = 0;
            double full_delay = strtod(optarg, NULL);
            if ((full_delay <= 0)        ||
                    (full_delay == INFINITY) ||
                    (full_delay == NAN)      ||
                    (errno != 0)        )
                debug_fail("Invalid delay string");

            debug_print(2, "delay: %f", full_delay);

            delay.tv_sec  = (long)full_delay;
            delay.tv_nsec = (long)((full_delay-floor(full_delay))*1000000000.0);

            debug_print(2, "delay seconds: %ld", delay.tv_sec);
            debug_print(2, "delay nanoseconds: %ld", delay.tv_nsec);
        }

        // specify host file
        else if (i == 'f') {
            input = open(optarg, O_RDONLY, 0x0);
            if (input < 0)
                debug_fail("Failed to open %s: %s", optarg, strerror(errno));

            debug_print(0, "opened input %s", optarg);

            // need to close file somewhere if not stdin!!
        }

        // print usage and quit
        else if (i == 'h') {
            print_usage();
            exit(EXIT_SUCCESS);
        }

        // allow interactive mode
        else if (i == 'i')
            interac = true;

        // setup parallel execution
        else if (i == 'p') {
            // if optional argument given
            if (optarg) {
                // set number of parallel commands
                errno = 0;
                npar = (unsigned)strtol(optarg, (char**)NULL, 10);
                if (errno != 0)
                    debug_fail("%s", strerror(errno));
            }
            else
                // use default number of parallel commands
                npar = npar_default;
        }

        else if (i == 'q')
            debug_set(0);

        else if (i == 'v') {
            if (debug > 0) {
                ++debug;
                debug_print(1, "debug level: %d", debug);
            }
        }

        // print usage and quit on unknown argument
        else {
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    // skip remaining arguments if in interactive mode
    if (interac) {
        if (optind < narg)
            debug_print(1, "Ignoring commands in interactive mode");
        return; 
    }

    // fail if not interactive and
    // no remaining arguments
    if (optind == narg) {
        debug_print(1, "No command given");
        print_usage();
        exit(EXIT_FAILURE);
    }

    // grab first command argument
    command = malloc(sizeof(char)*(strlen(arg[optind])+1));
    if (command == NULL)
        debug_fail_errno("Failed to allocate memory");
    strcpy(command, arg[optind]);

    // concatenate any additional command arguments
    for (i = optind+1; i < narg; ++i) {
        command = realloc(command,
                sizeof(char)*(strlen(command)+strlen(arg[i])+2));
        if (command == NULL)
            debug_fail_errno("Failed to allocate memory");

        strcat(command, " ");
        strcat(command, arg[i]);
    }
}

/*
*/
char *host_get()
{
    unsigned  i = 0;
    unsigned  rbuff_cursize;
    char      curc;
    char     *rbuff;

    /* consider using getline! */

    // consume any spaces
    do {
        curc = fgetc(stdin);

        // we're done if reached
        // end before getting string
        if (curc == EOF)
            return NULL;
    }
    while(curc == '\n' ||
            curc == '\r' ||
            curc == ' '  ||
            curc == '\0' );

    rbuff = malloc(sizeof(char)*rbuff_isize);
    rbuff_cursize = rbuff_isize;

    while (curc != '\n' &&
            curc != '\r' &&
            curc != ' '  &&
            curc != '\0' &&
            curc != EOF  ) {
        if (i > rbuff_cursize-2) {
            rbuff = realloc(rbuff, rbuff_cursize+rbuff_isize);
            if (rbuff == NULL)
                debug_fail("%s", strerror(errno));
            rbuff_cursize += rbuff_isize;
        }

        rbuff[i++] = curc;

        curc = fgetc(stdin);
    }

    // null terminate
    rbuff[i] = '\0';

    return rbuff;
}

/*
*/
void host_print(const char *host)
{
    if(debug < 1)
        return;

    if (usecol) {
        color_set(coltx_host, colfg_host, colbg_host);
        printf("%s\n", host);
        color_set(coltx_dash, colfg_dash, colbg_dash);
        printf("-------\n");
        color_reset();

        //printf("%c[%d;%dm%s\n", col_esc, coltx_host, colfg_host, host);
        //printf("%c[%d;%dm-------", col_esc, coltx_dash, colfg_dash);
        //printf("%c[%dm\n", col_esc, col_res);
    }
    else
        printf("%s\n-------\n", host);
}

/*
*/
void seq_run()
{
    char *host;

    debug_print(1, "Running sequentially", npar);

    while ((host = host_get()) != NULL) {
        int    status;
        pid_t  id;

        host_print(host);
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);

        if ((id = fork()) < 0)
            debug_warn_errno("Failed to fork");

        else if (id == 0) {
            char  *arg[] = {rcmd, cmd_args, host, command, NULL};

            int tty = open("/dev/tty", O_RDONLY, 0x0);
            if (tty < 0)
                debug_warn_errno("Failed to open teletype /dev/tty");
            ioredir_set_in(tty);
            ioredir_set_err(STDOUT_FILENO);

            if (execvp(rcmd, arg) < 0)
                debug_fail_errno("Failed to exec " rcmd);
        }

        // why else??  shouldn't you just exit above?
        else if (wait(&status) < 0)
            debug_warn_errno("Failed to wait for child " rcmd);

        free(host);

        if (debug > 0)
            printf("\n");

        if (nanosleep(&delay,NULL) < 0)
            debug_fail_errno("Failed to sleep");
    }
}

/*
*/
void par_sync_run()
{
    debug_print(1, "Running %d in parallel synchronously", npar);
}

/*
*/
void par_async_monitor(char *host, int lock_fd)
{
    int temp_fd;
    int status;
    pid_t id;

    // create temp file
    const unsigned temp_namelen = 31+strlen(host)+strlen(prog_name);
    char *temp_name = (char*)malloc(sizeof(char)*temp_namelen);
    snprintf(temp_name, sizeof(char)*temp_namelen, "/tmp/%s-%s-XXXXXX", prog_name, host);

    //if ((temp_fd = mkstemp(temp_name)) < 0)
    if ((temp_fd = mkostemp(temp_name, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC)) < 0)
        debug_fail_errno("Failed to open tempfile %s", temp_name);

    debug_print(2, "using temp file %s", temp_name);

    ioredir_desc iod = ioredir_set_out(temp_fd);

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    // fork and exec ssh
    if ((id = fork()) < 0)
        debug_warn_errno("Failed to fork");

    else if (id == 0) {
        char *arg[] = {rcmd, cmd_args, host, command, NULL};

        int tty = open("/dev/tty", O_RDONLY, 0x0);
        if (tty < 0)
            debug_warn_errno("Failed to open teletype /dev/tty");
        ioredir_set_in(tty);
        ioredir_set_err(STDOUT_FILENO);

        /***
          when to fail vs error in fork?
          prolly need to check exit status
         ***/

        if (execvp(rcmd, arg) < 0)
            debug_fail_errno("Failed to exec " rcmd);
    }

    else if (wait(&status) < 0)
        debug_warn_errno("Failed to wait for child " rcmd);

    fflush(stdout);
    if (fseek(stdout, 0, SEEK_SET) != 0)
        debug_fail_errno("Seek failed on stream for %s", temp_name);
    /*if (lseek(temp_fd, 0, SEEK_SET) < 0)
      debug_fail_errno("Seek failed on %s", temp_name); */
    //
    ioredir_restore(iod);

    FILE* temp_file = fdopen(temp_fd, "r+");

    //
    if (lockf(lock_fd, F_LOCK, 0) != 0)
        debug_fail_errno("Unable to obtain file lock");

    debug_print(3, "output locked");

    host_print(host);

    if ((status != 0) && usecol)
        color_set(coltx_err, colfg_err, colbg_err);

    //
    while (true) {
        /*    char c;
              size_t r = read(temp_fd, &c, 1);
              if (r == 0)
              break;
              else if (r < 0)
              debug_fail_errno("Failed to read temp file %s", temp_name);

              if (write(STDOUT_FILENO, &c, 1) < 0)
              debug_fail_errno("Failed to write output"); */

        char rbuff[rbuff_isize];
        size_t r = fread(rbuff, sizeof(char), rbuff_isize, temp_file);
        if (r == 0)
            break;
        else if (r < 0) {
            if (usecol)
                color_reset();
            debug_fail_errno("Failed to read temp file %s", temp_name);
        }

        if (fwrite(rbuff, sizeof(char), r, stdout) < 0)
            debug_fail_errno("Failed to write output");
    }

    if ((status != 0) && usecol)
        color_reset();

    if (debug > 0)
        printf("\n");

    debug_print(3, "unlocking output");
    fflush(stdout);

    //
    if (lockf(lock_fd, F_ULOCK, 0) != 0)
        debug_fail_errno("Unable to free file lock");

    debug_print(2, "removing tempfile %s", temp_name);

    //  close(temp_fd);
    fclose(temp_file);
    remove(temp_name);

    exit(EXIT_SUCCESS);
}

/*
*/
void par_async_wait(int *nrunning)
{
    int status;

    if (wait(&status) < 0)
        debug_warn_errno("Failed to wait for sync_run child");

    --(*nrunning);
}

/*
*/
void par_async_run()
{
    int   nrunning = 0;
    int   lock_fd = STDOUT_FILENO;
    char *lock_name = NULL;
    char *host;

    debug_print(1, "running %d in parallel asynchronously", npar);

    // always use lock file or can use log file or tty stdout?
    /*  if (isatty(STDOUT_FILENO))
        { */
    const unsigned lock_namelen = 31+strlen(prog_name);
    lock_name = (char*)malloc(sizeof(char)*lock_namelen);
    snprintf(lock_name, sizeof(char)*lock_namelen, "/tmp/%s-lockfile-XXXXXX", prog_name);

    if ((lock_fd = mkostemp(lock_name, lockfile_flags)) < 0)
        debug_fail_errno("Failed to open lockfile %s", lock_name);

    debug_print(2, "using lockfile %s", lock_name);
    //}

    while ((host = host_get()) != NULL) {
        pid_t id;

        fflush(stdin);
        fflush(stdout);
        fflush(stderr);

        if ((id = fork()) < 0)
            debug_warn_errno("Failed to fork");

        else if (id == 0)
            par_async_monitor(host, lock_fd);

        else if (++nrunning >= npar)
            par_async_wait(&nrunning);

        free(host);

        if (nanosleep(&delay,NULL) < 0)
            debug_fail_errno("Failed to sleep");
    }

    // wait for remaining monitors to finish
    while (nrunning > 0)
        par_async_wait(&nrunning);

    /*if (lock_name != NULL)
      {*/
    debug_print(2, "closing lockfile");
    close(lock_fd);
    remove(lock_name);
    //}
}

/*
*/
int main(int narg, char *arg[])
{
    //
    prog_name = basename(arg[0]);

    parse_args(narg, arg);

    ioredir_desc orig_in = ioredir_set_in(input);

    if ((colstat == color_always) ||
            (colstat == color_auto && isatty(STDOUT_FILENO)))
        usecol = true;

    if (npar < 1)
        seq_run();

    else if (async)
        par_async_run();

    else 
        par_sync_run();

    ioredir_restore(orig_in);

    return 0;
}
