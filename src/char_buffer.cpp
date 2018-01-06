/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "char_buffer.h"
#include <stdexcept>
#include <cstdlib>
#include <algorithm>

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
   buffer = new char_t[bufsize];
}

template <>
char_buffer_base<const char>::char_buffer_base(size_t bufsize)
{
   throw std::runtime_error("Cannot allocate an uninitialized const character buffer");
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
char_buffer_base<char_t>& char_buffer_base<char_t>::resize(size_t size, size_t datasize)
{
   // make sure the size of the data within the buffer doesn't exceed buffer size
   if(datasize > bufsize)
      throw std::invalid_argument("Cannot copy more data than there is in the buffer");

   // allocate a new buffer, so we can copy data if needed
   char_t *newbuf = alloc(size);

   if(buffer) {
      // copy as much data from the current buffer as the new one can take
      if(size && datasize)
         memcpy(newbuf, buffer, std::min(size, datasize) * sizeof(char_t));

      // release the current buffer if we own the memory
      if(!holder)
         free(buffer);
   }

   // finish up and assign all data members
   buffer = newbuf;
   bufsize = size;
   holder = false;

   return *this;
}

template <>
char_buffer_base<const char>& char_buffer_base<const char>::resize(size_t size, size_t datasize)
{
   // see char_buffer_base<const char>::alloc
   throw std::runtime_error("Cannot resize a const character buffer");
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(char_t *buffer, size_t bufsize, size_t datasize)
{
   // allocate a new buffer, so we can copy data if needed
   char_t *newbuf = alloc(bufsize);

   if(buffer) {
      // copy as much data from the current buffer as the new one can take
      if(bufsize && datasize)
         memcpy(newbuf, buffer, std::min(bufsize, datasize) * sizeof(char_t));

      // release the current buffer
      free(buffer);
   }

   return newbuf;
}

template <>
const char *char_buffer_base<const char>::alloc(const char *buffer, size_t bufsize, size_t datasize)
{
   //
   // We cannot use realloc with a const character buffer because it, effectively, 
   // copies chracters into a block of memory that is supposed to contain const
   // characters.
   //
   throw std::runtime_error("Cannot reallocate a const character buffer");
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(size_t bufsize)
{
   return new char_t[bufsize];
}

template <>
const char *char_buffer_base<const char>::alloc(size_t bufsize)
{
   throw std::runtime_error("Cannot allocate an uninitialized const character buffer");
}

template <typename char_t>
void char_buffer_base<char_t>::free(char_t *buffer)
{
   delete [] buffer;
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
template class char_buffer_base<const char>;
template class char_buffer_base<unsigned char>;
