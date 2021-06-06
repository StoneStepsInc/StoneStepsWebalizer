/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    unode.cpp
*/
#include "pch.h"

#include "unode.h"
#include "serialize.h"
#include "config.h"
#include "exception.h"
#include "util_url.h"

// -----------------------------------------------------------------------
//
// unode_t
//
// -----------------------------------------------------------------------

unode_t::unode_t(uint64_t nodeid) : base_node<unode_t>(nodeid) 
{
   target = false;
   count = files = entry = exit = 0; 
   xfer = 0;
   avgtime = maxtime = .0; 
   urltype = URL_TYPE_UNKNOWN; 
   pathlen = 0;
   vstref = 0;
}

unode_t::unode_t(unode_t&& unode) noexcept :
      base_node<unode_t>(std::move(unode)) 
{
   target = unode.target;
   count = unode.count;
   files = unode.files;
   entry = unode.entry;
   exit = unode.exit; 
   xfer = unode.xfer;
   avgtime = unode.avgtime; 
   maxtime = unode.maxtime;
   urltype = unode.urltype; 
   pathlen = unode.pathlen;

   vstref = unode.vstref;
   unode.vstref = 0;
}

unode_t::unode_t(const string_t& urlpath, nodetype_t type, const string_t& srchargs) :
      base_node<unode_t>(urlpath, type) 
{
   pathlen = (u_short) string.length();

   if(!srchargs.isempty()) {
      string += '?';
      string += srchargs;
   }

   target = false;
   urltype = URL_TYPE_UNKNOWN;

   count = 0;
   files = entry = exit = 0; 
   xfer = 0;
   avgtime = maxtime = .0; 

   vstref = 0;
}

void unode_t::reset(uint64_t nodeid)
{
   base_node<unode_t>::reset(nodeid);

   target = false; 
   count = files = entry = exit = 0; 
   xfer = 0;
   avgtime = maxtime = .0; 
   urltype = URL_TYPE_UNKNOWN; 
   pathlen = 0;
   vstref = 0;
}

u_char unode_t::update_url_type(u_char type)
{
   if(type == URL_TYPE_UNKNOWN) {
      // if we already saw at least one request with a known port, treat the unknown type as other
      if(urltype & (URL_TYPE_HTTP | URL_TYPE_HTTPS))
         type |= URL_TYPE_OTHER;
   }
   else {
      // make sure only allowed bits are set in the argument
      if(type & ~(URL_TYPE_HTTP | URL_TYPE_HTTPS | URL_TYPE_OTHER))
         throw exception_t(0, "Bad URL type");
   }

   urltype |= type;

   return urltype;
}

char unode_t::get_url_type_ind(void) const
{
   return (urltype == URL_TYPE_HTTPS) ? '*' : 
            (urltype == URL_TYPE_MIXED) ? '-' : 
            (urltype & URL_TYPE_OTHER) ? '+' : ' ';
}

const char *unode_t::get_query(void) const
{
   // skip the question mark
   return string.length() > pathlen ? string.c_str() + pathlen + 1 : "";
}

size_t unode_t::get_query_len(void) const
{
   // account for the question mark
   return string.length() > pathlen ? string.length() - pathlen - 1 : 0;
}

bool unode_t::match_key(const string_t& url, const string_t& srchargs) const
{
   const char *eopath;

   // compare URL lengths
   if(url.length() != pathlen)
      return false;

   eopath = &string[pathlen];

   // check if either one has search arguments and the other one doesn't
   if(!*eopath && !srchargs.isempty() || *eopath && srchargs.isempty()) 
      return false;

   // if lengths match, compare the URLs
   if(strncmp(string, url.c_str(), pathlen)) 
      return false;

   // check if both URLs have search arguments
   if(*eopath && srchargs) {
      // compare search argument lengths
      if(string.length() - pathlen - 1 != srchargs.length())
         return false;

      // if lengths match, compare search arguments
      if(strcmp(&eopath[1], srchargs.c_str()))
         return false;
   }

   return true;
}

//
// serialization
//
size_t unode_t::s_data_size(void) const
{
   return base_node<unode_t>::s_data_size() + 
            sizeof(u_char) * 3 +       // hexenc, target, urltype 
            sizeof(u_short) +          // pathlen
            sizeof(uint64_t) * 5 +     // count, files, entry, exit, value hash
            sizeof(double) * 2 +       // avgtime, maxtime
            sizeof(uint64_t);          // xfer
}

size_t unode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<unode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, false);
   ptr = sr.serialize(ptr, urltype);
   ptr = sr.serialize(ptr, pathlen);
   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, entry);
   ptr = sr.serialize(ptr, exit);
   ptr = sr.serialize(ptr, xfer);
   ptr = sr.serialize(ptr, avgtime);

   ptr = sr.serialize(ptr, s_hash_value());
   ptr = sr.serialize(ptr, maxtime);
   ptr = sr.serialize(ptr, target);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t unode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   bool tmp;
   
   size_t basesize = base_node<unode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.s_skip_field<bool>(ptr);         // hexenc

   ptr = sr.deserialize(ptr, urltype);
   ptr = sr.deserialize(ptr, pathlen);
   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, entry);
   ptr = sr.deserialize(ptr, exit);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, avgtime);

   ptr = sr.s_skip_field<uint64_t>(ptr);      // value hash

   if(version >= 2)
      ptr = sr.deserialize(ptr, maxtime);
   else
      maxtime = 0;

   if(version >= 3)
      ptr = sr.deserialize(ptr, tmp), target = tmp;
   else
      target = false;

   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

const void *unode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<unode_t>::s_data_size(buffer, bufsize) + 
            sizeof(u_char) * 2 + 
            sizeof(u_short) + 
            sizeof(uint64_t) * 4 + 
            sizeof(double) +
            sizeof(uint64_t);          // xfer
}

const void *unode_t::s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<unode_t>::s_data_size(buffer, bufsize) + 
            sizeof(u_char) * 2 + 
            sizeof(u_short) + 
            sizeof(uint64_t) * 4;
}

const void *unode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<unode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) * 2 + sizeof(u_short);
}

const void *unode_t::s_field_entry(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<unode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) * 2 + sizeof(u_short) + sizeof(uint64_t) * 2;
}

const void *unode_t::s_field_exit(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<unode_t>::s_data_size(buffer, bufsize) + sizeof(u_char) * 2 + sizeof(u_short) + sizeof(uint64_t) * 3;
}

int64_t unode_t::s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t unode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t unode_t::s_compare_entry(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t unode_t::s_compare_exit(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t unode_t::s_unpack_data(const void *buffer, size_t bufsize, unode_t::s_unpack_cb_t<> upcb);
