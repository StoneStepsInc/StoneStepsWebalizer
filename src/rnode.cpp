/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rnode.cpp
*/
#include "pch.h"

#include "rnode.h"
#include "serialize.h"

//
//
//
rnode_t::rnode_t(const rnode_t& rnode) : base_node<rnode_t>(rnode)
{
   count = rnode.count;
   visits = rnode.visits;
}

rnode_t::rnode_t(const string_t& ref) : base_node<rnode_t>(ref) 
{
   count = 0;
   visits = 0;
}

//
// serialization
//

size_t rnode_t::s_data_size(void) const
{
   return base_node<rnode_t>::s_data_size() + 
               sizeof(u_char) +                 // hexenc
               sizeof(uint64_t) * 2 +             // value hash, count
               sizeof(uint64_t);                  // visits
}

size_t rnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
   void *ptr;

   basesize = base_node<rnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<rnode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, false);
   ptr = serialize(ptr, count);

   ptr = serialize(ptr, s_hash_value());
   
   ptr = serialize(ptr, visits);

   return datasize;
}

size_t rnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   size_t datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = base_node<rnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);

   base_node<rnode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = s_skip_field<bool>(ptr);            // hexenc

   ptr = deserialize(ptr, count);

   ptr = s_skip_field<uint64_t>(ptr);      // value hash
   
   if(version >= 2)
      ptr = deserialize(ptr, visits);
   else
      visits = 0;

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

size_t rnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = base_node<rnode_t>::s_data_size(buffer) + 
         sizeof(u_char) +                 // hexenc
         sizeof(uint64_t) * 2;            // count, value hash

   if(version < 2)
      return datasize;

   return datasize + sizeof(uint64_t);   // visits
}

const void *rnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<rnode_t>::s_data_size(buffer) + sizeof(u_char) + sizeof(uint64_t);
}

const void *rnode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return &((u_char*)buffer)[base_node<rnode_t>::s_data_size(buffer)];
}

int64_t rnode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}
