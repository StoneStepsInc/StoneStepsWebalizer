/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tnode.cpp
*/
#include "pch.h"

#include "tnode.h"
#include "serialize.h"

//
// serialization
//
u_int tnode_t::s_data_size(void) const
{
   return base_node<tnode_t>::s_data_size() + sizeof(uint64_t);
}

u_int tnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr;

   basesize = base_node<tnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<tnode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, s_hash_value());

   return datasize;
}

u_int tnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   const void *ptr;
   u_int basesize, datasize;

   basesize = base_node<tnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   base_node<tnode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = s_skip_field<uint64_t>(ptr);      // value hash

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int tnode_t::s_data_size(const void *buffer)
{
   return base_node<tnode_t>::s_data_size(buffer) + sizeof(uint64_t);
}

const void *tnode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = s_data_size(buffer);
   return (u_char*) buffer + base_node<tnode_t>::s_data_size(buffer);
}
