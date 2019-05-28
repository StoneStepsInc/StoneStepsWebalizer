/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

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
            serializer_t::s_size_of(tstamp) +   // tstamp 
            sizeof(double) * 2 +    // avgtime, maxtime
            sizeof(uint64_t);       // xfer
}

size_t inode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<inode_t>::s_pack_data(buffer, bufsize);
   void *ptr = &((u_char*)buffer)[basesize];

   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, visit);
   ptr = sr.serialize(ptr, tstamp);
   ptr = sr.serialize(ptr, xfer);

   ptr = sr.serialize(ptr, s_hash_value());

   ptr = sr.serialize(ptr, avgtime);
   ptr = sr.serialize(ptr, maxtime);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t inode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<inode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = &((u_char*)buffer)[basesize];

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, visit);

   if(version >= 3)
      ptr = sr.deserialize(ptr, tstamp);
   else {
      uint64_t tmp;
      ptr = sr.deserialize(ptr, tmp);
      tstamp.reset((time_t) tmp);
   }

   ptr = sr.deserialize(ptr, xfer);

   ptr = sr.s_skip_field<uint64_t>(ptr);      // value hash

   if(version >= 2) {
      ptr = sr.deserialize(ptr, avgtime);
      ptr = sr.deserialize(ptr, maxtime);
   }
   else {
      avgtime = .0;
      maxtime = .0;
   }

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

const void *inode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);

   u_short version = s_node_ver(buffer);
   size_t offset = base_node<inode_t>::s_data_size(buffer, bufsize) + 
            sizeof(uint64_t) * 3;     // count, files, visit

   serializer_t sr(buffer, bufsize);

   if(version >= 3)
      offset += sr.s_size_of<tstamp_t>((u_char*) buffer + offset);  // tstamp
   else
      offset += sizeof(uint64_t);     // tstamp

   offset += sizeof(uint64_t);        // xfer

   return (u_char*) buffer + offset;
}

const void *inode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<inode_t>::s_data_size(buffer, bufsize);
}

int64_t inode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

//
// Instantiate all template callbacks
//
template size_t inode_t::s_unpack_data(const void *buffer, size_t bufsize, inode_t::s_unpack_cb_t<> upcb, void *arg);
