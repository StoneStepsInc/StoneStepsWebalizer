/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

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
   hexenc = rnode.hexenc;
   count = rnode.count;
   visits = rnode.visits;
}

rnode_t::rnode_t(const char *ref) : base_node<rnode_t>(ref) 
{
   count = 0;
   visits = 0;
   hexenc = (ref && strchr(ref, '%')) ? true : false;
}

//
// serialization
//

u_int rnode_t::s_data_size(void) const
{
   return base_node<rnode_t>::s_data_size() + 
               sizeof(u_char) +                 // hexenc
               sizeof(u_long) * 2 +             // value hash, count
               sizeof(u_long);                  // visits
}

u_int rnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr;

   basesize = base_node<rnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<rnode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, hexenc);
   ptr = serialize(ptr, count);

   ptr = serialize(ptr, s_hash_value());
   
   ptr = serialize(ptr, visits);

   return datasize;
}

u_int rnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_int datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = base_node<rnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);

   base_node<rnode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, hexenc);
   ptr = deserialize(ptr, count);

   ptr = s_skip_field<u_long>(ptr);      // value hash
   
   if(version >= 2)
      ptr = deserialize(ptr, visits);
   else
      visits = 0;

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int rnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_int datasize = base_node<rnode_t>::s_data_size(buffer) + sizeof(u_char) + sizeof(u_long) * 2;
   
   if(version < 2)
      return datasize;
      
   return datasize + sizeof(u_long);   // visits
}

const void *rnode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<rnode_t>::s_data_size(buffer) + sizeof(u_char) + sizeof(u_long);
}

const void *rnode_t::s_field_hits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return &((u_char*)buffer)[base_node<rnode_t>::s_data_size(buffer)];
}

int rnode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return *(u_long*) buf1 - *(u_long*) buf2;
}
