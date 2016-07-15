/*
webalizer - a web server log analysis program

Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

See COPYING and Copyright files for additional licensing and copyright information

formatter.cpp
*/
#include "pch.h"

#include "formatter.h"
#include "tstring.h"
#include "util.h"

#include <cstddef>
#include <utility>

buffer_formatter_t::buffer_formatter_t(char *buffer, size_t bufsize, mode_t mode) : 
   buffer(buffer), 
   bufsize(bufsize), 
   cptr(buffer), 
   mode(mode)
{
}

void buffer_formatter_t::pop_scope(size_t scopeid)
{
   // make sure scopes are being destroyed in the proper order
   if(scopeid != scopes.size())
      throw exception_t(0, "Bad formatter scope is being restored");

   // restore the formatter state
   cptr = scopes.top().cptr;
   mode = scopes.top().mode;

   scopes.pop();
}

buffer_formatter_t::scope_t buffer_formatter_t::set_scope_mode(mode_t newmode)
{
   // push the current formatter state onto the stack
   scopes.emplace(cptr, mode);

   // set the new formatter mode
   set_mode(newmode);

   return scope_t(*this, scopes.size()); 
}

template <typename format_cb_t, typename ... format_param_t>
string_t::char_buffer_t buffer_formatter_t::format(format_cb_t&& formatcb, format_param_t&& ... arg)
{
   size_t avsize, olen;
   string_t::char_buffer_t outbuf;

   // check if cptr points one element past the end of the buffer
   if(cptr - buffer >= (std::ptrdiff_t) bufsize)
      return outbuf;

   // compute the remaining unused buffer size
   avsize = bufsize - (cptr-buffer);

   // create a fixed-size holder buffer for avsize characters
   string_t::char_buffer_t inbuf(cptr, avsize, true);

   // format the string in the available buffer space
   olen = formatcb(inbuf, arg ...);

   // initialize in the output buffer as a holder buffer pointing to the formatted string
   outbuf.attach(cptr, olen, true);

   // and move the current pointer past the last formatted character in the append mode
   if(mode == append)
      cptr += olen;

   return outbuf;
}

template <typename format_cb_t, typename ... format_param_t>
const char *buffer_formatter_t::operator () (format_cb_t&& formatcb, format_param_t&& ... arg)
{
   //
   // format always returns a holder buffer, which is not affected by the buffer 
   // object's destructor, so we can just return the buffer pointer without 
   // detaching the underlying block of memory
   //
   return format(std::forward<format_cb_t>(formatcb), std::forward<format_param_t>(arg) ...).get_buffer();
}
