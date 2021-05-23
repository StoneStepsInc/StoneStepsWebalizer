/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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
   if(cp == nullptr) {
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
   if(cp == nullptr) {
      encode_char_html(nullptr, cbc, op, obc);
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
   if(cp == nullptr) {
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

char *encode_char_json(const char *cp, size_t cbc, char *op, size_t& obc)
{
   // check if we need to return the length of the longest encoded sequence
   if(cp == nullptr) {
      //
      // This is the longest encoded sequence size, not the longest output
      // size. Unlike with other encoder functions, the longest encoded
      // sequence is shorter than the maximum character size returned from
      // this function.
      //
      obc = 2;
      return op;
   }

   //
   // Encode accorting to the rules at www.json.org, but leave the
   // forward slash slash character as is (it makes sense to encode
   // it only in HTML context, when attackers can get end the script
   // context). JSON spec only requires control characters, double
   // quote and backslash escaped with a backslash.
   // 
   // Control characters '\b' and '\f' are not explicitly listed and
   // will be converted to private-use code points by encode_string.
   //
   switch (*cp) {
      case '\"':
      case '\\':
         obc = 2;
         *op++ = '\\';
         *op++ = *cp;
         break;
      case '\r':
         obc = 2;
         memcpy(op, "\\r", 2);
         break;
      case '\n':
         obc = 2;
         memcpy(op, "\\n", 2);
         break;
      case '\t':
         obc = 2;
         memcpy(op, "\\t", 2);
         break;
      default:
         //
         // Control characters are converted to private-use code points
         // by the caller and all other Unicode sequences are allowed
         // in JSON verbatim.
         //
         obc = cbc;
         memcpy(op, cp, cbc);
   }

   return op;
}

///
/// The function does not change the size of the buffer and just checks that the
/// encoded string fits in the buffer.
///
/// It is expected that the buffer always has room for one longest encoded sequence,
/// even if it is not going to be used. This simplifies the conversion by not having
/// to perform a test conversions for each character to see if its encoded sequence
/// fits into the buffer.
/// 
/// The longest encoded sequence may be shorter than the longest character returned
/// from the encoder function. For example, JSON encoder may yield 2-byte encoded
/// sequences and 4-byte UTF-8 characters. When called with a null pointer input,
/// it returns `2`, not `4`. This allows us to require only two reserved bytes at
/// the end of the buffer, not four for 1- and 2-byte characters.
///
/// For example, the longest encoded sequence for HTML is six bytes (i.e. `&#x27;`),
/// so in a 7-byte buffer a single character `A` will be placed in the first byte,
/// the null character will be placed in the second byte and the remaining 5 bytes
/// will not be used. If, on the other hand, a single quote character is encoded in
/// the same buffer, the first six bytes will contain the sequence `&#x27;` and the
/// last byte will contain the null character.
///
/// The function returns the number of bytes the encoded string occupies within 
/// the buffer, including the null character.
///
template <encode_char_t encode_char>
size_t encode_string(string_t::char_buffer_t& buffer, const char *str)
{
   const char *cptr = str;
   size_t slen = 0;
   size_t cbc, ebc, mebc;  // character byte count, encoded byte count and maximum encoded byte count

   // hold onto the size of the longest encoded sequence
   encode_char(nullptr, 0, nullptr, mebc);

   while(*cptr) {
      // get the input character size in bytes (may be zero, if invalid)
      cbc = utf8size(cptr);

      // always require that any encoded sequence or any UTF-8 character fits in the buffer and there's room for the null character
      if(buffer == nullptr || (slen + std::max(cbc, mebc)) >= buffer.capacity())
         throw exception_t(0, "Insufficient buffer capacity");
      
      // check for invalid UTF-8 sequences and control characters, except \t, \r and \n
      if(cbc == 0 || cbc == 1 && (((unsigned char) *cptr < '\x20' && !strchr("\t\r\n", *cptr)) || *cptr == '\x7F')) {
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

   // the >= check inside the loop guarantees that the buffer has room for the null character
   buffer[slen] = '\x0';
   
   // include the null character in the returned size
   return slen + 1;
}

//
// Instantiate templates
//
template size_t encode_string<encode_char_html>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_char_xml>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_char_js>(string_t::char_buffer_t& buffer, const char *str);
template size_t encode_string<encode_char_json>(string_t::char_buffer_t& buffer, const char *str);
