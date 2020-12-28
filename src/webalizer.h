/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   webalizer.h
*/
#ifndef WEBALIZER_H
#define WEBALIZER_H

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

/// 
/// @brief  Main application class that processes logs, generates reports and 
///         performs maintenance tasks.
///
class webalizer_t {
   private:
      // character buffer allocator and holder types
      typedef p2_buffer_allocator_tmpl<string_t::char_type> buffer_allocator_t;
      typedef char_buffer_holder_tmpl<string_t::char_type, size_t> buffer_holder_t;
      
      ///
      /// @brief  A URL search argument descriptor
      ///
      /// A search argument descriptor that points to the beginning of a search argument 
      /// within a query string. The descriptor does not own memory contaiing the search 
      /// argument and must have shorter life span than the underlying query string.
      ///
      /// Pointers returned from `name` and `value` methods should not be used in pointer
      /// arithmetic because they may return pointer values outside of the query string 
      /// range. Neither method ever returns a nullptr pointer.
      ///
      struct arginfo_t {
         const char  *arg;             ///< Points to a search argument within the query string
         size_t      namelen;          ///< Search argument name length
         size_t      arglen;           ///< Entire search argument length, including name and value
         
         /// Constructs an empty search argument descriptor.
         arginfo_t(void) : arg(nullptr), namelen(0), arglen(0) {}

         /// Constructs a complete search argument descriptor
         arginfo_t(const char *arg, size_t namelen, size_t arglen) : arg(arg), namelen(namelen), arglen(arglen) {}

         /// Returns a pointer to the search argument name or an empty string if there is none.
         const char *name(void) const
         {
            return namelen ? arg : "";
         }

         /// Returns the length of the search argument value (zero if there is no value).
         size_t value_length(void) const 
         {
            // value is present only if there is an equal sign, even if namelen is zero
            return namelen == arglen ? 0 : arglen - namelen - 1;
         }

         /// Return a pointer to the search argument value or an empty string, if there is none.
         const char *value(void) const
         {
            return namelen == arglen ? "" : arg + namelen + 1;
         }
      };

      // search argument vector allocator
      typedef pool_allocator_t<arginfo_t, 8> srch_arg_alloc_t;
      
      ///
      /// @brief  Log file parser state descriptor
      ///
      /// A log file parser state descriptor maintains a log file object and a current log 
      /// record object populated from the log record that was just read from the log file.
      ///
      /// The descriptor does not own either of the objects.
      ///
      struct lfp_state_t {
         logfile_t   *logfile;
         log_struct  *logrec;
         
         public:
         lfp_state_t(void) : logfile(nullptr), logrec(nullptr) {} 
         
         lfp_state_t(logfile_t *logfile, log_struct *logrec) : logfile(logfile), logrec(logrec) {}
         
         lfp_state_t(const lfp_state_t& otherme) : logfile(otherme.logfile), logrec(otherme.logrec) {}
         
         lfp_state_t& operator = (const lfp_state_t& otherme) {logfile = otherme.logfile; logrec = otherme.logrec; return *this;}
         
         void reset(void) {logfile = nullptr; logrec = nullptr;}
      };

      typedef std::list<lfp_state_t, pool_allocator_t<lfp_state_t, FOPEN_MAX>> lfp_state_list_t;
      typedef std::list<log_struct*, pool_allocator_t<log_struct*, FOPEN_MAX>> logrec_list_t;
      typedef std::list<logfile_t*, pool_allocator_t<logfile_t*, FOPEN_MAX>> logfile_list_t;

      // user agent token types
      enum toktype_t {vertok, urltok, strtok};

      ///
      /// @brief  A user agent token descriptor
      ///
      /// A user agent string contains multiple tokens that describe various aspects of 
      /// the user agent. Each instance of `ua_token_t` corresponds to one of the tokens.
      ///
      /// The `start` pointer points to the location of the token within the original 
      /// user agent string or to the name of the matching pattern in the `GroupAgentArgs`
      /// list.
      ///
      struct ua_token_t {
         const char  *start;     ///< Argument start in the user agent string or a matching pattern name
         size_t      namelen;    ///< Name length (e.g. 7 for Mozilla/5.0)
         size_t      mjverlen;   ///< Major version length (e.g. Firefox/2   in Firefox/2.0.1.10)
         size_t      mnverlen;   ///< Minor version length (e.g. Firefox/2.0 in Firefox/2.0.1.10)
         size_t      arglen;     ///< Entire argument length (e.g. 10 for Opera/9.25)
         toktype_t   argtype;
         
         ua_token_t(void) {start = nullptr; namelen = mjverlen = mnverlen = arglen = 0; argtype = strtok;}
         void reset(const char *str, toktype_t type) {start = str; namelen = mjverlen = mnverlen = arglen = 0; argtype = type;}
      };

      // user agent token allocators
      typedef pool_allocator_t<ua_token_t, 8> ua_token_alloc_t;
      typedef pool_allocator_t<size_t, 8> ua_grp_idx_alloc_t;

      ///
      /// @brief  Various processing times collected across multiple application calls
      ///
      struct proc_times_t {
         uint64_t dns_time = 0;                    ///< DNS wait time
         uint64_t mnt_time = 0;                    ///< maintenance time (saving state, etc)
         uint64_t rpt_time = 0;                    ///< report time
      };

      ///
      /// @brief  Run time log record counts
      ///
      struct logrec_counts_t {
         uint64_t total_rec = 0;                   ///< Total records processed
         uint64_t total_ignore = 0;                ///< Total records ignored
         uint64_t total_bad = 0;                   ///< Total bad records
      };

   private:
      static bool abort_signal;                    ///< Was Ctrl-C pressed?
      
      const config_t& config;                      ///< Read-only application configuration object
      
      parser_t    parser;                          ///< Log record parser
      state_t     state;                           ///< Monthly state database
      dns_resolver_t dns_resolver;                 ///< DNS and GeoIP resolver database

      std::vector<output_t*> output;               ///< Report generators

      buffer_allocator_t buffer_allocator;         ///< Pooled buffer allocator

      ua_token_alloc_t ua_token_alloc;             ///< Pooled user agent token allocator
      ua_grp_idx_alloc_t ua_grp_idx_alloc;         ///< Pooled group index user agent token allocator

      srch_arg_alloc_t srch_arg_alloc;             ///< Pooled search argument allocator 

   private:
      bool init_output_engines(void);
      void cleanup_output_engines(void);

      void write_main_index(void);
      void write_monthly_report(void);
      
      bool check_for_spam_urls(const char *str, size_t slen) const;
      bool srch_string(const string_t& refer, const string_t& srchargs, u_short& termcnt, string_t& srchterms, bool spamcheck);
      void group_host_by_name(const hnode_t& hnode, const vnode_t& vnode);
      void process_resolved_hosts(void);
      bool check_ignore_url_list(const string_t& url, const string_t& srchargs, std::vector<arginfo_t, srch_arg_alloc_t>& sr_args) const;
      void filter_srchargs(string_t& srchargs, std::vector<arginfo_t, srch_arg_alloc_t>& sr_args);
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
      int parse_log_record(string_t::char_buffer_t& buffer, size_t reclen, log_struct& logrec, u_int fileid, uint64_t recnum);

      //
      // put_xnode methods
      //
      storable_t<hnode_t> *put_hnode(const string_t& ipaddr, const tstamp_t& tstamp, int64_t relts, uint64_t xfer, bool fileurl, bool pageurl, bool spammer, bool robot, bool target, bool& newvisit, bool& newnode, bool& newthost, bool& newspammer);
      storable_t<hnode_t> *put_hnode(const string_t& grpname, int64_t relts, uint64_t hits, uint64_t files, uint64_t pages, uint64_t xfer, uint64_t visitlen, bool& newnode);

      rnode_t *put_rnode(const string_t&, int64_t relts, nodetype_t type, uint64_t, bool newvisit, bool& newnode);

      storable_t<unode_t> *put_unode(const string_t& url, int64_t relts, const string_t& srchargs, nodetype_t type, uint64_t xfer, double proctime, u_short port, bool entryurl, bool target, bool& newnode);

      anode_t *put_anode(const string_t& agent, int64_t relts, nodetype_t type, uint64_t xfer, bool newvisit, bool robot, bool& newnode);

      snode_t *put_snode(const string_t& srch, int64_t relts, u_short termcnt, bool newvisit, bool& newnode);

      inode_t *put_inode(const string_t& ident, int64_t relts, nodetype_t type, bool fileurl, uint64_t xfer, const tstamp_t& tstamp, double proctime, bool& newnode);

      rcnode_t *put_rcnode(const string_t& method, int64_t relts, const string_t& url, u_short respcode, bool restore, uint64_t count, bool *newnode = nullptr);

      dlnode_t *put_dlnode(const string_t& name, int64_t relts, u_int respcode, const tstamp_t& tstamp, uint64_t proctime, uint64_t xfer, storable_t<hnode_t>& hnode, bool& newnode);

      //
      //
      //
      static int qs_srcharg_name_cmp(const arginfo_t *e1, const arginfo_t *e2);
      static int qs_srcharg_cmp(const arginfo_t *e1, const arginfo_t *e2);

      static void unpack_inactive_hnode_cb(hnode_t& hnode, bool active, void *_this);

      static storable_t<vnode_t> *end_visit_cb(storable_t<hnode_t> *hnode, void *arg);
      static storable_t<danode_t> *end_download_cb(storable_t<dlnode_t> *dlnode, void *arg);

      storable_t<vnode_t> *end_visit(storable_t<hnode_t> *hnode);
      storable_t<danode_t> *end_download(storable_t<dlnode_t> *dlnode);

   public:
      webalizer_t(const config_t& config);

      ~webalizer_t(void);

      void initialize(void);

      void cleanup(void);

      int run(void);

      static void ctrl_c_handler(void);

      void print_options(const char *pname);
      void print_warranty(void);
      void print_version(void);
      void print_intro(void);
      void print_config(void);

      //
      //
      //
      storable_t<vnode_t> *update_visit(storable_t<hnode_t> *hptr, const tstamp_t& tstamp);

      void update_visits(const tstamp_t& tstamp);

      storable_t<danode_t> *update_download(storable_t<dlnode_t> *dlnode, const tstamp_t& tstamp);

      void update_downloads(const tstamp_t& tstamp);
};

#endif  // WEBALIZER_H
