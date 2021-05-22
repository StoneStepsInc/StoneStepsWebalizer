/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hnode.cpp
*/
#include "pch.h"

#include "hnode.h"
#include "serialize.h"

// -----------------------------------------------------------------------
//
// hnode_t
//
// -----------------------------------------------------------------------

hnode_t::hnode_t(void) : base_node<hnode_t>(),
      geoname_id(0),
      as_num(0)
{
   count = files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0;
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   spammer = false;
   robot = false;
   resolved = false;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = nullptr;
   dlref = 0;
   grp_visit = nullptr;
   latitude = longitude = 0.;
}

hnode_t::hnode_t(hnode_t&& hnode) noexcept : base_node<hnode_t>(std::move(hnode)),
      name(std::move(hnode.name)),
      city(std::move(hnode.city)),
      geoname_id(hnode.geoname_id),
      as_num(hnode.as_num),
      as_org(std::move(hnode.as_org))
{
   spammer = hnode.spammer;
   robot = hnode.robot;
   resolved = hnode.resolved;
   count = hnode.count;
   files = hnode.files;
   pages = hnode.pages;

   xfer = hnode.xfer;

   visits = hnode.visits; 
   visits_conv = hnode.visits_conv;

   visit_avg = hnode.visit_avg;
   visit_max = hnode.visit_max; 

   max_v_hits = hnode.max_v_hits;
   max_v_files = hnode.max_v_files;
   max_v_pages = hnode.max_v_pages;
   max_v_xfer = hnode.max_v_xfer;

   ccode[0] = hnode.ccode[0];
   ccode[1] = hnode.ccode[1];
   ccode[2] = 0;

   dlref = hnode.dlref;
   hnode.dlref = 0;

   tstamp = hnode.tstamp;

   grp_visit = hnode.grp_visit;
   hnode.grp_visit = nullptr;

   latitude = hnode.latitude;
   longitude = hnode.longitude;

   visit = hnode.visit;
   hnode.visit = nullptr;
}

hnode_t::hnode_t(const string_t& ipaddr, nodetype_t type) :
      base_node<hnode_t>(ipaddr, type),
      geoname_id(0),
      as_num(0)
{
   spammer = false;
   robot = false;
   resolved = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = nullptr;
   dlref = 0;
   grp_visit = nullptr;
   latitude = longitude = 0.;
}

hnode_t::~hnode_t(void)
{
   vnode_t *vptr;

   if(visit) {
      if(!--visit->hostref)
         delete visit;
   }

   // if grp_visit isn't nullptr, something went wrong
   while(grp_visit) {
      vptr = grp_visit;
      grp_visit = grp_visit->next;
      delete vptr;
   }
}

void hnode_t::set_visit(storable_t<vnode_t> *vnode)
{
   if(visit == vnode)
      return;

   if(visit)
      visit->hostref--;

   if((visit = vnode) != nullptr)
      visit->hostref++;
}

void hnode_t::add_grp_visit(storable_t<vnode_t> *vnode)
{
   if(vnode) {
      vnode->next = grp_visit;
      grp_visit = vnode;
   }
}

vnode_t *hnode_t::get_grp_visit(void)
{
   vnode_t *vnode = grp_visit;
   
   if(vnode)
      grp_visit = vnode->next;
      
   return vnode;
}

void hnode_t::reset(uint64_t nodeid)
{
   base_node<hnode_t>::reset(nodeid);

   spammer = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = 0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   dlref = 0;
   tstamp.reset();

   set_visit(nullptr);
}

void hnode_t::set_ccode(const char _ccode[2])
{
   ccode[0] = _ccode[0];
   ccode[1] = _ccode[1];
   ccode[2] = 0;
}

void hnode_t::reset_ccode(void)
{
   ccode[0] = ccode[1] = ccode[2] = 0;
}

//
// serialization
//

size_t hnode_t::s_data_size(void) const
{
   return base_node<hnode_t>::s_data_size() + 
               sizeof(u_char) * 3 +             // spammer, active, robot
               sizeof(uint64_t) * 3 +             // count, files, pages
               sizeof(uint64_t) * 3 +             // visits, visit_max, visits_conv
               sizeof(uint64_t) * 3 +             // max_v_hits, max_v_files, max_v_pages
               sizeof(uint64_t)     +             // hash(value)  
               serializer_t::s_size_of(tstamp) +// tstamp 
               sizeof(double)  +                // visit_avg
               sizeof(uint64_t) * 2 +           // xfer, max_v_xfer
               serializer_t::s_size_of(name) +  // name
               ccode_size         +             // country code
               serializer_t::s_size_of(city) +  // city
               sizeof(double) * 2 +             // latitude, longitude
               sizeof(uint32_t) +               // geoname_id
               serializer_t::s_size_of(as_num) +      // as_num
               serializer_t::s_size_of(as_org);       // as_org
}

size_t hnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = base_node<hnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, spammer);
   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, pages);
   ptr = sr.serialize(ptr, xfer);
   ptr = sr.serialize(ptr, visits);
   ptr = sr.serialize(ptr, visit_avg);
   ptr = sr.serialize(ptr, visit_max);
   ptr = sr.serialize(ptr, max_v_hits);
   ptr = sr.serialize(ptr, max_v_files);
   ptr = sr.serialize(ptr, max_v_pages);
   ptr = sr.serialize(ptr, max_v_xfer);
   ptr = sr.serialize(ptr, (visit) ? true : false);

   ptr = sr.serialize(ptr, s_hash_value());
   ptr = sr.serialize(ptr, name);
   ptr = sr.serialize(ptr, (char (&)[2]) ccode);

   ptr = sr.serialize(ptr, robot);
   ptr = sr.serialize(ptr, visits_conv);
   ptr = sr.serialize(ptr, tstamp);

   ptr = sr.serialize(ptr, city);

   ptr = sr.serialize(ptr, latitude);
   ptr = sr.serialize(ptr, longitude);

   ptr = sr.serialize(ptr, geoname_id);

   ptr = sr.serialize(ptr, as_num);
   ptr = sr.serialize(ptr, as_org);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t hnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   bool active, tmp;

   size_t basesize = base_node<hnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, tmp); spammer = tmp;
   ptr = sr.deserialize(ptr, count);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, pages);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, visits);
   ptr = sr.deserialize(ptr, visit_avg);
   ptr = sr.deserialize(ptr, visit_max);
   ptr = sr.deserialize(ptr, max_v_hits);
   ptr = sr.deserialize(ptr, max_v_files);
   ptr = sr.deserialize(ptr, max_v_pages);
   ptr = sr.deserialize(ptr, max_v_xfer);
   ptr = sr.deserialize(ptr, active);

   ptr = sr.s_skip_field<uint64_t>(ptr);      // value hash

   ptr = sr.deserialize(ptr, name);
   ptr = sr.deserialize(ptr, (char (&)[2]) ccode);

   if(version >= 2)
      {ptr = sr.deserialize(ptr, tmp); robot = tmp;}
   else
      robot = false;

   if(version >= 3)
      ptr = sr.deserialize(ptr, visits_conv);
   else
      visits_conv = 0;

   if(version >= 4) {
      if(version >= 5)
         ptr = sr.deserialize(ptr, tstamp);
      else {
         uint64_t tmp;
         ptr = sr.deserialize(ptr, tmp);
         tstamp.reset((time_t) tmp);
      }
   }
   else
      tstamp.reset();

   if(version >= 6)
      ptr = sr.deserialize(ptr, city);
   else
      city.clear();

   if(version >= 7) {
      ptr = sr.deserialize(ptr, latitude);
      ptr = sr.deserialize(ptr, longitude);
   }

   if(version >= 8)
      ptr = sr.deserialize(ptr, geoname_id);
   else
      geoname_id = 0;

   if(version >= 9) {
      ptr = sr.deserialize(ptr, as_num);
      ptr = sr.deserialize(ptr, as_org);
   }
   else {
      as_num = 0;
      as_org.reset();
   }

   visit = nullptr;

   // all hosts in the state database are assumed to be resolved
   resolved = true;
   
   if(upcb)
      upcb(*this, active, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

const void *hnode_t::s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*)buffer + base_node<hnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(u_char) * 2 +       // spammer, active
            sizeof(uint64_t) * 3 +     // count, files, pages
            sizeof(uint64_t) * 2 +     // visits, visit_max
            sizeof(uint64_t) * 3 +     // max_v_hits, max_v_files, max_v_pages
            sizeof(double)       +     // visit_avg
            sizeof(uint64_t) * 2;      // xfer, max_v_xfer
}

const void *hnode_t::s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(u_char) +           // spammer 
            sizeof(uint64_t) * 3;      // count, files, pages
}

const void *hnode_t::s_field_hits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(u_char);            // spammer
}

int64_t hnode_t::s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

int64_t hnode_t::s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

//
// Instantiate all template callbacks
//
template size_t hnode_t::s_unpack_data(const void *buffer, size_t bufsize, hnode_t::s_unpack_cb_t<> upcb);
template size_t hnode_t::s_unpack_data(const void *buffer, size_t bufsize, hnode_t::s_unpack_cb_t<void*> upcb, void *arg);
template size_t hnode_t::s_unpack_data(const void *buffer, size_t bufsize, hnode_t::s_unpack_cb_t<storable_t<vnode_t>&, storable_t<unode_t>&> upcb, storable_t<vnode_t>& vnode, storable_t<unode_t>& unode);
