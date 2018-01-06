/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   tstring.h
*/

#ifndef TSTRING_H
#define TSTRING_H

#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <memory>

#include "char_buffer.h"

//
// This macro forms a member tempate function call with all supported character types,
// so the right one for the current string template character type can be returned by
// the character literal selection function. This macro is undefined at the end of this
// file because it can only be used within the string_base class definition.
//
#define CHLT(c) select_char_literal<char_t>(c, L ## c)

///
/// @class  string_base
///
/// @brief  A genric character string object
///
/// @tparam chart_t  Character type (char or wchar_t)
///
/// 1. string_base may be used to manage three kinds of strings:
///
///    * dynamically allocated modifiable strings (holder == false)
///    * modifiable strings within a fixed-size buffer (holder == true && bufsize > 0)
///    * read-only strings (holder == true && bufsize == 0)
///
/// 2. A read-only string cannot be modified in any way and cannot be repurposed as a
/// modifiable string. Use string_t::hold to wrap string literals in a string_t class:
/// ```
///    const string_t& str = string_t::hold("ABC", 3);
/// ```
/// Note that if a non-const instance of string_t is constructed instead in the example
/// above, calling non-const methods of this instance will throw an exception.
///
/// Read-only strings may be moved between `string_t` instances via the move constructor 
/// or the move assignment operator.
///
/// 3. [removed]
///
/// 4. Unlike with `const_char_buffer_t`, which can be used independently from `char_buffer_t`,
/// having a distinct `string_base` class defined with a `const` character type would make it 
/// imposible to pass such string instances into functions that take `string_t` arguments. 
/// Read-only string_t instances are used instead to wrap string literals or other data that 
/// cannot be modified.
///
/// 5. When `string_base` is instantiated for `char`, the intended character set is UTF-8 and
/// any other character encoding, such as various Windows code pages, will not work. The 
/// side effect of having a single char value passed into member functions such as `tolower`
/// is that they can only operate on ASCII characters. Implementing support for arbitrary 
/// UTF-8 characters would require a character abstraction that could take the form of a 
/// char sequence (UTF-8) or a single `wchar_t` character or a sequence of `char16_t` or a single 
/// `char32_t` character. Given how this class is used within this project, where case-dependent 
/// comparisons and conversions are done only against ASCII characters, only ASCII characters
/// are recognized in functions that deal with character case.
///
template <typename char_t>
class string_base {
   public:
      typedef char_t char_type;

      // char_buffer_base with a matching character type
      typedef char_buffer_base<char_t> char_buffer_t;
      typedef char_buffer_base<const char_t> const_char_buffer_t;

      // fixed_char_buffer_t with a matching character type
      template <size_t BUFSIZE> using fixed_char_buffer_t = ::fixed_char_buffer_t<char_t, BUFSIZE>;

      // character buffer holder type with arbitrary allocation parameters
      template <typename ... alloc_params_t> using char_buffer_holder_t = char_buffer_holder_tmpl<char_t, alloc_params_t ...>;

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
         char_t         *string;    ///< modifiable string
         const char_t   *c_string;  ///< const string
      };

      size_t   slen     : 31;       ///< string length, in characters
      size_t            :  1;
      size_t   bufsize  : 31;       ///< buffer size, in characters, including the null character
      bool     holder   :  1;       ///< if true, does not own string memory

      static char_t empty_string[];

      static const char ex_readonly_string[];
      static const char ex_bad_char_buffer[];
      static const char ex_bad_hold_string[];
      static const char ex_holder_resize[];
      static const char ex_not_implemented[];
      static const char ex_fmt_error[];
      static const char ex_bad_utf8_char[];

   private:
      void init(void);

      void realloc_buffer(size_t len);

      //
      // VC++2015 has a bug that triggers a compiler error if this member function is declared 
      // as a template taking the character conversion function as a template parameter and then 
      // the primary template definition and the explicit specialization for char are defined
      // outside of the class definition. Use a function pointer instead to work around this bug. 
      //
      string_base& transform(char_t (*convchar)(char_t), size_t start, size_t length);

      //
      // This member function is intended to be used with CHLT macro that generates a call 
      // of this function with all types of character literals supported by string_base, so 
      // each function specialization can select the matching parameter type and return its 
      // value. This, in turn, allows us to use character literals that are compatible with
      // the string character type without casting.
      //
      template <typename slct_char_t>
      static slct_char_t select_char_literal(char ch, wchar_t wch) = delete;

   public:
      static const size_t npos;

   public:
      string_base(void) {init();}

      string_base(const string_base& str) {init(); assign(str);}
      string_base(string_base&& str);

      explicit string_base(const char_t *str);
      string_base(const char_t *str, size_t len) {init(); assign(str, len);}

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

      static int compare(const char_t *str1, const char_t *str2);
      static int compare(const char_t *str1, const char_t *str2, size_t count);

      static int compare_ci(const char_t *str1, const char_t *str2);
      static int compare_ci(const char_t *str1, const char_t *str2, size_t count);

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

      string_base& tolower(size_t start = 0, size_t length = SIZE_MAX);
      string_base& toupper(size_t start = 0, size_t length = SIZE_MAX);

      static char_t tolower(char_t chr);
      static char_t toupper(char_t chr);

      //
      // These functions avoid any locale-specific checks and conversions that may be
      // done by the equivalent standard library functions and instead evaluate only 
      // ASCII characters, which is sufficient for the purposes of this project.
      //
      static inline bool islower(char_t ch) {return ch >= CHLT('a') && ch <= CHLT('z');}
      static inline bool isupper(char_t ch) {return ch >= CHLT('A') && ch <= CHLT('Z');}
      static inline bool isalpha(char_t ch) {return ch >= CHLT('A') && ch <= CHLT('Z') || ch >= CHLT('a') && ch <= CHLT('z');}
      static inline bool isdigit(char_t ch) {return ch >= CHLT('0') && ch <= '9';}
      static inline bool isalnum(char_t ch) {return isalpha(ch) || isdigit(ch);}
      static inline bool isxdigit(char_t ch) {return ch >= CHLT('0') && ch <= CHLT('9') || ch >= CHLT('A') && ch <= CHLT('F') || ch >= CHLT('a') && ch <= CHLT('f');}
      static inline bool isspace(char_t ch) {return ch == CHLT(' ') || ch == CHLT('\t');}
      static inline bool iswspace(char_t ch) {return isspace(ch) || ch == CHLT('\r') || ch == CHLT('\n');}

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
// Declare explicit specializations for methods that require special handling of
// UTF-8 characters
//
template <> 
template <>
inline char string_base<char>::select_char_literal(char ch, wchar_t wch) {return ch;}

template <> 
template <>
inline wchar_t string_base<wchar_t>::select_char_literal(char ch, wchar_t wch) {return wch;}

template <> 
char string_base<char>::tolower(char chr);

template <> 
char string_base<char>::toupper(char chr);

template <>
string_base<char>& string_base<char>::format_va(const char *fmt, va_list valist);

template <> 
string_base<char>& string_base<char>::transform(char (*convchar)(char), size_t start, size_t length);

//
// Undefine this macro because it can only be used within string_base
//
#undef CHLT

//
//
//
typedef string_base<char> string_t;

#endif // TSTRING_H
