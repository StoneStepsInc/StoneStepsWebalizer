/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.h
*/
#ifndef ENCODER_H
#define ENCODER_H

#include "unicode.h"

#include <stddef.h>

//
// encode_string uses encode_char_t to encode a single vaid UTF-8 character.
// This function should not evaluate any bytes beyond the size of the character.
//
// If NULL is passed in, the function should return the length of the longest
// enchoded character sequence. In this case the input character size and the
// output character pointer are not evaluated and may be zero and NULL.
//
// The size of the output buffer pointed to by op must be large enough to
// accommodate the largest encoded sequence.
//
// The caller must never assume that the returned sequence is null-terminated 
// and should always rely on the returned size to determine the length of the
// encoded sequence.
//
typedef char *(*encode_char_t)(const char *cp, size_t cbc, char *op, size_t& obc);

//
// Character encoding functions for encode_string
//
char *encode_char_html(const char *cp, size_t cbc, char *op, size_t& obc);
char *encode_char_xml(const char *cp, size_t cbc, char *op, size_t& obc);
char *encode_char_js(const char *cp, size_t cbc, char *op, size_t& obc);

template <encode_char_t encode_char> size_t encode_string(string_t::char_buffer_t& buffer, const char *str);

#endif // ENCODER_H
