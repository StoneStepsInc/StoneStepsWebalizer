/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

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

char *cp1252utf8(const char *str, char *out, size_t bsize, size_t *olen)
{
   return cp1252utf8(str, SIZE_MAX, out, bsize, olen);
}

char *cp1252utf8(const char *str, size_t slen, char *out, size_t bsize, size_t *olen)
{
   extern const wchar_t CP1252_UCS2[];

   char *ucp = out;

   for(size_t i = 0; i < slen && str[i]; i++) {
      // make sure we have enough room in the buffer for the next UTF-8 character
      if(ucs2utf8size(CP1252_UCS2[(unsigned char) str[i]]) > bsize - (ucp - out)) {
         if(olen)
            *olen = 0;
         return NULL;
      } 

      // output the character
      ucp += ucs2utf8(CP1252_UCS2[(unsigned char) str[i]], ucp);
   }

   // update the output string length (not including the null character)
   if(olen)
      *olen = ucp - out;

   // if there's room in the output buffer, add a null character
   if(bsize - (ucp - out))
      *ucp = 0;
   else {
      // can't use the result without a null character and output length
      if(!olen)
         return NULL;
   }

   return out;
}

const string_t& toutf8(const string_t& str, string_t& out)
{
   extern const wchar_t CP1252_UCS2[];

   size_t osize = 0, bsize = 0;

   // return if the input string is empty or already contains only UTF-8 characters
   if(str.isempty() || isutf8str(str)) {
      out.reset();
      return str;
   }

   // find out the size of the UTF-8 string
   for(size_t i = 0; i < str.length(); i++)
      osize += ucs2utf8size(CP1252_UCS2[(unsigned char) str[i]]);

   string_t::char_buffer_t omem = out.detach();

   // check if we can fit the new string in the old block, including the null character
   if(omem.capacity() <= osize)
      omem.resize(osize + 1);

   // convert the input string to UTF-8
   if(!cp1252utf8(str, omem, omem.capacity()))
      return out;

   // attach the memory block to the output string
   out.attach(omem, osize);

   return out;
}
