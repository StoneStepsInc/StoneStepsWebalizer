/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   hashtab.cpp
*/
#include "pch.h"

#include "hashtab.h"

#include <cstdlib>
#include <cstring>
#include <climits>

/*********************************************/
/* HASH - return hash value for string       */
/*********************************************/

uint64_t hash_bin(uint64_t hashval, const u_char *buf, size_t blen)
{
   for(; blen; buf++, blen--)
      hashval = hash_byte(hashval, *buf);

   return hashval;
}

uint64_t hash_str(uint64_t hashval, const char *str, size_t slen)
{
   if(str == NULL)
      return hashval;

   if(slen) {
      for(; slen && *str != 0; str++, slen--)
         hashval = hash_byte(hashval, (u_char) *str);
   }
   else {
      for(; *str != 0; str++)
         hashval = hash_byte(hashval, (u_char) *str);
   }

   return hashval;
}

template <typename type_t>
uint64_t hash_num(uint64_t hashval, type_t num)
{
   int index;

   for(index = 0; index < sizeof(num); index++)
      hashval = hash_byte(hashval, (u_char) (((u_char*) &num)[index]));

   return hashval;
}

//
// Instantiate numeric hash functions
//
template uint64_t hash_num<u_short>(uint64_t hashval, u_short num);
template uint64_t hash_num<uint32_t>(uint64_t hashval, uint32_t num);
template uint64_t hash_num<uint64_t>(uint64_t hashval, uint64_t num);
