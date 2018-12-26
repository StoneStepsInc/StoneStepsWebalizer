/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_path.cpp
*/
#include "pch.h"

#include "util_path.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <cctype>

bool is_abs_path(const char *path)
{
   if(path == NULL || *path == 0)
      return false;

   if(*path == '/')
      return true;

#ifdef _WIN32
   // check for \dir\file or \\server\share\file
   if(*path == '\\')
      return true;

   // check for c:\dir\file
   if(isalpha(*path) && path[1] == ':')
      return true;
#endif
   
   return false;
}

string_t get_cur_path(void)
{
   char buffer[PATH_MAX+1];
   string_t path;

   if(!getcwd(buffer, (int) (sizeof(buffer)-1)))
      return path;

   path = buffer;

   return path;
}

string_t make_path(const char *base, const char *path)
{
   const char *cp1;
   string_t result;

   // if there's no path, return base
   if(path == NULL || *path == 0)
      return result = base;

   // if there's no base, return path
   if(base == NULL || *base == 0)
      return result = path;

   // if path is an absolute path, return it 
   if(is_abs_path(path))
      return result = path;

   // otherwise, combine base and path
   result = base;

   cp1 = &base[strlen(base) - 1];

   if(*cp1 != '/' && *cp1 != '\\')
      result += PATH_SEP_CHAR;

   return result + path;
}

