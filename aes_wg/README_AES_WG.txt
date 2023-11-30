                        README_AES_WG.txt
                        -----------------

      Overview
      --------

   The Info-ZIP programs UnZip (version 6.1 and later) and Zip (version
3.1 and later) offer optional support for Advanced Encryption Standard
(AES) encryption, a relatively strong encryption method.  Traditional
zip encryption, in contrast, is now considered weak and relatively easy
to crack.  The below describes our AES implementation as well as
provides terms of use.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      WARNING
      -------

    The Info-ZIP AES source kit (iz_aes_wg.zip) is subject to US
    export control laws.  BEFORE downloading OR using any version
    of this kit, read the following Encryption Notice.  You agree
    to follow these terms (as well as the terms of the Info-ZIP
    license) when you download and/or use our AES source kit.

      Encryption Notice
      -----------------

    This distribution includes cryptographic software.  The country
    in which you currently reside may have restrictions on the import,
    possession, use, and/or re-export to another country, of encryption
    software.  BEFORE using any encryption software, please check your
    country's laws, regulations and policies concerning the import,
    possession, or use, and re-export of encryption software, to see if
    this is permitted. See <http://www.wassenaar.org/> for more
    information.

    The U.S. Government Department of Commerce, Bureau of Industry and
    Security (BIS), has classified this software as Export Commodity
    Control Number (ECCN) 5D002.C.1, which includes information
    security software using or performing cryptographic functions and
    makes it eligible for export under the License Exception ENC
    Technology Software Unrestricted (TSU) exception.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Description
      -----------

   The Info-ZIP AES source kit adds to Zip and UnZip support for
WinZip-compatible AES encryption using AES encryption code supplied
by Brian Gladman.  Our implementation is based on the WinZip
implementation and is referred to as Info-ZIP AES WinZip/Gladman
(IZ_AES_WG).  The WinZip AES scheme is described in:

      http://www.winzip.com/aes_info.htm

Briefly, the WinZip AES scheme uses compression method 99 to flag the
AES-encrypted entries.  (In contrast, PKZIP AES encryption uses an
incompatible scheme with different archive data structures.  However,
current versions of PKZIP may also be able to process WinZip AES
encrypted archive entries.)

   Our AES implementation supports 128-, 192-, and 256-bit keys.  See
the various discussions of WinZip AES encryption on the Internet for
more on the security of the WinZip AES encryption implementation.  We
may add additional encryption methods in the future.

   IZ_AES_WG is based on an AES source kit provided by Brian Gladman,
which used to be at:

      http://gladman.plushost.co.uk/oldsite/cryptography_technology/
      fileencrypt/files.zip

However, this site is no longer active.  Dr. Gladman's current site
is:

      http://www.gladman.me.uk/AES

and he now makes his AES code available on GITHUB:

      https://github.com/BrianGladman

The contents of the old discontinued kit we use is fully included in
our kit, with the exception that we added one header file
("brg_endian.h") from a slightly newer kit that used to be at:

      http://gladman.plushost.co.uk/oldsite/AES/aes-src-11-01-11.zip

to address some minor compatibility issues.

   There are two main reasons we are providing essentially a repackaging
of the Gladman AES code.  First, some minor changes were needed to
improve its portability to different hardware and operating systems.
Second, the version of Gladman code used, though recommended by WinZip,
is old.  We felt it necessary to capture this version of the Gladman AES
code with our changes and make it available.  (Indeed, it may be hard
to find this code elsewhere at this point.)  We are currently looking at
newer versions of the Gladman AES code and may update our implementation
in the future.

   The portability-related changes to the original Gladman code include:

      Use of <string.h> instead of <memory.h>.

      Use of "brg_endian.h" for endian determination.

      Changing "brg_endian.h" to work with GNU C on non-Linux systems,
      and on SunOS 4.x systems.

      #include <limits.h> instead of "limits.h" in aes.h.

      Changing some "long" types to "int" or "sha1_32t" in hmac.c and
      hmac.h to accommodate systems (like Mac OS X on Intel) where a
      64-bit "long" type caused bad results.

Comments in the code identify the changes.  (Look for "Info-ZIP".)  The
original files are preserved in an "orig" subdirectory, for reference.

   The name "IZ_AES_WG" (Info-ZIP AES WinZip/Gladman) is used by
Info-ZIP to identify our implementation of WinZip AES encryption of Zip
entries, using encryption code supplied by Dr. Gladman.  We also refer
to our Info-ZIP implementation as AES WG.  WinZip is a registered
trademark of WinZip International LLC.  PKZIP is a registered trademark
of PKWARE, Inc.

   The source code files from Dr. Gladman are subject to the LICENSE
TERMS at the top of each source file.  The entire Info-ZIP AES source
kit is provided under the Info-ZIP license, a copy of which is included
in LICENSE.txt.  The latest version of the Info-ZIP license should be
available at:

      http://www.info-zip.org/license.html

NOTE:  The Info-ZIP AES source kit is designed for use with our UnZip
and Zip kits ONLY.  Any other use is unsupported and not recommended.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Building UnZip and Zip with AES WG Encryption Support
      -----------------------------------------------------

   First, obtain the Info-ZIP AES source kit (AES kit), which is
packaged separately from the basic UnZip and Zip source kits for export
control reasons.  On the Info-ZIP FTP server, the kit should be found
in:

      ftp://ftp.info-zip.org/pub/infozip/crypt/

The latest kit should be:

      ftp://ftp.info-zip.org/pub/infozip/crypt/iz_aes_wg.zip

Different UnZip and Zip versions may need particular kit versions, so
before downloading a particular IZ_AES_WG source kit, consult the
file INSTALL in the UnZip and Zip source kits.  In general, the latest
AES kit should work with the latest UnZip and Zip kits.

   The build instructions (INSTALL) in the UnZip and Zip source kits
describe how to unpack the AES kit and build UnZip and Zip with AES WG
encryption.

   The UnZip and Zip README files have additional general information on
AES encryption, and the UnZip and Zip manual pages provide the details
on how to use AES encryption.  The extended help option (-hh) of UnZip
and Zip also provides guidance on using encryption and the -Y option.

   Be aware that some countries or jurisdictions may restrict who may
download and use strong encryption source code and binaries.
Prospective users are responsible for determining whether they are
legally allowed to download and use this encryption software.

   Note that many of the servers that distribute Info-ZIP software are
situated in the United States, and so you should assume that the
restrictions in the above Encryption Notice apply.  The file
USexport_AES_WG.msg included in the kit provides the US export
exception specifics.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Acknowledgements
      ----------------

   We're grateful to Dr. Gladman for providing the AES encryption code.
Any problems involving AES WG encryption in Info-ZIP programs should be
reported to the Info-ZIP team, not to Dr. Gladman.  However, any questions
on AES encryption or decryption algorithms, or regarding Gladman's code
(except as we modified and use it) should be addressed to Dr. Gladman.

   We're grateful to WinZip for making the WinZip AES specification
available, and providing the detailed information needed to implement
it.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      IZ_AES_WG Version History
      -------------------------

      1.5  2016-07-15  Updated USexport_AES_WG.msg to current version.
                       Updated zip-comment.txt.  Updated this file
                       (README_AES_WG.txt) to include legal notices
                       and updates.  [EG]
      1.4  2015-04-03  Changed "long" types to "int" for counters, and
                       to "sha1_32t" for apparent 32-bit byte groups,
                       where a 64-bit "long" type caused bad results (on
                       Mac OS X, Intel).  (hmac.c, hmac.h) [SMS]
      1.3  2013-11-18  Renamed USexport.msg to USexport_AES_WG.msg to
                       distinguish it from the Traditional encryption
                       notice, USexport.msg.  [SMS]
      1.2  2013-04-12  Avoid <sys/isa_defs.h> on __sun systems with
                       __sparc defined (for SunOS 4.x).  (brg_endian.h)
                       [SMS]
      1.1  2012-12-31  #include <limits.h> instead of "limits.h" in
                       aes.h (for VAX C).  [SMS]
      1.0  2011-07-07  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
                       US Department of Commerce BIS notified.
      0.5  2011-07-07  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.4  2011-06-25  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.3  2011-06-22  Initial beta version.  [SMS, EG]
      0.2  2011-06-20  Minor documentation updates.  [EG]
      0.1  2011-06-17  Initial alpha version.  [SMS]

