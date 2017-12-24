/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.h
*/
#ifndef ENCODER_H
#define ENCODER_H

#include "unicode.h"

#include <cstddef>

///
/// @typedef   encode_char_t
///
/// @brief  A function used as a character encoder implementation by `encode_string`.
///
/// `encode_string` uses `encode_char_t` to encode a single vaid UTF-8 character.
/// This function should not evaluate any bytes beyond the size of the character.
///
/// If `NULL` is passed in, the function should return the length of the longest
/// enchoded character sequence. In this case the input character size and the
/// output character pointer are not evaluated and may be zero and `NULL`.
///
/// The size of the output buffer pointed to by op must be large enough to
/// accommodate the largest encoded sequence.
///
/// The caller must never assume that the returned sequence is null-terminated 
/// and should always rely on the returned size to determine the length of the
/// encoded sequence.
///
typedef char *(*encode_char_t)(const char *cp, size_t cbc, char *op, size_t& obc);

///
/// @brief  HTML character encoding function for `encode_string`
///
char *encode_char_html(const char *cp, size_t cbc, char *op, size_t& obc);

///
/// @brief  XML character encoding function for `encode_string`
///
char *encode_char_xml(const char *cp, size_t cbc, char *op, size_t& obc);

///
/// @brief  JavaScript character encoding function for `encode_string`
///
char *encode_char_js(const char *cp, size_t cbc, char *op, size_t& obc);

///
/// @brief  Encodes each character in the specified string to be safe if used 
///         within some formatted text, such as HTML or JavaScript.
///
template <encode_char_t encode_char> size_t encode_string(string_t::char_buffer_t& buffer, const char *str);

#endif // ENCODER_H
