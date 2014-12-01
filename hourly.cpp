/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hourly.cpp
*/
#include "pch.h"

#include "hourly.h"
#include "serialize.h"

hourly_t::hourly_t(u_int hour) : keynode_t<uint32_t>(hour)
{
	th_hits = th_files = th_pages = 0;
	th_xfer = 0;
}

void hourly_t::reset(u_int nodeid)
{
   keynode_t<u_int>::reset(nodeid);
   datanode_t<hourly_t>::reset();

	th_hits = th_files = th_pages = 0;
	th_xfer = 0;
}

//
// serialization
//

u_int hourly_t::s_data_size(void) const
{
   return datanode_t<hourly_t>::s_data_size() + 
            sizeof(uint64_t) * 3 + 
            sizeof(uint64_t);          // th_xfer
}

u_int hourly_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize;
   void *ptr;

   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<hourly_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + datanode_t<hourly_t>::s_data_size();

   ptr = serialize(ptr, th_hits);
   ptr = serialize(ptr, th_files);
   ptr = serialize(ptr, th_pages);
   ptr = serialize(ptr, th_xfer);

   return datasize;
}

u_int hourly_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool fixver = (intptr_t) arg == -1;
   u_short version;
   u_int datasize;
   const void *ptr;

   datasize = s_data_size(buffer, fixver);

   if(bufsize < datasize)
      return 0;

   // see the comment in state_t::upgrade_database
   if(fixver) {
      version = 1;
      ptr = (u_char*) buffer;
   }
   else {
      version = s_node_ver(buffer);
      ptr = (u_char*) buffer + datanode_t<hourly_t>::s_data_size(buffer);
   }

   ptr = deserialize(ptr, th_hits);
   ptr = deserialize(ptr, th_files);
   ptr = deserialize(ptr, th_pages);
   ptr = deserialize(ptr, th_xfer);
   
   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int hourly_t::s_data_size(const void *buffer, bool fixver)
{
   return datanode_t<hourly_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3 + 
            sizeof(uint64_t);          // th_xfer
}
