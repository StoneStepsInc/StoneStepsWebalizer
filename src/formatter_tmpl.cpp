/*
webalizer - a web server log analysis program

Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

See COPYING and Copyright files for additional licensing and copyright information

formatter_tmpl.cpp
*/
#include "pch.h"

#include "formatter.h"
#include "tstring.h"
#include "exception.h"

#include <cstddef>
#include <utility>

///
/// @brief  Formats output within the remaining buffer space using the formatting 
///         function passed as the template argument.
///
/// @tparam `format_cb_t`    A formatting function.
///
/// A formatting function or a function call operator of some object that is passed 
/// into this method is expected to have this signature:
/// ```
///    size_t formatcb(string_t::char_buffer_t&, format_param_t ...);
/// ```
/// The character buffer object passed into the formatting function will hold formatted 
/// text after the function returns. The function should return the number of characters 
/// in the new formatted text, including the null character, if there is one. The formatter 
/// does not interpret or modify the formatted string in any way. There is no error value 
/// reserved and the function should throw an exception in case of any error.
///
template <typename format_cb_t, typename ... format_param_t>
string_t::char_buffer_t buffer_formatter_t::format(format_cb_t&& formatcb, format_param_t&& ... arg)
{
   size_t avsize, olen;
   string_t::char_buffer_t outbuf;

   // check if cptr points one element past the end of the buffer
   if(cptr - buffer >= (std::ptrdiff_t) bufsize)
      throw exception_t(0, "Insufficient buffer capacity");

   // compute the remaining unused buffer size
   avsize = bufsize - (cptr-buffer);

   // create a fixed-size holder buffer for avsize characters
   string_t::char_buffer_t inbuf(cptr, avsize, true);

   // format the string in the available buffer space
   olen = formatcb(inbuf, arg ...);

   // initialize the output buffer as a holder buffer pointing to the formatted string
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
