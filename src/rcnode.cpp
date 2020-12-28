/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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

rcnode_t::rcnode_t(void) :
      keynode_t<uint64_t>(0),
      count(0),
      respcode(0)
{
}

rcnode_t::rcnode_t(const string_t& method, const string_t& url, u_short respcode) :
      keynode_t<uint64_t>(0),
      url(url),
      respcode(respcode),
      count(0),
      method(method)
{
}

bool rcnode_t::match_key(u_short respcode, const string_t& method, const string_t& url) const
{
   // compare HTTP response codes first
   if(this->respcode != respcode)
      return false;

   // then HTTP method names
   if(this->method != method)
      return false;

   // finally, compare URLs
   return this->url == url;
}

//
// serialization
//

size_t rcnode_t::s_data_size(void) const
{
   return datanode_t<rcnode_t>::s_data_size() +
               sizeof(u_char) +                       // node type
               serializer_t::s_size_of(url) +         // url
               sizeof(u_char) +                       // hexenc
               sizeof(u_short) +                      // respcode
               sizeof(uint64_t) * 2 +                 // count, value hash
               serializer_t::s_size_of(method);       // method
}

size_t rcnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<rcnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = &((u_char*)buffer)[basesize];

   // used to be in base_node and should be written first
   ptr = sr.serialize<u_char, nodetype_t>(ptr, OBJ_REG);
   ptr = sr.serialize(ptr, url);

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

   size_t basesize = datanode_t<rcnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (const u_char*) buffer + basesize;

   // used to be in base_node; skip node type - errors cannot be grouped
   ptr = sr.s_skip_field<u_char>(ptr);
   ptr = sr.deserialize(ptr, url);

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
   return hash_key(respcode, method, url);
}

const void *rcnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   serializer_t sr(buffer, bufsize);

   datasize = sizeof(uint64_t);

   const void *ptr = (const u_char*) buffer + 
         datanode_t<rcnode_t>::s_data_size(buffer, bufsize);

   // used to be in base_node
   ptr = sr.s_skip_field<u_char>(ptr);       // node type
   ptr = sr.s_skip_field<string_t>(ptr);     // url

   ptr = sr.s_skip_field<u_char>(ptr);       // hexenc
   ptr = sr.s_skip_field<u_short>(ptr);      // respcode
   ptr = sr.s_skip_field<uint64_t>(ptr);     // count

   return sr.s_skip_field<string_t>(ptr);    // method
}

int64_t rcnode_t::s_compare_value(const void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   const char *str;
   size_t slen;
   u_short tcode;
   int64_t diff;

   const void *ptr = (const u_char*) buffer + 
         datanode_t<rcnode_t>::s_data_size(buffer, bufsize);

   ptr = sr.s_skip_field<u_char>(ptr);       // node type

   ptr = sr.deserialize(ptr, str, slen);     // url

   if((diff = (int64_t) (url.length() - slen)) != 0)
      return diff;
   
   // base_node compared buffer URL on the right-hand side
   if((diff = (int64_t) strncmp(url.c_str(), str, slen)) != 0)
      return diff;

   ptr = sr.s_skip_field<bool>(ptr);         // hexenc

   ptr = sr.deserialize(ptr, tcode);         // respcode

   if((diff = (int64_t) (tcode - respcode)) != 0)
      return diff;

   ptr = sr.s_skip_field<uint64_t>(ptr);     // count

   ptr = sr.deserialize(ptr, str, slen);     // method

   if((diff = (int64_t) (method.length() - slen)) != 0)
      return diff;

   return (int64_t) strncmp(method.c_str(), str, slen);
}

const void *rcnode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   serializer_t sr(buffer, bufsize);

   datasize = sizeof(uint64_t);

   const void *ptr = (const u_char*) buffer + 
         datanode_t<rcnode_t>::s_data_size(buffer, bufsize);

   // $$$ basenode_t -- comment
   ptr = sr.s_skip_field<u_char>(ptr);       // node type
   ptr = sr.s_skip_field<string_t>(ptr);     // url

   ptr = sr.s_skip_field<u_char>(ptr);       // hexenc

   return sr.s_skip_field<u_short>(ptr);     // respcode
}

int64_t rcnode_t::s_compare_value_hash(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t rcnode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t rcnode_t::s_unpack_data(const void *buffer, size_t bufsize, rcnode_t::s_unpack_cb_t<> upcb);
