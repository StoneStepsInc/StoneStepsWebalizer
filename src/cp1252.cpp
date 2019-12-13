/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   cp1252.cpp
*/
#include "pch.h"

#include "cp1252.h"
#include "unicode.h"

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
         return nullptr;
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
         return nullptr;
   }

   return out;
}

const string_t& cp1252utf8(const string_t& str, string_t& out)
{
   extern const wchar_t CP1252_UCS2[];

   size_t osize = 0;

   // nothing to do if the input string is empty
   if(str.isempty()) {
      out.reset();
      return out;
   }

   // find out the size of the UTF-8 string
   for(size_t i = 0; i < str.length(); i++)
      osize += ucs2utf8size(CP1252_UCS2[(unsigned char) str[i]]);

   string_t::char_buffer_t omem = out.detach();

   // check if we can fit the new string in the old block, including the null character
   if(omem.capacity() <= osize)
      omem.resize(osize + 1, 0);

   // convert the input string to UTF-8
   if(!cp1252utf8(str, omem, omem.capacity()))
      return out;

   // attach the memory block to the output string
   out.attach(std::move(omem), osize);

   return out;
}
