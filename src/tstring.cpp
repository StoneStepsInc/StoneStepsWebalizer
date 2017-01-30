/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "tstring.h"
#include "exception.h"
#include "unicode.h"

#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

#ifdef _WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define vsnwprintf _vsnwprintf
#else
#define vsnwprintf vswprintf
#endif

//
// A set of overloaded functions, so we can use the same name for equivalent 
// narrow-character and wide-character functions.
//
inline static size_t strlen_(const char *str) {return (size_t) strlen(str);}
inline static size_t strlen_(const wchar_t *str) {return (size_t) wcslen(str);}

inline static int vsnprintf_(char *buffer, size_t count, const char *fmt, va_list valist) {return vsnprintf(buffer, (size_t) count, fmt, valist);}
inline static int vsnprintf_(wchar_t *buffer, size_t count, const wchar_t *fmt, va_list valist) {return vsnwprintf(buffer, (size_t) count, fmt, valist);}

inline static const char *strchr_(const char *str, char chr) {return strchr(str, chr);}
inline static const wchar_t *strchr_(const wchar_t *str, wchar_t chr) {return wcschr(str, chr);}

//
// It is unspecified whether char and wchar_t are signed or unsigned, which may lead
// to an incorrect character value subtraction result if the high-order bit of one of
// the characters is set. The template specializations below cast each character type
// and size to the size-matching unsigned type to avoid sign extension, which would 
// lead to incorrect subtraction results.
//
template <typename char_t, size_t char_size = sizeof(char_t)>
inline int char_diff(char_t ch1, char_t ch2) = delete;

template <>
inline int char_diff<char, sizeof(char)>(char ch1, char ch2)
{
   return (unsigned char) ch1 - (unsigned char) ch2;
}

template <>
inline int char_diff<wchar_t, sizeof(unsigned short)>(wchar_t ch1, wchar_t ch2)
{
   return (unsigned short) ch1 - (unsigned short) ch2;
}

template <>
inline int char_diff<wchar_t, sizeof(unsigned int)>(wchar_t ch1, wchar_t ch2)
{
   return (unsigned int) ch1 - (unsigned int) ch2;
}

//
// Static data members
//
template <typename char_t> const std::locale& string_base<char_t>::locale = std::locale::classic();

template<typename char_t> char_t string_base<char_t>::empty_string[] = {0};
template<typename char_t> const size_t string_base<char_t>::npos = (size_t) -1;

template<typename char_t> const char string_base<char_t>::ex_readonly_string[] = "Cannot change a read-only string";
template<typename char_t> const char string_base<char_t>::ex_bad_char_buffer[] = "Bad character buffer";
template<typename char_t> const char string_base<char_t>::ex_bad_hold_string[] = "Bad string to hold";
template<typename char_t> const char string_base<char_t>::ex_holder_resize[] = "Cannot resize a string holder";
template<typename char_t> const char string_base<char_t>::ex_not_implemented[] = "Not implemented";

//
//
//
template <typename char_t>
string_base<char_t>::string_base(const char_t *str) 
{
   init();
   
   if(str && *str) 
      assign(str, strlen_(str));
}

template <typename char_t>
string_base<char_t>::string_base(string_base&& other) : string(other.string), slen(other.slen), bufsize(other.bufsize), holder(other.holder)
{
   other.init();
}

template <typename char_t>
string_base<char_t>::string_base(const_char_buffer_t&& char_buffer, size_t len) : bufsize(0), holder(true)
{
   // cannot hold a NULL pointer
   if(!char_buffer)
      throw exception_t(0, ex_bad_char_buffer);

   // check if the buffer is large enough to hold len plus the null character
   if(len >= char_buffer.capacity())
      throw exception_t(0, ex_bad_char_buffer);

   // read-only strings must be null-terminated
   if(char_buffer[len])
      throw exception_t(0, ex_bad_char_buffer);

   // move the string out of the buffer
   c_string = char_buffer.detach(NULL, NULL);

   // set the string length
   slen = len;
}

template <typename char_t>
string_base<char_t>::~string_base(void) 
{
   if(string && !holder && string != empty_string)
      char_buffer_t::free(string);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::operator = (string_base&& other)
{
   string = other.string;
   slen = other.slen;
   bufsize = other.bufsize;
   holder = other.holder;

   other.init();

   return *this;
}

//
// This method must not evaluate data members in any way because data may be not 
// initialized if it's called from a constructor or we may need to reset original 
// data members when moving objects in the move constructor or move assignment 
// operator.
//
template <typename char_t>
void string_base<char_t>::init(void)
{
   string = empty_string;
   bufsize = slen = 0;
   holder = false;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::clear(void)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(string) {
      if(!holder && string != empty_string)
         char_buffer_t::free(string);
      string = empty_string;
      bufsize = slen = 0;
   }
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::reset(void)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(string != empty_string) 
      *string = 0;
   slen = 0;
   return *this;
}

template <typename char_t>
void string_base<char_t>::reserve(size_t len)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(holder)
      clear();
   realloc_buffer(len);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::assign(const char_t *str) 
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   reset();

   if(str && *str)
      return append(str, strlen_(str));

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::append(const char_t *str) 
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(str && *str)
      return append(str, strlen_(str));

   return *this;
}

template <typename char_t>
void string_base<char_t>::realloc_buffer(size_t len)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   // fail if we don't own the storage
   if(holder)
      throw exception_t(0, ex_holder_resize);

   // bufsize includes the null terminator and len does not
   if(len >= bufsize) {
      // allocate storage in multiples of four, plus one for the null terminator
      bufsize = (((len >> 2) + 1) << 2) + 1;

      if(string != empty_string)
         string = char_buffer_t::alloc(string, bufsize);
      else {
         string = char_buffer_t::alloc(bufsize);
         *string = 0;
      }
   }
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::append(const char_t *str, size_t len)
{
   // can't append to a read-only string 
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   // check if there's anything to append
   if(!str || !*str || !len)
      return *this;

   // bufsize must be greater than slen at this point
   if(bufsize - slen <= len)
      realloc_buffer(slen+len);
   
   memcpy(&string[slen], str, char_buffer_t::memsize(len));
   slen += len;
   string[slen] = 0;

   return *this;
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str, size_t count) const
{
   return compare(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str) const
{
   return compare(string, str ? str : empty_string);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str, size_t count) const
{
   return compare_ci(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str) const
{
   return compare_ci(string, str ? str : empty_string);
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str1, const char_t *str2)
{
   return compare(str1, str2, SIZE_MAX);
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str1, const char_t *str2, size_t count)
{
   // if either of the strings is NULL, consider it an empty string
   if(!str1)
      str1 = empty_string;

   if(!str2)
      str2 = empty_string;

   //
   // For case-sensitive simple comparisons (i.e. no normaization, etc) individual code 
   // units can be compared. Note that 'count' contains the number of code units and not 
   // characters.
   //
   for(const char_t *cp1 = str1, *cp2 = str2; (*cp1 || *cp2) && count; cp1++, cp2++, count--) {
      // one of the characters may be a null character if on of the strings is shorter
      if(*cp1 != *cp2)
         return char_diff(*cp1, *cp2);
   }

   return 0;
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str1, const char_t *str2)
{
   return compare_ci(str1, str2, SIZE_MAX);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str1, const char_t *str2, size_t count)
{
   throw exception_t(0, ex_not_implemented);
}

template <>
int string_base<char>::compare_ci(const char *str1, const char *str2, size_t count)
{
   size_t chsz1, chsz2;
   const char *cp1 = str1, *cp2 = str2;

   // if either of the strings is NULL, consider it an empty string
   if(!str1)
      str1 = empty_string;

   if(!str2)
      str2 = empty_string;

   while((*cp1 || *cp2) && count) {
      // TODO: throw instead, once we can guarantee that only UTF-8 strings are in play
      if((chsz1 = utf8size(cp1)) == 0)
         return -1;

      if((chsz2 = utf8size(cp2)) == 0)
         return -1;

      if(chsz1 == 1 && chsz2 == 1) {
         // compare ASCII characters case-insensitively
         if(string_base<char>::tolower(*cp1) != string_base<char>::tolower(*cp2))
            return (unsigned char) *cp1 - (unsigned char) *cp2;
         cp1++, cp2++;
         count--;
      }
      else {
         // the first byte in a valid UTF-8 sequence is different for all character sizes
         if(chsz1 != chsz2)
            return (unsigned char) *cp1 - (unsigned char) *cp2;

         // compare same-size characters byte-by-byte
         for(; chsz1; cp1++, cp2++, chsz1--, count--) {
            if(*cp1 != *cp2)
               return (unsigned char) *cp1 - (unsigned char) *cp2;
         }
      }
   }

   return 0;
}

template <>
template <char convchar(char, const std::locale&)>
string_base<char>& string_base<char>::transform(size_t start, size_t length)
{
   char *cp1, *cp2;
   size_t chsz;

   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(start >= slen)
      return *this;

   // cannot check for (start + length > slen) because length is defaulted to SIZE_MAX
   if(length - start > slen - start)
      length = slen - start;

   // walk through UTF-8 characters from start to end
   for(cp1 = &string[start], cp2 = &string[start+length]; cp1 < cp2; cp1 += chsz) { 
      // TODO: throw instead, once we can guarantee that only UTF-8 strings are in play
      if((chsz = utf8size(cp1)) == 0)
         return (*this);

      // only convert one-byte (ASCII) characters
      if(chsz == 1)
         *cp1 = convchar(*cp1, locale);
   }

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::tolower(size_t start, size_t length)
{
   return transform<std::tolower<char_t>>(start, length);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::toupper(size_t start, size_t length)
{
   return transform<std::toupper<char_t>>(start, length);
}

template <>
char string_base<char>::tolower(char chr)
{
   // convert only ASCII characters
   return (isutf8char(chr)) ? std::tolower(chr, locale) : chr;
}

template <>
char string_base<char>::toupper(char chr)
{
   // convert only ASCII characters
   return (isutf8char(chr)) ? std::toupper(chr, locale) : chr;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::replace(char_t from, char_t to)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   char_t *cptr = string;
   while(*cptr) {
      if(*cptr == from)
         *cptr = to;
      cptr++;
   }
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::truncate(size_t at)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(at < slen) {
      slen = at;
      string[slen] = 0;
   }
   return *this;
}

template <typename char_t>
size_t string_base<char_t>::find(char_t chr, size_t start) const
{
   const char_t *cptr;

   return (start < slen && (cptr = strchr_(&string[start], chr)) != NULL) ? cptr-string : npos;
}

template <typename char_t>
size_t string_base<char_t>::r_find(char_t chr) const
{
   const char_t *cp1 = string + slen;

   while(cp1 != string && *cp1 != chr) cp1--;

   return cp1 == string ? *cp1 == chr ? 0 : npos : cp1 - string;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::format(const char_t *fmt, ...)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   va_list valist;
   va_start(valist, fmt);
   format_va(fmt, valist);
   va_end(valist);
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::format_va(const char_t *fmt, va_list valist)
{
   // can't format a read-only string
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(!fmt) {
      reset();
      return *this;
   }

   // if the string is empty, allocate enough storage to hold the format
   if(string == empty_string)
      realloc_buffer(strlen_(fmt));

   //
   // The code below calls vsnprintf multiple times in order to make sure 
   // sufficient buffer is allocated. When compiled using GCC x64, valist 
   // becomes unusable after the first vsnprintf call, so we have to create 
   // a copy of valist before every vsnprintf call. However, the official 
   // status of the vs_copy macro in the context of C++ is quite vague, as 
   // it is not listed in the standard (14882:2003) and is not implemented 
   // by some compilers (e.g. Visual C++).
   //
#if defined(va_copy) || defined(__va_copy)
   #if defined(va_copy)
      #define __VA_DEFINE__(dst, src) va_list dst; va_copy(dst, src)
      #define __VA_COPY__(dst, src) va_copy(dst, src)
   #elif defined __va_copy
      #define __VA_DEFINE__(dst, src) va_list dst; __va_copy(dst, src)
      #define __VA_COPY__(dst, src) __va_copy(dst, src)
   #endif
   #define __VA_END__(va)  va_end(va)
#else
   //
   // If we don't know how to copy the list, leave it alone. Copying the 
   // list variable by assignment doesn't seem very reliable because the 
   // list may be implemented as a pointer to the stack frame or as some 
   // form of structure holding variable arguments or a pointer to such 
   // structure. Hopefully, the environment that doesn't provide va_copy 
   // will not render the list unusable inside vsnprintf. Visual C++ is 
   // one example of such environment.
   //
   #define __VA_DEFINE__(dst, src) va_list& dst = src
   #define __VA_COPY__(dst, src) 
   #define __VA_END__(va)
#endif

   __VA_DEFINE__(valist_, valist);

   //
   // If slen is equal to the buffer size or if -1 is returned (converted 
   // to unsigned), the buffer is too small. Double the buffer size and 
   // try again.
   //
   while((slen = vsnprintf_(string, bufsize, fmt, valist_)) >= bufsize) {

      __VA_END__(valist_);
      
      realloc_buffer(bufsize << 1);
      
      __VA_COPY__(valist_, valist);
   }
   
   __VA_END__(valist_);

   return *this;
}

template <typename char_t>
string_base<char_t> string_base<char_t>::_format(const char_t *fmt, ...)
{
   string_base str;
   va_list valist;
   va_start(valist, fmt);
   str.format_va(fmt, valist);
   va_end(valist);
   return str;
}

template <typename char_t>
string_base<char_t> string_base<char_t>::_format_va(const char_t *fmt, va_list valist)
{
   return string_base<char_t>().format(fmt, valist);
}

template <typename char_t>
typename string_base<char_t>::char_buffer_t string_base<char_t>::detach(void)
{
   char_buffer_t string_buffer;

   // cannot detach a read-only string
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(string != empty_string)
      string_buffer.attach(string, bufsize, holder);

   init();

   return string_buffer;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::attach(char_buffer_t&& char_buffer, size_t len)
{
   // cannot attach to a read-only string
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   // cannot attach a NULL pointer
   if(!char_buffer)
      throw exception_t(0, ex_bad_char_buffer);

   // check if the buffer is large enough to hold len plus the null character
   if(len >= char_buffer.capacity())
      throw exception_t(0, ex_bad_char_buffer);

   // release current memory block
   if(!holder && string != empty_string)
      char_buffer_t::free(string);

   // set up string storage (can't pass bit fields into char_buffer_t::detach)
   bufsize = char_buffer.capacity();
   holder = char_buffer.isholder();

   // and attach the buffer supplied by the caller
   string = char_buffer.detach(NULL, NULL);

   // set the string length
   slen = len;

   // and make sure the string is null-terminated
   if(string[slen])
      string[slen] = 0;

   return *this;
}

template <typename char_t>
string_base<char_t> string_base<char_t>::hold(const char_t *str) 
{
   return string_t::hold(str, str && *str ? strlen_(str) : 0);
}

template <typename char_t>
string_base<char_t> string_base<char_t>::hold(const char_t *str, size_t len) 
{
   // cannot hold a NULL pointer
   if(!str)
      throw exception_t(0, ex_bad_hold_string);

   // str may be in read-only memory, make sure it's terminated
   if(str[len])
      throw exception_t(0, ex_bad_hold_string);

   // create a temporary holder buffer and initialize a read-only string.
   const_char_buffer_t char_buffer(str, len+1, true);

   // create and return a read-only string (see note #2 in the class definition)
   return string_base<char_t>(char_buffer, len);
}

//
// Instantiate the string template (see comments at the end of hashtab.cpp)
//
template class string_base<char>;

