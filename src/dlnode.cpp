/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    dlnode.cpp
*/
#include "pch.h"

#include "dlnode.h"
#include "hnode.h"
#include "serialize.h"
#include "exception.h"

#include <typeinfo>

//
// Download Jobs
//

dlnode_t::dlnode_t(void) : base_node<dlnode_t>() 
{
   count = 0;
   sumhits = 0;
   sumxfer = 0;
   avgxfer = 0.;
   avgtime = sumtime = 0;

   download = NULL;
   hnode = NULL;
}

dlnode_t::dlnode_t(const string_t& name, hnode_t& nptr) : base_node<dlnode_t>(name) 
{
   count = 0;
   sumhits = 0;
   sumxfer = 0;
   avgxfer = 0.;
   avgtime = sumtime = 0;

   download = NULL;

   hnode = &nptr;
   hnode->dlref++;
}

dlnode_t::dlnode_t(dlnode_t&& tmp) : base_node<dlnode_t>(std::move(tmp))
{
   count = tmp.count;
   sumhits = tmp.sumhits;
   sumxfer = tmp.sumxfer;
   avgxfer = tmp.avgxfer;
   avgtime = tmp.avgtime;
   sumtime = tmp.sumtime;

   download = tmp.download;
   tmp.download = nullptr;

   hnode = tmp.hnode;
   tmp.hnode = nullptr;
}

dlnode_t::~dlnode_t(void)
{
   set_host(NULL);

   if(download)
      delete download;
}

void dlnode_t::reset(uint64_t nodeid)
{
   base_node<dlnode_t>::reset(nodeid);

   count = 0;
   sumhits = 0;
   sumxfer = 0;
   avgxfer = 0.;
   avgtime = sumtime = 0;

   download = NULL;

   set_host(NULL);
}

void dlnode_t::set_host(hnode_t *nptr)
{
   if(hnode == nptr)
      return;

   if(hnode)
      hnode->dlref--;

   if((hnode = nptr) != NULL)
      hnode->dlref++;
}

bool dlnode_t::match_key_ex(const dlnode_t::param_block *pb) const
{
   if(!hnode)
      return false;

   // compare IP addresses first
   if(strcmp(hnode->string, pb->ipaddr)) 
      return false;

   // and then download names
   return !strcmp(string, pb->name);
}

uint64_t dlnode_t::get_hash(void) const
{
   return hash_ex(hash_ex(0, hnode ? hnode->string : string_t()), string);
}

//
// serialization
//

size_t dlnode_t::s_data_size(void) const
{
   return base_node<dlnode_t>::s_data_size() + 
            sizeof(uint64_t) * 2 +
            sizeof(double) * 3 +
            sizeof(uint64_t) +        // sumxfer
            sizeof(uint64_t)     +
            sizeof(u_char)     +
            sizeof(uint64_t);         // host name
}

size_t dlnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<dlnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, sumhits);
   ptr = sr.serialize(ptr, sumxfer);
   ptr = sr.serialize(ptr, avgxfer);
   ptr = sr.serialize(ptr, avgtime);
   ptr = sr.serialize(ptr, sumtime);
   ptr = sr.serialize(ptr, s_hash_value());
   ptr = sr.serialize(ptr, download ? true : false);
   ptr = sr.serialize(ptr, hnode ? hnode->nodeid : 0);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   uint64_t hostid;
   bool active;

   size_t basesize = base_node<dlnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, sumhits);
   ptr = sr.deserialize(ptr, sumxfer);
   ptr = sr.deserialize(ptr, avgxfer);
   ptr = sr.deserialize(ptr, avgtime);
   ptr = sr.deserialize(ptr, sumtime);
   ptr = sr.s_skip_field<uint64_t>(ptr);      // value hash
   ptr = sr.deserialize(ptr, active);

   ptr = sr.deserialize(ptr, hostid);

   download = NULL;

   if(upcb)
      upcb(*this, hostid, active, arg, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

uint64_t dlnode_t::s_hash_value(void) const
{
   return hash_num(base_node<dlnode_t>::s_hash_value(), hnode ? hnode->nodeid : 0);
}

int64_t dlnode_t::s_compare_value(const void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   uint64_t hostid;
   size_t datasize;
   int64_t diff;

   if(!hnode)
      return -1;

   if((diff = base_node<dlnode_t>::s_compare_value(buffer, bufsize)) != 0)
      return diff;

   sr.deserialize(s_field_value_mp_hostid(buffer, bufsize, datasize), hostid);

   return hnode->nodeid == hostid ? 0 : hnode->nodeid > hostid ? 1 : -1;
}

const void *dlnode_t::s_field_value_mp_dlname(const void *buffer, size_t bufsize, size_t& datasize)
{
   return base_node<dlnode_t>::s_field_value(buffer, bufsize, datasize);
}

const void *dlnode_t::s_field_value_mp_hostid(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<dlnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(uint64_t) * 2 + 
            sizeof(double) * 3 + 
            sizeof(uint64_t) +         // sumxfer
            sizeof(uint64_t) + 
            sizeof(u_char);
}

const void *dlnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(uint64_t) * 2 + 
            sizeof(double) * 3 + 
            sizeof(uint64_t);          // sumxfer
}

const void *dlnode_t::s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer, bufsize) + sizeof(uint64_t) * 2;
}

int64_t dlnode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

//
// Instantiate all template callbacks
//
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<> upcb, void *arg);
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<const storable_t<hnode_t>&> upcb, void *arg, const storable_t<hnode_t>& hnode);
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<storable_t<hnode_t>&> upcb, void *arg, storable_t<hnode_t>& hnode);
