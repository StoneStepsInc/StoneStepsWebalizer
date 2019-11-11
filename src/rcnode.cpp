/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rcnode.cpp
*/
#include "pch.h"

#include "rcnode.h"
#include "serialize.h"
#include "exception.h"

#include <typeinfo>

//
//
//

rcnode_t::rcnode_t(const string_t& _method, const string_t& url, u_short _respcode) : base_node<rcnode_t>(url) 
{
   count = 0; 
   method = _method;
   respcode = _respcode;
}

bool rcnode_t::match_key_ex(const rcnode_t::param_block *pb) const
{
   // compare HTTP response codes first
   if(respcode != pb->respcode)
      return false;

   // then HTTP method names
   if(strcmp(method, pb->method))
      return false;

   // finally, compare URLs
   return !strcmp(string, pb->url);
}

//
// serialization
//

size_t rcnode_t::s_data_size(void) const
{
   return base_node<rcnode_t>::s_data_size() + 
               sizeof(u_char) +              // hexenc
               sizeof(u_short) +             // respcode
               sizeof(uint64_t) * 2 +        // count, value hash
               serializer_t::s_size_of(method); // method
}

size_t rcnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<rcnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = &((u_char*)buffer)[basesize];

   ptr = sr.serialize(ptr, false);              // hexenc
   ptr = sr.serialize(ptr, respcode);
   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, method);

   ptr = sr.serialize(ptr, s_hash_value());

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t rcnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<rcnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   ptr = sr.s_skip_field<bool>(ptr);            // hexenc

   ptr = sr.deserialize(ptr, respcode);
   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, method);

   ptr = sr.s_skip_field<uint64_t>(ptr);        // value hash
   
   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

uint64_t rcnode_t::s_hash_value(void) const
{
   return hash_str(hash_num(base_node<rcnode_t>::s_hash_value(), respcode), method, method.length());
}

const void *rcnode_t::s_field_value_mp_url(const void *buffer, size_t bufsize, size_t& datasize)
{
   return base_node<rcnode_t>::s_field_value(buffer, bufsize, datasize);
}

const void *rcnode_t::s_field_value_mp_method(const void *buffer, size_t bufsize, size_t& datasize)
{
   const void *ptr = (u_char*) buffer + base_node<rcnode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) + sizeof(u_short) + sizeof(uint64_t);

   datasize = serializer_t(buffer, bufsize).s_size_of<string_t>(ptr);

   return ptr;
}

const void *rcnode_t::s_field_value_mp_respcode(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(u_short);
   return (u_char*) buffer + base_node<rcnode_t>::s_data_size(buffer, bufsize) + sizeof(u_char);
}

const void *rcnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   const void *ptr;
   datasize = sizeof(uint64_t);
   ptr = (u_char*) buffer + base_node<rcnode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) + sizeof(u_short) + sizeof(uint64_t);
   return (u_char*) ptr + serializer_t(buffer, bufsize).s_size_of<string_t>(ptr);
}

int64_t rcnode_t::s_compare_value(const void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t datasize;
   string_t tstr;
   u_short tcode;
   int64_t diff;

   if((diff = base_node<rcnode_t>::s_compare_value(buffer, bufsize)) != 0)
      return diff;

   sr.deserialize(s_field_value_mp_respcode(buffer, bufsize, datasize), tcode);

   // tcode and respcode are both u_short (no overflow)
   if((diff = tcode - respcode) != 0)
      return diff;

   sr.deserialize(s_field_value_mp_method(buffer, bufsize, datasize), tstr);

   return method.compare(tstr);
}

const void *rcnode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<rcnode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) + sizeof(u_short);
}

int64_t rcnode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t rcnode_t::s_unpack_data(const void *buffer, size_t bufsize, rcnode_t::s_unpack_cb_t<> upcb);
