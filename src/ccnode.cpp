/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ccnode.cpp
*/
#include "pch.h"

#include "ccnode.h"
#include "serialize.h"

#include <stdexcept>

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
            serializer_t::s_size_of(ccode);
}

size_t ccnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<ccnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, xfer);
   ptr = sr.serialize(ptr, visits);
   ptr = sr.serialize(ptr, ccode);
   ptr = sr.serialize(ptr, pages);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t ccnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<ccnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, visits);
   
   if(version >= 2)
      ptr = sr.deserialize(ptr, ccode);
   else
      ccode = idx_ctry(nodeid);
   
   if(version >= 3)
      ptr = sr.deserialize(ptr, pages);
   else
      pages = 0;

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

int64_t ccnode_t::s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

const void *ccnode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + datanode_t<ccnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(uint64_t) * 3;      // hits, pages, files
}

//
// hash table
//

const ccnode_t& cc_hash_table::get_ccnode(const string_t& ccode) const
{
   const ccnode_t *cptr;

   if((cptr = find_node(ccode, OBJ_REG)) != NULL)
      return *cptr;

   if((cptr = find_node(string_t("*"), OBJ_REG)) != NULL)
      return *cptr;

   return empty;
}

ccnode_t& cc_hash_table::get_ccnode(const string_t& ccode, int64_t tstamp) 
{
   ccnode_t *cptr;

   if(!ccode.isempty()) {
      if((cptr = find_node(ccode, OBJ_REG, tstamp)) != NULL)
         return *cptr;
   }

   if((cptr = find_node(string_t::hold("*", 1), OBJ_REG, tstamp)) != NULL)
      return *cptr;

   throw std::runtime_error(string_t::_format("Country code list must include '*' (%s)", ccode.c_str()));
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

//
// Instantiate all template callbacks
//
template size_t ccnode_t::s_unpack_data(const void *buffer, size_t bufsize, ccnode_t::s_unpack_cb_t<> upcb, void *arg);
