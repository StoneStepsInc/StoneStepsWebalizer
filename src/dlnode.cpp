/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

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

   ownhost = false;
}

dlnode_t::dlnode_t(const string_t& name, hnode_t *nptr) : base_node<dlnode_t>(name) 
{
   count = 0;
   sumhits = 0;
   sumxfer = 0;
   avgxfer = 0.;
   avgtime = sumtime = 0;

   download = NULL;

   if((hnode = nptr) != NULL)
      hnode->dlref++;

   ownhost = false;
}

dlnode_t::dlnode_t(const dlnode_t& tmp) : base_node<dlnode_t>(tmp)
{
   count = tmp.count;
   sumhits = tmp.sumhits;
   sumxfer = tmp.sumxfer;
   avgxfer = tmp.avgxfer;
   avgtime = tmp.avgtime;
   sumtime = tmp.sumtime;

   download = tmp.download;
   if((hnode = tmp.hnode) != NULL)
      hnode->dlref++;

   ownhost = false;
}

dlnode_t::~dlnode_t(void)
{
   set_host(NULL);
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

   if(hnode) {
      if(ownhost)
         delete hnode;
      else
         hnode->dlref--;
   }

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
   size_t basesize, datasize;
   void *ptr = buffer;

   basesize = base_node<dlnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<dlnode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, count);
   ptr = serialize(ptr, sumhits);
   ptr = serialize(ptr, sumxfer);
   ptr = serialize(ptr, avgxfer);
   ptr = serialize(ptr, avgtime);
   ptr = serialize(ptr, sumtime);
   ptr = serialize(ptr, s_hash_value());
   ptr = serialize(ptr, download ? true : false);
   ptr = serialize(ptr, hnode ? hnode->nodeid : 0);

   return datasize;
}

size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   uint64_t hostid;
   bool active;
   size_t basesize, datasize;
   const void *ptr = buffer;

   basesize = base_node<dlnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   base_node<dlnode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, sumhits);
   ptr = deserialize(ptr, sumxfer);
   ptr = deserialize(ptr, avgxfer);
   ptr = deserialize(ptr, avgtime);
   ptr = deserialize(ptr, sumtime);
   ptr = s_skip_field<uint64_t>(ptr);      // value hash
   ptr = deserialize(ptr, active);

   ptr = deserialize(ptr, hostid);

   download = NULL;

   if(upcb)
      upcb(*this, hostid, active, arg);

   return datasize;
}

uint64_t dlnode_t::s_hash_value(void) const
{
   return hash_num(base_node<dlnode_t>::s_hash_value(), hnode ? hnode->nodeid : 0);
}

int64_t dlnode_t::s_compare_value(const void *buffer, size_t bufsize) const
{
   uint64_t hostid;
   size_t datasize;
   int64_t diff;

   if(bufsize < s_data_size(buffer))
      throw exception_t(0, string_t::_format("Record size is smaller than expected (node: %s; size: %zu; expected: %zu)", typeid(*this).name(), bufsize, s_data_size()));

   if(!hnode)
      return -1;

   if((diff = base_node<dlnode_t>::s_compare_value(buffer, bufsize)) != 0)
      return diff;

   deserialize(s_field_value_mp_hostid(buffer, bufsize, datasize), hostid);

   return hnode->nodeid == hostid ? 0 : hnode->nodeid > hostid ? 1 : -1;
}

size_t dlnode_t::s_data_size(const void *buffer)
{
   size_t basesize = base_node<dlnode_t>::s_data_size(buffer);
   return basesize + 
            sizeof(uint64_t) * 2 + 
            sizeof(double) * 3 + 
            sizeof(uint64_t)  +          // sumxfer
            sizeof(uint64_t)     + 
            sizeof(u_char)     + 
            sizeof(uint64_t);            // host name
}

const void *dlnode_t::s_field_value_mp_dlname(const void *buffer, size_t bufsize, size_t& datasize)
{
   return base_node<dlnode_t>::s_field_value(buffer, bufsize, datasize);
}

const void *dlnode_t::s_field_value_mp_hostid(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<dlnode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 2 + 
            sizeof(double) * 3 + 
            sizeof(uint64_t) +         // sumxfer
            sizeof(uint64_t) + 
            sizeof(u_char);
}

const void *dlnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 2 + 
            sizeof(double) * 3 + 
            sizeof(uint64_t);          // sumxfer
}

const void *dlnode_t::s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer) + sizeof(uint64_t) * 2;
}

int64_t dlnode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}
