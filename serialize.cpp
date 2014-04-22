/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   serialize.cpp
*/
#include "pch.h"

#include "serialize.h"

void *serialize(void *ptr, const string_t& value)
{
   void *cp = serialize(ptr, value.length());
   if(value.length())
      memcpy(cp, value.c_str(), value.length());
   return (u_char*) ptr + sizeof(value.length()) + value.length();
}

const void *deserialize(const void *ptr, string_t& value)
{
   u_int slen;
   const void *cp;
   char *sp;

   cp = deserialize(ptr, slen);

   if(slen) {
      sp = new char[slen+1];
      memcpy(sp, cp, slen);
      sp[slen] = 0;
      value.attach(sp, slen);
   }
   else
      value.reset();

   return &((u_char*)ptr)[sizeof(u_int)+slen];
}

