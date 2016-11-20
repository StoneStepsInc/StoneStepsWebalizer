/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)
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
// 1. ua_args, ua_groups and sr_args are stored in the class to minimize
// memory allocations.
//
class webalizer_t {
   private:
      // search argument descriptor
      struct arginfo_t {
         const char  *name;
         size_t      namelen; 
         size_t      arglen;
         
         arginfo_t(void) {name = NULL; namelen = arglen = 0;}
      };
      
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
      
   private:
      const config_t& config;
      
      parser_t    parser;
      state_t     state;
      dns_resolver_t dns_resolver;

      std::vector<output_t*> output;

      bool        check_dup;                       // check for dups flag

      char        *buffer;

      uint64_t      total_rec;                       // Total Records Processed
      uint64_t      total_ignore;                    // Total Records Ignored
      uint64_t      total_bad;                       // Total Bad Records

      uint64_t      dns_time;                        // DNS wait time
      uint64_t      mnt_time;                        // maintenance time (saving state, etc)
      uint64_t      rpt_time;                        // report time
      
      std::vector<ua_token_t> ua_args;                // user agent argument tokens
      std::vector<size_t>     ua_groups;              // user agent group indexes (ua_args)
      
      std::vector<arginfo_t>  sr_args;                // search string arguments

   private:
      bool init_output_engines(void);
      void cleanup_output_engines(void);

      void write_main_index(void);
      void write_monthly_report(void);
      
      void print_options(const char *pname);
      void print_warranty(void);
      void print_version(void);

      bool check_for_spam_urls(const char *str, size_t slen) const;
      bool srch_string(const string_t& refer, const string_t& srchargs, u_short& termcnt, string_t& srchterms, bool spamcheck);
      void group_host_by_name(const hnode_t& hnode, const vnode_t& vnode);
      void group_hosts_by_name(void);
      void filter_srchargs(string_t& srchargs);
      void proc_index_alias(string_t& url);
      void mangle_user_agent(string_t& agent);
      void filter_user_agent(string_t& agent);

      int prep_report(void);
      int end_month(void);
      int database_info(void);
      int compact_database(void);
      int proc_logfile(void);

      void prep_logfiles(logfile_list_t& logfiles);
      void prep_lfstates(logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs);
      bool get_logrec(lfp_state_t& wlfs, logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs);
      
      int read_log_line(logfile_t& logfile); 
      int parse_log_record(char *buffer, size_t reclen, log_struct& logrec);

      //
      // put_xnode methods
      //
      hnode_t *put_hnode(const string_t& ipaddr, const tstamp_t& tstamp, uint64_t xfer, bool fileurl, bool pageurl, bool spammer, bool robot, bool target, bool& newvisit, bool& newnode, bool& newthost);
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

      bool initialize(int argc, const char * const argv[]);

      bool cleanup(void);

      int run(void);

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
