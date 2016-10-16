/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util.cpp
*/
#include "pch.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "util.h"

#include "unicode.h"
#include "tstamp.h"
#include "exception.h"

#include <cstddef>
#include <memory>

static char *url_decode(const char *str, char *out, size_t *slen = NULL);

//
// Derive table size from the size of the character. Note that when 
// accessing the array, char values have to be cast to u_char to make
// sure that the sign bit isn't extended. That is, if the character
// with the code \xE2 is converted to an integer without a cast to 
// an unsigned char, the resulting index will be -30:
//
//  const char *str = "Ô";           // 0xE2 (226)
//  int n = deltas[str[0]];          // index = -30 ((int) 0xE2)
//  int m = deltas[(u_char)str[0]];  // index = 226 ((int)(u_char) 0xE2)
//
const size_t bmh_delta_table::table_size = 1 << sizeof(u_char) * 8;

void bmh_delta_table::reset(const string_t& str)
{
   size_t index, slen;

   if(deltas)
      delete [] deltas;
   deltas = NULL;

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

   if(dest == NULL || src == NULL || destsize == 0)
      return 0;

   if(srclen) {
      destlen = MIN(srclen, maxlen);
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

uint64_t usec2msec(uint64_t usec)
{
   return (usec / 1000) + ((usec % 1000 >= 500) ? 1 : 0);
}

bool is_http_redirect(size_t respcode)
{
   switch (respcode) {
      case RC_MOVEDPERM:         // 301
      case RC_MOVEDTEMP:         // 302
      case RC_SEEOTHER:          // 303
      case RC_MOVEDTEMPORARILY:  // 307
         return true;
   }

   return false;
}

bool is_http_error(size_t respcode)
{
   size_t code = respcode / 100;
   return code == 4 || code == 5;
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
const char *strstr_ex(const char *str1, const char *str2, size_t l1, size_t l2, const bmh_delta_table *delta)
{
   size_t i1;
   const char *cp1, *cp2, *cptr;
   char lastch;

   if(str1 == NULL || *str1 == 0 || str2 == NULL || *str2 == 0)
      return NULL;

   if(l1 == 0)
      l1 = strlen(str1);

   if(l2 == 0)
      l2 = strlen(str2);

   if(l2 > l1) 
      return NULL;

   // if a delta table is available, use it
   if(delta && delta->isvalid()) {
      lastch = str2[l2-1];
      i1 = l2 - 1;

      while(i1 < l1) {
         // check if the last character of the pattern matches the string character
         if(str1[i1] == lastch) {
            // if it does, compare all other characters
            if(!memcmp(&str1[i1-l2+1], str2, l2-1))
               return &str1[i1];
         }
         // move the pattern to match the character we just compared
         i1 += (*delta)[(u_char)str1[i1]];
      }
   }
   else {
      cptr = str1;

      while(*cptr && l1 >= l2) {
         // try to match the pattern left-to-right
         cp1 = cptr; cp2 = str2;
         while(*cp1 && *cp2 && *cp1 == *cp2 && cp1 != &cptr[l1]) cp1++, cp2++;

         // if at the end of the pattern, return
         if(*cp2 == 0)
            return cptr;

         // move the string pointer and reduce the remaining length
         cptr++, l1--;
      }
   }

   return NULL;
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
   if((rc = strncmp(str1, str2, MIN(slen1, slen2))) != 0)
      return rc;

   // return the first character following the base of the longer string
   if(slen1 < slen2)
      return -str2[slen1];

   if(slen1 > slen2)
      return str1[slen2];

   return 0;
}

/*********************************************/
/* UNESCAPE - convert escape seqs to chars   */
/*********************************************/

string_t& url_decode(const string_t& str, string_t& out)
{
   size_t olen;
   string_t::char_buffer_t optr;
   
   out.reset();
   
   if(!str.isempty()) {
      // allocate enough memory
      out.reserve(str.length());
      
      // and detach the block
      optr = out.detach();
      
      // decode the string in place 
      url_decode(str, optr, &olen);
      
      // and attach it back
      out.attach(optr, olen);
   }
   
   return out;
}

char *url_decode(const char *str, char *out, size_t *slen)
{
   const char *cp1 = str;
   char *cp2 = out;

   if (!str || !out) 
      return NULL;                              /* make sure strings valid */

   while (*cp1) {
      if (*cp1=='%')                            /* Found an escape?        */
      {
         cp1++;
         if(isxdigitex(*cp1) && isxdigitex(*(cp1+1)))/* ensure a hex digit      */
            cp1 = from_hex(cp1, cp2++);
         else 
            *cp2++='%';
      }
      else
         *cp2++ = *cp1++;                       /* if not, just continue   */
   }
   *cp2 = *cp1;                                 /* don't forget terminator */
   
   if(slen)
      *slen = cp2-out;
   
   return out;                                  /* return the string       */
}

/*********************************************/
/* FROM_HEX - convert hex char to decimal    */
/*********************************************/

const char *from_hex(const char *cp1, char *cp2)
{
   if(!cp1 || !cp2)
      return cp1;

   //
   // Convert the hex number to an octet, which is supposed to be UTF-8, but may 
   // be in some other encoding. In the latter case there's no way to identify 
   // the encoding, so Latin1 will be assumed by the caller if any of the octets 
   // produced by this function form invalid UTF-8 characters.
   //
   *cp2 = from_hex(*cp1++) << 4;
   *cp2 |= from_hex(*cp1++);

   // change control characters to underscore
   if((unsigned char) *cp2 < '\x20') 
      *cp2 = '_';

   return cp1;
}

char from_hex(char c)
{
   if(c >= '0' && c <= '9')
      return c - '0';

   if(c >= 'A' && c <= 'F')
      return c - 'A' + 10;

   if(c >= 'a' && c <= 'f')
      return c - 'a' + 10;

   return 0;                      /* return 0 if bad...      */
}

//
// [length]string
//
const char *cstr2str(const char *cp, string_t& str)
{
   char *ecp = NULL;
   size_t slen;

   str.reset();

   if(cp == NULL || *cp != '[')
      return NULL;

   slen = strtol(++cp, &ecp, 10);

   if(*ecp != ']')
      return NULL;

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

   if(str == NULL)
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
      *eptr = NULL;

   if(str == NULL || *str == 0 || len == 0 || !isdigitex(*cp))
      return 0;

   value = *cp++ - '0';

   while(*cp && (size_t) (cp - str) < len && isdigitex(*cp)) {
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

   if(str == NULL)
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

/*********************************************/
/* CUR_TIME - return date/time as a string   */
/*********************************************/

string_t cur_time(bool local_time)
{
   size_t slen;
   wchar_t wctstamp[256];
   char    timestamp[256];
   time_t  now;

   /* get system time */
   now = time(NULL);
   
   // format time stamp for the current locale
   if (local_time)
      slen = wcsftime(wctstamp, sizeof(wctstamp)/sizeof(wchar_t), L"%d-%b-%Y %H:%M %Z", localtime(&now));
   else
      slen = wcsftime(wctstamp, sizeof(wctstamp)/sizeof(wchar_t), L"%d-%b-%Y %H:%M UTC", gmtime(&now));
   
   // convert the wide-character string to UTF-8
   slen = ucs2utf8(wctstamp, slen, timestamp, sizeof(timestamp));

   return string_t(timestamp, slen);
}

/*********************************************/
/* CTRY_IDX - create unique # from domain    */
/*********************************************/

uint64_t ctry_idx(const char *str)
{
   uint64_t idx=0;
   const char *cp1=str;

   if(str) {
      if(*str == '*')
         return 0;
      while(*cp1) {
         idx = (idx <<= 5) | (*cp1 - 'a' + 1);
         cp1++;
      }
   }

   return idx;
}

string_t idx_ctry(uint64_t idx)
{
   char ch, buf[7], *cp;
   string_t ccode;
   
   // if index is zero, return the unknown country code
   if(idx == 0) {
      ccode = "*";
      return ccode;
   }
   
   // set the pointer to one character past the buffer
   cp = buf+sizeof(buf);
   
   // loop through 5-bit integers and fill the buffer
   while(idx) {
      ch = (char) (idx & 0x1F);
      *--cp = (ch + 'a' - 1);
      idx >>= 5;
   }
   
   // assign and return
   ccode.assign(cp, buf+sizeof(buf)-cp);
   
   return ccode;
}

/*********************************************/
/* GET_DOMAIN - Get domain portion of host   */
/*********************************************/

const char *get_domain(const char *str, size_t labelcnt)
{
   const char *cp;
   size_t i = labelcnt + 1;

   if(str == NULL || *str == 0)
      return NULL;

   cp = &str[strlen(str)-1];

   if (isdigitex(*cp)) return NULL;   /* ignore IP addresses */

   while(cp != str) {
      if(*cp == '.')
         if(!(--i)) return ++cp;
      cp--;
   }

   return cp;
}

char *get_url_domain(const char *url, char *buffer, size_t bsize)
{
   const char *cp1 = url, *cp2;
   size_t nlen;

   if(!cp1 || !*cp1 || *cp1 == '/')
      return NULL;

   //
   // [http[s]://]www.site.com[:port]/path
   //
   if(!strncasecmp(cp1, "http", 4)) {
      cp1 += 4;

      if(tolower(*cp1) == 's')
         cp1++;

      if(strncasecmp(cp1, "://", 3))
         return NULL;

      cp1 += 3;
   }

   if(isdigitex(*cp1)) return NULL;   // ignore IP addresses

   for(cp2 = cp1; *cp2 && *cp2 != '/' && *cp2 != ':'; cp2++);

   nlen = cp2 - cp1;

   return (strncpy_ex(buffer, bsize, cp1, nlen) == nlen) ? buffer : NULL;
}

bool is_abs_path(const char *path)
{
   if(path == NULL || *path == 0)
      return false;

   if(*path == '/')
      return true;

#ifdef _WIN32
   // check for \dir\file or \\server\share\file
   if(*path == '\\')
      return true;

   // check for c:\dir\file
   if(isalpha(*path) && path[1] == ':')
      return true;
#endif
   
   return false;
}

string_t get_cur_path(void)
{
   char buffer[PATH_MAX+1];
   string_t path;

   if(!getcwd(buffer, (int) (sizeof(buffer)-1)))
      return path;

   path = buffer;

   return path;
}

string_t make_path(const char *base, const char *path)
{
   const char *cp1;
   string_t result;

   // if there's no path, return base
   if(path == NULL || *path == 0)
      return result = base;

   // if there's no base, return path
   if(base == NULL || *base == 0)
      return result = path;

   // if path is an absolute path, return it 
   if(is_abs_path(path))
      return result = path;

   // otherwise, combine base and path
   result = base;

   cp1 = &base[strlen(base) - 1];

   if(*cp1 != '/' && *cp1 != '\\')
      result += PATH_SEP_CHAR;

   return result + path;
}

bool is_ipv4_address(const char *cp)
{
   size_t dcnt, gcnt;    // digit and group counts

   if(!cp)
      return false;

   for(gcnt = 0, dcnt = 0; *cp; cp++) {
      if(isdigitex(*cp)) {
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
      if(*cp && isdigitex(*cp)) {
         dcnt++;
      }
      else if(*cp && isxdigitex(*cp)) {
         xcnt++;
      }
      // check if it's a colon or the end of an all-colon IPv6 address
      else if(*cp == ':' || (!*cp && !dgcnt)) {
         // cannot have a colon after an IPv4 dotted group was seen
         if(dgcnt)
            return false;

         // cannot have more than six digits in a group
         if(xcnt + dcnt > 6)
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

uint64_t elapsed(uint64_t stime, uint64_t etime)
{
   return stime < etime ? etime-stime : (~stime+1)+etime;
}

//
// Formats text within the output buffer using a printf-style format and returns 
// the number of characters in the buffer, including the null character. Throws 
// an exception if the output buffer is too small.
//
// fmt_vprintf is intended as a callback formatter function for buffer_formatter_t.
//
size_t fmt_vprintf(string_t::char_buffer_t& buffer, const char *fmt, va_list args)
{
   int res;
   size_t olen;

   if((res = vsnprintf(buffer, buffer.capacity(), fmt, args)) == -1)
      throw exception_t(0, "Cannot format text because of a bad format string");

   olen = (size_t) res;

   // make sure the string fits in the buffer
   if(olen >= buffer.capacity())
      throw exception_t(0, "Cannot format text because the output buffer is too small");

   return olen + 1;
}

//
// Formats the specified number within the output buffer in a human-readable form, 
// such as 12.3 MB, and returns the number of characters in the buffer, including 
// the null character. The function throws an exception if the output buffer is too 
// small.
// 
// fmt_num is intended as a callback formatter function for buffer_formatter_t.
//
// The value of decimal indicates whether the number should be formatted in multiples 
// of 1000 or 1024.
//
// The unit prefix array must contain enough elements to format the largest unsigned 
// 64-bit value, which is 18446744073709551615.
//
// The buffer must have sufficient room for the largest formatted number, which is
// 999.9 for the decimal multiplier and 1023.9 otherwise, the separator string, the 
// prefix and the unit strings.
//
size_t fmt_hr_num(string_t::char_buffer_t& buffer, uint64_t num, const char *sep, const char *msg_unit_pfx[], const char *unit, bool decimal)
{
   double kbyte = decimal ? 1000. : 1024.;
   unsigned int prefix = 0;
   double result = (double) num;
   size_t olen, slen;
   int res;
   static const char *small_buffer = "Cannot format a human-readabale number because the output buffer is too small";
   static const char *snprintf_error = "Cannot format a human-readabale number because of a bad format string";

   // if the number is less than one kilobyte, just print the number
   if(num < (uint64_t) kbyte) {
      // MSDN describes snprintf returning a negative number on error, not -1
      if((res = snprintf(buffer, buffer.capacity(), "%" PRIu64, num)) < 0)
         throw exception_t(0, snprintf_error);

      olen = (size_t) res;

      // make sure the string fits in the buffer
      if(olen >= buffer.capacity())
         throw exception_t(0, small_buffer);

      return olen + 1;
   }

   // otherwise divide by a kilobyte in a loop to figure out the unit prefix
   do {
      result /= kbyte;
      prefix++;
   } while (result >= kbyte);

   // print round numbers as integers and otherwise with one digit of precision (e.g. print 4.96 as 5 and 4.93 as 4.9)
   if((uint64_t) ((result * 10.) + .5) % 10 == 0)
      res = snprintf(buffer, buffer.capacity(), "%" PRIu64, (uint64_t) (result + 0.5));
   else
      res = snprintf(buffer, buffer.capacity(), "%.1lf", result);

   // MSDN describes snprintf returning a negative number on error, not -1
   if(res < 0)
      throw exception_t(0, snprintf_error);

   olen = (size_t) res;

   // make sure the string fits in the buffer
   if(olen >= buffer.capacity())
      throw exception_t(0, small_buffer);

   // add the separator between the number and the suffix
   if(sep && *sep) {
      if((slen = strlen(sep)) >= buffer.capacity() - olen)
         throw exception_t(0, small_buffer);

      strcpy(buffer + olen, sep);
      olen += slen;
   }

   if((slen = strlen(msg_unit_pfx[prefix-1])) >= buffer.capacity() - olen)
      throw exception_t(0, small_buffer);

   // add the selected unit prefix
   strcpy(buffer + olen, msg_unit_pfx[prefix-1]);
   olen += slen;

   // and a unit, if there is one
   if(unit && *unit) {
      if((slen = strlen(unit)) >= buffer.capacity() - olen)
         throw exception_t(0, small_buffer);

      strcpy(buffer + olen, unit);
      olen += slen;
   }

   // account for the null character in the final length
   return olen + 1;
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
bool isinstrex(const char *str, const char *cp, size_t slen, size_t cplen, bool substr, const bmh_delta_table *deltas)
{
   const char *cp1,*cp2,*eos;

   if(str == NULL || *str == 0 || cp == NULL || *cp == 0)
      return 0;

   if(slen == 0)
      slen = strlen(str);
   eos = &str[slen];

   if(cplen == 0)
      cplen = strlen(cp);

   cp1 = &cp[cplen-1];
   if (*cp=='*')
   {
      if(slen < cplen-1)
         return false;

      /* if leading wildcard, start from end */
      cp2 = &str[slen-1];
      while (cp1 != cp && cp2 != str)
      {
         if (*cp1 == '*') return true;
         if (*cp1-- != *cp2--) return false;
      }
      return (cp1 == cp) ? true : false;
   }
   else
   {
      if(substr && *cp1 != '*') {
         /* if no leading/trailing wildcard, just strstr */
         return (strstr_ex(str, cp, slen, cplen, deltas) != NULL);
      }

      if(*cp1 == '*') {
         if(slen < cplen-1) 
            return false;
      }
      else if(slen < cplen) 
         return false;

      /* otherwise do normal forward scan */
      cp1 = cp; cp2 = str;
      while (cp2 != eos)
      {
         if (*cp1=='*') return true;
         if (*cp1++!=*cp2++) return false;
      }
      return (*cp1 == '*' || (*cp1 == 0 && cp2 == eos)) ? true : false;
   }
}

template <typename char_t>
const char_t *strptr(const char_t *str, const char_t *defstr)
{
   static const char_t empty[1] = {0};
   
   if(str)
      return str;
      
   if(defstr)
      return defstr;
      
   return empty;
}

//
// Instantiate templates
//
template const char *strptr<char>(const char *str, const char *defstr);
template const wchar_t *strptr<wchar_t>(const wchar_t *str, const wchar_t *defstr);
