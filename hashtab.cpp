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

u_long hash_bin(u_long hashval, const u_char *buf, u_int blen)
{
   for(; blen; buf++, blen--)
      hashval = hash_byte(hashval, *buf);

   return hashval;
}

u_long hash_str(u_long hashval, const char *str, u_int slen)
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
u_long hash_num(u_long hashval, type_t num)
{
   int index;

   for(index = 0; index < sizeof(num); index++)
      hashval = hash_byte(hashval, (u_char) (((u_char*) &num)[index]));

   return hashval;
}

//
// Instantiate numeric hash functions
//
template u_long hash_num<u_short>(u_long hashval, u_short num);
template u_long hash_num<u_long>(u_long hashval, u_long num);
