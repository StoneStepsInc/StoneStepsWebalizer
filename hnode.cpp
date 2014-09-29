/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hnode.cpp
*/
#include "pch.h"

#include "hnode.h"
#include "serialize.h"
#include "util.h"

const u_int hnode_t::ccode_size = 2;   // in bytes, not counting the zero terminator

// -----------------------------------------------------------------------
//
// hnode_t
//
// -----------------------------------------------------------------------

hnode_t::hnode_t(void) : base_node<hnode_t>()
{
   count = files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0;
   xfer = .0;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = .0;
   spammer = false;
   robot = false;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = NULL;
   dlref = 0;
   tstamp = 0;
   grp_visit = NULL;
}

hnode_t::hnode_t(const hnode_t& hnode) : base_node<hnode_t>(hnode)
{
   spammer = hnode.spammer;
   robot = hnode.robot;
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

   name = hnode.name;

   ccode[0] = hnode.ccode[0];
   ccode[1] = hnode.ccode[1];
   ccode[2] = 0;

   dlref = 0;
   tstamp = hnode.tstamp;

   grp_visit = NULL;                         // grp_visit is not copied

   if((visit = hnode.visit) != NULL)
      visit->hostref++;
}

hnode_t::hnode_t(const string_t& ipaddr) : base_node<hnode_t>(ipaddr)
{
   spammer = false;
   robot = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0.;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = .0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   visit = NULL;
   dlref = 0;
   tstamp = 0;
   grp_visit = NULL;
}

hnode_t::~hnode_t(void)
{
   vnode_t *vptr;

   if(visit) {
      if(!--visit->hostref)
         delete visit;
   }

   // if grp_visit isn't NULL, something went wrong
   while(grp_visit) {
      vptr = grp_visit;
      grp_visit = grp_visit->next;
      delete vptr;
   }
}

void hnode_t::set_visit(vnode_t *vnode)
{
   if(visit == vnode)
      return;

   if(visit)
      visit->hostref--;

   if((visit = vnode) != NULL)
      visit->hostref++;
}

void hnode_t::add_grp_visit(vnode_t *vnode)
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

void hnode_t::reset(u_long nodeid)
{
   base_node<hnode_t>::reset(nodeid);

   spammer = false;
   count = 0;
   files = pages = visits = visits_conv = 0;
   visit_avg = .0;
   visit_max = 0; 
   xfer = 0.;
   max_v_hits = max_v_files = max_v_pages = 0;
   max_v_xfer = .0;
   ccode[0] = ccode[1] = ccode[2] = 0;
   dlref = 0;
   tstamp = 0;

   set_visit(NULL);
}

void hnode_t::set_ccode(const char _ccode[])
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

u_int hnode_t::s_data_size(void) const
{
   return base_node<hnode_t>::s_data_size() + 
               sizeof(u_char) * 3 +             // spammer, active, robot
               sizeof(u_long) * 3 +             // count, files, pages
               sizeof(u_long) * 3 +             // visits, visit_max, visits_conv
               sizeof(u_long) * 3 +             // max_v_hits, max_v_files, max_v_pages
               sizeof(u_long)     +             // hash(value)  
               sizeof(int64_t)    +             // tstamp 
               sizeof(double) * 3 +             // xfer, visit_avg, max_v_xfer
               s_size_of(name)    + 
               ccode_size;                      // country code
}

u_int hnode_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr;

   basesize = base_node<hnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   base_node<hnode_t>::s_pack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   ptr = serialize(ptr, spammer);
   ptr = serialize(ptr, count);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, pages);
   ptr = serialize(ptr, xfer);
   ptr = serialize(ptr, visits);
   ptr = serialize(ptr, visit_avg);
   ptr = serialize(ptr, visit_max);
   ptr = serialize(ptr, max_v_hits);
   ptr = serialize(ptr, max_v_files);
   ptr = serialize(ptr, max_v_pages);
   ptr = serialize(ptr, max_v_xfer);
   ptr = serialize(ptr, (visit) ? true : false);

   ptr = serialize(ptr, s_hash_value());
   ptr = serialize(ptr, name);
   ptr = serialize(ptr, ccode, ccode_size);

   ptr = serialize(ptr, robot);
   ptr = serialize(ptr, visits_conv);
         serialize<int64_t>(ptr, tstamp);

   return datasize;
}

u_int hnode_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   bool active, tmp;
   u_int datasize, basesize;
   u_short version;
   const void *ptr;

   basesize = base_node<hnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   base_node<hnode_t>::s_unpack_data(buffer, bufsize);
   ptr = &((u_char*)buffer)[basesize];

   compatibility_deserializer<5, u_long, int64_t, time_t> deserialize_time_t(version);

   ptr = deserialize(ptr, tmp); spammer = tmp;
   ptr = deserialize(ptr, count);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, pages);
   ptr = deserialize(ptr, xfer);
   ptr = deserialize(ptr, visits);
   ptr = deserialize(ptr, visit_avg);
   ptr = deserialize(ptr, visit_max);
   ptr = deserialize(ptr, max_v_hits);
   ptr = deserialize(ptr, max_v_files);
   ptr = deserialize(ptr, max_v_pages);
   ptr = deserialize(ptr, max_v_xfer);
   ptr = deserialize(ptr, active);

   ptr = s_skip_field<u_long>(ptr);      // value hash

   ptr = deserialize(ptr, name);
   ptr = deserialize(ptr, ccode, ccode_size);

   if(version >= 2)
      {ptr = deserialize(ptr, tmp); robot = tmp;}
   else
      robot = false;

   if(version >= 3)
      ptr = deserialize(ptr, visits_conv);
   else
      visits_conv = 0;

   if(version >= 4)
      ptr = deserialize_time_t(ptr, tstamp);
   else
      tstamp = 0;

   visit = NULL;
   
   if(upcb)
      upcb(*this, active, arg);

   return datasize;
}

u_int hnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_int datasize = base_node<hnode_t>::s_data_size(buffer) + 
               sizeof(u_char) * 2 +    // spammer, active
               sizeof(u_long) * 3 +    // count, files, pages 
               sizeof(u_long) * 2 +    // visits, visit_max
               sizeof(u_long) * 3 +    // max_v_hits, max_v_files, max_v_pages
               sizeof(u_long)     +    // hash(value)
               sizeof(double) * 3;     // xfer, visit_avg, max_v_xfer

   // host address and country code
   datasize += s_size_of<string_t>((u_char*) buffer + datasize) + ccode_size;

   if(version < 2)
      return datasize;

   datasize += sizeof(u_char);         // robot

   if(version < 3)
      return datasize;

   datasize += sizeof(u_long);         // visits_conv

   if(version < 4)
      return datasize;
 
   if(version < 5)
      return datasize + 
               sizeof(u_long);         // tstamp

   return datasize + sizeof(int64_t);  // tstamp
}

const void *hnode_t::s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*)buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char) * 2 +       // spammer, active
            sizeof(u_long) * 3 +       // count, files, pages
            sizeof(u_long) * 2 +       // visits, visit_max
            sizeof(u_long) * 3 +       // max_v_hits, max_v_files, max_v_pages
            sizeof(double) * 3;        // xfer, visit_avg, max_v_xfer
}

const void *hnode_t::s_field_xfer(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(double);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char) +           // spammer 
            sizeof(u_long) * 3;        // count, files, pages
}

const void *hnode_t::s_field_hits(const void *buffer, u_int bufsize, u_int& datasize)
{
   datasize = sizeof(u_long);
   return (u_char*) buffer + base_node<hnode_t>::s_data_size(buffer) + 
            sizeof(u_char);            // spammer
}

int hnode_t::s_compare_xfer(const void *buf1, const void *buf2)
{
   return s_compare<double>(buf1, buf2);
}

int hnode_t::s_compare_hits(const void *buf1, const void *buf2)
{
   return s_compare<u_long>(buf1, buf2);
}


