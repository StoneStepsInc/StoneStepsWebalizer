/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   char_buffer.h
*/

#ifndef __CHAR_BUFFER_H
#define __CHAR_BUFFER_H

#include <stddef.h>

//
// 1. char_buffer_base is a transient object that maintains a block of memory, large 
// enough to accommodate bufsize narrow or wide characters, depending on the template 
// argument.
//
// 2. char_buffer_base makes no assumption about the content of the buffer and does not
// ever interpret the content as a zero-terminated string.
//
// 3. Character buffers cannot be copied because there is no indication how much data is
// in the buffer and copying the entire buffer would be inefficient. Character buffers
// are intended to facilitate detaching/attaching memory from/to strings and should use
// move semantics.
//
template <typename char_t>
class char_buffer_base {
   private:
      char_t   *buffer;          // character buffer
      size_t   bufsize;          // buffer size, in characters
      bool     holder;           // if true, does not own buffer memory

   public:
      char_buffer_base(void);

      // copying character buffers would be inefficient (see #3 above)
      char_buffer_base(const char_buffer_base& other) = delete;

      char_buffer_base(char_buffer_base&& other);

      char_buffer_base(char_t *buffer, size_t bufsize, bool holder = false);

      explicit char_buffer_base(size_t bufsize);

      ~char_buffer_base(void);

      // copying character buffers would be inefficient (see #3 above)
      char_buffer_base& operator = (const char_buffer_base& other) = delete;

      char_buffer_base& operator = (char_buffer_base&& other);

      void reset(void);

      char_buffer_base& attach(char_t *buf, size_t bsize, bool hold);

      char_t *detach(size_t *bsize = NULL, bool *hold = NULL);

      char_buffer_base& resize(size_t size);

      size_t capacity(void) const {return bufsize;}

      operator char_t*(void) {return buffer;}

      operator const char_t*(void) const {return buffer;}

      bool operator == (const char_t *ptr) const {return buffer == ptr;}

      bool operator == (const char_buffer_base& other) const {return buffer == other.buffer && holder == other.holder && bufsize - other.bufsize;}

      char_t *operator + (size_t offset) {return offset < bufsize ? buffer + offset : NULL;}

      const char_t *operator + (size_t offset) const {return offset < bufsize ? buffer + offset : NULL;}

      char_t *get_buffer(void) {return buffer;}

      const char_t *get_buffer(void) const {return buffer;}

      size_t memsize(void) const {return bufsize * sizeof(char_t);}

      bool isholder(void) const {return holder;}

      bool isempty(void) const {return bufsize == 0;}

      //
      // Buffer memory management functions
      //
      static char_t *alloc(char_t *buffer, size_t bufsize);

      static char_t *alloc(size_t bufsize);

      static void free(char_t *buffer);

      static size_t memsize(size_t char_count);
};

//
// A fixed-size character buffer that can be used as a temporary storage in a local 
// scope. For example, this code creates a temporary string without having to allocate
// memory:
//
//    string_t::fixed_char_buffer_t<hnode_t::ccode_size+1> ccode_buffer;
//    string_t ccode(ccode_buffer, 0, false);
//    ccnode_t *ccptr = &state.cc_htab.get_ccnode(hnode.get_ccode(ccode));
//
//
template <typename char_t, size_t BUFSIZE>
class fixed_char_buffer_t : public char_buffer_base<char_t> {
   private:
      char_t   fixed_buffer[BUFSIZE];

   public:
      fixed_char_buffer_t(void) : char_buffer_base<char_t>(fixed_buffer, BUFSIZE, true)
      {
      }
};

#endif // __CHAR_BUFFER_H
