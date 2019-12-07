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

dlnode_t::dlnode_t(void) :
      keynode_t<uint64_t>(0)
{
   count = 0;
   sumhits = 0;
   sumxfer = 0;
   avgxfer = 0.;
   avgtime = sumtime = 0;

   download = NULL;
   hnode = NULL;
}

dlnode_t::dlnode_t(const string_t& name, hnode_t& nptr) :
      keynode_t<uint64_t>(0),
      name(name)
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

dlnode_t::dlnode_t(dlnode_t&& tmp) :
      keynode_t<uint64_t>(std::move(tmp)),
      name(std::move(tmp.name))
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
   keynode_t<uint64_t>::reset(nodeid);

   name.clear();

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

bool dlnode_t::match_key(const string_t& ipaddr, const string_t& dlname) const
{
   if(!hnode)
      return false;

   // compare IP addresses first
   if(strcmp(hnode->string, ipaddr)) 
      return false;

   // and then download names
   return !strcmp(name, dlname);
}

uint64_t dlnode_t::get_hash(void) const
{
   return hash_ex(hash_ex(0, hnode ? hnode->string : string_t()), name);
}

//
// serialization
//

size_t dlnode_t::s_data_size(void) const
{
   return datanode_t<dlnode_t>::s_data_size() +
            sizeof(u_char) +                       // node type
            serializer_t::s_size_of(name) +        // url
            sizeof(uint64_t) * 2 +                 // count, sumhits
            sizeof(double) * 3 +                   // avgxfer, avgtime, sumtime
            sizeof(uint64_t) +                     // sumxfer
            sizeof(uint64_t) +                     // hash value
            sizeof(u_char) +                       // active download
            sizeof(uint64_t);                      // host node ID
}

size_t dlnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<dlnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   // used to be in basenode and should be written first
   ptr = sr.serialize<u_char, nodetype_t>(ptr, OBJ_REG);
   ptr = sr.serialize(ptr, name);

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
size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   uint64_t hostid;
   bool active;

   size_t basesize = datanode_t<dlnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (const u_char*) buffer + basesize;

   // used to be in basenode; skip node type - download jobs cannot be grouped
   ptr = sr.s_skip_field<u_char>(ptr);       // node type
   ptr = sr.deserialize(ptr, name);

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
      upcb(*this, hostid, active, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

uint64_t dlnode_t::s_hash_value(void) const
{
   //
   // Stored hash is different from the one used in the hash table because
   // we don't want to look up host name in the database when just walking
   // the downloads table. See more in s_compare_value.
   //
   return hash_num(hash_ex(0, name), hnode ? hnode->nodeid : 0);
}

int64_t dlnode_t::s_compare_value(const void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   const char *str;
   size_t slen;
   uint64_t hostid;
   int64_t diff;

   if(!hnode)
      return -1;

   const void *ptr = (const u_char*) buffer + 
         datanode_t<dlnode_t>::s_data_size(buffer, bufsize);

   ptr = sr.s_skip_field<u_char>(ptr);       // node type

   ptr = sr.deserialize(ptr, str, slen);     // name

   if((diff = (int64_t) (name.length() - slen)) != 0)
      return diff;

   if((diff = (int64_t) strncmp(name.c_str(), str, slen)) != 0)
      return diff;

   ptr = sr.s_skip_field<uint64_t>(ptr);     // count
   ptr = sr.s_skip_field<uint64_t>(ptr);     // sumhits
   ptr = sr.s_skip_field<uint64_t>(ptr);     // sumxfer
   ptr = sr.s_skip_field<double>(ptr);       // avgxfer
   ptr = sr.s_skip_field<double>(ptr);       // avgtime
   ptr = sr.s_skip_field<double>(ptr);       // sumtime
   ptr = sr.s_skip_field<uint64_t>(ptr);     // value hash
   ptr = sr.s_skip_field<bool>(ptr);         // active download

   ptr = sr.deserialize(ptr, hostid);

   //
   // This function is called against the download record in the database,
   // so there is no host name available at this point and we compare to
   // the next best thing - the host ID. This means that duplicate download
   // names will be ordered by their numeric host node ID, which will seem
   // like random ordering, since the host ID value is just a sequential
   // value incremented as new host nodes are added. Storing host name
   // in the download node sounds like a good idea, but host names may
   // change as DNS is enabled/disabled or refreshed, so some more complex
   // way of doing this might be required. For now, node ID keeps these
   // records unique, even though it fails for sorting.
   //
   return hnode->nodeid == hostid ? 0 : hnode->nodeid > hostid ? 1 : -1;
}

const void *dlnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   serializer_t sr(buffer, bufsize);

   datasize = sizeof(uint64_t);

   const void *ptr = (const u_char*) buffer + 
         datanode_t<dlnode_t>::s_data_size(buffer, bufsize);

   ptr = sr.s_skip_field<u_char>(ptr);       // node type
   ptr = sr.s_skip_field<string_t>(ptr);     // name
   ptr = sr.s_skip_field<uint64_t>(ptr);     // count
   ptr = sr.s_skip_field<uint64_t>(ptr);     // sumhits
   ptr = sr.s_skip_field<uint64_t>(ptr);     // sumxfer
   ptr = sr.s_skip_field<double>(ptr);       // avgxfer
   ptr = sr.s_skip_field<double>(ptr);       // avgtime

   return sr.s_skip_field<double>(ptr);      // sumtime
}

const void *dlnode_t::s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize)
{
   serializer_t sr(buffer, bufsize);

   datasize = sizeof(uint64_t);

   const void *ptr = (const u_char*) buffer + 
         datanode_t<dlnode_t>::s_data_size(buffer, bufsize);

   ptr = sr.s_skip_field<u_char>(ptr);       // node type
   ptr = sr.s_skip_field<string_t>(ptr);     // name
   ptr = sr.s_skip_field<uint64_t>(ptr);     // count
   return sr.s_skip_field<uint64_t>(ptr);    // sumhits
}

int64_t dlnode_t::s_compare_value_hash(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t dlnode_t::s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<> upcb);
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<void*, const storable_t<hnode_t>&> upcb, void *arg, const storable_t<hnode_t>& hnode);
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<storable_t<hnode_t>&> upcb, storable_t<hnode_t>& hnode);
template size_t dlnode_t::s_unpack_data(const void *buffer, size_t bufsize, dlnode_t::s_unpack_cb_t<void*, storable_t<hnode_t>&> upcb, void *arg, storable_t<hnode_t>& hnode);

#include "hashtab_tmpl.cpp"
