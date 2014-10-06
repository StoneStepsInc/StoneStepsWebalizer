/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   config.h
*/

#ifndef __CONFIG_H
#define __CONFIG_H

#include "vector.h"
#include "linklist.h"
#include "tstring.h"
#include "types.h"
#include "lang.h"
#include "tstamp.h"
#include "tmranges.h"

#define DNS_CACHE_TTL     86400*30					// Default TTL of an entry in the DNS cache (in seconds)

#define DNS_MAX_THREADS   100						   // Maximum number of DNS threads

#define DB_DEF_CACHE_SIZE  ((u_long) 0)         // Default cache size (ignored)
#define DB_MIN_CACHE_SIZE  ((u_long) 20 * 1024) // Minimum cache size (20K)
#define DB_DEF_TRICKLE_RATE ((u_int) 5)         // Default database trickle rate

#define FONT_SIZE_SMALL       8.                // points
#define FONT_SIZE_MEDIUM      10.               // points

/* Port numbers */
#define PORT_UNKNOWN			   ((u_short) 0)
#define DEF_HTTP_PORT	      ((u_short) 80)
#define DEF_HTTPS_PORT	      ((u_short) 443)

#define BUFSIZE  16384                 /* Max buffer size for log record   */
#define HALFBUFSIZE  (BUFSIZE >> 1)

#define SLOP_VAL 3600                  /* out of sequence slop (seconds)   */

/* Request types (bit field) */
#define URL_TYPE_UNKNOWN	0x00
#define URL_TYPE_HTTP		0x01
#define URL_TYPE_HTTPS		0x02
#define URL_TYPE_MIXED		0x03			/* HTTP and HTTPS (nothing else) */

/* Response code defines as per draft ietf HTTP/1.1 rev 6 */
#define RC_CONTINUE           100
#define RC_SWITCHPROTO        101
#define RC_OK                 200
#define RC_CREATED            201
#define RC_ACCEPTED           202
#define RC_NONAUTHINFO        203
#define RC_NOCONTENT          204
#define RC_RESETCONTENT       205
#define RC_PARTIALCONTENT     206
#define RC_MULTIPLECHOICES    300
#define RC_MOVEDPERM          301
#define RC_MOVEDTEMP          302
#define RC_SEEOTHER           303
#define RC_NOMOD              304
#define RC_USEPROXY           305
#define RC_MOVEDTEMPORARILY   307
#define RC_BAD                400
#define RC_UNAUTH             401
#define RC_PAYMENTREQ         402
#define RC_FORBIDDEN          403
#define RC_NOTFOUND           404
#define RC_METHODNOTALLOWED   405
#define RC_NOTACCEPTABLE      406
#define RC_PROXYAUTHREQ       407
#define RC_TIMEOUT            408
#define RC_CONFLICT           409
#define RC_GONE               410
#define RC_LENGTHREQ          411
#define RC_PREFAILED          412
#define RC_REQENTTOOLARGE     413
#define RC_REQURITOOLARGE     414
#define RC_UNSUPMEDIATYPE     415
#define RC_RNGNOTSATISFIABLE  416
#define RC_EXPECTATIONFAILED  417
#define RC_SERVERERR          500
#define RC_NOTIMPLEMENTED     501
#define RC_BADGATEWAY         502
#define RC_UNAVAIL            503
#define RC_GATEWAYTIMEOUT     504
#define RC_BADHTTPVER         505

// -----------------------------------------------------------------
//
// config_t
//
// -----------------------------------------------------------------
// 1. Configuration files are processed first and should not output
// anything. Instead, messages are collected in a vector, and will
// be output after the copyright message has been printed.
//
class config_t {
   private:
      struct dst_pair_t {
         string_t   dst_start;
         string_t     dst_end;
      };
      
   private:
      vector_t<string_t> config_fnames;
      vector_t<string_t> messages;
      vector_t<dst_pair_t> dst_pairs;
      bool user_config;

   public:
      bool print_options;
      bool print_version;
      bool print_warranty;
      bool time_me;                               // timing display flag      

      bool hide_robots;                          // hide robots?
      bool group_robots;                         // group robots?
      bool ignore_robots;                        // ignore robots altogether?

      // run modes (process log file if none is set)
      bool prep_report;                          // prepare a report from a database
      bool compact_db;                           // compact database
      bool db_info;                              // print database information
      bool end_month;                            // end active visits, update totals, etc for the current month
      
      // run flags
      bool incremental;                          // incremental mode (partial log procesing)
      bool batch;                                // batch processing (no report generated)
      bool memory_mode;                          // store least used data on disk during a run?
      
      bool pipe_log_names ;                      // read log file names from stdin
      bool last_log;                             // end month after this log file?

      bool conv_url_lower_case;
      bool bundle_groups;
      bool no_def_index_alias;				       // ignore default index alias? 
      bool html_meta_noindex;                    // add noindex, nofollow?      
      bool enable_phrase_values;
      bool upstream_traffic;
      bool font_anti_aliasing;
      bool graph_true_color;
      bool sort_srch_args;
      bool ignore_referrer_partial;
      bool local_time;                           // 1=localtime 0=GMT (UTC)  
      bool ignore_hist;                          // history flag (1=skip)    
      bool hourly_graph;                         // hourly graph display     
      bool hourly_stats;                         // hourly stats table       
      bool enable_js;
      bool monthly_totals_stats;                 // monthly totals report?
      bool daily_graph;                          // daily graph display
      bool daily_stats;                          // daily stats table        
      bool ctry_graph;                           // country graph display    
      bool shade_groups;                         // Group shading 0=no 1=yes 
      bool hlite_groups;                         // Group hlite 0=no 1=yes   
      bool use_https;                            // use 'https://' on URL's  
      bool graph_legend;                         // graph legend (1=yes)     
      bool fold_seq_err;                         // fold seq err (0=no)      
      bool hide_hosts;                           // Hide ind. sites (0=no)   
      bool html_ext_lang;                        // HTMLExtensionLang

      bool all_hosts;                            // List All sites (0=no)    
      bool all_urls;                             // List All URL's (0=no)    
      bool all_refs;                             // List All Referrers       
      bool all_agents;                           // List All User Agents     
      bool all_search;                           // List All Search Strings  
      bool all_users;                            // List All Usernames       
      bool all_errors;                           // List All Errors          
      bool all_downloads;                        // List All Downloads
      
      bool accept_host_names;                   // accept host names instead of IP addresses in the logs?
      bool local_utc_offset;                    // assign utc_offset using local time zone?

      u_long max_hosts;                         // maximum hosts   
      u_long max_hosts_kb; 
      u_long max_urls;                          // maximum URL's
      u_long max_urls_kb;
      u_long max_refs;                          // maximum referrers       
      u_long max_agents;                        // maximum user Agents     
      u_long max_search;                        // maximum search Strings  
      u_long max_users;                         // maximum usernames       
      u_long max_errors;                        // maximum errors          
      u_long max_downloads;                     // maximum downloads

      bool dump_hosts;                           // Dump tab delimited hosts 
      bool dump_urls;                            // URL's                    
      bool dump_refs;                            // Referrers                
      bool dump_agents;                          // User Agents              
      bool dump_users;                           // Usernames                
      bool dump_search;                          // Search strings           
      bool dump_header;                          // Dump header as first rec 
      bool dump_errors;                          // HTTP errors              
      bool dump_downloads;                       // Downloads
      
      bool use_classic_mangler;
      bool target_downloads;
      bool page_entry;
      bool use_ext_ent;                          // Use external XML entities?

      u_int	http_port;
      u_int	https_port;

      u_int mangle_agent;                        // mangle user agents       
      u_int group_domains;                       // Group domains 0=none     
      u_int group_url_domains;                   // Group domains 0=none     
      u_int graph_lines;                         // graph lines (0=none)     

      log_type_t log_type;

      u_int graph_border_width;

      u_int graph_background_alpha;

      u_int graph_background;
      u_int graph_gridline;
      u_int graph_shadow;
      u_int graph_title_color;
      u_int graph_hits_color;
      u_int graph_files_color;
      u_int graph_hosts_color;
      u_int graph_pages_color;
      u_int graph_visits_color;
      u_int graph_volume_color;
      u_int graph_outline_color;
      u_int graph_legend_color;
      u_int graph_weekend_color;

      u_int max_hist_length;

      u_long db_cache_size;                      // database cache size, in bytes
      u_int db_seq_cache_size;                   // database sequence cache size
      u_int db_trickle_rate;                     // database trickle rate
      bool db_direct;                            // use system buffering?
      bool db_dsync;                             // write-through?

      u_int swap_first_record;                   // first record to start swapping
      u_int swap_frequency;                      // swap every n'th record

      u_long visit_timeout;                      // visit timeout, in seconds (30 min)   
      u_long max_visit_length;                   // maximum visit length, in seconds
      u_long download_timeout;                   // download job timeout (seconds)

      u_long ntop_sites;                         // top n sites to display   
      u_long ntop_sitesK;                        // top n sites (by kbytes)  
      u_long ntop_urls;                          // top n url's to display   
      u_long ntop_urlsK;                         // top n url's (by kbytes)  
      u_long ntop_entry;                         // top n entry url's        
      u_long ntop_exit;                          // top n exit url's         
      u_long ntop_refs;                          // top n referrers ""       
      u_long ntop_agents;                        // top n user agents ""     
      u_long ntop_ctrys;                         // top n countries   ""     
      u_long ntop_search;                        // top n search strings     
      u_long ntop_users;                         // top n users to display   
      u_long ntop_errors;                        // top n HTTP errors        
      u_long ntop_downloads;                     // top n downlaods

      double font_size_small;
      double font_size_medium;

      string_t xsl_path;                         // XSL file path

      string_t apache_log_format;			       // is ApacheLogFormat valid?   
      string_t html_charset;
      string_t font_file_normal;
      string_t font_file_bold;

      string_t hname;                            // hostname for reports     
      string_t state_fname;                      // run state file name      
      string_t hist_fname;                       // name of history file     
      string_t html_ext;                         // HTML file prefix         
      string_t dump_ext;                         // Dump file prefix         
      string_t out_dir;                          // output directory         
      string_t cur_dir;
      string_t geoip_db_path;
      string_t dump_path;                        // Path for dump files      
      string_t html_css_path;
      string_t html_js_path;
      string_t rpt_title;
      string_t db_path;
      string_t db_fname;
      string_t db_fname_ext;
      string_t report_db_name;                   // path of a DB file for a report
      string_t log_dir;                          // optional log file directory
      string_t help_file;                        // XML help file path
      string_t help_xml;                         // content of the XML help file
      
      string_t graph_type;                       // graph type (PNG, Flash-OFC, etc)
      
      vector_t<string_t> log_fnames;            // all input log file names
      
      string_t dns_cache;                        // DNS cache file name      
      u_int dns_children;                        // # of DNS children        
      u_int dns_cache_ttl;						       // Default TTL of a DNS cache entry 

      bool ignored_domain_names;                 // true if ignored hosts contain domain patterns

      //
      // "Group" lists
      //
      glist group_sites;
      glist group_urls;
      glist group_refs;
      glist group_agents;
      glist group_users;
      glist search_list;                         // Search engine list
      glist includes;
      glist downloads;
      glist robots;

      //
      // "Hidden" lists
      //
      nlist hidden_hosts;
      nlist hidden_urls ;
      nlist hidden_refs;
      nlist hidden_agents;
      nlist hidden_users;

      //
      // "Ignored" lists
      //
      nlist ignored_hosts;
      nlist ignored_urls;
      nlist ignored_refs;
      nlist ignored_agents;
      nlist ignored_users;

      //
      // "Spam" lists
      //
      nlist spam_refs;                           // spam referrers

      //
      // "Include" lists
      //
      nlist include_sites;
      nlist include_urls;
      nlist include_refs;
      nlist include_agents;
      nlist include_users;

      nlist index_alias;                         // index. aliases           
      nlist html_pre;                            // before anything else :)  
      nlist html_head;                           // top HTML code            
      nlist html_body;                           // body HTML code           
      nlist html_post;                           // middle HTML code         
      nlist html_tail;                           // tail HTML code           
      nlist html_end;                            // after everything else    

      nlist page_type;                           // page view types          

      nlist incl_srch_args;
      nlist excl_srch_args;

      nlist excl_agent_args;                     // user agent arguments to exclude
      nlist incl_agent_args;                     // user agent arguments to include
      glist group_agent_args;                    // user agent argument group patterns
      
      nlist target_urls;
      nlist output_formats;                      // Report formats (HTML, CSV, XML)

      tm_ranges_t dst_ranges;                    // DST time ranges
      int dst_offset;                            // DST offset in minutes
      int utc_offset;                            // UTC offset in minutes, not including DST

      lang_t lang;
      
   private:
      static void get_config_cb(const char *fname, void *_this);

      void add_ignored_host(const char *value);

      void get_config(const char *fname);

      void proc_cmd_line(int argc, const char * const argv[]);

      void set_enable_phrase_values(bool enable);

      u_long get_db_cache_size(const char *value) const;

      int get_interval(const char *value, int div = 1) const;

      void process_includes(void);

      string_t& save_path_opt(const char *str, string_t& path) const;
      
      void add_output_format(const string_t& format);
      
      void read_help_xml(const char *fname);

      void set_dst_range(const string_t *dst_start, const string_t *dst_end);
      
      void proc_stdin_log_files(void);

   public:
      config_t(void);

      ~config_t(void);

      void initialize(const string_t& basepath, int argc, const char * const argv[]);

      bool ispage(const string_t& url) const;

      u_int get_url_type(u_short port, u_int urltype) const;
      
      string_t get_db_path(void) const;
      
      bool is_default_db(void) const;

      void report_config(void) const;
      
      bool is_maintenance(void) const;

      bool is_dns_enabled(void) const;
      
      const char *get_log_type_desc(void) const;

      int get_utc_offset(const tstamp_t& tstamp, tm_ranges_t::iterator& dst_iter) const;
};

#endif // __CONFIG_H
