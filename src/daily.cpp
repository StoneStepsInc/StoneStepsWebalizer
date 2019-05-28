/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    daily.cpp
*/
#include "pch.h"

#include "daily.h"
#include "serialize.h"

daily_t::daily_t(u_int day) : keynode_t<uint32_t>(day)
{
   td_hours = 0;
   tm_hits = tm_files = tm_hosts = tm_pages = tm_visits = 0;
   h_hits_max = h_files_max = h_pages_max = h_visits_max = h_hosts_max = 0;
   h_xfer_max = 0;
   h_xfer_avg = .0;
   tm_xfer = 0;
   h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_hosts_avg = .0;
}

void daily_t::reset(u_int day)
{
   keynode_t<u_int>::reset(day);
   datanode_t<daily_t>::reset();

   td_hours = 0;
   tm_hits = tm_files = tm_hosts = tm_pages = tm_visits = 0;
   h_hits_max = h_files_max = h_pages_max = h_visits_max = h_hosts_max = 0;
   h_xfer_max = 0;
   h_xfer_avg = .0;
   tm_xfer = 0;
   h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_hosts_avg = .0;
}

//
// serialization
//

size_t daily_t::s_data_size(void) const
{
   return datanode_t<daily_t>::s_data_size() + 
            sizeof(uint64_t) * 10   + 
            sizeof(double)   * 6    +        // h_hits_avg, h_files_avg, h_pages_avg, h_visits_avg, h_hosts_avg, hxfer_avg
            sizeof(uint64_t)        +        // h_xfer_max
            sizeof(uint64_t)        +        // tm_xfer  
            sizeof(u_short);
}

size_t daily_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<daily_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;
      
   ptr = sr.serialize(ptr, tm_hits);
   ptr = sr.serialize(ptr, tm_files);
   ptr = sr.serialize(ptr, tm_pages);
   ptr = sr.serialize(ptr, tm_hosts);
   ptr = sr.serialize(ptr, tm_visits);
   ptr = sr.serialize(ptr, tm_xfer);
   
   ptr = sr.serialize(ptr, h_hits_max);
   ptr = sr.serialize(ptr, h_files_max);
   ptr = sr.serialize(ptr, h_pages_max);
   ptr = sr.serialize(ptr, h_xfer_max);
   ptr = sr.serialize(ptr, h_visits_max);
   ptr = sr.serialize(ptr, h_hosts_max);
   
   ptr = sr.serialize(ptr, h_hits_avg);
   ptr = sr.serialize(ptr, h_files_avg);
   ptr = sr.serialize(ptr, h_pages_avg);
   ptr = sr.serialize(ptr, h_xfer_avg);
   ptr = sr.serialize(ptr, h_visits_avg);
   ptr = sr.serialize(ptr, h_hosts_avg);
   
   ptr = sr.serialize(ptr, td_hours);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t daily_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<daily_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, tm_hits);
   ptr = sr.deserialize(ptr, tm_files);
   ptr = sr.deserialize(ptr, tm_pages);
   ptr = sr.deserialize(ptr, tm_hosts);
   ptr = sr.deserialize(ptr, tm_visits);
   ptr = sr.deserialize(ptr, tm_xfer);

   if(version >= 2) {
      ptr = sr.deserialize(ptr, h_hits_max);
      ptr = sr.deserialize(ptr, h_files_max);
      ptr = sr.deserialize(ptr, h_pages_max);
      ptr = sr.deserialize(ptr, h_xfer_max);
      ptr = sr.deserialize(ptr, h_visits_max);
      ptr = sr.deserialize(ptr, h_hosts_max);
      
      ptr = sr.deserialize(ptr, h_hits_avg);
      ptr = sr.deserialize(ptr, h_files_avg);
      ptr = sr.deserialize(ptr, h_pages_avg);
      ptr = sr.deserialize(ptr, h_xfer_avg);
      ptr = sr.deserialize(ptr, h_visits_avg);
      ptr = sr.deserialize(ptr, h_hosts_avg);

      ptr = sr.deserialize(ptr, td_hours);
   }
   else {
      h_hits_max = h_files_max = h_pages_max = h_visits_max = 0;
      h_hits_avg = h_files_avg = h_pages_avg = h_visits_avg = h_xfer_avg = .0;
      h_xfer_max = 0;
      td_hours = 0;
   }
   
   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);
   
   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
template size_t daily_t::s_unpack_data(const void *buffer, size_t bufsize, daily_t::s_unpack_cb_t<> upcb, void *arg);
