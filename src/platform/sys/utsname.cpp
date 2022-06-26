/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    utsname.cpp
*/
#include "../../pch.h"

#include <windows.h>
#include <errno.h>
#include <cstdio>
#include "utsname.h"

//
// Microsoft deprecated GetVersion and GetVersionEx functions in favor of 
// VerifyVersionInfo, which allows applications to check whether they are
// compatible with a current version of Windows, but makes it impossible 
// to report Windows version. Disable this warning until this functionality
// is reworked.
//
#pragma warning(disable: 4996)   // 'GetVersionExA': was declared deprecated

extern "C" int uname(struct utsname *name)
{
   OSVERSIONINFO verinfo = {0};
   DWORD buffsize;

   if(name == nullptr)
      return EFAULT;

   buffsize = SYS_NMLN;
   GetComputerName(name->machine, &buffsize);
   strcpy(name->nodename, name->machine);

   verinfo.dwOSVersionInfoSize = sizeof(verinfo);
   GetVersionEx(&verinfo);
   
   sprintf(name->version, "%d.%d.%d", verinfo.dwMajorVersion, verinfo.dwMinorVersion, verinfo.dwBuildNumber);
   strcpy(name->release, name->version);

   sprintf(name->sysname, "Windows");

   return 0;
}
