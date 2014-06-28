/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
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

#include <zlib.h>

#ifdef _WIN32
#	include <winsock.h>
#else
#	include <netinet/in.h>       /* needed for in_addr structure definition   */
#	ifndef INADDR_NONE
#	define INADDR_NONE 0xFFFFFFFF
#	endif  /* INADDR_NONE */
#endif


#ifdef _WIN32
#define F_OK 00
#define R_OK 04
#define ETCDIR getenv("WINDIR")
#else
#ifndef ETCDIR
#error ETCDIR is not defined
#endif
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
         u_long      namelen; 
         u_long      arglen;
         
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
      
      // user agent token types
      enum toktype_t {vertok, urltok, strtok};

      // user agent string descriptor
      struct ua_token_t {
         const char  *start;     // argument start in the user agent string
         u_long      namelen;    // name length (e.g. 7 for Mozilla/5.0)
         u_long      mjverlen;   // major version length (e.g. Firefox/2   in Firefox/2.0.1.10)
         u_long      mnverlen;   // minor version length (e.g. Firefox/2.0 in Firefox/2.0.1.10)
         u_long      arglen;     // entire argument length (e.g. 10 for Opera/9.25)
         toktype_t   argtype;
         
         ua_token_t(void) {start = NULL; namelen = mjverlen = mnverlen = arglen = 0; argtype = strtok;}
         void reset(const char *str, toktype_t type) {start = str; namelen = mjverlen = mnverlen = arglen = 0; argtype = type;}
      };
      
   private:
      config_t    _config;
      const config_t& config;
      
      parser_t    parser;
      state_t     state;

      vector_t<output_t*> output;

      bool        check_dup;                       // check for dups flag

      char        *buffer;

      char        *f_buf;                          // our_getfs buffer
      char        *f_cp;                           // pointer into the buffer
      int         f_end;                           // count to end of buffer

      u_long      total_rec;                       // Total Records Processed
      u_long      total_ignore;                    // Total Records Ignored
      u_long      total_bad;                       // Total Bad Records

      u_long      start_ts;                        // start of the run
      u_long      end_ts;                          // end of the run

      u_long      dns_time;                        // DNS wait time
      u_long      mnt_time;                        // maintenance time (saving state, etc)
      u_long      rpt_time;                        // report time
      
      vector_t<ua_token_t> ua_args;                // user agent argument tokens
      vector_t<u_int>      ua_groups;              // user ageng group indexes (ua_args)
      
      vector_t<arginfo_t>  sr_args;                // search string arguments

   private:
      bool init_output_engines(void);
      void cleanup_output_engines(void);

      void write_main_index(void);
      void write_monthly_report(void);
      
      void print_options(const char *pname);
      void print_warranty(void);
      void print_version(void);

      void srch_string(const string_t& refer, const string_t& srchargs, u_int& scount, bool newvisit);
      void group_host(const hnode_t& hnode, const vnode_t& vnode);
      void group_hosts(void);
      void filter_srchargs(string_t& srchargs);
      void proc_index_alias(string_t& url);
      void mangle_user_agent(string_t& agent);
      void filter_user_agent(string_t& agent, const string_t *ragent);

      char *our_gzgets(gzFile fp, char *buf, int size);

      int prep_report(void);
      int end_month(void);
      int database_info(void);
      int compact_database(void);
      int proc_logfile(void);
      
      int read_log_line(logfile_t& logfile); 
      int parse_log_record(char *buffer, size_t reclen, log_struct& logrec);

      //
      // put_xnode methods
      //
      hnode_t *put_hnode(const string_t& ipaddr, u_long tstamp, double xfer, bool fileurl, bool pageurl, bool spammer, bool robot, bool target, bool& newvisit, bool& newnode, bool& newthost);
      hnode_t *put_hnode(const string_t& grpname, u_long hits, u_long files, u_long pages, double xfer, u_long visitlen, bool& newnode);

      rnode_t *put_rnode(const string_t&, int, u_long, bool newvisit, bool& newnode);

      unode_t *put_unode(const string_t& url, const string_t& srchargs, u_int type, double xfer, double proctime, u_short port, bool entryurl, bool target, bool& newnode);

      anode_t *put_anode(const string_t& str, u_int type, double xfer, bool newvisit, bool robot, bool& newnode);

      snode_t *put_snode(const string_t& str, u_int termcnt, bool newvisit, bool& newnode);

      inode_t *put_inode(const string_t& str, u_int type, bool fileurl, double xfer, u_long tstamp, double proctime, bool& newnode);

      rcnode_t *put_rcnode(const string_t& method, const string_t& url, u_short respcode, bool restore, u_long count, bool *newnode = NULL);

      dlnode_t *put_dlnode(const string_t& name, u_int respcode, u_long tstamp, u_long proctime, u_long xfer, hnode_t& hnode, bool& newnode);

      //
      //
      //
      static int qs_srcharg_cmp(const arginfo_t *e1, const arginfo_t *e2);

   public:
      webalizer_t(void);

      ~webalizer_t(void);

      bool initialize(int argc, const char * const argv[]);

      bool cleanup(void);

      int run(void);

      //
      //
      //
      vnode_t *update_visit(hnode_t *hptr, u_long tstamp);

      void update_visits(u_long tstamp);

      danode_t *update_download(dlnode_t *dlnode, u_long tstamp);

      void update_downloads(u_long tstamp);

      spnode_t *put_spnode(const string_t& host);
};

extern const char *version  ;                 /* program version          */
extern const char *editlvl  ;                 /* edit level               */
extern const char *buildnum ;                 /* build number             */
extern const char *moddate  ;                 /* modification date        */
extern const char *copyright;

extern int     verbose      ;                 /* 2=verbose,1=err, 0=none  */
extern int     debug_mode   ;                 /* debug mode flag          */

#endif  /* _WEBALIZER_H */
