/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<snode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, termcnt);
   ptr = sr.serialize(ptr, count);

   ptr = sr.serialize(ptr, s_hash_value());

   ptr = sr.serialize(ptr, visits);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t snode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<snode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, termcnt);
   ptr = sr.deserialize(ptr, count);

   ptr = sr.s_skip_field<uint64_t>(ptr);      // value hash

   if(version >= 2)
      ptr = sr.deserialize(ptr, visits);
   else
      visits = 0;

   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

const void *snode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<snode_t>::s_data_size(buffer, bufsize) + sizeof(u_short) + sizeof(uint64_t);
}

const void *snode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return &((u_char*)buffer)[base_node<snode_t>::s_data_size(buffer, bufsize)] + sizeof(u_short);
}

int64_t snode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t snode_t::s_unpack_data(const void *buffer, size_t bufsize, snode_t::s_unpack_cb_t<> upcb);
