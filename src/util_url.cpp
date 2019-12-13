/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_url.cpp
*/
#include "pch.h"

#include "util_url.h"

#include "unicode.h"
#include "cp1252.h"

#include <algorithm>
#include <stdexcept>

static char *to_hex(unsigned char cp, char *out);
static char from_hex(char c);

char *to_hex(unsigned char cp, char *out)
{
   if(!out)
      throw std::invalid_argument("Output argument of to_hex cannot be nullptr");

   if(!cp)
      throw std::invalid_argument("A null character argument is not allowed in to_hex");

   *out++ = (cp & 0xF0) <= 0x90 ? (cp >> 4) + '0' : (cp >> 4) - 10 + 'A';
   *out++ = (cp & 0xF) <= 0x9 ? (cp & 0xF) + '0' : (cp & 0xF) - 10 + 'A';

   return out;
}

const char *from_hex(const char *cp1, char *cp2)
{
   if(!cp1 || !cp2)
      throw std::invalid_argument("Neither argument of from_hex can be nullptr");

   // convert the hex number to an octet
   *cp2 = from_hex(*cp1++) << 4;
   *cp2 |= from_hex(*cp1++);

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

///
/// All non-UTF-8 characters are assumed to be encoded as CP1252 and converted to 
/// UTF-8. This covers most maformed URLs, but will mangle characters from non-Latin1
/// character sets. There is no way around that, as it is impossible to figure out the 
/// actual character set of an arbitrary character.
///
/// No assumption is made about the format of the string, so it can be a URL, a user
/// agent with URL-encoded characters, etc. One side effect of this is that the '+'
/// character is not converted to a space because such conversion can be done only 
/// for form arguments and without knowing the underlying string format, it could
/// mangle the string.
/// ```
///  x%25y      -> x%25y        : reserved URL characters are not URL-decoded
///  x+y        -> x+y          : the plus character is not converted to a space (see above)
///  x%20y      -> x y          : the space character is URL-decoded 
///  x%41y      -> xAy          : ASCII alpha-numeric characters are URL-decoded
///  x%y        -> x%25y        : a misplaced percent character is URL-encoded
///  xAy        -> xAy          : plain ASCII alpha-numeric characters are not changed
///  x\x01y     -> x%01y        : control characters are URL-encoded
///  x%A3y      -> x\xC2\xA3y   : non-UTF-8 characters are URL-decoded and converted to UTF-8
///  x\xA3y     -> x\xC2\xA3y   : non-UTF-8 characters are converted to UTF-8
///  %E8%A8%98  -> \xE8\xA8\x98 : UTF-8 characters are URL-decoded
///  x\xC2\xA3y -> x\xC2\xA3y   : well-formed UTF-8 characters are not changed
/// ```
/// Normalized URLs are suitable for display and can be copied into the browser URL
/// box, but they are not valid URLs because non-ASCII characters must be percent-encoded 
/// in URLs.
///
/// The strbuf parameter is used as an intermediary string buffer to minimize memory
/// allocations when the caller needs to normalize multiple URL strings one after another.
///
void norm_url_str(string_t& str, string_t::char_buffer_t& strbuf)
{
   size_t chsz, of;
   const char *cp1, *cp2;
   char *bcp;
   char chr[2] = {0};

   // keep buffer reallocation in this function, so there's no extra parameter validation required
   auto realloc_buffer = [] (string_t::char_buffer_t& buf, char *bcp) -> char*
   {
      // hold onto the current buffer offset (bcp points to the first unused byte, possibly past current buffer size)
      size_t of = bcp - buf;

      // grab half the current buffer size more
      buf.resize(std::max((size_t) 32, buf.capacity() + buf.capacity() / 2), of);

      // set the output pointer to the same position within the new buffer
      bcp = buf + of;

      return bcp;
   };

   if(str.isempty())
      return;

   //
   // Look for a URL-encoded sequence, a control character or a non-UTF-8 character. 
   // All characters up to such character do not need to be examined again.
   //
   for(cp1 = str.c_str(); *cp1; cp1 += chsz) {
      if((unsigned char) *cp1 < '\x20' || (unsigned char) *cp1 == '\x7F')
         break;

      if(*cp1 == '%')
         break;

      if((chsz = utf8size(cp1)) == 0)
         break;
   }

   // nothing to change if we reached the end of the string
   if(!*cp1)
      return;

   //
   // A 3-byte URL-encoded sequence translates into one byte of output. A misplaced percent 
   // character or a control character translates into three bytes of URL-encoded output. 
   // The size of the remainder of the original string should be large enough to cover most 
   // cases and more memory will be allocated, if required.
   //
   strbuf.resize(str.length() - (cp1 - str.c_str()) + sizeof(char), 0);

   // set up the source and the destination pointers
   bcp = strbuf;
   cp2 = cp1;

   // decode URL-encoded sequences and fix misplaced % characters
   while(*cp2) {
      // check if we have room for at least one URL-encoded sequence and the null character
      if(strbuf.capacity() - (bcp - strbuf) < 4)
         bcp = realloc_buffer(strbuf, bcp);

      if(*cp2 != '%')
         // URL-encode control charactrs and copy anything else
         if((unsigned char) *cp2 < '\x20' || (unsigned char) *cp2 == '\x7F')
            *bcp++ = '%', bcp = to_hex(*cp2++, bcp);
         else 
            *bcp++ = *cp2++;
      else {
         cp2++;
         if(string_t::isxdigit(*cp2) && string_t::isxdigit(*(cp2+1))) {
            from_hex(cp2, chr);

            // do not decode URL component separators because it is irreversible or control characters
            if((unsigned char) *chr < '\x20' || (unsigned char) *chr == '\x7F' || strchr(":/?#[]@!$&'()*+,;=%", (unsigned char) *chr))
               *bcp++ = '%', *bcp++ = string_t::toupper(*cp2++), *bcp++ = string_t::toupper(*cp2++);
            else
               *bcp++ = *chr, cp2 += 2;
         }
         else { 
            // URL-encode a misplaced percent character
            *bcp++ = '%', *bcp++ = '2', *bcp++ = '5';
         }
      }
   }

   // add a null character to the buffer string
   strbuf[bcp - strbuf] = 0;

   // hold onto the current offset to the first URL-encoded sequence or a non-UTF-8 character
   of = cp1 - str.c_str();

   // detach the source buffer and set up the pointers to copy characters from the buffer string
   string_t::char_buffer_t buf = str.detach();
   bcp = buf + of;

   cp2 = strbuf;

   // convert all non-UTF-8 characters as if they are in CP1252
   while(*cp2) {
      //
      // Check if we have enough room in the buffer for the next sequence, which can 
      // be up to 4 bytes for a UTF-8 character or 2 bytes for a CP1252 character 
      // converted to UTF-8, plus the null terminator.
      //
      if(buf.capacity() - (bcp - buf) < 5)
         bcp = realloc_buffer(buf, bcp);

      // interpret non-UTF-8 characters as CP1252 (Latin1)
      if((chsz = utf8size(cp2)) == 0) {
         cp1252utf8(cp2, 1, bcp, 2, &chsz);
         bcp += chsz;
         cp2++;
      }
      else {
         // copy UTF-8 character bytes one by one
         while(chsz) {
            *bcp++ = *cp2++;
            chsz--;
         }
      }
   }

   // attach normalized characters back to the original string
   str.attach(std::move(buf), bcp - buf);
}

///
/// It is expected that the input string is normalized by norm_url_str.
///
string_t& url_encode(const string_t& str, string_t& out)
{
   size_t chsz, of;
   string_t::char_buffer_t buf;
   const char *cp;
   char *bcp;
   
   out.reset();
   
   if(str.isempty())
      return out;

   cp = str.c_str();

   // the encoded string will be at least as long as the original
   out.reserve(str.length());
      
   // detach the buffer
   buf = out.detach();
   bcp = buf;

   // URL-encode all 2+ byte characters, space and control characters
   while(*cp) {
      // url_encode may only be called after norm_url_str
      if((chsz = utf8size(cp)) == 0)
         throw std::invalid_argument("Bad UTF-8 character in the URL");

      // the size includes the null character
      size_t reqsz = chsz > 1 || (unsigned char) *cp <= '\x20' || (unsigned char) *cp == '\x7F' || strchr("\"<>", (unsigned char) *cp) ? chsz * 3 + 1 : 2;

      // check if the buffer has enough room for the next sequence
      if(buf.capacity() - (bcp - buf) < reqsz) {
         // hold onto the current buffer offset in case the buffer is moved
         of = bcp - buf;

         // half of the minimum (32) should be enough to accommodate 4 code units * 3 pct-seq + 1 null character
         buf.resize(std::max((size_t) 32, buf.capacity() + buf.capacity() / 2), of);
         bcp = buf + of;
      }

      // copy safe one-byte characters
      if(reqsz == 2)
         *bcp++ = *cp++;
      else {
         // and URL-encode everything else
         while(chsz) {
            *bcp++ = '%';
            bcp = to_hex(*cp++, bcp);
            chsz--;
         }
      }
   }
      
   // and attach it back
   out.attach(std::move(buf), bcp - buf);
   
   return out;
}

///
/// The returned temporary holder buffer points to the domain name within `url` and
/// its size is set to indicate the length of the domain name.
///
string_t::const_char_buffer_t get_url_host(const char *url, size_t slen)
{
   const char *cp1 = url, *cp2, *atp = nullptr, *colp = nullptr;

   if(!cp1 || !*cp1 || !slen)
      return string_t::const_char_buffer_t();

   //
   // [http[s]:]//[user[:password]@]]www.site.com[:port][/path]
   //
   if(slen >= 4 && !string_t::compare_ci(cp1, "http", 4)) {
      cp1 += 4;

      if(slen >= 5) {
         if(string_t::tolower(*cp1) == 's')
            cp1++;
      }

      // check the remaining length, as from this point the number of preceding characters varies
      if(cp1 - url == slen || *cp1 != ':')
         return string_t::const_char_buffer_t();

      cp1++;
   }

   // check for double slashes preceding the host name
   if(slen - (cp1 - url) < 2 || *cp1 != '/' || *(cp1 + 1) != '/')
      return string_t::const_char_buffer_t();

   // skip both slashes
   cp1 += 2;

   // look for either the first slash or the end of the string
   for(cp2 = cp1; ((size_t) (cp2 - url) < slen) && *cp2 != '/'; cp2++) {
      if(*cp2 == ':') {
         // a colon may start a password or a port
         colp = cp2;
      }
      else if(*cp2 == '@') {
         atp = cp2;
         // if we saw a colon, it was a password separator
         colp = nullptr;
      }
   }

   // if we saw an at sign, skip the user name and password
   if(atp)
      cp1 = atp + 1;

   // a non-nullptr colon pointer points is a port delimiter
   if(colp)
      cp2 = colp;

   // return a host name wrapped into a const character holder buffer
   return string_t::const_char_buffer_t(cp1, cp2 - cp1, true);
}

string_t& get_url_host(const char *url, string_t& domain)
{
   const char *cp1 = url;
   string_t::const_char_buffer_t host;

   domain.reset();

   if(!url || !*url)
      return domain;

   host = get_url_host(url, strlen(url));

   if(!host || !*host)
      return domain;

   // ignore IP addresses
   if(string_t::isdigit(host[0])) 
      return domain;

   domain.assign(host, host.capacity());

   return domain;
}

const char *get_domain(const char *str, size_t labelcnt)
{
   const char *cp;
   size_t i = labelcnt + 1;

   if(str == nullptr || *str == 0)
      return nullptr;

   cp = &str[strlen(str)-1];

   if (string_t::isdigit(*cp)) return nullptr;   /* ignore IP addresses */

   while(cp != str) {
      if(*cp == '.')
         if(!(--i)) return ++cp;
      cp--;
   }

   return cp;
}
