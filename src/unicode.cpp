/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   unicode.cpp
*/
#include "pch.h"

#include "unicode.h"
#include <cstddef>

///
/// The wide character string is expected to contain UCS-2 or UCS-4 characters, depending
/// on `wchar_t` implementation.
///
/// If `slen` includes a wide null character, it will be converted into a null character.
/// in the output buffer. Otherwise, no null character will be inserted into the output.
///
/// Returns the size of the UTF-8 string, in bytes. If a null character was included in
/// the input, it will be counted in the returned size.
///
/// Only complete UTF-8 character sequences are placed into the output buffer. If there
/// is not enough room for the entire character, the conversion stops and the number of
/// characters converted so far is returned.
///
size_t ucs2utf8(const wchar_t *cp, size_t slen, char *out, size_t bsize)
{
   char *op = out;
   
   // convert one wide character at a time
   while(slen && bsize-(op-out) >= ucs2utf8size(*cp))
      op += ucs2utf8(*cp++, op), slen--;
   
   // return the size of the UTF-8 string, without the null character
   return op - out;
}
