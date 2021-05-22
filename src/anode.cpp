/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   anode.cpp
*/
#include "pch.h"

#include "anode.h"
#include "serialize.h"

anode_t::anode_t(void) : base_node<anode_t>() 
{
   count = 0;
   visits = 0; 
   xfer = 0;
   robot = false;
}

anode_t::anode_t(const string_t& agent, nodetype_t type, bool robot) :
      base_node<anode_t>(agent, type), robot(robot)
{
   count = 1;
   visits = 0; 
   xfer = 0;
}

//
// serialization
//

size_t anode_t::s_data_size(void) const
{
   return base_node<anode_t>::s_data_size() + sizeof(uint64_t) * 3 + sizeof(u_char) + sizeof(double);
}

size_t anode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<anode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, visits);

   ptr = sr.serialize(ptr, s_hash_value());

   ptr = sr.serialize(ptr, robot);
   
   ptr = sr.serialize(ptr, xfer);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t anode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t...> upcb, param_t ... param)
{
   bool tmp;

   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<anode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, visits);

   ptr = sr.s_skip_field<uint64_t>(ptr);     // value hash

   if(version >= 2) {
      ptr = sr.deserialize(ptr, tmp); robot = tmp;
   }
   else
      robot = false;

   if(version >= 3)
      ptr = sr.deserialize(ptr, xfer);
   else
      xfer = 0;

   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

const void *anode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer, bufsize) + sizeof(uint64_t) * 2;
}

const void *anode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer, bufsize);
}

const void *anode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer, bufsize) + sizeof(uint64_t);
}

int64_t anode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t anode_t::s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t anode_t::s_unpack_data(const void *buffer, size_t bufsize, anode_t::s_unpack_cb_t<> upcb);
template size_t anode_t::s_unpack_data(const void *buffer, size_t bufsize, anode_t::s_unpack_cb_t<void*> upcb, void *arg);
