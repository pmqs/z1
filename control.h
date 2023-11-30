/*
  Copyright (c) 1990-2019 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-2 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/

/* This file is used to enable and disable encryption and compression
 * methods.
 *
 * Currently this file is only used for Windows.
 *
 * Unix (including MAC OS X) and VMS use scripts to detect and set these
 * settings.  Other ports have not been tested with the additional encryption
 * and compression methods enabled.
 *
 * Other settings, such as NO_UNICODE_SUPPORT, can also be set in this
 * file.  See the comments at the bottom of this file for more on this.
 */

#ifndef __control_h           /* Don't include more than once. */
#define __control_h

#ifdef WIN32

/* On Windows, set this to 1 to enable AES encryption, 0 to disable.
 * The AES kit must be installed to enable AES encryption.
 *
 * See INSTALL for more on AES encryption, including how to get the AES
 * kit as well as how to install it to add AES encryption to Zip.
 */
# if 1
#  ifndef CRYPT_AES_WG
#   define CRYPT_AES_WG
#  endif
# endif

/* On Windows, set this to 1 to enable BZIP2 compression, 0 to disable. */
# if 1
#  ifndef BZIP2_SUPPORT
#   define BZIP2_SUPPORT
#  endif
# endif

/* On Windows, set this to 1 to enable LZMA compression, 0 to disable. */
# if 1
#  ifndef LZMA_SUPPORT
#   define LZMA_SUPPORT
#  endif
# endif

/* On Windows, set this to 1 to enable PPMd compression, 0 to disable. */
# if 1
#  ifndef PPMD_SUPPORT
#   define PPMD_SUPPORT
#  endif
# endif


/* On Windows, if compiling in bzip2 (as VS 6 and VS 2010 do) rather than
 * build the bzip2 library and link to it, set bzip2 compilation settings.
 * (Do not alter these, unless linking to the bzip2 library instead.)
 */
# ifdef BZIP2_SUPPORT
#  ifndef BZ_NO_STDIO
#   define BZ_NO_STDIO
#  endif
#  ifndef BZIP2_USEBZIP2DIR
#   define BZIP2_USEBZIP2DIR
#  endif
# endif


/* Windows global defines.
 *
 * If there are other compilation settings you need set when Zip on
 * Windows is built, they can be specified here.  For instance,
 * #define NO_CRYPT_TRAD
 * would disable inclusion of Traditional (ZipCrypto) encryption.  And
 * #define NO_UNICODE_SUPPORT
 * would disable Unicode support (not recommended on Win32 except for testing).
 */
 
/*
These are here for testing different builds.
#define NO_UNICODE_SUPPORT
#define NO_USE_ZIP64_PLACEHOLDER
#define NO_STREAM_EF_SUPPORT
#define NO_BACKUP_SUPPORT
#define NO_CD_ONLY_SUPPORT
#define NO_SKIP_SCAN
#define NO_FIFO_SUPPORT
#define NO_ZIP64_SUPPORT
#define NO_CRYPT
#define NO_CRYPT_TRAD
#define NO_CRYPT_AES_WG
#define NO_ETWODD_SUPPORT
#define NO_WIN32_WIDE_CMD_LINE
#define NO_WINDOWS_LONG_PATHS
#define NO_ENABLE_ENTRY_TIMING
#define STREAM_COMMENTS
#define NO_LARGE_FILE_SUPPORT
#define NO_USE_EF_UT_TIME
#define NO_CHANGE_DIRECTORY
*/


#endif /* def WIN32 */


/* ALL platforms.
 *
 * Settings common to ALL platforms can be specified here, but this should
 * be done with caution as the Unix and VMS build environments use scripts
 * that could be disrupted by overriding them here.  In particular, linker
 * settings may not be properly set if the build scripts are not told that
 * a feature is enabled or disabled here.  Preferred approach is to provide
 * such settings to Unix and VMS via the platform specific build scripts.
 * Use the above section to set Windows only settings, not this section.
 */



#endif /* ndef __control_h */
