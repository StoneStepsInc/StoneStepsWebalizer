/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

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
   return base_node<inode_t>::s_data_size() + 
            sizeof(u_long) * 3 +    // count, files, visit
            sizeof(u_long)     +    // hash(value)
            sizeof(int64_t)    +    // tstamp 
            sizeof(double) * 3;     // xfer, avgtime, maxtime
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
   ptr = serialize<int64_t>(ptr, tstamp);
   ptr = serialize(ptr, xfer);

   ptr = serialize(ptr, s_hash_value());

   ptr = serialize(ptr, avgtime);
   ptr = serialize(ptr, maxtime);

   return datasize;
}

u_int inode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   u_int datasize, basesize;
   const void *ptr;

   basesize = base_node<inode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   base_node<inode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   compatibility_deserializer<3, u_long, int64_t, time_t> deserialize_time_t(version);

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, visit);
   ptr = deserialize_time_t(ptr, tstamp);
   ptr = deserialize(ptr, xfer);

   ptr = s_skip_field<u_long>(ptr);      // value hash

   if(version >= 2) {
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
   u_short version = s_node_ver(buffer);
   u_int datasize = base_node<inode_t>::s_data_size(buffer) + 
            sizeof(u_long) * 3 +          // count, files, visit 
            sizeof(u_long)     +          // hash(value)
            sizeof(double);               // xfer

   // tstamp was switched from u_long to int64_t in version 3
   if(version < 2)
      return datasize + sizeof(u_long);   // tstamp

   datasize += sizeof(double) * 2;        // avgsize, maxsize 

   if(version < 3)
      return datasize + sizeof(u_long);   // tsatmp 

   datasize += sizeof(int64_t);           // tstamp

   return datasize;
}

const void *inode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   u_short version = s_node_ver(buffer);
   u_int offset = base_node<inode_t>::s_data_size(buffer) + 
            sizeof(u_long) * 3 +    // count, files, visit
            sizeof(double);         // xfer

   datasize = sizeof(u_long);

   if(version < 3)
      return (u_char*) buffer + offset +
            sizeof(u_long);         // tstamp

   return (u_char*) buffer + offset + 
            sizeof(int64_t);        // tstamp
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
