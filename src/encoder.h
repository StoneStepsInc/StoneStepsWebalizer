/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.h
*/
#ifndef __ENCODER_H
#define __ENCODER_H

#include "unicode.h"

#include <stddef.h>

//
// buffer_encoder_t uses encode_char_t to encode a single vaid UTF-8 character.
// This function should not evaluate any bytes beyond the size of the character.
//
// If NULL is passed in, the function should return the length of the longest
// enchoded character sequence. In this case the input character size and the
// output character pointer are not evaluated and may be zero and NULL.
//
// The size of the output buffer pointed to by op must be large enough to
// accommodate the largest encoded sequence.
//
// The caller must never assume that the returned sequence is null-terminated 
// and should always rely on the returned size to determine the length of the
// encoded sequence.
//
typedef char *(*encode_char_t)(const char *cp, size_t cbc, char *op, size_t& obc);

//
// buffer_encoder_t provides a quick way to encode a string within a pre-allocated 
// buffer using the encoding function pointer passed as a template argument. The 
// encoding mode may be set to append to the last encoding result or overwrite it.
//
// Copying an encoder with a new mode creates an encoder that uses only free space 
// from the original buffer, which makes it possible to manipulate the new buffer
// without affecting the original encoder. For example, provided that each character
// below is encoded as a single narrow character in the buffer, the encoder inside
// the for loop will have a 38-byte buffer after it's constructed.
//
//    buffer_encoder_t<x> encoder(buffer, 50, buffer_encoder_t<x>::append);
//    buffer_encoder_t<x>& encode = encoder;    // alias to make it look like a function name
//
//    printf("%s %s\n", encode("1234"), encode("567890"));
//
//    for(int i = 0; i < max; i++) {
//       buffer_encoder_t<x> encoder(encoder, buffer_encoder_t<x>::overwrite);
//       buffer_encoder_t<x>& encode = encoder;
//       printf("%s\n", encode(cp3));
//       printf("%s\n", encode(cp4));
//    }
//
template <encode_char_t encode_char>
class buffer_encoder_t {
   public:
      enum mode_t {append, overwrite};

      //
      // scope_mode_t changes the mode of the specified encoder within a given scope. The 
      // original encoder mode is restored after the instance of scope_mode_t holding the
      // encoder is destroyed.
      //
      // scope_mode_t is useful where a new encoder with a new mode cannot be instantiated.
      // For example, cp1 and cp2 will be encoded within the same buffer space, while cp3
      // and cp4 will be encoded one after another (note the comma after set_scope_mode is
      // called):
      //
      //    buffer_encoder_t<x> encode(buffer, bufsize, buffer_encoder_t<x>::overwrite);
      //
      //    printf("%s\n", encode(cp1);
      //    printf("%s\n", encode(cp2);
      //
      //    encode.set_scope_mode(encode, buffer_encoder_t<x>::append),
      //    printf("%s %s\n", encode(cp3), encode(cp4));
      //
      // Only one active encoder exists across all instances of scope_mode_t copied from 
      // the first one. This ensures that the active encoder isn't reset multiple times. 
      //
      class scope_mode_t {
         private:
            buffer_encoder_t<encode_char> *encoder;         // active encoder

            buffer_encoder_t<encode_char> saved_encoder;    // copy of the original encoder

         public:
            scope_mode_t(buffer_encoder_t<encode_char>& encoder, typename buffer_encoder_t<encode_char>::mode_t newmode) : encoder(&encoder), saved_encoder(encoder) 
            {
               this->encoder->set_mode(newmode);
            }

            scope_mode_t(scope_mode_t&& other) : encoder(other.encoder), saved_encoder(other.saved_encoder)
            {
               other.encoder = NULL;
            }

            ~scope_mode_t(void) 
            {
               if(encoder)
                  *encoder = saved_encoder;
            }
      };

   private:
      char *buffer;        // start of the buffer
      char *cptr;          // first unused byte
      size_t bufsize;      // size of the entire buffer
      mode_t mode;         // move cptr forward after each encode call?

   private:
      static char *encode(const char *str, char *buffer, size_t bsize, size_t *slen);

   public:
      buffer_encoder_t(char *buffer, size_t bufsize, mode_t mode);

      buffer_encoder_t(const buffer_encoder_t& other, mode_t mode);

      buffer_encoder_t(const buffer_encoder_t& other) = default;

      void reset(void) {cptr = buffer;}

      char *encode(const char *str, size_t *olen = NULL);

      char *operator () (const char *str, size_t *olen = NULL)
      {
         return encode(str, olen);
      }

      void set_mode(mode_t newmode) 
      {
         mode = newmode;
      }

      typename buffer_encoder_t<encode_char>::scope_mode_t set_scope_mode(mode_t newmode)
      {
         scope_mode_t scope_mode(*this, newmode);
         return scope_mode; 
      }
};

//
// Character encoding functions for encode_string
//
char *encode_html_char(const char *cp, size_t cbc, char *op, size_t& obc);
char *encode_xml_char(const char *cp, size_t cbc, char *op, size_t& obc);
char *encode_js_char(const char *cp, size_t cbc, char *op, size_t& obc);

template <encode_char_t encode_char> size_t encode_string(string_t::char_buffer_t& buffer, const char *str);

#endif // __ENCODER_H
