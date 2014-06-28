/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "tstring.h"

#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

//
// A set of overloaded functions, so we can use the same name for equivalent 
// narrow-character and wide-character functions.
//
inline static uint_t strlen_(const char *str) {return (uint_t) strlen(str);}
inline static uint_t strlen_(const wchar_t *str) {return (uint_t) wcslen(str);}

inline static int strcmp_(const char *s1, const char *s2) {return strcmp(s1, s2);}
inline static int strcmp_(const wchar_t *s1, const wchar_t *s2) {return wcscmp(s1, s2);}

inline static int stricmp_(const char *s1, const char *s2) {return strcasecmp(s1, s2);}
inline static int stricmp_(const wchar_t *s1, const wchar_t *s2) {return wcscasecmp(s1, s2);}

inline static int strncmp_(const char *s1, const char *s2, uint_t count) {return strncmp(s1, s2, (size_t) count);}
inline static int strncmp_(const wchar_t *s1, const wchar_t *s2, uint_t count) {return wcsncmp(s1, s2, (size_t) count);}

inline static int strnicmp_(const char *s1, const char *s2, uint_t count) {return strncasecmp(s1, s2, (size_t) count);}
inline static int strnicmp_(const wchar_t *s1, const wchar_t *s2, uint_t count) {return wcsncasecmp(s1, s2, (size_t) count);}

inline static int vsnprintf_(char *buffer, uint_t count, const char *fmt, va_list valist) {return vsnprintf(buffer, (size_t) count, fmt, valist);}
inline static int vsnprintf_(wchar_t *buffer, uint_t count, const wchar_t *fmt, va_list valist) {return vsnwprintf(buffer, (size_t) count, fmt, valist);}

inline static const char *strchr_(const char *str, char chr) {return strchr(str, chr);}
inline static const wchar_t *strchr_(const wchar_t *str, wchar_t chr) {return wcschr(str, chr);}

//
// Static data members
//
template<typename char_t> char_t string_base<char_t>::empty_string[] = {0};
template<typename char_t> const uint_t string_base<char_t>::npos = (uint_t) -1;

//
//
//
template <typename char_t>
string_base<char_t>::~string_base(void) 
{
   if(string && !holder && string != empty_string)
      free(string);
}

template <typename char_t>
void string_base<char_t>::make_bad_string(void)
{
   if(string != empty_string && bufsize > 1) {
      slen = bufsize - 1;
      memset(string, '#', c2b(slen));
      string[slen] = 0;
   }
}

//
// This method is called from a constuructor and must not evaluate data 
// members in any way (i.e. nothing is initilized at this point).
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
   if(string) {
      if(!holder && string != empty_string)
         free(string);
      string = empty_string;
      bufsize = slen = 0;
   }
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::reset(void)
{
   if(string != empty_string) 
      *string = 0;
   slen = 0;
   return *this;
}

template <typename char_t>
void string_base<char_t>::reserve(u_int len)
{
   if(holder)
      clear();
   realloc_buffer(len);
}

template <typename char_t>
bool string_base<char_t>::realloc_buffer(uint_t len)
{
   // if we have enough storage, we are done
   if(bufsize > len)
      return true;

   // fail if we don't own the storage
   if(holder)
      return false;

   // allocate storage in multiples of four, plus one for the zero terminator
   bufsize = (((len >> 2) + 1) << 2) + 1;

   if(string != empty_string)
      string = (char_t*) realloc(string, c2b(bufsize));
   else {
      string = (char_t*) malloc(c2b(bufsize));
      *string = 0;
   }

   return true;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::append(const char_t *str, uint_t len, bool cstr)
{
   if(str == NULL || *str == 0)
      return *this;

   if(len == 0) {
      if(!cstr)
         return *this;
      len = strlen_(str);
   }

   if(bufsize - slen <= len) {
      if(!realloc_buffer(slen+len)) {
         make_bad_string();
         return *this;
      }
   }
   
   memcpy(&string[slen], str, c2b(len));
   slen += len;
   string[slen] = 0;

   return *this;
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str, uint_t count) const
{
   return strncmp_(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare(const char_t *str) const
{
   return strcmp_(string, str ? str : empty_string);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str, uint_t count) const
{
   return strnicmp_(string, str ? str : empty_string, count);
}

template <typename char_t>
int string_base<char_t>::compare_ci(const char_t *str) const
{
   return stricmp_(string, str ? str : empty_string);
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::tolower(uint_t start, uint_t end)
{
   char_t *cp1, *cp2;

   if(end == 0 || end >= slen)
      end = slen-1;

   if(start >= slen || start > end)
      return *this;

   for(cp1 = &string[start], cp2 = &string[end]; cp1 <= cp2; cp1++) 
      *cp1 = ::tolower(*cp1);

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::toupper(uint_t start, uint_t end)
{
   char_t *cp1, *cp2;

   if(end == 0 || end >= slen)
      end = slen-1;

   if(start >= slen || start > end)
      return *this;

   for(cp1 = &string[start], cp2 = &string[end]; cp1 <= cp2; cp1++) 
      *cp1 = ::toupper(*cp1);

   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::replace(char_t from, char_t to)
{
   char_t *cptr = string;
   while(*cptr) {
      if(*cptr == from)
         *cptr = to;
      cptr++;
   }
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::truncate(uint_t at)
{
   if(at < slen) {
      slen = at;
      string[slen] = 0;
   }
   return *this;
}

template <typename char_t>
uint_t string_base<char_t>::find(char_t chr, uint_t start)
{
   const char_t *cptr;

   return (start < slen && (cptr = strchr_(&string[start], chr)) != NULL) ? cptr-string : npos;
}

template <typename char_t>
uint_t string_base<char_t>::r_find(char_t chr) const
{
   const char_t *cp1 = string + slen;

   while(cp1 != string && *cp1 != chr) cp1--;

   return cp1 == string ? *cp1 == chr ? 0 : npos : cp1 - string;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::format(const char_t *fmt, ...)
{
   va_list valist;
   va_start(valist, fmt);
   format_va(fmt, valist);
   va_end(valist);
   return *this;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::format_va(const char_t *fmt, va_list valist)
{
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
char_t *string_base<char_t>::detach(u_int *bsize)
{
   char_t *str = NULL;

   if(string != empty_string) {
      str = string;
      if(bsize)
         *bsize = bufsize;
      init();
   }
   return str;
}

template <typename char_t>
string_base<char_t>& string_base<char_t>::use_string(char_t *str, uint_t len, bool cstr, uint_t bsize, bool hold)
{
   clear();

   if(!str)
      return *this;

   // if len is zero and str is a zero-terminated string, get the length
   if(!len && cstr) {
      if(*str)
         len = strlen_(str);
   }

   // make sure the buffer is large enough to hold the string
   if(!bsize)
      bsize = len+1;
   else if(bsize <= len)
      return *this;

   string = str;
   slen = len;
   bufsize = bsize;
   string[slen] = 0;
   holder = hold;

   return *this;
}

//
// Instantiate the string template (see comments at the end of hashtab.cpp)
//
template class string_base<char>;

