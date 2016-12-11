/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   p2_buffer_allocator.cpp
*/
#include "pch.h"

#include "p2_buffer_allocator.h"
#include "util.h"
#include <algorithm>

template <typename char_t>
p2_buffer_allocator_tmpl<char_t>::p2_buffer_allocator_tmpl(void) : buffers(24) // 32-8 bits
{
}

template <typename char_t>
char_buffer_base<char_t> p2_buffer_allocator_tmpl<char_t>::get_buffer(size_t bufsize)
{
   char_buffer_base<char_t> buffer;

   // enforce the minimum size of 256 characters and round it up to the nearest power of two
   bufsize = ceilp2((uint32_t) std::max(bufsize, (size_t) 256u));

   // get the bucket for the requested buffer size
   buffer_stack_t& buffer_stack = buffers[tzbits(bufsize) - 8];

   // and either return a buffer from the top of the stack or allocate a new one
   if(buffer_stack.empty())
      buffer.resize(bufsize);
   else {
      buffer = std::move(buffer_stack.top());
      buffer_stack.pop();
   }

   // move the buffer to the return value
   return buffer;
}

template <typename char_t>
void p2_buffer_allocator_tmpl<char_t>::release_buffer(char_buffer_base<char_t>&& buffer)
{
   // enforce buffer size again to catch buffers that weren't allocated by get_buffer
   size_t bufsize = (size_t) std::max(ceilp2((uint32_t) buffer.capacity()), (uint32_t) 256u);

   // if buffer size isn't a power of two, resize it to match one of the buckets
   if(bufsize != buffer.capacity())
      buffer.resize(bufsize);

   // get the bucket for the requested buffer size
   buffer_stack_t& buffer_stack = buffers[tzbits(bufsize) - 8];

   // keep a few buffers and deallocate any extras
   if(buffer_stack.size() <= 16)
      buffer_stack.push(std::move(buffer));
   else
      buffer.reset();
}

//
// Instatiate the power of two char buffer allocator for its intended character types
//
template class p2_buffer_allocator_tmpl<char>;
template class p2_buffer_allocator_tmpl<unsigned char>;
