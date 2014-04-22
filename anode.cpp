/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

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
   xfer = .0;
   robot = false;
}

anode_t::anode_t(const string_t& agent, bool robot) : base_node<anode_t>(agent), robot(robot)
{
   count = 1;
   visits = 0; 
   xfer = .0;
}

anode_t::anode_t(const anode_t& anode) : base_node<anode_t>(anode)
{
   count = anode.count;
   visits = anode.visits;
   xfer = anode.xfer;
   robot = anode.robot;
}

//
// serialization
//

u_int anode_t::s_data_size(void) const
{
   return base_node<anode_t>::s_data_size() + sizeof(u_long) * 3 + sizeof(u_char) + sizeof(double);
}

u_int anode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
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

u_int anode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool tmp;
   u_int datasize, basesize;
   const void *ptr;

   basesize = base_node<anode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   base_node<anode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, visits);

   ptr = s_skip_field<u_long>(ptr);      // value hash

   if(s_node_ver(buffer) >= 2) {
      ptr = deserialize(ptr, tmp); robot = tmp;
   }
   else
      robot = false;

   if(s_node_ver(buffer) >= 3)
      ptr = deserialize(ptr, xfer);
   else
      xfer = .0;

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int anode_t::s_data_size(const void *buffer)
{
   u_long datasize = base_node<anode_t>::s_data_size(buffer) + sizeof(u_long) * 3;

   if(s_node_ver(buffer) < 2)
      return datasize;

   datasize += sizeof(u_char);         // robot
   
   if(s_node_ver(buffer) < 3)
      return datasize;
      
   return datasize + sizeof(double);   // xfer
}

const void *anode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer) + sizeof(u_long) * 2;
}

const void *anode_t::s_field_hits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer);
}

const void *anode_t::s_field_visits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<anode_t>::s_data_size(buffer) + sizeof(u_long);
}

int anode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<u_long>(buf1, buf2);
}

int anode_t::s_compare_visits(const void *buf1, const void *buf2)
{
   return s_compare<u_long>(buf1, buf2);
}
