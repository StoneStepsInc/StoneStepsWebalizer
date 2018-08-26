/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    snode.cpp
*/
#include "pch.h"

#include "snode.h"
#include "serialize.h"

//
// serialization
//

size_t snode_t::s_data_size(void) const
{
   return base_node<snode_t>::s_data_size() + sizeof(u_short) + sizeof(uint64_t) * 3;
}

size_t snode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
   void *ptr;

   basesize = base_node<snode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<snode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, termcnt);
   ptr = serialize(ptr, count);

   ptr = serialize(ptr, s_hash_value());

   ptr = serialize(ptr, visits);

   return datasize;
}

template <typename ... param_t>
size_t snode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   size_t datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = base_node<snode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);

   base_node<snode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, termcnt);
   ptr = deserialize(ptr, count);

   ptr = s_skip_field<uint64_t>(ptr);      // value hash

   if(version >= 2)
      ptr = deserialize(ptr, visits);
   else
      visits = 0;

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return datasize;
}

size_t snode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = base_node<snode_t>::s_data_size(buffer) + sizeof(u_short) + sizeof(uint64_t) * 2;
   
   if(version < 2)
      return datasize;
      
   return datasize + sizeof(uint64_t);   // visits
}

const void *snode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<snode_t>::s_data_size(buffer) + sizeof(u_short) + sizeof(uint64_t);
}

const void *snode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return &((u_char*)buffer)[base_node<snode_t>::s_data_size(buffer)] + sizeof(u_short);
}

int64_t snode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

//
// Instantiate all template callbacks
//
template size_t snode_t::s_unpack_data(const void *buffer, size_t bufsize, snode_t::s_unpack_cb_t<> upcb, void *arg);
