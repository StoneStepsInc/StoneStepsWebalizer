/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include <memory.h>
#include "char_buffer.h"

template <typename char_t>
char_buffer_base<char_t>::char_buffer_base(void) : buffer(NULL), bufsize(0), holder(false)
{
}

template <typename char_t>
char_buffer_base<char_t>::char_buffer_base(char_buffer_base&& other)
{
   buffer = other.detach(&bufsize, &holder);
}

template <typename char_t>
char_buffer_base<char_t>::char_buffer_base(char_t *buffer, size_t bufsize, bool holder) : buffer(buffer), bufsize(bufsize), holder(holder) 
{
}

template <typename char_t>
char_buffer_base<char_t>::char_buffer_base(size_t bufsize) : bufsize(bufsize), holder(false)
{
   buffer = (char_t*) malloc(bufsize * sizeof(char_t));
}

template <typename char_t>
char_buffer_base<char_t>::~char_buffer_base(void) 
{
   // release the memory if we own the buffer
   if(!holder && buffer)
      free(buffer);
}

template <typename char_t>
char_buffer_base<char_t>& char_buffer_base<char_t>::operator = (char_buffer_base<char_t>&& other)
{
   buffer = other.detach(&bufsize, &holder);

   return *this;
}

template <typename char_t>
void char_buffer_base<char_t>::reset(void)
{
   if(!holder && buffer)
      free(buffer);

   buffer = NULL;
   bufsize = 0;
   holder = false;
}

template <typename char_t>
char_buffer_base<char_t>& char_buffer_base<char_t>::attach(char_t *buf, size_t bsize, bool hold)
{
   if(!holder && buffer)
      free(buffer);

   buffer = buf;
   bufsize = bsize;
   holder = hold;

   return *this;
}

template <typename char_t>
char_t *char_buffer_base<char_t>::detach(size_t *bsize, bool *hold)
{
   char_t *buf = buffer;

   if(bsize)
      *bsize = bufsize;

   if(hold)
      *hold = holder;

   buffer = NULL;
   bufsize = 0;
   holder = false;

   return buf;
}

template <typename char_t>
char_buffer_base<char_t>& char_buffer_base<char_t>::resize(size_t size)
{
   if(holder)
      buffer = (char_t*) malloc(size * sizeof(char_t));
   else
      buffer = (char_t*) realloc(buffer, size * sizeof(char_t));

   bufsize = size;
   holder = false;

   return *this;
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(char_t *buffer, size_t bufsize)
{
   return (char_t*) realloc(buffer, bufsize * sizeof(char_t));
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(size_t bufsize)
{
   return (char_t*) malloc(bufsize * sizeof(char_t));
}

template <typename char_t>
void char_buffer_base<char_t>::free(char_t *buffer)
{
   ::free(buffer);
}

template <typename char_t>
size_t char_buffer_base<char_t>::memsize(size_t char_count)
{
   return char_count * sizeof(char_t);
}

//
// Instantiate character buffer for narrow characters
//
template class char_buffer_base<char>;
