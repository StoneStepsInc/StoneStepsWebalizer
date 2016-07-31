/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.cpp
*/
#include "pch.h"

#include "encoder.h"
#include "tstring.h"
#include "util.h"
#include "exception.h"

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
      if(buffer == NULL || (slen + MAX(cbc, mebc)) >= buffer.capacity())
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
   
   return slen;
}

//
// Instantiate templates
//

template size_t encode_string<encode_html_char>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_xml_char>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_js_char>(string_t::char_buffer_t& buffer, const char *str);
