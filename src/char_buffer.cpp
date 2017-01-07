/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstring.cpp
*/
#include "pch.h"

#include "char_buffer.h"
#include "exception.h"
#include <stdlib.h>
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
   // copy as much data as fits in the new buffer if we don't know how much we have
   return resize(size, bufsize);
}

template <typename char_t>
char_buffer_base<char_t>& char_buffer_base<char_t>::resize(size_t size, size_t datasize)
{
   // make sure the size of the data within the buffer doesn't exceed buffer size
   if(datasize > bufsize)
      throw exception_t(0, "Cannot copy more data than there is in the buffer");

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
   throw exception_t(0, "Cannot resize a const character buffer");
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(char_t *buffer, size_t bufsize)
{
   char_t *newbuf = (char_t*) ::realloc(buffer, bufsize * sizeof(char_t));

   // if we failed to allocate memory, the old buffer should be freed by its owner
   if(!newbuf)
      throw std::bad_alloc();

   return newbuf;
}

template <>
const char *char_buffer_base<const char>::alloc(const char *buffer, size_t bufsize)
{
   //
   // We cannot use realloc with a const character buffer because it, effectively, 
   // copies chracters into a block of memory that is supposed to contain const
   // characters. It is possible to allocate a new block of memory, but in a static
   // member function, the size of the the underlying memory block is unknown, and
   // having the caller to maintain buffer size outside of this implementation would
   // be quite error prone.
   //
   throw exception_t(0, "Cannot resize a const character buffer");
}

template <typename char_t>
char_t *char_buffer_base<char_t>::alloc(size_t bufsize)
{
   char_t *newbuf = (char_t*) malloc(bufsize * sizeof(char_t));

   if(!newbuf)
      throw std::bad_alloc();

   return newbuf;
}

template <>
const char *char_buffer_base<const char>::alloc(size_t bufsize)
{
   // use operator new, so we can delete a const buffer later
   return new char[bufsize];
}

template <typename char_t>
void char_buffer_base<char_t>::free(char_t *buffer)
{
   ::free(buffer);
}

template <>
void char_buffer_base<const char>::free(const char *buffer)
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
