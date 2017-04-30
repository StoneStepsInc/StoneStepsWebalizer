/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   dns_resolv.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sys/stat.h>
#include <errno.h>

/* Need socket header? */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* ensure sys/types */
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifndef _WIN32
#include <netinet/in.h>          /* include stuff we need for dns lookups, */
#include <arpa/inet.h>           /* DB access, file control, etc...        */
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "lang.h"                              /* language declares        */
#include "hashtab.h"                           /* hash table functions     */
#include "dns_resolv.h"                        /* our header               */

#include "mutex.h"
#include "event.h"
#include "thread.h"
#include "util.h"
#include "serialize.h"
#include "tstamp.h"

//
//
//
#define DNS_DB_REC_V1      ((u_int) 1)          // dns_db_record
#define DNS_DB_REC_V2      ((u_int) 2)          // dns_db_record
#define DNS_DB_REC_V3      ((u_int) 3)          // initial version of serialized dnode_t
#define DNS_DB_REC_V4      ((u_int) 4)          // added GeoIP database build time 
#define DNS_DB_REC_VER_MAX ((u_int) 250)        // maximum valid record version

#define DBBUFSIZE          ((size_t) 8192)      // database buffer size

//
// The compatibility DNS DB record structure is saved to the database as a block of 
// underlying memory of sizeof(dns_db_record) size. Consequently, the structure may 
// contain only fundamental data types, cannot contain pointers, must use same data
// representation (e.g. size of time_t) and have the same alignment requirements from 
// a build to a build. It is currently used only to read old DNS records from the 
// database.
//
struct dns_db_record {
   u_int    version;                            // structure version (v1 or v2)
   time_t   tstamp;                             // time when the address was resolved
   char     ccode[2];                           // two-character country code
   char     hostname[1];                        // host name (variable length)
};

//
// DNS DB record
//
struct dns_db_record_t {
   u_int       version;                         // structure version
   char        ccode[hnode_t::ccode_size];      // two-character country code
   bool        spammer;                         // caught spamming?
   tstamp_t    tstamp;                          // time when the address was resolved
   string_t    city;                            // city name
   string_t    hostname;                        // host name
   uint64_t    geoip_tstamp;                    // GeoIP build time

   public:
      dns_db_record_t(void) : version(0), spammer(false), geoip_tstamp(0)
      {
         memset(ccode, 0, hnode_t::ccode_size);
      }

      size_t s_data_size(void) const;
      
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      size_t s_unpack_data(const void *buffer, size_t bufsize);
};

//
// DNS resolver node
//
class dns_resolver_t::dnode_t {
   friend hnode_t *dns_resolver_t::get_hnode(void);

   private:
      hnode_t        *m_hnode;            // modifiable host node

   public:
      const hnode_t  *hnode;              // read-only host node
      class dnode_t  *llist;

      string_t       hostaddr;            // only populated when hnode is NULL

      string_t       hostname;
      string_t       ccode;
      string_t       city;

      bool           spammer;

      uint64_t       geoip_tstamp;

      union {
         sockaddr       s_addr_ip;        // s_addr would be better, but wisock2 defines it as a macro
         sockaddr_in    s_addr_ipv4;      // IPv4 socket address
         sockaddr_in6   s_addr_ipv6;      // IPv6 socket address
      };

   private:
      void remove_host_node(void);

   public:
      dnode_t(hnode_t& hnode, unsigned short sa_family);

      ~dnode_t(void);

      const string_t& key(void) const {return hnode ? hnode->string : hostaddr;}

      bool fill_sockaddr(void);
};

//
// DNS DB record
//

size_t dns_db_record_t::s_data_size(void) const
{
   return s_size_of(version) +
            s_size_of(tstamp) +
            hnode_t::ccode_size + 
            s_size_of(hostname) + 
            s_size_of(spammer) + 
            s_size_of(geoip_tstamp);
}

size_t dns_db_record_t::s_pack_data(void *buffer, size_t bufsize) const
{
   void *ptr = buffer;

   // make sure all data fits in the buffer
   if(s_data_size() > bufsize)
      return 0;

   // serialize all data members
   ptr = serialize(ptr, version);
   ptr = serialize(ptr, tstamp);
   ptr = serialize(ptr, ccode, hnode_t::ccode_size);
   ptr = serialize(ptr, city);
   ptr = serialize(ptr, hostname);
   ptr = serialize(ptr, spammer);
   ptr = serialize(ptr, geoip_tstamp);

   // return the size of the serialized data
   return (char*) ptr - (const char*) buffer;
}

size_t dns_db_record_t::s_unpack_data(const void *buffer, size_t bufsize)
{
   u_int version = 0;
   const void *ptr = buffer;

   if(bufsize < s_size_of(version))
      return 0;

   ptr = deserialize(ptr, version);

   //
   // DNS record version must be checked against the exact version number or against 
   // a small range of version numbers starting with one (e.g. 1-250) because previous
   // versions of the record or records saved by other Webalizer forks will have either 
   // a 32-bit or a 64-bit time_t value as the first data member, which, depending on 
   // the endianness, may be a zero (64-bit time_t, big endian, before 2106) or a large 
   // number (32-bit time_t or 64-bit time_t, little endian, before 2106).
   //
   if(version  == 0 || version > DNS_DB_REC_VER_MAX)
      return 0;

   // DNS record v2 was never released, but may occur in intermediate source builds
   if(version == DNS_DB_REC_V1 || version == DNS_DB_REC_V2) {
      const dns_db_record *dnsrec = (const dns_db_record*)(buffer);

      // make sure we have enough bytes for the entire structure
      if(bufsize < sizeof(dns_db_record))
         return 0;

      //
      // Search from the right end of the buffer for a null character in the host 
      // name, just in case the record was saved with a different alignment or if 
      // any data member had a different size. This way we may report a host name
      // that is invalid, but will not reach past the end of the buffer.
      //
      const char *cp = (const char*) buffer + bufsize - 1;
      while(cp != dnsrec->hostname && *cp) cp--;
      
      // error out if we didn't find a null character
      if(*cp)
         return 0;

      tstamp.reset(dnsrec->tstamp);
      hostname = dnsrec->hostname;

      memcpy(ccode, dnsrec->ccode, hnode_t::ccode_size);

      // reset data members that were not saved in dns_db_record
      city.clear();
      spammer = false;

      //
      // sizeof(dns_db_record) includes one byte for the null character in the 
      // host name and we also need to add the number of bytes in the host name 
      // and account for one more byte that was historically added to the database 
      // record size.
      //
      return sizeof(dns_db_record) + hostname.length() + 1;
   }

   // read the rest of the record from the buffer
   ptr = deserialize(ptr, tstamp);

   ptr = deserialize(ptr, ccode, hnode_t::ccode_size);

   ptr = deserialize(ptr, city);
   ptr = deserialize(ptr, hostname);
   ptr = deserialize(ptr, spammer);

   if(version >= DNS_DB_REC_V4)
      ptr = deserialize(ptr, geoip_tstamp);

   return (const char*) ptr - (char*) buffer;
}

//
// DNS resolver node 
//

dns_resolver_t::dnode_t::dnode_t(hnode_t& hnode, unsigned short sa_family) : 
      hnode(hnode.resolved ? NULL : &hnode), 
      m_hnode(hnode.resolved ? NULL : &hnode), 
      llist(NULL), 
      spammer(hnode.spammer),
      geoip_tstamp(0)
{
   memset(&s_addr_ip, 0, MAX(sizeof(s_addr_ipv4), sizeof(s_addr_ipv6)));

   s_addr_ip.sa_family = sa_family;

   // if the node has been resolved, then it's an update and we copy all values from the host node
   if(hnode.resolved) {
      hostaddr = hnode.string;
      hostname = hnode.name;
      ccode.assign(hnode.ccode, hnode_t::ccode_size);
      city = hnode.city;
   }
}

dns_resolver_t::dnode_t::~dnode_t(void) 
{
}

//
// Converts the IP address string from the host node into a sockaddr of 
// the appropriate type. 
// 
// Note that while we could do this in the constructor, having a separate 
// method with a return value makes this operation more straightforward 
// than handling exceptions thrown from the constructor or having to check 
// some state data member that would indicate whether sockaddr is valid or 
// not.
//
bool dns_resolver_t::dnode_t::fill_sockaddr(void)
{
   // this method may only be called if we have a host node
   if(!hnode)
      return false;

   if(s_addr_ip.sa_family == AF_INET) {
      if(inet_pton(AF_INET, hnode->string, &s_addr_ipv4.sin_addr) != 1)
         return false;
   }
   else if(s_addr_ip.sa_family == AF_INET6) {
      if(inet_pton(AF_INET6, hnode->string, &s_addr_ipv6.sin6_addr) != 1)
         return false;
   }

   return true;
}

void dns_resolver_t::dnode_t::remove_host_node(void)
{
   if(hnode) {
      hostaddr = hnode->string;
      hnode = NULL;
      m_hnode = NULL;
   }
}

//
// DNS resolver
//

dns_resolver_t::dns_resolver_t(const config_t& config) : config(config)
{
   dnode_list = dnode_end = NULL;

   dnode_mutex = NULL;
   hqueue_mutex = NULL;

   dns_done_event = NULL;

   memset(&mmdb, 0, sizeof(MMDB_s));
   geoip_db = NULL;
   dns_db_env = NULL;

   dns_thread_stop = false;

   dns_live_workers = 0;
   dns_unresolved = 0;

   dns_cache_ttl = 0;
   
   accept_host_names = false;

   dns_live_workers = 0;

   dns_unresolved = 0;
   dns_resolved = 0;
   dns_cached = 0;
}

dns_resolver_t::~dns_resolver_t(void)
{
}

/*********************************************/
/* PUT_HNODE - insert/update dns host node   */
/*********************************************/
bool dns_resolver_t::put_hnode(hnode_t *hnode)
{
   unsigned short sa_family;
   dnode_t* nptr;

   if(wrk_ctxs.empty() && !geoip_db)
      return false;

   /* skip bad hostnames */
   if(!hnode || hnode->string.isempty() || hnode->string[0] == ' ') 
      return false;
   
   if(is_ipv4_address(hnode->string))
      sa_family = AF_INET;
   else if(is_ipv6_address(hnode->string))
      sa_family = AF_INET6;
   else
      sa_family = AF_UNSPEC;

   if(sa_family == AF_UNSPEC) {
      if(!accept_host_names) 
         return false;

      //
      // If we are accepting host names in place of IP addresses, derive 
      // country code from the host name right here and return, so the 
      // DNS cache file is not littered with bad IP addresses.
      //
      string_t::char_buffer_t ccode_buffer(hnode->ccode, hnode_t::ccode_size+1, true);
      string_t ccode(ccode_buffer, 0);

      hnode->name = hnode->string;
      dns_derive_ccode(hnode->name, ccode);
      
      return true;
   }
   
   // create a new DNS node
   nptr = new dnode_t(*hnode, sa_family);

   // check if this node should be resolved
   if(nptr->hnode) {
      // convert the IP address string to sockaddr
      if(!nptr->fill_sockaddr()) {
         delete nptr;
         return false;
      }
   }

   // and insert it to the end of the list
   queue_dnode(nptr);

   return true;
}
void dns_resolver_t::queue_dnode(dnode_t *dnode)
{
   mutex_lock(dnode_mutex);
   if(dnode) {
      if(dnode_end == NULL)
         dnode_list = dnode;
      else
         dnode_end->llist = dnode;
      dnode_end = dnode;

      // block the main thread until all queued nodes are processed
      event_reset(dns_done_event);

      // notify resolver threads
      dns_unresolved++;
   }
   mutex_unlock(dnode_mutex);
}

hnode_t *dns_resolver_t::get_hnode(void)
{
   hnode_t *hnode;
   dnode_t *dnode;

   mutex_lock(hqueue_mutex);
   dnode = hqueue.remove();
   mutex_unlock(hqueue_mutex);

   // return if there are no resolved nodes
   if(!dnode)
      return NULL;

   hnode = dnode->m_hnode;

   // indicate that the host node has gone through the DNS resolver
   hnode->resolved = true;

   // copy all resolved dnode_t values into the host node
   hnode->set_ccode(dnode->ccode.c_str());
   hnode->name = dnode->hostname;
   hnode->city = dnode->city;

   //
   // If the spammer flag is the same in both nodes, we are done. However, if neither
   // of the flags was set and if the spammer flag is set in the host node at a later 
   // time, when the node is outside of the DNS resolver, the host must be queued for 
   // an update via put_hnode.
   //
   // If dnode_t has the spammer flag set and the host node doesn't, it means that we 
   // found this address in the DNS database and it was recorded as a spammer. Update 
   // the host node to reflect the same. 
   // 
   // If it's the other way around and the host node has the spammer flag set and
   // the dnode_t doesn't, then we need to update the DNS database to reflect that
   // the host node was caught spamming after it was originally queued for DNS 
   // resolution.
   //
   if(!hnode->spammer && dnode->spammer) {
      hnode->spammer = true;
      delete dnode;
   }
   else if(hnode->spammer && !dnode->spammer) {
      dnode->remove_host_node();
      queue_dnode(dnode);
   }
   else
      delete dnode;

   return hnode;
}

//
// resolve_domain_name
//
// Resolves the IP address in dnode to a domain name for this address.
//
bool dns_resolver_t::resolve_domain_name(dnode_t* dnode)
{
   time_t stime;
   char hostname[NI_MAXHOST];

   if(!dnode || !dnode->hnode)
      return false;

   dnode->hostname.clear();

   // remember the time if we need to report how long it took us to resolve this address later
   if(config.debug_mode)
      stime = time(NULL);

   *hostname = 0;

   if(dnode->s_addr_ip.sa_family == AF_INET) {
      if(getnameinfo(&dnode->s_addr_ip, sizeof(dnode->s_addr_ipv4), hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD))
         goto funcexit;
   }
   else if(dnode->s_addr_ip.sa_family == AF_INET6) {
      if(getnameinfo(&dnode->s_addr_ip, sizeof(dnode->s_addr_ipv6), hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD))
         goto funcexit;
   }
   else
      goto funcexit;

   // make sure it's not empty
   if(!*hostname)
      goto funcexit;

   dnode->hostname = hostname;

funcexit:
   if(config.dns_lookups && config.debug_mode)
      fprintf(stderr, "[%04x] DNS lookup: %s: %s (%.0f sec)\n", thread_id(), dnode->hnode->string.c_str(), dnode->hostname.isempty() ? "NXDOMAIN" : dnode->hostname.c_str(), difftime(time(NULL), stime));

   return !dnode->hostname.isempty();
}

//
//   dns_init
//
bool dns_resolver_t::dns_init(void)
{
   // check if needs to initialize
   if(!config.is_dns_enabled())
      return true;

   dns_cache_ttl = config.dns_cache_ttl;
   
   accept_host_names = config.accept_host_names && config.ntop_ctrys;

#ifdef _WIN32
   WSADATA wsdata;
#endif

#ifdef _WIN32
   if(WSAStartup(MAKEWORD(2, 2), &wsdata) == -1) {
      if(config.verbose)
         fprintf(stderr, "Cannot initialize Windows sockets\n");
      return false;
   }
#endif

   // initialize synchronization primitives
   if((dnode_mutex = mutex_create()) == NULL) {
      if(config.verbose)
         fprintf(stderr, "Cannot initialize DNS mutex\n");
      return false;
   }

   if((hqueue_mutex = mutex_create()) == NULL) {
      if(config.verbose)
         fprintf(stderr, "Cannot initialize the host queue mutex\n");
      return false;
   }

   if((dns_done_event = event_create(true, true)) == NULL) {
      if(config.verbose)
         fprintf(stderr, "Cannot initialize DNS event\n");
      return false;
   }

   // open the DNS cache database
   if(!config.dns_cache.isempty()) {
      dns_db_env = new DbEnv(0);

      //
      // We need a Berkeley DB environment to deal with a threading bug in BDB that 
      // causes some of successful Db::put calls using distinct Db handles in separate
      // threads would lose data before it is written into the database on disk. The 
      // visible effect of this bug was that a small number of IP addresses would go 
      // through DNS resolution again and again when more than one DNS handle was used
      // concurrently. 
      //
      u_int32_t dbenv_flags = DB_CREATE | DB_INIT_LOCK | DB_THREAD | DB_INIT_MPOOL | DB_PRIVATE;

      if(dns_db_env->open(config.dns_db_path, dbenv_flags, 0664)) {
         if (config.verbose) 
            fprintf(stderr,"%s %s\n",lang_t::msg_dns_nodb, config.dns_cache.c_str());
      }
      
      //
      // Open a database handle for each thread to work around a bug in Berkeley DB 
      // threaded Db handles. 
      //
      // Berkeley DB API reference describes that one Db handle can be used by multiple 
      // threads if it is opened with the DB_THREAD flag, which worked for years in this 
      // code while there were DNS calls that took time between BDB calls. With the 
      // introduction of DNSLookups, BDB calls are more frequent and multiple DNS worker 
      // threads started to get blocked indefinitely in the Db handle code on Windows. 
      //
      // This issue can be worked around either by introducing a shared DbEnv with a 
      // DB_INIT_LOCK flag or by using a dedicated Db handle per DNS worker thread. The 
      // latter approach is used here because threaded Db handles serialize calls from 
      // multiple DNS worker threads.
      //
      for(size_t index = 0; index < config.dns_children; index++) {
         // push a new DNS worker context into the vector
         wrk_ctxs.emplace_back(*this);

         // dns_db_open will report errors
         if((wrk_ctxs.back().dns_db = dns_db_open(config.dns_db_fname)) == NULL)
            return false;
      }

      if (config.verbose > 1) {
         /* Using DNS cache file <filaneme> */
         printf("%s %s\n", lang_t::msg_dns_usec, config.dns_cache.c_str());
      }
   }

   // open the GeoIP database
   if(!config.geoip_db_path.isempty()) {
      int mmdb_error;
      if((mmdb_error = MMDB_open(config.geoip_db_path, MMDB_MODE_MMAP, &mmdb)) != MMDB_SUCCESS) {
         if(config.verbose)
            fprintf(stderr, "%s %s (%d - %s)\n", lang_t::msg_dns_geoe, config.geoip_db_path.c_str(), mmdb_error, MMDB_strerror(mmdb_error));
      }
      else {
         if (config.verbose > 1) 
            printf("%s %s\n", lang_t::msg_dns_useg, config.geoip_db_path.c_str());

         geoip_db = &mmdb;

         //
         // Find out if the selected language is in the list of languages provided in
         // the GeoIP database. If we didn't find one, but there's English available, 
         // use it.
         //
         for(size_t i = 0; i < mmdb.metadata.languages.count; i++) {
            if(!string_t::compare_ci(mmdb.metadata.languages.names[i], config.lang.language_code, 2)) {
               geoip_language.assign(config.lang.language_code, 2);
               break;
            }
            else if(geoip_language.isempty() && !string_t::compare_ci(mmdb.metadata.languages.names[i], "en", 2))
               geoip_language.assign("en", 2);
         }
      }

   }

   // get the current time once to avoid doing it for every host
   runtime.reset(time(NULL));

   if(!wrk_ctxs.empty() || geoip_db)
      dns_create_worker_threads();

   if (config.verbose > 1) {
      /* DNS Lookup (#children): */
      printf("%s (%d)\n",lang_t::msg_dns_rslv, config.dns_children);
   }

   return (wrk_ctxs.size() || geoip_db) ? true : false;
}

//
//   dns_clean_up
//
void dns_resolver_t::dns_clean_up(void)
{
   u_int index;
   u_int waitcnt = 300;

   dns_thread_stop = true;

   if(!config.is_dns_enabled())
      return;

   // wait for 15 seconds for DNS workers to stop
   while(get_live_workers() && waitcnt--)
      msleep(50);

   if(!wrk_ctxs.empty()) {
      // free all resources (buffers are deleted in the thread)
      for(index = 0; index < wrk_ctxs.size(); index++) { 
         thread_destroy(workers[index]);
         dns_db_close(wrk_ctxs[index].dns_db);
      }
   }

   // delete the BDB environment, if we have one
   if(dns_db_env) {
      dns_db_env->close(0);
      delete dns_db_env;
      dns_db_env = NULL;
   }

   // don't leave dangling pointers behind
   workers.clear();
   wrk_ctxs.clear();

   // close the GeoIP database and clean-up its structures
   if(geoip_db) {
      MMDB_close(geoip_db);
      geoip_db = NULL;
      memset(&mmdb, 0, sizeof(MMDB_s));
   }

   event_destroy(dns_done_event);
   mutex_destroy(dnode_mutex);
   mutex_destroy(hqueue_mutex);

#ifdef _WIN32
      WSACleanup();
#endif

}

void dns_resolver_t::dns_wait(void)
{
   bool done = false;

   // if there are no workers, return right away
   if(!config.is_dns_enabled())
      return;

   // otherwise wait for all of them to finish
   if(event_wait(dns_done_event, (uint32_t) -1) == EVENT_OK)
      return;

   if(config.verbose)
      fprintf(stderr, "DNS event wait operation has failed - using polling to wait\n");

   while(!done) {
      mutex_lock(dnode_mutex);
      done = dns_unresolved == 0 ? true : false;
      mutex_unlock(dnode_mutex);
      msleep(500);
   } 
}

//
// process_node
//
// Picks the next available IP address to resolve and calls resolve_domain_name.
// Returns true if any work was done (even unsuccessful), false otherwise.
//
bool dns_resolver_t::process_node(Db *dns_db, void *buffer, size_t bufsize)
{
   bool cached = false, lookup = false;
   dnode_t* nptr;

   mutex_lock(dnode_mutex);

   if(dnode_list == NULL)
      goto funcidle;

   // check the node at the start of the queue
   if((nptr = dnode_list) == NULL)
      goto funcidle;

   // remove the node and set dnode_end to NULL if there no more nodes
   if((dnode_list = dnode_list->llist) == NULL)
      dnode_end = NULL;

   // reset the pointer to the next node
   nptr->llist = NULL;

   mutex_unlock(dnode_mutex);

   // check if we just need to update a DNS record
   if(!nptr->hnode)
      dns_db_put(nptr, dns_db, buffer, bufsize);
   else {
      // if we have a host node, look it up in the database and if not found, resolve it
      lookup = true;
      if(dns_db_get(nptr, dns_db, false, buffer, bufsize)) 
         cached = true;

      //
      // Resolve the address if it's not cached and/or look up the country code if it's 
      // empty and the GeoIP database is newer than the one we used when we saved the 
      // address in the DNS database. 
      //
      if(!cached || geoip_db && nptr->ccode.isempty() && geoip_db->metadata.build_epoch > nptr->geoip_tstamp) {
         bool goodcc = false;
         
         // look up country code in the GeoIP database if there is a GeoIP database
         if(geoip_db)
            goodcc = geoip_get_ccode(nptr->hnode->string, nptr->s_addr_ip, nptr->ccode, nptr->city);

         // resolve the IP address if requested and not in the database already
         if(!cached && config.dns_lookups) {
            if(resolve_domain_name(nptr) && !goodcc) {
               // if GeoIP failed, derive country code from the domain name
               dns_derive_ccode(nptr->hostname, nptr->ccode);
            }
         }

         // update the database if it's a new IP address or if we found a country code for an existing one
         if(!cached || goodcc)
            dns_db_put(nptr, dns_db, buffer, bufsize);
      }
   }

   mutex_lock(dnode_mutex);

   // update resolver stats
   if(lookup) {
      if(cached) 
         dns_cached++; 
      else 
         dns_resolved++;
   }

   if(nptr->hnode) {
      // add the node to the queue of resolved nodes
      mutex_lock(hqueue_mutex);
      hqueue.add(nptr);
      mutex_unlock(hqueue_mutex);
   }
   else {
      // if we just updated the DNS record, delete dnode_t
      delete nptr;
   }

   // if there are no more unresolved addresses, signal the event
   if(--dns_unresolved == 0)
      event_set(dns_done_event);

   mutex_unlock(dnode_mutex);

   return true;

funcidle:
   mutex_unlock(dnode_mutex);
   return false;
}

void dns_resolver_t::inc_live_workers(void)
{
   mutex_lock(dnode_mutex);
   dns_live_workers++;
   mutex_unlock(dnode_mutex);
}

void dns_resolver_t::dec_live_workers(void)
{
   mutex_lock(dnode_mutex);
   dns_live_workers--;
   mutex_unlock(dnode_mutex);
}

int dns_resolver_t::get_live_workers(void)
{
   int retval;
   mutex_lock(dnode_mutex);
   retval = dns_live_workers;
   mutex_unlock(dnode_mutex);
   return retval;
}

//
// Domain name resolved worker thread function
//
#ifdef _WIN32
unsigned int __stdcall dns_resolver_t::dns_worker_thread_proc(void *arg)
#else
void *dns_resolver_t::dns_worker_thread_proc(void *arg)
#endif
{
   dns_resolver_t::wrk_ctx_t *wrk_ctx = (dns_resolver_t::wrk_ctx_t*) arg;
   wrk_ctx->dns_resolver.dns_worker_thread_proc(*wrk_ctx);
   return 0;
}

void dns_resolver_t::dns_worker_thread_proc(wrk_ctx_t& wrk_ctx)
{
   wrk_ctx.buffer = new u_char[DBBUFSIZE];
   wrk_ctx.bufsize = DBBUFSIZE;

   while(!dns_thread_stop) {
      if(process_node(wrk_ctx.dns_db, wrk_ctx.buffer, wrk_ctx.bufsize) == false)
         msleep(200);
   }

   delete [] wrk_ctx.buffer;
   wrk_ctx.buffer = NULL;

   dec_live_workers();
}

//
// dns_create_worker_threads
//
// Creates DNS resolver worker threads. 
//
void dns_resolver_t::dns_create_worker_threads(void)
{
   // create as many worker threads as we have worker contexts
   for(size_t index = 0; index < wrk_ctxs.size(); index++) {
      inc_live_workers();
      workers.push_back(thread_create(dns_worker_thread_proc, &wrk_ctxs[index]));
   }
}

bool dns_resolver_t::geoip_get_ccode(const string_t& hostaddr, const sockaddr& ipaddr, string_t& ccode, string_t& city)
{
   static const char *ccode_path[] = {"country", "iso_code", NULL};
   const char *city_path[] = {"city", "names", geoip_language.c_str(), NULL};

   int mmdb_error;
   MMDB_lookup_result_s result;
   MMDB_entry_data_s entry_data;

   if(!geoip_db)
      throw std::runtime_error("GeoIP database is not open");

   ccode.reset();
   city.reset();

   // look up the IP address
   result = MMDB_lookup_sockaddr(geoip_db, &ipaddr, &mmdb_error);

   if(mmdb_error != MMDB_SUCCESS) {
      if(config.debug_mode)
         fprintf(stderr, "Cannot lookup IP address %s (%d - %s)\n", hostaddr.c_str(), mmdb_error, MMDB_strerror(mmdb_error));
      return false;
   }
   
   if(!result.found_entry) {
      if(config.debug_mode)
         fprintf(stderr, "Cannot find IP address %s\n", hostaddr.c_str());
      return false;
   }

   // get the country code first and make sure it fits the storage in hnode_t
   if(MMDB_aget_value(&result.entry, &entry_data, ccode_path) == MMDB_SUCCESS) {
      if(entry_data.has_data) {
         if(entry_data.data_size != 2)
            return false;

         ccode.assign(entry_data.utf8_string, entry_data.data_size);
         ccode.tolower();
      }
   }

   // get the city if requested and is available either in the selected language or English
   if(config.geoip_city && !geoip_language.isempty()) {
      if(MMDB_aget_value(&result.entry, &entry_data, city_path) == MMDB_SUCCESS) {
         if(entry_data.has_data)
            city.assign(entry_data.utf8_string, entry_data.data_size);
      }
   }

   //
   // Some IP addresses may be found in the database, but do not have a country code. An 
   // example of this are addresses that have no designated country and there is no value 
   // in country/iso_code, but other fields, such as continent/code, have values, so the 
   // look-up succeeds. Return true only if we found a country code for this IP address.
   //
   return !ccode.isempty();
}

//
// derives country code from the domain name
//
bool dns_resolver_t::dns_derive_ccode(const string_t& name, string_t& ccode)
{
   const char *label;

   ccode.reset();

   if(name.isempty())
      return false;

   for(label = &name[name.length()-1]; *label != '.' && label != name.c_str(); label--);

   if(*label != '.')
      return false;

   ccode = ++label;

   return true;
}

//
// dns_db_get
//
// Looks up the dnode->ipaddr in the database. If nocheck is true, the 
// function will not check whether the entry is stale or not (may be 
// used for subsequent searches to avoid unnecessary DNS lookups).
//
bool dns_resolver_t::dns_db_get(dnode_t* dnode, Db *dns_db, bool nocheck, void *buffer, size_t bufsize)
{
   bool retval = false;
   int dberror;
   Dbt key, recdata;
   dns_db_record_t dnsrec;

   /* ensure we have a dns db */
   if (!dns_db || !dnode) 
      return false;

   memset(&key, 0, sizeof(key));
   memset(&recdata, 0, sizeof(recdata));

   key.set_data((void*) dnode->hnode->string.c_str());
   key.set_size((u_int32_t) dnode->hnode->string.length());

   // point the record to the internal buffer
   recdata.set_flags(DB_DBT_USERMEM);
   recdata.set_ulen((u_int32_t) bufsize);
   recdata.set_data(buffer);

   if (config.debug_mode) fprintf(stderr,"[%04x] Checking DNS cache for %s...\n", thread_id(), dnode->hnode->string.c_str());

   switch((dberror = dns_db->get(NULL, &key, &recdata, 0)))
   {
      case  DB_NOTFOUND: 
         if (config.debug_mode) 
            fprintf(stderr,"[%04x] ... not found\n", thread_id());
         break;
      case  0:
         if(dnsrec.s_unpack_data(recdata.get_data(), recdata.get_size()) == recdata.get_size()) {
            if(nocheck || runtime.elapsed(dnsrec.tstamp) <= dns_cache_ttl) {
               dnode->hostname = dnsrec.hostname;
               dnode->ccode.assign(dnsrec.ccode, hnode_t::ccode_size);
               dnode->city = dnsrec.city;
               dnode->spammer = dnsrec.spammer;
               dnode->geoip_tstamp = dnsrec.geoip_tstamp;
               retval = true;
            }

            if (retval && config.debug_mode)
               fprintf(stderr,"[%04x] ... found: %s (age: %0.2f days)\n", thread_id(), dnode->hostname.isempty() ? "NXDOMAIN" : dnode->hostname.c_str(), runtime.elapsed(dnsrec.tstamp) / 86400.);
         }

         break;

      default: 
         if (config.debug_mode) 
            fprintf(stderr," error (%04x - %s)\n", dberror, db_strerror(dberror));
         break;
   }

   return retval;
}

/*********************************************/
/* DB_PUT - put key/val in the cache db      */
/*********************************************/

void dns_resolver_t::dns_db_put(const dnode_t* dnode, Db *dns_db, void *buffer, size_t bufsize)
{
   Dbt k, v;
   size_t recSize;
   int dberror;
   dns_db_record_t dnsrec;

   if(!dns_db || !dnode)
      return;

   dnsrec.version = DNS_DB_REC_V4;
   dnsrec.tstamp = runtime;

   // record contents must come from dnode_t
   dnsrec.city = dnode->city;
   dnsrec.hostname = dnode->hostname;
   dnsrec.spammer = dnode->spammer;

   // if we have a GeoIP database, hold onto its build time
   if(geoip_db)
      dnsrec.geoip_tstamp = geoip_db->metadata.build_epoch;

   memcpy(dnsrec.ccode, dnode->ccode.c_str(), hnode_t::ccode_size);

   recSize = dnsrec.s_pack_data(buffer, bufsize);

   if(recSize == 0)
      return;

   k.set_data((void*) dnode->key().c_str());
   k.set_size((u_int32_t) dnode->key().length());

   v.set_data(buffer);
   v.set_size((u_int32_t) recSize);

   if((dberror = dns_db->put(NULL, &k, &v, 0)) < 0) {
      if(config.verbose)
         fprintf(stderr,"dns_db_put failed (%04x - %s)!\n", dberror, db_strerror(dberror));
   }
}

Db *dns_resolver_t::dns_db_open(const string_t& dns_cache)
{
   struct stat  dbStat;
   int major, minor, patch;
   Db *dns_db = NULL;

   /* double check filename was specified */
   if(dns_cache.isempty())
      return NULL; 

   db_version(&major, &minor, &patch);

   if(major < 4 && minor < 3) {
      if(config.verbose)
         fprintf(stderr, "Error: The Berkeley DB library must be v4.3 or newer (found v%d.%d.%d).\n", major, minor, patch);
      return NULL;
   }

   /* minimal sanity check on it */
   if(stat(dns_cache, &dbStat) < 0) {
      if(errno != ENOENT) 
         return NULL;
   }
   else {
      if(!dbStat.st_size)  /* bogus file, probably from a crash */
      {
         unlink(dns_cache);  /* remove it so we can recreate... */
      }
   }
  
   dns_db = new Db(dns_db_env, 0);

   /* open cache file */
   if(dns_db->open(NULL, dns_cache, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0664))
   {
      /* Error: Unable to open DNS cache file <filename> */
      if (config.verbose) fprintf(stderr,"%s %s\n",lang_t::msg_dns_nodb, dns_cache.c_str());

      delete dns_db;
      dns_db = NULL;

      return NULL;
   }

   return dns_db;
}

void dns_resolver_t::dns_db_close(Db *dns_db)
{
   if(dns_db) {
      dns_db->close(0);

      delete dns_db;
   }
}

//
//
//
#include "queue_tmpl.cpp"

//
//
//
template class queue_t<dns_resolver_t::dnode_t>;
