/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_string.h
*/
#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include "tstring.h"

//
// Boyer-Moore-Horspool delta table (see strstr_ex for details)
//
class bmh_delta_table {
   private:
      size_t                *deltas;

      static const size_t   table_size;

   public:
      bmh_delta_table(void) {deltas = nullptr;}

      bmh_delta_table(const string_t& str) : deltas(nullptr) {reset(str);}

      ~bmh_delta_table(void) {if(deltas) delete [] deltas;}

      void reset(const string_t& str);

      bool isvalid(void) const {return !(deltas == nullptr);}

      size_t operator [] (size_t chr) const {return deltas[chr];}
};

const char *strstr_ex(const char *str1, const char *str2, const bmh_delta_table *delta = nullptr, bool nocase = false);
const char *strstr_ex(const char *str1, const char *str2, size_t l1, const bmh_delta_table *delta = nullptr, bool nocase = false);
const char *strstr_ex(const char *str1, const char *str2, size_t l1, size_t l2, const bmh_delta_table *delta = nullptr, bool nocase = false);

size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen);

int strncmp_ex(const char *str1, size_t slen1, const char *str2, size_t slen2);

const char *cstr2str(const char *cp, string_t& str);

size_t ul2str(uint64_t value, char *str);
uint64_t str2ul(const char *str, const char **eptr = nullptr, size_t len = ~0);
int64_t str2l(const char *str, const char **eptr = nullptr, size_t len = ~0);

bool isinstrex(const char *str, const char *cp, size_t slen, size_t cplen, bool substr, const bmh_delta_table *deltas, bool nocase = false);

#endif // UTIL_STRING_H
