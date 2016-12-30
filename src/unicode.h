/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   unicode.h
*/

#ifndef UNICODE_H
#define UNICODE_H

#include "tstring.h"

//
// Converts the UCS-2 character to a UTF-8 sequence and returns the number of bytes 
// in the output.
//
// [Unicode v5 ch.3 p.103]
//
// Scalar Value                  First Byte  Second Byte Third Byte  Fourth Byte
// 00000000 0xxxxxxx             0xxxxxxx
// 00000yyy yyxxxxxx             110yyyyy    10xxxxxx
// zzzzyyyy yyxxxxxx             1110zzzz    10yyyyyy    10xxxxxx
// 000uuuuu zzzzyyyy yyxxxxxx    11110uuu    10uuzzzz    10yyyyyy    10xxxxxx
//
inline size_t ucs2utf8(wchar_t wchar, char *out)
{
   if(!out)
      return 0;

   // 1-byte character
   if(wchar <= L'\x7F') {
      *out++ = (char) (wchar & L'\x7F');
      return 1;
   }
   
   // 2-byte sequence
   if(wchar <= L'\x7FF') {
      *out++ = (char) ((wchar >> 6) | L'\xC0');
      *out++ = (char) ((wchar & L'\x3F') | L'\x80');
      return 2;
   }

   // 3-byte sequence
   if(wchar <= L'\xFFFF') {
      *out++ = (char) ((wchar >> 12) | L'\xE0');
      *out++ = (char) (((wchar >> 6) & L'\x3F') | L'\x80');
      *out = (char) ((wchar & L'\x3F') | L'\x80');
      return 3;
   }

   // GCC implements wchar_t as a 4-byte entity on many platforms
#if WCHAR_MAX > 0xFFFF
   //
   // GCC v4.8.3 incorrectly reports L'\x10FFFF' as a literal that is too long 
   // for its type. Later versions of GCC fix this issue (checked v6.3.0), but 
   // in the meantime cast wchar explicitly to a 32-bit integer to avoid this
   // warning.
   //
   // 4-byte sequence
   if((uint32_t) wchar <= 0x10FFFF) {
      *out++ = (char) ((wchar >> 18) | L'\xF0');
      *out++ = (char) (((wchar >> 12) & L'\x3F') | L'\xE0');
      *out++ = (char) (((wchar >> 6) & L'\x3F') | L'\x80');
      *out = (char) ((wchar & L'\x3F') | L'\x80');
      return 4;
   }
#endif

   // no conversion is done
   return 0;
}

//
// Returns the size of a UTF-8 character corresponding to the specified UCS-2
// character.
//
inline size_t ucs2utf8size(wchar_t wchar)
{
   return (wchar <= L'\x7F') ? 1 : (wchar <= L'\x7FF') ? 2 : (wchar <= L'\xFFFF') ? 3 : 4;
}

//
// Converts char to unsigned char and checks if the argument is within the range
// defined by the template arguments.
//
template <unsigned char lo, unsigned char hi>
inline bool in_range(unsigned char ch) 
{
   return ch >= lo && ch <= hi;
}

//
// Returns the number of bytes in a UTF-8 character or zero if the sequence has any
// bytes outside of the ranges defined for UTF-8 characters. 
//
// IMPORTANT: this function does not check whether the UTF-8 character is valid and
// will just count bytes based on the ranges described below. For example, the 0x01
// is not a valid character.
//
// [Unicode v5 ch.3 p.104]
//
// Code Points          First Byte  Second Byte Third Byte  Fourth Byte
// U+0000   .. U+007F   00..7F
// U+0080   .. U+07FF   C2..DF      80..BF
// U+0800   .. U+0FFF   E0          A0..BF      80..BF
// U+1000   .. U+CFFF   E1..EC      80..BF      80..BF
// U+D000   .. U+D7FF   ED          80..9F      80..BF
// U+E000   .. U+FFFF   EE..EF      80..BF      80..BF
// U+10000  .. U+3FFFF  F0          90..BF      80..BF      80..BF
// U+40000  .. U+FFFFF  F1..F3      80..BF      80..BF      80..BF
// U+100000 .. U+10FFFF F4          80..8F      80..BF      80..BF
//
inline size_t utf8size(const char *cp)
{
   if(!cp)
      return 0;

   // 1-byte character (including zero terminator)
   if(in_range<'\x00', '\x7F'>(*cp))
      return 1;
   
   // 2-byte sequences
   if(in_range<'\xC2', '\xDF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) ? 2 : 0;
   
   // 3-byte sequences
   if(*cp == '\xE0')
      return in_range<'\xA0', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xE1', '\xEC'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(*cp == '\xED')
      return in_range<'\x80', '\x9F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xEE', '\xEF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;

   // 4-byte sequences
   if(*cp == '\xF0')
      return in_range<'\x90', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;

   if(in_range<'\xF1', '\xF3'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
      
   if(*cp == '\xF4')
      return in_range<'\x80', '\x8F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
   
   return 0;
}

//
// Converts a UCS-2 string to a UTF-8 string. Returns the size of the result, in bytes,
// not including the null character, if one was inserted. If slen was provided, all slen 
// characters will be converted, whether they contain a null character or not. 
//
size_t ucs2utf8(const wchar_t *str, size_t slen, char *out, size_t bsize);
size_t ucs2utf8(const wchar_t *str, char *out, size_t bsize);

//
// Checks if str is a valid UTF-8 string
//
bool isutf8str(const char *str);
bool isutf8str(const char *str, size_t slen);

//
// Converts Windows 1252 code page string to a UTF-8 string. Returns out if conversion
// was successful or NULL otherwise. The third function always returns out, but clears
// it if the conversion failed.
//
char *cp1252utf8(const char *str, char *out, size_t bsize, size_t *olen = NULL);
char *cp1252utf8(const char *str, size_t slen, char *out, size_t bsize, size_t *olen = NULL);
const string_t& cp1252utf8(const string_t& str, string_t& out);

#endif // UNICODE_H
