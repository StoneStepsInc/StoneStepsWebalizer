/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_path.h
*/
#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include "tstring.h"

#ifdef _WIN32
#define F_OK 00      // existance
#define W_OK 02      // write access
#define R_OK 04      // read access
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#else
#ifndef PATH_MAX
#define PATH_MAX 255
#endif
#endif

#ifdef _WIN32
#define PATH_SEP_CHAR    '\\'
#else
#define PATH_SEP_CHAR    '/'
#endif

bool is_abs_path(const char *path);
string_t get_cur_path(void);
string_t make_path(const char *base, const char *path);

#endif // UTIL_PATH_H
