/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

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

anode_t::anode_t(const string_t& agent, bool robot) : base_node<anode_t>(agent), robot(robot)
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
   size_t datasize, basesize;
   void *ptr;

   basesize = base_node<anode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<anode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, count);
   ptr = serialize(ptr, visits);

   ptr = serialize(ptr, s_hash_value());

   ptr = serialize(ptr, robot);
   
   ptr = serialize(ptr, xfer);

   return datasize;
}

template <typename ... param_t>
size_t anode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   bool tmp;
   size_t datasize, basesize;
   const void *ptr;

   basesize = base_node<anode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   base_node<anode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, visits);

   ptr = s_skip_field<uint64_t>(ptr);      // value hash

   if(s_node_ver(buffer) >= 2) {
      ptr = deserialize(ptr, tmp); robot = tmp;
   }
   else
      robot = false;

   if(s_node_ver(buffer) >= 3)
      ptr = deserialize(ptr, xfer);
   else
      xfer = 0;

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return datasize;
}

size_t anode_t::s_data_size(const void *buffer)
{
   size_t datasize = base_node<anode_t>::s_data_size(buffer) + sizeof(uint64_t) * 3;

   if(s_node_ver(buffer) < 2)
      return datasize;

   datasize += sizeof(u_char);         // robot
   
   if(s_node_ver(buffer) < 3)
      return datasize;
      
   return datasize + sizeof(uint64_t);   // xfer
}

const void *anode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer) + sizeof(uint64_t) * 2;
}

const void *anode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer);
}

const void *anode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer) + sizeof(uint64_t);
}

int64_t anode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

int64_t anode_t::s_compare_visits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

//
// Instantiate all template callbacks
//
template size_t anode_t::s_unpack_data(const void *buffer, size_t bufsize, anode_t::s_unpack_cb_t<> upcb, void *arg);
