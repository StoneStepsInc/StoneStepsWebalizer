/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   pch.h
*/
#ifndef __PCH_H
#define __PCH_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <wchar.h>
#include <limits.h>

#include "util.h"
#include "tstring.h"
#include "webalizer.h"
#include "config.h"
#include "parser.h"
#include "logrec.h"

//
// VC9 generates warnings about deprecated POSIX functions, such as getcwd, 
// unless they begin with an underscore (i.e. _getcwd). Redefining these
// names (i.e. #define getcwd _getcwd) only works if #define is in the same 
// source file where the name was used. If defined in a header file, VC9 
// continues to generate the warning.
//
#if _MSC_VER >= 1500
#pragma warning(disable: 4996)   // The POSIX name for this item is deprecated...
#endif

#endif // __PCH_H
