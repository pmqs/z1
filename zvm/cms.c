/*
  Copyright (c) 1990-2016 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-2 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 * VM/CMS specific things.
 */

#include "zip.h"

int procname(n, caseflag)
char *n;                /* name to process */
int caseflag;           /* true to force case-sensitive match */
/* Process a name or sh expression to operate on (or exclude).  Return
   an error code in the ZE_ class. */
{
  FILE *stream;

  if (is_stdin || (!no_stdin && strcmp(n, "-") == 0))   /* if compressing stdin */
    return newname(n, 0, caseflag);
  else {
     if ((stream = fopen(n, "r")) != (FILE *)NULL)
        {
        fclose(stream);
        return newname(n, 0, caseflag);
        }
     else return ZE_MISS;
  }
  return ZE_OK;
}
