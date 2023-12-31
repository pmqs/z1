/*
  Copyright (c) 1990-2019 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2007-Mar-4 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
#include "riscos.h"

#define RISCOS
#define NO_SYMLINKS
#define NO_FCNTL_H
#define NO_UNISTD_H
#define NO_MKTEMP

/* aSc problem undefined reference to uname/ttyname/getpid in c/zip */
#ifndef NO_USER_PROGRESS
# define NO_USER_PROGRESS
#endif

/* aSc problem undefined reference to popen/pclose in c/zip */
#define NO_CHECK_UNZIP

/* chdir, getcwd?  <unistd.h>? */
#define NO_CHANGE_DIRECTORY

#define PROCNAME(n) (action == ADD || action == UPDATE ? wild(n) : \
                     procname(n, 1))

#define isatty(a) 1
#define fseek(f,o,t) riscos_fseek((f),(o),(t))

#define localtime riscos_localtime
#define gmtime riscos_gmtime

#ifdef ZCRYPT_INTERNAL
#  define ZCR_SEED2     (unsigned)3141592654L   /* use PI as seed pattern */
#endif
