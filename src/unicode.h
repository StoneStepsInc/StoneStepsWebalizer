/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   unicode.h
*/

#ifndef UNICODE_H
#define UNICODE_H

#include <cstddef>

///
/// @brief  Converts a UCS-2 character to a UTF-8 sequence and returns the number 
///         of bytes in the output.
///
/// [Unicode v5 ch.3 p.103]
/// ```
/// Scalar Value                  First Byte  Second Byte Third Byte  Fourth Byte
/// 00000000 0xxxxxxx             0xxxxxxx
/// 00000yyy yyxxxxxx             110yyyyy    10xxxxxx
/// zzzzyyyy yyxxxxxx             1110zzzz    10yyyyyy    10xxxxxx
/// 000uuuuu zzzzyyyy yyxxxxxx    11110uuu    10uuzzzz    10yyyyyy    10xxxxxx
/// ```
///
/// @note   Wide characters on Windows are encoded as UCS-2 and represent characters from the
/// Basic Multilingual Plane of ISO-10646. However, other compilers may encode wide characters
/// in other forms, some of which may not even work because this function relies on a wide
/// character being represented either in UCS-2 or UCS-4 or UTF-32. If wide characters are
/// represented as UTF-16, orphaned surrogate pairs will be converted to the replacement
/// character (U+FFFD) because there is no way to know the other part of that surrogate pair.
/// This restriction is not material for this application, but this function should not be used
/// in applications that are intended to work with the full spectrum of some compiler-specific
/// implementation of wide characters.
///
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

   // convert orphaned high/low surrogate pairs to a replacement character (U+FFFD)
   if(wchar >= L'\xD800' && wchar <= L'\xDBFF' || wchar >= L'\xDC00' && wchar <= L'\xDFFF') {
      *out++ = '\xEF';
      *out++ = '\xBF';
      *out = '\xBD';
      return 3;
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
      *out++ = (char) (((wchar >> 12) & L'\x3F') | L'\x80');
      *out++ = (char) (((wchar >> 6) & L'\x3F') | L'\x80');
      *out = (char) ((wchar & L'\x3F') | L'\x80');
      return 4;
   }
#endif

   // no conversion is done
   return 0;
}

///
/// @brief  Returns the size of a UTF-8 character corresponding to the specified 
///         UCS-2 character.
///
inline size_t ucs2utf8size(wchar_t wchar)
{
   if(wchar <= L'\x7F')
      return 1;
      
   if(wchar <= L'\x7FF')
      return 2;
      
   // orphaned surrogate pairs are reported as the replacement character (U+FFFD)
   if(wchar >= L'\xD800' && wchar <= L'\xDBFF' || wchar >= L'\xDC00' && wchar <= L'\xDFFF')
      return 3;

   if(wchar <= L'\xFFFF')
      return 3;
      
   return 4;
}

///
/// @brief  Returns `true` if the argument is  within the range defined by
///         the template arguments and `false` otherwise.
///
/// This funciton used to have `unsigned char` template and function parameters,
/// but GCC v9.2 converts `char` template arguments to a sign-extended `int`
/// and only then checks against `unsigned char` template parameter type, which
/// causes value narrowing errors. Use `char` types for parameters and convert
/// to `unsigned char` when comparing instead.
///
template <char lo, char hi>
inline bool in_range(char ch) 
{
   return static_cast<unsigned char>(ch) >= static_cast<unsigned char>(lo) &&
            static_cast<unsigned char>(ch) <= static_cast<unsigned char>(hi);
}

///
/// @brief  Returns the number of bytes in a UTF-8 character or a zero if the sequence 
///         has any bytes outside of the ranges defined for UTF-8 characters. 
///
/// If `maxcnt` is specified, at most `maxcnt` bytes will be evaluated and a zero will 
/// be returned if the sequence of `maxcnt` bytes does not represent a complete UTF-8 
/// character.
///
/// IMPORTANT: this function does not check whether the UTF-8 character is valid and
/// will just count bytes based on the ranges described below. For example, the 0x01
/// is not a valid character.
///
/// [Unicode v5 ch.3 p.104]
/// ```
/// Code Points          First Byte  Second Byte Third Byte  Fourth Byte
/// U+0000   .. U+007F   00..7F
/// U+0080   .. U+07FF   C2..DF      80..BF
/// U+0800   .. U+0FFF   E0          A0..BF      80..BF
/// U+1000   .. U+CFFF   E1..EC      80..BF      80..BF
/// U+D000   .. U+D7FF   ED          80..9F      80..BF
/// U+E000   .. U+FFFF   EE..EF      80..BF      80..BF
/// U+10000  .. U+3FFFF  F0          90..BF      80..BF      80..BF
/// U+40000  .. U+FFFFF  F1..F3      80..BF      80..BF      80..BF
/// U+100000 .. U+10FFFF F4          80..8F      80..BF      80..BF
/// ```
inline size_t utf8size(const char *cp, size_t maxcnt = 4)
{
   if(!cp || !maxcnt || maxcnt > 4)
      return 0;

   // 1-byte character (including zero terminator)
   if(in_range<'\x00', '\x7F'>(*cp))
      return 1;
   
   if(maxcnt < 2)
      return 0;

   // 2-byte sequences
   if(in_range<'\xC2', '\xDF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) ? 2 : 0;
   
   if(maxcnt < 3)
      return 0;

   // 3-byte sequences
   if(*cp == '\xE0')
      return in_range<'\xA0', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xE1', '\xEC'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(*cp == '\xED')
      return in_range<'\x80', '\x9F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;
      
   if(in_range<'\xEE', '\xEF'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) ? 3 : 0;

   if(maxcnt < 4)
      return 0;

   // 4-byte sequences
   if(*cp == '\xF0')
      return in_range<'\x90', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;

   if(in_range<'\xF1', '\xF3'>(*cp))
      return in_range<'\x80', '\xBF'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
      
   if(*cp == '\xF4')
      return in_range<'\x80', '\x8F'>(*(cp+1)) && in_range<'\x80', '\xBF'>(*(cp+2)) &&  in_range<'\x80', '\xBF'>(*(cp+3)) ? 4 : 0;
   
   return 0;
}

///
/// @brief  Converts `slen` characters of a wide character string to a UTF-8 string.
///
size_t ucs2utf8(const wchar_t *str, size_t slen, char *out, size_t bsize);

#endif // UNICODE_H
