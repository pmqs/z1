# Microsoft Developer Studio Project File - Name="zip32_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=zip32_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zip32_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zip32_dll.mak" CFG="zip32_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zip32_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "zip32_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zip32_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "zip32_dll___Win32_Release"
# PROP BASE Intermediate_Dir "zip32_dll___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Release"
# PROP Intermediate_Dir "zip32_dll___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Zp4 /MT /W3 /GX /O2 /I "..\..\.." /I "..\..\..\WINDLL" /I "..\..\..\BZIP2" /I "..\..\..\WIN32" /D "LZMA_SUPPORT" /D "PPMD_SUPPORT" /D "BZIP2_SUPPORT" /D "BZ_NO_STDIO" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "NO_ASM" /D "WINDLL" /D "MSDOS" /FR /YX /FD /c
# ADD CPP /nologo /Zp4 /MT /W3 /GX /O2 /I "..\..\.." /I "..\..\..\WINDLL" /I "..\..\..\BZIP2" /I "..\..\..\WIN32" /D "WINDLL" /D "MSDOS" /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D "NO_ASM" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "WIN32"
# ADD RSC /l 0x409 /d "NDEBUG" /d "WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT BASE LINK32 /map
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "zip32_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "zip32_dll___Win32_Debug"
# PROP BASE Intermediate_Dir "zip32_dll___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "zip32_dll___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Zp4 /MTd /W3 /Gm /GX /ZI /Od /I "..\..\.." /I "..\..\..\WINDLL" /I "..\..\..\BZIP2" /I "..\..\..\WIN32" /I "..\..\..\ZIP" /D "LZMA_SUPPORT" /D "PPMD_SUPPORT" /D "BZIP2_SUPPORT" /D "BZ_NO_STDIO" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "NO_ASM" /D "WINDLL" /D "MSDOS" /D "USE_ZIPMAIN" /D "_DEBUG" /FR /YX /FD /c
# ADD CPP /nologo /Zp4 /MTd /W3 /Gm /GX /ZI /Od /I "..\..\.." /I "..\..\..\WINDLL" /I "..\..\..\BZIP2" /I "..\..\..\WIN32" /I "..\..\..\ZIP" /D "WINDLL" /D "MSDOS" /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D "NO_ASM" /D "USE_ZIPMAIN" /D "_DEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "WIN32" /d "_DEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "WIN32" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /map
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "zip32_dll - Win32 Release"
# Name "zip32_dll - Win32 Debug"
# Begin Source File

SOURCE=..\..\..\wrap\aescrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\aeskey.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\aestab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\api.c
# End Source File
# Begin Source File

SOURCE=..\..\..\api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\blocksort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\bzlib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\bzlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\bzlib_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\compress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\control.h
# End Source File
# Begin Source File

SOURCE=..\..\..\crc32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\crctable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\decompress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\deflate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\fileenc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\fileio.c
# End Source File
# Begin Source File

SOURCE=..\..\..\globals.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\hmac.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\huffman.c
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\LzFind.c
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\LzFind.h
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\LzmaEnc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\LzmaEnc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\win32\nt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\Ppmd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\Ppmd8.c
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\Ppmd8.h
# End Source File
# Begin Source File

SOURCE=..\..\..\szip\Ppmd8Enc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\prng.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\pwd2key.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bzip2\randtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wrap\sha1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\trees.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ttyio.c
# End Source File
# Begin Source File

SOURCE=..\..\..\util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\win32\win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\win32\win32i64.c
# End Source File
# Begin Source File

SOURCE=..\..\..\win32\win32zip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\windll\windll.c
# End Source File
# Begin Source File

SOURCE=..\..\windll.h
# End Source File
# Begin Source File

SOURCE=..\..\..\windll\windll.rc
# End Source File
# Begin Source File

SOURCE=..\..\..\windll\windll32.def
# End Source File
# Begin Source File

SOURCE=..\..\..\zbz2err.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zipfile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zipup.c
# End Source File
# End Target
# End Project
