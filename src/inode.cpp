/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    inode.cpp
*/
#include "pch.h"

#include "inode.h"
#include "serialize.h"

inode_t::inode_t(void) : base_node<inode_t>() 
{
   count = files = visit = 0; 
   xfer = 0;
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

inode_t::inode_t(const string_t& ident) : base_node<inode_t>(ident) 
{
   count = 1; 
   visit = 1; 
   files = 0; 
   xfer = 0;
   avgtime = maxtime = .0;
}

//
// serialization
//

size_t inode_t::s_data_size(void) const
{
   return base_node<inode_t>::s_data_size() + 
            sizeof(uint64_t) * 3 +    // count, files, visit
            sizeof(uint64_t)     +    // hash(value)
            s_size_of(tstamp)  +    // tstamp 
            sizeof(double) * 2 +    // avgtime, maxtime
            sizeof(uint64_t);       // xfer
}

size_t inode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
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

size_t inode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   size_t datasize, basesize;
   const void *ptr;

   basesize = base_node<inode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   base_node<inode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, visit);

   if(version >= 3)
      ptr = deserialize(ptr, tstamp);
   else {
      uint64_t tmp;
      ptr = deserialize(ptr, tmp);
      tstamp.reset((time_t) tmp);
   }

   ptr = deserialize(ptr, xfer);

   ptr = s_skip_field<uint64_t>(ptr);      // value hash

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

size_t inode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = base_node<inode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3;           // count, files, visit 

   if(version < 3)
      datasize += sizeof(uint64_t);         // tstamp
   else
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // tstamp

   datasize += sizeof(uint64_t)  +          // hash(value)
            sizeof(uint64_t);               // xfer

   if(version < 2)
      return datasize;

   return datasize + 
            sizeof(double) * 2;           // avgsize, maxsize
}

const void *inode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   u_short version = s_node_ver(buffer);
   size_t offset = base_node<inode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3;     // count, files, visit

   if(version >= 3)
      offset += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // tstamp
   else
      offset += sizeof(uint64_t);     // tstamp

   offset += sizeof(uint64_t);        // xfer

   return (u_char*) buffer + offset;
}

const void *inode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<inode_t>::s_data_size(buffer);
}

int64_t inode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}
