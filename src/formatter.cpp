/*
webalizer - a web server log analysis program

Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

See COPYING and Copyright files for additional licensing and copyright information

formatter.cpp
*/
#include "pch.h"

#include "formatter.h"
#include "tstring.h"

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
