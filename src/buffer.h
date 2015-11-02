/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   buffer.h
*/

#ifndef __BUFFER_H
#define __BUFFER_H

#include "types.h"
#include <memory.h>

//
// 1. buffer_t maintains a resizable block of memory.
//
class buffer_t {
   private:
      u_char *buffer;      // buffer 
      size_t bufsize;      // size of the buffer, in bytes

   public:
      buffer_t(void) : buffer(NULL), bufsize(0)
      {
      }

      buffer_t(const buffer_t& other) = delete;

      buffer_t(buffer_t&& other) : buffer(other.buffer), bufsize(other.bufsize)
      {
         other.buffer = NULL;
         other.bufsize = 0;
      }

      ~buffer_t(void)
      {
         if(buffer)
            free(buffer);
      }

      buffer_t& operator = (const buffer_t& other) = delete;

      buffer_t& operator = (buffer_t&& other)
      {
         buffer = other.buffer;
         bufsize = other.bufsize;

         other.buffer = NULL;
         other.bufsize = 0;

         return *this;
      }

      u_char *resize(size_t newsize)
      {
         if(newsize > bufsize) {
            if(buffer)
               buffer = (u_char*) realloc(buffer, newsize);
            else
               buffer = (u_char*) malloc(newsize);

            bufsize = newsize;
         }

         return buffer;
      }

      size_t get_size(void) const {return bufsize;}

      u_char *get_buffer(void) {return buffer;}

      const u_char *get_buffer(void) const {return buffer;}

      operator u_char*(void) {return buffer;}

      operator const u_char*(void) const {return buffer;}
};

#endif // __BUFFER_H
