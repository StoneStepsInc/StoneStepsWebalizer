/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "tstring.h"
#include "unicode.h"

#include <memory.h>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cwchar>
#include <cctype>
#include <stdexcept>

//
// Define the same macro as was used in tstring.h, so we don't make this rather
// short macro name linger throughout the project and possibly cause any conflicts.
// See the same macro in tstring.h for more details.
//
#define CHLT(c) select_char_literal<char_t>(c, L ## c)

//
// A set of overloaded functions, so we can use the same name for equivalent 
// narrow-character and wide-character functions.
//
inline static size_t strlen_(const char *str) {return (size_t) strlen(str);}
inline static size_t strlen_(const wchar_t *str) {return (size_t) wcslen(str);}

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
template<typename char_t> char_t string_base<char_t>::empty_string[] = {0};
template<typename char_t> const size_t string_base<char_t>::npos = (size_t) -1;

template<typename char_t> const char string_base<char_t>::ex_readonly_string[] = "Cannot change a read-only string";
template<typename char_t> const char string_base<char_t>::ex_bad_char_buffer[] = "Bad character buffer";
template<typename char_t> const char string_base<char_t>::ex_bad_hold_string[] = "Bad string to hold";
template<typename char_t> const char string_base<char_t>::ex_holder_resize[] = "Cannot resize a string holder";
template<typename char_t> const char string_base<char_t>::ex_not_implemented[] = "Not implemented";
template<typename char_t> const char string_base<char_t>::ex_fmt_error[] = "Cannot format a string";
template<typename char_t> const char string_base<char_t>::ex_bad_utf8_char[] = "A bad UTF-8 character is encountered";
template<typename char_t> const char string_base<char_t>::ex_bad_self_assign[] = "A string should not be copied or moved into itself";


//
//
//

template <typename char_t>
string_base<char_t>::string_base(void) :
   string(empty_string),
   bufsize(0),
   slen(0),
   holder(false)
{
}

template <typename char_t>
string_base<char_t>::string_base(const string_base& str) :
   string_base(str.c_str(), str.length())
{
}


template <typename char_t>
string_base<char_t>::string_base(const char_t *str) :
   string_base(str, str && *str ? strlen_(str) : 0)
{
}

template <typename char_t>
string_base<char_t>::string_base(const char_t *str, size_t len)
{
   if(!str || !*str || !len) {
      string = empty_string;
      bufsize = slen = 0;
      holder = false;
   }
   else {
      bufsize = bufsize_for_length(len);
      string = char_buffer_t::alloc(bufsize);

      // copy the source and null-terminate the string
      memcpy(string, str, len);
      string[len] = '\x0';

      slen = len;
      holder = false;
   }
}

template <typename char_t>
string_base<char_t>::string_base(string_base&& other) noexcept :
   string(other.string),
   slen(other.slen),
   bufsize(other.bufsize),
   holder(other.holder)
{
   other.make_empty();
}

template <typename char_t>
string_base<char_t>::string_base(const_char_buffer_t&& char_buffer, size_t len) : bufsize(0), holder(true)
{
   // cannot hold a nullptr pointer
   if(!char_buffer)
      throw std::runtime_error(ex_bad_char_buffer);

   // check if the buffer is large enough to hold len plus the null character
   if(len >= char_buffer.capacity())
      throw std::runtime_error(ex_bad_char_buffer);

   // read-only strings must be null-terminated
   if(char_buffer[len])
      throw std::runtime_error(ex_bad_char_buffer);

   // move the string out of the buffer
   c_string = char_buffer.detach(nullptr, nullptr);

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
string_base<char_t>& string_base<char_t>::operator = (const string_base& other) noexcept(false)
{
   // report bad same-instance copy
   if(&other == this)
      throw std::logic_error(ex_bad_self_assign);

   return assign(other.string, other.slen);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::operator = (string_base&& other) noexcept(false)
{
   // report bad same-instance move
   if(&other == this)
      throw std::logic_error(ex_bad_self_assign);

   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   // free the existing string if it was allocated
   if(!holder && string != empty_string)
      char_buffer_t::free(string);

   // and take over the other string
   string = other.string;
   slen = other.slen;
   bufsize = other.bufsize;
   holder = other.holder;

   other.make_empty();

   return *this;
}

template <typename char_t>
void string_base<char_t>::make_empty(void)
{
   string = empty_string;
   bufsize = slen = 0;
   holder = false;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::clear(void)
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

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
      throw std::runtime_error(ex_readonly_string);

   if(string != empty_string) 
      *string = 0;
   slen = 0;
   return *this;
}

template <typename char_t>
void string_base<char_t>::reserve(size_t len)
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(holder)
      clear();
   realloc_buffer(len);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::assign(const char_t *str) 
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   reset();

   if(str && *str)
      return append(str, strlen_(str));

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::append(const char_t *str) 
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(str && *str)
      return append(str, strlen_(str));

   return *this;
}

template <typename char_t>
void string_base<char_t>::realloc_buffer(size_t len)
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   // fail if we don't own the storage
   if(holder)
      throw std::runtime_error(ex_holder_resize);

   // bufsize includes the null terminator and len does not
   if(len >= bufsize) {
      // allocate storage in multiples of four, plus one for the null terminator
      bufsize = bufsize_for_length(len);

      if(string != empty_string)
         string = char_buffer_t::alloc(string, bufsize, slen + 1);
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
      throw std::runtime_error(ex_readonly_string);

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
   // if either of the strings is nullptr, consider it an empty string
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
      // one of the characters may be a null character if one of the strings is shorter
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
   size_t chsz1, chsz2;
   const char_t *cp1, *cp2;

   // if either of the strings is nullptr, consider it an empty string
   cp1 = str1 ? str1 : empty_string;
   cp2 = str2 ? str2 : empty_string;

   while((*cp1 || *cp2) && count) {
      if(tolower(*cp1) != tolower(*cp2))
         return char_diff(tolower(*cp1), tolower(*cp2));
      cp1++, cp2++;
      count--;
   }

   return 0;
}

template <>
int string_base<char>::compare_ci(const char *str1, const char *str2, size_t count)
{
   size_t chsz1, chsz2;
   const char *cp1, *cp2;

   // if either of the strings is nullptr, consider it an empty string
   cp1 = str1 ? str1 : empty_string;
   cp2 = str2 ? str2 : empty_string;

   while((*cp1 || *cp2) && count) {
      // make sure both characters are valid UTF-8 characters
      if((chsz1 = utf8size(cp1)) == 0)
         throw std::runtime_error(ex_bad_utf8_char);

      if((chsz2 = utf8size(cp2)) == 0)
         throw std::runtime_error(ex_bad_utf8_char);

      if(chsz1 == 1 && chsz2 == 1) {
         // compare ASCII characters case-insensitively
         if(tolower(*cp1) != tolower(*cp2))
            return char_diff(tolower(*cp1), tolower(*cp2));
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

template <typename char_t>
string_base<char_t>& string_base<char_t>::transform(char_t (*convchar)(char_t), size_t start, size_t length)
{
   char_t *cp1, *cp2;

   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(start >= slen)
      return *this;

   // cannot check for (start + length > slen) because length is defaulted to SIZE_MAX
   if(length - start > slen - start)
      length = slen - start;

   // walk through characters from start to end
   for(cp1 = &string[start], cp2 = &string[start+length]; cp1 < cp2; cp1++)
      *cp1 = convchar(*cp1);

   return *this;
}

template <>
string_base<char>& string_base<char>::transform(char (*convchar)(char), size_t start, size_t length)
{
   char *cp1, *cp2;
   size_t chsz;

   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(start >= slen)
      return *this;

   // cannot check for (start + length > slen) because length is defaulted to SIZE_MAX
   if(length - start > slen - start)
      length = slen - start;

   // walk through UTF-8 characters from start to end
   for(cp1 = &string[start], cp2 = &string[start+length]; cp1 < cp2; cp1 += chsz) { 
      if((chsz = utf8size(cp1)) == 0)
         throw std::runtime_error(ex_bad_utf8_char);

      // only convert one-byte (ASCII) characters
      if(chsz == 1)
         *cp1 = convchar(*cp1);
   }

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::tolower(size_t start, size_t length)
{
   return transform(tolower, start, length);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::toupper(size_t start, size_t length)
{
   return transform(toupper, start, length);
}

//
// The char specialization of tolower can only handle UTF-8 characters in the ASCII
// range, which according to UnicodeData.txt from unicode.org is contiguous for upper
// and lower case characters, so we can just use the difference between the character
// 'A' of the appropriate case.
//
template <>
char string_base<char>::tolower(char chr)
{
   // convert only ASCII characters
   return (utf8size(&chr, 1) == 1 && isupper(chr)) ? 'a' + (chr - 'A') : chr;
}

template <>
char string_base<char>::toupper(char chr)
{
   // convert only ASCII characters
   return (utf8size(&chr, 1) == 1 && islower(chr)) ? 'A' + (chr - 'a') : chr;
}

//
// See #5 above class definition
//
template <typename char_t>
char_t string_base<char_t>::tolower(char_t chr)
{
   // convert only ASCII characters
   return isupper(chr) ? CHLT('a') + (chr - CHLT('A')) : chr;
}

template <typename char_t>
char_t string_base<char_t>::toupper(char_t chr)
{
   // convert only ASCII characters
   return islower(chr) ? CHLT('A') + (chr - CHLT('a')) : chr;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::replace(char_t from, char_t to)
{
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

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
      throw std::runtime_error(ex_readonly_string);

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

   return (start < slen && (cptr = strchr_(&string[start], chr)) != nullptr) ? cptr-string : npos;
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
      throw std::runtime_error(ex_readonly_string);

   va_list valist;
   va_start(valist, fmt);
   format_va(fmt, valist);
   va_end(valist);
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::format_va(const char_t *fmt, va_list valist)
{
   va_list ap;
   int plen;

   // can't format a read-only string
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(!fmt) {
      reset();
      return *this;
   }

   // copy valist in case if we need to reallocate memory and use it again
   va_copy(ap, valist);

   // if the string is empty, allocate enough storage to hold the format
   if(string == empty_string)
      realloc_buffer(strlen_(fmt));

   //
   // Unlike vsnprintf, vswprintf does not return the number of characters that would 
   // be printed if the buffer was large enough and instead returns a negative value
   // if the formatted string doesn't fit in the buffer. Double the size of the buffer 
   // in each iteration until the string fits or until we reached the amount of memory 
   // we can afford.
   //
   while((plen = vswprintf(string, bufsize, fmt, ap)) >= (int) bufsize || plen < 0) {
      va_end(ap);
      if(bufsize > 500 * 1024) {
         reset();
         throw std::runtime_error(ex_fmt_error);
      }
      realloc_buffer(bufsize << 1);
      va_copy(ap, valist);
   }
   
   va_end(ap);

   slen = (size_t) plen;

   return *this;
}

template <>
string_base<char>& string_base<char>::format_va(const char *fmt, va_list valist)
{
   va_list ap;
   int plen;

   // can't format a read-only string
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   if(!fmt) {
      reset();
      return *this;
   }

   // copy valist in case if we need to reallocate memory and use it again
   va_copy(ap, valist);

   // if the string is empty, allocate enough storage to hold the format
   if(string == empty_string)
      realloc_buffer(strlen(fmt));

   //
   // vsnprintf returns the number of bytes in the buffer if it was large enough 
   // or the number of bytes that would be in the buffer if it had enough room or 
   // -1 in case of an error. Reallocate the buffer to fit the formatted string in 
   // the second case and skip the loop in the latter. Note that we need to cast 
   // bufsize to int to make sure the negative number (-1) isn't converted to an 
   // unsigned value on platforms where size_t is the same size as int.
   // 
   while((plen = vsnprintf(string, bufsize, fmt, ap)) >= (int) bufsize) {
      va_end(ap);
      realloc_buffer(plen);
      va_copy(ap, valist);
   }
   
   va_end(ap);

   // check if there was a formatting error
   if(plen < 0) {
      reset();
      throw std::runtime_error(ex_fmt_error);
   }

   slen = (size_t) plen;

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
      throw std::runtime_error(ex_readonly_string);

   if(string != empty_string)
      string_buffer.attach(string, bufsize, holder);

   make_empty();

   return string_buffer;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::attach(char_buffer_t&& char_buffer, size_t len)
{
   // cannot attach to a read-only string
   if(holder && !bufsize)
      throw std::runtime_error(ex_readonly_string);

   // cannot attach a nullptr pointer
   if(!char_buffer)
      throw std::runtime_error(ex_bad_char_buffer);

   // check if the buffer is large enough to hold len plus the null character
   if(len >= char_buffer.capacity())
      throw std::runtime_error(ex_bad_char_buffer);

   // release current memory block
   if(!holder && string != empty_string)
      char_buffer_t::free(string);

   // set up string storage (can't pass bit fields into char_buffer_t::detach)
   bufsize = char_buffer.capacity();
   holder = char_buffer.isholder();

   // and attach the buffer supplied by the caller
   string = char_buffer.detach(nullptr, nullptr);

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
   // cannot hold a nullptr pointer
   if(!str)
      throw std::runtime_error(ex_bad_hold_string);

   // str may be in read-only memory, make sure it's terminated
   if(str[len])
      throw std::runtime_error(ex_bad_hold_string);

   // create a temporary holder buffer and initialize a read-only string.
   const_char_buffer_t char_buffer(str, len+1, true);

   // create and return a read-only string (see note #2 in the class definition)
   return string_base<char_t>(std::move(char_buffer), len);
}

//
// Instantiate the string template (see comments at the end of hashtab.cpp)
//
template class string_base<char>;

