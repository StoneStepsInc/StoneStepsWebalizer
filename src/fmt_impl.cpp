/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   fmt_impl.cpp
*/
#include "pch.h"

#include "fmt_impl.h"
#include "exception.h"

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
// fmt_hr_num is intended as a callback formatter function for buffer_formatter_t.
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
size_t fmt_hr_num(string_t::char_buffer_t& buffer, uint64_t num, const char *sep, const char * const msg_unit_pfx[], const char *unit, bool decimal)
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

