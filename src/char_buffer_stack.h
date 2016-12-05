/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   char_buffer_queue.h
*/
#ifndef CHAR_BUFFER_STACK_H
#define CHAR_BUFFER_STACK_H

#include "char_buffer.h"

#include <stack>
#include <vector>

//
// char_buffer_stack_tmpl maintains all cached buffers in a single vector and is 
// intended to reuse buffers of the same size or buffers that are repeatedly 
// allocated and released in the same LIFO sequence. 
//
// The caller must ensure that the returned buffer has sufficient capcity. The 
// buffer stack does not alter the capacity of any buffers it receives or returns.
//
template <typename char_t>
class char_buffer_stack_tmpl : public char_buffer_allocator_tmpl<char_t> {
   private:
      typedef std::stack<char_buffer_base<char_t>, std::vector<char_buffer_base<char_t>>> buffer_stack_t;

   private:
      buffer_stack_t buffers;

   public:
      char_buffer_base<char_t> get_buffer(void) override;

      void release_buffer(char_buffer_base<char_t>&& buffer) override;
};



#endif // CHAR_BUFFER_STACK_H
