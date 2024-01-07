/*
  api.c - Zip 3

  Copyright (c) 1990-2024 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-2 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  api.c

  This module supplies a Zip LIB/DLL engine for use directly from C/C++
  and other programs.  This module was originally used only by the Windows
  and OS2 DLLs, but has been adapted for more general use with the LIB
  interface of some ports.  See api.h for more information.

  zip32 is compiled as 32 bit.  zip64 is compiled as 64 bit.  zip32 should
  work on 32-bit and 64-bit systems.  zip64 is for 64-bit systems only.

  The entry points are:

    void ZIPEXPENTRY ZpVersion()              - get version information
    int  ZIPEXPENTRY ZpZip()                  - call Zip with command line
    int  ZIPEXPENTRY ZpZipArgs()              - call Zip with argv, argc
    int  ZIPEXPENTRY ZpStringCopy()           - copy string to provided memory

  Deprecated/Obsolete:
    int  ZIPEXPENTRY ZpInit()                 - init function structure
    int  ZIPEXPENTRY ZpArchive()              - call Zip with Options structure

  For testing:
    int   ZIPEXPENTRY ZpTestComParse()        - test command line parsing
    void  ZIPEXPENTRY ZpTestCallback()        - test callback
    void  ZIPEXPENTRY ZpTestCallbackStruct()  - test callback structure
    int   ZIPEXPENTRY ZpZipTest()             - test the main Zip callbacks
    int   ZIPEXPENTRY ZpCalcCrc32()           - Calculate CRC32

  See api.h and below comments for additional details.

  ---------------------------------------------------------------------------*/
#define __API_C

#include "api.h"                /* this includes zip.h */

#ifdef WINDLL
  /* used for the WIN32 DLL and LIB */
# include <direct.h>
# include <malloc.h>
# include <windows.h>
# include "windll/windll.h"

  /* for other ports the mappings to standard functions in api.h are used */
#endif

#if defined(UNIX) && defined(ZIP_LIB_DLL)
# include <unistd.h>
#endif

#ifdef OS2
#  define  INCL_DOSMEMMGR
#  include <os2.h>
#endif

#ifdef __BORLANDC__
#  include <dir.h>
#endif

#include <ctype.h>
#include <errno.h>

#include "crypt.h"
#include "revision.h"

#ifdef USE_ZLIB
#  include "zlib.h"
#endif

#include "crc32.h"


ZIPUSERFUNCTIONS ZipUserFunctions, far * lpZipUserFunctions;



int ZipRet;
char szOrigDir[PATH_MAX];
BOOL fNo_int64 = FALSE; /* flag for DLLSERVICE_NO_INT64 */

/* Local forward declarations */
extern int  zipmain OF((int, char **));
int AllocMemory(unsigned int, char *, char *, BOOL);
int ParseString(LPSTR, unsigned int);
void FreeArgVee(void);
int commandline_to_argv(char *commandline, char ***pargv);

ZPOPT ZipOpts;
char **argVee;
unsigned int argCee;

char szRootDir[PATH_MAX];
char szExcludeList[PATH_MAX];
char szIncludeList[PATH_MAX];
char szTempDir[PATH_MAX];

/*---------------------------------------------------------------------------
    Local functions
  ---------------------------------------------------------------------------*/

#ifndef NO_PROTO
int ParseString(LPSTR s, unsigned int ArgC)
#else
int ParseString(s, ArgC)
  LPSTR s;
  unsigned int ArgC;
#endif
{
unsigned int i;
int root_flag, m, j;
char *str1, *str2, *str3;
size_t size;

i = ArgC;
str1 = (char *) malloc(lstrlen(s)+4);
lstrcpy(str1, s);
lstrcat(str1, " @");

if (szRootDir[0] != '\0')
    {
    root_flag = TRUE;
    if (szRootDir[lstrlen(szRootDir)-1] != '\\')
        lstrcat(szRootDir, "\\");
    }
else
    root_flag = FALSE;

str2 = strchr(str1, '\"'); /* get first occurrence of double quote */

while ((str3 = strchr(str1, '\t')) != NULL)
    {
    str3[0] = ' '; /* Change tabs into a single space */
    }

/* Note that if a quoted string contains multiple adjacent spaces, they
   will not be removed, because they could well point to a valid
   folder/file name.
*/
while ((str2 = strchr(str1, '\"')) != NULL)
    /* Found a double quote if not NULL */
    {
    str3 = strchr(str2+1, '\"'); /* Get the second quote */
    if (str3 == NULL)
        {
        free(str1);
        return ZE_PARMS; /* Something is screwy with the
                            string, bail out */
        }
    str3[0] = '\0';  /* terminate str2 with a NULL */

    /* strip unwanted fully qualified path from entry */
    if (root_flag)
        if ((_strnicmp(szRootDir, str2+1, lstrlen(szRootDir))) == 0)
            {
            m = 0;
            str2++;
            for (j = lstrlen(szRootDir); j < lstrlen(str2); j++)
                str2[m++] = str2[j];
            str2[m] = '\0';
            str2--;
            }
    size = _msize(argVee);
    if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
        {
        fprintf(stdout, "Unable to allocate memory in zip dll\n");
        free(str1);
        return ZE_MEM;
        }
    /* argCee is incremented in AllocMemory */
    if (AllocMemory(i, str2+1, "Creating file list from string", TRUE) != ZE_OK)
        {
        free(str1);
        return ZE_MEM;
        }
    i++;
    str3+=2;        /* Point past the whitespace character */
    str2[0] = '\0'; /* Terminate str1 */
    lstrcat(str1, str3);
    }    /* end while */

/* points to first occurance of a space */
str2 = strchr(str1, ' ');

/*  Go through the string character by character, looking for instances
    of two spaces together. Terminate when you find the trailing @
*/
while ((str2[0] != '\0') && (str2[0] != '@'))
    {
    while ((str2[0] == ' ') && (str2[1] == ' '))
        {
        str3 = &str2[1];
        str2[0] = '\0';
        lstrcat(str1, str3);
        }
    str2++;
    }

/* Do we still have a leading space? */
if (str1[0] == ' ')
    {
    str3 = &str1[1];
    lstrcpy(str1, str3); /* Dump the leading space */
    }


/* Okay, now we have gotten rid of any tabs and replaced them with
   spaces, and have replaced multiple spaces with a single space. We
   couldn't do this before because the folder names could have actually
   contained these characters.
*/

str2 = str3 = str1;

while ((str2[0] != '\0') && (str3[0] != '@'))
    {
    str3 = strchr(str2+1, ' ');
    str3[0] = '\0';
    /* strip unwanted fully qualified path from entry */
    if (root_flag)
        if ((_strnicmp(szRootDir, str2, lstrlen(szRootDir))) == 0)
           {
            m = 0;
            for (j = lstrlen(ZipOpts.szRootDir); j < lstrlen(str2); j++)
            str2[m++] = str2[j];
            str2[m] = '\0';
            }
    size = _msize(argVee);
    if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
        {
        fprintf(stdout, "Unable to allocate memory in zip dll\n");
        free(str1);
        return ZE_MEM;
        }
    if (AllocMemory(i, str2, "Creating file list from string", TRUE) != ZE_OK)
        {
        free(str1);
        return ZE_MEM;
        }
    i++;
    str3++;
    str2 = str3;
    }
free(str1);
return ZE_OK;
}

#ifndef NO_PROTO
int AllocMemory(unsigned int i, char *cmd, char *str, BOOL incrementArgCee)
#else
int AllocMemory(i, cmd, str, incrementArgCee)
  unsigned int i;
  char *cmd;
  char *str;
  BOOL incrementArgCee;
#endif
{
if ((argVee[i] = (char *) malloc( sizeof(char) * strlen(cmd)+1 )) == NULL)
   {
   if (incrementArgCee)
       argCee++;
   FreeArgVee();
   fprintf(stdout, "Unable to allocate memory in zip library at %s\n", str);
   return ZE_MEM;
   }
strcpy( argVee[i], cmd );
argCee++;
return ZE_OK;
}

void FreeArgVee(void)
{
unsigned i;

/* Free the arguments in the array */
for (i = 0; i < argCee; i++)
    {
    free (argVee[i]);
    argVee[i] = NULL;
    }
/* Then free the array itself */
free(argVee);

#ifdef CHANGE_DIRECTORY

/* Restore the original working directory */
if (CHDIR(szOrigDir) == -1)
  {
  return;
  }
# ifdef __BORLANDC__
  setdisk(toupper(szOrigDir[0]) - 'A');
# endif

#endif /* def CHANGE_DIRECTORY */

}


/*---------------------------------------------------------------------------
    Documented API entry points
  ---------------------------------------------------------------------------*/


/* ZpInit - Call first to initialize the LIB or DLL
 *
 * This is deprecated.  Use ZpZip and ZpZipArgs, which does it all in one
 * call.
 *
 * This can be used in a LIB application to set up function callbacks
 * before calling zipmain().
 *
 * Avoid using ZpInit with DLL as it doesn't share.  It's use can also
 * cause a race condition in the DLL environment.  DLL users should only
 * use ZpZip or ZpZipArgs.
 */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpInit(LPZIPUSERFUNCTIONS lpZipUserFunc)
#else
int ZIPEXPENTRY ZpInit(lpZipUserFunc)
  LPZIPUSERFUNCTIONS lpZipUserFunc;
#endif
{
#if 0
ZipUserFunctions = *lpZipUserFunc;
lpZipUserFunctions = &ZipUserFunctions;
#endif

  /* Instead of getting address for user's structure and saving
     that, now copy over function addresses to a DLL global
     structure. */
  ZipUserFunctions.print            = lpZipUserFunc->print;
  ZipUserFunctions.ecomment         = lpZipUserFunc->ecomment;
  ZipUserFunctions.acomment         = lpZipUserFunc->acomment;
  ZipUserFunctions.password         = lpZipUserFunc->password;
  ZipUserFunctions.service          = lpZipUserFunc->service;
  ZipUserFunctions.service_no_int64 = lpZipUserFunc->service_no_int64;
  ZipUserFunctions.progress         = lpZipUserFunc->progress;
  ZipUserFunctions.error            = lpZipUserFunc->error;
  ZipUserFunctions.finish           = lpZipUserFunc->finish;

  /* Pointer now points to local DLL structure. */
  lpZipUserFunctions = &ZipUserFunctions;

  /* It's up to user what callback functions they want to use
     with LIB or DLL.  If the user uses -z to set an archive
     comment without providing acomment callback, for example,
     Zip will complain. */
#if 0
if (!lpZipUserFunctions->print ||
    !lpZipUserFunctions->acomment)
    return FALSE;
#endif

return TRUE;
}


/* ZpArchive - Call to pass in options and actually do work
 *
 * This call is deprecated and is replaced by ZpZip and ZpZipArgs.  This entry
 * point remains as older application code is being converted to use ZpZip and
 * ZpZipArgs.  Note that the Opts structure has not been updated to include
 * any recent changes or additions to Zip.
 */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpArchive(ZCL C, LPZPOPT Opts)
#else
int ZIPEXPENTRY ZpArchive(C, Opts)
  ZCL C;
  LPZPOPT Opts;
#endif
  /* Add, update, freshen, or delete zip entries in a zip file.  See the
   command help in help() zip.c */
{
int k, j, m;
size_t size;

ZipOpts = *Opts; /* Save off options, and make them available locally */
szRootDir[0] = '\0';
szExcludeList[0] = '\0';
szIncludeList[0] = '\0';
szTempDir[0] = '\0';
if (ZipOpts.szRootDir) lstrcpy(szRootDir, ZipOpts.szRootDir);
if (ZipOpts.szExcludeList) lstrcpy(szExcludeList, ZipOpts.szExcludeList);
if (ZipOpts.szIncludeList) lstrcpy(szIncludeList, ZipOpts.szIncludeList);
if (ZipOpts.szTempDir) lstrcpy(szTempDir, ZipOpts.szTempDir);

#ifdef CHANGE_DIRECTORY

if (GETCWD(szOrigDir, PATH_MAX) == NULL) { /* Save current drive and directory */
    return ZE_PATH;
}

if (szRootDir[0] != '\0')
    {
# ifdef WIN32
    char c;

    /* Make sure there isn't a trailing slash */
    c = szRootDir[lstrlen(szRootDir)-1];
    if (c == '\\' || c == '/')
        szRootDir[lstrlen(szRootDir)-1] = '\0';
# endif

    if (CHDIR(szRootDir) == -1)
        {
        return ZE_PARMS;
        }
#ifdef __BORLANDC__
        setdisk(toupper(szRootDir[0]) - 'A');
#endif
    }
    
#endif /* def CHANGE_DIRECTORY */

argCee = 0;

/* malloc additional 40 to allow for additional command line arguments. Note
    that we are also adding in the count for the include lists as well as the
    exclude list. */
if ((argVee = (char **)malloc((C.argc+40)*sizeof(char *))) == NULL)
    {
    fprintf(stdout, "Unable to allocate memory in zip dll\n");
    return ZE_MEM;
    }
if ((argVee[argCee] = (char *) malloc( sizeof(char) * strlen("wiz.exe")+1 )) == NULL)
    {
    free(argVee);
    fprintf(stdout, "Unable to allocate memory in zip dll\n");
    return ZE_MEM;
    }

strcpy( argVee[argCee], "wiz.exe" );
argCee++;


/* Process options */

/* Set compression level efficacy -0...-9 */
if (AllocMemory(argCee, "-0", "Compression", FALSE) != ZE_OK)
    return ZE_MEM;

/* Check to see if the compression level is set to a valid value. If
  not, then set it to the default.
*/
if ((ZipOpts.fLevel < '0') || (ZipOpts.fLevel > '9'))
    {
    int level = ZipOpts.fLevel;
    ZipOpts.fLevel = '6';
    if (!ZipOpts.fDeleteEntries)
        fprintf(stdout, "Compression level set to invalid value:  ASCII hex %02x.\n", level);
        fprintf(stdout, "Setting to default\n");
    }

argVee[argCee-1][1] = ZipOpts.fLevel;

/* this is klugy - should find out why -0 for deflate generates packing error */
if (ZipOpts.fLevel == '0')
    {
    if (AllocMemory(argCee, "-4", "Compression level command", FALSE) != ZE_OK)
        return ZE_MEM;
    if (AllocMemory(argCee, "-Z", "Compression method command", FALSE) != ZE_OK)
        return ZE_MEM;
    if (AllocMemory(argCee, "store", "Compression method string", FALSE) != ZE_OK)
        return ZE_MEM;
    fprintf(stdout, "Compression method set to STORE.\n");
    }

if ((ZipOpts.szCompMethod != NULL) && (ZipOpts.szCompMethod[0] != '\0'))
    {
    if (AllocMemory(argCee, "-Z", "Compression method command", FALSE) != ZE_OK)
        return ZE_MEM;
    if (AllocMemory(argCee, ZipOpts.szCompMethod, "Compression method string", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fOffsets)    /* Update offsets for SFX prefix */
    {
    if (AllocMemory(argCee, "-A", "Offsets", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fDeleteEntries)    /* Delete files from zip file -d */
    {
    if (AllocMemory(argCee, "-d", "Delete", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fNoDirEntries) /* Do not add directory entries -D */
    {
        if (AllocMemory(argCee, "-D", "No Dir Entries", FALSE) != ZE_OK)
            return ZE_MEM;
    }
if (ZipOpts.fFreshen) /* Freshen zip file--overwrite only -f */
    {
    if (AllocMemory(argCee, "-f", "Freshen", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fRepair)  /* Fix archive -F or -FF */
    {
    if (ZipOpts.fRepair == 1)
      {
      if (AllocMemory(argCee, "-F", "Repair", FALSE) != ZE_OK)
          return ZE_MEM;
      }
    else
      {
      if (AllocMemory(argCee, "-FF", "Repair", FALSE) != ZE_OK)
        return ZE_MEM;
      }
    }
if (ZipOpts.fGrow) /* Allow appending to a zip file -g */
    {
    if (AllocMemory(argCee, "-g", "Appending", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fJunkDir) /* Junk directory names -j */
    {
    if (AllocMemory(argCee, "-j", "Junk Dir Names", FALSE) != ZE_OK)
        return ZE_MEM;
    }

/* fEncrypt */
if ((ZipOpts.fEncrypt < 0) || (ZipOpts.fEncrypt > 1)) {
        fprintf(stdout,
          "Only Encrypt method 1 currently supported, ignoring %d\n", ZipOpts.fEncrypt);
} else
if (ZipOpts.fEncrypt == 1) /* encrypt -e */
    {
    if (AllocMemory(argCee, "-e", "Encrypt", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fJunkSFX) /* Junk sfx prefix */
    {
    if (AllocMemory(argCee, "-J", "Junk SFX", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fForce) /* Make entries using DOS names (k for Katz) -k */
    {
    if (AllocMemory(argCee, "-k", "Force DOS", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fLF_CRLF) /* Translate LF_CRLF -l */
    {
    if (AllocMemory(argCee, "-l", "LF-CRLF", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fCRLF_LF) /* Translate CR/LF to LF -ll */
    {
    if (AllocMemory(argCee, "-ll", "CRLF-LF", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fMove) /* Delete files added to or updated in zip file -m */
    {
    if (AllocMemory(argCee, "-m", "Move", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fLatestTime) /* Set zip file time to time of latest file in it -o */
    {
    if (AllocMemory(argCee, "-o", "Time", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fComment) /* Add archive comment "-z" */
    {
    if (AllocMemory(argCee, "-z", "Comment", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fQuiet) /* quiet operation -q */
    {
    if (AllocMemory(argCee, "-q", "Quiet", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fRecurse == 1) /* recurse into subdirectories -r */
    {
    if (AllocMemory(argCee, "-r", "Recurse -r", FALSE) != ZE_OK)
        return ZE_MEM;
    }
else if (ZipOpts.fRecurse == 2) /* recurse into subdirectories -R */
    {
    if (AllocMemory(argCee, "-R", "Recurse -R", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fSystem)  /* include system and hidden files -S */
    {
    if (AllocMemory(argCee, "-S", "System", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if ((ZipOpts.ExcludeBeforeDate) &&
      (ZipOpts.ExcludeBeforeDate[0] != '\0'))    /* Exclude files newer than specified date -tt */
    {
      {
      if (AllocMemory(argCee, "-tt", "Date", FALSE) != ZE_OK)
          return ZE_MEM;
      if (AllocMemory(argCee, ZipOpts.ExcludeBeforeDate, "Date", FALSE) != ZE_OK)
          return ZE_MEM;
      }
    }

if ((ZipOpts.IncludeBeforeDate) &&
      (ZipOpts.IncludeBeforeDate[0] != '\0'))    /* Include files newer than specified date -t */
    {
      {
      if (AllocMemory(argCee, "-t", "Date", FALSE) != ZE_OK)
          return ZE_MEM;
      if (AllocMemory(argCee, ZipOpts.IncludeBeforeDate, "Date", FALSE) != ZE_OK)
          return ZE_MEM;
      }
    }

if (ZipOpts.fUpdate) /* Update zip file--overwrite only if newer -u */
    {
    if (AllocMemory(argCee, "-u", "Update", FALSE) != ZE_OK)
        return ZE_MEM;
    }

/* fUnicode (was fMisc) */
if (ZipOpts.fUnicode & 2) /* No UTF-8 */
    {
    if (AllocMemory(argCee, "-UN=N", "UTF-8 No", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fUnicode & 4) /* Native UTF-8 */
    {
    if (AllocMemory(argCee, "-UN=U", "UTF-8 Native", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (ZipOpts.fVerbose)  /* Mention oddities in zip file structure -v */
    {
    if (AllocMemory(argCee, "-v", "Verbose", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.fVolume)  /* Include volume label -$ */
    {
    if (AllocMemory(argCee, "-$", "Volume", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.szSplitSize != NULL)   /* Turn on archive splitting */
    {
    if (AllocMemory(argCee, "-s", "Splitting", FALSE) != ZE_OK)
        return ZE_MEM;
    if (AllocMemory(argCee, ZipOpts.szSplitSize, "Split size", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (lpZipUserFunctions->split != NULL)   /* Turn on archive split destinations select */
    {
    if (AllocMemory(argCee, "-sp", "Split Pause Select Destination", FALSE) != ZE_OK)
        return ZE_MEM;
    }
#ifdef WIN32
if (ZipOpts.fPrivilege)  /* Use privileges -! */
    {
    if (AllocMemory(argCee, "-!", "Privileges", FALSE) != ZE_OK)
        return ZE_MEM;
    }
#endif
if (ZipOpts.fExtra)  /* Exclude extra attributes -X */
    {
    if (AllocMemory(argCee, "-X", "Extra", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.IncludeList != NULL) /* Include file list -i */
    {
    if (AllocMemory(argCee, "-i", "Include file list", FALSE) != ZE_OK)
        return ZE_MEM;
    k = 0;
    if (ZipOpts.IncludeListCount > 0)
        while ((ZipOpts.IncludeList[k] != NULL) && (ZipOpts.IncludeListCount != k+1))
            {
            size = _msize(argVee);
            if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
                {
                fprintf(stdout, "Unable to allocate memory in zip dll\n");
                return ZE_MEM;
                }
            if (AllocMemory(argCee, ZipOpts.IncludeList[k], "Include file list array", TRUE) != ZE_OK)
                {
                return ZE_MEM;
                }
            k++;
            }
    else
        while (ZipOpts.IncludeList[k] != NULL)
            {
            size = _msize(argVee);
            if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
                {
                FreeArgVee();
                fprintf(stdout, "Unable to allocate memory in zip dll\n");
                return ZE_MEM;
                }
            if (AllocMemory(argCee, ZipOpts.IncludeList[k], "Include file list array", TRUE) != ZE_OK)
                return ZE_MEM;
            k++;
            }

    if (AllocMemory(argCee, "@", "End of Include List", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (ZipOpts.ExcludeList != NULL)  /* Exclude file list -x */
    {
    if (AllocMemory(argCee, "-x", "Exclude file list", FALSE) != ZE_OK)
        return ZE_MEM;
    k = 0;
    if (ZipOpts.ExcludeListCount > 0)
        while ((ZipOpts.ExcludeList[k] != NULL) && (ZipOpts.ExcludeListCount != k+1))
            {
            size = _msize(argVee);
            if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
                {
                fprintf(stdout, "Unable to allocate memory in zip dll\n");
                return ZE_MEM;
                }
            if (AllocMemory(argCee, ZipOpts.ExcludeList[k], "Exclude file list array", TRUE) != ZE_OK)
                return ZE_MEM;
            k++;
            }
    else
        while (ZipOpts.ExcludeList[k] != NULL)
            {
            size = _msize(argVee);
            if ((argVee = (char **)realloc(argVee, size + sizeof(char *))) == NULL)
                {
                FreeArgVee();
                fprintf(stdout, "Unable to allocate memory in zip dll\n");
                return ZE_MEM;
                }
            if (AllocMemory(argCee, ZipOpts.ExcludeList[k], "Exclude file list array", TRUE) != ZE_OK)
                return ZE_MEM;
            k++;
            }
    if (AllocMemory(argCee, "@", "End of Exclude List", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (szIncludeList[0] != '\0') /* Include file list -i */
    {
    if (AllocMemory(argCee, "-i", "Include file list", FALSE) != ZE_OK)
        return ZE_MEM;
    if ((k = ParseString(szIncludeList, argCee)) != ZE_OK)
        return k;  /* Something was screwy with the parsed string
                      bail out */
    if (AllocMemory(argCee, "@", "End of Include List", FALSE) != ZE_OK)
        return ZE_MEM;
    }
if (szExcludeList[0] != '\0')  /* Exclude file list -x */
    {
    if (AllocMemory(argCee, "-x", "Exclude file list", FALSE) != ZE_OK)
        return ZE_MEM;

    if ((k = ParseString(szExcludeList, argCee)) != ZE_OK)
        return k;  /* Something was screwy with the parsed string
                      bail out */

    if (AllocMemory(argCee, "@", "End of Exclude List", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if (szTempDir[0] != '\0') /* Use temporary directory -b */
    {
    if (AllocMemory(argCee, "-b", "Temp dir switch command", FALSE) != ZE_OK)
        return ZE_MEM;
    if (AllocMemory(argCee, szTempDir, "Temporary directory", FALSE) != ZE_OK)
        return ZE_MEM;
    }

if ((ZipOpts.szProgressSize != NULL) && (ZipOpts.szProgressSize[0] != '\0')) /* Progress chunk size */
    {
      progress_chunk_size = ReadNumString(ZipOpts.szProgressSize);
    }
    else
    {
      progress_chunk_size = 0;
    }

if (AllocMemory(argCee, C.lpszZipFN, "Zip file name", FALSE) != ZE_OK)
    return ZE_MEM;

if (szRootDir[0] != '\0')
    {
    if (szRootDir[lstrlen(szRootDir)-1] != '\\')
          lstrcat(szRootDir, "\\"); /* append trailing \\ */
    if (C.FNV != NULL)
        {
        for (k = 0; k < C.argc; k++)
            {
            if (AllocMemory(argCee, C.FNV[k], "Making argv", FALSE) != ZE_OK)
                return ZE_MEM;
            if ((_strnicmp(szRootDir, C.FNV[k], lstrlen(szRootDir))) == 0)
                {
                m = 0;
                for (j = lstrlen(szRootDir); j < lstrlen(C.FNV[k]); j++)
                    argVee[argCee-1][m++] = C.FNV[k][j];
                argVee[argCee-1][m] = '\0';
                }
            }
        }

    }
else
  if (C.FNV != NULL)
    for (k = 0; k < C.argc; k++)
        {
        if (AllocMemory(argCee, C.FNV[k], "Making argv", FALSE) != ZE_OK)
            return ZE_MEM;
        }

if (C.lpszAltFNL != NULL)
    {
    if ((k = ParseString(C.lpszAltFNL, argCee)) != ZE_OK)
        return k;  /* Something was screwy with the parsed string
                      bail out
                    */
    }



argVee[argCee] = NULL;

ZipRet = zipmain(argCee, argVee);

/* Free the arguments in the array. Note this also restores the
    current directory
  */
FreeArgVee();

return ZipRet;
}

#ifdef IZ_CRYPT_ANY

#if 0
/* Version used by UnZip. */
int encr_passwd(int modeflag, char *pwbuf, int size, const char *zfn)
    {
    return (*lpZipUserFunctions->password)(pwbuf, size, ((modeflag == ZP_PW_VERIFY) ?
                  "Verify password: " : "Enter password: "),
                  (char *)zfn);
    }
#endif

/* Zip does not understand passwords for individual files (as UnZip does), so
   passing the Zip file name (zfn) does not make sense and is, in fact,
   misleading, as passwords are generic and not associated with a particular
   file in Zip (though entries copied from an existing archive retain their
   passwords).  If the caller uses both the Zip and UnZip DLLs, they will
   need to supply separate password functions to each. */

#ifndef NO_PROTO
int simple_encr_passwd(int modeflag, char *pwbuf, size_t bufsize)
#else
int simple_encr_passwd(modeflag, pwbuf, bufsize)
  int modeflag;
  char *pwbuf;
  size_t bufsize;
#endif
{
  char passwordbuf[MAX_PASSWORD_LEN + 1];
	char prompt[20];
	long ret;

	if (modeflag == ZP_PW_VERIFY)
      strcpy(prompt, "Verify password: ");
	else
	strcpy(prompt, "Enter password: ");

  if ((void *)*lpZipUserFunctions->password) {
	  if (bufsize > MAX_PASSWORD_LEN)
		  bufsize = MAX_PASSWORD_LEN;
#if 0
    strcpy(passwordbuf, "12345678901");
#endif
    ret = (*lpZipUserFunctions->password)((long)bufsize, prompt, passwordbuf);
    if (ret == 0)
      return -1;
    strcpy(pwbuf, passwordbuf);
  }
  else
  {
    ZIPERR(ZE_PARMS, "-e used to request password but no callback");
  }

return (int)strlen(pwbuf);
}

#endif /* def IZ_CRYPT_ANY */



/* --------------------------------------------- */
/* DLL test entry points
 */
#ifdef WINDLL

/* Currently these are only set up and tested for WINDLL (Windows
 * LIB and DLL), though there's probably no reason these could not
 * be made available for other platforms. */

/* The below are some test DLL entry points for working out bugs in
 * the interfacing between a user app and the DLL.
 *
 * See the VB example in vb10 for how to call these from VB.
 *
 * These may be updated or removed in a later Zip version and should
 * not be relied on.
 *
 * 2014-04-08 EG
 */


/* Calculate CRC32 for buffer.  (2021-02-28) */

#ifndef NO_PROTO
unsigned long ZIPEXPENTRY ZpCalcCrc32Buffer(unsigned char *buffer, unsigned long len)
#else
unsigned long ZIPEXPENTRY ZpCalcCrc32Buffer(buffer, len)
  unsigned char *buffer;
  unsigned long len;
#endif
{
  unsigned long chksum;

  // For testing
  //unsigned long i;
  //char s[100000];
  //char sbuf[20];

  //s[0] = '\0';
  //strcat(s, itoa(len, sbuf, 10));
  //strcat(s, ": ");
  //
  //for (i = 0; i < len; i++) {
  //  strcat(s, itoa(buffer[i], sbuf, 10));
  //  strcat(s, " ");
  //}
  //s[i] = '\0';
  //
  //MessageBoxA(NULL, s, "ZpCalcCrc32: in",
  //            MB_OK | MB_ICONINFORMATION);

  chksum = CRCVAL_INITIAL;
  chksum = crc32(chksum, buffer, len);

  //sprintf(sbuf, "%08x", chksum);
  //
  //MessageBoxA(NULL, sbuf, "ZpCalcCrc32: out",
  //            MB_OK | MB_ICONINFORMATION);

  return chksum;
}

/* Calculate CRC32 for file.  (2021-03-01) */

#ifndef NO_PROTO
unsigned long ZIPEXPENTRY ZpCalcCrc32File(char *path)
#else
unsigned long ZIPEXPENTRY ZpCalcCrc32File(path)
  char *path;
#endif
{
  unsigned char *buffer;
  unsigned long chksum;
  FILE *infile;
  //unsigned long len;
  size_t len;
  char sbuf[20];

  if ((buffer = (unsigned char *)malloc(SBSZ)) == NULL) {
    MessageBoxA(NULL, "mem error", "ZpCalcCrc32File: out",
                MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  MessageBoxA(NULL, path, "ZpCalcCrc32File: path",
              MB_OK | MB_ICONINFORMATION);

  chksum = CRCVAL_INITIAL;

  infile = fopen(path, "rb");
  while (!feof(infile) && !ferror(infile)) {
    len = fread(buffer, 1, SBSZ, infile);
    chksum = crc32(chksum, buffer, len);
  }
  fclose(infile);
  free(buffer);

  if (ferror(infile)) {
    MessageBoxA(NULL, "read error", "ZpCalcCrc32File: out",
                MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  sprintf(sbuf, "%08x", chksum);
  MessageBoxA(NULL, sbuf, "ZpCalcCrc32File: out",
              MB_OK | MB_ICONINFORMATION);

  return chksum;
}


/* Test the parsing of the input command line. */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpTestComParse(char *commandline, char *parsedline)
#else
int ZIPEXPENTRY ZpTestComParse(commandline, parsedline)
  char *commandline;
  char *parsedline;
#endif
{
  int argc;
  char **argv;
  char parsed[4000];
  int i;
  char arg[1000];

  argc = commandline_to_argv(commandline, &argv);
  
  parsed[0] = '\0';

  if (argc) {
    for (i = 0; i < argc; i++) {
      sprintf(arg, "\"%s\"", argv[i]);
      if (i > 0) {
        strcat(parsed, "  ");
      }
      strcat(parsed, arg);
    }
  }

  strcpy(parsedline, parsed);

  return argc;
}


/* Simple callback test. */

typedef void (__stdcall *FUNCPTR)(char *pstr);

#ifndef NO_PROTO
void ZIPEXPENTRY ZpTestCallback(ZIPADDR cbAddress)
#else
void ZIPEXPENTRY ZpTestCallback(cbAddress)
  ZIPADDR cbAddress;
#endif
{
  /* Takes a callback address and returns a string to the
   * caller.  VS 2010 works with char * and does not need
   * the string arguments to be BSTR.
   */
  FUNCPTR CallbackFunc;

  char *mystring = "This is my string from the DLL";
  char b[4000];

  /* Point the function pointer at the passed-in address. */
  CallbackFunc = (FUNCPTR)cbAddress;

  /* Call the function through the function pointer. */

  sprintf(b, "'%s'", mystring);
  MessageBoxA(NULL, b, "Just before CallbackFunc",
              MB_OK | MB_ICONINFORMATION);

  if (CallbackFunc == NULL) {
    MessageBoxA(NULL, "Null function - skip running it", "Just before CallbackFunc",
                MB_OK | MB_ICONINFORMATION);
  } else {
    CallbackFunc(mystring);
  }

}


/* This example shows the passing of a structure with function
 * addresses.
 */

#ifndef NO_PROTO
void ZIPEXPENTRY ZpTestCallbackStruct(LPZpTESTFUNCTION lpTestFuncStruct)
#else
void ZIPEXPENTRY ZpTestCallbackStruct(lpTestFuncStruct)
  LPZpTESTFUNCTION lpTestFuncStruct
#endif
{
  /* typedef void (__stdcall *FUNCPTR)(BSTR pbstr); */
  typedef void (__stdcall *FUNCPTR)(char *pstr);
  FUNCPTR CallbackFunc;

  char *mystring = "This is my string from the DLL";

  /* Point the function pointer at the passed-in address. */
  CallbackFunc = (FUNCPTR)lpTestFuncStruct->print;

  /* Call the function through the function pointer. */

  MessageBoxA(NULL, "Just before CallbackFunc", "ZpTestStructCallback",
              MB_OK | MB_ICONINFORMATION);

  if (CallbackFunc == NULL) {
    MessageBoxA(NULL, "Null function - skip running it", "Just before CallbackFunc",
                MB_OK | MB_ICONINFORMATION);
  } else {
    CallbackFunc(mystring);
  }

}


/* Test the main Zip callbacks. */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpZipTest(char *CommandLine, char *CurrentDir, LPZIPUSERFUNCTIONS lpZipUserFunc)
#else
int ZIPEXPENTRY ZpZipTest(CommandLine, CurrentDir, lpZipUserFunc)
  char *CommandLine;
  char *CurrentDir;
  LPZIPUSERFUNCTIONS lpZipUserFunc;
#endif
{
  char msg[4000];
  long comlen;
  int pwlen;
  char pass[20];
  char fname[400];
  char uname[400];
  zoff_t fsize;
  zoff_t csize;
  int aborted = 0;
  char us[100];
  char cs[100];
  long perc = 0;


  sprintf(msg, "In ZpZipTest\nCommandLine = '%s'\nCurrentDir = '%s'\n", CommandLine, CurrentDir);

  MessageBoxA(NULL, msg, "ZpZip",
                MB_OK | MB_ICONINFORMATION);

  /* copy function pointers to global structure */
  ZipUserFunctions.print = lpZipUserFunc->print;
  ZipUserFunctions.ecomment = lpZipUserFunc->ecomment;
  ZipUserFunctions.acomment = lpZipUserFunc->acomment;
  ZipUserFunctions.password = lpZipUserFunc->password;
  ZipUserFunctions.service  = lpZipUserFunc->service;
  ZipUserFunctions.service_no_int64 = lpZipUserFunc->service_no_int64;
  ZipUserFunctions.progress = lpZipUserFunc->progress;
  ZipUserFunctions.error = lpZipUserFunc->error;
  ZipUserFunctions.finish = lpZipUserFunc->finish;

  lpZipUserFunctions = &ZipUserFunctions;

  if (ZipUserFunctions.acomment) {
    /* zcomlen is current zipfile comment length - should be 0 unless updating
       an archive */
    comlen = zcomlen;
    acomment(comlen);
    printf("The comment: '%s'\n", zcomment);
  }

  pwlen = simple_encr_passwd(ZP_PW_ENTER, pass, 10);
  if (pwlen > 0)
    printf("The password:  '%s'\n", pass);
  else
    printf("Did not receive a password\n");

  strcpy(fname, "a file name.txt");
  strcpy(uname, "unicode file name.txt");
  fsize = 1200;
  csize = 900;

  WriteNumString(fsize, us);
  WriteNumString(csize, cs);
  if (fsize)
    perc = percent(fsize, csize);

  if (ZipUserFunctions.service != NULL)
  {
    if ((ZipUserFunctions.service)(fname,
                                   uname,
                                   us,
                                   cs,
                                   fsize,
                                   csize,
                                   "Add",
                                   "Deflate",
                                   "",
                                   perc))
    {
      printf("ZpZip: service:  Zip operation would have been aborted\n");
      aborted = 1;
    }
    else
      printf("ZpZip: service:  Successful return from service callback\n");
  }

  if (ZipUserFunctions.progress != NULL)
  {
    if ((ZipUserFunctions.progress)(fname,
                                    uname,
                                    1000,
                                    500,
                                    300,
                                    "300",
                                    "Add",
                                    "Deflate",
                                    ""))
    {
      printf("ZpZip: progress:  Zip operation would have been aborted\n");
      aborted = 1;
    }
    else
      printf("ZpZip: progress:  Successful return from ProgressReport callback\n");
  }

  if (ZipUserFunctions.error)
  {
    (ZipUserFunctions.error)("ZpZip: error callback test message\n");
  }

  if (ZipUserFunctions.finish)
  {
    (ZipUserFunctions.finish)(us, cs, fsize, csize, perc);
  }

  sprintf(msg, "Leaving ZpZip\n");
  MessageBoxA(NULL, msg, "ZpZip",
                MB_OK | MB_ICONINFORMATION);

  return aborted;
}

#endif /* WINDLL */

/* --------------------------------------------- */

/* Below are the API entry points for new Zip DLL interface. */


/* ZpErrorMessage()
 *
 * This is called when an error occurs in the below DLL interfaces.  Nothing
 * is returned to the caller when an error occurs unless the user has provided
 * a PRINT callback, an ERROR callback, or both.  It would be up to the caller
 * to display the error message.
 */

#ifndef NO_PROTO
long ZpErrorMessage(int errcode, char *message)
#else
long ZpErrorMessage(errcode, message)
  int errcode;
  char *message;
#endif
{
  /* output message to stdout (probably redirected to PRINT callback) */
  zprintf("%s", message);

  /* output message to ERROR callback */
  if (ZipUserFunctions.error)
  {
    (ZipUserFunctions.error)(message);
  }

  return errcode;
}


/* ZpZip()
 *
 * This is the new main entry point the user calls.
 *
 * CommandLine       - Command line to Zip that it's to execute.
 * CurrentDir        - Dir to set as current dir.  Must be set for DLL.  Can
 *                     be NULL for LIB (if NULL, app's current directory used).
 * lpZipUserFunc     - Structure that caller sets callback addresses in.
 *                     Make sure any unused callback is set to NULL.
 * ProgressChunkSize - If progress callback is used, this must be set to the
 *                     number of bytes to process before next progress report
 *                     (call of progress callback).  This is set as a string
 *                     in the same form as used elsewhere in Zip, e.g. 50m for
 *                     50 MiB.  This can be NULL if the progress callback is
 *                     not used.
 *
 * Note that, on Windows, CommandLine can contain UTF-8 strings (if Unicode
 * support is enabled), which Zip will detect and convert to wide character
 * Unicode strings for processing.  If your application is dealing with wide
 * character paths and strings, likely this will mean using the standard
 * wide to UTF-8 conversion to get the UTF-8 strings that you would then pass
 * to ZpZip.  On other ports, CommandLine should be in the current character
 * set.  If that character set is UTF-8, then UTF-8 is supported.
 */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpZip(char *commandLine,
                      char *currentDir,
                      LPZIPUSERFUNCTIONS lpZipUserFunc,
                      char *progressChunkSize)
#else
int ZIPEXPENTRY ZpZip(commandLine,
                      currentDir,
                      lpZipUserFunc,
                      progressChunkSize)
  char *commandLine;
  char *currentDir;
  LPZIPUSERFUNCTIONS lpZipUserFunc;
  char *progressChunkSize;
#endif
{
  int ZipReturn = 0;
  int argc;
  char **args;
#if 0
  DWORD ret = 0;
  char msg[MAX_ZIP_ARG_SIZE + 1];
  int i;
  char arg[MAX_ZIP_ARG_SIZE + 1];
  char parsed[MAX_ZIP_COMMAND_STRING_SIZE + 1];
  char parsedcommandline[MAX_ZIP_COMMAND_STRING_SIZE + 1];
  char OrigDir[MAX_ZIP_ARG_SIZE + 1];
#endif

  mesg = stdout;

#if 0
  sprintf(msg, "In ZpZip\ncommandLine = '%s'\ncurrentDir = '%s'\n", commandLine, currentDir);
  MessageBoxA(NULL, msg, "ZpZip",
                MB_OK | MB_ICONINFORMATION);
#endif

  /* copy function pointers to global structure */
  ZipUserFunctions.print = lpZipUserFunc->print;
  ZipUserFunctions.ecomment = lpZipUserFunc->ecomment;
  ZipUserFunctions.acomment = lpZipUserFunc->acomment;
  ZipUserFunctions.password = lpZipUserFunc->password;
  ZipUserFunctions.service = lpZipUserFunc->service;
  ZipUserFunctions.service_no_int64 = lpZipUserFunc->service_no_int64;
  ZipUserFunctions.progress = lpZipUserFunc->progress;
  ZipUserFunctions.error = lpZipUserFunc->error;
  ZipUserFunctions.finish = lpZipUserFunc->finish;

  lpZipUserFunctions = &ZipUserFunctions;

#ifdef ZIPDLL
  /* The DLL does not know the app's current dir, so must be told.  The
     LIB, however, has the same current dir as the caller, so its OK
     for CurrentDir to be NULL on a LIB call. */
  if (currentDir == NULL || strlen(currentDir) == 0)
  {
    return ZpErrorMessage(ZE_PARMS,
           "ZpZip Error:  Empty current directory to ZpZip - aborting");
  }
#endif

  /* used to convert full paths to relative paths */
  ZpZip_root_dir = string_dup(currentDir, "ZpZip_root_dir (ZpZip)", NO_FLUFF);
  ZpZip_root_dirw = NULL;
#ifdef UNICODE_SUPPORT_WIN32
  if (is_utf8_string(currentDir, NULL, NULL, NULL, NULL)) {
    ZpZip_root_dirw = utf8_to_wchar_string(currentDir);
  }
#endif

  if (ZipUserFunctions.progress != NULL) {
    if (progressChunkSize == NULL) {
      return ZpErrorMessage(ZE_PARMS,
             "ZpZip Error:  ProgressReport callback provided but ProgressChunkSize empty");
    }
    progress_chunk_size = ReadNumString(progressChunkSize);
    if (progress_chunk_size < 1024) {
      return ZpErrorMessage(ZE_PARMS,
             "ZpZip Error:  Bad ProgressChunkSize - must be >= 1k");
    }
  }

  /* convert string command line to argv array */
  argc = commandline_to_argv(commandLine, &args);

#if 0
  /* convert argv back to string and pass back to caller to
     verify command line was parsed correctly */
  parsed[0] = '\0';
  if (argc) {
    for (i = 0; i < argc; i++) {
      sprintf(arg, "\"%s\"", argv[i]);
      if (i > 0) {
        strcat(parsed, "  ");
      }
      strcat(parsed, arg);
    }
  }

  strcpy(parsedcommandline, "commandLine as seen by DLL:  ");
  strcat(parsedcommandline, parsed);
  MessageBoxA(NULL, parsedcommandline, "ZpZip",
                MB_OK | MB_ICONINFORMATION);
#endif /* 0 */

#if 0
  ret = GetCurrentDirectory(MAX_ZIP_ARG_SIZE, OrigDir);
  if ( ret == 0 )
  {
    sprintf(msg, "ZpZip:  GetCurrentDirectory failed (%d)", GetLastError());
    return ZpErrorMessage(ZE_PARMS, msg);
  }
  if (ret > MAX_ZIP_ARG_SIZE)
  {
    sprintf(msg, "ZpZip:  GetCurrentDirectory buffer too small; need %d characters\n", ret);
    return ZpErrorMessage(ZE_PARMS, msg);
  }
#endif /* 0 */

#if 0
  printf("ZpZip:  DLL original directory:  '%s'\n", OrigDir);
#endif /* 0 */

#ifdef CHANGE_DIRECTORY

  if (currentDir) {
# ifdef UNICODE_SUPPORT_WIN32
    int dir_utf8 = is_utf8_string(currentDir, NULL, NULL, NULL, NULL);
    wchar_t *dirw;

    if (dir_utf8) {
      /* UTF-8 currentDir */
      dirw = utf8_to_wchar_string(currentDir);
      if (dirw == NULL) {
        ZIPERR(ZE_PARMS, "converting currentDir to wide");
      }
      if (_wchdir(dirw)) {
        sprintf(errbuf, "changing to dir: %S\n  %s", dirw, strerror(errno));
        free(dirw);
        ZIPERR(ZE_PARMS, errbuf);
      }
    }
    else {
      /* local charset CurrentDir */
      if (CHDIR(currentDir)) {
        sprintf(errbuf, "changing to dir: %s\n  %s", currentDir, strerror(errno));
        ZIPERR(ZE_PARMS, errbuf);
      }
    }
# else /* not UNICODE_SUPPORT_WIN32 */
    if (CHDIR(currentDir)) {
      sprintf(errbuf, "changing to dir: %s\n  %s", currentDir, strerror(errno));
      ZIPERR(ZE_PARMS, errbuf);
    }
# endif /* def UNICODE_SUPPORT_WIN32 else */
  }
  
#endif /* def CHANGE_DIRECTORY */

#if 0
    if( !SetCurrentDirectory(currentDir) )
    {
      sprintf(msg, "ZpZip:  SetCurrentDirectory failed (%d)\n", GetLastError());
      return ZpErrorMessage(ZE_PARMS, msg);
    }
  }
#endif /* 0 */

  /* call Zip to do the work */
  ZipReturn = zipmain(argc, args);

#if 0
  sprintf(msg, "Leaving ZpZip\n");
  MessageBoxA(NULL, msg, "ZpZip",
                MB_OK | MB_ICONINFORMATION);
#endif /* 0 */

  if (ZpZip_root_dir) {
    free(ZpZip_root_dir);
    ZpZip_root_dir = NULL;
  }
  if (ZpZip_root_dirw) {
    free(ZpZip_root_dirw);
    ZpZip_root_dirw = NULL;
  }
  if (args) {
    free_argsz(args);
    args = NULL;
  }
  
  return ZipReturn;

} /* ZpZip */


/* ZpZipArgs()
 *
 * This is the same as ZpZip(), but user provides array of args instead of
 * combined command string.
 *
 * args[]            - Same command line args that would be passed to Zip from
 *                     the OS.
 * argc              - Count of args in args[].
 * currentDir        - Dir to set as current dir.  Must be set for DLL.  Can
 *                     be NULL for LIB (if NULL, app's current directory used).
 * lpZipUserFunc     - Structure that caller sets callback addresses in.
 *                     Make sure any unused callback is set to NULL.
 * progressChunkSize - If progress callback is used, this must be set to the
 *                     number of bytes you want to process before getting the
 *                     next progress report (call of progress callback).  This
 *                     is set as a string in the same form as used elsewhere in
 *                     Zip, e.g. 50m for 50 MiB.  This can be NULL if the progress
 *                     callback is not used.
 *
 * Note that, on Windows, CommandLine can contain UTF-8 strings (if Unicode
 * support is enabled), which Zip will detect and convert to wide character
 * Unicode strings for processing.  If your application is dealing with wide
 * character paths and strings, likely this will mean using the standard
 * wide to UTF-8 conversion to get the UTF-8 strings that you would then pass
 * to ZpZip.  On other ports, CommandLine should be in the current character
 * set.  If that character set is UTF-8, then UTF-8 is supported.
 */

#ifndef NO_PROTO
/* The Windows DLL only supports this PROTO version. */
int ZIPEXPENTRY ZpZipArgs(char *args[],
                          int argc,
                          char *currentDir,
                          LPZIPUSERFUNCTIONS lpZipUserFunc,
                          char *progressChunkSize)
#else
/* This is provided mainly for other ports. */
int ZpZipArgs(args, argc, currentDir, lpZipUserFunc, progressChunkSize)
  char *args[];
  int argc;
  char *currentDir;
  LPZIPUSERFUNCTIONS lpZipUserFunc;
  char *progressChunkSize;
#endif
{
  int ZipReturn = 0;
#if 0
  DWORD ret = 0;
  char msg[MAX_ZIP_ARG_SIZE + 1];
  int i;
  char arg[MAX_ZIP_ARG_SIZE + 1];
  char parsed[MAX_ZIP_COMMAND_STRING_SIZE + 1];
  char parsedcommandline[MAX_ZIP_COMMAND_STRING_SIZE + 1];
  char OrigDir[MAX_ZIP_ARG_SIZE + 1];
  int total_args_size = 0;
  char *dialog_message;
  char tempbuf[100];
#endif /* 0 */

  mesg = stdout;

#if 0
  {

    for (i = 0; i < argc; i++) {
      total_args_size += strlen(args[i]) + 10;
    }

    total_args_size += 100 + strlen(currentDir);

    if (total_args_size > MAX_ZIP_COMMAND_STRING_SIZE) {
      printf("Can't display args, too big (%i)\n", total_args_size);
    }
    else {
      sprintf(dialog_message, "ZpZipArgs:\n");
      for (i = 0; i < argc; i++) {
        sprintf(tempbuf, "  arg[%i]: '", i);
        strcat(dialog_message, tempbuf);
        strcat(dialog_message, args[i]);
        strcat(dialog_message, "'\n");
      }
      strcat(dialog_message, "Current Dir:  ");
      strcat(dialog_message, currentDir);
      strcat(dialog_message, "\n");

      MessageBoxA(NULL, dialog_message, "ZpZipArgs",
                    MB_OK | MB_ICONINFORMATION);
    }
  }
#endif /* 0 */

  /* copy function pointers to global structure */
  ZipUserFunctions.print = lpZipUserFunc->print;
  ZipUserFunctions.ecomment = lpZipUserFunc->ecomment;
  ZipUserFunctions.acomment = lpZipUserFunc->acomment;
  ZipUserFunctions.password = lpZipUserFunc->password;
  ZipUserFunctions.service = lpZipUserFunc->service;
  ZipUserFunctions.service_no_int64 = lpZipUserFunc->service_no_int64;
  ZipUserFunctions.progress = lpZipUserFunc->progress;
  ZipUserFunctions.error = lpZipUserFunc->error;
  ZipUserFunctions.finish = lpZipUserFunc->finish;

  lpZipUserFunctions = &ZipUserFunctions;

#ifdef ZIPDLL
  /* The DLL does not know the app's current dir, so must be told.  The
     LIB, however, has the same current dir as the caller, so its OK
     for CurrentDir to be NULL on a LIB call. */
  if (currentDir == NULL || strlen(currentDir) == 0)
  {
    return ZpErrorMessage(ZE_PARMS,
           "ZpZip Error:  Empty current directory to ZpZip - aborting");
  }
#endif

  /* used to convert full paths to relative paths */
  ZpZip_root_dir = string_dup(currentDir, "ZpZip_root_dir (ZpZipArgs)", NO_FLUFF);
  ZpZip_root_dirw = NULL;
#ifdef UNICODE_SUPPORT_WIN32
  if (is_utf8_string(currentDir, NULL, NULL, NULL, NULL)) {
    ZpZip_root_dirw = utf8_to_wchar_string(currentDir);
  }
#endif

  if (ZipUserFunctions.progress != NULL) {
    if (progressChunkSize == NULL) {
      return ZpErrorMessage(ZE_PARMS,
             "ZpZip Error:  ProgressReport callback provided but ProgressChunkSize empty");
    }
    progress_chunk_size = ReadNumString(progressChunkSize);
    if (progress_chunk_size < 1024) {
      return ZpErrorMessage(ZE_PARMS,
             "ZpZip Error:  Bad ProgressChunkSize - must be >= 1k");
    }
  }

  /* convert string command line to argv array */
  // argc = commandline_to_argv(CommandLine, &argv);

#if 0
  /* convert argv back to string and pass back to caller to
     see representative command line */
  parsed[0] = '\0';
  if (argc) {
    for (i = 0; i < argc; i++) {
      sprintf(arg, "\"%s\"", argv[i]);
      if (i > 0) {
        strcat(parsed, "  ");
      }
      strcat(parsed, arg);
    }
  }

  strcpy(parsedcommandline, "CommandLine as seen by DLL:  ");
  strcat(parsedcommandline, parsed);
  MessageBoxA(NULL, parsedcommandline, "ZpZipArgs",
                MB_OK | MB_ICONINFORMATION);
#endif

#if 0
  ret = GetCurrentDirectory(MAX_ZIP_ARG_SIZE, OrigDir);
  if ( ret == 0 )
  {
    sprintf(msg, "ZpZip:  GetCurrentDirectory failed (%d)", GetLastError());
    return ZpErrorMessage(ZE_PARMS, msg);
  }
  if (ret > MAX_ZIP_ARG_SIZE)
  {
    sprintf(msg, "ZpZip:  GetCurrentDirectory buffer too small; need %d characters\n", ret);
    return ZpErrorMessage(ZE_PARMS, msg);
  }
#endif

#if 0
  printf("ZpZip:  DLL original directory:  '%s'\n", OrigDir);
#endif

#ifdef CHANGE_DIRECTORY

  if (currentDir) {
# ifdef UNICODE_SUPPORT_WIN32
    int dir_utf8 = is_utf8_string(currentDir, NULL, NULL, NULL, NULL);
    wchar_t *dirw;

    if (dir_utf8) {
      /* UTF-8 currentDir */
      dirw = utf8_to_wchar_string(currentDir);
      if (dirw == NULL) {
        ZIPERR(ZE_PARMS, "converting currentDir to wide");
      }
      if (_wchdir(dirw)) {
        sprintf(errbuf, "changing to dir: %S\n  %s", dirw, strerror(errno));
        free(dirw);
        ZIPERR(ZE_PARMS, errbuf);
      }
    }
    else {
      /* local charset currentDir */
      if (CHDIR(currentDir)) {
        sprintf(errbuf, "changing to dir: %s\n  %s", currentDir, strerror(errno));
        ZIPERR(ZE_PARMS, errbuf);
      }
    }
# else /* not UNICODE_SUPPORT_WIN32 */
    if (CHDIR(currentDir)) {
      sprintf(errbuf, "changing to dir: %s\n  %s", currentDir, strerror(errno));
      ZIPERR(ZE_PARMS, errbuf);
    }
# endif /* def UNICODE_SUPPORT_WIN32 else */
  }
  
#endif /* def CHANGE_DIRECTORY */

#if 0
    if( !SetCurrentDirectory(currentDir) )
    {
      sprintf(msg, "ZpZip:  SetCurrentDirectory failed (%d)\n", GetLastError());
      return ZpErrorMessage(ZE_PARMS, msg);
    }
  }
#endif /* 0 */

  /* call Zip to do the work */
  ZipReturn = zipmain(argc, args);

#if 0
  sprintf(msg, "Leaving ZpZip\n");
  MessageBoxA(NULL, msg, "ZpZip",
                MB_OK | MB_ICONINFORMATION);
#endif /* 0 */

  if (ZpZip_root_dir) {
    free(ZpZip_root_dir);
    ZpZip_root_dir = NULL;
  }
  if (ZpZip_root_dirw) {
    free(ZpZip_root_dirw);
    ZpZip_root_dirw = NULL;
  }

  return ZipReturn;

} /* ZpZipArgs */



/* ZpVersion()
 *
 * The user calls this to get version information.  This should be done before
 * calling ZpZip() or ZpZipArgs() to make sure the current DLL is compatible.
 */

#define FEATURE_LIST_SIZE 1000   /* Make sure this is big enough to hold
                                    everything.  If you change it here,
                                    may need to update example apps. */

#ifndef NO_PROTO
void ZIPEXPENTRY ZpVersion(ZpVer far *p)   /* should be pointer to const struct */
#else
void ZIPEXPENTRY ZpVersion(p)   /* should be pointer to const struct */
  ZpVer far * p;
#endif
{
    char tempstring[100];       /* size of biggest entry */
    char featurelist[FEATURE_LIST_SIZE + 1];     

    p->structlen = ZPVER_LEN;

    strcpy(tempstring, "");     /* avoid compiler not used warning */

/* Currently flag = bit 0: is_beta,   bit 1: uses_zlib,   bit 2: Zip64 */
#if BETA == 1
    p->flag = 1;
#else
    p->flag = 0;
#endif
    strcpy(p->BetaLevel, Z_BETALEVEL);
    strcpy(p->Version, VERSION);
    strcpy(p->RevDate, REVDATE);
    strcpy(p->RevYMD, REVYMD);
#ifdef ZLIB_VERSION
    strcpy(p->zlib_Version, ZLIB_VERSION);
    p->flag |= 2;
#else
    p->zlib_Version[0] = '\0';
#endif

#ifdef IZ_CRYPT_ANY
    p->fEncryption = 1;
#else
    p->fEncryption = 0;
#endif

#ifdef ZIP64_SUPPORT
    p->flag |= 4; /* Flag that ZIP64 was compiled in. */
#endif

    p->zip.major = Z_MAJORVER;
    p->zip.minor = Z_MINORVER;
    p->zip.patchlevel = Z_PATCHLEVEL;

#ifdef OS2
    p->os2dll.major = D2_MAJORVER;
    p->os2dll.minor = D2_MINORVER;
    p->os2dll.patchlevel = D2_PATCHLEVEL;
#else
    p->os2dll.major = 0;
    p->os2dll.minor = 0;
    p->os2dll.patchlevel = 0;
#endif
#ifdef ZIP_DLL_LIB
    p->libdll_interface.major = LD_MAJORVER;
    p->libdll_interface.minor = LD_MINORVER;
    p->libdll_interface.patchlevel = LD_PATCHLEVEL;
#endif
    p->opt_struct_size = sizeof(ZPOPT);

    /* feature list */
    /* all features start and end with a semicolon for easy parsing */

    /* KEEP THIS LIST UP TO DATE */

    strcpy(featurelist, ";");
#ifdef ASM_CRC
    strcat(featurelist, "asm_crc;");
#endif
#ifdef ASMV
    strcat(featurelist, "asmv;");
#endif
#ifdef BACKUP_SUPPORT
    strcat(featurelist, "backup;");
#endif
#if defined(DEBUG)
    strcat(featurelist, "debug_trace;");
#endif
#if defined(_DEBUG)
    strcat(featurelist, "_debug;");
#endif
#ifdef USE_EF_UT_TIME
    strcat(featurelist, "ef_ut_time;");
#endif
#ifdef NTSD_EAS
    strcat(featurelist, "ntsd_eas;");
#endif
#if defined(WIN32) && defined(NO_W32TIMES_IZFIX)
    strcat(featurelist, "no_w32times_izfix;");
#endif
#ifdef VMS
# ifdef VMS_IM_EXTRA
    strcat(featurelist, "vms_im_extra;");
# endif
# ifdef VMS_PK_EXTRA
    strcat(featurelist, "vms_pk_extra;");
# endif
#endif /* VMS */
#ifdef WILD_STOP_AT_DIR
    strcat(featurelist, "wild_stop_at_dir;");
#endif
#ifdef WIN32_OEM
    strcat(featurelist, "win32_oem;");
#endif
    strcpy(tempstring, "compmethods:store,deflate,cd_only");
#ifdef BZIP2_SUPPORT
    strcat(tempstring, ",bzip2");
#endif
#ifdef LZMA_SUPPORT
    strcat(tempstring, ",lzma");
#endif
#ifdef PPMD_SUPPORT
    strcat(tempstring, ",ppmd");
#endif
    strcat(tempstring, ";");
    strcat(featurelist, tempstring);
#ifdef LARGE_FILE_SUPPORT
    strcat(featurelist, "large_file;");
#endif
#ifdef ZIP64_SUPPORT
    strcat(featurelist, "zip64;");
#endif
#ifdef UNICODE_SUPPORT
    strcat(featurelist, "unicode;");
#endif
#ifdef UNIX
    strcat(featurelist, "store_unix_uids_gids;");
# ifdef UIDGID_NOT_16BIT
    strcat(featurelist, "uidgid_not_16bit;");
# else
    strcat(featurelist, "uidgid_16bit;");
# endif
#endif
#ifdef IZ_CRYPT_TRAD
    strcat(featurelist, "crypt;");
#endif
#ifdef IZ_CRYPT_AES_WG
    strcat(featurelist, "crypt_aes_wg;");
#endif
#ifdef USE_ZLIB
    strcat(featurelist, "zlib;");
    sprintf(tempstring, "zlib_version:%s,%s;", ZLIB_VERSION, zlibVersion());
    strcat(featurelist, tempstring);
#endif
#ifdef WINDOWS_LONG_PATHS
    strcat(featurelist, "windows_long_paths;");
#endif
#ifdef WINDLL
    strcat(featurelist, "windll;");
#endif
#ifdef ZIPLIB
    strcat(featurelist, "ziplib;");
#endif
#if defined(ZIPDLL) && !defined(ZIPLIB)
    strcat(featurelist, "zipdll;");
#endif
#ifdef SYMLINKS
    strcat(featurelist, "symlinks;");
#endif

    strcpy(p->szFeatures, featurelist);
}


/* windll/windll.[ch] stuff for non-Windows systems. */
/* comment() and other functions for WinDLL in windll.c. */

LPSTR szCommentBuf;
HANDLE hStr;

#if 0

void comment(unsigned int comlen)
{
    unsigned int i;
    long maxcomlen = 32767;
    long newcomlen;

    /* The entire record can't be bigger than 65535L so the comment
       needs to be smaller than that.  Changed 65534L to 32767L.  EG */

    if (comlen > 32766L)
        comlen = (unsigned int) 32766L;
    hStr = GlobalAlloc( GPTR, (DWORD)32767L);
    if (!hStr)
    {
        hStr = GlobalAlloc( GPTR, (DWORD) 2);
        szCommentBuf = GlobalLock(hStr);
        szCommentBuf[0] = '\0';
        return;
    }
    if (comlen)
    {
        for (i = 0; i < comlen; i++)
        szCommentBuf[i] = zcomment[i];
        szCommentBuf[comlen] = '\0';
    }
    else
        szCommentBuf[0] = '\0';

    free(zcomment);
    zcomment = malloc(1);
    *zcomment = 0;
    if (lpZipUserFunctions->acomment)
    {
        lpZipUserFunctions->acomment(szCommentBuf, maxcomlen, &newcomlen);
        if (newcomlen > maxcomlen)
          newcomlen = maxcomlen;
        szCommentBuf[newcomlen] = '\0';
    }
    return;
}
#endif /* NO_ZPARCHIVE */


#ifdef ZIP_DLL_LIB

/* The entire record must be less than 65535, so the comment length needs to
   be kept well below that.  Changed 65534L to 32766L.  EG */

/* This moved to zip.h */
# if 0
#  define MAX_COM_LEN (32766L)
# endif


/* acomment() - Get the archive comment. */
#ifndef NO_PROTO
void acomment(long comlen)
#else
void acomment(comlen)
  long comlen;
#endif
{
  long ret;
  long maxcomlen = MAX_COM_LEN;
  long newcomlen = 0;

  if (comlen > MAX_COM_LEN)
     comlen = MAX_COM_LEN;

  if (zcomment == NULL) {
    /* create zero length string to pass to callback */
    if ((zcomment = (char *)malloc(1)) == NULL) {
      ZIPERR(ZE_MEM, "getting comment from callback (1)");
    }
    *zcomment = '\0';
    comlen = 0;
  } else {
    /* make sure zcomment is NUL terminated */
    zcomment[comlen] = '\0';
  }

  if (lpZipUserFunctions->acomment) {
    char newcomment[MAX_COM_LEN + 1];
  
    if (maxcomlen > MAX_COM_LEN)
      maxcomlen = MAX_COM_LEN;

    ret = lpZipUserFunctions->acomment(zcomment, maxcomlen,
                                       &newcomlen, newcomment);

    if (ret == 0)
      return;

    if (newcomlen > maxcomlen)
      newcomlen = maxcomlen;
    newcomment[newcomlen] = '\0';

    if (zcomment)
      free(zcomment);
    if ((zcomment = (char *)malloc((newcomlen + 1) * sizeof(char))) == NULL) {
      ZIPERR(ZE_MEM, "getting comment from callback (2)");
    }

    zcomlen = (ush)newcomlen;
    strcpy(zcomment, newcomment);
  }
  else
  {
    ZIPERR(ZE_PARMS, "-z used to request archive comment but no callback");
  }
}


/* ecomment() - Get a comment for an entry. */
#ifndef NO_PROTO
void ecomment(struct zlist far *z)
#else
void ecomment(z)
  struct zlist far *z;
#endif
{
  long maxcomlen = MAX_COM_LEN;
  long newcomlen = 0;

  /* The entire record must be less than 65535, so the comment length needs to
   be kept well below that.  Current comment limit 32765. */

  if (z->com == 0 || z->comment == NULL) {
    /* create zero length string to pass to callback */
    if ((z->comment = (char *)malloc(1)) == NULL) {
      ZIPERR(ZE_MEM, "getting comment from callback (1)");
    }
    z->com = 0;
    z->comment[0] = '\0';
  } else {
    /* make sure entry comment is NUL terminated */
    z->comment[z->com] = '\0';
  }

  if (lpZipUserFunctions->ecomment) {
    long ret;
    char newcomment[MAX_COM_LEN + 1];
    char *oname;
    char *uname;

    oname = z->oname;
# ifdef UNICODE_SUPPORT
    uname = z->uname;
# else
    uname = NULL;
# endif

    if (maxcomlen > MAX_COM_LEN)
      maxcomlen = MAX_COM_LEN;

    ret = lpZipUserFunctions->ecomment(z->comment,
                                       oname,
                                       uname,
                                       maxcomlen,
                                       &newcomlen,
                                       newcomment);

    if (ret == 0)
      return;
    
    if (newcomlen > maxcomlen)
      newcomlen = maxcomlen;
    newcomment[newcomlen] = '\0';

    if (z->com && z->comment)
      free(z->comment);
    if ((z->comment = (char *)malloc((newcomlen + 1) * sizeof(char))) == NULL) {
      ZIPERR(ZE_MEM, "getting comment from callback (2)");
    }

    z->com = (ush)newcomlen;
    strcpy(z->comment, newcomment);
  }
  else
  {
    ZIPERR(ZE_PARMS, "-c used to request entry comment but no callback");
  }
}
#endif


/* ZpStringCopy()
 *
 * Copy the source string to the destination.  Destination must already
 * be allocated.
 *
 * This function does not check if the space allocated in the
 * destination string is sufficient for holding the source string.
 *
 * This function is provided to allow returning string arguments in
 * Visual Studio 2010 Visual Basic.  See vb10 example for how this is used.
 *
 * Returns the number of bytes in the destination.
 */

#ifndef NO_PROTO
int ZIPEXPENTRY ZpStringCopy(char *deststring, char *sourcestring, int maxlength)
#else
int ZIPEXPENTRY ZpStringCopy(deststring, sourcestring, maxlength)
  char *deststring;
  char *sourcestring;
  int maxlength;
#endif
{
  int length = 0;

  if (deststring == NULL || sourcestring == NULL)
    return -1;

  length = (int)strlen(sourcestring);
  if (length > maxlength - 1)
    length = maxlength - 1;

  strncpy(deststring, sourcestring, length);
  deststring[length] = '\0';

  return (int)strlen(deststring);
}


/* commandline_to_argv()
 *
 * Parse a commandline string into an argv array that can then be processed
 * by Zip.  Just break up the string into arguments at whitespace.  Respect
 * quotes and escapes.
 *
 * This version is tuned for the Windows shell.  Only double quotes (" ") can enclose
 * whitespace (not single quotes (' ')).  Escapes (like \t and \n) are not used on
 * command line.  Assume the character encoding does not include whitespace or quotes
 * in multi-byte sequences (UTF-8 meets this).
 *
 * Embedded double quotes are kept.  Start and end double quotes are removed.
 *
 * Input is a string that is the command line to parse.  Currently this is assumed
 * to be a byte string, though it can be a multibyte string.
 *
 * The Zip DLL/LIB currently does not handle wide character command lines.  All input
 * to the DLL/LIB should be UTF-8.
 *
 * NULL terminated argv[] returned as argument.
 * Returns argc.  If empty or NULL commandline, returns 0.
 *
 * EG 2017-07-09
 */
#ifndef NO_PROTO
int commandline_to_argv(char *commandline, char ***pargv)
#else
int commandline_to_argv(commandline, pargv)
  char *commandline;
  char ***pargv;
#endif
{
  int i;
  int argc = 0;
  char *arg;
  char *c;
  unsigned char uc;
  char *a;
  char *argstart;
  char *argend;
  int argsize;
  char **args = NULL;
  char *arg0 = "zip";
  int in_quote = 0;

  /* first arg */
  /* allocate space for args[0] and initial termination (args[1]) */
  if ((args = (char **)malloc(2 * sizeof(char *))) == NULL) {
    ZIPERR(ZE_MEM, "parsing command line (1)");
  }
  /* set up args[0] */
  if ((arg = (char *)malloc((strlen(arg0) + 1) * sizeof(char))) == NULL) {
    ZIPERR(ZE_MEM, "parsing command line (2)");
  }
  strcpy(arg, arg0);
  args[0] = arg;

  argc = 1;
  args[argc] = NULL;

  if (commandline == NULL || commandline[0] == '\0') {
    *pargv = args;
 
    return argc;
  }
  
  c = commandline;
  

  while (1) {
    /* skip any leading whitespace */
    while (*c && isspace((uc = *c))) {
      c++;
    }
    argstart = c;

    if (*c == '\0')
      break;

    /* find end of arg */
    while (*c && (!isspace((uc = *c)) || in_quote)) {
      if (*c == '\"') {
        if (in_quote)
          in_quote = 0;
        else
          in_quote = 1;
      }
      c++;
    }

    argsize = (int)(c - argstart);
    if (argsize == 0)
      break;

    /* add arg to args[] */
    argc++;
  
    argend = c;

    /* remove end quotes */
    if ((*argstart == '\"') && (c > (argstart + 1) && *(c - 1) == '\"')) {
      argstart++;
      argend--;
      argsize -= 2;
    }

    /* additional arg */
    if ((args = (char **)realloc(args, (argc + 1) * sizeof(char *))) == NULL) {
      ZIPERR(ZE_MEM, "parsing command line (3)");
    }

    if ((arg = (char *)malloc((argsize + 1) * sizeof(char))) == NULL) {
      ZIPERR(ZE_MEM, "parsing command line (4)");
    }
    for (a = argstart, i = 0; a < argend; a++, i++) {
      arg[i] = *a;
    }
    arg[i] = '\0';
    args[argc - 1] = arg;
    
    args[argc] = NULL;
  } /* while */

  *pargv = args;

  return argc;
}


/* argv_to_commandline
 *
 * Combine args in argv[] into a single string, excluding argv[0].
 * If argc > 0, include up to argc arguments or until NULL arg
 * found.  If argc = 0, include all args up to NULL arg.
 *
 * Return commandline string or NULL if error.
 *
 * 2014-07-12 EG
 */
#ifndef NO_PROTO
char * ZIPEXPENTRY ZpArgvToCommandlineString(int argc, char *argv[])
#else
char * ZIPEXPENTRY ZpArgvToCommandlineString(argc, argv)
  int argc;
  char *argv[];
#endif
{
  int i;
  char *commandline = NULL;
  int total_size = 0;
  int j;
  int has_space;
  char c;
  unsigned char uc;

  /* add up total length needed */
  for (i = 1; (!argc || i < argc) && argv[i]; i++)
  {
    /* add size of arg and 2 for double quotes */
    total_size += (int)(strlen(argv[i]) + 2) * sizeof(char);
    if (i > 1)
    {
      /* add 1 for space between args */
      total_size += 1 * sizeof(char);
    }
  }
  /* add 1 for string terminator */
  total_size += 1 * sizeof(char);

  /* allocate space for string */
  if ((commandline = (char *)malloc(total_size)) == NULL) {
    return NULL;
  }

  /* build command line string */
  commandline[0] = '\0';
  for (i = 1; (!argc || i < argc) && argv[i]; i++)
  {
    if (i > 1)
    {
      strcat(commandline, " ");
    }
    /* only quote if has space */
    has_space = 0;
    for (j = 0; (c = argv[i][j]); j++)
    {
      if (isspace((uc = c)))
        has_space = 1;
    }
    if (has_space)
      strcat(commandline, "\"");
    strcat(commandline, argv[i]);
    if (has_space)
      strcat(commandline, "\"");
  }

  return commandline;
}
