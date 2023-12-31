/*
  Copyright (c) 1990-2016 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-2 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/

/* 2005-02-14 SMS.
   Added some ODS5 support.
      Use longer name structures in NAML, where available.
      Locate special characters mindful of "^" escapes.
      Replaced compile-time case preservation (VMS_PRESERVE_CASE macro)
      with command-line-specified case preservation (vms_case_x
      variables).
   Prototyped all functions.
   Removed "#ifndef UTIL", as no one should be compiling it that way.
*/

#include "zip.h"
#include "vmsmunch.h"
#include "vms.h"

#include <ctype.h>
#include <time.h>
#include <unixlib.h>

#include <dvidef.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <stsdef.h>
#include <starlet.h>

/* On VAX, define Goofy VAX Type-Cast to obviate /standard = vaxc.
   Otherwise, lame system headers on VAX cause compiler warnings.
   (GNU C may define vax but not __VAX.)
*/
#ifdef vax
# define __VAX 1
#endif /* def vax */

#ifdef __VAX
# define GVTC (unsigned int)
#else /* def __VAX */
# define GVTC
#endif /* def __VAX */

/* Directory file type with version, and its strlen(). */
#define DIR_TYPE_VER ".DIR;1"
#define DIR_TYPE_VER_LEN (sizeof( DIR_TYPE_VER)- 1)

/* Extra malloc() space in names for cutpath().  (May have to change
   ".FOO]" to "]FOO.DIR;1".)
*/
#define DIR_PAD (DIR_TYPE_VER_LEN- 1)

/* Hex digit table. */

char hex_digit[ 16] = {
 '0', '1', '2', '3', '4', '5', '6', '7',
 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/* 2008-10-15 SMS.
 * Revised the char_prop[] table to add these characters (omitted in the
 * VMS documentation) to the caret-escape list:  "  :  \  |
 */

/* Character property table for (re-)escaping ODS5 extended file names.
   Note that this table ignores Unicode, and does not identify invalid
   characters.

   ODS2 valid characters: 0-9 A-Z a-z $ - _

   ODS5 Invalid characters:
      C0 control codes (0x00 to 0x1F inclusive)
      Asterisk (*)
      Question mark (?)

   ODS5 Invalid characters only in VMS V7.2 (which no one runs, right?):
      Double quotation marks (")
      Backslash (\)
      Colon (:)
      Left angle bracket (<)
      Right angle bracket (>)
      Slash (/)
      Vertical bar (|)

   Characters escaped by "^":
      SP  !  "  #  %  &  '  (  )  +  ,  .  :  ;  =
       @  [  \  ]  ^  `  {  |  }  ~

   Either "^_" or "^ " is accepted as a space.  Period (.) is a special
   case.  Note that un-escaped < and > can also confuse a directory
   spec.

   Characters put out as ^xx:
      7F (DEL)
      80-9F (C1 control characters)
      A0 (nonbreaking space)
      FF (Latin small letter y diaeresis)

   Other cases:
      Unicode: "^Uxxxx", where "xxxx" is four hex digits.

    Property table values:
      Normal escape:    1
      Space:            2
      Dot:              4
      Hex-hex escape:   8
      -------------------
      Hex digit:       64
*/

unsigned char char_prop[ 256] = {

/* NUL SOH STX ETX EOT ENQ ACK BEL   BS  HT  LF  VT  FF  CR  SO  SI */
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/* DLE DC1 DC2 DC3 DC4 NAK SYN ETB  CAN  EM SUB ESC  FS  GS  RS  US */
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*  SP  !   "   #   $   %   &   '    (   )   *   +   ,   -   .   /  */
    2,  1,  1,  1,  0,  1,  1,  1,   1,  1,  0,  1,  1,  0,  4,  0,

/*  0   1   2   3   4   5   6   7    8   9   :   ;   <   =   >   ?  */
   64, 64, 64, 64, 64, 64, 64, 64,  64, 64,  1,  1,  1,  1,  1,  1,

/*  @   A   B   C   D   E   F   G    H   I   J   K   L   M   N   O  */
    1, 64, 64, 64, 64, 64, 64,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*  P   Q   R   S   T   U   V   W    X   Y   Z   [   \   ]   ^   _  */
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  1,  1,  1,  1,  0,

/*  `   a   b   c   d   e   f   g    h   i   j   k   l   m   n   o  */
    1, 64, 64, 64, 64, 64, 64,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*  p   q   r   s   t   u   v   w    x   y   z   {   |   }   ~  DEL */
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  1,  1,  1,  1,  8,

    8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
    8,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  8
};

/* The C RTL from OpenVMS 7.0 and newer supplies POSIX compatible versions of
 * opendir() et al. Thus, we have to use other names in our private code for
 * directory scanning to prevent symbol name conflicts at link time.
 * For now, we do not use the library supplied "dirent.h" functions, since
 * our private implementation provides some functionality which may not be
 * present in the library versions.  For example:
 * ==> zopendir("DISK:[DIR.SUB1]SUB2.DIR") scans "DISK:[DIR.SUB1.SUB2]".
 */

typedef struct zdirent {
  int d_wild;                /* flag for wildcard vs. non-wild */
  struct FAB fab;
  struct NAMX_STRUCT nam;
  char d_qualwildname[ NAMX_MAXRSS+ 1];
  char d_name[ NAMX_MAXRSS+ 1];
} zDIR;

local ulg label_time = 0;
local ulg label_mode = 0;
local time_t label_utim = 0;

local int relative_dir_s = 0;   /* Relative directory spec. */

/* Local functions */
local void vms_wild OF((char *, zDIR *));
local zDIR *zopendir OF((ZCONST char *));
local char *readd OF((zDIR *));
local char *strlower OF((char *));
local char *strupper OF((char *));

/* 2004-09-25 SMS.
   str[n]casecmp() replacement for old C RTL.
   Assumes a prehistorically incompetent toupper().
*/
#ifndef HAVE_STRCASECMP

int strncasecmp( char *s1, char *s2, size_t n)
{
  /* Initialization prepares for n == 0. */
  char c1 = '\0';
  char c2 = '\0';

  while (n-- > 0)
  {
    /* Set c1 and c2.  Convert lower-case characters to upper-case. */
    if (islower( c1 = *s1))
      c1 = toupper( c1);

    if (islower( c2 = *s2))
      c2 = toupper( c2);

    /* Quit at inequality or NUL. */
    if ((c1 != c2) || (c1 == '\0'))
      break;

    s1++;
    s2++;
  }
return ((unsigned int) c1- (unsigned int) c2);
}

#ifndef UINT_MAX
#define UINT_MAX 4294967295U
#endif

#define strcasecmp( s1, s2) strncasecmp( s1, s2, UINT_MAX)

#endif /* ndef HAVE_STRCASECMP */


/* 2004-09-27 SMS.
   eat_carets().

   Delete ODS5 extended file name escape characters ("^") in the
   (UNIX-style) path name in its original buffer.
   Note that the current scheme does not handle all EFN cases, but it
   could be made more complicated.
*/

/* 2008-10-15 SMS.
 * Added code to look for a caret-escaped dot in the name field, and
 * return a boolean value accordingly.
 */

local int eat_carets( char *str)
/* char *str;      Source pointer. */
{
  char *strd;   /* Destination pointer. */
  char hdgt;
  unsigned char uchr;
  unsigned char prop;
  int caret_dot_in_name;

  caret_dot_in_name = 0;
  /* Skip ahead to the first "^", if any. */
  while ((*str != '\0') && (*str != '^'))
     str++;

  /* If no caret was found, quit early. */
  if (*str != '\0')
  {
    /* Shift characters leftward as carets are found. */
    strd = str;
    while (*str != '\0')
    {
      uchr = *str;
      if (uchr == '^')
      {
        /* Found a caret.  Skip it, and check the next character. */
        uchr = *(++str);
        prop = char_prop[ uchr];
        if (prop& 64)
        {
          /* Hex digit.  Get char code from this and next hex digit. */
          if (uchr <= '9')
          {
            hdgt = uchr- '0';           /* '0' - '9' -> 0 - 9. */
          }
          else
          {
            hdgt = ((uchr- 'A')& 7)+ 10;    /* [Aa] - [Ff] -> 10 - 15. */
          }
          hdgt <<= 4;                   /* X16. */
          uchr = *(++str);              /* Next char must be hex digit. */
          if (uchr <= '9')
          {
            uchr = hdgt+ uchr- '0';
          }
          else
          {
            uchr = hdgt+ ((uchr- 'A')& 15)+ 10;
          }
        }
        else if (uchr == '_')
        {
          /* Convert escaped "_" to " ". */
          uchr = ' ';
        }
        else if (uchr == '/')
        {
          /* Convert escaped "/" (invalid Zip) to "?" (invalid VMS). */
          uchr = '?';
        }
        else if (uchr == '.')
        {
          /* Record caret-dot (which may be in the name field). */
          caret_dot_in_name = 1;
        }
        /* Else, not a hex digit.  Must be a simple escaped character
           (or Unicode, which is not yet handled here).
        */
      }
      else if (uchr == '/')
      {
        /* Unescaped end-of-directory character.  Clear caret-dot-in-name. */
        caret_dot_in_name = 0;
      }
      /* Else, not a caret.  Use as-is. */
      *strd = uchr;

      /* Advance destination and source pointers. */
      strd++;
      str++;
    }
    /* Terminate the destination string. */
    *strd = '\0';
  }
  return caret_dot_in_name;
}


/* 2007-05-22 SMS.
 * explicit_dev().
 *
 * Determine if an explicit device name is present in a (VMS) file
 * specification.
 */
local int explicit_dev( char *file_spec)
{
  int sts;
  struct FAB fab;               /* FAB. */
  struct NAMX_STRUCT nam;       /* NAM[L]. */

  /* Initialize the FAB and NAM[L], and link the NAM[L] to the FAB. */
  nam = CC_RMS_NAMX;
  fab = cc$rms_fab;
  fab.FAB_NAMX = &nam;

  /* Point the FAB/NAM[L] fields to the actual name and default name. */
  NAMX_DNA_FNA_SET( fab)

  /* File name. */
  FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = file_spec;
  FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen( file_spec);

  nam.NAMX_NOP = NAMX_M_SYNCHK; /* Syntax-only analysis. */
  sts = sys$parse( &fab, 0, 0); /* Parse the file spec. */

  /* Device found = $PARSE success and "device was explicit" flag. */
  return (((sts& STS$M_SEVERITY) == STS$M_SUCCESS) &&
   ((nam.NAMX_FNB& NAMX_M_EXP_DEV) != 0));
}


/* 2005-02-04 SMS.
   find_dir().

   Find directory boundaries in an ODS2 or ODS5 file spec.
   Returns length (zero if no directory, negative if error),
   and sets "start" argument to first character (typically "[") location.

   No one will care about the details, but the return values are:

       0  No dir.
      -2  [, no end.              -3  <, no end.
      -4  [, multiple start.      -5  <, multiple start.
      -8  ], no start.            -9  >, no start.
     -16  ], wrong end.          -17  >, wrong end.
     -32  ], multiple end.       -33  >, multiple end.

   Note that the current scheme handles only simple EFN cases, but it
   could be made more complicated.
*/
int find_dir( char *file_spec, char **start)
{
  char *cp;
  char chr;

  char *end_tmp = NULL;
  char *start_tmp = NULL;
  int lenth = 0;

  for (cp = file_spec; cp < file_spec+ strlen( file_spec); cp++)
  {
    chr = *cp;
    if (chr == '^')
    {
      /* Skip ODS5 extended name escaped characters. */
      cp++;
      /* If escaped char is a hex digit, skip the second hex digit, too. */
      if (char_prop[ (unsigned char) *cp]& 64)
        cp++;
    }
    else if (chr == '[')
    {
      /* Found start. */
      if (start_tmp == NULL)
      {
        /* First time.  Record start location. */
        start_tmp = cp;
        /* Error if no end. */
        lenth = -2;
      }
      else
      {
        /* Multiple start characters.  */
        lenth = -4;
        break;
      }
    }
    else if (chr == '<')
    {
      /* Found start. */
      if (start_tmp == NULL)
      {
        /* First time.  Record start location. */
        start_tmp = cp;
        /* Error if no end. */
        lenth = -3;
      }
      else
      {
        /* Multiple start characters.  */
        lenth = -5;
        break;
      }
    }
    else if (chr == ']')
    {
      /* Found end. */
      if (end_tmp == NULL)
      {
        /* First time. */
        if (lenth == 0)
        {
          /* End without start. */
          lenth = -8;
          break;
        }
        else if (lenth != -2)
        {
          /* Wrong kind of end. */
          lenth = -16;
          break;
        }
        /* End ok.  Record end location. */
        end_tmp = cp;
        lenth = end_tmp+ 1- start_tmp;
        /* Could break here, ignoring excessive end characters. */
      }
      else
      {
        /* Multiple end characters. */
        lenth = -32;
        break;
      }
    }
    else if (chr == '>')
    {
      /* Found end. */
      if (end_tmp == NULL)
      {
        /* First time. */
        if (lenth == 0)
        {
          /* End without start. */
          lenth = -9;
          break;
        }
        else if (lenth != -3)
        {
          /* Wrong kind of end. */
          lenth = -17;
          break;
        }
        /* End ok.  Record end location. */
        end_tmp = cp;
        lenth = end_tmp+ 1- start_tmp;
        /* Could break here, ignoring excessive end characters. */
      }
      else
      {
        /* Multiple end characters. */
        lenth = -33;
        break;
      }
    }
  }

  /* If both start and end were found,
     then set result pointer where safe.
  */
  if (lenth > 0)
  {
    if (start != NULL)
    {
      *start = start_tmp;
    }
  }
  return lenth;
}


/* 2005-02-08 SMS.
   file_sys_type().

   Determine the file system type for the (VMS) path name argument.
*/
local int file_sys_type( char *path)
{
  int acp_code;

#ifdef DVI$C_ACP_F11V5

/* Should know about ODS5 file system.  Do actual check.
   (This should be non-VAX with __CRTL_VER >= 70200000.)
*/

  int sts;

  struct dsc$descriptor_s dev_descr =
   { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0 };

  /* Load path argument into device descriptor. */
  dev_descr.dsc$a_pointer = path;
  dev_descr.dsc$w_length = strlen( dev_descr.dsc$a_pointer);

  /* Get filesystem type code.
     (Text results for this item code have been unreliable.)
  */
  sts = lib$getdvi( &((int) DVI$_ACPTYPE), 0, &dev_descr, &acp_code, 0, 0);

  if ((sts & STS$M_SUCCESS) != STS$K_SUCCESS)
  {
    acp_code = -1;
  }

#else /* def DVI$C_ACP_F11V5 */

/* Too old for ODS5 file system.  Must be ODS2. */

  acp_code = DVI$C_ACP_F11V2;

#endif /* def DVI$C_ACP_F11V5 */

  return acp_code;
}


/* 2013-02-18 SMS.
 * get_volume_label( int len, char *pth)
 *
 * Return pointer to static volume name for the specified device.
 */
local char *get_volume_label( int len, char *pth)
{
  char *lbl = NULL;
  int sts;
  char vol_nam[ 33];    /* Include NUL-term.  Actual max = ??? */

  /* Volume label descriptor. */
  struct dsc$descriptor_s vol_nam_descr =
   { ((sizeof vol_nam)- 1), DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };
  unsigned short vol_nam_len;

  struct dsc$descriptor_s dev_descr =
   { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0 };

  vol_nam_descr.dsc$a_pointer = vol_nam;

  /* Load path arguments into device descriptor. */
  dev_descr.dsc$a_pointer = pth;
  dev_descr.dsc$w_length = len;

  /* Get volume name. */
  sts = lib$getdvi( &((int) DVI$_VOLNAM),       /* Item code. */
                    0,                          /* Channel. */
                    &dev_descr,                 /* Device name. */
                    0,                          /* Result buffer (int). */
                    &vol_nam_descr,             /* Result buffer (str). */
                    &vol_nam_len,               /* Result length. */
                    0);                         /* Path name. */

  if ((sts & STS$M_SUCCESS) == STS$K_SUCCESS)
  {
    *(vol_nam+ vol_nam_len) = '\0';     /* NUL-terminate the label. */
    lbl = vol_nam;
    /* Set (fake) mode, time, and utim for the label. */
    label_mode = (ulg)(0x28);
    label_time = (ulg)(1);
    label_utim = (time_t)(1);
  }
  return lbl;
}


/*---------------------------------------------------------------------------

    _vms_findfirst() and _vms_findnext(), based on public-domain DECUS C
    fwild() and fnext() routines (originally written by Martin Minow, poss-
    ibly modified by Jerry Leichter for bintnxvms.c), were written by Greg
    Roelofs and are still in the public domain.  Routines approximate the
    behavior of MS-DOS (MSC and Turbo C) findfirst and findnext functions.

    2005-01-04 SMS.
    Changed to use NAML instead of NAM, where available.

  ---------------------------------------------------------------------------*/

static char wild_version_part[10]="\0";

local void vms_wild( char *p, zDIR *d)
{
  /*
   * Do wildcard setup.
   */
  /* Set up the FAB and NAM[L] blocks. */
  d->fab = cc$rms_fab;                  /* Initialize FAB. */
  d->nam = CC_RMS_NAMX;                 /* Initialize NAM[L]. */

  d->fab.FAB_NAMX = &d->nam;            /* FAB -> NAM[L] */

  /* Point the FAB/NAM[L] fields to the actual name and default name. */
  NAMX_DNA_FNA_SET( d->fab)

  /* Argument file name and length. */
  d->FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = p;
  d->FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen(p);

#define DEF_DEVDIR "SYS$DISK:[]"

  /* Default file spec and length. */
  d->FAB_OR_NAML( fab, nam).FAB_OR_NAML_DNA = DEF_DEVDIR;
  d->FAB_OR_NAML( fab, nam).FAB_OR_NAML_DNS = sizeof( DEF_DEVDIR)- 1;

  d->nam.NAMX_ESA = d->d_qualwildname;  /* qualified wild name */
  d->nam.NAMX_ESS = NAMX_MAXRSS;        /* max length */
  d->nam.NAMX_RSA = d->d_name;          /* matching file name */
  d->nam.NAMX_RSS = NAMX_MAXRSS;        /* max length */

  /* parse the file name */
  if (sys$parse(&d->fab) != RMS$_NORMAL)
    return;
  /* Does this replace d->fab.fab$l_fna with a new string in its own space?
     I sure hope so, since p is free'ed before this routine returns. */

  /* have qualified wild name (i.e., disk:[dir.subdir]*.*); null-terminate
   * and set wild-flag */
  d->d_qualwildname[d->nam.NAMX_ESL] = '\0';
  d->d_wild = (d->nam.NAMX_FNB & NAM$M_WILDCARD)? 1 : 0;   /* not used... */
#ifdef DEBUG
  fprintf(mesg, "  incoming wildname:  %s\n", p);
  fprintf(mesg, "  qualified wildname:  %s\n", d->d_qualwildname);
#endif /* DEBUG */
}

local zDIR *zopendir( ZCONST char *n)
/* ZCONST char *n;         directory to open */
/* Start searching for files in the VMS directory n */
{
  char *c;              /* scans VMS path */
  zDIR *d;              /* malloc'd return value */
  int m;                /* length of name */
  char *p;              /* malloc'd temporary string */

  if ((d = (zDIR *)malloc(sizeof(zDIR))) == NULL ||
      (p = malloc((m = strlen(n)) + 4)) == NULL) {
    if (d != NULL) free((zvoid *)d);
    return NULL;
  }
  /* Directory may be in form "[DIR.SUB1.SUB2]" or "[DIR.SUB1]SUB2.DIR;1".
     If latter, convert to former.
     2005-01-31 SMS.  Changed to require ";1", as VMS does, which
     simplified the code slightly, too.  Note that ODS5 allows ".DIR" in
     any case (upper, lower, mixed).
  */
  if ((m > 0) && (*(c = strcpy(p,n)+m-1) != ']'))
  {
    if ((c- p < DIR_TYPE_VER_LEN) ||
     strcasecmp((c+ 1- DIR_TYPE_VER_LEN), DIR_TYPE_VER))
    {
      free((zvoid *)d);  free((zvoid *)p);
      return NULL;
    }
    c -= 4;             /* The "D". */
    *c-- = '\0';        /* terminate at "DIR;1" */
    *c = ']';           /* "." --> "]" */

    /* Replace the formerly last "]" with ".".
       For ODS5, ignore "^]".
    */
    while ((c > p) && ((*--c != ']') || (*(c- 1) == '^')))
      ;
    *c = '.';           /* "]" --> "." */
  }
  strcat(p, "*.*");
  strcat(p, wild_version_part);
  vms_wild(p, d);       /* set up wildcard */
  free((zvoid *)p);
  return d;
}

local char *readd( zDIR *d)
/* zDIR *d;                directory stream to read from */
/* Return a pointer to the next name in the directory stream d, or NULL if
   no more entries or an error occurs. */
{
  int r;                /* return code */

  do {
    d->fab.fab$w_ifi = 0;       /* internal file index:  what does this do? */
/*
  2005-02-04 SMS.

  From the docs:

        Note that you must close the file before invoking the Search
        service (FAB$W_IFI must be 0).

  The same is true for PARSE.  Most likely, it's cleared by setting
  "fab = cc$rms_fab", and left that way, so clearing it here may very
  well be pointless.  (I think it is, and I've never seen it explicitly
  cleared elsewhere, but I haven't tested it everywhere either.)
*/
    /* get next match to possible wildcard */
    if ((r = sys$search(&d->fab)) == RMS$_NORMAL)
    {
        d->d_name[d->nam.NAMX_RSL] = '\0';   /* null terminate */
        return (char *)d->d_name;   /* OK */
    }
  } while (r == RMS$_PRV);
  return NULL;
}


int wild( char *p)
/* char *p;                path/pattern to match */
/* Expand the pattern based on the contents of the file system.
   Return an error code in the ZE_ class.
   Note that any command-line file argument may need wildcard expansion,
   so all user-specified constituent file names pass through here.
*/
{
  zDIR *d;              /* stream for reading directory */
  char *e;              /* name found in directory */
  int f;                /* true if there was a match */

  int dir_len;          /* Length of the directory part of the name. */
  char *dir_start;      /* First character of the directory part. */

  /* special handling of stdin request */
  if (is_stdin || (!no_stdin && strcmp(p, "-") == 0))   /* if compressing stdin */
    return newname(p, 0, 0);

  /* Determine whether this name has an absolute or relative directory
     spec.  It's relative if there is no directory, or if the directory
     has a leading dot ("[.").
  */
  dir_len = find_dir( p, &dir_start);
  relative_dir_s = ((dir_len <= 0)? 1 : (dir_start[ 1] == '.'));

  /* Search given pattern for matching names */
  if ((d = (zDIR *)malloc(sizeof(zDIR))) == NULL)
    return ZE_MEM;
  vms_wild(p, d);       /* pattern may be more than just directory name */

  /* 2013-02-18 SMS.
   * If wanting a volume label, then get it from this (first) file.
   * MSDOS/Windows allows a drive letter with no file spec ("A:") to
   * specify a drive to provide a volume label without adding a file.
   * We look for a bare device name (with a colon), like "xxxx:".
   */
  if (volume_label == 1)
  {
    volume_label = 2;
    label = get_volume_label( d->nam.NAMX_B_DEV, d->nam.NAMX_L_DEV);
    if ((label != NULL) && (*label != '\0'))
    {
      newname( label, 0, 0);
    }
    /* If only a device name (last char is ":", no name, no type), then
     * pretend that all is well, and leave early.  (Using $PARSE results
     * obviates looking for "^:" and other quirky elements.)
     */
    if ((p == NULL) ||                  /* Not a real name. */
     ((strlen( p) > 1) &&               /* More than ":". */
     (p[ strlen( p)- 1] == ':') &&      /* Ends with ":", */
     (d->nam.NAMX_B_DEV > 0) &&         /* Some device characters. */
     (d->nam.NAMX_B_NAME == 0) &&       /* No name characters. */
     (d->nam.NAMX_B_TYPE <= 1)))        /* No (non-dot) .type characters. */
    {
      return ZE_OK;
    }
  }

  /*
   * For a non-directory file, save the version specified by the user
   * for use in recursive drops into subdirectories.  (If the user
   * specifies "-r fred.dir;1", then we don't want to save the ";1".)
   */

  if (((d->nam.NAMX_B_TYPE+ d->nam.NAMX_B_VER) == DIR_TYPE_VER_LEN) &&
   strncasecmp( d->nam.NAMX_L_TYPE, DIR_TYPE_VER, DIR_TYPE_VER_LEN) == 0)
  {
    wild_version_part[ 0] = '\0';
  }
  else
  {
    strncpy(wild_version_part, d->nam.NAMX_L_VER, d->nam.NAMX_B_VER);
    wild_version_part[d->nam.NAMX_B_VER] = '\0';
  }

  f = 0;
  while ((e = readd(d)) != NULL)        /* "dosmatch" is already built in */
    if (procname(e, 0) == ZE_OK)
      f = 1;
  free(d);

  /* Done */
  return f ? ZE_OK : ZE_MISS;
}

int procname( char *n, int caseflag)
/* char *n;                name to process */
/* int caseflag;           true to force case-sensitive match */
/* Process a name or sh expression to operate on (or exclude).  Return
   an error code in the ZE_ class. */
{
  zDIR *d;              /* directory stream from zopendir() */
  char *e;              /* pointer to name from readd() */
  int m;                /* matched flag */
  char *p;              /* path for recursion */
  struct stat s;        /* result of stat() */
  struct zlist far *z;  /* steps through zfiles list */

  if (n == NULL)        /* volume_label request in freshen|delete mode ?? */
    return ZE_OK;

  if (is_stdin || (!no_stdin && strcmp(n, "-") == 0))   /* if compressing stdin */
    return newname(n, 0, caseflag);
  else if (LSSTAT(n, &s)
#if defined(__TURBOC__) || defined(VMS) || defined(__WATCOMC__)
           /* For these 3 compilers, stat() succeeds on wild card names! */
           || isshexp(n)
#endif
          )
  {
    /* Not a file or directory--search for shell expression in zip file */
    if (caseflag) {
      p = malloc(strlen(n) + 1);
      if (p != NULL)
        strcpy(p, n);
    } else
      p = ex2in(n, 0, (int *)NULL);     /* shouldn't affect matching chars */
    m = 1;
    for (z = zfiles; z != NULL; z = z->nxt) {
      if (MATCH(p, z->iname, caseflag))
      {
        z->mark = pcount ? filter(z->zname, caseflag) : 1;
        if (verbose)
            fprintf(mesg, "zip diagnostic: %scluding %s\n",
               z->mark ? "in" : "ex", z->name);
        m = 0;
      }
    }
    free((zvoid *)p);
    return m ? ZE_MISS : ZE_OK;
  }

  /* Live name.  Recurse if directory.  Use if file. */
  if (S_ISDIR( s.st_mode))
  {
    /* Directory. */
    if (dirnames && (m = newname(n, ZFLAG_DIR, caseflag)) != ZE_OK) {
      return m;
    }
    /* recurse into directory */
    if (recurse && (d = zopendir(n)) != NULL)
    {
      while ((e = readd(d)) != NULL) {
        if ((m = procname(e, caseflag)) != ZE_OK)     /* recurse on name */
        {
          free(d);
          return m;
        }
      }
      free(d);
    }
  }
  else /* S_ISDIR( s.st_mode) */
  {
    /* Non-directory.  Add or remove name of file. */
    if ((m = newname(n, 0, caseflag)) != ZE_OK)
      return m;
  } /* S_ISDIR( s.st_mode) [else] */
  return ZE_OK;
}

/* 2004-09-24 SMS.
   Cuter strlower() and strupper() functions.
*/

local char *strlower( char *s)
/* Convert all uppercase letters to lowercase in string s */
{
  for ( ; *s != '\0'; s++)
    if (isupper( *s))
      *s = tolower( *s);

  return s;
}

local char *strupper( char *s)
/* Convert all lowercase letters to uppercase in string s */
{
  for ( ; *s != '\0'; s++)
    if (islower( *s))
      *s = toupper( *s);

  return s;
}

char *ex2in( char *x, int isdir, int *pdosflag)
/* char *x;                external file name */
/* int isdir;              input: x is a directory */
/* int *pdosflag;          output: force MSDOS file attributes? */

/* Convert the external file name to a zip file name, returning the
   malloc'ed string or NULL if not enough memory.

   2005-02-09 SMS.
   Added some ODS5 support.

   Note that if we were really clever, we'd save the truncated original
   file name for later use as "iname", instead of running the de-escaped
   product back through in2ex() to recover it later.

   2005-11-13 SMS.
   Changed to translate "[..." into enough "/" characters to cause
   in2ex() to reconstruct it.  This should not be needed, however, as
   pattern matching really should avoid ex2in() and in2ex().
*/
{
  char *n;                      /* Internal file name (malloc'ed). */
  char *nn;                     /* Temporary "n"-like pointer. */
  char *ext_dir_and_name;       /* External dir]name (less "dev:["). */
  char chr;                     /* Temporary character storage. */
  int dosflag;
  int down_case;                /* Resultant down-case flag. */
  int dir_len;                  /* Directory spec length. */
  int escaped_dot_in_name;      /* Escaped dot in name field. */
  int ods_level;                /* File system type. */

  dosflag = dosify; /* default for non-DOS and non-OS/2 */

  /* Locate the directory part of the external name. */
  dir_len = find_dir( x, &ext_dir_and_name);
  if (dir_len <= 0)
  {
    /* Directory not found.  Use whole external name. */
    ext_dir_and_name = x;
  }
  else if (pathput)
  {
    /* Include directory. */
    if (ext_dir_and_name[ 1] == '.')
    {
      /* Relative path.  If not a directory-depth wildcard, then drop
         first "[." (or "<.").  If "[..." (or "<..."), then preserve all
         characters, including the first "[" (or "<") for special
         handling below.
      */
      if ((ext_dir_and_name[ 2] != '.') || (ext_dir_and_name[ 3] != '.'))
      {
        /* Normal relative path.  Drop first "[." (or "<."). */
        dir_len -= 2;
        ext_dir_and_name += 2;
      }
    }
    else
    {
      /* Absolute path.  Skip first "[" (or "<"). */
      dir_len -= 1;
      ext_dir_and_name += 1;

      /* 2007-04-26 SMS.
         Skip past "000000." or "000000]" (or "000000>"), which should
         not be stored in the archive.  This arises, for example, with
         "zip -r archive [000000]foo.dir"
      */
#define MFD "000000"

      if ((strncmp( ext_dir_and_name, MFD, strlen( MFD)) == 0) &&
       ((ext_dir_and_name[ 6] == '.') ||
       (ext_dir_and_name[ 6] == ']') ||
       (ext_dir_and_name[ 6] == '>')))
      {
        dir_len -= 7;
        ext_dir_and_name += 7;
      }
    }
  }
  else
  {
    /* Junking paths.  Skip the whole directory spec. */
    ext_dir_and_name += dir_len;
    dir_len = 0;
  }

  /* Malloc space for internal name and copy it. */
  if ((n = malloc(strlen( ext_dir_and_name)+ 1)) == NULL)
    return NULL;
  strcpy( n, ext_dir_and_name);

  /* Convert VMS directory separators (".") to "/". */
  if (dir_len > 0)
  {
    for (nn = n; nn < n+ dir_len; nn++)
    {
      chr = *nn;
      if (chr == '^')
      {
        /* Skip ODS5 extended name escaped characters. */
        nn++;
        /* If escaped char is a hex digit, skip the second hex digit, too. */
        if (char_prop[ (unsigned char) *nn]& 64)
          nn++;
      }
      else if ((chr == '.') || ((nn == n) && ((chr == '[') || (chr == '<'))))
      {
        /* Convert VMS directory separator (".", or initial "[" or "<"
           of "[..." or "<...") to "/".
        */
        *nn = '/';
      }
    }
    /* Replace directory end character (typically "]") with "/". */
    n[ dir_len- 1] = '/';
  }

  /* If relative path, then strip off the current directory. */
  if (relative_dir_s)
  {
    char cwd[ NAMX_MAXRSS+ 1];
    char *cwd_dir_only;
    char *q;
    int cwd_dir_only_len;

    q = getcwd( cwd, (sizeof( cwd)- 1));

    /* 2004-09-24 SMS.
       With SET PROCESSS /PARSE = EXTENDED, getcwd() can return a
       mixed-case result, confounding the comparisons below with an
       all-uppercase name in "n".  Always use a case-insensitive
       comparison around here.
    */

    /* Locate the directory part of the external name. */
    dir_len = find_dir( q, &cwd_dir_only);
    if (dir_len > 0)
    {
      /* Skip first "[" (or "<"). */
      cwd_dir_only++;
      /* Convert VMS directory separators (".") to "/". */
      for (q = cwd_dir_only; q < cwd_dir_only+ dir_len; q++)
      {
        chr = *q;
        if (chr == '^')
        {
          /* Skip ODS5 extended name escaped characters. */
          q++;
          /* If escaped char is a hex digit, skip the second hex digit, too. */
          if (char_prop[ (unsigned char) *q]& 64)
            q++;
        }
        else if (chr == '.')
        {
          /* Convert VMS directory separator (".") to "/". */
          *q = '/';
        }
      }
      /* Replace directory end character (typically "]") with "/". */
      cwd_dir_only[ dir_len- 2] = '/';
    }

    /* If the slash-converted cwd matches the front of the internal
       name, then shuffle the remainder of the internal name to the
       beginning of the internal name storage.

       Because we already know that the path is relative, this test may
       always succeed.
    */
    cwd_dir_only_len = strlen( cwd_dir_only);
    if (strncasecmp( n, cwd_dir_only, cwd_dir_only_len) == 0)
    {
       nn = n+ cwd_dir_only_len;
       q = n;
       while (*q++ = *nn++);
    }
  } /* (relative_dir_s) */

  /* 2007-05-22 SMS.
   * If a device name is present, assume that it's a real (VMS) file
   * specification, and do down-casing according to the ODS2 or ODS5
   * down-casing policy.  If no device name is present, assume that it's
   * a pattern ("-i", ...), and do no down-casing here.  (Case
   * sensitivity in patterns is handled elsewhere.)
   */
  if (explicit_dev( x))
  {
    /* If ODS5 is possible, do complicated down-case check.

       Note that the test for ODS2/ODS5 is misleading and over-broad.
       Here, "ODS2" includes anything from DVI$C_ACP_F11V1 (=1, ODS1) up
       to (but not including) DVI$C_ACP_F11V5 (= 11, DVI$C_ACP_F11V5),
       while "ODS5" includes anything from DVI$C_ACP_F11V5 on up.  See
       DVIDEF.H.
    */

#if defined( DVI$C_ACP_F11V5) && defined( NAML$C_MAXRSS)

    /* Check options and/or ODS level for down-case or preserve case. */
    down_case = 0;      /* Assume preserve case. */
    if ((vms_case_2 <= 0) && (vms_case_5 < 0))
    {
      /* Always down-case. */
      down_case = 1;
    }
    else if ((vms_case_2 <= 0) || (vms_case_5 < 0))
    {
      /* Down-case depending on ODS level.  (Use (full) external name.) */
      ods_level = file_sys_type( x);

      if (ods_level > 0)
      {
        /* Valid ODS level.  (Name (full) contains device.)
         * Down-case accordingly.
         */
        if (((ods_level < DVI$C_ACP_F11V5) && (vms_case_2 <= 0)) ||
         ((ods_level >= DVI$C_ACP_F11V5) && (vms_case_5 < 0)))
        {
          /* Down-case for this ODS level. */
          down_case = 1;
        }
      }
    }

#else /* defined( DVI$C_ACP_F11V5) && defined( NAML$C_MAXRSS) */

/* No case-preserved names are possible (VAX).  Do simple down-case check. */

    down_case = (vms_case_2 <= 0);

#endif /* defined( DVI$C_ACP_F11V5) && defined( NAML$C_MAXRSS) [else] */

    /* If down-casing, convert to lower case. */
    if (down_case != 0)
    {
      strlower( n);
    }
  }

  /* Remove simple ODS5 extended file name escape characters.
   * Note the presence of an escaped dot in the name field.
   */
  escaped_dot_in_name = eat_carets( n);

  if (isdir)
  {
    if (strcasecmp( (nn = n+ strlen( n)- DIR_TYPE_VER_LEN), DIR_TYPE_VER))
      error("directory not version 1");
    else
      if (pathput)
        strcpy( nn, "/");
      else
        *n = '\0';              /* directories are discarded with zip -rj */
  }
  else if (vmsver == 0)
  {
    /* If not keeping version numbers, truncate the name at the ";".
     * (A version should always be present, and should contain no
     * escaped characters, so a simple leftward search should work.)
     */
    if ((ext_dir_and_name = strrchr( n, ';')) != NULL)
      *ext_dir_and_name = '\0';
  }
  else if (vmsver > 1)
  {
    /* Keeping version numbers, but as ".nnn", not ";nnn". */
    if ((ext_dir_and_name = strrchr( n, ';')) != NULL)
      *ext_dir_and_name = '.';
  }

  /* 2013-03-23 SMS.
   * Added option -pv (prsrv_vms) to bypass adjustment of idiosyncratic
   * VMS file names.
   */
  /* 2008-10-15 SMS.
   * Changed the scheme here to remove a trailing dot only if there's
   * no escaped dot in the name field.  This covers a case like "x^.y.",
   * which had been losing its type-dot, and being archived as "x.y".
   * This method also covers the ".." ("^..") case naturally.
   *
   * 2008-07-17 SMS.
   * Added code to handle "." and ".." cases,
   * and, with "-ww", to remove a dot from "name..ver".
   */
  /* Remove a null-type (trailing "."), unless the name is null or there
   * is an unescaped dot in the name.
   */
  if ((prsrv_vms <= 0) &&               /* User wants VMS names adjusted. */
   (strlen( n) > 1) &&                  /* Won't leave a null result. */
   (escaped_dot_in_name == 0) &&        /* No escaped dot in name. */
   ((ext_dir_and_name = strrchr( n, '.')) != NULL))
  {
    if ((ext_dir_and_name[ 1] == '\0') &&       /* "name." -> "name",  */
     (ext_dir_and_name[ -1] != '/'))            /* but "." -> ".".     */
    {
      *ext_dir_and_name = '\0';
    }
    else if ((ext_dir_and_name[ 1] == ';') &&   /* "nm.;ver" -> "nm;ver",  */
     (ext_dir_and_name[ -1] != '/'))            /* but ".;ver" -> ".;ver". */
    {
      char *f = ext_dir_and_name+ 1;
      while (*ext_dir_and_name++ = *f++);
    }
    else if ((vmsver > 1) &&                    /* -ww, */
     (strlen( n) > 2) &&                        /* and enough characters, */
     (ext_dir_and_name[ -1] == '.') &&          /* "nm..ver" -> "nm.ver",  */
     (ext_dir_and_name[ -2] != '/'))            /* but "..ver" -> "..ver". */
    {
      char *f = ext_dir_and_name+ 1;
      while (*ext_dir_and_name++ = *f++);
    }
  }

  if (dosify)
    msname(n);

  /* Returned malloc'ed name */
  if (pdosflag)
    *pdosflag = dosflag;

  return n;
}


char *in2ex( char *n)
/* char *n;                internal file name */
/* Convert the zip file name to an external file name, returning the malloc'ed
   string or NULL if not enough memory. */
{
  char *x;              /* external file name */
  char *t;              /* scans name */
  int i;
  char chr;
  char *endp;
  char *last_slash;
  char *versionp;

#ifdef NAML$C_MAXRSS

  char buf[ NAML$C_MAXRSS+ 1];
  unsigned char prop;
  unsigned char uchr;
  char *last_dot;

#endif /* def NAML$C_MAXRSS */

  /* Locate the last slash. */
  last_slash = strrchr( n, '/');

/* If ODS5 is possible, replace escape carets in name. */

#ifdef NAML$C_MAXRSS

  endp = n+ strlen( n);

  /* Locate the version delimiter, if one is expected. */
  if (vmsver == 0)
  { /* No version expected. */
    versionp = endp;
  }
  else
  {
    if (vmsver > 1)
    { /* Expect a dot-version, ".nnn".  Locate the version ".".
         Temporarily terminate at this dot to allow the last-dot search
         below to find the last non-version dot.
      */
      versionp = strrchr( n, '.');
      if (versionp != NULL)     /* Can't miss. */
      {
        *versionp = '\0';
      }
    }
    else
    { /* Expect a semi-colon-version, ";nnn".  Locate the ";".  */
      versionp = strrchr( n, ';');
    }
    if ((versionp == NULL) || (versionp < last_slash))
    { /* If confused, and the version delimiter was not in the name,
         then ignore it.
      */
      versionp = endp;
    }
  }

  /* No escape needed for the last dot, if it's part of the file name.
     All dots in a directory must be escaped.
  */
  last_dot = strrchr( n, '.');

  if ((last_dot != NULL) && (last_slash != NULL) && (last_dot < last_slash))
  {
    last_dot = last_slash;
  }

  /* Replace the version dot if necessary. */
  if ((vmsver > 1) && (versionp != NULL) && (versionp < endp))
  {
    *versionp = '.';
  }

  /* Add ODS5 escape sequences.  Leave "/" and "?" for later.
     The name here looks (roughly) like: dir1/dir2/a.b
  */
  t = n;
  x = buf;
  while (uchr = *t++)
  {
    /* Characters in the version do not need escaping. */
    if (t <= versionp)
    {
      prop = char_prop[ uchr]& 31;
      if (prop)
      {
        if (prop& 4)
        { /* Dot. */
          /* 2008-07-16 SMS.
           * Note: Because "t" has been incremented, a "<=" test is
           * needed here to catch a dot immediately before "last_dot".
           */
          if (t <= last_dot)
          {
            /* Dot which must be escaped. */
            *x++ = '^';
          }
        }
        else if (prop& 8)
        {
          /* Character needing hex-hex escape. */
          *x++ = '^';
          *x++ = hex_digit[ uchr>> 4];
          uchr = hex_digit[ uchr& 15];
        }
        else
        {
          /* Non-dot character which must be escaped (and simple works).
             "?" gains the caret but remains "?" until later.
             ("/" remains (unescaped) "/".)
          */
          *x++ = '^';
          if (prop& 2)
          {
            /* Escaped space (represented as "^_"). */
            uchr = '_';
          }
        }
      }
    }
    *x++ = uchr;
  }
  *x = '\0';

  /* Point "n" to altered name buffer, and re-find the last slash. */
  n = buf;
  last_slash = strrchr( n, '/');

#endif /* def NAML$C_MAXRSS */

  if ((t = last_slash) == NULL)
  {
    if ((x = malloc(strlen(n) + 1 + DIR_PAD)) == NULL)
      return NULL;
    strcpy(x, n);
  }
  else
  {
    if ((x = malloc(strlen(n) + 3 + DIR_PAD)) == NULL)
      return NULL;

    /* Begin with "[". */
    x[ 0] = '[';
    i = 1;
    if (*n != '/')
    {
      /* Relative path.  Add ".". */
      x[ i++] = '.';
    }
    else
    {
      /* Absolute path.  Skip leading "/". */
      n++;
    }
    strcpy( (x+ i), n);

    /* Place the final ']'.  Remember where the name starts. */
    *(t = x + i + (t - n)) = ']';
    last_slash = t;

    /* Replace "/" with ".", and "?" with (now escaped) "/", in the
       directory part of the name.
    */
    while (--t > x)
    {
      chr = *t;
      if (chr == '/')
      {
        *t = '.';
      }
      else if (chr == '?')
      {
        *t = '/';
      }
    }

    /* Replace "?" with (now escaped) "/", in the non-directory part of
       the name.
    */
    while ((chr = *(++last_slash)) != '\0')
    {
      if (chr == '?')
      {
        *last_slash = '/';
      }
    }
  }

/* If case preservation is impossible (VAX, say), and down-casing, then
   up-case.  If case preservation is possible and wasn't done, then
   there's no way to ensure proper restoration of original case, so
   don't try.  This may differ from pre-3.0 behavior.
*/
#ifndef NAML$C_MAXRSS

  if (vms_case_2 <= 0)
  {
    strupper( x);
  }

#endif /* ndef NAML$C_MAXRSS */

  return x;
}

void stamp( char *f, ulg d)
/* char *f;                name of file to change */
/* ulg d;                  dos-style time to change it to */
/* Set last updated and accessed time of file f to the DOS time d. */
{
  int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year;
  char timbuf[24];
  static ZCONST char *month[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                 "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  struct VMStimbuf {
      char *actime;           /* VMS revision date, ASCII format */
      char *modtime;          /* VMS creation date, ASCII format */
  } ascii_times;

  ascii_times.actime = ascii_times.modtime = timbuf;

  /* Convert DOS time to ASCII format for VMSmunch */
  tm_sec = (int)(d << 1) & 0x3e;
  tm_min = (int)(d >> 5) & 0x3f;
  tm_hour = (int)(d >> 11) & 0x1f;
  tm_mday = (int)(d >> 16) & 0x1f;
  tm_mon = ((int)(d >> 21) & 0xf) - 1;
  tm_year = ((int)(d >> 25) & 0x7f) + 1980;
  sprintf(timbuf, "%02d-%3s-%04d %02d:%02d:%02d.00", tm_mday, month[tm_mon],
    tm_year, tm_hour, tm_min, tm_sec);

  /* Set updated and accessed times of f */
  if (VMSmunch(f, SET_TIMES, (char *)&ascii_times) != RMS$_NMF)
    zipwarn("can't set zipfile time: ", f);
}

ulg filetime( char *f, ulg *a, zoff_t *n, iztimes *t)
/* char *f;                name of file to get info on */
/* ulg *a;                 return value: file attributes */
/* zoff_t *n;              return value: file size */
/* iztimes *t;             return value: access, modific. and creation times */
/* If file *f does not exist, return 0.  Else, return the file's last
   modified date and time as an MSDOS date and time.  The date and
   time is returned in a long with the date most significant to allow
   unsigned integer comparison of absolute times.  Also, if a is not
   a NULL pointer, store the file attributes there, with the high two
   bytes being the Unix attributes, and the low byte being a mapping
   of that to DOS attributes.  If n is not NULL, store the file size
   there.  If t is not NULL, the file's access, modification and creation
   times are stored there as UNIX time_t values.
   If f is "-", use standard input as the file. If f is a device, return
   a file size of -1 */
{
  struct stat s;        /* results of stat() */
  /* convert to a malloc string dump FNMAX - 11/8/04 EG */
  char *name;
  int len = strlen(f);

  if (f == label) {
    if (a != NULL)
      *a = label_mode;
    if (n != NULL)
      *n = -2; /* convention for a label name */
    if (t != NULL)
      t->atime = t->mtime = t->ctime = label_utim;
    return label_time;
  }
  if ((name = malloc(len + 1)) == NULL) {
    ZIPERR(ZE_MEM, "filetime");
  }
  strcpy(name, f);
  if (name[len - 1] == '/')
    name[len - 1] = '\0';
  /* not all systems allow stat'ing a file with / appended */

  if (is_stdin || (!no_stdin && strcmp(f, "-") == 0)) {
    if (fstat(fileno(stdin), &s) != 0) {
      free(name);
      name = NULL;
      error("fstat(stdin)");
    }
  } else if (LSSTAT(name, &s) != 0) {
             /* Accept about any file kind including directories
              * (stored with trailing / with -r option)
              */
    free(name);
    return 0;
  }
  if (name != NULL)
    free(name);

  if (a != NULL) {
    *a = ((ulg)s.st_mode << 16) | !(s.st_mode & S_IWRITE);
    if (S_ISDIR( s.st_mode)) {
      *a |= MSDOS_DIR_ATTR;
    }
  }
  if (n != NULL)
    *n = (S_ISREG( s.st_mode) ? s.st_size : -1);
  if (t != NULL) {
    t->atime = s.st_mtime;
#ifdef USE_MTIME
    t->mtime = s.st_mtime;            /* Use modification time in VMS */
#else
    t->mtime = s.st_ctime;            /* Use creation time in VMS */
#endif
    t->ctime = s.st_ctime;
  }

#ifdef USE_MTIME
  return unix2dostime((time_t *)&s.st_mtime); /* Use modification time in VMS */
#else
  return unix2dostime((time_t *)&s.st_ctime); /* Use creation time in VMS */
#endif
}

int deletedir( char *dir_name)
/* char *dir_name;              directory to delete */

/* Delete the directory *dir_name if it is empty, do nothing otherwise.
   Return the result of delete().
   For VMS, dir_name must be in format [x.y]z.dir;1  (not [x.y.z]).
 */

 /* original code from Greg Roelofs, who horked it from Mark Edwards (unzip) */
 /*
   int r, len;
   char *s;

   len = strlen(d);
   if ((s = malloc(len + 34)) == NULL)
     return 127;

   system(strcat(strcpy(s, "set prot=(o:rwed) "), d));
   r = delete(d);
   free(s);
   return r;
  */

  /* 2009-11-28 SMS.
    Changed to use $QIOW instead of system( "set prot=(o:rwed) <dir>"),
    to eliminate annoying %SET-E-PRONOTCHG messages when SET PROTECTION
    failed.  The new scheme adds (S:D, O:D, G:D, W:D) if the original
    protection can be read.  Otherwise, it attempts to set the protection
    to (S:D, O:D, G:D, W:D), which will probably fail (but quietly).
  */

{
    int i;
    int ret;
    int sts;

    struct FAB        fab;           /* File Access Block */
    struct RAB        rab;           /* Record Access Block */
    struct NAMX_STRUCT nam;          /* Name Block */

    int atr_devchn;                  /* Disk device channel. */

    struct fibdef    atr_fib;        /* File Information Block. */

    /* Disk device name descriptor. */
    struct dsc$descriptor_s atr_devdsc =
     { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

    /* File Information Block descriptor. */
    struct dsc$descriptor atr_fibdsc =
     { sizeof( atr_fib), DSC$K_DTYPE_Z, DSC$K_CLASS_S, NULL };

    /* File name descriptor. */
    struct dsc$descriptor_s atr_fnam =
     { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

    /* IOSB for QIO[W] miscellaneous ACP operations. */
    struct
    {
        unsigned short  status;
        unsigned short  dummy;
        unsigned int    count;
    } atr_acp_iosb;

    /* Attribute item list: file protection, terminator. */
    struct atrdef atr_atr[ 2] =
     { { ATR$S_FPRO, ATR$C_FPRO, GVTC NULL },
       { 0, 0, GVTC NULL }
     };

    unsigned short atr_prot;

    /* Expanded name storage. */
    char exp_nam[ NAMX_MAXRSS];

    /* Special ODS5-QIO-compatible name storage. */
#ifdef NAML$C_MAXRSS
    char sys_nam[ NAML$C_MAXRSS];       /* Probably need less here. */
#endif /* NAML$C_MAXRSS */

    fab = cc$rms_fab;                   /* Initialize FAB. */
    rab = cc$rms_rab;                   /* Initialize RAB. */
    nam = CC_RMS_NAMX;                  /* Initialize NAM[L]. */

    rab.rab$l_fab = &fab;               /* Point RAB to FAB. */
    fab.FAB_NAMX = &nam;                /* Point FAB to NAM[L]. */

  /* Point the FAB/NAM[L] fields to the actual name and default name. */
#ifdef NAML$C_MAXRSS

    NAMX_DNA_FNA_SET( fab)

    /* Special ODS5-QIO-compatible name storage. */
    nam.naml$l_filesys_name = sys_nam;
    nam.naml$l_filesys_name_alloc = sizeof( sys_nam);

#endif /* NAML$C_MAXRSS */

    /* File specification. */
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = dir_name;
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen( dir_name);

    /* Expanded name storage. */
    nam.NAMX_ESA = exp_nam;
    nam.NAMX_ESS = sizeof(exp_nam);

    sts = sys$parse( &fab);
    if ((sts& STS$M_SEVERITY) == STS$M_SUCCESS)
    {
        /* Set the address in the device name descriptor. */
        atr_devdsc.dsc$a_pointer = &nam.NAMX_DVI[ 1];

        /* Set the length in the device name descriptor. */
        atr_devdsc.dsc$w_length = (unsigned short) nam.NAMX_DVI[ 0];

        /* Open a channel to the disk device. */
        sts = sys$assign( &atr_devdsc, &atr_devchn, 0, 0);
        if ((sts& STS$M_SEVERITY) == STS$M_SUCCESS)
        {
            /* Move the directory ID from the NAM[L] to the FIB.
               Clear the FID in the FIB, as we're using the name.
            */
            for (i = 0; i < 3; i++)
            {
                atr_fib.FIB$W_DID[i] = nam.NAMX_DID[i];
                atr_fib.FIB$W_FID[i] = 0;
            }

#ifdef NAML$C_MAXRSS

            /* Enable fancy name characters.  Note that "fancy" here does
               not include Unicode, for which there's no support elsewhere.
            */
            atr_fib.fib$v_names_8bit = 1;
            atr_fib.fib$b_name_format_in = FIB$C_ISL1;

            /* ODS5 Extended names used as input to QIO have peculiar
               encoding (perhaps to minimize storage?), so the special
               filesys_name result (typically containing fewer carets) must
               be used here.
            */
            atr_fnam.dsc$a_pointer = nam.naml$l_filesys_name;
            atr_fnam.dsc$w_length = nam.naml$l_filesys_name_size;

#else /* def NAML$C_MAXRSS */

            /* ODS2-only: Use the whole name. */
            atr_fnam.dsc$a_pointer = nam.NAMX_L_NAME;
            atr_fnam.dsc$w_length =
             nam.NAMX_B_NAME+ nam.NAMX_B_TYPE+ nam.NAMX_B_VER;

#endif /* def NAML$C_MAXRSS [else] */

            /* Set the address in the FIB descriptor. */
            atr_fibdsc.dsc$a_pointer = (void *) &atr_fib;

            /* Set the address of the prot word in the prot descriptor. */
            atr_atr[ 0].atr$l_addr = GVTC &atr_prot;

            /* Fetch the file (directory) protection attributes. */
            sts = sys$qiow( 0,                  /* event flag */
                            atr_devchn,         /* channel */
                            IO$_ACCESS,         /* function code */
                            &atr_acp_iosb,      /* IOSB */
                            0,                  /* AST address */
                            0,                  /* AST parameter */
                            &atr_fibdsc,        /* P1 = File Info Block */
                            &atr_fnam,          /* P2 = File name */
                            0,                  /* P3 = Rslt nm len */
                            0,                  /* P4 = Rslt nm str */
                            atr_atr,            /* P5 = Attributes */
                            0);                 /* P6 (not used) */

            /* Mask out or clear No-Delete bits. */
            if ((sts & STS$M_SEVERITY) == STS$K_SUCCESS)
            {
                atr_prot &= 0x7777;     /* Add (S:D, O:D, G:D, W:D) */
            }
            else
            {
                atr_prot = 0x7777;      /* Set (S:D, O:D, G:D, W:D) */
            }

            /* Try to modify the file (directory) attributes. */
            sts = sys$qiow( 0,                  /* event flag */
                            atr_devchn,         /* channel */
                            IO$_MODIFY,         /* function code */
                            &atr_acp_iosb,      /* IOSB */
                            0,                  /* AST address */
                            0,                  /* AST parameter */
                            &atr_fibdsc,        /* P1 = File Info Block */
                            &atr_fnam,          /* P2 = File name */
                            0,                  /* P3 = Rslt nm len */
                            0,                  /* P4 = Rslt nm str */
                            atr_atr,            /* P5 = Attributes */
                            0);                 /* P6 (not used) */

            if ((sts & STS$M_SEVERITY) == STS$K_SUCCESS)
            {
                sts = atr_acp_iosb.status;
            }
        }
    }

    ret = delete( dir_name);
    return ret;
}
