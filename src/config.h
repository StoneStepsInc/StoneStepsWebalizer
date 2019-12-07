/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
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

#define BUFSIZE  16384                 /* Max buffer size for log record   */

/// Unit test classes that need access to private members.
namespace sswtest {
   class ConfigTest_GetInterval_Test;
   class ConfigTest_DSTRanges_Test;
}
      
///
/// @brief  A configuration object that reads application configuration files
///         and is accessible from any other application component.
///
/// Configuration files are processed first and should not output anything. 
/// Instead, messages are collected in a vector, and will be printed after 
/// the copyright message.
///
/// The `dns_lookups` flag is intended to disable resolving host addresses via
/// DNS look-ups, while still using the DNS database to maintain host address 
/// information that persists outside monthly state databases.
///
class config_t {
   friend class sswtest::ConfigTest_GetInterval_Test;
   friend class sswtest::ConfigTest_DSTRanges_Test;

   private:
      ///
      /// @struct dst_pair_t
      ///
      /// @brief  Textual unvalidated representation of a DST start and end time stamps.
      ///
      struct dst_pair_t {
         string_t   dst_start;                  ///< DST range start time stamp
         string_t     dst_end;                  ///< DST range end time stamp
      };
      
   private:
      std::vector<string_t> config_fnames;      ///< Configuration files used in this run
      std::vector<string_t> messages;           ///< Configuration warnings
      std::vector<string_t> errors;             ///< Configuration errors
      std::vector<dst_pair_t> dst_pairs;        ///< Intermediate textual DST ranges

   public:
      bool print_options;                       ///< Print help?
      bool print_version;                       ///< Print version?
      bool print_warranty;                      ///< Print warranty?
      bool time_me;                             ///< timing display flag      

      bool hide_robots;                         ///< hide robots?
      bool group_robots;                        ///< group robots?
      bool ignore_robots;                       ///< ignore robots altogether?

      // run modes (process log file if none is set)
      bool prep_report;                         ///< prepare a report from a database
      bool compact_db;                          ///< compact database
      bool db_info;                             ///< print database information
      bool end_month;                           ///< end active visits, update totals, etc for the current month
      
      // run flags
      bool incremental;                         ///< incremental mode (partial log procesing)
      bool batch;                               ///< batch processing (no report generated)
      
      bool pipe_log_names ;                     ///< Read log file names from stdin?
      bool last_log;                            ///< End month after this log file?

      bool conv_url_lower_case;                 ///< Convert URL to lower case (including query)
      bool bundle_groups;                       ///< Bundle groups at the top of the report?
      bool no_def_index_alias;                  ///< Ignore default index alias? 
      bool html_meta_noindex;                   ///< Add noindex, nofollow?      
      bool enable_phrase_values;                ///< Allow spaces in ignore/hide patterns?
      bool upstream_traffic;                    ///< Include request size in the transfer column?
      bool font_anti_aliasing;                  ///< Use True Type font antialiasing (PNG only)?
      bool graph_true_color;                    ///< Initialize PNG graphing engine sing True Color?
      bool sort_srch_args;                      ///< Sort search arguments by name?
      bool ignore_referrer_partial;             ///< Ignore referrer of partial content HTTP responses (code 206)?
      bool local_time;                          ///< Use local time for reports?
      bool ignore_hist;                         ///< Ignore history file when restoring incremental state?
      bool hourly_graph;                        ///< Show hourly graph?
      bool hourly_stats;                        ///< Show hourly stats table       
      bool enable_js;                           ///< Enable JavaScript in reports?
      bool monthly_totals_stats;                ///< monthly totals report?
      bool daily_graph;                         ///< daily graph display
      bool daily_stats;                         ///< daily stats table        
      bool ctry_graph;                          ///< country graph display    
      bool shade_groups;                        ///< Group shading 0=no 1=yes 
      bool hlite_groups;                        ///< Group hlite 0=no 1=yes   
      bool use_https;                           ///< use 'https://' on URL's  
      bool graph_legend;                        ///< graph legend (1=yes)     
      bool fold_seq_err;                        ///< fold seq err (0=no)      
      bool hide_hosts;                          ///< Hide ind. hosts (0=no)   
      bool html_ext_lang;                       ///< HTMLExtensionLang

      bool all_hosts;                           ///< List All hosts (0=no)    
      bool all_urls;                            ///< List All URL's (0=no)    
      bool all_refs;                            ///< List All Referrers       
      bool all_agents;                          ///< List All User Agents     
      bool all_search;                          ///< List All Search Strings  
      bool all_users;                           ///< List All Usernames       
      bool all_errors;                          ///< List All Errors          
      bool all_downloads;                       ///< List All Downloads

      bool hide_grp_items;                      ///< Hide grouped items?
      
      bool accept_host_names;                   ///< accept host names instead of IP addresses in the logs?
      bool local_utc_offset;                    ///< assign utc_offset using local time zone?

      bool geoip_city;                          ///< output city name in the reports?

      uint64_t max_hosts;                       ///< Maximum hosts in the main host report
      uint64_t max_hosts_kb;                    ///< Maximum hosts in the transfer report
      uint64_t max_urls;                        ///< Maximum URLs in the main report
      uint64_t max_urls_kb;                     ///< Maximum URLs in the transfer report
      uint64_t max_refs;                        ///< Maximum referrers in the report
      uint64_t max_agents;                      ///< Maximum user agents in the report
      uint64_t max_search;                      ///< Maximum search strings in the report
      uint64_t max_users;                       ///< Maximum user names in the report
      uint64_t max_errors;                      ///< Maximum errors in the report
      uint64_t max_downloads;                   ///< Maximum downloads in the report

      bool dump_hosts;                          ///< Dump tab delimited hosts?
      bool dump_urls;                           ///< Dump tab delimited URLs?
      bool dump_refs;                           ///< Dump tab delimited referrers?
      bool dump_agents;                         ///< Dump tab delimited user agents?
      bool dump_users;                          ///< Dump tab delimited user names?
      bool dump_search;                         ///< Dump tab delimited search strings?
      bool dump_header;                         ///< Dump a header row in all tab-delimited files?
      bool dump_errors;                         ///< Dump tab delimited HTTP errors?
      bool dump_downloads;                      ///< Dump tab delimited downloads?
      bool dump_countries;                      ///< Dump tab-delimited countries?
      bool dump_cities;                         ///< Dump tab-delimited cities?
      bool dump_asn;                            ///< Dump tab-delimited Autonomous System entries?
      
      bool use_classic_mangler;                 ///< Use the original Webalizer user agent mangling algorithm?
      bool target_downloads;                    ///< Consider downloads as visit targets?
      bool page_entry;                          ///< Consider only pages for entry and exit URLs?

      bool decimal_kbytes;                      ///< Use 1000, not 1024 as a transfer multiplier?
      bool classic_kbytes;                      ///< Output classic transfer amounts (xfer/1024)?

      u_short  http_port;                       ///< HTTP port for non-SSL traffic.
      u_short  https_port;                      ///< HTTP port for SSL traffic.

      u_int mangle_agent;                       ///< Mangle level for user agents.
      u_int group_domains;                      ///< Number of domain labels to use in domain grouping (0=none).
      u_int group_url_domains;                  ///< Number of domain labels to use in proxy domain grouping (0=none).
      u_int graph_lines;                        ///< Number of horizontal graph lines (PNG only)     

      log_type_t log_type;                      ///< Log file type

      u_int graph_border_width;                 ///< PNG graph border width, in pixels

      u_int graph_background_alpha;             ///< PNG graph background transparency, in percent (opaque=0, transparent=100)

      string_t graph_background;                ///< Chart background color in hex notation
      string_t graph_gridline;                  ///< Chart gridline color in hex notation
      string_t graph_shadow;                    ///< Chart shadow color in hex notation
      string_t graph_title_color;               ///< Chart title color in hex notation
      string_t graph_hits_color;                ///< Chart hits series color in hex notation
      string_t graph_files_color;               ///< Chart files series color in hex notation
      string_t graph_hosts_color;               ///< Chart hosts series color in hex notation
      string_t graph_pages_color;               ///< Chart pages series color in hex notation
      string_t graph_visits_color;              ///< Chart visits series color in hex notation
      string_t graph_xfer_color;                ///< Chart transfer series color in hex notation
      string_t graph_outline_color;             ///< Chart hits series color in hex notation
      string_t graph_legend_color;              ///< Chart column outlint color in hex notation (PNG only)
      string_t graph_weekend_color;             ///< Chart weekend label color in hex notation

      u_int max_hist_length;                    ///< Maximum history length, in months

      uint32_t db_cache_size;                   ///< Database cache size, in bytes.
      uint32_t db_seq_cache_size;               ///< Database sequence cache size, in elements.
      bool db_direct;                           ///< use system buffering?

      u_int visit_timeout;                      ///< visit timeout, in seconds (30 min)   
      u_int max_visit_length;                   ///< maximum visit length, in seconds
      u_int download_timeout;                   ///< download job timeout (seconds)

      uint32_t ntop_hosts;                      ///< top n hostss to display   
      uint32_t ntop_hostsK;                     ///< top n hosts (by kbytes)  
      uint32_t ntop_urls;                       ///< top n url's to display   
      uint32_t ntop_urlsK;                      ///< top n url's (by kbytes)  
      uint32_t ntop_entry;                      ///< top n entry url's        
      uint32_t ntop_exit;                       ///< top n exit url's         
      uint32_t ntop_refs;                       ///< top n referrers ""       
      uint32_t ntop_agents;                     ///< top n user agents ""     
      uint32_t ntop_ctrys;                      ///< top n countries   ""     
      u_int ntop_cities;                        ///< Top n cities.
      uint32_t ntop_search;                     ///< top n search strings     
      uint32_t ntop_users;                      ///< top n users to display   
      uint32_t ntop_errors;                     ///< top n HTTP errors        
      uint32_t ntop_downloads;                  ///< top n downlaods
      uint32_t ntop_asn;                        ///< Top ASN entries.

      double font_size_small;                   ///< PNG graph small font size, in points
      double font_size_medium;                  ///< PNG graph medium font size, in points

      string_t apache_log_format;               ///< Apache log line format   
      string_t font_file_normal;                ///< Normal True Type font file path
      string_t font_file_bold;                  ///< Bold True Type font file path

      string_t hname;                           ///< Host name for reports     
      string_t hist_fname;                      ///< Name of history file     
      string_t html_ext;                        ///< HTML file prefix         
      string_t dump_ext;                        ///< Dump file prefix         
      string_t out_dir;                         ///< Output directory         
      string_t cur_dir;                         ///< Current directory
      string_t geoip_db_path;                   ///< MaxMind GeoIP database path
      string_t asn_db_path;                     ///< MaxMind ASN database path
      string_t dump_path;                       ///< Path for dump files      
      string_t html_css_path;                   ///< URL path webalizer.css for reports
      string_t html_js_path;                    ///< URL path webalizer.js for reports
      string_t rpt_title;                       ///< Report title
      string_t db_path;                         ///< Monthly state database path
      string_t db_fname;                        ///< Monthly state database file name
      string_t db_fname_ext;                    ///< Monthly state database file extension
      string_t report_db_name;                  ///< Path to a state database file for a historical report
      string_t log_dir;                         ///< Optional log file directory

      string_t ext_map_url;                     ///< URL for a 3rd-party map service that can interpret latitude/longitude
      
      string_t graph_type;                      ///< graph type (PNG, Flash-OFC, etc)
      bool js_charts_map;                       ///< render country chart as a world map?
      string_t js_charts;                       ///< empty, Highcharts
      std::vector<string_t> js_charts_paths;    ///< alternative JavaScript charts framework paths
      
      std::vector<string_t> log_fnames;         ///< all input log file names
      
      string_t dns_cache;                       ///< DNS cache file name      
      string_t dns_db_path;                     ///< DNS cache directory
      string_t dns_db_fname;                    ///< DNS cache file name
      u_int dns_children;                       ///< Number of DNS resolver threads
      u_int dns_cache_ttl;                      ///< Default TTL of a DNS cache entry 

      bool dns_lookups;                         ///< Perform DNS look-ups for host addresses?

      //
      // "Group" lists
      //
      glist group_hosts;                        ///< Host grouping patterns
      glist group_urls;                         ///< URL grouping patterns
      glist group_refs;                         ///< Referrer grouping patterns
      glist group_agents;                       ///< User agent grouping patterns
      glist group_users;                        ///< User grouping patterns
      glist search_list;                        ///< Search engine identification patterns
      glist includes;                           ///< Configuration include paths
      glist downloads;                          ///< Download identification patterns
      glist robots;                             ///< Robot identification patterns

      //
      // "Hidden" lists
      //
      nlist hidden_hosts;                       ///< Host hide patterns
      nlist hidden_urls ;                       ///< URL hide patterns
      nlist hidden_refs;                        ///< Referrer hide patterns
      nlist hidden_agents;                      ///< User agent hide patterns
      nlist hidden_users;                       ///< User hide patterns

      //
      // "Ignored" lists
      //
      nlist ignored_hosts;                      ///< Host ignore patterns
      glist ignored_urls;                       ///< URL ignore patterns
      nlist ignored_refs;                       ///< Referrer ignore patterns
      nlist ignored_agents;                     ///< User agent ignore patterns
      nlist ignored_users;                      ///< User ignore patterns

      //
      // "Spam" lists
      //
      nlist spam_refs;                          ///< Spam referrer patterns

      //
      // "Include" lists
      //
      nlist include_hosts;                      ///< Host include patterns
      nlist include_urls;                       ///< URL include patterns
      nlist include_refs;                       ///< Referrer include patterns
      nlist include_agents;                     ///< User agent include patterns
      nlist include_users;                      ///< User include patterns

      nlist index_alias;                        ///< Website index file aliases
      nlist html_pre;                           ///< Inserted before DOCTYPE
      nlist html_head;                          ///< Inserted at the end of the head element
      nlist html_body;                          ///< Inserted at the start of the body element
      nlist html_post;                          ///< Inserted after the page header in the body element
      nlist html_tail;                          ///< Inserted before the page footer in the body element
      nlist html_end;                           ///< Inserted at the end of the body element

      nlist page_type;                          ///< Page extension patterns

      nlist incl_srch_args;                     ///< Search argument include patterns
      nlist excl_srch_args;                     ///< Search argument exclude patterns

      nlist excl_agent_args;                    ///< User agent argument exclude patterns
      nlist incl_agent_args;                    ///< User agent argument include patterns
      glist group_agent_args;                   ///< User agent argument grouping patterns
      
      nlist target_urls;                        ///< Target URL patterns (e.g. order page)
      nlist output_formats;                     ///< Report formats (HTML, CSV)

      nlist site_aliases;                       ///< Web site domain name aliases

      tm_ranges_t dst_ranges;                   ///< DST time ranges
      int dst_offset;                           ///< DST offset in minutes
      int utc_offset;                           ///< UTC offset in minutes, not including DST

      lang_t lang;                              ///< Selected language for reports and messages

      int verbose;                              ///< Verbosity level (2=verbose,1=err, 0=none)
      int debug_mode;                           ///< Debug mode flag

   private:
      static void get_config_cb(const char *fname, void *_this);

      void get_config(const char *fname);

      void proc_cmd_line(int argc, const char * const argv[]);

      void set_enable_phrase_values(bool enable);

      uint32_t get_db_cache_size(const char *value) const;

      int get_interval(const char *value, std::vector<string_t>& errors) const;

      bool process_includes(void);

      string_t& set_url_path(const char *str, string_t& path) const;
      
      void add_output_format(const string_t& format);
      
      void set_dst_range(const char *dst_start, const char *dst_end);
      
      void proc_stdin_log_files(void);

      void set_dns_db_path(const char *path);

      void deprecated_p_option(void);

      void validate_custom_html(void);

      void proc_dst_ranges(void);

      void add_def_ua_filters(void);

      void add_def_srch_list(void);

      void prep_and_validate(void);

      void add_grp_item(glist& grplist, nlist& hidlist, const char *value);
      
   public:
      config_t(void);

      ~config_t(void);

      void initialize(const string_t& basepath, int argc, const char * const argv[]);

      bool ispage(const string_t& url) const;

      u_char get_url_type(u_short port) const;
      
      bool is_secure_url(u_char urltype) const;

      /// Concatenates all state database path and file components and returns the combined path.
      string_t get_db_path(void) const;

      /// Concatenates the current state database file name and extension and returns the combined database name.
      string_t get_db_name(void) const;

      bool is_default_db(void) const;

      void report_config(void) const;
      
      void report_errors(void) const;

      /// Returns `true` if the configuration object is invalid, `false` otherwise.
      bool is_bad(void) const {return !errors.empty();}

      bool is_maintenance(void) const;

      bool is_dns_enabled(void) const;

      /// Returns `true` if JavaScript charts are enabled, `false` otherwise.
      bool use_js_charts(void) const {return !js_charts.isempty() || !js_charts_paths.empty();}
      
      const char *get_log_type_desc(void) const;

      int get_utc_offset(const tstamp_t& tstamp, tm_ranges_t::iterator& dst_iter) const;
};

#endif // CONFIG_H
