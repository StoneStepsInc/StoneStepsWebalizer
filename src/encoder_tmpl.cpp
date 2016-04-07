/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.cpp
*/
#include "pch.h"

#include "encoder.h"
#include "util.h"
#include <algorithm>

template <encode_char_t encode_char>
buffer_encoder_t<encode_char>::buffer_encoder_t(char *buffer, size_t bufsize, mode_t mode) : 
      buffer(buffer), 
      bufsize(bufsize), 
      cptr(buffer), 
      mode(mode) 
{
}

template <encode_char_t encode_char>
buffer_encoder_t<encode_char>::buffer_encoder_t(const buffer_encoder_t& other, mode_t mode) : 
      buffer(other.cptr), 
      bufsize(other.bufsize - (other.cptr-other.buffer)), 
      cptr(buffer), 
      mode(mode) 
{
} 

template <encode_char_t encode_char>
char *buffer_encoder_t<encode_char>::encode(const char *str, size_t *olptr)
{
   char *out;
   size_t olen, avsize;

   // check if cptr points one element past the end of the buffer
   if(!(avsize = bufsize - (cptr-buffer)))
      return NULL;

   // encode the string in the available buffer space
   if((out = encode(str, cptr, avsize, &olen)) == NULL)
      return NULL;

   // move the current pointer to include the null character
   if(mode == append)
      cptr += olen + 1;

   if(olptr)
      *olptr = olen;

   return out;
}

template <encode_char_t encode_char>
char *buffer_encoder_t<encode_char>::encode(const char *str, char *buffer, size_t bsize, size_t *olen)
{
   const char *cptr = str;
   size_t slen = 0, cbc, ebc, mebc;

   // hold onto the size of the longest encoded sequence
   encode_char(NULL, 0, NULL, mebc);

   while(*cptr) {
      // get the input character size in bytes (may be zero, if invalid)
      cbc = utf8size(cptr);

      // always require that any encoded sequence or any UTF-8 character fits in the buffer
      if(buffer == NULL || (slen + MAX(cbc, mebc)) > bsize) {
         buffer = NULL;
         break;
      }
      
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

   if(buffer && slen < bsize)
      buffer[slen] = 0;
   
   // update output length, if requested   
   if(olen)
      *olen = slen;

   return buffer;
}
