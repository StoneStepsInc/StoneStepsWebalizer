/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    vnode.cpp
*/
#include "pch.h"

#include "vnode.h"
#include "unode.h"
#include "serialize.h"

vnode_t::vnode_t(uint64_t nodeid) : keynode_t<uint64_t>(nodeid),
   next(NULL)
{
   entry_url = false;
   robot = false;
   converted = false;
   hits = files = pages = 0; 
   xfer = 0;
   lasturl = NULL;
   hostref = 0;
}

vnode_t::vnode_t(vnode_t&& vnode) : keynode_t<uint64_t>(std::move(vnode)),
   next(vnode.next)
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

   vnode.next = nullptr;

   lasturl = vnode.lasturl;
   vnode.lasturl = nullptr;

   hostref = vnode.hostref;
   vnode.hostref = 0;
}

vnode_t::~vnode_t(void)
{
   set_lasturl(NULL);
}

void vnode_t::reset(uint64_t nodeid)
{
   keynode_t<uint64_t>::reset(nodeid);
   datanode_t<vnode_t>::reset();

   entry_url = false;
   robot = false;
   converted = false;
   start.reset();
   end.reset();
   hits = files = pages = 0; 
   xfer = 0;
   set_lasturl(NULL);
}

void vnode_t::set_lasturl(storable_t<unode_t> *unode)
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

size_t vnode_t::s_data_size(void) const
{
   return datanode_t<vnode_t>::s_data_size() + 
            sizeof(u_char)  * 3 +   // entry_url, robot, converted
            sizeof(uint64_t) * 3 +  // hits, files, pages
            s_size_of(start)    +   // start
            s_size_of(end)      +   // end
            sizeof(uint64_t)    +   // xfer 
            sizeof(uint64_t);       // lasturl->nodeid
}

size_t vnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
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
      ptr = serialize(ptr, (uint64_t) 0);

   ptr = serialize(ptr, robot);
   ptr = serialize(ptr, converted);

   return datasize;
}

template <typename ... param_t>
size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   bool tmp;
   uint64_t urlid;
   size_t datasize, basesize;
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

   if(version >= 4) {
      ptr = deserialize(ptr, start);
      ptr = deserialize(ptr, end);
   }
   else {
      uint64_t tmp;

      ptr = deserialize(ptr, tmp); 
      start.reset((time_t) tmp);

      ptr = deserialize(ptr, tmp);
      end.reset((time_t) tmp);
   }

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
      upcb(*this, urlid, arg, std::forward<param_t>(param) ...);

   return datasize;
}

size_t vnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = datanode_t<vnode_t>::s_data_size(buffer) + 
            sizeof(u_char);         // entry_url

   if(version < 4)
      datasize += sizeof(uint64_t) * 2;  // start, end
   else {
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // start
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // end
   }
    
   datasize +=
            sizeof(uint64_t) * 3 +    // hits, files, pages
            sizeof(uint64_t)     +    // xfer
            sizeof(uint64_t);         // urlid (last URL)

   if(version < 2)
      return datasize;

   datasize += sizeof(u_char);      // robot
   
   if(version < 3)
      return datasize;
   
   datasize += sizeof(u_char);      // converted   

   return datasize;
}

//
// Instantiate all template callbacks
//
struct hnode_t;
template <> struct storable_t<hnode_t>;

template size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, vnode_t::s_unpack_cb_t<storable_t<unode_t>&> upcb, void *arg, storable_t<unode_t>& unode);
template size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, vnode_t::s_unpack_cb_t<storable_t<hnode_t>&, storable_t<unode_t>&> upcb, void *arg, storable_t<hnode_t>& hnode, storable_t<unode_t>& unode);
