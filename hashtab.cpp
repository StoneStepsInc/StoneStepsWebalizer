/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   hashtab.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util.h"
#include "hashtab.h"

/*********************************************/
/* HASH - return hash value for string       */
/*********************************************/

uint64_t hash_bin(uint64_t hashval, const u_char *buf, u_int blen)
{
   for(; blen; buf++, blen--)
      hashval = hash_byte(hashval, *buf);

   return hashval;
}

uint64_t hash_str(uint64_t hashval, const char *str, u_int slen)
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
template uint64_t hash_num<uint64_t>(uint64_t hashval, uint64_t num);
