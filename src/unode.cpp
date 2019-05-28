/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

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

unode_t::unode_t(unode_t&& unode) : base_node<unode_t>(std::move(unode)) 
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

unode_t::unode_t(const string_t& urlpath, const string_t& srchargs) : base_node<unode_t>(urlpath) 
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

bool unode_t::match_key_ex(const unode_t::param_block *pb) const
{
   const char *eopath;

   if(pb->type != flag && (pb->type == OBJ_GRP || flag == OBJ_GRP))
      return false;

   // compare URL lengths
   if(pb->url->length() != pathlen)
      return false;

   eopath = &string[pathlen];

   // check if either one has search arguments and the other one doesn't
   if(!*eopath && pb->srchargs || *eopath && !pb->srchargs) 
      return false;

   // if lengths match, compare the URLs
   if(strncmp(string, pb->url->c_str(), pathlen)) 
      return false;

   // check if both URLs have search arguments
   if(*eopath && pb->srchargs) {
      // compare search argument lengths
      if(string.length() - pathlen - 1 != pb->srchargs->length())
         return false;

      // if lengths match, compare search arguments
      if(strcmp(&eopath[1], pb->srchargs->c_str()))
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
size_t unode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
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
      upcb(*this, arg, std::forward<param_t>(param) ...);

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

int64_t unode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

int64_t unode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

int64_t unode_t::s_compare_entry(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

int64_t unode_t::s_compare_exit(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

//
// Instantiate all template callbacks
//
template size_t unode_t::s_unpack_data(const void *buffer, size_t bufsize, unode_t::s_unpack_cb_t<> upcb, void *arg);
