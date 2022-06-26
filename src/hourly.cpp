/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

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

void hourly_t::reset(u_int hour)
{
   keynode_t<u_int>::reset(hour);
   datanode_t<hourly_t>::reset();

   th_hits = th_files = th_pages = 0;
   th_xfer = 0;
}

//
// serialization
//

size_t hourly_t::s_data_size(void) const
{
   return datanode_t<hourly_t>::s_data_size() + 
            sizeof(uint64_t) * 3 + 
            sizeof(uint64_t);          // th_xfer
}

size_t hourly_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<hourly_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, th_hits);
   ptr = sr.serialize(ptr, th_files);
   ptr = sr.serialize(ptr, th_pages);
   ptr = sr.serialize(ptr, th_xfer);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t hourly_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<hourly_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;   

   ptr = sr.deserialize(ptr, th_hits);
   ptr = sr.deserialize(ptr, th_files);
   ptr = sr.deserialize(ptr, th_pages);
   ptr = sr.deserialize(ptr, th_xfer);
   
   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
template size_t hourly_t::s_unpack_data(const void *buffer, size_t bufsize, hourly_t::s_unpack_cb_t<> upcb);
