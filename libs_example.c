/*
  Copyright (c) 1990-2019 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  libs_example.c

  Example main program illustrating how to use the zip and unzip libs in a
  single program.

  This program needs both the Zip static lib (libizzip.a) and the UnZip static
  lib (libizunzip.a).  These must both be in the current directory for
  libs_example to build.  See izzip_example.c for details on the Zip lib.  See
  the UnZip source kit for details on building the UnZip static lib.
  
  unzip.h from the UnZip source kit must also be copied to the current
  directory.  A modified version of unzip.h may be included in this kit.
  Changes to unzip.h were needed and, as of this writing, the changes have
  not been published in the UnZip kit.
  
  Basic build procedure, Unix:

    cc libs_example.c -DZIPLIB -IZip_Source_Dir -o libs_example \
     -LZip_Object_Dir -lizzip

  (On Unix, the default Zip_Object_Dir is the same as the Zip_Source_Dir.)

  For example:

    cc libs_example.c -o libs_example libizzip.a libizunzip.a -DZIPLIB

  Basic build procedure, VMS (VMS has not been tested):

    cc libs_example.c /define = ZIPLIB=1 -
     /include = (Zip_Source_Dir, Zip_Source_Vms_Dir)
    link libs_example.obj, Zip_Object_Dir:libizzip.olb /library

  Now that bzip2 is included in the libizzip combined library build, no
  additional library linkages should be required.  Be sure the build of
  libizzip.a includes any compression and encryption capabilities needed.
  If an external/system library is used to build libizzip, it may be
  necessary to link explicitly to that external library when building
  izzip_example.

  On VMS, a link options file (LIB_IZZIP.OPT) is generated along with
  the object library (in the same directory), and it can be used to
  simplify the LINK command.  (LIB_IZZIP.OPT contains comments showing
  how to define the logical names which it uses.)

    define LIB_IZZIP Dir        ! See comments in LIB_IZZIP.OPT.
    define LIB_other Dir        ! See comments in LIB_IZZIP.OPT.
    link izzip_example.obj, Zip_Object_Dir:lib_izzip.opt /options

  See ReadLibDll.txt for more on the updated Zip 3.1 LIB/DLL interface.

  This example creates an example_out directory in the current directory,
  zips up files given as the target and puts that archive in the current
  directory, does a cd to the example_out directory and unzips the archive
  in that directory.  The arguments are a zip command line to create the
  archive.  The first argument must be a simple name for the archive.
  
  A typical test command line might be:
    libs_example testarchive test.txt -ecz
  which would add/update test.txt (you may need to create this input file)
  in testarchive.zip, using the password callback to get the password, the
  entry comment callback to get the entry comment for test.txt, and the
  archive comment callback to get the archive comment.  The file test.txt
  is then extracted using UnZip in the example_out directory.

  This example enables most of the zip callbacks.  Which ones you use is up
  to you.  However, be sure to set at least the Print or the Error callback
  so you see any errors that occur.
  
  Note that API_FILESIZE_T is usually 64 bits (unsigned long long), but
  is 32 bits (unsigned long) on VAX VMS.

  Last updated 2018-12-30.

  
  ---------------------------------------------------------------------------*/

#include "api.h"                /* Zip specifics. */

#include <ctype.h>
#include <errno.h>

#include "unzip.h"


/* ------------- */
/* zip callbacks */

long MyZipPrint(char *buf,               /* in - what was printed */
                unsigned long size);     /* in - length of buf */

long MyZipEComment(char *oldcomment,     /* in - old comment */
                   char *filename,       /* in - local char file name */
                   char *ufilename,      /* in - UTF-8 file name */
                   long maxcommentsize,  /* in - max new comment size */
                   long *newcommentsize, /* out - size of new comment */
                   char *newcomment);    /* out - new comment */

long MyZipAComment(char *oldcomment,     /* in - old comment */
                   long maxcommentsize,  /* in - max new comment size */
                   long *newcommentsize, /* out - new comment size */
                   char *newcomment);    /* out - new comment */

long MyZipPassword(long bufsize,         /* in - max password size */
                   char *prompt,         /* in - prompt ("enter"/"verify") */
                   char *password);      /* out - password */

long MyZipService(char *file_name,
                  char *unicode_file_name,
                  char *uncompressed_size_string,
                  char *compressed_size_string,
                  API_FILESIZE_T uncompressed_size,
                  API_FILESIZE_T compressed_size,
                  char *action,
                  char *method,
                  char *info,
                  long percent);
                  
long MyZipProgress(char *file_name,
                    char *unicode_file_name,
                    long percent_entry_done_x_100,
                    long percent_all_done_x_100,
                    API_FILESIZE_T uncompressed_size,
                    char *uncompressed_size_string,
                    char *action,
                    char *method,
                    char *info);
                    
void MyZipError(char *errstring);
                  
long MyZipFinish(char *final_uncompressed_size_string,
                  char *final_compressed_size_string,
                  API_FILESIZE_T final_uncompressed_size,
                  API_FILESIZE_T final_compressed_size,
                  long final_percent);

/* ----------- */
/* Zip version */

int GetZipVersionInfo(unsigned char *maj,
                      unsigned char *min,
                      unsigned char *patch);

/* ==================================== */

/* --------------- */
/* UnZip callbacks */

int MyUzpPassword( zvoid *pG,            /* Ignore (globals pointer). */
                          int *rcnt,            /* Retry count. */
                          char *pwbuf,          /* Password buffer. */
                          int size,             /* Password buffer size. */
                          ZCONST char *zfn,     /* Archive name. */
                          ZCONST char *efn);    /* Archive member name. */
                          
/* ------------- */
/* UnZip version */

int GetUnZipVersionInfo(unsigned char *umaj,
                        unsigned char *umin,
                        unsigned char *upatch);

                   
/****************************************************************************

    Main()

    Returns 90 + something if our error, else returns what Zip or UnZip
    returns.

****************************************************************************/
int main(int argc, char **argv)
{
    int ret;
#ifdef __VMS
    int vsts;
#endif
    int r;
    
    /* ======================== */
    /* Zip variables */
    
    char *commandline = NULL;

    /* The API first does a cd to currentdir.  This becomes the root
       directory Zip starts in.  If -cd is also specified, Zip will then
       cd to that directory once Zip starts up.
       
       If currentdir is a full path, then input paths starting with
       this path will be updated to be relative to this.  This
       was done by ZpArchive and is restored here by popular demand. */
    char *currentdir = ".";

    /* As seen in the output, the Progress callback gets called every
       progress_size bytes, which an application can use to update some
       progress indicator.  The progress callback can also be used to
       terminate the zip operation.  (See the API docs for more on this.)
       Typically progress_size is something like 100m, but is set to 1m
       here to demonstrate the Progress callback with small files. */
    char *progress_size = "1m";

    /* Zip version.  These will be updated. */
    unsigned char zmaj = 0;
    unsigned char zmin = 0;
    unsigned char zpatch = 0;

  /* Callback functions structure. */
    ZIPUSERFUNCTIONS ZipUserFunctions;

    /* ======================== */
    /* UnZip variables */
    
    UzpCB unzip_user_functions;       /* User-supplied call-back functions. */
    int unzip_argc;
    char *unzip_argv[10];
    char *zipfile;

    /* UnZip version.  These will be updated. */
    unsigned char umaj = 0;
    unsigned char umin = 0;
    unsigned char upatch = 0;

    /* Populate the user-supplied call-back function structure.
     *
     * See unzip.h for details of the UzpCB structure, and for
     * prototypes of the various call-back functions.  The only
     * call-back function supplied here is the one which returns an
     * encryption password.
     */
    unzip_user_functions.structlen = sizeof(unzip_user_functions);
    unzip_user_functions.msgfn = NULL;
    unzip_user_functions.inputfn = NULL;
    unzip_user_functions.pausefn = NULL;
    unzip_user_functions.passwdfn = MyUzpPassword;
    unzip_user_functions.statrepfn = NULL;
    
    /* ======================== */
    
    if (argc < 2)
    {
      printf("usage: libs_example  zipfile_name  zip_command_arguments\n");
      printf("\n");
      printf("The first argument must be a simple archive name that will be the\n");
      printf("name of the created archive in the current directory.\n");
      return 0;           /* Exits if no arguments */
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "libs_example\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Getting Zip version information:\n");
    ret = GetZipVersionInfo(&zmaj, &zmin, &zpatch);
    
    fprintf(stderr, "\n");
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "Getting UnZip version information:\n");
    ret = GetUnZipVersionInfo(&umaj, &umin, &upatch);
    
    fprintf(stderr, "\n");
    fprintf(stderr, "-----------\n");
    fprintf(stderr, "Checking Zip lib version.\n");
    
    /* We need at least version 3.1 of the LIB for this. */
    fprintf(stderr, "Zip LIB version is:  %d.%d.%d\n", zmaj, zmin, zpatch);
    if (zmaj != 3 && zmin != 1) {
      fprintf(stderr, "This example program requires Zip version 3.1.x\n");
      return 90;
    }
    fprintf(stderr, "Zip version compatible with this example program.\n");
    fprintf(stderr, "\n");

    /* The Zip LIB now provides a (simple) command line builder. */
    commandline = ZpArgvToCommandlineString(0, argv);
    if (commandline == NULL)
    {
      fprintf(stderr, "could not allocate commandline\n");
      return 91;
    }

    fprintf(stderr, "-----------\n");
    fprintf(stderr, "zip command line arguments:\n");
    fprintf(stderr, "'%s'\n", commandline);
    fprintf(stderr, "\n");

    lpZipUserFunctions = &ZipUserFunctions;

    /* DON'T CHANGE THIS.
     * Any unused callbacks must be set to NULL.  Leave this
     * block as is and add functions as needed below.  Do not use
     * split and avoid using service_no_int64 (less functionality
     * than service).
     */
    ZipUserFunctions.print = NULL;        /* gets most output to stdout */
    ZipUserFunctions.ecomment = NULL;     /* called to set entry comment */
    ZipUserFunctions.acomment = NULL;     /* called to set archive comment */
    ZipUserFunctions.password = NULL;     /* called to get password */
    ZipUserFunctions.split = NULL;        /* NOT IMPLEMENTED - DON'T USE */
    ZipUserFunctions.service = NULL;      /* called each entry */
    ZipUserFunctions.service_no_int64 = NULL; /* 32-bit version of service */
    ZipUserFunctions.progress = NULL;     /* provides entry progress */
    ZipUserFunctions.error = NULL;        /* called when warning/error */
    ZipUserFunctions.finish = NULL;       /* called when zip is finished */

    /* CHANGE THIS.  Set callbacks for the functions we're using.  Leave
       NULL the ones we aren't. */
    ZipUserFunctions.print        = MyZipPrint;
    ZipUserFunctions.ecomment     = MyZipEComment;
    ZipUserFunctions.acomment     = MyZipAComment;
    ZipUserFunctions.password     = MyZipPassword;
    ZipUserFunctions.service      = MyZipService;
    ZipUserFunctions.progress     = MyZipProgress;
    ZipUserFunctions.error        = MyZipError;
    ZipUserFunctions.finish       = MyZipFinish;


    /* We call Zip using ZpZipArgs() to zip up the target files into an archive.
       We then create the example_out directory if it doesn't exist.  Then we
       call UnZip to list the files and extract them into example_out.  There
       is a limit on argument size (currently 8192 bytes).  See izzip_example.c
       for an example using ZpZip() and a command line string instead. */
     
    /* Check that the first argument is a simple archive name (no path). */
    if (strchr(argv[1], '/') || strchr(argv[1], '\\') || strchr(argv[1], ':') ||
        argv[1][0] == '-') {
      fprintf(stderr, 
        "First argument must be archive name (not an option) and must not include\n");
      fprintf(stderr, 
        "a path (contain '/', '\\' or ':').");
      return 92;
    }

    /* Using ZpZipArgs */
    
    fprintf(stderr, "Calling Zip using ZpZipArgs ...\n");
    fprintf(stderr, "-----------------------------------------\n");

    ret = ZpZipArgs(argv, argc, currentdir, lpZipUserFunctions, progress_size);

    fprintf(stderr, "-----------------------------------------\n");

    fprintf(stderr, "  Zip returned = %d (%%x%08x).\n", ret, ret);
#ifdef __VMS
    vsts = vms_status(ret);
    fprintf( stderr, "  VMS sts = %d (%%x%08x).\n", vsts, vsts);
#endif

    fprintf(stderr, "\n");
    
    if (ret != 0) {
      fprintf(stderr, "Zip returned error %d - aborting\n", ret);
      return ret;
    }
    
    fprintf(stderr, "Creating example_out directory.\n");
    
    r = mkdir("example_out", 0777);
    if (r == -1) {
      fprintf(stderr, "mkdir returned %d (%s)\n", errno, strerror(errno));
      if (errno != 17) {  /* 17 = file exists */
        return 910;
      }
    }
    
    fprintf(stderr, "\n");
    fprintf(stderr, "Building args for UnZip\n");
    
    unzip_argv[0] = string_dup("unzip", "unzip arg 0", NO_FLUFF);
    unzip_argv[1] = string_dup(argv[1], "unzip arg 1", NO_FLUFF);
    unzip_argv[2] = string_dup("-l", "unzip arg 2", NO_FLUFF);
    unzip_argv[3] = NULL;
    unzip_argc = 3;
        
    fprintf(stderr, "\n");
    fprintf(stderr, "Calling UnZip to list files ...\n");
    fprintf(stderr, "-----------------------------------------\n");
    ret = UzpMainI( unzip_argc, unzip_argv, &unzip_user_functions);
    fprintf(stderr, "-----------------------------------------\n");

    free(unzip_argv[2]);
    unzip_argv[2] = string_dup("-dexample_out", "unzip arg 2", NO_FLUFF);

    fprintf(stderr, "\n");
    fprintf(stderr, "Calling UnZip to extract files to example_out ...\n");
    fprintf(stderr, "-----------------------------------------\n");
    ret = UzpMainI( unzip_argc, unzip_argv, &unzip_user_functions);
    fprintf(stderr, "-----------------------------------------\n");

    free(unzip_argv[0]);
    free(unzip_argv[1]);
    free(unzip_argv[2]);
    
    return ret;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipEComment(): Entry comment call-back function.
 *
 * Maximum comment length: 32765.  (The actual maximum length is dependent
 * on various factors, but this should be safe.  zip.h defines MAX_COM_LEN
 * for this.)
 *
 * New comment must fit in provided buffer newcomment, which is size
 * maxcommentsize.
 *
 * Return 0 on fail (don't update comment), 1 on success.
 *
 * This callback is called for each entry only if option -c is used.
 *
 * Update this function to meet the needs of your application.
 */

long MyZipEComment(char *oldcomment,
                   char *filename,
                   char *ufilename,
                   long maxcommentsize,
                   long *newcommentsize,
                   char *newcomment)
{
    int oldcommentsize = 0;
    char *comment = NULL;
    char newcommentstring[MAX_COM_LEN];

    if (oldcomment)
      oldcommentsize = strlen(oldcomment);

    fprintf(stderr, "\n");
    fprintf(stderr, ". EComment callback:\n");
    fprintf(stderr, ".   old comment for %s (%d bytes):\n",
            filename, oldcommentsize);
    if (oldcomment)
      fprintf(stderr, ".     \"%s\"\n", oldcomment);
    else
      fprintf(stderr, ".     \"\"\n");

    /* here is where you would update the comment */

    fprintf(stderr, ". - Enter the new comment for %s (hit ENTER to keep):\n",
            filename);
    fprintf(stderr, ".   Entry comment: ");
    fflush(stderr);

    comment = fgets(newcommentstring, MAX_COM_LEN, stdin);
    if (comment = NULL) {
      return 0;
    }

    *newcommentsize = strlen(newcommentstring);

    if (*newcommentsize == 0 ||
        (*newcommentsize == 1 && newcommentstring[0] == '\n')) {
      fprintf(stderr, ".   keeping: %s\n", oldcomment);
      return 0;
    }

    if (*newcommentsize > maxcommentsize - 1)
      *newcommentsize = maxcommentsize - 1;

    strncpy(newcomment, newcommentstring, *newcommentsize);
    newcomment[*newcommentsize] = '\0';

    if (newcomment[*newcommentsize - 1] == '\n')
      newcomment[*newcommentsize - 1] = '\0';

    fprintf(stderr, ".   new comment for %s (%d bytes):\n",
            filename, *newcommentsize);
    fprintf(stderr, ".     \"%s\"\n",
            newcomment);
    *newcommentsize = strlen(newcomment);

    return 1;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipAComment(): Archive comment call-back function.
 *
 * Maximum comment length: 32765.  (The actual maximum length is dependent
 * on various factors, but this should be safe.  zip.h defines MAX_COM_LEN
 * for this.)
 *
 * New comment must fit in provided buffer newcomment, which is size
 * maxcommentsize.
 *
 * Return 0 on fail (don't update comment), 1 on success.
 *
 * Update this function to meet the needs of your application.
 */

long MyZipAComment(char *oldcomment,
                   long maxcommentsize,
                   long *newcommentsize,
                   char *newcomment)
{
    int oldcommentsize = 0;
    char *comment = NULL;
    char newcommentstring[MAX_COM_LEN];

    if (oldcomment)
      oldcommentsize = strlen(oldcomment);

    fprintf(stderr, "\n");
    fprintf(stderr, ". AComment callback:\n");
    fprintf(stderr, ".   old archive comment (%d bytes):\n",
            oldcommentsize);
    if (oldcomment)
      fprintf(stderr, ".     \"%s\"\n", oldcomment);
    else
      fprintf(stderr, ".     \"\"\n");

    /* here is where you update the comment */

    fprintf(stderr, ". - Enter the new zip file comment (hit ENTER to keep):\n");

    /* The zip file comment can be multiple lines (with \r\n line ends), but we
       only get one line here. */
    fprintf(stderr, ".   Zip file comment: ");
    fflush(stderr);
    comment = fgets(newcommentstring, MAX_COM_LEN, stdin);
    if (comment = NULL) {
      return 0;
    }

    *newcommentsize = strlen(newcommentstring);
    if (*newcommentsize > maxcommentsize - 1)
      *newcommentsize = maxcommentsize - 1;

    if (*newcommentsize == 0 ||
        (*newcommentsize == 1 && newcommentstring[0] == '\n')) {
      if (oldcomment)
        fprintf(stderr, ".   keeping: %s\n", oldcomment);
      else
        fprintf(stderr, ".   keeping: \n");
      return 0;
    }

    strncpy(newcomment, newcommentstring, *newcommentsize);
    newcomment[*newcommentsize] = '\0';

    if (newcomment[*newcommentsize - 1] == '\n')
      newcomment[*newcommentsize - 1] = '\0';

    fprintf(stderr, ".   new archive comment (%d bytes):\n",
            *newcommentsize);
    fprintf(stderr, ".     \"%s\"\n",
            newcomment);
    *newcommentsize = strlen(newcomment);

    return 1;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipPassword(): Encryption password call-back function.
 *
 * bufsize  - max size of password, including terminating NULL
 * prompt   - either the "password" or "verify password" prompt text
 * password - already allocated memory to write password to
 *
 * Note that the password must fit in the password buffer, which is
 * size bufsize.
 *
 * Return 0 on fail, 1 on success.
 *
 * Modify as needed for your application.
 */

long MyZipPassword(long bufsize,
                   char *prompt,
                   char *password)
{
    char *passwd = NULL;
    int passwordlen;
    char newpassword[MAX_PASSWORD_LEN];

    fprintf(stderr, "\n");
    fprintf(stderr, ". Password callback:\n");
    fprintf(stderr, ".   bufsize            = %d\n", bufsize);
    fprintf(stderr, ".   prompt             = '%s'\n", prompt);

    fprintf(stderr, ". - Enter the password: ");
    fflush(stderr);

    passwd = fgets(newpassword, MAX_PASSWORD_LEN, stdin);
    if (passwd = NULL) {
      return 0;
    }

    passwordlen = strlen(newpassword);
    if (passwordlen > bufsize - 1)
      passwordlen = bufsize - 1;

    if (newpassword[passwordlen - 1] == '\n')
      newpassword[passwordlen - 1] = '\0';

    strncpy(password, newpassword, passwordlen);
    password[passwordlen] = '\0';

    fprintf(stderr, ".   returning password = \"%s\"\n",
            newpassword);

    password = string_dup(newpassword, "copying password", NO_FLUFF);
    fprintf(stderr, "password = '%s'\n", password);
    
    return 1;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipService(): Service call-back function.
 *
 * MyZipService is called after each entry is processed to provide a status
 * update.  See also the Progress call-back function, which provides updates
 * during processing as well as after an entry is processed and allows
 * terminating the processing of a large file.
 *
 * If MyZipService() returns non-zero, operation will terminate.  Return 0 to
 * continue.
 *
 *   file_name                     current file being processed
 *   unicode_file_name             UTF-8 name of file
 *   uncompressed_size_string      string usize (e.g. "1.2m" = 1.2 MiB)
 *   compressed_size_string        string csize
 *   uncompressed_size             int usize
 *   compressed_size               int csize
 *   action                        what was done (e.g. "UPDATE", "DELETE")
 *   method                        method used (e.g. "Deflate")
 *   info                          additional info (e.g. "AES256")
 *   percent                       compression percent result
 *
 * This example assumes file sizes no larger than an unsigned long (2 TiB).
 * If larger file sizes are expected, the below casts and output statements
 * need to be modified.  On most ports API_FILESIZE_T is 64 bits.
 *
 * Return 0 to continue.  Return 1 to abort the Zip operation.
 *
 * Modify as needed for your application.
 */

long MyZipService(char *file_name,
                  char *unicode_file_name,
                  char *uncompressed_size_string,
                  char *compressed_size_string,
                  API_FILESIZE_T uncompressed_size,
                  API_FILESIZE_T compressed_size,
                  char *action,
                  char *method,
                  char *info,
                  long percent)
{
  /* Assume that any files tested by the example are smaller than 2 TB. */
  unsigned long us = (unsigned long)uncompressed_size;
  unsigned long cs = (unsigned long)compressed_size;

  fprintf(stderr, "\n");
  fprintf(stderr, ". Service callback:\n");
  fprintf(stderr, ".   File name:                 %s\n", file_name);
  fprintf(stderr, ".   UTF-8 File name:           %s\n", unicode_file_name);
  fprintf(stderr, ".   Uncompressed size string:  %s bytes\n", uncompressed_size_string);
  fprintf(stderr, ".   Uncompressed size:         %ld bytes\n", us);
  fprintf(stderr, ".   Compressed size string:    %s bytes\n", compressed_size_string);
  fprintf(stderr, ".   Compressed size:           %ld bytes\n", cs);
  fprintf(stderr, ".   Action:                    %s\n", action);
  fprintf(stderr, ".   Method:                    %s\n", method);
  fprintf(stderr, ".   Info:                      %s\n", info);
  fprintf(stderr, ".   Percent:                   %ld%%\n", percent);

  /* Return 0 to continue.  Set this to 1 to abort the operation. */
  return 0;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipProgress(): Progress call-back function.
 *
 * MyZipProgress is called when an entry starts processing, every progress_size
 * bytes during processing, and when the entry is finished.  Unlike Service,
 * Progress provides callbacks at regular intervals, allowing the calling
 * application to update a progress bar, for example as the entry is processed.
 * It also allows terminating a compression operation in the middle of entry
 * processing, instead of waiting until the entry is done as Service requires.
 * The Service callback does provide compression results (compressed size and
 * compression percent) not provided by Progress, so an application working
 * with large files probably wants to set up both Service and Progress
 * callbacks.
 *
 * If MyZipProgress() returns non-zero, operation will terminate.  Return 0 to
 * continue.
 *
 *   file_name                     current file being processed (string)
 *   unicode_file_name             UTF-8 name of file (string)
 *   percent_all_done_x_100        percent of all files processed (long)
 *   percent_entry_done_x_100      percent of current entry processed (long)
 *   uncompressed_size             usize current entry (API_FILESIZE_T)
 *   uncompressed_size_string      string usize (e.g. "1.2m" = 1.2 MiB)
 *   action                        what's being done (e.g. "UPDATE", "DELETE")
 *   method                        method used (e.g. "Deflate")
 *   info                          additional info (e.g. "AES256")
 *
 * This example assumes file sizes no larger than an unsigned long (2 TiB).
 * If larger file sizes are expected, the below casts and output statements
 * need to be modified.  On most ports API_FILESIZE_T is 64 bits.
 *
 * Return 0 to continue.  Return 1 to abort the Zip operation.
 *
 * Modify as needed for your application.
 */

long MyZipProgress(char *file_name,
                   char *unicode_file_name,
                   long percent_entry_done_x_100,
                   long percent_all_done_x_100,
                   API_FILESIZE_T uncompressed_size,
                   char *uncompressed_size_string,
                   char *action,
                   char *method,
                   char *info)
{
  /* Assume that any files tested by the example are smaller than 2 TB. */
  unsigned long us = (unsigned long)uncompressed_size;
  double percent_all = percent_all_done_x_100 * 0.01;
  double percent_entry = percent_entry_done_x_100 * 0.01;
  
  fprintf(stderr, "\n");
  fprintf(stderr, ". Progress callback:\n");
  fprintf(stderr, ".   File name:                 %s\n", file_name);
  fprintf(stderr, ".   UTF-8 File name:           %s\n", unicode_file_name);
  fprintf(stderr, ".   Percent this entry done:   %6.2f%%\n", percent_entry);
  fprintf(stderr, ".   Percent all files done:    %6.2f%%\n", percent_all);
  fprintf(stderr, ".   Uncompressed size string:  %s bytes\n", uncompressed_size_string);
  fprintf(stderr, ".   Entry uncompressed size:   %ld bytes\n", us);
  fprintf(stderr, ".   Action:                    %s\n", action);
  fprintf(stderr, ".   Method:                    %s\n", method);
  fprintf(stderr, ".   Info:                      %s\n", info);

  /* Return 0 to continue.  Set this to 1 to abort the operation. */
  return 0;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipError(): Error message call-back function.
 *
 * Any Warning or Error message is sent to this callback.
 */

void MyZipError(char *errstring)
{
  fprintf(stderr, "\n");
  fprintf(stderr, ". Error callback:\n");
  fprintf(stderr, ".   %s", errstring);
}


/* -------------------------------------------------------------------- */
/*
 * MyZipFinish(): Provides the finish stats after all files processed.
 *
 *   final_uncompressed_size_string   total size of files processed (string)
 *   final_compressed_size_string     total size of compressed files (string)
 *   final_uncompressed_size          total usize (API_FILESIZE_T)
 *   final_compressed_size            total csize (API_FILESIZE_T)
 *   final_percent                    compression percent for archive (long)
 *
 */

long MyZipFinish(char *final_uncompressed_size_string,
                 char *final_compressed_size_string,
                 API_FILESIZE_T final_uncompressed_size,
                 API_FILESIZE_T final_compressed_size,
                 long final_percent)
{
  /* Assume that total usize and total csize of archive are smaller than 2 TB. */
  unsigned long fus = (unsigned long)final_uncompressed_size;
  unsigned long fcs = (unsigned long)final_compressed_size;

  fprintf(stderr, "\n");
  fprintf(stderr, ". Finish callback:\n");
  fprintf(stderr, ".   Total usize string:         %s bytes\n", final_uncompressed_size_string);
  fprintf(stderr, ".   Total usize:                %ld bytes\n", fus);
  fprintf(stderr, ".   Total csize string:         %s bytes\n", final_compressed_size_string);
  fprintf(stderr, ".   Total csize:                %ld bytes\n", fcs);
  fprintf(stderr, ".   Final compression percent:  %ld%%\n", final_percent);

  return 0;
}


/* -------------------------------------------------------------------- */
/*
 * MyZipPrint(): Print output call-back function.
 *
 * Any message Zip would normally print to the display is instead received by
 * this function.  Outputting all strings provided by this callback would
 * simulate the output of Zip to the console.
 */

long MyZipPrint(char *buf,
                unsigned long size)
{
#if 0
  fprintf(stderr, "\nZP (%d bytes):  %s", size, buf);
#endif
  fprintf(stderr, "%s", buf);
  return size;
}


/* -------------------------------------------------------------------- */

/* GetZipVersionInfo():  Get Zip version info.
 *
 * Calls ZpVersion to see the version of the LIB you have and the features
 * it supports.  Note that a LIB may have been compiled with different
 * compilation options and so may not have the capabilities you expect.
 *
 * Note that many of the structures are allocated before ZpVersion is
 * called.  The LIB will fill in those with the information.  The ZpVer
 * structure is defined in api.h.  The sizes used below should be considered
 * minimum sizes.
 */
int GetZipVersionInfo(unsigned char *zmaj,
                      unsigned char *zmin,
                      unsigned char *zpatch)
{
  ZpVer zip_ver;

  char zipversnum[100];
  char libdll_interface_vers_num[100];
  char c;
  int i;
  int j;
  int k = 0;
  char features[ZPVER_FEATURES_SIZE];

/* Allocate memory for each thing to be returned.  Here are the
 * definitions from api.h as of this writing.  You should go there
 * and check for changes if these sizes are important to you.
 *     
 *   ZPVER_BETA_LEVEL_SIZE    10      e.g. "g BETA"
 *   ZPVER_VERSION_SIZE       20      e.g. "3.1d28"
 *   ZPVER_REV_DATE_SIZE      20      e.g. "4 September 1995"
 *   ZPVER_REV_YMD_SIZE       20      e.g. "1995/09/04"
 *   ZPVER_ZLIB_VERSION_SIZE  10      e.g. "0.95"
 *   ZPVER_FEATURES_SIZE      4000    e.g. "; backup; compmethods:store,deflate,cd_only,bzip2,lzma,ppmd;"
 */

  if ((zip_ver.BetaLevel = malloc(ZPVER_BETA_LEVEL_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate BetaLevel\n");
    return 93;
  }
  if ((zip_ver.Version = malloc(ZPVER_VERSION_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate Version\n");
    free(zip_ver.BetaLevel);
    return 94;
  }
  if ((zip_ver.RevDate = malloc(ZPVER_REV_DATE_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate RevDate\n");
    free(zip_ver.BetaLevel);
    free(zip_ver.Version);
    return 95;
  }
  if ((zip_ver.RevYMD = malloc(ZPVER_REV_YMD_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate RevYMD\n");
    free(zip_ver.BetaLevel);
    free(zip_ver.Version);
    free(zip_ver.RevDate);
    return 96;
  }
  if ((zip_ver.zlib_Version = malloc(ZPVER_ZLIB_VERSION_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate zlib_Version\n");
    free(zip_ver.BetaLevel);
    free(zip_ver.Version);
    free(zip_ver.RevDate);
    free(zip_ver.RevYMD);
    return 97;
  }
  if ((zip_ver.szFeatures = malloc(ZPVER_FEATURES_SIZE)) == NULL)
  {
    fprintf(stderr, "Could not allocate szFeatures\n");
    free(zip_ver.BetaLevel);
    free(zip_ver.Version);
    free(zip_ver.RevDate);
    free(zip_ver.RevYMD);
    free(zip_ver.zlib_Version);
    return 98;
  }
  
  ZpVersion(&zip_ver);

  features[0] = '\0';
  for (i = 0, j = 0;
       i < ZPVER_FEATURES_SIZE && (c = zip_ver.szFeatures[i]);
       i++)
  {
    features[j++] = c;
    k++;
    if (c == ';')
    {
      features[j++] = ' ';
      if (k > 60) {
        features[j++] = '\n';
        for (k = 0; k < 21; k++) {
          features[j++] = ' ';
        }
        k = 0;
      }
    }
  }
  features[j] = '\0';

  *zmaj = zip_ver.libdll_interface.major;
  *zmin = zip_ver.libdll_interface.minor;
  *zpatch = zip_ver.libdll_interface.patchlevel;

  sprintf(zipversnum, "%d.%d.%d",
          zip_ver.zip.major,
          zip_ver.zip.minor,
          zip_ver.zip.patchlevel);
  sprintf(libdll_interface_vers_num, "%d.%d.%d",
          *zmaj,
          *zmin,
          *zpatch);
  fprintf(stderr, "  Zip version:       %s\n", zip_ver.Version);
  fprintf(stderr, "  Zip version num:   %s %s\n", zipversnum,
                                                   zip_ver.BetaLevel);
  fprintf(stderr, "  Zip rev date:      %s\n", zip_ver.RevDate);
  fprintf(stderr, "  Zip rev YMD:       %s\n", zip_ver.RevYMD);
  fprintf(stderr, "  LIB/DLL interface: %s\n", libdll_interface_vers_num);
  fprintf(stderr, "  Feature list:      %s\n", features);

  free(zip_ver.BetaLevel);
  free(zip_ver.Version);
  free(zip_ver.RevDate);
  free(zip_ver.RevYMD);
  free(zip_ver.zlib_Version);
  free(zip_ver.szFeatures);

  return 0;
}


/* ========================================================================= */

/* UnZip functions */

int MyUzpPassword(zvoid *pG,            /* Ignore (globals pointer). */
                  int *rcnt,            /* Retry count. */
                  char *pwbuf,          /* Password buffer. */
                  int size,             /* Password buffer size. */
                  ZCONST char *zfn,     /* Archive name. */
                  ZCONST char *efn)     /* Archive member name. */
{
    char pass[MAX_PASSWORD_LEN + 1];
    int len;
    
    fprintf( stderr, "unzip:  password callback:  size = %d, zfn: >%s<\n", size, zfn);
    fprintf( stderr, "unzip:  password callback:  efn: >%s<\n", efn);
    fprintf( stderr, "unzip:  password callback:  *rcnt = %d\n", *rcnt);

    fprintf(stderr, "\n");
    fprintf(stderr, "Enter password for zipfile '%s', entry '%s':  ", zfn, efn);
    if (fgets(pwbuf, size, stderr) == NULL) {
      fprintf(stderr, "unzip:  password callback:  unable to read password\n");
      strcpy(pwbuf, "");
      return 1;
    }
    len = strlen(pwbuf);
    if (len > 0 && pwbuf[len-1] == '\n') {
      len--;
      pwbuf[len] = '\0';
    }
    fprintf(stderr, "Password:  (%d bytes) '%s'\n", len, pwbuf);
    return 0;
}


/* -------------------------------------------------------------------- */

int GetUnZipVersionInfo(unsigned char *umaj,
                        unsigned char *umin,
                        unsigned char *upatch)
{
  int i;
  int j;
  int k = 0;
  char c;
  char *featurelist;
  char features[ZPVER_FEATURES_SIZE];
  ZCONST UzpVer *unzip_ver_p; /* Storage for program version string. */
  
  /* Get and display the library version. */
  unzip_ver_p = UzpVersion();
  
  *umaj = unzip_ver_p->unzip.vmajor;
  *umin = unzip_ver_p->unzip.vminor;
  *upatch = unzip_ver_p->unzip.patchlevel;
  fprintf( stderr, " UnZip version %d.%d%d%s\n", *umaj, *umin, *upatch,
    unzip_ver_p->betalevel);

  /* Get and display the library feature list. */
  featurelist = UzpFeatures();
  if (featurelist == NULL) {
    fprintf(stderr, "Failed to retrieve UnZip feature list\n");
  }
  else
  {
    features[0] = '\0';
    for (i = 0, j = 0;
         i < ZPVER_FEATURES_SIZE && (c = featurelist[i]);
         i++)
    {
      features[j++] = c;
      k++;
      if (c == ';')
      {
        features[j++] = ' ';
        if (k > 60) {
          features[j++] = '\n';
          for (k = 0; k < 21; k++) {
            features[j++] = ' ';
          }
          k = 0;
        }
      }
    }
    features[j] = '\0';
    fprintf( stderr, " UnZip features: %s\n", features);
  }
}    

