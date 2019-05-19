/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   unicode.cpp
*/
#include "pch.h"

#include "unicode.h"
#include <cstddef>

size_t ucs2utf8(const wchar_t *cp, size_t slen, char *out, size_t bsize)
{
   char *op = out;
   
   // convert one wide character at a time
   while(slen && bsize-(op-out) >= ucs2utf8size(*cp))
      op += ucs2utf8(*cp++, op), slen--;
   
   // return the size of the UTF-8 string, without the null character
   return op - out;
}
