/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.h
*/
#ifndef __ENCODER_H
#define __ENCODER_H

//
// Text encoder function (HTML, XML, TSV, etc)
//
typedef char *(*encodecb_t)(const char *str, char *buffer, size_t bsize, bool multiline, size_t *olen);

//
// buffer_encoder_t provides a quick way to encode a string within a pre-allocated 
// buffer using the encoding function pointer passed as a template argument. The 
// encoding mode may be set to append to the last encoding result or overwrite it.
//
// Copying an encoder with a new mode creates an encoder that uses only free space 
// from the original buffe, which allows new buffer to be reset without affecting
// the original encoder. For example, provided that each character below is encoded
// as a single character in the buffer, the encoder inside the for loop will have a
// 40-byte buffer after it's constructed.
//
//    buffer_encoder_t<x> encoder(buffer, 50, buffer_encoder_t<x>::append);
//    buffer_encoder_t<x>& encode;
//
//    printf("%s %s\n", encode("1234"), encode("567890"));
//
//    for(int i = 0; i < max; i++) {
//       buffer_encoder_t<x> encoder(encoder, buffer_encoder_t<x>::overwrite);
//       printf("%s\n", encode(cp3));
//       printf("%s\n", encode(cp4));
//    }
//
template <encodecb_t encodecb>
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
      // and cp4 will be encoded one after another:
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
      template <encodecb_t encodecb>
      class scope_mode_t {
         private:
            buffer_encoder_t<encodecb> *encoder;         // active encoder

            buffer_encoder_t<encodecb> saved_encoder;    // original encoder

         public:
            scope_mode_t(buffer_encoder_t<encodecb>& encoder, typename buffer_encoder_t<encodecb>::mode_t newmode) : encoder(&encoder), saved_encoder(encoder) 
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

   public:
      buffer_encoder_t(char *buffer, size_t bufsize, mode_t mode);

      buffer_encoder_t(const buffer_encoder_t& other, mode_t mode);

      buffer_encoder_t(const buffer_encoder_t& other) = default;

      void reset(void) {cptr = buffer;}

      char *encode(const char *str, bool multiline = false, size_t *olen = NULL);

      char *operator () (const char *str, bool multiline = false, size_t *olen = NULL)
      {
         return encode(str, multiline, olen);
      }

      void set_mode(mode_t newmode) 
      {
         mode = newmode;
      }

      typename buffer_encoder_t<encodecb>::scope_mode_t<encodecb> set_scope_mode(mode_t newmode)
      {
         scope_mode_t<encodecb> scope_mode(*this, newmode);
         return scope_mode; 
      }
};

#endif // __ENCODER_H
