/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    vnode.cpp
*/
#include "pch.h"

#include "vnode.h"
#include "unode.h"
#include "serialize.h"

vnode_t::vnode_t(u_long nodeid) : keynode_t<u_long>(nodeid)
{
   entry_url = false;
   robot = false;
   converted = false;
   start = end = 0;
   hits = files = pages = 0; 
   xfer = .0;
   lasturl = NULL;
   hostref = 0;

   dirty = true;
}

vnode_t::vnode_t(const vnode_t& vnode) : keynode_t<u_long>(vnode)
{
   entry_url = vnode.entry_url;
   robot = vnode.robot;
   converted = vnode.converted;
   start = vnode.start;
   end = vnode.end;
   hits = vnode.hits;
   files = vnode.files;
   pages = vnode.pages; 
   xfer = vnode.xfer;

   if((lasturl = vnode.lasturl) != NULL)
      lasturl->vstref++;

   dirty = true;

   //
   // vnode.hostref should not be copied, since there are no hosts
   // referring to the new node. The caller must adjust hostref, if
   // necessary.
   //
   hostref = 0;
}

vnode_t::~vnode_t(void)
{
   set_lasturl(NULL);
}

void vnode_t::reset(u_long nodeid)
{
   keynode_t<u_long>::reset(nodeid);
   datanode_t<vnode_t>::reset();

   entry_url = false;
   robot = false;
   converted = false;
   start = end = 0;
   hits = files = pages = 0; 
   xfer = .0;
   set_lasturl(NULL);

   dirty = true;
}

void vnode_t::set_lasturl(unode_t *unode)
{
   if(lasturl == unode)
      return;

   if(lasturl)
      lasturl->vstref--;

   if((lasturl = unode) != NULL)
      lasturl->vstref++;
}

//
// serialization
//

u_int vnode_t::s_data_size(void) const
{
   return datanode_t<vnode_t>::s_data_size() + sizeof(u_char) * 3 + sizeof(u_long) * 5 + sizeof(double) + sizeof(u_long);
}

u_int vnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<vnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<vnode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, entry_url);
   ptr = serialize(ptr, start);
   ptr = serialize(ptr, end);
   ptr = serialize(ptr, hits);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, pages);
   ptr = serialize(ptr, xfer);

   if(lasturl)
      ptr = serialize(ptr, lasturl->nodeid);
   else
      ptr = serialize(ptr, (u_long) 0);

   ptr = serialize(ptr, robot);
   ptr = serialize(ptr, converted);

   return datasize;
}

u_int vnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool tmp;
   u_long urlid;
   u_int datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = datanode_t<vnode_t>::s_data_size();
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   datanode_t<vnode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, tmp); entry_url = tmp;
   ptr = deserialize(ptr, start);
   ptr = deserialize(ptr, end);
   ptr = deserialize(ptr, hits);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, pages);
   ptr = deserialize(ptr, xfer);
   ptr = deserialize(ptr, urlid);

   if(version >= 2)
      ptr = deserialize(ptr, tmp), robot = tmp;
   else
      robot = false;

   if(version >= 3)
      ptr = deserialize(ptr, tmp), converted = tmp;
   else
      converted = false;
   
   if(upcb)
      upcb(*this, urlid, arg);

   return datasize;
}

u_int vnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_int datasize = datanode_t<vnode_t>::s_data_size(buffer) + sizeof(u_char) + sizeof(u_long) * 5 + sizeof(double) + sizeof(u_long);

   if(version < 2)
      return datasize;

   datasize += sizeof(u_char);         // robot
   
   if(version < 3)
      return datasize;
      
   return datasize + sizeof(u_char);   // converted
}

