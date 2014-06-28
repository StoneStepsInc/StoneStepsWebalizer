/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   dns_resolv.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

#include <db.h>                                /* DB header ****************/

#include "GeoIP.h"

#include "lang.h"                              /* language declares        */
#include "hashtab.h"                           /* hash table functions     */
#include "dns_resolv.h"                        /* our header               */

#include "mutex.h"
#include "event.h"
#include "thread.h"

//
//
//
#define DNS_DB_REC_V1      ((u_int) 1)          // version[4], tstamp[4], ccode[2], hostname[]
#define DNS_DB_REC_VER     DNS_DB_REC_V1        // current version
#define DBBUFSIZE          8192                 // database buffer size

//
// DNS DB record (old)
//
struct dnsRecord { 
   time_t    timeStamp;			                  // Timestamp of resolv data
   char      hostName[1];                       // Hostname (var length)
};

//
// DNS DB record
//
struct dns_db_record {
   u_int    version;                            // structure version
   time_t   tstamp;                             // time when the address was resolved
   char     ccode[2];                           // two-character country code
   char     hostname[1];                        // host name (variable length)
};

//
// DNS resolver node 
//

dnode_t::dnode_t(hnode_t& hnode) : hnode(hnode)
{
   llist = NULL; 
   tstamp = 0; 
   memset(&addr, 0, sizeof(addr));
}

dnode_t::~dnode_t(void) 
{
}

//
// DNS resolver
//

dns_resolver_t::dns_resolver_t(void)
{
   buffer = new u_char[DBBUFSIZE];

   geoip_db = NULL;
   dns_db   = NULL;
   dnode_threads = NULL;
   dns_thread_stop = false;

   dns_live_workers = 0;
   dns_unresolved = 0;

   dns_children = 0;
   dns_cache_ttl = 0;
   
   accept_host_names = false;
}

dns_resolver_t::~dns_resolver_t(void)
{
   delete [] buffer;
}

/*********************************************/
/* PUT_HNODE - insert/update dns host node   */
/*********************************************/
bool dns_resolver_t::put_dnode(hnode_t *hnode, struct in_addr *addr)
{
   bool retval = false;
   DNODEPTR nptr;

	if(!dns_db && !geoip_db)
      return false;

   /* skip bad hostnames */
   if(!hnode || hnode->string.isempty() || hnode->string[0] == ' ') 
      return false;
   
   // check if the address has already been resolved or scheduled
   mutex_lock(dnode_mutex);
   if(hnode->resolved || hnode->scheduled) {
      mutex_unlock(dnode_mutex);
      return true;
   }
   
   //
   // If we are accepting host names in place of IP addresses, derive 
   // country code from the host name right here and return, so the 
   // DNS cache file is not littered with bad IP addresses.
   //
   if(accept_host_names && !is_ip4_address(hnode->string)) {
      string_t ccode;
      ccode.hold(hnode->ccode, 0, true, hnode_t::ccode_size+1);

      hnode->name = hnode->string;
      dns_derive_ccode(hnode->name, ccode);
      
      hnode->resolved = true;
      hnode->scheduled = false;
      
      mutex_unlock(dnode_mutex);
      return true;
   }
   
   hnode->scheduled = true;
   mutex_unlock(dnode_mutex);

   // create a new DNS node
   if((nptr = new dnode_t(*hnode)) != NULL) {
      if (addr) 
			memcpy(&nptr->addr, addr, sizeof(struct in_addr));
      else   
			memset(&nptr->addr, 0, sizeof(struct in_addr));
   }

   // and insert it to the end of the list
   mutex_lock(dnode_mutex);
   if(nptr) {
      if(dnode_end == NULL)
         dnode_list = nptr;
      else
         dnode_end->llist = nptr;
      dnode_end = nptr;

      // notify resolver threads
      event_reset(dns_done_event);
      dns_unresolved++;
   }
   mutex_unlock(dnode_mutex);

   return true;
}

hnode_t *dns_resolver_t::get_hnode(void)
{
   hnode_t *hnode;

   mutex_lock(hqueue_mutex);
   hnode = hqueue.remove();
   mutex_unlock(hqueue_mutex);

   return hnode;
}

//
// process_dnode
//
// Resolves the IP address in dnode to a domain name and looks up country
// code for this address.
//
void dns_resolver_t::process_dnode(DNODEPTR dnode)
{
   bool goodcc;
   int size = 0;
   struct hostent *res_ent;
   string_t ccode;

   if(!dnode)
      goto funcexit;

	if((dnode->addr.s_addr = inet_addr(dnode->hnode.string)) == INADDR_NONE)
		goto funcexit;

	dnode->tstamp = time(NULL);

   // create an empty temporary string
   ccode.hold(dnode->hnode.ccode, 0, true, hnode_t::ccode_size+1);

   // try GeoIP first
   goodcc = geoip_get_ccode(dnode->addr.s_addr, dnode->hnode.string, ccode);

   // resolve the domain name
   if(dns_db) {
      if((res_ent = gethostbyaddr((const char*) &dnode->addr.s_addr, sizeof(dnode->addr.s_addr), AF_INET)) == NULL)
		   goto funcexit;

	   // make sure it's not empty
	   if(!res_ent->h_name || !*res_ent->h_name)
		   goto funcexit;

      dnode->hnode.name = res_ent->h_name;

      // if GeoIP failed, derive country code from the domain name
      if(!goodcc)
         dns_derive_ccode(dnode->hnode.name, ccode);
   }

funcexit:
	if(debug_mode)
		fprintf(stderr, "[%04x] DNS lookup: %s: %s (%d sec)\n", thread_id(), dnode->hnode.string.c_str(), dnode->hnode.name.isempty() ? "NXDOMAIN" : dnode->hnode.name.c_str(), time(NULL)-dnode->tstamp);
}

bool dns_resolver_t::dns_geoip_db(void) const
{
   return geoip_db ? true : false;
}

//
//	dns_init
//
bool dns_resolver_t::dns_init(const config_t& config)
{
   // check if already initialized
   if(dns_children)
      return true;

   // check if needs to initialize
   if(!config.dns_children)
      return true;

   dns_children = config.dns_children;
   dns_cache_ttl = config.dns_cache_ttl;
   
   accept_host_names = config.accept_host_names && config.ntop_ctrys;

#ifdef _WIN32
	WSADATA wsdata;
#endif

#ifdef _WIN32
	if(WSAStartup(MAKEWORD(2, 2), &wsdata) == -1) {
      if(verbose)
         fprintf(stderr, "Cannot initialize Windows sockets\n");
      return false;
	}
#endif

   // initialize synchronization primitives
	if((dnode_mutex = mutex_create()) == NULL) {
      if(verbose)
         fprintf(stderr, "Cannot initialize DNS mutex\n");
      return false;
   }

	if((hqueue_mutex = mutex_create()) == NULL) {
      if(verbose)
         fprintf(stderr, "Cannot initialize the host queue mutex\n");
      return false;
   }

   if((dns_done_event = event_create(true, true)) == NULL) {
      if(verbose)
         fprintf(stderr, "Cannot initialize DNS event\n");
      return false;
   }

   // open the DNS cache database
   if(!config.dns_cache.isempty()) {
      if(!dns_db_open(config.dns_cache))  
			dns_db = NULL;
      else {
         if (verbose > 1) {
				/* Using DNS cache file <filaneme> */
				printf("%s %s\n", lang_t::msg_dns_usec, config.dns_cache.c_str());
			}
      }
   }

   // open the GeoIP database
   if(!config.geoip_db_path.isempty()) {
      if((geoip_db = GeoIP_open(config.geoip_db_path, GEOIP_MEMORY_CACHE)) == NULL) {
         if(verbose)
            fprintf(stderr, "%s %s\n", lang_t::msg_dns_geoe, config.geoip_db_path.c_str());
      }
      else {
         if (verbose > 1) 
            printf("%s %s\n", lang_t::msg_dns_useg, config.geoip_db_path.c_str());
      }

   }

	time(&runtime);

	if(dns_db || geoip_db)
		dns_create_worker_threads();

   if (verbose > 1) {
		/* DNS Lookup (#children): */
		printf("%s (%d)\n",lang_t::msg_dns_rslv, dns_children);
   }

	return (dns_db || geoip_db) ? true : false;
}

//
//	dns_clean_up
//
void dns_resolver_t::dns_clean_up(void)
{
	u_int index;
	u_int waitcnt = 300;

	dns_thread_stop = true;

   if(!dns_children)
      return;

	while(get_live_workers() && waitcnt--)
		msleep(50);

   if(dnode_threads) {
		for(index = 0; index < dns_children; index++) 
			thread_destroy(dnode_threads[index]);
      delete [] dnode_threads;
      dnode_threads = NULL;
   }

	if(dns_db) 
		dns_db_close();
   dns_db = NULL;

   if(geoip_db) 
      GeoIP_delete(geoip_db);
   geoip_db = NULL;

   event_destroy(dns_done_event);
	mutex_destroy(dnode_mutex);
	mutex_destroy(hqueue_mutex);

#ifdef _WIN32
		WSACleanup();
#endif

}

//
// dns_resolve_name 
//
// Resolves the IP address to a domain name. If the domain name is not 
// found, returns the IP address.
//
// IMPORTANT: This method is synchronous and will cause major performance 
// degradation when called directly by the log processing thread(s).
//
string_t dns_resolver_t::dns_resolve_name(const string_t& ipaddr)
{
   hnode_t hnode(ipaddr);
   dnode_t dnode(hnode);

	if(dns_db_get(&dnode, true))
		return string_t(ipaddr);

	process_dnode(&dnode);

	dns_db_put(&dnode);

   return string_t(hnode.name.isempty() ? ipaddr : hnode.name);
}

void dns_resolver_t::dns_wait(void)
{
   bool done = false;

   // if there are no workers, return right away
	if(!dns_children)
      return;

   // otherwise wait for all of them to finish
   if(event_wait(dns_done_event, (u_long) -1) == EVENT_OK)
      return;

   if(verbose)
      fprintf(stderr, "DNS event wait operation has failed - using polling to wait\n");

   while(!done) {
	   mutex_lock(dnode_mutex);
      done = dns_unresolved == 0 ? true : false;
	   mutex_unlock(dnode_mutex);
      msleep(500);
   } 
}

//
//	resolve_domain_name
//
// Picks the next available IP address to resolve and calls process_dnode.
// Returns true if any work was done (even unsuccessful), false otherwise.
//
bool dns_resolver_t::resolve_domain_name(void)
{
	bool cached = false, lookup = false;
	DNODEPTR nptr;
   hnode_t *hnode;

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

   // if the node hasn't been resolved yet, look it up
	if(!nptr->hnode.resolved) {
		lookup = true;
		if(dns_db_get(nptr, false)) 
			cached = true;
		else {
			process_dnode(nptr);
			dns_db_put(nptr);
		}
	}

	mutex_lock(dnode_mutex);

   // recover the host node
   hnode = &nptr->hnode;

   // update DNS state of the host node
   nptr->hnode.resolved = true;
   nptr->hnode.scheduled = false;

   // done with dnode, can be deleted
   delete nptr;

   // update resolver stats
	if(lookup) {
		if(cached) 
         dns_cached++; 
      else 
         dns_resolved++;
   }

   // add the node to the queue of resolved nodes
   mutex_lock(hqueue_mutex);
   hqueue.add(hnode);
   mutex_unlock(hqueue_mutex);

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
   ((dns_resolver_t*) arg)->dns_worker_thread_proc();
   return 0;
}

void dns_resolver_t::dns_worker_thread_proc(void)
{
	while(!dns_thread_stop) {
		if(resolve_domain_name() == false)
			msleep(200);
	}

	dec_live_workers();
}

//
// dns_create_worker_threads
//
// Creates DNS resolver worker threads. 
//
void dns_resolver_t::dns_create_worker_threads(void)
{
	u_int index;

	dnode_threads = new thread_t[dns_children];
	for(index = 0; index < dns_children; index++) {
		inc_live_workers();
		dnode_threads[index] = thread_create(dns_worker_thread_proc, this);
	}
}

bool dns_resolver_t::geoip_get_ccode(u_long ipnum, const string_t& ipaddr, string_t& ccode)
{
   if(!geoip_db) {
      ccode.reset();
      return false;
   }

   if(ipnum == INADDR_NONE || ipnum == INADDR_ANY)
      ccode = GeoIP_country_code_by_addr(geoip_db, ipaddr);
   else
      ccode = GeoIP_country_code_by_ipnum(geoip_db, ntohl(ipnum));

   if(ccode.length() != 2) {
      ccode.reset();
      return false;
   }

   ccode.tolower();

   return true;
}

//
// derives country code from the domain name
//
bool dns_resolver_t::dns_derive_ccode(const string_t& name, string_t& ccode) const
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
//	Looks up the dnode->ipaddr in the database. If nocheck is true, the 
// function will not check whether the entry is stale or not (may be 
// used for subsequent searches to avoid unnecessary DNS lookups).
//
bool dns_resolver_t::dns_db_get(DNODEPTR dnode, bool nocheck)
{
	bool retval = false;
	int dberror;
   DBT key, recdata;
   time_t tstamp;
   dns_db_record *dnsrec;
   string_t ccode;

	/* ensure we have a dns db */
   if (!dns_db || !dnode) 
		return false;

	memset(&key, 0, sizeof(key));
	memset(&recdata, 0, sizeof(recdata));

   key.data = (void*) dnode->hnode.string.c_str();
   key.size = dnode->hnode.string.length();

   // point the record to the internal buffer
	recdata.flags = DB_DBT_USERMEM;
   recdata.ulen = DBBUFSIZE;
   recdata.data = buffer;

   if (debug_mode) fprintf(stderr,"[%04x] Checking DNS cache for %s...\n", thread_id(), dnode->hnode.string.c_str());

   ccode.hold(dnode->hnode.ccode, 0, true, hnode_t::ccode_size+1);

   switch((dberror = dns_db->get(dns_db, NULL, &key, &recdata, 0)))
   {
      case  DB_NOTFOUND: 
			if (debug_mode) 
				fprintf(stderr,"[%04x] ... not found\n", thread_id());
			break;
      case  0:
         dnsrec = (dns_db_record*) recdata.data;
         tstamp = (dnsrec->version >= DNS_DB_REC_V1) ? dnsrec->tstamp : ((struct dnsRecord *)recdata.data)->timeStamp;

			if(nocheck || (u_long) (runtime - tstamp) <= dns_cache_ttl) {
            if(dnsrec->version >= DNS_DB_REC_V1) {
               dnode->tstamp = tstamp;
               dnode->hnode.name = dnsrec->hostname;
               if(*dnsrec->ccode)
                  dnode->hnode.set_ccode(dnsrec->ccode);
               else
                  geoip_get_ccode(dnode->addr.s_addr, dnode->hnode.string, ccode);
            }
            else {
				   dnode->tstamp = ((struct dnsRecord *)recdata.data)->timeStamp;
               dnode->hnode.name = ((struct dnsRecord *)recdata.data)->hostName;
               geoip_get_ccode(dnode->addr.s_addr, dnode->hnode.string, ccode);
            }

				if (debug_mode)
					fprintf(stderr,"[%04x] ... found: %s (age: %0.2lf days)\n", thread_id(), dnode->hnode.name.isempty() ? "NXDOMAIN" : dnode->hnode.name.c_str(), difftime(runtime, dnode->tstamp) / 86400);
				retval = true;
			}
			break;

      default: 
			if (debug_mode) 
				fprintf(stderr," error (%04x - %s)\n", dberror, db_strerror(dberror));
			break;
   }

	return retval;
}

/*********************************************/
/* DB_PUT - put key/val in the cache db      */
/*********************************************/

void dns_resolver_t::dns_db_put(DNODEPTR dnode)
{
   DBT k, v;
   dns_db_record *recPtr = NULL;
   int nameLen, recSize, dberror;

   if(!dns_db || !dnode)
      return;

   nameLen = dnode->hnode.name.length() + 1;

   recSize = sizeof(dns_db_record) + nameLen;
	
	memset(&k, 0, sizeof(k));
	memset(&v, 0, sizeof(v));

   recPtr = (dns_db_record*) buffer;

   recPtr->version = DNS_DB_REC_VER;
   recPtr->tstamp = dnode->tstamp;

   memcpy(&recPtr->ccode, dnode->hnode.ccode, sizeof(recPtr->ccode));

   memcpy(&recPtr->hostname, dnode->hnode.name, nameLen);
   recPtr->hostname[nameLen] = 0;

   k.data = (void*) dnode->hnode.string.c_str();
   k.size = dnode->hnode.string.length();

   v.data = recPtr;
   v.size = recSize;

   mutex_lock(db_mutex);
   if((dberror = dns_db->put(dns_db, NULL, &k, &v, 0)) < 0) {
      if(verbose)
         fprintf(stderr,"dns_db_put failed (%04x - %s)!\n", dberror, db_strerror(dberror));
   }
   mutex_unlock(db_mutex);
}

/*********************************************/
/* OPEN_CACHE - open our cache file RDONLY   */
/*********************************************/

bool dns_resolver_t::dns_db_open(const string_t& dns_cache)
{
   struct stat  dbStat;
	int major, minor, patch;

   /* double check filename was specified */
   if(dns_cache.isempty()) { 
      dns_db=NULL; 
      return false; 
   }

	db_version(&major, &minor, &patch);

	if(major < 4 && minor < 3) {
      if(verbose)
		   fprintf(stderr, "Error: The Berkeley DB library must be v4.3 or newer (found v%d.%d.%d).\n", major, minor, patch);
		return false;
	}

   /* minimal sanity check on it */
   if(stat(dns_cache, &dbStat) < 0) {
      if(errno != ENOENT) 
         return false;
   }
   else {
      if(!dbStat.st_size)  /* bogus file, probably from a crash */
      {
         unlink(dns_cache);  /* remove it so we can recreate... */
      }
   }
  
   if(db_create(&dns_db, NULL, 0)) {
      if (verbose) fprintf(stderr,"%s %s\n",lang_t::msg_dns_nodb, dns_cache.c_str());
      return false;                  /* disable cache */
   }

   /* open cache file */
   if(dns_db->open(dns_db, NULL, dns_cache, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0664))
   {
      /* Error: Unable to open DNS cache file <filename> */
      if (verbose) fprintf(stderr,"%s %s\n",lang_t::msg_dns_nodb, dns_cache.c_str());
      return false;                  /* disable cache */
   }

   db_mutex = mutex_create();

   return true;
}

/*********************************************/
/* CLOSE_CACHE - close our RDONLY cache      */
/*********************************************/

bool dns_resolver_t::dns_db_close(void)
{
	if(dns_db)
		dns_db->close(dns_db, 0);
   dns_db = NULL;

   if(db_mutex)
      mutex_destroy(db_mutex);
   db_mutex = NULL;

   return true;
}
