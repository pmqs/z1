README_VC10.txt

This directory contains projects for building and using the Zip 3.1
static and dynamic libraries (LIB and DLL) using Visual Studio 2010.
(The projects should convert to later versions of Visual Studio.)

See the Readme and other files in the various directories under windll/
for more information on the LIB and DLL libraries and the examples
provided for more on how to use the LIB and DLL.

See WhatsNew_DLL_LIB.txt in the windll directory and the comments in
the api.h and api.c source files for details on the updated LIB/DLL
interfaces.

The libraries built using the zip32_lib and zip32_dll projects in the
windll/vc10 directory are:

Static linking:

  zip32_lib.lib    - This is the RELEASE version of the 32-bit static
                     library.

  zip32_libd.lib   - This is the DEBUG version of the 32-bit static
                     library.

  zip64_lib.lib    - This is the RELEASE version of the 64-bit static
                     library.

  zip64_libd.lib   - This is the DEBUG version of the 64-bit static
                     library.

  NOTE:  Be sure to use the DEBUG version of the static library if
         the main app is compiled with DEBUG, and the RELEASE version
         of the static library if you compile your app with RELEASE.
         Mixing DEBUG and RELEASE versions can cause unexpected crashes.

Dynamic linking:

  zip32_dll.lib    - This is a static library that calls the DLL.  Link
                     this one to create the DLL references in your
                     project.

  zip32_dll.dll    - This is the dynamic library.  When you link to
                     zip32_dll.lib at link time, zip32_dll.dll needs to
                     be in the project directory or in a directory
                     listed in PATH.  You can also load this dynamically
                     without using zip32_dll.lib to reference it.

  zip64_dll.lib    - This is the 64-bit version of zip32_dll.lib.
  
  zip64_dll.dll    - This is the 64-bit version of zip32_dll.dll.

The 32-bit versions should run fine on either 32-bit or 64-bit systems.
The 64-bit versions can only be used on 64-bit systems.

All references to the bzip2 library should now be removed.  That code is
now included in Zip and compiled with Zip, removing the need for including
a reference to an external bzip2 library.  On Windows, the Zip library is
all you need.

By default, building the solution builds all these (for EITHER Debug or
Release and for either 32-bit or 64-bit, as selected).  However, if you
are adding AES encryption, be sure to read INSTALL for important build
instructions.

3 March 2019

