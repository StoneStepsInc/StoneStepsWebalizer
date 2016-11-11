/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "tstring.h"
#include "exception.h"

#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define vsnwprintf _vsnwprintf
#define wcsncasecmp _wcsnicmp
#define wcscasecmp _wcsicmp
#else
#define vsnwprintf vswprintf
#endif

//
// A set of overloaded functions, so we can use the same name for equivalent 
// narrow-character and wide-character functions.
//
inline static size_t strlen_(const char *str) {return (size_t) strlen(str);}
inline static size_t strlen_(const wchar_t *str) {return (size_t) wcslen(str);}

inline static int strcmp_(const char *s1, const char *s2) {return strcmp(s1, s2);}
inline static int strcmp_(const wchar_t *s1, const wchar_t *s2) {return wcscmp(s1, s2);}

inline static int stricmp_(const char *s1, const char *s2) {return strcasecmp(s1, s2);}
inline static int stricmp_(const wchar_t *s1, const wchar_t *s2) {return wcscasecmp(s1, s2);}

inline static int strncmp_(const char *s1, const char *s2, size_t count) {return strncmp(s1, s2, (size_t) count);}
inline static int strncmp_(const wchar_t *s1, const wchar_t *s2, size_t count) {return wcsncmp(s1, s2, (size_t) count);}

inline static int strnicmp_(const char *s1, const char *s2, size_t count) {return strncasecmp(s1, s2, (size_t) count);}
inline static int strnicmp_(const wchar_t *s1, const wchar_t *s2, size_t count) {return wcsncasecmp(s1, s2, (size_t) count);}

inline static int vsnprintf_(char *buffer, size_t count, const char *fmt, va_list valist) {return vsnprintf(buffer, (size_t) count, fmt, valist);}
inline static int vsnprintf_(wchar_t *buffer, size_t count, const wchar_t *fmt, va_list valist) {return vsnwprintf(buffer, (size_t) count, fmt, valist);}

inline static const char *strchr_(const char *str, char chr) {return strchr(str, chr);}
inline static const wchar_t *strchr_(const wchar_t *str, wchar_t chr) {return wcschr(str, chr);}

//
// Static data members
//
template<typename char_t> char_t string_base<char_t>::empty_string[] = {0};
template<typename char_t> const size_t string_base<char_t>::npos = (size_t) -1;

template<typename char_t> const char_t string_base<char_t>::ex_readonly_string[] = "Cannot change a read-only string";
template<typename char_t> const char_t string_base<char_t>::ex_bad_char_buffer[] = "Bad character buffer";
template<typename char_t> const char_t string_base<char_t>::ex_bad_hold_string[] = "Bad string to hold";

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

template <typename char_t>
void string_base<char_t>::make_bad_string(void)
{
   if(string != empty_string && bufsize > 1) {
      slen = bufsize - 1;
      memset(string, '#', char_buffer_t::memsize(slen));
      string[slen] = 0;
   }
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
bool string_base<char_t>::realloc_buffer(size_t len)
{
   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   // fail if we don't own the storage
   if(holder)
      return false;

   // if we have enough storage, we are done
   if(bufsize > len)
      return true;

   // allocate storage in multiples of four, plus one for the zero terminator
   bufsize = (((len >> 2) + 1) << 2) + 1;

   if(string != empty_string)
      string = char_buffer_t::alloc(string, bufsize);
   else {
      string = char_buffer_t::alloc(bufsize);
      *string = 0;
   }

   return true;
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
   if(bufsize - slen <= len) {
      if(!realloc_buffer(slen+len)) {
         make_bad_string();
         return *this;
      }
   }
   
   memcpy(&string[slen], str, char_buffer_t::memsize(len));
   slen += len;
   string[slen] = 0;

   return *this;
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str, size_t count) const
{
   return strncmp_(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str) const
{
   return strcmp_(string, str ? str : empty_string);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str, size_t count) const
{
   return strnicmp_(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str) const
{
   return stricmp_(string, str ? str : empty_string);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::tolower(size_t start, size_t end)
{
   char_t *cp1, *cp2;

   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(end == 0 || end >= slen)
      end = slen-1;

   if(start >= slen || start > end)
      return *this;

   for(cp1 = &string[start], cp2 = &string[end]; cp1 <= cp2; cp1++) 
      *cp1 = (char) ::tolower(*cp1);

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::toupper(size_t start, size_t end)
{
   char_t *cp1, *cp2;

   if(holder && !bufsize)
      throw exception_t(0, ex_readonly_string);

   if(end == 0 || end >= slen)
      end = slen-1;

   if(start >= slen || start > end)
      return *this;

   for(cp1 = &string[start], cp2 = &string[end]; cp1 <= cp2; cp1++) 
      *cp1 = (char) ::toupper(*cp1);

   return *this;
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
   if(string == empty_string) {
      if(!realloc_buffer(strlen_(fmt))) {
         make_bad_string();
         return *this;
      }
   }

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
      
      if(!realloc_buffer(bufsize << 1)) {
         make_bad_string();
         return *this;
      }
      
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

