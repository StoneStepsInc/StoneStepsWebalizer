/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.cpp
*/
#include "pch.h"

#include "encoder.h"
#include "tstring.h"
#include "exception.h"

#include <algorithm>

char *encode_char_html(const char *cp, size_t cbc, char *op, size_t& obc)
{
   // check if we need to return the length of the longest encoded sequence
   if(cp == NULL) {
      obc = 6;
      return op;
   }

   switch (*cp) {
      case '<':
         obc = 4;
         memcpy(op, "&lt;", 4);
         break;
      case '>':
         obc = 4;
         memcpy(op, "&gt;", 4);
         break;
      case '&':
         obc = 5;
         memcpy(op, "&amp;", 5);
         break;
      case '"':
         obc = 6;
         memcpy(op, "&quot;", 6);
         break;
      case '\'':
         obc = 6;
         memcpy(op, "&#x27;", 6);
         break;
      default:
         obc = cbc;
         memcpy(op, cp, cbc);
         break;
      }
      
   return op;
}

char *encode_char_xml(const char *cp, size_t cbc, char *op, size_t& obc)
{
   // check if we need to return the length of the longest encoded sequence
   if(cp == NULL) {
      encode_char_html(NULL, cbc, op, obc);
      if(obc < 6)
         obc = 6;
      return op;
   }
         
   if(*cp == '\'') {
      obc = 6;
      memcpy(op, "&apos;", 6);
      return op;
   }
      
   return encode_char_html(cp, cbc, op, obc);
}

char *encode_char_js(const char *cp, size_t cbc, char *op, size_t& obc)
{
   // check if we need to return the length of the longest encoded sequence
   if(cp == NULL) {
      obc = 6;
      return op;
   }

   switch (*cp) {
      case '\'':
         obc = 2;
         memcpy(op, "\\'", 2);
         break;
      case '\"':
         obc = 2;
         memcpy(op, "\\\"", 2);
         break;
      case '\r':
         obc = 2;
         memcpy(op, "\\r", 2);
         break;
      case '\n':
         obc = 2;
         memcpy(op, "\\n", 2);
         break;
      case '\xE2':
         // check if the sequence is either E2 80 A8 (LS) or E2 80 A9 (PS)
         if(*(cp+1) == '\x80') {
            if(*(cp+2) == '\xA8') {
               obc = 6;
               memcpy(op, "\\u2028", 6);
            }
            else if(*(cp+2) == '\xA9') {
               obc = 6;
               memcpy(op, "\\u2029", 6);
            }
            else {
               obc = 3;
               memcpy(op, cp, 3);
            }
         }
         else {
            obc = 3;
            memcpy(op, cp, 3);
         }
         break;
      default:
         obc = cbc;
         memcpy(op, cp, cbc);
         break;
   }

   return op;
}

template <encode_char_t encode_char>
size_t encode_string(string_t::char_buffer_t& buffer, const char *str)
{
   const char *cptr = str;
   size_t slen = 0;
   size_t cbc, ebc, mebc;  // character byte count, encoded byte count and maximum encoded byte count

   // hold onto the size of the longest encoded sequence
   encode_char(NULL, 0, NULL, mebc);

   while(*cptr) {
      // get the input character size in bytes (may be zero, if invalid)
      cbc = utf8size(cptr);

      // always require that any encoded sequence or any UTF-8 character fits in the buffer
      if(buffer == NULL || (slen + std::max(cbc, mebc)) >= buffer.capacity())
         throw exception_t(0, "Insufficient buffer capacity");
      
      // check for invalid UTF-8 sequences and control characters, except \t, \r and \n
      if(cbc == 0 || ((unsigned char) *cptr < '\x20' && !strchr("\t\r\n", *cptr)) || *cptr == '\x7F') {
         // replace the bad character with a private-use code point [Unicode v5 ch.3 p.91]
         slen += ucs2utf8((wchar_t) (0xE000 + ((u_char) *cptr)), buffer+slen);
         cptr++;
         continue;
      }
         
      // encode one UTF-8 character
      encode_char(cptr, cbc, buffer+slen, ebc);

      // bump up the encoded length 
      slen += ebc;

      // and move the pointer to the next UTF-8 character
      cptr += cbc;
   }

   // the check inside the loop guarantees that the buffer has room for the null character
   buffer[slen] = 0;
   
   // include the null character in the returned size
   return slen + 1;
}

//
// Instantiate templates
//
template size_t encode_string<encode_char_html>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_char_xml>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_char_js>(string_t::char_buffer_t& buffer, const char *str);
