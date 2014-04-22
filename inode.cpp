/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    inode.cpp
*/
#include "pch.h"

#include "inode.h"
#include "serialize.h"

inode_t::inode_t(void) : base_node<inode_t>() 
{
   count = files = visit = 0; 
   tstamp = 0; 
   xfer = .0;
   avgtime = maxtime = .0;
}

inode_t::inode_t(const inode_t& inode) : base_node<inode_t>(inode)
{
   count = inode.count;
   files = inode.files;
   visit = inode.visit;
   tstamp = inode.tstamp;
   xfer = inode.xfer;
   avgtime = inode.avgtime;
   maxtime = inode.maxtime;
}

inode_t::inode_t(const char *ident) : base_node<inode_t>(ident) 
{
   count = 1; 
   visit = 1; 
   files = 0; 
   tstamp = 0; 
   xfer = .0;
   avgtime = maxtime = .0;
}

//
// serialization
//

u_int inode_t::s_data_size(void) const
{
   return base_node<inode_t>::s_data_size() + sizeof(u_long) * 5 + sizeof(double) * 3;
}

u_int inode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr;

   basesize = base_node<inode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<inode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, count);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, visit);
   ptr = serialize(ptr, tstamp);
   ptr = serialize(ptr, xfer);

   ptr = serialize(ptr, s_hash_value());

   ptr = serialize(ptr, avgtime);
   ptr = serialize(ptr, maxtime);

   return datasize;
}

u_int inode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_int datasize, basesize;
   const void *ptr;

   basesize = base_node<inode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   base_node<inode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, visit);
   ptr = deserialize(ptr, tstamp);
   ptr = deserialize(ptr, xfer);

   ptr = s_skip_field<u_long>(ptr);      // value hash

   if(s_node_ver(buffer) >= 2) {
      ptr = deserialize(ptr, avgtime);
      ptr = deserialize(ptr, maxtime);
   }
   else {
      avgtime = .0;
      maxtime = .0;
   }

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int inode_t::s_data_size(const void *buffer)
{
   u_int datasize = base_node<inode_t>::s_data_size(buffer) + sizeof(u_long) * 5 + sizeof(double);

   if(s_node_ver(buffer) < 2)
      return datasize;

   return datasize + sizeof(double) * 2;  // avgsize, maxsize
}

const void *inode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<inode_t>::s_data_size(buffer) + sizeof(u_long) * 4 + sizeof(double);
}

const void *inode_t::s_field_hits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<inode_t>::s_data_size(buffer);
}

int inode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<u_long>(buf1, buf2);
}
