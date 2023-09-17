/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
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
#include "dns_resolv.h"                        /* our header               */

#include "event.h"
#include "thread.h"
#include "util_ipaddr.h"
#include "serialize.h"
#include "tstamp.h"
#include "exception.h"

#include "hnode.h"

extern "C" {
#include <maxminddb.h>
}

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <sys/stat.h>

///
/// @name   DNS database record versions
///
/// @{
constexpr u_int DNS_DB_REC_V1 = 1;              ///< `dns_db_record`
constexpr u_int DNS_DB_REC_V2 = 2;              ///< `dns_db_record`
constexpr u_int DNS_DB_REC_V3 = 3;              ///< Initial version of serialized `dnode_t`.
constexpr u_int DNS_DB_REC_V4 = 4;              ///< Added GeoIP database build time.
constexpr u_int DNS_DB_REC_V5 = 5;              ///< Added GeoIP latitude and longitude.
constexpr u_int DNS_DB_REC_V6 = 6;              ///< Added `geoname_id`.
constexpr u_int DNS_DB_REC_V7 = 7;              ///< Added ASN number and organization.
constexpr u_int DNS_DB_REC_VER_MAX = 250;       ///< Maximum valid record version.
/// @}

constexpr size_t DBBUFSIZE = 8192;              ///< Database buffer size.

constexpr int DBFILEMASK = 0664;                ///< DNS cache database file mask (rw-rw-r--).

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
   u_int    version;                            ///< Structure version (v1 or v2)
   time_t   tstamp;                             ///< Time when the address was resolved
   char     ccode[2];                           ///< Two-character country code
   char     hostname[1];                        ///< Host name (variable length)
};

///
/// @brief  A serializable DNS cache database record
///
/// An instance of this class contains only serializable data saved in the DNS cache
/// database. 
///
struct dns_db_record_t {
   char        ccode[hnode_t::ccode_size];      ///< Two-character country code (no null character)
   bool        spammer;                         ///< Caught spamming?
   tstamp_t    tstamp;                          ///< Time stamp when the address was resolved
   string_t    city;                            ///< City name
   string_t    hostname;                        ///< Host name
   uint64_t    geoip_tstamp;                    ///< GeoIP build time (`time_t`)
   double      latitude;                        ///< IP address latitude
   double      longitude;                       ///< IP address longitude
   uint32_t    geoname_id;                      ///< The geoname_id from the GeoIP database

   uint64_t    asn_tstamp;                      ///< ASN database build time (`time_t`).
   uint32_t    as_num;                          ///< Autonomous system number.
   string_t    as_org;                          ///< Autonomous system organization.

   public:
      /// Constructs an empty DNS cache record instance.
      dns_db_record_t(void) : spammer(false), geoip_tstamp(0), latitude(0.), longitude(0.), geoname_id(0), asn_tstamp(0), as_num(0)
      {
         memset(ccode, 0, hnode_t::ccode_size);
      }

      /// Constructs a DNS cache record by moving resources from anoher instance.
      dns_db_record_t(dns_db_record_t&& other) noexcept;

      /// Returns a serialized size of this instance, in bytes.
      size_t s_data_size(void) const;
      
      /// Serializes instance data members into the buffer.
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      /// Reads serialized data from the buffer and assigns its data members.
      size_t s_unpack_data(const void *buffer, size_t bufsize);
};

///
/// @brief  An internal DNS resolver node associated with each host node being processed
///
/// DNS resolver cannot modify const host nodes in the context of its own threads and 
/// uses `dnode_t` instances to carry new and updated information until `get_hnode` is
/// called, at which point `hnode_t` may be modified in the context of the application
/// thread that queued the host node for resolution.
///
/// A `dnode_t` instance may be queued for a database update after it has been resolved.
/// In this case `hnode` will be nullptr and `hostaddr` will be a non-empty sting. Such
/// instances must be destroyed after the database was updated and should never be queued
/// to be returned from `get_hnode`.
///
class dns_resolver_t::dnode_t {
   /// Allow `get_hnode` to get a modifiable pointer to `hnode_t`.
   friend hnode_t *dns_resolver_t::get_hnode(void);

   private:
      hnode_t        *m_hnode;            ///< A modifiable host node.

   public:
      const hnode_t  *hnode;              ///< A read-only host node.
      class dnode_t  *llist;

      string_t       hostaddr;            ///< An IP address that is populated only when `hnode` is nullptr.

      string_t       hostname;            ///< Host name (populated only if DNS look-ups are enabled).
      string_t       ccode;               ///< Two-character country code
      string_t       city;                ///< City name.

      bool           spammer;             ///< Caught spamming?

      uint64_t       geoip_tstamp;        ///< GeoIP build time (`time_t`)

      double         latitude;            ///< IP address latitude
      double         longitude;           ///< IP address longitude

      uint32_t       geoname_id;          ///< The geoname_id from the GeoIP database

      uint64_t       asn_tstamp;          ///< ASN database build time (`time_t`).
      uint32_t       as_num;              ///< Autonomous system number.
      string_t       as_org;              ///< Autonomous system organization name.

      union {
         sockaddr       s_addr_ip;        // s_addr would be better, but wisock2 defines it as a macro
         sockaddr_in    s_addr_ipv4;      // IPv4 socket address
         sockaddr_in6   s_addr_ipv6;      // IPv6 socket address
      };

   private:
      void remove_host_node(void);

   public:
      /// Constructs an instance from a host IP address and an address family identifier.
      dnode_t(hnode_t& hnode, unsigned short sa_family);

      ~dnode_t(void);

      /// Assigns this instance data members by moving resources from a DNS cache record.
      dnode_t& operator = (dns_db_record_t&& dnsrec);

      /// Constructs a DNS cache record out of data within this instance.
      dns_db_record_t get_db_record(const tstamp_t& ts_runtime, uint64_t ts_geoip_db, uint64_t ts_asn_db) const;

      /// Returns an IP address for this instance.
      const string_t& key(void) const {return hnode ? hnode->string : hostaddr;}

      /// Populates one of the `sockaddr` entries based on the IP address family.
      bool fill_sockaddr(void);
};

//
// DNS DB record
//

dns_db_record_t::dns_db_record_t(dns_db_record_t&& other) noexcept :
   spammer(other.spammer),
   tstamp(other.tstamp),
   city(std::move(other.city)),
   hostname(std::move(other.hostname)),
   geoip_tstamp(other.geoip_tstamp),
   latitude(other.latitude),
   longitude(other.longitude),
   geoname_id(other.geoname_id),
   asn_tstamp(other.asn_tstamp),
   as_num(other.as_num),
   as_org(other.as_org)
{
   memcpy(ccode, other.ccode, hnode_t::ccode_size);
}

size_t dns_db_record_t::s_data_size(void) const
{
   return sizeof(u_int) +                 // version
            serializer_t::s_size_of(tstamp) +
            hnode_t::ccode_size + 
            serializer_t::s_size_of(city) +
            serializer_t::s_size_of(hostname) + 
            serializer_t::s_size_of(spammer) + 
            serializer_t::s_size_of(geoip_tstamp) + 
            serializer_t::s_size_of(latitude) + 
            serializer_t::s_size_of(longitude) + 
            serializer_t::s_size_of(geoname_id) + 
            serializer_t::s_size_of(asn_tstamp) +
            serializer_t::s_size_of(as_num) +
            serializer_t::s_size_of(as_org);
}

size_t dns_db_record_t::s_pack_data(void *buffer, size_t bufsize) const
{
   void *ptr = buffer;

   // make sure all data fits in the buffer
   if(s_data_size() > bufsize)
      return 0;

   serializer_t sr(buffer, bufsize);

   // always save the latest version
   ptr = sr.serialize(ptr, DNS_DB_REC_V6);

   // sr.serialize all data members
   ptr = sr.serialize(ptr, tstamp);
   ptr = sr.serialize(ptr, ccode);
   ptr = sr.serialize(ptr, city);
   ptr = sr.serialize(ptr, hostname);
   ptr = sr.serialize(ptr, spammer);
   ptr = sr.serialize(ptr, geoip_tstamp);
   ptr = sr.serialize(ptr, latitude);
   ptr = sr.serialize(ptr, longitude);
   ptr = sr.serialize(ptr, geoname_id);
   ptr = sr.serialize(ptr, asn_tstamp);
   ptr = sr.serialize(ptr, as_num);
   ptr = sr.serialize(ptr, as_org);

   // return the size of the serialized data
   return (char*) ptr - (const char*) buffer;
}

size_t dns_db_record_t::s_unpack_data(const void *buffer, size_t bufsize)
{
   u_int version = 0;
   const void *ptr = buffer;

   serializer_t sr(buffer, bufsize);

   ptr = sr.deserialize(ptr, version);

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
   ptr = sr.deserialize(ptr, tstamp);

   ptr = sr.deserialize(ptr, ccode);

   ptr = sr.deserialize(ptr, city);
   ptr = sr.deserialize(ptr, hostname);
   ptr = sr.deserialize(ptr, spammer);

   if(version >= DNS_DB_REC_V4)
      ptr = sr.deserialize(ptr, geoip_tstamp);

   if(version >= DNS_DB_REC_V5) {
      ptr = sr.deserialize(ptr, latitude);
      ptr = sr.deserialize(ptr, longitude);
   }

   if(version >= DNS_DB_REC_V6)
      ptr = sr.deserialize(ptr, geoname_id);

   if(version >= DNS_DB_REC_V7) {
      ptr = sr.deserialize(ptr, asn_tstamp);
      ptr = sr.deserialize(ptr, as_num);
      ptr = sr.deserialize(ptr, as_org);
   }

   return (const char*) ptr - (char*) buffer;
}

//
// DNS resolver node 
//

dns_resolver_t::dnode_t::dnode_t(hnode_t& hnode, unsigned short sa_family) : 
      hnode(hnode.resolved ? nullptr : &hnode), 
      m_hnode(hnode.resolved ? nullptr : &hnode), 
      llist(nullptr), 
      spammer(false),
      geoip_tstamp(0),
      latitude(0.),
      longitude(0.),
      geoname_id(0),
      asn_tstamp(0),
      as_num(0)
{
   memset(&s_addr_ip, 0, std::max(sizeof(s_addr_ipv4), sizeof(s_addr_ipv6)));

   s_addr_ip.sa_family = sa_family;

   //
   // A resolved host node can be queued for an update, which currently just means
   // that their spammer flag has been set. While technically an existing DNS record
   // can be updated via an offset to the spammer field, doing so across multiple
   // DNS record versions is non-trivial and given that these updates happen not
   // very frequently, just copy all fields into this dnode_t instance and update
   // the entire DNS record to the latest version.
   //
   if(hnode.resolved) {
      hostaddr = hnode.string;
      hostname = hnode.name;
      spammer = hnode.spammer;
      ccode.assign(hnode.ccode, hnode_t::ccode_size);
      city = hnode.city;
      latitude = hnode.latitude;
      longitude = hnode.longitude;
      geoname_id = hnode.geoname_id;
      as_num = hnode.as_num;
      as_org = hnode.as_org;
   }
}

dns_resolver_t::dnode_t::~dnode_t(void) 
{
}

dns_resolver_t::dnode_t& dns_resolver_t::dnode_t::operator = (dns_db_record_t&& dnsrec)
{
   ccode.assign(dnsrec.ccode, hnode_t::ccode_size);

   hostname = std::move(dnsrec.hostname);
   city = std::move(dnsrec.city);

   spammer = dnsrec.spammer;

   geoip_tstamp = dnsrec.geoip_tstamp;

   latitude = dnsrec.latitude;
   longitude = dnsrec.longitude;

   geoname_id = dnsrec.geoname_id;

   asn_tstamp = dnsrec.asn_tstamp;
   as_num = dnsrec.as_num;
   as_org = dnsrec.as_org;

   return *this;
}

dns_db_record_t dns_resolver_t::dnode_t::get_db_record(const tstamp_t& ts_runtime, uint64_t ts_geoip_db, uint64_t ts_asn_db) const
{
   dns_db_record_t dnsrec;

   dnsrec.tstamp = ts_runtime;

   memcpy(dnsrec.ccode, ccode.c_str(), hnode_t::ccode_size);

   dnsrec.hostname = hostname;
   dnsrec.city = city;

   dnsrec.spammer = spammer;

   dnsrec.geoip_tstamp = ts_geoip_db;

   dnsrec.latitude = latitude;
   dnsrec.longitude = longitude;
   dnsrec.geoname_id = geoname_id;

   dnsrec.asn_tstamp = asn_tstamp;
   dnsrec.as_num = as_num;
   dnsrec.as_org = as_org;

   return dnsrec;
}

///
/// Converts the IP address string from the host node into a sockaddr of 
/// the appropriate type. 
/// 
/// Note that while we could do this in the constructor, having a separate 
/// method with a return value makes this operation more straightforward 
/// than handling exceptions thrown from the constructor or having to check 
/// some state data member that would indicate whether sockaddr is valid or 
/// not.
///
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
      hnode = nullptr;
      m_hnode = nullptr;
   }
}

//
// DNS resolver
//

dns_resolver_t::dns_resolver_t(const config_t& config) :
      config(config)
{
   dnode_list = dnode_end = nullptr;

   dns_done_event = nullptr;

   dns_thread_stop = false;

   dns_live_workers = 0;

   dns_cache_ttl = 0;
   
   accept_host_names = false;

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
            dns_db_close(std::move(wrk_ctxs[index].dns_db));
      }

      wrk_ctxs.clear();

      if(dns_db_env) {
         dns_db_env->close(0);
         dns_db_env.reset();
      }

      if(geoip_db) {
         MMDB_close(geoip_db.get());
         geoip_db.reset();
      }

      if(asn_db) {
         MMDB_close(asn_db.get());
         asn_db.reset();
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

   // check if this node should be DNS-resolved
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
      if(dnode_end == nullptr)
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
      return nullptr;

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
   hnode->as_num = dnode->as_num;
   hnode->as_org = std::move(dnode->as_org);

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
      //
      // Need to make a copy of the data we moved above so we don't update
      // the DNS record with empty values. This update should happen quite
      // rarely to justify this extra step.
      //
      dnode->as_org = hnode->as_org;
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
      stime = time(nullptr);

   *hostname = 0;

   if(dnode->s_addr_ip.sa_family == AF_INET) {
      if(getnameinfo(&dnode->s_addr_ip, sizeof(dnode->s_addr_ipv4), hostname, NI_MAXHOST, nullptr, 0, NI_NAMEREQD))
         goto funcexit;
   }
   else if(dnode->s_addr_ip.sa_family == AF_INET6) {
      if(getnameinfo(&dnode->s_addr_ip, sizeof(dnode->s_addr_ipv6), hostname, NI_MAXHOST, nullptr, 0, NI_NAMEREQD))
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
      fprintf(stderr, "[%04lx] DNS lookup: %s: %s (%.0f sec)\n", thread_id(), dnode->hnode->string.c_str(), dnode->hostname.isempty() ? "NXDOMAIN" : dnode->hostname.c_str(), difftime(time(nullptr), stime));

   return !dnode->hostname.isempty();
}

///
/// @brief  Opens the specified MaxMind database.
///
/// Returns a pointer to a `MMDB_s` structure if the database was successfully
/// opened or throws an exceptino otherwise. The caller must close the database
/// via `MMDB_close`.
///
/// A database language identifier that most closely matches the one in our
/// configuration is returned in `lang`.
///
std::unique_ptr<MMDB_s> dns_resolver_t::open_mmdb(const string_t& db_path, string_t& lang, const char *goodmsg, const char *errmsg) const
{
   int mmdb_error;

   std::unique_ptr<MMDB_s> mmdb(new MMDB_s());

   if((mmdb_error = MMDB_open(db_path, MMDB_MODE_MMAP, mmdb.get())) != MMDB_SUCCESS)
      throw exception_t(0, string_t::_format("%s %s (%d - %s)", errmsg, db_path.c_str(), mmdb_error, MMDB_strerror(mmdb_error)));

   if (config.verbose > 1) 
      printf("%s %s\n", goodmsg, db_path.c_str());

   //
   // Find out if there is a suitable language in the list of languages provided in the 
   // MMDB database. Stop looking if we found an exact match (e.g. pt-br or en). If a 
   // partial match is found (e.g. zh in zh-cn), hold onto the MMDB language code and 
   // keep looking. Every subsequent partial match is taken only if it's shorter than 
   // the previous match. In other words, the first of equal-length matches (e.g. en-us 
   // out of en-us, en-uk) or the first shorter partial match (e.g. en out of en-us, en, 
   // en-uk) wins. If we didn't find a matching language, but there is English available, 
   // use it.
   //
   for(size_t i = 0; i < mmdb->metadata.languages.count; i++) {
      if(lang_t::check_language(mmdb->metadata.languages.names[i], config.lang.language_code)) {
         lang.assign(mmdb->metadata.languages.names[i]);
         break;
      }
      else if(lang_t::check_language(mmdb->metadata.languages.names[i], config.lang.language_code, 2) &&
               (!lang_t::check_language(lang, config.lang.language_code, 2) || strlen(mmdb->metadata.languages.names[i]) < lang.length()))
         lang.assign(mmdb->metadata.languages.names[i]);
      else if(lang.isempty() && lang_t::check_language(mmdb->metadata.languages.names[i], "en"))
         lang.assign("en", 2);
   }

   return mmdb;
}

///
/// @brief   Initializes the DNS resolver
///
/// This method reports progress to the standard output stream and throws an exception if
/// the DNS resolver could not be initialized.
///
void dns_resolver_t::dns_init(void)
{
   // dns_init shouldn't be called if there are no workers configured
   if(!config.dns_children) {
      throw exception_t(0, string_t::_format("%s (zero DNS workers)", config.lang.msg_dns_init));
   }

   dns_cache_ttl = config.dns_cache_ttl;
   
   accept_host_names = config.accept_host_names && config.ntop_ctrys;

#ifdef _WIN32
   WSADATA wsdata;

   if(WSAStartup(MAKEWORD(2, 2), &wsdata) == -1) {
      throw exception_t(0, string_t::_format("%s (Windows sockets)", config.lang.msg_dns_init));
   }
#endif

   // initialize synchronization primitives
   if((dns_done_event = event_create(true, true)) == nullptr) {
      throw exception_t(0, string_t::_format("%s (event object)", config.lang.msg_dns_init));
   }

   // initialize a context for each worker thread
   for(size_t index = 0; index < config.dns_children; index++)
      wrk_ctxs.emplace_back(*this);

   // open the DNS cache database
   if(!config.dns_cache.isempty()) {
      dns_db_env.reset(new DbEnv((u_int32_t) 0));

      //
      // Initialize Berkeley DB for concurrent access (DB_INIT_CDB), which allows multiple
      // concurrent readers and a single writer. Note that DB_INIT_LOCK alone does not 
      // protect against deadlocks that may occur during some operations like page splits.
      //
      u_int32_t dbenv_flags = DB_CREATE | DB_INIT_CDB | DB_THREAD | DB_INIT_MPOOL | DB_PRIVATE;

      if(dns_db_env->open(config.dns_db_path, dbenv_flags, DBFILEMASK)) {
         throw exception_t(0, string_t::_format("%s %s", config.lang.msg_dns_nodb, config.dns_cache.c_str()));
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
         wrk_ctxs[index].dns_db = dns_db_open();
      }

      if (config.verbose > 1) {
         /* Using DNS cache file <filaneme> */
         printf("%s %s\n", config.lang.msg_dns_usec, config.dns_cache.c_str());
      }
   }

   // open the GeoIP database
   if(!config.geoip_db_path.isempty())
      geoip_db = open_mmdb(config.geoip_db_path, geoip_language, config.lang.msg_dns_useg, config.lang.msg_dns_geoe);

   // open the ASN database
   if(!config.asn_db_path.isempty())
      asn_db = open_mmdb(config.asn_db_path, asn_language, config.lang.msg_dns_usea, config.lang.msg_dns_asne);

   // get the current time once to avoid doing it for every host
   runtime.reset(time(nullptr));

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
         dns_db_close(std::move(wrk_ctxs[index].dns_db));
      }
   }

   // delete the BDB environment, if we have one
   if(dns_db_env) {
      dns_db_env->close(0);
      dns_db_env.reset();
   }

   // don't leave dangling pointers behind
   workers.clear();
   wrk_ctxs.clear();

   // close the GeoIP database and clean-up its structures
   if(geoip_db) {
      MMDB_close(geoip_db.get());
      geoip_db.reset();
   }

   // same for ASN database
   if(asn_db) {
      MMDB_close(asn_db.get());
      asn_db.reset();
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

   if(dnode_list == nullptr)
      goto funcidle;

   // check the node at the start of the queue
   if((nptr = dnode_list) == nullptr)
      goto funcidle;

   // remove the node and set dnode_end to nullptr if there no more nodes
   if((dnode_list = dnode_list->llist) == nullptr)
      dnode_end = nullptr;

   // reset the pointer to the next node
   nptr->llist = nullptr;

   dnode_mutex.unlock();

   // check if we just need to update a DNS record
   if(!nptr->hnode)
      dns_db_put(*nptr, dns_db, buffer, bufsize);
   else {
      // if we have a host node, look it up in the database and if not found, resolve it
      lookup = true;
      if(dns_db && dns_db_get(*nptr, dns_db, buffer, bufsize)) 
         cached = true;

      bool goodcc = false, goodasn = false;

      //
      // Resolve the address if it's not cached and/or look up the country code if it's 
      // empty and the GeoIP database is newer than the one we used when we saved the 
      // address in the DNS database. 
      //
      if(!cached || geoip_db && nptr->ccode.isempty() && geoip_db->metadata.build_epoch > nptr->geoip_tstamp) {
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
      }

      if(!cached || asn_db && !nptr->as_num && asn_db->metadata.build_epoch > nptr->asn_tstamp) {
         // look up an assigned system number in the ASN database if there is one
         if(asn_db)
            goodasn = asn_get_info(nptr->hnode->string, nptr->s_addr_ip, nptr->as_num, nptr->as_org);
      }

      // update the database if it's a new IP address or if we found either a country code or an ASN entry for an existing one
      if(dns_db && (!cached || goodcc || goodasn))
         dns_db_put(*nptr, dns_db, buffer, bufsize);
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
   set_os_ex_translator();

   wrk_ctx_t& wrk_ctx = *wrk_ctx_ptr;
   wrk_ctx.buffer.resize(DBBUFSIZE, 0);

   while(!dns_thread_stop) {
      try {
         if(process_node(wrk_ctx.dns_db.get(), wrk_ctx.buffer, wrk_ctx.buffer.capacity()) == false)
            msleep(200);
      }
      catch(const os_ex_t& err) {
         //
         // TODO: Main thread should detect when there are no DNS workers running
         // and should shut down gracefully. Until then, if all DNS worker threads
         // exit with an exception, the main thread will block while waiting for
         // DNS to process all queued IP addresses.
         //
         fprintf(stderr, "%s\n", err.desc().c_str());
         break;
      }
      catch (const DbException &err) {
         fprintf(stderr, "[%d] %s\n", err.get_errno(), err.what());
         break;
      }
      catch (const exception_t &err) {
         fprintf(stderr, "%s\n", err.desc().c_str());
         break;
      }
      catch (const std::exception &err) {
         fprintf(stderr, "%s\n", err.what());
         break;
      }
   }

   wrk_ctx.buffer.reset();

   dec_live_workers();
}

bool dns_resolver_t::geoip_get_ccode(const string_t& hostaddr, const sockaddr& ipaddr, string_t& ccode, string_t& city, double& latitude, double& longitude, uint32_t& geoname_id)
{
   static const char *ccode_path[] = {"country", "iso_code", nullptr};
   static const char *loc_lat_path[] = {"location", "latitude", nullptr};
   static const char *loc_lon_path[] = {"location", "longitude", nullptr};
   static const char *city_geoname_id[] = {"city", "geoname_id", nullptr};
   const char *city_path[] = {"city", "names", geoip_language.c_str(), nullptr};

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
   result = MMDB_lookup_sockaddr(geoip_db.get(), &ipaddr, &mmdb_error);

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
      if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
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
            if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING)
               city.assign(entry_data.utf8_string, entry_data.data_size);
         }
      }

      // get the geoname identifier for this city
      if(MMDB_aget_value(&result.entry, &entry_data, city_geoname_id) == MMDB_SUCCESS) {
         if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UINT32)
            geoname_id = entry_data.uint32;
      }

      // check if we have the coordinates in the entry
      if(MMDB_aget_value(&result.entry, &entry_data, loc_lat_path) == MMDB_SUCCESS) {
         if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            // hold onto the latitude until we are sure we have the longitude
            double lat = entry_data.double_value;

            if(MMDB_aget_value(&result.entry, &entry_data, loc_lon_path) == MMDB_SUCCESS) {
               if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
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

bool dns_resolver_t::asn_get_info(const string_t& hostaddr, const sockaddr& ipaddr, uint32_t& as_num, string_t& as_org)
{
   static const char *as_num_path[] = {"autonomous_system_number", nullptr};
   static const char *as_org_path[] = {"autonomous_system_organization", nullptr};

   int mmdb_error;
   MMDB_lookup_result_s result = {};
   MMDB_entry_data_s entry_data = {};

   if(!asn_db)
      throw std::runtime_error("ASN database is not open");

   // look up the IP address
   result = MMDB_lookup_sockaddr(asn_db.get(), &ipaddr, &mmdb_error);

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

   // get the assigned system number first
   if(MMDB_aget_value(&result.entry, &entry_data, as_num_path) == MMDB_SUCCESS) {
      if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UINT32)
         as_num = entry_data.uint32;
   }

   // get the organization registered for this system number
   if(MMDB_aget_value(&result.entry, &entry_data, as_org_path) == MMDB_SUCCESS) {
      if(entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING)
         as_org.assign(entry_data.utf8_string, entry_data.data_size);
   }

   // see the comment above return in geoip_get_ccode
   return as_num != 0;
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
bool dns_resolver_t::dns_db_get(dnode_t& dnode, Db *dns_db, void *buffer, size_t bufsize)
{
   bool retval = false;
   int dberror;
   Dbt key, recdata;

   /* ensure we have a dns db */
   if (!dns_db) 
      return false;

   memset(&key, 0, sizeof(key));
   memset(&recdata, 0, sizeof(recdata));

   key.set_data((void*) dnode.hnode->string.c_str());
   key.set_size((u_int32_t) dnode.hnode->string.length());

   // point the record to the internal buffer
   recdata.set_flags(DB_DBT_USERMEM);
   recdata.set_ulen((u_int32_t) bufsize);
   recdata.set_data(buffer);

   if (config.debug_mode) fprintf(stderr,"[%04lx] Checking DNS cache for %s...\n", thread_id(), dnode.hnode->string.c_str());

   switch((dberror = dns_db->get(nullptr, &key, &recdata, 0)))
   {
      case  DB_NOTFOUND: 
         if (config.debug_mode) 
            fprintf(stderr,"[%04lx] ... not found\n", thread_id());
         break;
      case  0: {
         dns_db_record_t dnsrec;

         if(dnsrec.s_unpack_data(recdata.get_data(), recdata.get_size()) == recdata.get_size()) {
            // hold onto the elapsed time in case if we need to report it (dnode_t doesn't keep the time stamp when it was resolved)
            uint64_t elapsed = runtime.elapsed(dnsrec.tstamp);

            if(elapsed <= dns_cache_ttl) {
               dnode = std::move(dnsrec);
               retval = true;
            }

            if (retval && config.debug_mode)
               fprintf(stderr,"[%04lx] ... found: %s (age: %0.2f days)\n", thread_id(), dnode.hostname.isempty() ? "NXDOMAIN" : dnode.hostname.c_str(), elapsed/86400.);
         }
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
void dns_resolver_t::dns_db_put(const dnode_t& dnode, Db *dns_db, void *buffer, size_t bufsize)
{
   Dbt k, v;
   size_t recSize;
   int dberror;

   if(!dns_db)
      return;

   dns_db_record_t dnsrec = dnode.get_db_record(runtime,
                                 geoip_db ? geoip_db->metadata.build_epoch : 0, 
                                 asn_db ? asn_db->metadata.build_epoch : 0);

   recSize = dnsrec.s_pack_data(buffer, bufsize);

   if(recSize == 0)
      return;

   k.set_data((void*) dnode.key().c_str());
   k.set_size((u_int32_t) dnode.key().length());

   v.set_data(buffer);
   v.set_size((u_int32_t) recSize);

   if((dberror = dns_db->put(nullptr, &k, &v, 0)) < 0) {
      if(config.verbose)
         fprintf(stderr,"dns_db_put failed (%04x - %s)!\n", dberror, db_strerror(dberror));
   }
}

///
/// @brief  Opens a DNS cache file within the Berkeley DB environment.
///
/// An exception is thrown if the DNS database cannot be opened for any reason. Berkeley DB
/// exceptions are translated into application exceptions.
///
/// If this function returns, the returned unique pointer will never be empty and the database
/// instance inside the unique pointer will be opened.
///
std::unique_ptr<Db> dns_resolver_t::dns_db_open(void)
{
   struct stat dbStat = {};
   int major = 0, minor = 0, patch = 0;

   /* double check filename was specified */
   if(config.dns_cache.isempty())
      throw exception_t(0, string_t::_format("%s (empty file name)", config.lang.msg_dns_nodb));

   db_version(&major, &minor, &patch);

   if(major < 4 || major == 4 && minor < 4) {
      throw exception_t(0, string_t::_format("%s. Berkeley DB must be v4.4 or newer (found v%d.%d.%d)", config.lang.msg_dns_nodb, major, minor, patch));
   }

   /* minimal sanity check on it */
   if(stat(config.dns_cache, &dbStat) < 0) {
      if(errno != ENOENT) 
         throw exception_t(0, string_t::_format("%s %s (%d)", config.lang.msg_dns_nodb, config.dns_cache.c_str(), errno));
   }
   else {
      if(!dbStat.st_size)  /* bogus file, probably from a crash */
      {
         unlink(config.dns_cache);  /* remove it so we can recreate... */
      }
   }
  
   try {
      std::unique_ptr<Db> dns_db(new Db(dns_db_env.get(), 0));
      int status = 0;

      //
      // Berkeley DB environment was opened with the DNS cache database directory, so
      // we can use just the database file name to open the database because relative
      // database paths are appended to the environment path.
      //
      if((status = dns_db->open(nullptr, config.dns_db_fname, nullptr, DB_HASH, DB_CREATE | DB_THREAD, DBFILEMASK)) != 0)
         throw exception_t(0, string_t::_format("%s %s (%s)", config.lang.msg_dns_nodb, config.dns_cache.c_str(), db_strerror(status)));

      return dns_db;
   }
   catch (const DbException& err) {
      throw exception_t(0, string_t::_format("%s %s (%s)", config.lang.msg_dns_nodb, config.dns_cache.c_str(), err.what()));
   }
}

void dns_resolver_t::dns_db_close(std::unique_ptr<Db> dns_db)
{
   if(dns_db) {
      dns_db->close(0);
      dns_db.reset();
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
