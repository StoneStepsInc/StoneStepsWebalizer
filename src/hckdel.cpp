/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   hckdel.cpp
*/
#include "pch.h"

#include "hckdel.h"

//
// Rounds a 32-bit integer to the nearest power of two
//
// http://www.hackersdelight.org/hdcodetxt/clp2.c.txt
// http://www.hackersdelight.org/permissions.htm
//
uint32_t ceilp2(uint32_t x) 
{
   x--;
   x |= x >> 1;
   x |= x >> 2;
   x |= x >> 4;
   x |= x >> 8;
   x |= x >>16;
   return ++x;
}

//
// Reiser's algorithm computing the number of trailing zero bits in a 32-bit number
//
// http://www.hackersdelight.org/hdcodetxt/ntz.c.txt
// http://www.hackersdelight.org/permissions.htm
//
uint32_t tzbits(uint32_t x) 
{
   // elements with an uppercase U suffix are unused
   static const uint32_t tzb[37] = {
      32u,  0u,  1u, 26u,  2u, 23u, 27u, 0U,  3u, 16u, 
      24u, 30u, 28u, 11u,  0U, 13u,  4u, 7u, 17u,  0U, 
      25u, 22u, 31u, 15u, 29u, 10u, 12u, 6u,  0U, 21u, 
      14u,  9u,  5u, 20u,  8u, 19u, 18u
   };

   // negative of an unsigned 32-bit value x is (2^32 - x) (5.3.1 Unary Operators)
   return tzb[(x & -x) % 37u];
}
