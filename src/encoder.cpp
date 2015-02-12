/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    encoder.cpp
*/
#include "pch.h"

#include "encoder.h"
#include "util.h"

template <encodecb_t encodecb>
buffer_encoder_t<encodecb>::buffer_encoder_t(char *buffer, size_t bufsize, mode_t mode) : 
      buffer(buffer), 
      bufsize(bufsize), 
      cptr(buffer), 
      mode(mode) 
{
}

template <encodecb_t encodecb>
buffer_encoder_t<encodecb>::buffer_encoder_t(const buffer_encoder_t& other, mode_t mode) : 
      buffer(other.cptr), 
      bufsize(other.bufsize - (other.cptr-other.buffer)), 
      cptr(buffer), 
      mode(mode) 
{
} 

template <encodecb_t encodecb>
char *buffer_encoder_t<encodecb>::encode(const char *str, bool multiline, size_t *olptr)
{
   char *out;
   size_t olen, avsize;

   // check if cptr points one element past the end of the buffer
   if(!(avsize = bufsize - (cptr-buffer)))
      return NULL;

   if((out = encodecb(str, cptr, avsize, multiline, &olen)) == NULL)
      return NULL;

   // move the current pointer to include the null character
   if(mode == append)
      cptr += olen + 1;

   if(olptr)
      *olptr = olen;

   return out;
}

//
// Instantiate templates
//
template class buffer_encoder_t<xml_encode>;
template class buffer_encoder_t<html_encode>;

