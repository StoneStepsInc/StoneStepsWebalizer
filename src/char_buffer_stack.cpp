/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   char_buffer_stack.cpp
*/
#include "pch.h"

#include "char_buffer_stack.h"

template <typename char_t>
char_buffer_base<char_t> char_buffer_stack_tmpl<char_t>::get_buffer(void)
{
   char_buffer_base<char_t> buffer;

   if(!buffers.empty()) {
      buffer = std::move(buffers.top());
      buffers.pop();
   }

   return buffer;
}

template <typename char_t>
void char_buffer_stack_tmpl<char_t>::release_buffer(char_buffer_base<char_t>&& buffer)
{
   buffers.push(std::move(buffer));
}

//
// Instatiate the char buffer stack for its intended character types
//
template class char_buffer_stack_tmpl<char>;
template class char_buffer_stack_tmpl<unsigned char>;
