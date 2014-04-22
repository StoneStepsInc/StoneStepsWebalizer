/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ccnode.cpp
*/
#include "pch.h"

#include "ccnode.h"
#include "serialize.h"
#include "exception.h"

ccnode_t::ccnode_t(void) : htab_node_t<ccnode_t>(), keynode_t<u_long>(0)
{
   count = files = visits = 0; 
   xfer = .0;
}

ccnode_t::ccnode_t(const char *cc, const char *desc) : htab_node_t<ccnode_t>(), keynode_t<u_long>(ctry_idx(cc))
{
   ccode = cc;
   cdesc = desc; 
   count = 0; 
   files = 0; 
   visits = 0;
   xfer = .0;
}

void ccnode_t::update(const ccnode_t& ccnode)
{
   count = ccnode.count; 
   files = ccnode.files; 
   visits = ccnode.visits;
   xfer = ccnode.xfer;
}

//
// serialization
//

u_int ccnode_t::s_data_size(void) const
{
   return datanode_t<ccnode_t>::s_data_size() + sizeof(u_long) * 3 + sizeof(double) + s_size_of(ccode);
}

u_int ccnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<ccnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<ccnode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, count);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, xfer);
   ptr = serialize(ptr, visits);
         serialize(ptr, ccode);

   return datasize;
}

u_int ccnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   u_int datasize, basesize;
   const void *ptr;

   basesize = datanode_t<ccnode_t>::s_data_size();
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);

   datanode_t<ccnode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, xfer);
   ptr = deserialize(ptr, visits);
   
   if(version >= 2)
      deserialize(ptr, ccode);
   else
      ccode = idx_ctry(nodeid);
   
   if(upcb)
      upcb(*this, arg);

   return datasize;
}

u_int ccnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_int datasize = datanode_t<ccnode_t>::s_data_size(buffer) + sizeof(u_long) * 3 + sizeof(double);
   
   if(version < 2)
      return datasize;

   return datasize + s_size_of<string_t>((u_char*) buffer + datasize);  // country code
}

//
// hash table
//

const ccnode_t& cc_hash_table::get_ccnode(const string_t& ccode) const
{
   const ccnode_t *cptr;

   if((cptr = find_node(ccode)) != NULL)
      return *cptr;

   if((cptr = find_node(string_t("*"))) != NULL)
      return *cptr;

   return empty;
}

ccnode_t& cc_hash_table::get_ccnode(const string_t& ccode) 
{
   ccnode_t *cptr;

   if((cptr = find_node(ccode)) != NULL)
      return *cptr;

   if((cptr = find_node(string_t("*"))) != NULL)
      return *cptr;

   throw exception_t(0, string_t::_format("Cannot find country node (%s)", ccode.c_str()));
}

void cc_hash_table::reset(void)
{
   ccnode_t *cptr;
   iterator iter = begin();

   while(iter.next()) {
      if((cptr = iter.item()) != NULL)
         cptr->reset();
   }
}

