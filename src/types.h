/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    types.h
*/

#ifndef __TYPES_H
#define __TYPES_H

#include <cinttypes>

#ifndef __BIT_TYPES_DEFINED__
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
#endif

//
// VC++ uses the I specifier for the same purpose as defined for the z specifier
// by C99. Use a macro similar to PRIx to print size_t values.
//
#if defined(_MSC_VER)
#define PRI_SZ   "Iu"
#else
#define PRI_SZ   "zu"
#endif

#endif // __TYPES_H

