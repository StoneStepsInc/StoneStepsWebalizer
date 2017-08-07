/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   fmt_impl.h
*/

#ifndef UTIL_FMT_H
#define UTIL_FMT_H

#include "tstring.h"

#include <cinttypes>

size_t fmt_vprintf(string_t::char_buffer_t& buffer, const char *fmt, va_list args);
size_t fmt_hr_num(string_t::char_buffer_t& buffer, uint64_t num, const char *sep, const char * const msg_unit_pfx[], const char *unit, bool decimal);

#endif // UTIL_FMT_H


