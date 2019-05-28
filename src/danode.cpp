/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    danode.cpp
*/
#include "pch.h"

#include "danode.h"
#include "serialize.h"

danode_t::danode_t(uint64_t _nodeid) : keynode_t<uint64_t>(_nodeid)
{
   hits = 0; 
   xfer = 0; 
   proctime = 0;
}

void danode_t::reset(uint64_t _nodeid)
{
   keynode_t<uint64_t>::reset(_nodeid);
   datanode_t<danode_t>::reset();

   hits = 0; 
   tstamp.reset(); 
   xfer = 0; 
   proctime = 0;
}

//
// serialization
//

size_t danode_t::s_data_size(void) const
{
   return datanode_t<danode_t>::s_data_size() + 
            sizeof(uint64_t) * 3 +       // hits, proctime, xfer
            serializer_t::s_size_of(tstamp); // tstamp
}

size_t danode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<danode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, hits);
   ptr = sr.serialize(ptr, tstamp);
   ptr = sr.serialize(ptr, proctime);
   ptr = sr.serialize(ptr, xfer);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t danode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<danode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, hits);

   if(version >= 2)
      ptr = sr.deserialize(ptr, tstamp);
   else {
      uint64_t tmp;
      ptr = sr.deserialize(ptr, tmp);
      tstamp.reset((time_t) tmp);
   }
      
   ptr = sr.deserialize(ptr, proctime);
   ptr = sr.deserialize(ptr, xfer);
   
   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
struct hnode_t;
template <> struct storable_t<hnode_t>;

struct dlnode_t;
template <> struct storable_t<dlnode_t>;

template size_t danode_t::s_unpack_data(const void *buffer, size_t bufsize, danode_t::s_unpack_cb_t<> upcb, void *arg);
template size_t danode_t::s_unpack_data(const void *buffer, size_t bufsize, danode_t::s_unpack_cb_t<storable_t<dlnode_t>&, storable_t<hnode_t>&> upcb, void *arg, storable_t<dlnode_t>& dlnode, storable_t<hnode_t>& hnode);
