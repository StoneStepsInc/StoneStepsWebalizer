/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_url.h
*/

#ifndef UTIL_URL_H
#define UTIL_URL_H

#include "tstring.h"
#include "char_buffer.h"

/// HTTP request types based on port values (bit values)
#define URL_TYPE_UNKNOWN      0x00        ///< Port was not set 
#define URL_TYPE_HTTP         0x01        ///< Port matches `HttpPort`
#define URL_TYPE_HTTPS        0x02        ///< Port matches `HttpsPort`
#define URL_TYPE_MIXED        0x03        ///< Both `HttpPort` and `HttpsPort` values have been seen
#define URL_TYPE_OTHER        0x04        ///< Port matches neither `HttpPort` nor `HttpsPort`

///
/// @brief  Converts two text hexadecimal characters in `cp1` to a decoded 
///         character, which is placed into the location pointed to by `cp2`.
///
const char *from_hex(const char *cp1, char *cp2);

///
/// @brief  Normalizes a URL-encoded string and corrects bad character encoding to 
///         minimize aliasing and improve readability. 
///
void norm_url_str(string_t& str, string_t::char_buffer_t& strbuf);

///
/// @brief  URL-encodes multi-byte UTF-8 sequences, space and control characters. 
///         No other character transformations are done. 
///
string_t& url_encode(const string_t& str, string_t& out);

///
/// @brief  Extracts a domain name from the specified URL
///
string_t& get_url_host(const char *url, string_t& domain);


///
/// @brief  Extracts a domain name from the specified URL and returns it in a
///         temporary holder buffer.
///
string_t::const_char_buffer_t get_url_host(const char *url, size_t slen);

///
/// @brief  Returns a pointer to a shorter domain name within a full domain 
///         name, which contains at most `labelcnt` domain labels counted 
///         from the right.
///
const char *get_domain(const char *name, size_t labelcnt);

#endif // UTIL_URL_H
