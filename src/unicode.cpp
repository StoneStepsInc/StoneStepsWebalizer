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

size_t ucs2utf8(const wchar_t *cp, char *out, size_t bsize)
{
   char *op = out;
   
   // convert one wide character at a time, up to the null character
   while(*cp && bsize-(op-out) >= ucs2utf8size(*cp))
      op += ucs2utf8(*cp++, op);
   
   // terminate the new string, but do not advance the pointer
   *op = 0;
   
   // return the size of the UTF-8 string, not including the null character
   return op - out;
}

bool isutf8str(const char *str)
{
   const char *cp1 = str;
   size_t csize;

   // look for a null character and count characters
   while(*cp1) {
      if((csize = utf8size(cp1)) == 0)
         return false;
      cp1 += csize;
   }

   return true;
}

bool isutf8str(const char *str, size_t slen)
{
   const char *cp1 = str;
   size_t csize;

   // accept empty strings
   if(!slen)
      return true;

   // walk UTF-8 characters up to slen
   while(cp1-str < (std::ptrdiff_t) slen) {
      if((csize = utf8size(cp1)) == 0)
         return false;
      cp1 += csize;
   }

   // slen may be less than the actual number of bytes we found
   return cp1-str == (std::ptrdiff_t) slen;
}

