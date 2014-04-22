/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    dlnode.cpp
*/
#include "pch.h"

#include "dlnode.h"
#include "hnode.h"
#include "util.h"
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
   sumxfer = avgxfer = 0;
   avgtime = sumtime = 0;

   download = NULL;
   hnode = NULL;

   ownhost = false;
}

dlnode_t::dlnode_t(const char *name, hnode_t *nptr) : base_node<dlnode_t>(name) 
{
   count = 0;
   sumhits = 0;
   sumxfer = avgxfer = 0;
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

void dlnode_t::reset(u_long nodeid)
{
   base_node<dlnode_t>::reset(nodeid);

   count = 0;
   sumhits = 0;
   sumxfer = avgxfer = 0;
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

//
// serialization
//

u_int dlnode_t::s_data_size(void) const
{
   return base_node<dlnode_t>::s_data_size() + 
            sizeof(u_long) * 2 +
            sizeof(double) * 4 +
            sizeof(u_long)     +
            sizeof(u_char)     +
            sizeof(u_long);         // host name
}

u_int dlnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int basesize, datasize;
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

u_int dlnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_long hostid;
   bool active;
   u_int basesize, datasize;
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
   ptr = s_skip_field<u_long>(ptr);      // value hash
   ptr = deserialize(ptr, active);

   ptr = deserialize(ptr, hostid);

   download = NULL;

   if(upcb)
      upcb(*this, hostid, active, arg);

   return datasize;
}

u_long dlnode_t::s_hash_value(void) const
{
   return hash_num(base_node<dlnode_t>::s_hash_value(), hnode ? hnode->nodeid : 0);
}

int dlnode_t::s_compare_value(const void *buffer, u_int bufsize) const
{
   u_long hostid;
   u_int datasize;
   int diff;

   if(bufsize < s_data_size(buffer))
      throw exception_t(0, string_t::_format("Record size is smaller than expected (node: %s; size: %d; expected: %d)", typeid(*this).name(), bufsize, s_data_size()));

   if(!hnode)
      return -1;

   if((diff = base_node<dlnode_t>::s_compare_value(buffer, bufsize)) != 0)
      return diff;

   deserialize(s_field_value_mp_hostid(buffer, bufsize, datasize), hostid);

   return hnode->nodeid == hostid ? 0 : hnode->nodeid > hostid ? 1 : -1;
}

u_int dlnode_t::s_data_size(const void *buffer)
{
   u_int basesize = base_node<dlnode_t>::s_data_size(buffer);
   return basesize + 
            sizeof(u_long) * 2 + 
            sizeof(double) * 4 + 
            sizeof(u_long)     + 
            sizeof(u_char)     + 
            sizeof(u_long);            // host name
}

const void *dlnode_t::s_field_value_mp_dlname(const void *buffer, u_int bufsize, u_int& datasize)
{
   return base_node<dlnode_t>::s_field_value(buffer, bufsize, datasize);
}

const void *dlnode_t::s_field_value_mp_hostid(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*)buffer + base_node<dlnode_t>::s_data_size(buffer) + sizeof(u_long) * 2 + sizeof(double) * 4 + sizeof(u_long) + sizeof(u_char);
}

const void *dlnode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer) + sizeof(u_long) * 2 + sizeof(double) * 4;
}

const void *dlnode_t::s_field_xfer(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(double);
   return (u_char*) buffer + base_node<dlnode_t>::s_data_size(buffer) + sizeof(u_long) * 2;
}

int dlnode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<double>(buf1, buf2);
}

//
// dl_hash_table
//
bool dl_hash_table::compare(const dlnode_t *nptr, const void *param)  const
{
   param_block *pb = (param_block*) param;

   if(!nptr->hnode)
      return false;

   if(strcmp(nptr->hnode->string, pb->ipaddr)) 
      return false;

   return strcmp(nptr->string, pb->name) ? false : true;
}
