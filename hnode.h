/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hnode.h
*/
#ifndef __HNODE_H
#define __HNODE_H

#include "linklist.h"
#include "hashtab.h"
#include "basenode.h"
#include "unode.h"
#include "vnode.h"
#include "types.h"

// -----------------------------------------------------------------------
//
// host node
//
// -----------------------------------------------------------------------
// 1. Host node cannot delete active visit until visit data is factored 
// into the host data, which requires the current log timestamp and cannot 
// be done at the hnode_t level.
//
// 2. Host name and country code may only be accessed when the host node 
// is not being processed by the DNS resolver, which is any time between 
// dns_resolver_t::put_hnode and dns_resolver_t::get_hnode calls.
//
// 4. grp_visit is a linked list of ended visits that have not been grouped
// because the host name has not been resolved. Visit nodes in this list 
// are owned by the host node and do not maintain reference counts for the
// referring host node.
//
// 5. The robot flag indicates that this IP address was the source address
// of a request with a user agent matching the robot criteria. Although it 
// is possible that a human visitor will share the IP address with a robot,
// it is not very likely to justify having to maintain separate counts 
// (hits, xfer, etc) for robot and non-robot users using a particular IP 
// address.
//
// 6. It is possible for hnode_t::robot and vnode_t::robot to have different 
// values. For example, different robots may operate from the same IP address
// and some of these robots may not be listed in the configuration. Robot flag
// in the visit node should always be ignored to make sure all counters are in
// sync regardless whether there is a visit active or not. See vnode_t for
// details.
//
typedef struct hnode_t : public base_node<hnode_t> {
      static const size_t ccode_size;

      uint64_t   count;
      uint64_t   files;
      uint64_t   pages;

      uint64_t   visits;               // visits started
      uint64_t   visits_conv;          // visits converted

      uint64_t   visit_max;            // maximum visit length (in seconds)

      uint64_t   max_v_hits;           // maximum number of hits
      uint64_t   max_v_files;          // files
      uint64_t   max_v_pages;          // pages per visit

      uint64_t   dlref;                // download node reference count
      
      tstamp_t tstamp;               // last request timestamp

      vnode_t  *visit;               // current visit (NULL if none)
      vnode_t  *grp_visit;           // visits queued for name grouping

      string_t name;                 // host name

      uint64_t max_v_xfer;           // maximum transfer amount per visit
      uint64_t xfer;
      double   visit_avg;            // average visit length (in seconds)

      bool     spammer  : 1;         // caught spamming?
      bool     robot    : 1;         // robot?

      char     ccode[3];             // two-character country code

      string_t city;

      public:
         typedef void (*s_unpack_cb_t)(hnode_t& hnode, bool active, void *arg);

      public:
         hnode_t(void);
         hnode_t(const hnode_t& tmp);
         hnode_t(const string_t& ipaddr);

         ~hnode_t(void);

         void set_visit(vnode_t *vnode);

         void reset(uint64_t nodeid = 0);

         bool entry_url_set(void) const {return visit && visit->entry_url;}

         void set_entry_url(void) {if(visit) visit->entry_url = true;}
         
         void set_last_url(unode_t *unode) {if(visit) visit->set_lasturl(unode);}

         void set_ccode(const char ccode[]);

         const string_t get_ccode(void) const {return string_t().hold(const_cast<char*>(ccode), ccode_size, true, ccode_size+1);}

         void reset_ccode(void);

         const string_t& hostname(void) const {return name.isempty() ? string : name;}

         void add_grp_visit(vnode_t *vnode);

         vnode_t *get_grp_visit(void);
         
         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_xfer(const void *buf1, const void *buf2);
         static int64_t s_compare_hits(const void *buf1, const void *buf2);
} *HNODEPTR;

//
// Hosts (monthly)
//
class h_hash_table : public hash_table<hnode_t> {
   public:
      h_hash_table(void) : hash_table<hnode_t>(LMAXHASH) {}
};

#endif // __HNODE_H
