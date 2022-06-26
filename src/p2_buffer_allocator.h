/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   p2_buffer_allocator.h
*/
#ifndef P2_BUFFER_ALLOCATOR_H
#define P2_BUFFER_ALLOCATOR_H

#include "char_buffer.h"

#include <vector>
#include <stack>

///
/// @brief  A character buffer allocator that maintains a few power-of-two sized 
///         char_buffer_base objects
///
/// The size of returned char_buffer_base objects is always rounded up to the nearest power
/// of two, compared to the requested size. The minimum size of the returned buffer is 256
/// characters.
///
template <typename char_t>
class p2_buffer_allocator_tmpl : public char_buffer_allocator_tmpl<char_t, size_t> {
   private:
      typedef std::stack<char_buffer_base<char_t>, std::vector<char_buffer_base<char_t>>> buffer_stack_t;

      std::vector<buffer_stack_t> buffers;

   public:
      p2_buffer_allocator_tmpl(void);

      char_buffer_base<char_t> get_buffer(size_t bufsize) override;

      void release_buffer(char_buffer_base<char_t>&& buffer) override;
};

#endif // P2_BUFFER_ALLOCATOR_H
