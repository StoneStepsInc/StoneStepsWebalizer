/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   cp1252.h
*/
#ifndef CP1252_H
#define CP1252_H

#include "tstring.h"
#include <cstddef>

//
// Converts Windows 1252 code page string to a UTF-8 string. Returns out if conversion
// was successful or NULL otherwise. The third function always returns out, but clears
// it if the conversion failed.
//
char *cp1252utf8(const char *str, char *out, size_t bsize, size_t *olen = NULL);
char *cp1252utf8(const char *str, size_t slen, char *out, size_t bsize, size_t *olen = NULL);
const string_t& cp1252utf8(const string_t& str, string_t& out);

#endif // CP1252_H
