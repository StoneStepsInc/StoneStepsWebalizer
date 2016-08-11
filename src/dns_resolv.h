/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)
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
#include "thread.h"
#include "queue.h"

#include "hnode.h"

#include <db.h>                                /* DB header ****************/

#ifdef _WIN32
#include <winsock2.h>
#endif

extern "C" {
#include <maxminddb.h>
}

struct hnode_t;

//
// DNS resolver
//
class dns_resolver_t {
   private:
      //
      // DNS host node
      //
      struct dnode_t {
            hnode_t&       hnode;               // host node reference
            time_t         tstamp;              // when the name was resolved
            struct dnode_t *llist;

            union {
               sockaddr       s_addr_ip;        // s_addr would be better, but wisock2 defines it as a macro
               sockaddr_in    s_addr_ipv4;      // IPv4 socket address
               sockaddr_in6   s_addr_ipv6;      // IPv6 socket address
            };

         public:
            dnode_t(hnode_t& hnode, unsigned short sa_family);

            ~dnode_t(void);

            const string_t& key(void) const {return hnode.string;}

            nodetype_t get_type(void) const {return OBJ_REG;}
      };

   public:
      uint64_t  dns_cached;                     // Number of IP addresses found in the DNS cache
      uint64_t  dns_resolved;                  // Number of IP addresses resolved by a DNS lookup

   private:
      const config_t& config;

      MMDB_s mmdb;
      MMDB_s *geoip_db;                      // GeoIP database

      DB *dns_db;                              // DNS cache database

      time_t runtime;
      
      bool accept_host_names;

      mutex_t dnode_mutex;
      mutex_t hqueue_mutex;
      event_t dns_done_event;

      thread_t *dnode_threads;
      bool dns_thread_stop;

      u_int dns_cache_ttl;

      dnode_t *dnode_list;
      dnode_t *dnode_end;
      int dns_live_workers;                  // total number of DNS threads
      uint64_t dns_unresolved;                 // number of addresses to resolve

      queue_t<hnode_t>  hqueue;              // resolved host node queue

      u_char *buffer;

      string_t geoip_language;

   private:
      bool dns_geoip_db(void) const;

      void inc_live_workers(void);

      void dec_live_workers(void);

      int get_live_workers(void);

      void dns_create_worker_threads(void);

      bool geoip_get_ccode(const string_t& hostaddr, const sockaddr& ipaddr, string_t& ccode, string_t& city);

      bool dns_db_get(dnode_t *dnode, bool nocheck);

      void dns_db_put(dnode_t *dnode);

      bool dns_db_open(const string_t& dns_cache);

      bool dns_db_close(void);

      bool resolve_domain_name(void);

      void dns_worker_thread_proc(void);

      void process_dnode(dnode_t *dnode);

      bool dns_derive_ccode(const string_t& name, string_t& ccode) const;

      #ifdef _WIN32
      static unsigned int __stdcall dns_worker_thread_proc(void *arg);
      #else
      static void *dns_worker_thread_proc(void *arg);
      #endif

   public:
      dns_resolver_t(const config_t& config);

      ~dns_resolver_t(void);

      bool dns_init(void);
      void dns_clean_up(void);
      void dns_wait(void);

      string_t dns_resolve_name(const string_t& ipaddr);

      bool put_hnode(hnode_t *hnode);

      hnode_t *get_hnode(void);
};

#endif  /* _DNS_RESOLV_H */
