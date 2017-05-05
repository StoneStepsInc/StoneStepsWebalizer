/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   webalizer.h
*/
#ifndef _WEBALIZER_H
#define _WEBALIZER_H

#include "version.h"
#include "logrec.h"
#include "config.h"
#include "types.h"
#include "hashtab.h"
#include "graphs.h"
#include "output.h"
#include "parser.h"
#include "history.h"
#include "preserve.h"
#include "dns_resolv.h"
#include "database.h"
#include "hashtab_nodes.h"
#include "logfile.h"
#include "pool_allocator.h"
#include "p2_buffer_allocator.h"

#include <zlib.h>
#include <vector>
#include <list>

#ifndef _WIN32
#include <netinet/in.h>       /* needed for in_addr structure definition   */
#ifndef INADDR_NONE
#define INADDR_NONE 0xFFFFFFFF
#endif  /* INADDR_NONE */
#else
#define F_OK 00
#define R_OK 04
#endif

// -----------------------------------------------------------------------
//
// webalizer_t
//
// -----------------------------------------------------------------------
//
// 1. Host nodes need to be resolved before their visits can be attributed 
// to various groups (i.e. address, domain name and country groups) and marked 
// as safe (or not) based on historical host address data from the DNS database.
//
// New host nodes go through the following stages before they end up in the 
// monthly state database:
//
//                         dns.put_hnode              dns.get_hnode
//  log processor -----+========+==========+================+========+-------
//                new hnode_t   |     access hnode          ^   group hnode
//                              v                   done    |
//  DNS resolver  --------------+===+==+==+==+==+=====+-----+-----------------
//                                GeoIP, DNS, spammer
//
// A host node that has been retrieved from the DNS resolver will have its
// resolved member set to true. While a host node is moving through the DNS
// resolver, some of its members, such as the IP address, will be accessed
// concurrently (the equal sign on the diagram) and cannot be modified by
// either of the threads above. 
//
// Note that dns.put_node/get_node run in the context of the log processor 
// thread and may modify the host node either before the DNS resolver thread 
// starts resolving the host node in question or after it finished processing 
// this host node.
//
// There's a short period of time between dns.put_hnode and dns.get_hnode 
// when non-spam requests for historic spammer hosts may be not counted as 
// as spam. Addressing this deficiency requires significant redesign around
// visit processing and will be done at some later time.
//
class webalizer_t {
   private:
      // character buffer allocator and holder types
      typedef p2_buffer_allocator_tmpl<string_t::char_type> buffer_allocator_t;
      typedef char_buffer_holder_tmpl<string_t::char_type, size_t> buffer_holder_t;
      
      // search argument descriptor
      struct arginfo_t {
         const char  *name;
         size_t      namelen; 
         size_t      arglen;
         
         arginfo_t(void) {name = NULL; namelen = arglen = 0;}
      };

      // search argument vector allocator
      typedef pool_allocator_t<arginfo_t, 8> srch_arg_alloc_t;
      
      // log file parser state (does not own either of the objects)
      struct lfp_state_t {
         logfile_t   *logfile;
         log_struct  *logrec;
         
         public:
         lfp_state_t(void) : logfile(NULL), logrec(NULL) {} 
         
         lfp_state_t(logfile_t *logfile, log_struct *logrec) : logfile(logfile), logrec(logrec) {}
         
         lfp_state_t(const lfp_state_t& otherme) : logfile(otherme.logfile), logrec(otherme.logrec) {}
         
         lfp_state_t& operator = (const lfp_state_t& otherme) {logfile = otherme.logfile; logrec = otherme.logrec; return *this;}
         
         void reset(void) {logfile = NULL; logrec = NULL;}
      };

      typedef std::list<lfp_state_t, pool_allocator_t<lfp_state_t, FOPEN_MAX>> lfp_state_list_t;
      typedef std::list<log_struct*, pool_allocator_t<log_struct, FOPEN_MAX>> logrec_list_t;
      typedef std::list<logfile_t*, pool_allocator_t<logfile_t, FOPEN_MAX>> logfile_list_t;

      // user agent token types
      enum toktype_t {vertok, urltok, strtok};

      // user agent string descriptor
      struct ua_token_t {
         const char  *start;     // argument start in the user agent string
         size_t      namelen;    // name length (e.g. 7 for Mozilla/5.0)
         size_t      mjverlen;   // major version length (e.g. Firefox/2   in Firefox/2.0.1.10)
         size_t      mnverlen;   // minor version length (e.g. Firefox/2.0 in Firefox/2.0.1.10)
         size_t      arglen;     // entire argument length (e.g. 10 for Opera/9.25)
         toktype_t   argtype;
         
         ua_token_t(void) {start = NULL; namelen = mjverlen = mnverlen = arglen = 0; argtype = strtok;}
         void reset(const char *str, toktype_t type) {start = str; namelen = mjverlen = mnverlen = arglen = 0; argtype = type;}
      };

      // user agent token allocators
      typedef pool_allocator_t<ua_token_t, 8> ua_token_alloc_t;
      typedef pool_allocator_t<size_t, 8> ua_grp_idx_alloc_t;

      // various processing times collected across multiple calls
      struct proc_times_t {
         uint64_t dns_time = 0;                    // DNS wait time
         uint64_t mnt_time = 0;                    // maintenance time (saving state, etc)
         uint64_t rpt_time = 0;                    // report time
      };

      // run time log record counts
      struct logrec_counts_t {
         uint64_t total_rec = 0;                   // total records processed
         uint64_t total_ignore = 0;                // total records ignored
         uint64_t total_bad = 0;                   // total bad records
      };

   private:
      const config_t& config;
      
      parser_t    parser;
      state_t     state;
      dns_resolver_t dns_resolver;

      std::vector<output_t*> output;

      buffer_allocator_t buffer_allocator;

      ua_token_alloc_t ua_token_alloc;          // pooled user agent token allocator
      ua_grp_idx_alloc_t ua_grp_idx_alloc;

      srch_arg_alloc_t srch_arg_alloc;          // pooled search argument allocator 

   private:
      bool init_output_engines(void);
      void cleanup_output_engines(void);

      void write_main_index(void);
      void write_monthly_report(void);
      
      bool check_for_spam_urls(const char *str, size_t slen) const;
      bool srch_string(const string_t& refer, const string_t& srchargs, u_short& termcnt, string_t& srchterms, bool spamcheck);
      void group_host_by_name(const hnode_t& hnode, const vnode_t& vnode);
      void process_resolved_hosts(void);
      void filter_srchargs(string_t& srchargs);
      void proc_index_alias(string_t& url);
      void mangle_user_agent(string_t& agent);
      void filter_user_agent(string_t& agent);

      int prep_report(void);
      int end_month(void);
      int database_info(void);
      int compact_database(void);
      int proc_logfile(proc_times_t& ptms, logrec_counts_t& lrcnt);

      void prep_logfiles(logfile_list_t& logfiles);
      void prep_lfstates(logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs, logrec_counts_t& lrcnt);
      bool get_logrec(lfp_state_t& wlfs, logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs, logrec_counts_t& lrcnt);
      
      int read_log_line(string_t::char_buffer_t& buffer, logfile_t& logfile, logrec_counts_t& lrcnt); 
      int parse_log_record(string_t::char_buffer_t& buffer, size_t reclen, log_struct& logrec, uint64_t recnum);

      //
      // put_xnode methods
      //
      hnode_t *put_hnode(const string_t& ipaddr, const tstamp_t& tstamp, uint64_t xfer, bool fileurl, bool pageurl, bool spammer, bool robot, bool target, bool& newvisit, bool& newnode, bool& newthost, bool& newspammer);
      hnode_t *put_hnode(const string_t& grpname, uint64_t hits, uint64_t files, uint64_t pages, uint64_t xfer, uint64_t visitlen, bool& newnode);

      rnode_t *put_rnode(const string_t&, nodetype_t type, uint64_t, bool newvisit, bool& newnode);

      unode_t *put_unode(const string_t& url, const string_t& srchargs, nodetype_t type, uint64_t xfer, double proctime, u_short port, bool entryurl, bool target, bool& newnode);

      anode_t *put_anode(const string_t& str, nodetype_t type, uint64_t xfer, bool newvisit, bool robot, bool& newnode);

      snode_t *put_snode(const string_t& str, u_short termcnt, bool newvisit, bool& newnode);

      inode_t *put_inode(const string_t& str, nodetype_t type, bool fileurl, uint64_t xfer, const tstamp_t& tstamp, double proctime, bool& newnode);

      rcnode_t *put_rcnode(const string_t& method, const string_t& url, u_short respcode, bool restore, uint64_t count, bool *newnode = NULL);

      dlnode_t *put_dlnode(const string_t& name, u_int respcode, const tstamp_t& tstamp, uint64_t proctime, uint64_t xfer, hnode_t& hnode, bool& newnode);

      //
      //
      //
      static int qs_srcharg_cmp(const arginfo_t *e1, const arginfo_t *e2);

   public:
      webalizer_t(const config_t& config);

      ~webalizer_t(void);

      void initialize(int argc, const char * const argv[]);

      void cleanup(void);

      int run(void);

      void print_options(const char *pname);
      void print_warranty(void);
      void print_version(void);
      void print_intro(void);
      void print_config(void);

      //
      //
      //
      vnode_t *update_visit(hnode_t *hptr, const tstamp_t& tstamp);

      void update_visits(const tstamp_t& tstamp);

      danode_t *update_download(dlnode_t *dlnode, const tstamp_t& tstamp);

      void update_downloads(const tstamp_t& tstamp);

      spnode_t *put_spnode(const string_t& host);
};

#endif  /* _WEBALIZER_H */
