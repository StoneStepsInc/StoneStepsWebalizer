/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ccnode.cpp
*/
#include "pch.h"

#include "ccnode.h"
#include "serialize.h"
#include "exception.h"

ccnode_t::ccnode_t(void) : keynode_t<uint64_t>(0)
{
   count = files = visits = pages = 0; 
   xfer = 0;
}

ccnode_t::ccnode_t(const char *cc, const char *desc) : keynode_t<uint64_t>(ctry_idx(cc))
{
   ccode = cc;
   cdesc = desc; 
   count = 0; 
   files = 0; 
   pages = 0; 
   visits = 0;
   xfer = 0;
}

void ccnode_t::update(const ccnode_t& ccnode)
{
   count = ccnode.count; 
   files = ccnode.files; 
   pages = ccnode.pages; 
   visits = ccnode.visits;
   xfer = ccnode.xfer;
}

//
// Encodes each character of a country code into five bits of the returned 64-bit 
// integer that can be used as a database key without having to use Berkeley DB
// sequences.
//
uint64_t ccnode_t::ctry_idx(const char *str)
{
   uint64_t idx=0;
   const char *cp1=str;

   if(str) {
      if(*str == '*')
         return 0;
      while(*cp1) {
         idx = (idx <<= 5) | (*cp1 - 'a' + 1);
         cp1++;
      }
   }

   return idx;
}

string_t ccnode_t::idx_ctry(uint64_t idx)
{
   char ch, buf[7], *cp;
   string_t ccode;
   
   // if index is zero, return the unknown country code
   if(idx == 0) {
      ccode = "*";
      return ccode;
   }
   
   // set the pointer to one character past the buffer
   cp = buf+sizeof(buf);
   
   // loop through 5-bit integers and fill the buffer
   while(idx) {
      ch = (char) (idx & 0x1F);
      *--cp = (ch + 'a' - 1);
      idx >>= 5;
   }
   
   // assign and return
   ccode.assign(cp, buf+sizeof(buf)-cp);
   
   return ccode;
}

//
// serialization
//

size_t ccnode_t::s_data_size(void) const
{
   return datanode_t<ccnode_t>::s_data_size() + 
            sizeof(uint64_t) * 3 +     // count, files, visits
            sizeof(uint64_t) +         // xfer 
            sizeof(uint64_t) +         // pages
            s_size_of(ccode);
}

size_t ccnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
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
   ptr = serialize(ptr, ccode);
         serialize(ptr, pages);

   return datasize;
}

size_t ccnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   size_t datasize, basesize;
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
      ptr = deserialize(ptr, ccode);
   else
      ccode = idx_ctry(nodeid);
   
   if(version >= 3)
      deserialize(ptr, pages);
   else
      pages = 0;

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

size_t ccnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = datanode_t<ccnode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3 +     // count, files, visits
            sizeof(uint64_t);          // xfer
   
   if(version < 2)
      return datasize;

   datasize += s_size_of<string_t>((u_char*) buffer + datasize);  // country code

   if(version < 3)
      return datasize;

   datasize += sizeof(uint64_t);       // pages

   return datasize;
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

