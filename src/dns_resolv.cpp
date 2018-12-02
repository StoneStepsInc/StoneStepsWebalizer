/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   dns_resolv.cpp
*/
#include "pch.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>          /* include stuff we need for dns lookups, */
#include <arpa/inet.h>           /* DB access, file control, etc...        */
#include <fcntl.h>
#include <unistd.h>
#endif

#include "lang.h"                              /* language declares        */
#include "hashtab.h"                           /* hash table functions     */
#include "dns_resolv.h"                        /* our header               */

#include "event.h"
#include "thread.h"
#include "util_ipaddr.h"
#include "serialize.h"
#include "tstamp.h"

extern "C" {
#include <maxminddb.h>
}

// winsock2.h and maxminddb.h define max as a macro, which conflicts with std::max
#undef max

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <sys/stat.h>

//
//
//
#define DNS_DB_REC_V1      ((u_int) 1)          // dns_db_record
#define DNS_DB_REC_V2      ((u_int) 2)          // dns_db_record
#define DNS_DB_REC_V3      ((u_int) 3)          // initial version of serialized dnode_t
#define DNS_DB_REC_V4      ((u_int) 4)          // added GeoIP database build time 
#define DNS_DB_REC_V5      ((u_int) 5)          // added GeoIP latitude and longitude
#define DNS_DB_REC_V6      ((u_int) 6)          // added geoname_id
#define DNS_DB_REC_VER_MAX ((u_int) 250)        // maximum valid record version

#define DBBUFSIZE          ((size_t) 8192)      // database buffer size

///
/// @struct dns_db_record
///
/// @brief  A compatibility DNS DB record
///
/// The compatibility DNS DB record structure is saved to the database as a block of 
/// underlying memory of sizeof(dns_db_record) size. Consequently, the structure may 
/// contain only fundamental data types, cannot contain pointers, must use same data
/// representation (e.g. size of time_t) and have the same alignment requirements from 
/// a build to a build. It is currently used only to read old DNS records from the 
/// database.
///
struct dns_db_record {
   u_int    version;                            // structure version (v1 or v2)
   time_t   tstamp;                             // time when the address was resolved
   char     ccode[2];                           // two-character country code
   char     hostname[1];                        // host name (variable length)
};

///
/// @struct dns_db_record_t
///
/// @brief  A new serializable DNS DB record
///
struct dns_db_record_t {
   u_int       version;                         // structure version
   char        ccode[hnode_t::ccode_size];      // two-character country code
   bool        spammer;                         // caught spamming?
   tstamp_t    tstamp;                          // time when the address was resolved
   string_t    city;                            // city name
   string_t    hostname;                        // host name
   uint64_t    geoip_tstamp;                    // GeoIP build time
   double      latitude;                        // IP address latitude
   double      longitude;                       // IP address longitude
   uint32_t    geoname_id;                      // The geoname_id from the GeoIP database

   public:
      dns_db_record_t(void) : version(0), spammer(false), geoip_tstamp(0), latitude(0.), longitude(0.), geoname_id(0)
      {
         memset(ccode, 0, hnode_t::ccode_size);
      }

      size_t s_data_size(void) const;
      
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      size_t s_unpack_data(const void *buffer, size_t bufsize);
};

///
/// @class  dns_resolver_t::dnode_t
///
/// @brief  An internal DNS resolver node associated with each host node being processed
///
/// DNS resolver cannot modify const host nodes in the context of its own threads and 
/// uses dnode_t instances to carry new and updated information until get_hnode is called, 
/// at which point hnode_t may be modified in the context of the application thread that 
/// queued the host node for resolution.
///
class dns_resolver_t::dnode_t {
   // allow get_hnode to get a modifiable pointer to hnode_t
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

      double         latitude;
      double         longitude;

      uint32_t       geoname_id;

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
            s_size_of(geoip_tstamp) + 
            s_size_of(latitude) + 
            s_size_of(longitude) + 
            s_size_of(geoname_id);
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
   ptr = serialize(ptr, latitude);
   ptr = serialize(ptr, longitude);
   ptr = serialize(ptr, geoname_id);

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

   if(version >= DNS_DB_REC_V5) {
      ptr = deserialize(ptr, latitude);
      ptr = deserialize(ptr, longitude);
   }

   if(version >= DNS_DB_REC_V6)
      ptr = deserialize(ptr, geoname_id);

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
      geoip_tstamp(0),
      latitude(0.),
      longitude(0.),
      geoname_id(0)
{
   memset(&s_addr_ip, 0, std::max(sizeof(s_addr_ipv4), sizeof(s_addr_ipv6)));

   s_addr_ip.sa_family = sa_family;

   // if the node has been resolved, then it's an update and we copy all values from the host node
   if(hnode.resolved) {
      hostaddr = hnode.string;
      hostname = hnode.name;
      ccode.assign(hnode.ccode, hnode_t::ccode_size);
      city = hnode.city;
      geoname_id = hnode.geoname_id;
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

   dns_done_event = NULL;

   geoip_db = new MMDB_s();

   dns_db_env = NULL;

   dns_thread_stop = false;

   dns_live_workers = 0;

   dns_cache_ttl = 0;
   
   accept_host_names = false;

   dns_live_workers = 0;

   dns_unresolved = 0;
   dns_resolved = 0;
   dns_cached = 0;
}

dns_resolver_t::~dns_resolver_t(void)
{
   //
   // If the dns_init failed, DNS resolver may be partially initialized and there 
   // will be no active worker threads. Clean up all resources that may have been 
   // allocated in this case (e.g. worker thread buffers will not be allocated). 
   // This code will not run if dns_clean_up was called.
   //
   if(!wrk_ctxs.empty() && !get_live_workers()) {
      for(size_t index = 0; index < wrk_ctxs.size(); index++) {
         if(wrk_ctxs[index].dns_db)
            dns_db_close(wrk_ctxs[index].dns_db);
      }

      wrk_ctxs.clear();

      if(dns_db_env) {
         dns_db_env->close(0);
         delete dns_db_env;
      }

      if(geoip_db) {
         MMDB_close(geoip_db);
         delete geoip_db;
      }

      event_destroy(dns_done_event);
   }
}

///
/// @brief   Queues the host node for a DNS database look-up
///
bool dns_resolver_t::put_hnode(hnode_t *hnode)
{
   unsigned short sa_family;
   dnode_t* nptr;

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
   
   // need workers to resolve IP addresses or look them up in the GeoIP database
   if(wrk_ctxs.empty())
      return false;

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
   dnode_mutex.lock();
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
   dnode_mutex.unlock();
}

///
/// @brief   Retrieves a resolved host node after a DNS database look-up
///
hnode_t *dns_resolver_t::get_hnode(void)
{
   hnode_t *hnode;
   dnode_t *dnode;

   hqueue_mutex.lock();
   dnode = hqueue.remove();
   hqueue_mutex.unlock();

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
   hnode->latitude = dnode->latitude;
   hnode->longitude = dnode->longitude;
   hnode->geoname_id = dnode->geoname_id;

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

///
/// @brief  resolve_domain_name
///
/// Resolves the IP address in dnode to a domain name for this address.
///
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

///
/// @brief   Initializes the DNS resolver
///
/// This method reports all errors to the standard error stream and returns `false` if 
/// the DNS resolver could not be initialized.
///
bool dns_resolver_t::dns_init(void)
{
   // dns_init shouldn't be called if there are no workers configured
   if(!config.dns_children) {
      fprintf(stderr, "The number of DNS workers cannot be zero");
      return false;
   }

   dns_cache_ttl = config.dns_cache_ttl;
   
   accept_host_names = config.accept_host_names && config.ntop_ctrys;

#ifdef _WIN32
   WSADATA wsdata;

   if(WSAStartup(MAKEWORD(2, 2), &wsdata) == -1) {
      fprintf(stderr, "Cannot initialize Windows sockets\n");
      return false;
   }
#endif

   // initialize synchronization primitives
   if((dns_done_event = event_create(true, true)) == NULL) {
      fprintf(stderr, "Cannot initialize DNS event\n");
      return false;
   }

   // initialize a context for each worker thread
   for(size_t index = 0; index < config.dns_children; index++)
      wrk_ctxs.emplace_back(*this);

   // open the DNS cache database
   if(!config.dns_cache.isempty()) {
      dns_db_env = new DbEnv(0);

      //
      // Initialize Berkeley DB for concurrent access (DB_INIT_CDB), which allows multiple
      // concurrent readers and a single writer. Note that DB_INIT_LOCK alone does not 
      // protect against deadlocks that may occur during some operations like page splits.
      //
      u_int32_t dbenv_flags = DB_CREATE | DB_INIT_CDB | DB_THREAD | DB_INIT_MPOOL | DB_PRIVATE;

      if(dns_db_env->open(config.dns_db_path, dbenv_flags, 0664)) {
         fprintf(stderr,"%s %s\n",config.lang.msg_dns_nodb, config.dns_cache.c_str());
         return false;
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
      for(size_t index = 0; index < wrk_ctxs.size(); index++) {
         // dns_db_open will report errors
         if((wrk_ctxs[index].dns_db = dns_db_open(config.dns_db_fname)) == NULL)
            return false;
      }

      if (config.verbose > 1) {
         /* Using DNS cache file <filaneme> */
         printf("%s %s\n", config.lang.msg_dns_usec, config.dns_cache.c_str());
      }
   }

   // open the GeoIP database
   if(!config.geoip_db_path.isempty()) {
      int mmdb_error;
      if((mmdb_error = MMDB_open(config.geoip_db_path, MMDB_MODE_MMAP, geoip_db)) != MMDB_SUCCESS) {
         fprintf(stderr, "%s %s (%d - %s)\n", config.lang.msg_dns_geoe, config.geoip_db_path.c_str(), mmdb_error, MMDB_strerror(mmdb_error));
         return false;
      }

      if (config.verbose > 1) 
         printf("%s %s\n", config.lang.msg_dns_useg, config.geoip_db_path.c_str());

      //
      // Find out if there is a suitable language in the list of languages provided in the 
      // GeoIP database. Stop looking if we found an exact match (e.g. pt-br or en). If a 
      // partial match is found (e.g. zh in zh-cn), hold onto the GeoIP language code and 
      // keep looking. Every subsequent partial match is taken only if it's shorter than 
      // the previous match. In other words, the first of equal-length matches (e.g. en-us 
      // out of en-us, en-uk) or the first shorter partial match (e.g. en out of en-us, en, 
      // en-uk) wins. If we didn't find a matching language, but there is English available, 
      // use it.
      //
      for(size_t i = 0; i < geoip_db->metadata.languages.count; i++) {
         if(lang_t::check_language(geoip_db->metadata.languages.names[i], config.lang.language_code)) {
            geoip_language.assign(geoip_db->metadata.languages.names[i]);
            break;
         }
         else if(lang_t::check_language(geoip_db->metadata.languages.names[i], config.lang.language_code, 2) &&
                  (!lang_t::check_language(geoip_language, config.lang.language_code, 2) || strlen(geoip_db->metadata.languages.names[i]) < geoip_language.length()))
            geoip_language.assign(geoip_db->metadata.languages.names[i]);
         else if(geoip_language.isempty() && lang_t::check_language(geoip_db->metadata.languages.names[i], "en"))
            geoip_language.assign("en", 2);
      }
   }

   // get the current time once to avoid doing it for every host
   runtime.reset(time(NULL));

   // create worker threads to handle DNS and GeoIP requests
   for(size_t index = 0; index < wrk_ctxs.size(); index++) {
      inc_live_workers();

      //
      // Pass the worker context as a pointer because VC++ 2015 has a bug that 
      // fails to compile a non-const reference passed to a thread when language
      // extensinos are disabled (/Za).
      //
      workers.emplace_back(&dns_resolver_t::dns_worker_thread_proc, this, &wrk_ctxs[index]);
   }

   if (config.verbose > 1) {
      /* DNS Lookup (#children): */
      printf("%s (%d)\n",config.lang.msg_dns_rslv, config.dns_children);
   }

   return true;
}

///
/// @brief  Cleans up the DNS resolver instance when it's no longer needed
///
void dns_resolver_t::dns_clean_up(void)
{
   u_int index;

   // make sure the DNS resolver is initialized
   if(workers.empty())
      throw std::runtime_error("DNS resolver is not initialized");

   // make sure there are no worker threads running
   if(get_live_workers())
      dns_abort();

   if(!wrk_ctxs.empty()) {
      // free all resources (buffers are deleted in the thread)
      for(index = 0; index < wrk_ctxs.size(); index++) { 
         workers[index].join();
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
      delete geoip_db;
      geoip_db = NULL;
   }

   event_destroy(dns_done_event);

   // if DNS resolution was aborted, there will be unresolved DNS nodes in the list
   while(dnode_list) {
      dnode_t *dnode = dnode_list;
      dnode_list = dnode_list->llist;
      delete dnode;
   }

   // if there are any leftover resolved addresses, delete them
   while(hqueue.top())
      delete hqueue.remove();

#ifdef _WIN32
      WSACleanup();
#endif

}

void dns_resolver_t::dns_wait(void)
{
   bool done = false;

   // make sure the DNS resolver is initialized
   if(workers.empty())
      throw std::runtime_error("DNS resolver is not initialized");

   // otherwise wait for all of them to finish
   if(event_wait(dns_done_event, (uint32_t) -1) == EVENT_OK)
      return;

   if(config.verbose)
      fprintf(stderr, "DNS event wait operation has failed - using polling to wait\n");

   while(!done) {
      dnode_mutex.lock();
      done = dns_unresolved == 0 ? true : false;
      dnode_mutex.unlock();
      msleep(500);
   } 
}

///
/// @brief  Interrupts all DNS resolver activities
///
/// Requests all DNS worker threads to stop and waits for a few seconds for all of them 
/// to do so. If any of the threads failed to stop, an exception is thrown. Otherwise, 
/// the DNS-done event is set to allow dns_wait to succeed, so resolved addresses can be 
/// processed by the caller. 
///
void dns_resolver_t::dns_abort(void)
{
   u_int waitcnt = 300;

   // make sure the DNS resolver is initialized
   if(workers.empty())
      throw std::runtime_error("DNS resolver is not initialized");

   dns_thread_stop = true;

   // wait for 15 seconds for DNS workers to stop
   while(get_live_workers() && waitcnt--)
      msleep(50);

   // cannot clean up if worker threads are still running
   if(get_live_workers())
      throw std::runtime_error("Cannot stop DNS worker threads");

   // set the event, so dns_wait can return and nodes in hqueue can be processed
   event_set(dns_done_event);
}

///
/// @brief   Uses enabled DNS resolver components to process a dnode_t instance 
///
/// @return `true` if any work was done, even unsuccessful, `false` otherwise
///
/// Picks up the next dnode_t instance from the ueue, looks it up in the DNS resolver
/// database and, if not found, looks up its GeoIP informatin and attempts to resolve
/// the IP address via DNS, if either of activities is enabled. 
///
bool dns_resolver_t::process_node(Db *dns_db, void *buffer, size_t bufsize)
{
   bool cached = false, lookup = false;
   dnode_t* nptr;

   dnode_mutex.lock();

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

   dnode_mutex.unlock();

   // check if we just need to update a DNS record
   if(!nptr->hnode)
      dns_db_put(nptr, dns_db, buffer, bufsize);
   else {
      // if we have a host node, look it up in the database and if not found, resolve it
      lookup = true;
      if(dns_db && dns_db_get(nptr, dns_db, buffer, bufsize)) 
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
            goodcc = geoip_get_ccode(nptr->hnode->string, nptr->s_addr_ip, nptr->ccode, nptr->city, nptr->latitude, nptr->longitude, nptr->geoname_id);

         // resolve the IP address if requested and not in the database already
         if(dns_db && !cached && config.dns_lookups) {
            if(resolve_domain_name(nptr) && !goodcc) {
               // if GeoIP failed, derive country code from the domain name
               dns_derive_ccode(nptr->hostname, nptr->ccode);
            }
         }

         // update the database if it's a new IP address or if we found a country code for an existing one
         if(dns_db && (!cached || goodcc))
            dns_db_put(nptr, dns_db, buffer, bufsize);
      }
   }

   dnode_mutex.lock();

   // update resolver stats
   if(lookup) {
      if(cached) 
         dns_cached++; 
      else 
         dns_resolved++;
   }

   if(nptr->hnode) {
      // add the node to the queue of resolved nodes
      hqueue_mutex.lock();
      hqueue.add(nptr);
      hqueue_mutex.unlock();
   }
   else {
      // if we just updated the DNS record, delete dnode_t
      delete nptr;
   }

   // if there are no more unresolved addresses, signal the event
   if(--dns_unresolved == 0)
      event_set(dns_done_event);

   dnode_mutex.unlock();

   return true;

funcidle:
   dnode_mutex.unlock();
   return false;
}

void dns_resolver_t::inc_live_workers(void)
{
   dnode_mutex.lock();
   dns_live_workers++;
   dnode_mutex.unlock();
}

void dns_resolver_t::dec_live_workers(void)
{
   dnode_mutex.lock();
   dns_live_workers--;
   dnode_mutex.unlock();
}

int dns_resolver_t::get_live_workers(void)
{
   int retval;
   dnode_mutex.lock();
   retval = dns_live_workers;
   dnode_mutex.unlock();
   return retval;
}

///
/// @brief  Domain name resolved worker thread function
///
void dns_resolver_t::dns_worker_thread_proc(wrk_ctx_t *wrk_ctx_ptr)
{
   wrk_ctx_t& wrk_ctx = *wrk_ctx_ptr;
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

bool dns_resolver_t::geoip_get_ccode(const string_t& hostaddr, const sockaddr& ipaddr, string_t& ccode, string_t& city, double& latitude, double& longitude, uint32_t& geoname_id)
{
   static const char *ccode_path[] = {"country", "iso_code", NULL};
   static const char *loc_lat_path[] = {"location", "latitude", NULL};
   static const char *loc_lon_path[] = {"location", "longitude", NULL};
   static const char *city_geoname_id[] = {"city", "geoname_id", nullptr};
   const char *city_path[] = {"city", "names", geoip_language.c_str(), NULL};

   int mmdb_error;
   MMDB_lookup_result_s result = {};
   MMDB_entry_data_s entry_data = {};

   if(!geoip_db)
      throw std::runtime_error("GeoIP database is not open");

   ccode.reset();
   city.reset();

   latitude = longitude = 0.;

   geoname_id = 0;

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
   if(config.geoip_city) {
      if(!geoip_language.isempty()) {
         if(MMDB_aget_value(&result.entry, &entry_data, city_path) == MMDB_SUCCESS) {
            if(entry_data.has_data)
               city.assign(entry_data.utf8_string, entry_data.data_size);
         }
      }

      // get the geoname identifier for this city
      if(MMDB_aget_value(&result.entry, &entry_data, city_geoname_id) == MMDB_SUCCESS) {
         if(entry_data.has_data)
            geoname_id = entry_data.uint32;
      }

      // check if we have the coordinates in the entry
      if(MMDB_aget_value(&result.entry, &entry_data, loc_lat_path) == MMDB_SUCCESS) {
         if(entry_data.has_data) {
            // hold onto the latitude until we are sure we have the longitude
            double lat = entry_data.double_value;

            if(MMDB_aget_value(&result.entry, &entry_data, loc_lon_path) == MMDB_SUCCESS) {
               if(entry_data.has_data) {
                  longitude = entry_data.double_value;
                  latitude = lat;
               }
            }
         }
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

///
/// @brief  Derives country code from the domain name
///
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

///
/// @brief  Looks up the IP address in the DNS resolver database
///
bool dns_resolver_t::dns_db_get(dnode_t* dnode, Db *dns_db, void *buffer, size_t bufsize)
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
            if(runtime.elapsed(dnsrec.tstamp) <= dns_cache_ttl) {
               dnode->hostname = dnsrec.hostname;
               dnode->ccode.assign(dnsrec.ccode, hnode_t::ccode_size);
               dnode->city = dnsrec.city;
               dnode->spammer = dnsrec.spammer;
               dnode->geoip_tstamp = dnsrec.geoip_tstamp;
               dnode->latitude = dnsrec.latitude;
               dnode->longitude = dnsrec.longitude;
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

///
/// @brief  Stores a dnode_t instance in the DNS resolver database
///
void dns_resolver_t::dns_db_put(const dnode_t* dnode, Db *dns_db, void *buffer, size_t bufsize)
{
   Dbt k, v;
   size_t recSize;
   int dberror;
   dns_db_record_t dnsrec;

   if(!dns_db || !dnode)
      return;

   dnsrec.version = DNS_DB_REC_V5;
   dnsrec.tstamp = runtime;

   // record contents must come from dnode_t
   dnsrec.city = dnode->city;
   dnsrec.hostname = dnode->hostname;
   dnsrec.spammer = dnode->spammer;

   // if we have a GeoIP database, hold onto its build time
   if(geoip_db)
      dnsrec.geoip_tstamp = geoip_db->metadata.build_epoch;

   dnsrec.latitude = dnode->latitude;
   dnsrec.longitude = dnode->longitude;

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

   if(major < 4 || major == 4 && minor < 4) {
      fprintf(stderr, "Berkeley DB must be v4.4 or newer (found v%d.%d.%d).\n", major, minor, patch);
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
      if (config.verbose) fprintf(stderr,"%s %s\n",config.lang.msg_dns_nodb, dns_cache.c_str());

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
