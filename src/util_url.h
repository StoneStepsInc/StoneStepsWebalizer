/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_url.h
*/

#ifndef UTIL_URL_H
#define UTIL_URL_H

#include "tstring.h"
#include "char_buffer.h"

/* Request types (bit values) */
#define URL_TYPE_UNKNOWN      0x00
#define URL_TYPE_HTTP         0x01
#define URL_TYPE_HTTPS        0x02
#define URL_TYPE_MIXED        0x03        /* HTTP and HTTPS (nothing else) */
#define URL_TYPE_OTHER        0x04

const char *from_hex(const char *cp1, char *cp2);

void norm_url_str(string_t& str, string_t::char_buffer_t& strbuf);

string_t& url_encode(const string_t& str, string_t& out);

string_t& get_url_host(const char *url, string_t& domain);
string_t::const_char_buffer_t get_url_host(const char *url, size_t slen);

const char *get_domain(const char *, size_t);       /* return domain name  */

#endif // UTIL_URL_H
