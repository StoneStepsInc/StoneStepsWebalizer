/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_string.cpp
*/
#include "pch.h"

#include "util_string.h"

#include <algorithm>
//
// Derive table size from the size of the character. Note that when 
// accessing the array, char values have to be cast to u_char to make
// sure that the sign bit isn't extended. That is, if the character
// with the code \xE2 is converted to an integer without a cast to 
// an unsigned char, the resulting index will be -30:
//
//  const char *str = "\xE2";        // 0xE2 (226)
//  int n = deltas[str[0]];          // index = -30 ((int) 0xE2)
//  int m = deltas[(u_char)str[0]];  // index = 226 ((int)(u_char) 0xE2)
//
const size_t bmh_delta_table::table_size = 1 << sizeof(u_char) * 8;

void bmh_delta_table::reset(const string_t& str)
{
   size_t index, slen;

   if(deltas)
      delete [] deltas;
   deltas = nullptr;

   if(str.isempty())
      return;

   slen = str.length();

   deltas = new size_t[table_size];

   // use the string length to fill the array
   for(index = 0; index < table_size; index++)
      deltas[index] = slen;

   //
   // Store the offset from the end of the string for each string  
   // character, except the last one. For example, the string 
   // "abcab" would produce this delta array:
   //
   //    deltas['a'] = 1;
   //    deltas['b'] = 3;
   //    deltas['c'] = 2;
   //    deltas[any other character] = 5;
   //
   for(index = 0; index < slen-1; index++)
      deltas[(u_char)str[index]] = slen - index - 1;
}

//
//   strncpy_ex
//
//   Copies at most destsize-1 source bytes to dest and terminates the 
// resulting string with a zero. If srclen is zero, copies the source
// character-by-character.
//
//   Unlike strncpy, this function doesn't pad the destination buffer 
// with zeros.
//
size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen)
{
   size_t destlen, maxlen = destsize - 1;
   const char *cp1;
   char *cp2;

   if(dest == nullptr || src == nullptr || destsize == 0)
      return 0;

   if(srclen) {
      destlen = std::min(srclen, maxlen);
      memcpy(dest, src, destlen);
   }
   else {
      cp1 = src; cp2 = dest;
      for(destlen = 0; *cp1 && destlen < maxlen; destlen++)
         *cp2++ = *cp1++;
   }

   dest[destlen] = 0;

   return destlen;
}


//
// NIGEL HORSPOOL, Practical Fast Searching in Strings, p505
//
//   delta12[*] <- patlen; { initialize whole array }
//   for j <- 1 to patlen - 1 do
//      delta12[pat[j]]. <- patlen - j;
//
//   lastch <- pat[patlen];
//   i <- patlen;
//   while i <= stringlen do
//      begin
//         ch <- string[i];
//         if ch = lastch then
//            if string[i - patlen + 1 ... j] = pat then
//               return i - patlen + 1 ;
//         i <- i + delta12[ch];
//      end;
//    return 0;
//
// For example, the pattern "abcab" will be matched as follows:
//
//    aabcaabcab
//    abcab         move by delta['a'] = 1 
//     abcab        move by delta['a'] = 1 
//      abcab       move by delta['b'] = 3
//         abcab    match
//
const char *strstr_ex(const char *str1, const char *str2, size_t l1, size_t l2, const bmh_delta_table *delta, bool nocase)
{
   size_t i1;
   const char *cp1, *cp2, *cptr;
   char lastch;

   // empty and nullptr strings never match
   if(!str1 || !*str1 || !str2 || !*str2 || !l1 || !l2)
      return nullptr;

   // return nullptr if the sub-string is longer then the string
   if(l2 > l1) 
      return nullptr;

   // if a delta table is available, use it
   if(delta && delta->isvalid()) {
      lastch = str2[l2-1];
      i1 = l2 - 1;

      while(i1 < l1) {
         //
         // For case-insensitive searches we need to check upper and lower character case 
         // slots in the case-sensitive delta table. Check character case of the selected
         // string character and pick the case conversion function that changes its case
         // to the opposite.
         //
         char (*flipcase)(char) = nocase ? string_t::islower(str1[i1]) ? (char (*)(char)) string_t::toupper : (char (*)(char)) string_t::tolower : nullptr;

         // check if the last character of the pattern matches the string character
         if(str1[i1] == lastch || nocase && flipcase(str1[i1]) == lastch) {
            // if it does, compare all other characters
            if(nocase) {
               if(!string_t::compare_ci(&str1[i1-l2+1], str2, l2-1))
                  return &str1[i1-l2+1];
            } else {
               if(!memcmp(&str1[i1-l2+1], str2, l2-1))
                  return &str1[i1-l2+1];
            }
         }

         //
         // Use the offset from the delta table to move the pattern along the string, so the 
         // string character we just compared matches the rightmost same pattern character. 
         // For case-insensitive searches pick the smallest distance from the delta table to 
         // move the pattern.
         //
         if(nocase)
            i1 += std::min((*delta)[(u_char)str1[i1]], (*delta)[(u_char)flipcase(str1[i1])]);
         else
            i1 += (*delta)[(u_char)str1[i1]];
      }
   }
   else {
      cptr = str1;

      while(*cptr && l1 >= l2) {
         // try to match the pattern left-to-right
         cp1 = cptr; cp2 = str2;
         while(*cp1 && *cp2 && cp1 != &cptr[l1] && cp2 != &str2[l2]) {
            if(nocase) {
               if(string_t::tolower(*cp1) != string_t::tolower(*cp2))
                  break;
            } else {
               if(*cp1 != *cp2)
                  break;
            }
            cp1++; cp2++;
         }

         // if at the end of the pattern, return
         if(*cp2 == 0 || cp2 == &str2[l2])
            return cptr;

         // move the string pointer and reduce the remaining length
         cptr++, l1--;
      }
   }

   return nullptr;
}

const char *strstr_ex(const char *str1, const char *str2, size_t l1, const bmh_delta_table *delta, bool nocase)
{
   return strstr_ex(str1, str2, l1, str2 ? strlen(str2) : 0, delta, nocase);
}

const char *strstr_ex(const char *str1, const char *str2, const bmh_delta_table *delta, bool nocase)
{
   return strstr_ex(str1, str2, str1 ? strlen(str1) : 0, str2 ? strlen(str2) : 0, delta, nocase);
}

int strncmp_ex(const char *str1, size_t slen1, const char *str2, size_t slen2)
{
   int rc;

   if(slen1 == 0 && slen2 == 0)
      return 0;

   if(slen1 == 0)
      return -*str2;

   if(slen2 == 0)
      return *str1;

   // compare the minimum of the two lengths characters
   if((rc = strncmp(str1, str2, std::min(slen1, slen2))) != 0)
      return rc;

   // return the first character following the base of the longer string
   if(slen1 < slen2)
      return -str2[slen1];

   if(slen1 > slen2)
      return str1[slen2];

   return 0;
}

//
// [length]string
//
const char *cstr2str(const char *cp, string_t& str)
{
   char *ecp = nullptr;
   size_t slen;

   str.reset();

   if(cp == nullptr || *cp != '[')
      return nullptr;

   slen = strtol(++cp, &ecp, 10);

   if(*ecp != ']')
      return nullptr;

   cp = ++ecp;

   if(slen)
      str.assign(cp, slen); 

   return cp + slen;
}

//
// ul2str
//
// Converts unsigned long value to a decimal string. Returns the
// number of characters in the resulting string
//
size_t ul2str(uint64_t value, char *str)
{
   char *cp1, *cp2, *cp3, tmp;

   if(str == nullptr)
      return 0;

   cp1 = cp2 = str;

   do {
      *cp2++ = '0' + (u_char) (value % 10);
      value /= 10;
   } while(value);

   *(cp3 = cp2) = 0;

   while(cp1 < --cp2) {
      tmp = *cp1;
      *cp1++ = *cp2;
      *cp2 = tmp;
   }

   return cp3-str;
}

uint64_t str2ul(const char *str, const char **eptr, size_t len)
{
   const char *cp = str;
   uint64_t value;

   if(eptr)
      *eptr = nullptr;

   if(str == nullptr || *str == 0 || len == 0 || !string_t::isdigit(*cp))
      return 0;

   value = *cp++ - '0';

   while(*cp && (size_t) (cp - str) < len && string_t::isdigit(*cp)) {
      value *= 10;
      value += *cp++ - '0';
   }

   if(eptr)
      *eptr = cp;

   return value;
}

int64_t str2l(const char *str, const char **eptr, size_t len)
{
   const char *cp = str;
   uint64_t value;
   bool neg = false;

   if(str == nullptr)
      return 0;

   // check for +/-
   if(*cp == '-') {
      cp++; 
      neg = true;
   }
   else if(*cp == '+') 
      cp++;

   value = str2ul(cp, eptr, len);

   return neg ? -((int64_t)value) : value;
}

//
// isinstrex checks if the string matches the pattern
//
//    str  - the string to match
//
//    cp   - the pattern; may take four forms:
//
//       pattern*  - match the beginning of the string
//       *pattern  - match the end of the string
//       pattern   - match the sub-string (substr == true)
//       pattern   - match the word (substr == false)
//
//    slen  - length of the string; if non-zero, then the string will be 
//            scanned only up to slen characters
// 
//    cplen - length of the pattern; if non-zero, it *must* indicate the 
//            exact length of the pattern
//
bool isinstrex(const char *str, const char *cp, size_t slen, size_t cplen, bool substr, const bmh_delta_table *deltas, bool nocase)
{
   const char *cp1,*cp2,*eos;

   if(!str || !*str || !cp || !*cp || !slen || !cplen)
      return false;

   eos = &str[slen];

   cp1 = &cp[cplen-1];

   if (*cp=='*') {
      if(slen < cplen-1)
         return false;

      /* if leading wildcard, start from end */
      cp2 = &str[slen-1];
      while (cp1 != cp && cp2 != str) {
         if (*cp1 == '*') return true;
         if(nocase) {
            if(string_t::tolower(*cp1--) != string_t::tolower(*cp2--)) 
               return false;
         } else {
            if(*cp1-- != *cp2--) 
               return false;
         }
      }

      // return true if we reached '*' or ran out of string right before '*'
      return cp1 == cp || cp1 == cp+1 && cp2 == str;
   }

   if(substr && *cp1 != '*') {
      /* if no leading/trailing wildcard, just strstr */
      return (strstr_ex(str, cp, slen, cplen, deltas, nocase) != nullptr);
   }

   if(*cp1 == '*') {
      if(slen < cplen-1) 
         return false;
   }
   else if(slen < cplen) 
      return false;

   /* otherwise do normal forward scan */
   cp1 = cp; cp2 = str;
   while (cp2 != eos) {
      if (*cp1=='*') return true;
      if(nocase) {
         if(string_t::tolower(*cp1++) != string_t::tolower(*cp2++)) 
            return false;
      } else {
         if (*cp1++!=*cp2++) 
            return false;
      }
   }

   return (*cp1 == '*' || (*cp1 == 0 && cp2 == eos)) ? true : false;
}
