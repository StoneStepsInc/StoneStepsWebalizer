/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    vnode.cpp
*/
#include "pch.h"

#include "vnode.h"
#include "unode.h"
#include "serialize.h"

vnode_t::vnode_t(uint64_t nodeid) : keynode_t<uint64_t>(nodeid),
   next(nullptr)
{
   entry_url = false;
   robot = false;
   converted = false;
   hits = files = pages = 0; 
   xfer = 0;
   lasturl = nullptr;
   hostref = 0;
}

vnode_t::vnode_t(vnode_t&& vnode) noexcept :
   keynode_t<uint64_t>(std::move(vnode)),
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
   set_lasturl(nullptr);
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
   set_lasturl(nullptr);
}

void vnode_t::set_lasturl(storable_t<unode_t> *unode)
{
   if(lasturl == unode)
      return;

   if(lasturl)
      lasturl->vstref--;

   if((lasturl = unode) != nullptr)
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
            serializer_t::s_size_of(start) +    // start
            serializer_t::s_size_of(end) +      // end
            sizeof(uint64_t)    +   // xfer 
            sizeof(uint64_t);       // lasturl->nodeid
}

size_t vnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<vnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, entry_url);
   ptr = sr.serialize(ptr, start);
   ptr = sr.serialize(ptr, end);
   ptr = sr.serialize(ptr, hits);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, pages);
   ptr = sr.serialize(ptr, xfer);

   if(lasturl)
      ptr = sr.serialize(ptr, lasturl->nodeid);
   else
      ptr = sr.serialize(ptr, (uint64_t) 0);

   ptr = sr.serialize(ptr, robot);
   ptr = sr.serialize(ptr, converted);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   bool tmp;
   uint64_t urlid;

   size_t basesize = datanode_t<vnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, tmp); entry_url = tmp;

   if(version >= 4) {
      ptr = sr.deserialize(ptr, start);
      ptr = sr.deserialize(ptr, end);
   }
   else {
      uint64_t tmp;

      ptr = sr.deserialize(ptr, tmp); 
      start.reset((time_t) tmp);

      ptr = sr.deserialize(ptr, tmp);
      end.reset((time_t) tmp);
   }

   ptr = sr.deserialize(ptr, hits);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, pages);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, urlid);

   if(version >= 2)
      ptr = sr.deserialize(ptr, tmp), robot = tmp;
   else
      robot = false;

   if(version >= 3)
      ptr = sr.deserialize(ptr, tmp), converted = tmp;
   else
      converted = false;
   
   if(upcb)
      upcb(*this, urlid, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
struct hnode_t;
template <> struct storable_t<hnode_t>;

template size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, vnode_t::s_unpack_cb_t<storable_t<unode_t>&> upcb, storable_t<unode_t>& unode);
template size_t vnode_t::s_unpack_data(const void *buffer, size_t bufsize, vnode_t::s_unpack_cb_t<void*, storable_t<hnode_t>&, storable_t<unode_t>&> upcb, void *arg, storable_t<hnode_t>& hnode, storable_t<unode_t>& unode);
