/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_ipaddr.cpp
*/
#include "pch.h"

#include "util_ipaddr.h"

bool is_ipv4_address(const char *cp)
{
   size_t dcnt, gcnt;    // digit and group counts

   if(!cp)
      return false;

   for(gcnt = 0, dcnt = 0; *cp; cp++) {
      if(string_t::isdigit(*cp)) {
         // if it's the first digit in the group, increment group count
         if(!dcnt) gcnt++;
         
         // can't be more than three digits
         if(++dcnt > 3) 
            return false;
      }
      else if(*cp == '.') {
         // can't start with a dot or have more than four groups
         if(dcnt == 0 || gcnt == 4) 
            return false;
         
         // reset digit count   
         dcnt = 0;
      }
      else
         return false;
   }

   return gcnt == 4 ? true : false;
}

bool is_ipv6_address(const char *cp)
{
   unsigned int dcnt = 0, xcnt = 0;       // decimal and hex digit counts
   unsigned int cgcnt = 0, dgcnt = 0;     // colon and dot group counts
   bool dblcol = false;                   // seen a double colon?

   if(!cp)
      return false;

   //
   // Walk the string to see if we have a valid IPv6 address. The loop also evaluates 
   // the null character at the end of the string, so we can check if the last group 
   // is valid. However, the total number of groups and other IP address constraints
   // should be validated after the loop.
   //
   do {
      // check for decimal digits first, so hex check only counts A-F
      if(*cp && string_t::isdigit(*cp)) {
         dcnt++;
      }
      else if(*cp && string_t::isxdigit(*cp)) {
         xcnt++;
      }
      // check if it's a colon or the end of an all-colon IPv6 address
      else if(*cp == ':' || (!*cp && !dgcnt)) {
         // cannot have a colon after an IPv4 dotted group was seen
         if(dgcnt)
            return false;

         // cannot have more than four hex digits in a group
         if(xcnt + dcnt > 4)
            return false;

         // cannot have more than seven 16-bit groups with compression or more than eight without
         if(!dblcol && cgcnt == 8 || dblcol && cgcnt == 7)
            return false;

         // check if it's a compressed zeroes group
         if(*cp && *(cp+1) == ':') {
            // can only occur once
            if(dblcol)
               return false;

            // set the double-colon flag and skip the second colon
            dblcol = true;
            cp++;
         }
         else {
            // a single colon must have at least one digit in front
            if(!dblcol && !dcnt && !xcnt)
               return false;
         }

         // count the preceding group of digits
         cgcnt++;

         // start new digit counts
         dcnt = xcnt = 0;
      }
      // check if it's a dot in a combined IPv6/IPv4 address or the end of such address
      else if(*cp == '.' || (!*cp && dgcnt)) {
         // a combined IPv4/IPv6 address must start with an IPv6 group, but can't have more than six
         if(!cgcnt || cgcnt > 6)
            return false;

         // if we saw any hex or more than three decimal digits in the last group, it's a bad IPv4 address
         if(xcnt || dcnt > 3)
            return false;

         // bump up the dot group count
         dgcnt++;

         // recet the decimal digit count for the next group
         dcnt = 0;
      }
      else
         return false;

   } while(*cp++);

   // if there's an IPv4 address, all four groups must be present
   if(dgcnt && dgcnt != 4)
      return false;

   // check if an address without compressed zeroes has all groups present
   if(!dblcol) { 
      if(!dgcnt && cgcnt != 8 || dgcnt == 4 && cgcnt != 6)
         return false;
   }

   return true;
}

bool is_ip_address(const char *cp)
{
   return is_ipv4_address(cp) || is_ipv6_address(cp);
}

