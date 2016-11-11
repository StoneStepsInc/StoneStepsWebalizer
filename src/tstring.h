/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   tstring.h
*/

#ifndef __TSTRING_H
#define __TSTRING_H

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <memory>

#include "char_buffer.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

//
// string_base
//
// 1. string_base may be used to manage three kinds of strings:
//
//    * dynamically allocated modifiable strings (holder == false)
//    * modifiable strings within a fixed-size buffer (holder == true && bufsize > 0)
//    * read-only strings (holder == true && bufsize == 0)
//
// 2. A read-only string cannot be modified in any way and cannot be repurposed as a
// modifiable string. Use string_t::hold to wrap string literals in a string_t class:
//
//    const string_t& str = string_t::hold("ABC", 3);
//
// Note that if a non-const instance of string_t is constructed instead in the example
// above, calling non-const methods of this instance will throw an exception.
//
// Read-only strings may be moved between string_t instances via the move constructor 
// or the move assignment operator.
//
// 3. Constructors that take just a reference to char_buffer_t initialize a zero-length
// string with the specified buffer. If these constructors are not explicitly declared, 
// this code will construct a string with random characters:
//
//    char_buffer_t char_buffer(100);
//    string_t str(char_buffer);
//
// , because it will be interpreted as:
//
//    char_buffer_t char_buffer(100);
//    string_t str(char_buffer.operator char*());
//
// 4. Unlike with const_char_buffer_t, which can be used independently from char_buffer_t,
// having a distinct string_base class defined with a const character type would make it 
// imposible to pass such string instances into functions that take string_t arguments. 
// Read-only string_t instances are used instead to wrap string literals or other data that 
// cannot be modified.
// 
template <typename char_t>
class string_base {
   public:
      typedef char_t char_type;

      // char_buffer_base with a matching character type
      typedef char_buffer_base<char_t> char_buffer_t;
      typedef char_buffer_base<const char_t> const_char_buffer_t;

      // fixed_char_buffer_t with a matching character type
      template <size_t BUFSIZE> using fixed_char_buffer_t = ::fixed_char_buffer_t<char_t, BUFSIZE>;

   private:
      //
      // Both character pointers in the union have the same size and alignment requirements 
      // and start at the same address, which allows us to assign a const character pointer, 
      // such as a string literal, without a const cast. The string instance in this case 
      // is set up as a read-only string, which guarantees that a non-const string pointer 
      // is only used in contexts where it's interpreted as a const string (e.g. returning
      // a string pointer from a const member function).
      //
      union {
         char_t         *string;    // modifiable string
         const char_t   *c_string;  // const string
      };

      size_t   slen     : 31;       // length
      size_t            :  1;
      size_t   bufsize  : 31;       // buffer size, in characters, including the null character
      bool     holder   :  1;       // if true, does not own string memory

      static char_t empty_string[];

      static const char_t ex_readonly_string[];
      static const char_t ex_bad_char_buffer[];
      static const char_t ex_bad_hold_string[];

   private:
      void init(void);

      void make_bad_string(void);

      bool realloc_buffer(size_t len);

   public:
      static const size_t npos;

   public:
      string_base(void) {init();}

      string_base(const string_base& str) {init(); assign(str);}
      string_base(string_base&& str);

      explicit string_base(const char_t *str);
      string_base(const char_t *str, size_t len) {init(); assign(str, len);}

      // see #3 above
      explicit string_base(char_buffer_t&& char_buffer) {init(); attach(std::move(char_buffer), 0);}

      string_base(char_buffer_t&& char_buffer, size_t len) {init(); attach(std::move(char_buffer), len);}

      string_base(const_char_buffer_t&& char_buffer, size_t len);

      ~string_base(void);

      size_t length(void) const {return slen;}

      size_t capacity(void) const {return (bufsize) ? bufsize - 1 : 0;}
      
      const char_t *c_str(void) const {return string;}

      operator const char_t*(void) const {return string;}

      string_base& clear(void);
      string_base& reset(void);
      
      void reserve(size_t len);

      bool isempty(void) const {return slen == 0;}

      string_base& operator = (const string_base& str) {return assign(str.string, str.slen);}
      string_base& operator = (string_base&& str);
      string_base& operator = (const char_t *str) {return assign(str);}
      string_base& operator = (char_t chr) {return assign(&chr, 1);}

      string_base& assign(const string_base& str) {return assign(str.string, str.slen);}
      string_base& assign(const char_t *str);
      string_base& assign(const char_t *str, size_t len) {reset(); return append(str, len);}

      string_base& append(const string_base& str) {return append(str.string, str.slen);}
      string_base& append(const char_t *str);
      string_base& append(char_t chr) {return append(&chr, 1);}
      string_base& append(const char_t *str, size_t len);

      int compare(const char_t *str, size_t count) const;
      int compare(const char_t *str) const;

      int compare_ci(const char_t *str, size_t count) const;
      int compare_ci(const char_t *str) const;

      string_base& operator += (const string_base& str) {return append(str);}
      string_base& operator += (const char_t *str) {return append(str);}
      string_base& operator += (char_t chr) {return append(&chr, 1);}

      string_base operator + (const string_base& str) const {return string_base(*this).append(str);}
      string_base operator + (const char_t *str) const {return string_base(*this).append(str);}
      string_base operator + (char_t chr) const {return string_base(*this).append(&chr, 1);}

      bool operator == (const string_base& str) const {return (slen != str.slen) ? false : compare(str) == 0 ? true : false;}
      bool operator != (const string_base& str) const {return (slen != str.slen) ? true : compare(str) != 0 ? true : false;}

      bool operator == (const char_t *str) const {return compare(str) == 0 ? true : false;}
      bool operator != (const char_t *str) const {return compare(str) != 0 ? true : false;}
      bool operator >  (const char_t *str) const {return compare(str) >  0 ? true : false;}
      bool operator <  (const char_t *str) const {return compare(str) <  0 ? true : false;}
      bool operator >= (const char_t *str) const {return compare(str) <= 0 ? true : false;}
      bool operator <= (const char_t *str) const {return compare(str) <= 0 ? true : false;}

      string_base& tolower(size_t start = 0, size_t end = 0);
      string_base& toupper(size_t start = 0, size_t end = 0);

      string_base& replace(char_t from, char_t to);

      string_base& truncate(size_t at);

      size_t find(char_t chr, size_t start = 0) const;
      size_t r_find(char_t chr) const;

      string_base& format(const char_t *fmt, ...);
      string_base& format_va(const char_t *fmt, va_list valist);

      static string_base _format(const char_t *fmt, ...);
      static string_base _format_va(const char_t *fmt, va_list valist);

      char_buffer_t detach(void);

      string_base& attach(char_buffer_t&& str, size_t len);

      static string_base<char_t> hold(const char_t *str);
      static string_base<char_t> hold(const char_t *str, size_t len);
};

//
//
//
typedef string_base<char> string_t;

#endif // __TSTRING_H
