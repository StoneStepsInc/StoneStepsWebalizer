/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    danode.cpp
*/
#include "pch.h"

#include "danode.h"
#include "serialize.h"


danode_t::danode_t(u_long _nodeid) : keynode_t<u_long>(_nodeid)
{
   hits = 0; 
   xfer = 0; 
   proctime = 0;

   dirty = true;
}

danode_t::danode_t(const danode_t& danode) : keynode_t<u_long>(danode)
{
   hits = danode.hits; 
   tstamp = danode.tstamp; 
   xfer = danode.xfer; 
   proctime = danode.proctime;

   dirty = true;
}

void danode_t::reset(u_long _nodeid)
{
   keynode_t<u_long>::reset(_nodeid);

   hits = 0; 
   tstamp.reset(); 
   xfer = 0; 
   proctime = 0;

   dirty = true;
}

//
// serialization
//

u_int danode_t::s_data_size(void) const
{
   return datanode_t<danode_t>::s_data_size() + 
            sizeof(u_long) * 3 +       // hits, proctime, xfer
            s_size_of(tstamp);         // tstamp
}

u_int danode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<danode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<danode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, hits);
   ptr = serialize(ptr, tstamp);
   ptr = serialize(ptr, proctime);
   ptr = serialize(ptr, xfer);

   return datasize;
}

u_int danode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   u_int datasize, basesize;
   const void *ptr;

   basesize = datanode_t<danode_t>::s_data_size();
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   datanode_t<danode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, hits);

   if(version >= 2)
      ptr = deserialize(ptr, tstamp);
   else {
      u_long tmp;
      ptr = deserialize(ptr, tmp);
      tstamp.reset((time_t) tmp);
   }
      
   ptr = deserialize(ptr, proctime);
   ptr = deserialize(ptr, xfer);
   
   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int danode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);

   size_t datasize = datanode_t<danode_t>::s_data_size(buffer) + 
            sizeof(u_long);               // hits

   if(version < 2)
      datasize += sizeof(u_long);         // tstamp
   else
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);   // tstamp

   return datasize + 
            sizeof(u_long) * 2;           // proctime, xfer;
}

