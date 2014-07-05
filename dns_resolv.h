/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   dns_resolv.h
*/

#ifndef _DNS_RESOLV_H
#define _DNS_RESOLV_H

#include "tstring.h"
#include "hashtab.h"
#include "config.h"

#include "hashtab.h"
#include "mutex.h"
#include "event.h"
#include "GeoIP.h"
#include "queue.h"

#include <db.h>                                /* DB header ****************/

struct hnode_t;

//
// DNS host node
//
typedef struct dnode_t {
      hnode_t&       hnode;               // host node reference
      time_t         tstamp;              // when the name was resolved
      struct in_addr addr;
      struct dnode_t *llist;

   public:
      dnode_t(hnode_t& hnode);

      ~dnode_t(void);

      const string_t& key(void) const {return hnode.string;}

      bool istype(u_int type) const {return true;}

} *DNODEPTR;                               /* DNS hash table node struct   */

//
// DNS resolver
//
class dns_resolver_t {
   public:
      u_long  dns_cached;					      // Number of IP addresses found in the DNS cache
      u_long  dns_resolved;						// Number of IP addresses resolved by a DNS lookup

   private:
      GeoIP *geoip_db;                       // GeoIP database
      DB *dns_db;							         // DNS cache database

      time_t runtime;
      
      bool accept_host_names;

      mutex_t dnode_mutex;
      mutex_t db_mutex;
      mutex_t hqueue_mutex;
      event_t dns_done_event;

      thread_t *dnode_threads;
      bool dns_thread_stop;

      u_int dns_children;
      u_int dns_cache_ttl;

      DNODEPTR dnode_list;
      DNODEPTR dnode_end;
      int dns_live_workers;                  // total number of DNS threads
      u_long dns_unresolved;                 // number of addresses to resolve

      queue_t<hnode_t>  hqueue;              // resolved host node queue

      u_char *buffer;

   private:
      bool dns_geoip_db(void) const;

      void inc_live_workers(void);

      void dec_live_workers(void);

      int get_live_workers(void);

      void dns_create_worker_threads(void);

      bool geoip_get_ccode(u_long ipnum, const string_t& ipaddr, string_t& ccode);

      bool dns_db_get(DNODEPTR dnode, bool nocheck);

      void dns_db_put(DNODEPTR dnode);

      bool dns_db_open(const string_t& dns_cache);

      bool dns_db_close(void);

      bool resolve_domain_name(void);

      void dns_worker_thread_proc(void);

      void process_dnode(DNODEPTR dnode);

      bool dns_derive_ccode(const string_t& name, string_t& ccode) const;

      #ifdef _WIN32
      static unsigned int __stdcall dns_worker_thread_proc(void *arg);
      #else
      static void *dns_worker_thread_proc(void *arg);
      #endif

   public:
      dns_resolver_t(void);

      ~dns_resolver_t(void);

      bool dns_init(const config_t& config);
      void dns_clean_up(void);
      void dns_wait(void);

      string_t dns_resolve_name(const string_t& ipaddr);

      bool put_hnode(hnode_t *hnode, struct in_addr *addr);

      hnode_t *get_hnode(void);
};

#endif  /* _DNS_RESOLV_H */
