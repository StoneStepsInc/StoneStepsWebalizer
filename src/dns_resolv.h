/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
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
#include "tstamp.h"

#include "hnode.h"

#include <db_cxx.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

extern "C" {
#include <maxminddb.h>
}

class dns_resolver_t;
struct hnode_t;

//
// DNS resolver
//
class dns_resolver_t {
   private:
      class dnode_t;

      // worker thread context
      struct wrk_ctx_t {
         dns_resolver_t&   dns_resolver;
         Db                *dns_db = NULL;
         void              *buffer = NULL;
         size_t            bufsize = 0;

         wrk_ctx_t(dns_resolver_t& dns_resolver) : dns_resolver(dns_resolver)
         {
         }
      };

   public:
      uint64_t  dns_cached;                  // Number of IP addresses found in the DNS cache
      uint64_t  dns_resolved;                // Number of IP addresses resolved by a DNS lookup

   private:
      const config_t& config;

      MMDB_s mmdb;
      MMDB_s *geoip_db;                      // GeoIP database

      std::vector<wrk_ctx_t> wrk_ctxs;       // DNS cache database

      tstamp_t runtime;
      
      bool accept_host_names;

      mutex_t dnode_mutex;
      mutex_t hqueue_mutex;
      event_t dns_done_event;

      std::vector<thread_t> workers;         // worker threads
      bool dns_thread_stop;

      u_int dns_cache_ttl;

      dnode_t *dnode_list;
      dnode_t *dnode_end;
      int dns_live_workers;                  // total number of DNS threads
      uint64_t dns_unresolved;               // number of addresses to resolve

      queue_t<dnode_t>  hqueue;              // resolved host node queue

      string_t geoip_language;

   private:
      bool dns_geoip_db(void) const;

      void inc_live_workers(void);

      void dec_live_workers(void);

      int get_live_workers(void);

      void dns_create_worker_threads(void);

      bool geoip_get_ccode(const string_t& hostaddr, const sockaddr& ipaddr, string_t& ccode, string_t& city);

      bool dns_db_get(dnode_t *dnode, Db *dns_db, bool nocheck, void *buffer, size_t bufsize);

      void dns_db_put(const dnode_t *dnode, Db *dns_db, void *buffer, size_t bufsize);

      Db *dns_db_open(const string_t& dns_cache);

      void dns_db_close(Db *dns_db);

      bool resolve_domain_name(Db *dns_db, void *buffer, size_t bufsize);

      void dns_worker_thread_proc(wrk_ctx_t& wrk_ctx);

      void process_dnode(dnode_t *dnode);

      void queue_dnode(dnode_t *dnode);

      static bool dns_derive_ccode(const string_t& name, string_t& ccode);

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

      bool put_hnode(hnode_t *hnode);

      hnode_t *get_hnode(void);
};

#endif  /* _DNS_RESOLV_H */
