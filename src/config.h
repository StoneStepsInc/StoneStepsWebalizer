/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   config.h
*/

#ifndef CONFIG_H
#define CONFIG_H

#include "linklist.h"
#include "tstring.h"
#include "types.h"
#include "lang.h"
#include "tstamp.h"
#include "tmranges.h"
#include "logrec.h"

#include <vector>

#ifdef _WIN32
#define ETCDIR getenv("WINDIR")
#else
#ifndef ETCDIR
#error ETCDIR is not defined
#endif
#endif

#define DNS_CACHE_TTL     86400*30                 // Default TTL of an entry in the DNS cache (in seconds)

#define DNS_MAX_THREADS   100                      // Maximum number of DNS threads

#define DB_DEF_CACHE_SIZE  ((uint32_t) 0)          // Default cache size (ignored)
#define DB_MIN_CACHE_SIZE  ((uint32_t) 20 * 1024)  // Minimum cache size (20K)
#define DB_DEF_TRICKLE_RATE ((u_int) 5)            // Default database trickle rate

#define FONT_SIZE_SMALL       8.                   // points
#define FONT_SIZE_MEDIUM      10.                  // points

/* Port numbers */
#define PORT_UNKNOWN             ((u_short) 0)
#define DEF_HTTP_PORT            ((u_short) 80)
#define DEF_HTTPS_PORT           ((u_short) 443)

#define BUFSIZE  16384                 /* Max buffer size for log record   */
      
// -----------------------------------------------------------------
//
// config_t
//
// -----------------------------------------------------------------
// 1. Configuration files are processed first and should not output
// anything. Instead, messages are collected in a vector, and will
// be output after the copyright message has been printed.
//
// 2. dns_lookups is intended to disable resolving host addresses via
// DNS look-ups, while using the DNS database to maintain host address 
// information that persists outside monthly state databases.
//
class config_t {
   private:
      struct dst_pair_t {
         string_t   dst_start;
         string_t     dst_end;
      };
      
   private:
      std::vector<string_t> config_fnames;
      std::vector<string_t> messages;
      std::vector<string_t> errors;
      std::vector<dst_pair_t> dst_pairs;
      bool user_config;

   public:
      bool print_options;
      bool print_version;
      bool print_warranty;
      bool time_me;                             // timing display flag      

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
      bool no_def_index_alias;                   // ignore default index alias? 
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
      bool hide_hosts;                           // Hide ind. hosts (0=no)   
      bool html_ext_lang;                        // HTMLExtensionLang

      bool all_hosts;                            // List All hosts (0=no)    
      bool all_urls;                             // List All URL's (0=no)    
      bool all_refs;                             // List All Referrers       
      bool all_agents;                           // List All User Agents     
      bool all_search;                           // List All Search Strings  
      bool all_users;                            // List All Usernames       
      bool all_errors;                           // List All Errors          
      bool all_downloads;                        // List All Downloads
      
      bool accept_host_names;                   // accept host names instead of IP addresses in the logs?
      bool local_utc_offset;                    // assign utc_offset using local time zone?

      bool geoip_city;                          // output city name in the reports?

      uint64_t max_hosts;                         // maximum hosts   
      uint64_t max_hosts_kb; 
      uint64_t max_urls;                          // maximum URL's
      uint64_t max_urls_kb;
      uint64_t max_refs;                          // maximum referrers       
      uint64_t max_agents;                        // maximum user Agents     
      uint64_t max_search;                        // maximum search Strings  
      uint64_t max_users;                         // maximum usernames       
      uint64_t max_errors;                        // maximum errors          
      uint64_t max_downloads;                     // maximum downloads

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

      bool decimal_kbytes;                       // Use 1000, not 1024 as a transfer multiplier
      bool classic_kbytes;                       // Output classic transfer amounts (xfer/1024)

      u_short  http_port;
      u_short   https_port;

      u_int mangle_agent;                        // mangle user agents       
      u_int group_domains;                       // Group domains 0=none     
      u_int group_url_domains;                   // Group domains 0=none     
      u_int graph_lines;                         // graph lines (0=none)     

      log_type_t log_type;

      u_int graph_border_width;

      u_int graph_background_alpha;

      string_t graph_background;
      string_t graph_gridline;
      string_t graph_shadow;
      string_t graph_title_color;
      string_t graph_hits_color;
      string_t graph_files_color;
      string_t graph_hosts_color;
      string_t graph_pages_color;
      string_t graph_visits_color;
      string_t graph_xfer_color;
      string_t graph_outline_color;
      string_t graph_legend_color;
      string_t graph_weekend_color;

      u_int max_hist_length;

      uint32_t db_cache_size;                    // database cache size, in bytes
      uint32_t db_seq_cache_size;                // database sequence cache size
      uint32_t db_trickle_rate;                  // database trickle rate
      bool db_direct;                            // use system buffering?
      bool db_dsync;                             // write-through?

      u_int swap_first_record;                   // first record to start swapping
      u_int swap_frequency;                      // swap every n'th record

      u_int visit_timeout;                      // visit timeout, in seconds (30 min)   
      u_int max_visit_length;                   // maximum visit length, in seconds
      u_int download_timeout;                   // download job timeout (seconds)

      uint32_t ntop_hosts;                         // top n hostss to display   
      uint32_t ntop_hostsK;                        // top n hosts (by kbytes)  
      uint32_t ntop_urls;                          // top n url's to display   
      uint32_t ntop_urlsK;                         // top n url's (by kbytes)  
      uint32_t ntop_entry;                         // top n entry url's        
      uint32_t ntop_exit;                          // top n exit url's         
      uint32_t ntop_refs;                          // top n referrers ""       
      uint32_t ntop_agents;                        // top n user agents ""     
      uint32_t ntop_ctrys;                         // top n countries   ""     
      uint32_t ntop_search;                        // top n search strings     
      uint32_t ntop_users;                         // top n users to display   
      uint32_t ntop_errors;                        // top n HTTP errors        
      uint32_t ntop_downloads;                     // top n downlaods

      double font_size_small;
      double font_size_medium;

      string_t apache_log_format;                // is ApacheLogFormat valid?   
      string_t font_file_normal;
      string_t font_file_bold;

      string_t hname;                            // hostname for reports     
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
      string_t help_xml;                         // content of the XML help file
      
      string_t graph_type;                       // graph type (PNG, Flash-OFC, etc)
      bool js_charts_map;                        // render country chart as a world map?
      string_t js_charts;                        // empty, Highcharts
      std::vector<string_t> js_charts_paths;     // alternative JavaScript charts framework paths
      
      std::vector<string_t> log_fnames;          // all input log file names
      
      string_t dns_cache;                        // DNS cache file name      
      string_t dns_db_path;                      // DNS cache directory
      string_t dns_db_fname;                     // DNS cache file name
      u_int dns_children;                        // # of DNS children        
      u_int dns_cache_ttl;                       // Default TTL of a DNS cache entry 

      bool dns_lookups;                          // Perform DNS look-ups for host addresses?

      //
      // "Group" lists
      //
      glist group_hosts;
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
      nlist include_hosts;
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

      nlist site_aliases;                        // web site aliases

      tm_ranges_t dst_ranges;                    // DST time ranges
      int dst_offset;                            // DST offset in minutes
      int utc_offset;                            // UTC offset in minutes, not including DST

      lang_t lang;

      int verbose;                              // 2=verbose,1=err, 0=none
      int debug_mode;                           // debug mode flag

   private:
      static void get_config_cb(const char *fname, void *_this);

      void add_ignored_host(const char *value);

      void get_config(const char *fname);

      void proc_cmd_line(int argc, const char * const argv[]);

      void set_enable_phrase_values(bool enable);

      uint32_t get_db_cache_size(const char *value) const;

      int get_interval(const char *value, std::vector<string_t>& errors) const;

      void process_includes(void);

      string_t& save_path_opt(const char *str, string_t& path) const;
      
      void add_output_format(const string_t& format);
      
      void set_dst_range(const string_t *dst_start, const string_t *dst_end);
      
      void proc_stdin_log_files(void);

      void set_dns_db_path(const char *path);

      void deprecated_p_option(void);
      
   public:
      config_t(void);

      ~config_t(void);

      void initialize(const string_t& basepath, int argc, const char * const argv[]);

      bool ispage(const string_t& url) const;

      u_char get_url_type(u_short port) const;
      
      bool is_secure_url(u_char urltype) const;

      string_t get_db_path(void) const;

      bool is_default_db(void) const;

      void report_config(void) const;
      
      void report_errors(void) const;

      bool is_bad(void) const {return !errors.empty();}

      bool is_maintenance(void) const;

      bool is_dns_enabled(void) const;

      bool use_js_charts(void) const {return !js_charts.isempty() || !js_charts_paths.empty();}
      
      const char *get_log_type_desc(void) const;

      int get_utc_offset(const tstamp_t& tstamp, tm_ranges_t::iterator& dst_iter) const;
};

#endif // CONFIG_H
