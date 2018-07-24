/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   config.cpp
*/
#include "pch.h"

#include <cstdio>
#include <cctype>

#ifdef _WIN32
#include <io.h>
#include "platform/sys/utsname.h"
#else
#include <sys/utsname.h>
#endif

#include <climits>

#include "config.h"
#include "linklist.h"
#include "lang.h"
#include "exception.h"
#include "util_http.h"
#include "util_url.h"
#include "util_ipaddr.h"
#include "util_path.h"

///
/// @struct kwinfo
///
/// @brief  Maps a configuration variable name to a numeric key
///
struct kwinfo {
   const char *keyword;
   u_int key;
};

///
/// @brief  Constructs an instance of the application configuration object.
///
config_t::config_t(void)
{
   print_options = false;
   print_version = false;
   print_warranty = false;

   time_me = false;
   batch = false;

   hide_robots = false;
   group_robots = false;
   ignore_robots = false;

   last_log = false;
   prep_report = false;
   compact_db = false;
   db_info = false;
   end_month = false;

   pipe_log_names  = false;

   //
   // public values
   //
   conv_url_lower_case = false;               // if true, URL stems will be converted lower case
   bundle_groups = true;                      // bundle groups together?
   no_def_index_alias = false;                // ignore default index alias?
   html_meta_noindex = true;                  // add noindex, nofollow?
   enable_phrase_values = false;
   upstream_traffic = false;                  // track upstream traffic?
   font_anti_aliasing = true;
   graph_true_color = false;
   sort_srch_args = true;
   ignore_referrer_partial = true;
   local_time = true;                         // true=localtime false=GMT (UTC)
   enable_js = false;
   monthly_totals_stats = true;
   daily_graph = true;                        // daily graph display
   daily_stats = true;                        // daily stats table
   ctry_graph = true;                         // country graph display
   shade_groups = true;                       // Group shading 0=no 1=yes
   hlite_groups = true;                       /* Group hlite 0=no 1=yes   */
   incremental = true;                        /* incremental mode 1=yes   */
   use_https = false;                         /* use 'https://' on URL's  */
   graph_legend = true;                       /* graph legend (1=yes)     */
   fold_seq_err = false;                      /* fold seq err (0=no)      */
   hide_hosts = false;                        /* Hide ind. sites (0=no)   */
   html_ext_lang = false;

   ignore_hist = false;                       // history flag (1=skip)
   hourly_graph = true;                       // hourly graph display
   hourly_stats = true;                       // hourly stats table

   all_hosts = false;                         /* List All sites (0=no)    */
   all_urls = false;                          /* List All URL's (0=no)    */
   all_refs = false;                          /* List All Referrers       */
   all_agents = false;                        /* List All User Agents     */
   all_search = false;                        /* List All Search Strings  */
   all_users = false;                         /* List All Usernames       */
   all_errors = false;                        /* List All HTTP Errors     */
   all_downloads = false;                     // List All Downloads

   dump_hosts = false;                        /* Dump tab delimited sites */
   dump_urls = false;                         /* URL's                    */
   dump_refs = false;                         /* Referrers                */
   dump_agents = false;                       /* User Agents              */
   dump_users = false;                        /* Usernames                */
   dump_search = false;                       /* Search strings           */
   dump_header = false;                       /* Dump header as first rec */
   dump_errors = false;                       /* HTTP errors              */
   dump_downloads = false;                    // Downloads

   db_trickle_rate = DB_DEF_TRICKLE_RATE;
   db_cache_size = DB_DEF_CACHE_SIZE;         // Default cache size (ignored)
   db_seq_cache_size = 1024;
   db_direct = false;
   db_dsync = false;

   http_port = DEF_HTTP_PORT;                 // HTTP port number
   https_port = DEF_HTTPS_PORT;               // HTTPS port number

   mangle_agent = 1;                          /* mangle user agents       */
   group_domains= 0;                          /* Group domains 0=none     */
   group_url_domains = 0;                     /* Group URL domains 0=none */
   graph_lines  = 2;                          /* graph lines (0=none)     */
   log_type = LOG_IIS;                        // (0=clf, 1=ftp, 2=squid, 3=iis, 4=apache, 5=w3c)

   graph_border_width = 0;

   graph_background_alpha = 0;                // percent: opaque=0, transparent=100

   graph_type = "png";

   max_hist_length = 24;

   visit_timeout = 1800;                      /* visit timeout (seconds)  */
   max_visit_length = 0;
   download_timeout = 180;                    // download job timeout (seconds)

   ntop_hosts = 30;                           /* top n sites to display   */
   ntop_hostsK = 10;                          /* top n sites (by kbytes)  */
   ntop_urls = 30;                            /* top n url's to display   */
   ntop_urlsK = 10;                           /* top n url's (by kbytes)  */
   ntop_entry = 10;                           /* top n entry url's        */
   ntop_exit = 10;                            /* top n exit url's         */
   ntop_refs = 30;                            /* top n referrers ""       */
   ntop_agents = 15;                          /* top n user agents ""     */
   ntop_ctrys = 30;                           /* top n countries   ""     */
   ntop_search = 20;                          /* top n search strings     */
   ntop_users = 20;                           /* top n users to display   */
   ntop_errors = 20;                          /* top n HTTP result codes  */
   ntop_downloads=20;                         // top n downloads

   // restrict maximums to a reasonable number by default
   max_hosts = max_urls = max_refs = max_agents = max_search = max_users = max_errors = max_downloads = 500;
   
   // display only top hosts and URLs sorted by transfer
   max_hosts_kb = ntop_hostsK; 
   max_urls_kb = ntop_urlsK;

   font_size_small = FONT_SIZE_SMALL;
   font_size_medium = FONT_SIZE_MEDIUM;

   rpt_title = lang.msg_title;
   hist_fname = "webalizer.hist";             /* name of history file     */
   html_ext = "html";                         /* HTML file prefix         */
   dump_ext = "tab";                          /* Dump file prefix         */
   db_fname = "webalizer";                    // database file name
   db_fname_ext = "db";                       // database file extension
   report_db_name = "webalizer";              // report database file name

   dns_children = 0;                          /* DNS children (0=don't do)*/
   dns_cache_ttl= DNS_CACHE_TTL;              /* Default TTL of a DNS cache entry */
   dns_lookups = true;

   dst_offset = utc_offset = 0;

   use_classic_mangler = false;
   target_downloads = true;
   page_entry = true;
   
   decimal_kbytes = false;
   classic_kbytes = false;

   accept_host_names = false;

   // do not use the local machine's UTC offset by default
   local_utc_offset = false;

   geoip_city = true;

   // push the initial empty DST pair into the vector
   dst_pairs.push_back(dst_pair_t());

   verbose = 2;
   debug_mode = 0;

   js_charts_map = false;
}

///
/// @brief  Destroys an instance of the application configuration object.
///
config_t::~config_t(void)
{
   lang.cleanup_lang_data();
}

///
/// @brief  Checks custom HTML inserted via HTMLHead and other configuration variables
///         for disallowed HTML constructs.
///
void config_t::validate_custom_html(void)
{
   nlist::const_iterator iter;

   // HTMLPre should be used for custom scripts and not to misidentify the type of HTML used in reports
   for(iter = html_pre.begin(); iter != html_pre.end(); iter++) {
      if(strstr_ex(iter->string, "<!DOCTYPE", NULL, true)) {
         errors.emplace_back("The <!DOCTYPE> tag is not allowed in HTMLPre");
         break;
      }
   }

   // disallow a custom <body> tag because we need our JavaScript initialization to run
   for(iter = html_body.begin(); iter != html_body.end(); iter++) {
      if(strstr_ex(iter->string, "<body", NULL, true)) {
         errors.emplace_back("The start <body> tag is not allowed in HTMLBody");
         break;
      }
   }

   for(iter = html_end.begin(); iter != html_end.end(); iter++) {
      if(strstr_ex(iter->string, "</body", NULL, true)) {
         errors.emplace_back("The end </body> tag is not allowed in HTMLEnd");
         break;
      }
   }
}

///
/// @brief  Converts all DST start/end time pairs collected from configuration 
///         files into time stamps and populates the DST time range vector.
///
void config_t::proc_dst_ranges(void)
{
   // process all DST ranges we collected
   std::vector<dst_pair_t>::iterator dst_iter = dst_pairs.begin();
   
   while(dst_iter != dst_pairs.end()) {
      const dst_pair_t& dst_pair = *dst_iter++;
      
      // ignore the range if both timestamps are empty (the top one)
      if(dst_pair.dst_start.isempty() && dst_pair.dst_end.isempty())
         continue;
      
      // report if either start or end timestamp is missing
      if(dst_pair.dst_start.isempty())
         errors.push_back(string_t::_format("Missing DST start time this end time: %s", dst_pair.dst_end.c_str()));

      if(dst_pair.dst_end.isempty())
         errors.push_back(string_t::_format("Missing DST end time for this start time: %s", dst_pair.dst_start.c_str()));
      
      // create actual timestamps (always in local time)
      tstamp_t dst_start, dst_end;

      if(!dst_start.parse(dst_pair.dst_start, utc_offset))
         errors.push_back(string_t::_format("Ivalid DST start timestamp: %s", dst_pair.dst_start.c_str()));

      if(!dst_end.parse(dst_pair.dst_end, utc_offset))
         errors.push_back(string_t::_format("Ivalid DST end timestamp: %s", dst_pair.dst_end.c_str()));

      // and make sure the end time stamp has the DST offset from UTC
      if(local_time && dst_offset)
         dst_end.tolocal(utc_offset+dst_offset);

      // add the range if there's nothing wrong with it
      if(!dst_ranges.add_range(dst_start, dst_end))
         errors.push_back(string_t::_format("Cannot add a DST range from %s to %s", dst_pair.dst_start.c_str(), dst_pair.dst_end.c_str()));
   }
   
   // clean up a bit and clear the text timestamps
   dst_pairs.clear();
}

///
/// @brief  Adds default user agent filters based on the current mangle level.
///
void config_t::add_def_ua_filters(void)
{
   //
   // At the lowest level, remove cryptic or strings that have been
   // overused to the point they lost their meaning.
   //
   // Version numbers are not truncated at this level.
   //
   if(mangle_agent > 0) {
      excl_agent_args.add_nlist(".NET CLR*");      // .NET Common Language Runtime version
      excl_agent_args.add_nlist("compatible");     // IE's silly platform string
      excl_agent_args.add_nlist("T312461");        // IE's security patch (KB312461)
      excl_agent_args.add_nlist("Mozilla/*");      // overused Mozilla product version
      excl_agent_args.add_nlist("rv:*");           // Mozilla's source control revision number
   }

   //
   // Then remove various service pack versions, less-known component
   // names, HTML layout engines and Mozilla's security level
   // identifiers.
   //
   // Version numbers are truncated at this level to include the
   // minor version number.
   //
   if(mangle_agent > 1) {
      excl_agent_args.add_nlist("U");              // strong security (Mozilla)
      excl_agent_args.add_nlist("I");              // weak security (Mozilla)
      excl_agent_args.add_nlist("N");              // no security (Mozilla)

      excl_agent_args.add_nlist("Gecko/*");        // Mozilla's layout engine
      excl_agent_args.add_nlist("like Gecko");     // KHTML (like Gecko)
      excl_agent_args.add_nlist("KHTML/*");        // KHTML version
      excl_agent_args.add_nlist("KHTML, like Gecko");// KDE's layout engine

      // AppleWebKit/523.10.6 (KHTML, like Gecko) Version/3.0.4 Safari/523.10.6
      excl_agent_args.add_nlist("AppleWebKit*");   // AppleWebKit version
      excl_agent_args.add_nlist("Version/*");      //

      excl_agent_args.add_nlist("InfoPath*");      // MS Office InfoPath
      excl_agent_args.add_nlist("SLCC1");          // Vista SP?
      excl_agent_args.add_nlist("SV1");            // WinXP SP1
   }

   //
   // Rename Windows NT x.y sequences to the corresponding Windows
   // version names and remove the word Windows, which is reported
   // by FireFox as a platform identifier, along with the actual
   // Windows version.
   //
   // Versions are truncated at this level to include only the major
   // version number.
   //
   if(mangle_agent > 2) {
      excl_agent_args.add_nlist("Windows");        // Mozilla's Windows platform identifier

      // if phrase values are not enabled, enable them temporarily
      if(!enable_phrase_values)
         group_agent_args.set_enable_phrase_values(true);

      group_agent_args.add_glist("Windows NT 10.0\t Windows 10");
      group_agent_args.add_glist("Windows NT 6.2\t Windows 8");
      group_agent_args.add_glist("Windows NT 6.1\t Windows 7");
      group_agent_args.add_glist("Windows NT 6.0\t Windows Vista");
      group_agent_args.add_glist("Windows NT 5.1\t Windows XP");
      group_agent_args.add_glist("Windows NT 5.2\t Windows Server 2003");
      group_agent_args.add_glist("Windows NT 5.0\t Windows 2000");

      // restore the global setting
      group_agent_args.set_enable_phrase_values(enable_phrase_values);
   }

   //
   // Remove all platform and CPU identifiers. Versions at this level
   // are truncated to include only the product name.
   //
   if(mangle_agent > 3) {
      excl_agent_args.add_nlist("Windows*");       // Windows flavors
      excl_agent_args.add_nlist("WOW64");          // 32-bit IE running on a 64-bit Windows
      excl_agent_args.add_nlist("Media Center PC*");//
      excl_agent_args.add_nlist("Zune*");          //
      excl_agent_args.add_nlist("Tablet PC*");     //


      excl_agent_args.add_nlist("X11");            // Mozilla's Unix platform identifier
      excl_agent_args.add_nlist("Debian*");        // Debian flavors
      excl_agent_args.add_nlist("Fedora*");        // Fedora flavors
      excl_agent_args.add_nlist("Red Hat*");       // Red Hat flavors
      excl_agent_args.add_nlist("SUSE*");          // SUSE flavors
      excl_agent_args.add_nlist("Linux*");         // Mozilla's Linux OS/CPU identifier
      excl_agent_args.add_nlist("x86_64");         // Linux i686 (x86_64)
      excl_agent_args.add_nlist("Ubuntu*");        // Ubuntu flavors
      excl_agent_args.add_nlist("gutsy");          // Ubuntu/7.10 (gutsy)

      excl_agent_args.add_nlist("Intel Mac OS X*");// Mozilla's Mac OS X OS/CPU identifier
      excl_agent_args.add_nlist("PPC Mac OS X*");  // Mozilla's PPC OS/CPU identifier
      excl_agent_args.add_nlist("Macintosh");      // Mozilla's Mac platform identifier
   }
}

///
/// @brief  Populate the search engine list with a few most commonly used search 
///         engine domain patterns and their search arguments.
///
void config_t::add_def_srch_list(void)
{
   search_list.add_glist("www.google.\t   q=");
   search_list.add_glist("www.google.\t   as_q=All Words",        true);
   search_list.add_glist("www.google.\t   as_epq=Exact Phrase",   true);
   search_list.add_glist("www.google.\t   as_oq=Any Word",        true);
   search_list.add_glist("www.google.\t   as_eq=Without Words",   true);
   search_list.add_glist("www.google.\t   as_filetype=File Type", true);
   search_list.add_glist("search.yahoo.\t p=");
   search_list.add_glist("search.yahoo.\t vf=File Type",          true);
   search_list.add_glist("www.bing.\t     q=");
   search_list.add_glist("ask.com\t       q=");
   search_list.add_glist("about.com\t     terms=");
}

///
/// @brief  Reads configuration files and processes command line options to
///         initialize the configuration object.
///
/// Configuration is read and processed in this order:
///
///   * The default configuration file 
///   * Configuration files included in the default configuration file
///   * Command line options, intermixed with additional configuration
///     files referenced via one or more `-c` options.
///   * Configuration files included in command line configuration files.
///
/// If a single configuration value, such as `TopURLs`, is found more than once
/// while processing these steps, its value is overwritten every subsequent time. 
/// If a cumulative configuration value, such as `IgnoreURL`, is found multiple
/// times, each subsequent value is added to the existing set of values.
///
/// Log file names are treated differently from other configuration values in 
/// that log file names collected while processing one source of log file names
/// are cleared if any names are found while processing the next source of log
/// file names. The three distinct sources of log file names are: a) the default
/// configuration file and its includes, b) the command line and its includes 
/// and c) standard input, if `--pipe-log-names` is used. For example, if log
/// files `A` and `B` are specified in `webalizer.conf`, and log files `C` and 
/// `D` are specified on the command line, then only `C` and `D` will be processed. 
///
/// Configuration issues are classified as warning messages, errors and 
/// unrecoverable errors.
///
/// Warnings are collected in the `messages` vector and will be reported when 
/// `report_config` is called. The configuration object can be used without any 
/// restrictions in this case. 
///
/// Errors are collected in the `errors` vector and if this vector is not empty, 
/// the configuration object is invalid and cannot be used. Use `config_t::is_bad` 
/// to detect if there are any configuration errors. Configuration errors can be 
/// reported via `report_errors`.
///
/// Unrecoverable errors are thrown as exceptions and the configuration object
/// should not be used in this case. 
///
void config_t::initialize(const string_t& basepath, int argc, const char * const argv[])
{
   const char *cp1;
   string_t fpath;

   //
   // Store the current directory to resolve relative paths
   //
   cur_dir = basepath;

   //
   // If no user configuration file was provided, try to locate the
   // default one in the
   //
   //    - current directory
   //    - common system directory
   //    - directory where the executable is
   //
   fpath = make_path(cur_dir, "webalizer.conf");
   if (!access(fpath, R_OK))
      get_config(fpath);
   else {
      fpath = make_path(ETCDIR, "webalizer.conf");
      if (!access(fpath, R_OK))
         get_config(fpath);
      else {
         if(argv[0] && *argv[0]) {
            cp1 = strchr(argv[0], 0);

            while(cp1 > argv[0] && *cp1 != '/' && *cp1 != '\\') cp1--;

            if(cp1 > argv[0]) {
               fpath.assign(argv[0], cp1-argv[0]+1);
               fpath.append("webalizer.conf");

               if (!access(fpath,R_OK))
                  get_config(fpath);
            }
         }
      }
   }

   // stop reading configuration if there are any errors
   if(!errors.empty())
      return;

   // process includes found in the main configuration file
   if(!process_includes())
      return;

   // keep track of how many log files were added (see method description for details)
   size_t logcnt = log_fnames.size();

   // process command line arguments
   proc_cmd_line(argc, argv);

   // stop reading configuration if there are any errors
   if(!errors.empty())
      return;

   // process includes queued from the command line
   if(!process_includes())
      return;

   // if any log files were added via the command line, remove those from earlier
   if(logcnt < log_fnames.size()) {
      log_fnames.erase(log_fnames.begin(), log_fnames.begin() + logcnt);
      logcnt = log_fnames.size();
   }

   // check stdin for log file names
   if(pipe_log_names)
      proc_stdin_log_files();

   // similarly, if any log files were added via stdin, remove those from earlier
   if(logcnt < log_fnames.size())
      log_fnames.erase(log_fnames.begin(), log_fnames.begin() + logcnt);

   prep_and_validate();
}

///
/// @brief  Provides additional configuration initializations and validations.
///
/// This method should not print any messages because they will appear before
/// the version text. Instead, warning messages should be added to `messages`, 
/// which is dumped to the standard output at a later time.
///
void config_t::prep_and_validate(void)
{
   uint32_t max_ctry;                      /* max countries defined       */
   utsname system_info;

   // check if custom HTML contains any disallowed tags
   validate_custom_html();

   // turn off batch mode if incremental is not specified
   if(batch && (!incremental || prep_report)) {
      messages.push_back(string_t("WARNING: Batch processing only works in the incremental mode"));
      batch = false;
   }

   //
   // If the UTC offset wasn't set and we weren't asked to skip this step, 
   // find out and use the UTC offset of the local machine.
   //
   if(local_utc_offset) {
      // get current UTC time
      time_t now = time(NULL);
      struct tm *utctm = gmtime(&now);

      //
      // mktime takes a tm structure containing local time, but we pass one
      // with UTC time, so the resulting time_t value will be offset from 
      // the actual UTC time_t by the number of seconds for this time zone. 
      // Note that tm::tm_isdst is set by gmtime to zero, which tells mktime 
      // to treat the argument as standard (non-DST) local time.
      //
      utc_offset = (int) (now - mktime(utctm)) / 60;
   }

   //
   // Make sure group_hosts does not contain bare IP addresses, as it
   // would cause collisions in hm_htab and the database. In order to
   // group by a single address, give this group a name that is not an
   // IP address:
   //
   // GroupHost   127.0.0.1   localhost
   //
   for(glist::iterator grph_iter = group_hosts.begin(); grph_iter != group_hosts.end(); grph_iter++) {
      if(is_ip_address(grph_iter->name)) {
         grph_iter->name = string_t::_format("group_%s", grph_iter->name.c_str());
         messages.push_back(string_t::_format("WARNING: A bare IP address in GroupHost is changed to %s\n", grph_iter->name.c_str()));
      }
   }
   
   //
   // If log_fnames is empty, insert one empty entry to force stdin (i.e. at most one)
   //
   if(!log_fnames.size())
      log_fnames.push_back(string_t());
      
   // treat a single dash as stdin
   std::vector<string_t>::iterator iter = log_fnames.begin();
   while(iter != log_fnames.end()) {
      string_t& fname = *iter++;
      if(fname.length() == 1 && fname[0] == '-')
         fname.clear();
   }

   //
   // If the log file path is relative and the log directory is not empty, combine 
   // the two to form the complete path. Note, though, that it's close to impossible 
   // to figure out two file names pointing the the same file. For example, these
   // two paths:
   //
   //    ..\logfiles\ex110203.log 
   //    c:\site_a\logfiles\ex110203.log 
   //
   // , may refer to the same file. 
   //
   if(!log_dir.isempty()) {
      std::vector<string_t>::iterator iter = log_fnames.begin();
      while(iter != log_fnames.end()) {
         string_t& fname = *iter++;
         // ignore empty file names (used to read stdin) and absolute paths
         if(fname.length() && !is_abs_path(fname))
            fname = make_path(log_dir, fname);
      }
   }

   //
   // If output directory is not set, use the current directory
   //
   if(out_dir.isempty() || !is_abs_path(out_dir))
      out_dir = make_path(cur_dir, out_dir);

   //
   // Initialize database path and file name
   //
   if(db_path.isempty())
      db_path = out_dir;

   if(db_fname.isempty())
      db_fname = "webalizer";

   if(db_fname_ext.isempty())
      db_fname = "db";

   if(db_seq_cache_size < 256)
      db_seq_cache_size = 256;

   if(db_trickle_rate > 100)
      db_trickle_rate = 100;

   // check DNS/GeoIP settings
   if(dns_children) {
      if(dns_children > DNS_MAX_THREADS)
         dns_children = DNS_MAX_THREADS;

      //
      // If DNS look-ups are enabled, DNS cache database must be configured to store
      // resolved host names. Otherwise, a GeoIP database must be configured to keep
      // workers busy. The DNS cache database may be optionally opened in the latter
      // case to preserve IP address information, such as whether it was used by a 
      // spammer. Note that after this point the number of DNS workers greater than 
      // zero indicates that DNS/GeoIP functionality is enabled. Non-empty database
      // paths will be validated by the DNS resolver.
      //
      if(dns_lookups && dns_cache.isempty() || !dns_lookups && geoip_db_path.isempty())
         errors.emplace_back(lang.msg_dns_init);
   }

   // enable JavaScript in reports if we have the source file
   enable_js = !html_js_path.isempty();

   proc_dst_ranges();

   // if no output format was specified, add HTML
   if(!output_formats.size())
      add_output_format(string_t("html"));

   // if a TSV format was specified, set all dump_x flags
   if(output_formats.isinlist(string_t("tsv"))) {
      dump_hosts = dump_urls = dump_refs = dump_agents = true;
      dump_users = dump_search = dump_errors = dump_downloads = true;
   }
   else {
      // otherwise, add a TSV format if there is at least one DumpX property set
      if(dump_hosts || dump_urls || dump_refs || dump_agents ||
            dump_users || dump_search || dump_errors || dump_downloads) {
         add_output_format(string_t("tsv"));
      }
   }

   /* add default index. alias */
   if(!no_def_index_alias)
      index_alias.add_nlist("index.");

   //
   // Check if page types present. If no page types specified, we
   // use the default ones here...
   //
   if (page_type.isempty())
   {
      page_type.add_nlist("htm");
      page_type.add_nlist("html");
      page_type.add_nlist("txt");

      if (log_type == LOG_CLF || log_type == LOG_APACHE)
      {
         page_type.add_nlist("php");
         page_type.add_nlist("cgi");
      }
      else if(log_type == LOG_IIS) {
         page_type.add_nlist("asp");
         page_type.add_nlist("aspx");
      }
      if (log_type == LOG_SQUID)
      {
         page_type.add_nlist("php");
         page_type.add_nlist("asp");
         page_type.add_nlist("aspx");
         page_type.add_nlist("cgi");
      }

      if (!page_type.isinlist(html_ext))
         page_type.add_nlist(html_ext);
   }

   // if there is no GeoIP database configured, don't output country information
   if(geoip_db_path.isempty()) 
      ntop_ctrys = 0;
   else {
      for (max_ctry=0; lang.ctry[max_ctry].desc; max_ctry++);
      if (ntop_ctrys > max_ctry) ntop_ctrys = max_ctry;   /* force upper limit */
   }

   if (graph_lines> 20)       graph_lines= 20;         /* keep graphs sane! */

   /* ensure entry/exits don't exceed urls */
   uint32_t i = (ntop_urls > ntop_urlsK) ? ntop_urls : ntop_urlsK;
   if(ntop_entry > i) ntop_entry=i;
   if(ntop_exit > i)  ntop_exit=i;
   
   // make sure maximums are not less than tops
   if(max_hosts < ntop_hosts) max_hosts = ntop_hosts;
   if(max_hosts_kb < ntop_hostsK) max_hosts_kb = ntop_hostsK;
   if(max_urls < ntop_urls) max_urls = ntop_urls;
   if(max_urls_kb < ntop_urlsK) max_urls_kb = ntop_urlsK;
   if(max_refs < ntop_refs) max_refs = ntop_refs;
   if(max_agents < ntop_agents) max_agents = ntop_agents;
   if(max_search < ntop_search) max_search = ntop_search;
   if(max_users < ntop_users) max_users = ntop_users;
   if(max_errors < ntop_errors) max_errors = ntop_errors;
   if(max_downloads < ntop_downloads) max_downloads = ntop_downloads;

   if(search_list.isempty())
      add_def_srch_list();

   if(!use_classic_mangler) {
      //
      // If the user agent argument exclusion and group lists are empty
      // and mangle_agent is a non-zero value, add some filters.
      //
      if(excl_agent_args.isempty() && group_agent_args.isempty())
         add_def_ua_filters();
   }

   // convert all site aliases to lower case
   for(nlist::iterator site_alias_it = site_aliases.begin(); site_alias_it != site_aliases.end(); site_alias_it++)
      site_alias_it->string.tolower();

   // if no site name was supplied, use the reporting server host name
   if (hname.isempty()) {
      if (uname(&system_info))
         hname = "localhost";
      else
         hname = system_info.nodename;
         hname.tolower();
   }

   // add the primary site host name to the list of site aliases
   site_aliases.add_nlist(hname.c_str());
}

///
/// @brief  Compares two configuration variable names
///
/// @returns   A value less than zero if the keyword in `e1` is less than the one in `e2`, 
///            a zero if the keywords in `e1` and `e2` are equal and a value greater than 
///            zero if the keyword in `e1` is greater than the one in `e2`.
///
static int cmp_conf_kw(const void *e1, const void *e2)
{
   return string_t::compare_ci(((kwinfo*)e1)->keyword, ((kwinfo*)e2)->keyword);
}

///
/// @brief  Reads the specified configuration file.
///
void config_t::get_config(const char *fname)
{
   static kwinfo kwords[] = {
                     //
                     // This array *must* be sorted alphabetically
                     //
                     // max key: 191; empty slots: 147, 151, 152 
                     //
                     {"AcceptHostNames",     186},          // Accept host names instead of IP addresses?
                     {"AllAgents",           67},           // List all User Agents?
                     {"AllDownloads",        123},          // List all downloads
                     {"AllErrors",           114},          // List All HTTP Errors?
                     {"AllHosts",            64},
                     {"AllReferrers",        66},           // List all Referrers?
                     {"AllSearchStr",        68},           // List all Search Strings?
                     {"AllSites",            64},           // List all sites?
                     {"AllURLs",             65},           // List all URLs?
                     {"AllUsers",            69},           // List all Users?
                     {"ApacheLogFormat",     95},           // Apache LogFormat Directive
                     {"Batch",               156},          // Batch processing?
                     {"BatchProcessing",     156},          // Batch processing?
                     {"BundleGroups",        91},           // Bundle groups together?
                     {"ClassicKBytes",       184},          // Output classic transfer amounts (xfer/1024)
                     {"ConvURLsLowerCase",   89},           // Convert URL's to lower case
                     {"CountryGraph",        54},           // Display ctry graph (0=no)
                     {"DailyGraph",          86},           // Daily Graph (0=no)
                     {"DailyStats",          87},           // Daily Stats (0=no)
                     {"DbCacheSize",         146},          // State database cache size
                     {"DbDirect",            153},          // Use OS buffering?
                     {"DbDSync",             154},          // Use write-through?
                     {"DbExt",               148},          // State database file extension
                     {"DbName",              145},          // State database file name
                     {"DbPath",              144},          // State database path
                     {"DbSeqCacheSize",      149},          // Database sequence cache size
                     {"DbTrickleRate",       150},          // Database trickle rate
                     {"Debug",               8},            // Produce debug information
                     {"DecimalKBytes",       172},          // Use 1000, not 1024 as a transfer multiplier
                     {"DNSCache",            84},           // DNS Cache file name
                     {"DNSCacheTTL",         93},           // TTL of a DNS cache entry (days)
                     {"DNSChildren",         85},           // DNS Children (0=no DNS)
                     {"DNSLookups",          190},          // Perform DNS look-ups for host addresses?
                     {"DownloadPath",        119},          // Download path
                     {"DownloadTimeout",     120},          // Download job timeout
                     {"DSTEnd",              162},          // Daylight saving end date/time
                     {"DSTOffset",           163},          // Daylight saving offset
                     {"DSTStart",            161},          // Daylight saving start date/time
                     {"DumpAgents",          81},           // Dump user agents tab file
                     {"DumpDownloads",       121},          // Dump downloads?
                     {"DumpErrors",          112},          // Dump HTTP errors?
                     {"DumpExtension",       76},           // Dump filename extension
                     {"DumpHeader",          77},           // Dump header as first rec?
                     {"DumpHosts",           78},
                     {"DumpPath",            75},           // Path for dump files
                     {"DumpReferrers",       80},           // Dump referrers tab file
                     {"DumpSearchStr",       83},           // Dump search str tab file
                     {"DumpSites",           78},           // Dump sites tab file
                     {"DumpURLs",            79},           // Dump urls tab file
                     {"DumpUsers",           82},           // Dump usernames tab file
                     {"EnablePhraseValues",  117},          // Enable phrases in configuration values
                     {"ExcludeAgentArgs",    164},          // Exclude user agent arguments
                     {"ExcludeSearchArg",    109},          // Exclude a search argument
                     {"ExternalMapURL",      191},          // An external map URL to show IP address locations
                     {"GeoIPCity",           53},           // Output city name in reports?
                     {"GeoIPDBPath",         141},          // Path to the GeoIP database file
                     {"GMTTime",             30},           // Local or UTC time?
                     {"GraphBackgroundAlpha",130},          // Graph background transparency
                     {"GraphBackgroundColor",105},          // Graph background color
                     {"GraphBorderWidth",    129},          // Graph border width
                     {"GraphFilesColor",     133},          // Graph files color
                     {"GraphFontBold",       101},          // True Type font path
                     {"GraphFontMedium",     103},          // Medium font size in points
                     {"GraphFontNormal",     100},          // True Type font path
                     {"GraphFontSmall",      102},          // Small font size in points
                     {"GraphFontSmoothing",  104},          // Use font anti-aliasing?
                     {"GraphGridlineColor",  124},          // Graph griline color
                     {"GraphHitsColor",      132},          // Graph hits color
                     {"GraphHostsColor",     134},          // Graph hosts color
                     {"GraphLegend",         51},           // Graph Legends (yes/no)
                     {"GraphLegendColor",    139},          // Graph legend color
                     {"GraphLines",          52},           // Graph Lines (0=none)
                     {"GraphOutlineColor",   138},          // Graph bar outline color
                     {"GraphPagesColor",     135},          // Graph pages color
                     {"GraphShadowColor",    106},          // Graph legend shadow color
                     {"GraphTitleColor",     131},          // Graph title color
                     {"GraphTransferColor",  137},          // Graph transfer color
                     {"GraphTrueColor",      111},          // Create true-color images?
                     {"GraphType",           115},          // Graph type (PNG, Flash-OFC, etc)
                     {"GraphVisitsColor",    136},          // Graph visits color
                     {"GraphVolumeColor",    137},          // Graph volume (transfer) color
                     {"GraphWeekendColor",   140},          // Graph weekend color
                     {"GroupAgent",          34},           // Group Agents
                     {"GroupAgentArgs",      167},          // Group agent arguments
                     {"GroupDomains",        62},           // Group domains (n=level)
                     {"GroupHighlight",      36},           // BOLD Grouped entries
                     {"GroupHost",           32},
                     {"GroupReferrer",       33},           // Group Referrers
                     {"GroupRobots",         159},          // Group Robots?
                     {"GroupShading",        35},           // Shade Grouped entries
                     {"GroupSite",           32},           // Group Sites
                     {"GroupURL",            31},           // Group URL's
                     {"GroupURLDomains",     126},          // Group URL domains (proxy)
                     {"GroupUser",           74},           // Usernames to group
                     {"HideAgent",           19},           // User Agents to hide
                     {"HideAllHosts",        63},
                     {"HideAllSites",        63},           // Hide ind. sites (0=no)
                     {"HideHost",            16},
                     {"HideReferrer",        18},           // Referrers to hide
                     {"HideRobots",          157},          // Hide robots?
                     {"HideSite",            16},           // Sites to hide
                     {"HideURL",             17},           // URL's to hide
                     {"HideUser",            71},           // Usernames to hide
                     {"HistoryLength",       143},          // Number of months in history
                     {"HistoryName",         39},           // Filename for history data
                     {"HostName",            4},            // Hostname to use
                     {"HourlyGraph",         9},            // Hourly stats graph
                     {"HourlyStats",         10},           // Hourly stats table
                     {"HTMLBody",            42},           // HTML body code
                     {"HTMLCssPath",         98},           // URL path to webalizer.css
                     {"HTMLEnd",             43},           // HTML code at end
                     {"HTMLExtension",       40},           // HTML filename extension
                     {"HTMLExtensionLang",   94},           // HTMLExtensionLang
                     {"HTMLHead",            21},           // HTML Top1 code
                     {"HTMLJsPath",          128},          // HTML JavaScript Path
                     {"HTMLMetaNoIndex",     110},          // Add noindex, nofollow?
                     {"HTMLPost",            22},           // HTML Top2 code
                     {"HTMLPre",             41},           // HTML code at beginning
                     {"HTMLTail",            23},           // HTML Tail code
                     {"HttpPort",            96},           // HTTP port number
                     {"HttpsPort",           97},           // HTTPS port number
                     {"IgnoreAgent",         28},           // User Agents to ignore
                     {"IgnoreHist",          5},            // Ignore history file
                     {"IgnoreHost",          25},           // Synonym for IgnoreSite
                     {"IgnoreReferrer",      27},           // Referrers to ignore
                     {"IgnoreReferrerPartial",125},         // Partial request (206)
                     {"IgnoreRobots",        158},          // Ignore robots altogether?
                     {"IgnoreSite",          25},           // Sites to ignore
                     {"IgnoreURL",           26},           // Url's to ignore
                     {"IgnoreUser",          72},           // Usernames to ignore
                     {"Include",             118},          // Include a config file
                     {"IncludeAgent",        48},           // User Agents to include
                     {"IncludeAgentArgs",    165},          // User agent argument to include
                     {"IncludeHost",         45},
                     {"IncludeReferrer",     47},           // Referrers to include
                     {"IncludeSearchArg",    108},          // Include a search argument
                     {"IncludeSite",         45},           // Sites to always include
                     {"IncludeURL",          46},           // URL's to always include
                     {"IncludeUser",         73},           // Usernames to include
                     {"Incremental",         37},           // Incremental runs
                     {"IndexAlias",          20},           // Aliases for index.html
                     {"JavaScriptCharts",    99},           // JavaScript charts package name
                     {"JavaScriptChartsMap", 38},           // Render country chart as a world map?
                     {"JavaScriptChartsPath",189},          // Alternative JavaScript charts path
                     {"LanguageFile",        90},           // Language file
                     {"LocalUTCOffset",      188},          // Do not use local UTC offset?
                     {"LogDir",              183},          // Log directory
                     {"LogFile",             2},            // Log file to use for input
                     {"LogType",             60},           // Log Type (clf/ftp/squid/iis)
                     {"MangleAgents",        24},           // Mangle User Agents
                     {"MaxAgents",           176},          // Maximum User Agents
                     {"MaxDownloads",        180},          // Maximum downloads
                     {"MaxErrors",           179},          // Maximum HTTP Errors
                     {"MaxHosts",            173},          // Maximum hosts
                     {"MaxKHosts",           181},          // Maximum hosts (transfer)
                     {"MaxKURLs",            182},          // Maximum URLs (transfer)
                     {"MaxReferrers",        175},          // Maximum Referrers
                     {"MaxSearchStr",        177},          // Maximum Search Strings
                     {"MaxURLs",             174},          // Maximum URLs
                     {"MaxUsers",            178},          // Maximum Users
                     {"MaxVisitLength",      187},          // Maximum visit length
                     {"MonthlyTotals",       127},          // Output monthly totals report?
                     {"NoDefaultIndexAlias", 92},           // Ignore default index alias?
                     {"OutputDir",           1},            // Output directory
                     {"OutputFormat",        171},          // Output format
                     {"PageEntryURL",        170},          // Show only pages in the entry report?
                     {"PageType",            49},           // Page Type (pageview)
                     {"Quiet",               6},            // Run in quiet mode
                     {"ReallyQuiet",         29},           // Dont display ANY messages
                     {"ReportTitle",         3},            // Title for reports
                     {"Robot",               155},          // Robot user agent filter
                     {"SearchEngine",        61},           // SearchEngine strings
                     {"SiteAlias",           185},          // One or more site aliases
                     {"SiteName",            4},            // Synonym for HostName
                     {"SortSearchArgs",      107},          // Sort search arguments ?
                     {"SpamReferrer",        142},          // Spam referrer
                     {"TargetDownloads",     169},          // Treat download URLs as targets?
                     {"TargetURL",           168},          // Target URL pattern
                     {"TimeMe",              7},            // Produce timing results
                     {"TopAgents",           14},           // Top User Agents
                     {"TopCountries",        15},           // Top Countries
                     {"TopDownloads",        122},          // Top downloads
                     {"TopEntry",            57},           // Top Entry Pages
                     {"TopErrors",           113},          // Top HTTP errors
                     {"TopExit",             58},           // Top Exit Pages
                     {"TopHosts",            11},
                     {"TopKHosts",           55},
                     {"TopKSites",           55},           // Top sites (by KBytes)
                     {"TopKURLs",            56},           // Top URL's (by KBytes)
                     {"TopReferrers",        13},           // Top Referrers
                     {"TopSearch",           59},           // Top Search Strings
                     {"TopSites",            11},           // Top sites
                     {"TopURLs",             12},           // Top URL's
                     {"TopUsers",            70},           // Top Usernames to show
                     {"UpstreamTraffic",     88},           // Track upstream traffic?
                     {"UseClassicMangleAgents",166},        // Use classic MangleAgents?
                     {"UseHTTPS",            44},           // Use https:// on URL's
                     {"UTCOffset",           160},          // UTC/local time difference
                     {"UTCTime",             30},           // Local or UTC time?
                     {"VisitTimeout",        50}            // Visit timeout (seconds)
                   };

   FILE *fp;

   string_t keyword, value;
   const char *cp1, *cp2;
   u_int num_kwords = sizeof(kwords)/sizeof(kwords[0]);
   kwinfo *kptr, key = {NULL, 0};
   string_t::char_buffer_t buffer;

   config_fnames.push_back(string_t(fname));

   if ( (fp=fopen(fname,"r")) == NULL)
   {
      errors.push_back(string_t::_format("%s %s\n", lang.msg_bad_conf, fname));
      return;
   }

   buffer.resize(BUFSIZE, 0);

   while ( (fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      cp1 = buffer;

      // check for the UTF-8 BOM sequence
      if(*cp1 == '\xEF' && *(cp1+1) == '\xBB' && *(cp1+2) == '\xBF')
         cp1 += 3;

      /* skip comments and blank lines */
      if(*cp1 =='#' || string_t::iswspace(*cp1) ) continue;

      // skip to the first non-alphanum character
      cp2 = cp1;
      while(string_t::isalnum(*cp2)) cp2++;
      
      // hold onto the keyword
      keyword.assign(cp1, cp2-cp1);

      // skip leading whitespace before the value
      cp1 = cp2;
      while(*cp1 != '\n' && *cp1 != '\0' && string_t::isspace(*cp1)) cp1++;
      
      // find the end of the line
      cp2 = cp1;
      while(*cp2 != '\n' && *cp2 != '\0') cp2++;
      
      // move backwards until we find a non-whitespace character
      while(cp2 != cp1 && string_t::iswspace(*cp2)) cp2--;
      if(*cp2 != 0 && *cp2 != '\n') cp2++;

      // ignore empty values
      if(cp2 == cp1) continue;
      
      // grab the value
      value.assign(cp1, cp2-cp1);

      /* check if blank keyword/value */
      if(keyword.isempty() || value.isempty()) continue;

      key.keyword = keyword;
      if((kptr = (kwinfo*) bsearch(&key, kwords, num_kwords, sizeof(kwords[0]), cmp_conf_kw)) == NULL) {
         /* Invalid keyword       */
         messages.push_back(string_t::_format("%s \"%s\" (%s)\n", lang.msg_bad_key, keyword.c_str(), fname));
         continue;
      }

      switch (kptr->key) {
         case 1:  out_dir=value; break;                           // OutputDir
         case 2:  log_fnames.push_back(value);break;              // LogFile
         case 3:  rpt_title = value; break;                       // ReportTitle
         case 4:  hname=value; break;                             // HostName
         case 5:  ignore_hist=(string_t::tolower(value[0])=='y'); break;    // IgnoreHist
         case 6:  verbose=(value[0]=='n')?2:1; break;             // Quiet
         case 7:  time_me=(value[0]=='n')?0:1; break;             // TimeMe
         case 8:  debug_mode=(value[0]=='n')?0:1; break;          // Debug
         case 9:  hourly_graph=(value[0]=='n')?false:true; break; // HourlyGraph
         case 10: hourly_stats=(value[0]=='n')?false:true; break; // HourlyStats
         case 11: ntop_hosts = atoi(value); break;                // TopSites
         case 12: ntop_urls = atoi(value); break;                 // TopURLs
         case 13: ntop_refs = atoi(value); break;                 // TopRefs
         case 14: ntop_agents = atoi(value); break;               // TopAgents
         case 15: ntop_ctrys = atoi(value); break;                // TopCountries
         case 16: hidden_hosts.add_nlist(value); break;           // HideSite
         case 17: hidden_urls.add_nlist(value); break;            // HideURL
         case 18: hidden_refs.add_nlist(value); break;            // HideReferrer
         case 19: hidden_agents.add_nlist(value); break;          // HideAgent
         case 20: index_alias.add_nlist(value); break;            // IndexAlias
         case 21: html_head.add_nlist(value); break;              // HTMLHead
         case 22: html_post.add_nlist(value); break;              // HTMLPost
         case 23: html_tail.add_nlist(value); break;              // HTMLTail
         case 24: mangle_agent=atoi(value); break;                // MangleAgents
         case 25: ignored_hosts.add_nlist(value); break;          // IgnoreSite, IgnoreHost
         case 26: ignored_urls.add_glist(value, true); break;     // IgnoreURL
         case 27: ignored_refs.add_nlist(value); break;           // IgnoreReferrer
         case 28: ignored_agents.add_nlist(value); break;         // IgnoreAgent
         case 29: if (value[0]=='y') verbose=0; break;            // ReallyQuiet
         case 30: local_time=(string_t::tolower(*value)=='y') ? false : true; break; // GMTTime
         case 31: group_urls.add_glist(value); break;             // GroupURL
         case 32: group_hosts.add_glist(value); break;            // GroupSite
         case 33: group_refs.add_glist(value); break;             // GroupReferrer
         case 34: group_agents.add_glist(value); break;           // GroupAgent
         case 35: shade_groups=(string_t::tolower(value[0])=='y'); break;   // GroupShading
         case 36: hlite_groups=(string_t::tolower(value[0])=='y'); break;   // GroupHighlight
         case 37: incremental=(value[0]=='y')?true:false; break;  // Incremental
         case 38: js_charts_map = string_t::tolower(*value) == 'y'; break;
         case 39: hist_fname=value; break;                        // History FName
         case 40: html_ext=value; break;                          // HTML extension
         case 41: html_pre.add_nlist(value); break;               // HTML Pre code
         case 42: html_body.add_nlist(value); break;              // HTML Body code
         case 43: html_end.add_nlist(value); break;               // HTML End code
         case 44: use_https=(string_t::tolower(value[0])=='y'); break;      // Use https://
         case 45: include_hosts.add_nlist(value); break;          // IncludeSite
         case 46: include_urls.add_nlist(value); break;           // IncludeURL
         case 47: include_refs.add_nlist(value); break;           // IncludeReferrer
         case 48: include_agents.add_nlist(value); break;         // IncludeAgent
         case 49: page_type.add_nlist(value); break;              // PageType
         case 50: visit_timeout=get_interval(value, errors); break;  // VisitTimeout
         case 51: graph_legend=(string_t::tolower(value[0])=='y'); break;   // GraphLegend
         case 52: graph_lines = atoi(value); break;               // GraphLines
         case 53: geoip_city = (string_t::tolower(value[0])=='y'); break;   // GeoIPCity
         case 54: ctry_graph=(string_t::tolower(value[0])=='y'); break;     // CountryGraph
         case 55: ntop_hostsK = atoi(value); break;               // TopKSites (KB)
         case 56: ntop_urlsK  = atoi(value); break;               // TopKUrls (KB)
         case 57: ntop_entry  = atoi(value); break;               // Top Entry pgs
         case 58: ntop_exit   = atoi(value); break;               // Top Exit pages
         case 59: ntop_search = atoi(value); break;               // Top Search pgs
         case 60: log_type=(string_t::tolower(value[0])=='s')?
              LOG_SQUID:(string_t::tolower(value[0])=='c')?
              LOG_CLF: (string_t::tolower(value[0])=='a')?
              LOG_APACHE : (string_t::tolower(value[0])=='w')?
              LOG_W3C : LOG_IIS;
              break;                                              // LogType
         case 61: search_list.add_glist(value, true); break;      // SearchEngine
         case 62: group_domains=atoi(value); break;               // GroupDomains
         case 63: hide_hosts=(string_t::tolower(value[0])=='y'); break;     // HideAllSites
         case 64: all_hosts=(string_t::tolower(value[0])=='y'); break;      // All Sites?
         case 65: all_urls=(string_t::tolower(value[0])=='y'); break;       // All URL's?
         case 66: all_refs=(string_t::tolower(value[0])=='y'); break;       // All Refs
         case 67: all_agents=(string_t::tolower(value[0])=='y'); break;     // All Agents?
         case 68: all_search=(string_t::tolower(value[0])=='y'); break;     // All Srch str
         case 69: all_users=(string_t::tolower(value[0])=='y'); break;      // All Users?
         case 70: ntop_users=atoi(value); break;                  // TopUsers
         case 71: hidden_users.add_nlist(value); break;           // HideUser
         case 72: ignored_users.add_nlist(value); break;          // IgnoreUser
         case 73: include_users.add_nlist(value); break;          // IncludeUser
         case 74: group_users.add_glist(value); break;            // GroupUser
         case 75: dump_path=value; break;                         // DumpPath
         case 76: dump_ext=value; break;                          // Dumpfile ext
         case 77: dump_header=(string_t::tolower(value[0])=='y'); break;    // DumpHeader?
         case 78: dump_hosts=(string_t::tolower(value[0])=='y'); break;     // DumpSites?
         case 79: dump_urls=(string_t::tolower(value[0])=='y'); break;      // DumpURLs?
         case 80: dump_refs=(string_t::tolower(value[0])=='y'); break;      // DumpReferrers?
         case 81: dump_agents=(string_t::tolower(value[0])=='y'); break;    // DumpAgents?
         case 82: dump_users=(string_t::tolower(value[0])=='y'); break;     // DumpUsers?
         case 83: dump_search=(string_t::tolower(value[0])=='y'); break;    // DumpSrchStrs?
         case 84: set_dns_db_path(value); break;                  // DNSCache fname
         case 85: dns_children=atoi(value); break;                // DNSChildren
         case 86: daily_graph=(string_t::tolower(value[0])=='y'); break;    // HourlyGraph
         case 87: daily_stats=(string_t::tolower(value[0])=='y'); break;    // HourlyStats
         case 88: upstream_traffic = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 89: conv_url_lower_case = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 90: lang.proc_lang_file(value, errors); break;
         case 91: bundle_groups = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 92: no_def_index_alias = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 93: dns_cache_ttl = atoi(value) * 86400; if(dns_cache_ttl == 0) dns_cache_ttl = DNS_CACHE_TTL; break;
         case 94: html_ext_lang = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 95: apache_log_format = value; break;
         case 96: http_port = (u_short) atoi(value); break;
         case 97: https_port = (u_short) atoi(value); break;
         case 98: set_url_path(value, html_css_path); break;
         case 99: js_charts = value.tolower(); break;
         case 100: font_file_normal = value; break;
         case 101: font_file_bold = value; break;
         case 102: font_size_small = atof(value); break;
         case 103: font_size_medium = atof(value); break;
         case 104: font_anti_aliasing = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 105: graph_background = value; break;
         case 106: graph_shadow = value; break;
         case 107: sort_srch_args = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 108: incl_srch_args.add_nlist(value);   break;
         case 109: excl_srch_args.add_nlist(value);   break;
         case 110: html_meta_noindex = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 111: graph_true_color = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 112: dump_errors = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 113: ntop_errors = atoi(value); break;
         case 114: all_errors=(string_t::tolower(value[0])=='y'); break;    // List all errors?
         case 115: graph_type = value.tolower(); break;
         case 117: set_enable_phrase_values(string_t::tolower(value[0]) == 'y'); break;
         case 118: includes.add_glist(value); break;
         case 119: downloads.add_glist(value); break;
         case 120: download_timeout = get_interval(value, errors); break;
         case 121: dump_downloads = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 122: ntop_downloads = atoi(value); break;
         case 123: all_downloads=(string_t::tolower(value[0]) == 'y'); break;
         case 124: graph_gridline = value; break;
         case 125: ignore_referrer_partial = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 126: group_url_domains=atoi(value); break;
         case 127: monthly_totals_stats=(string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 128: set_url_path(value, html_js_path); break;
         case 129: graph_border_width = atoi(value); if(graph_border_width > 7) graph_border_width = 7; break;
         case 130: graph_background_alpha = atoi(value); if(graph_background_alpha > 100) graph_background_alpha = 100; break;
         case 131: graph_title_color = value; break;
         case 132: graph_hits_color = value; break;
         case 133: graph_files_color = value; break;
         case 134: graph_hosts_color = value; break;
         case 135: graph_pages_color = value; break;
         case 136: graph_visits_color = value; break;
         case 137: graph_xfer_color = value; break;
         case 138: graph_outline_color = value; break;
         case 139: graph_legend_color = value; break;
         case 140: graph_weekend_color = value; break;
         case 141: geoip_db_path = value; break;
         case 142: spam_refs.add_nlist(value); break;
         case 143: max_hist_length = atoi(value); break;
         case 144: db_path = value; break;
         case 145: db_fname = value; break;
         case 146: db_cache_size = get_db_cache_size(value); break;
         case 148: db_fname_ext = value; break;
         case 149: db_seq_cache_size = atoi(value); break;
         case 150: db_trickle_rate = atoi(value); break;
         case 153: db_direct = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 154: db_dsync = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 155: robots.add_glist(value); break;
         case 156: batch = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 157: hide_robots = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 158: ignore_robots = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 159: group_robots = (string_t::tolower(value[0]) == 'y') ? true : false; break;
         case 160: utc_offset = get_interval(value, errors) / 60; break;  // in minutes
         case 161: set_dst_range(value, NULL); break;
         case 162: set_dst_range(NULL, value); break;
         case 163: dst_offset = get_interval(value, errors) / 60; break;  // in minutes
         case 164: excl_agent_args.add_nlist(value); break;
         case 165: incl_agent_args.add_nlist(value); break;
         case 166: use_classic_mangler = (string_t::tolower(value[0]) == 'y'); break;
         case 167: group_agent_args.add_glist(value); break;
         case 168: target_urls.add_nlist(value); break;
         case 169: target_downloads = (string_t::tolower(value[0]) == 'y'); break;
         case 170: page_entry = (string_t::tolower(value[0]) == 'y'); break;
         case 171: add_output_format(value.tolower()); break;
         case 172: decimal_kbytes = (string_t::tolower(value[0]) == 'y'); break;
         case 173: max_hosts=str2ul(value); break;
         case 174: max_urls=str2ul(value); break;
         case 175: max_refs=str2ul(value); break;
         case 176: max_agents=str2ul(value); break;
         case 177: max_search=str2ul(value); break;
         case 178: max_users=str2ul(value); break;
         case 179: max_errors=str2ul(value); break;
         case 180: max_downloads=str2ul(value); break;
         case 181: max_hosts_kb=str2ul(value); break;
         case 182: max_urls_kb=str2ul(value); break;
         case 183: log_dir = value; break;
         case 184: classic_kbytes = (string_t::tolower(value[0]) == 'y'); break;
         case 185: site_aliases.add_nlist(value); break;
         case 186: accept_host_names = (string_t::tolower(value[0]) == 'y'); break;
         case 187: max_visit_length = get_interval(value, errors); break;
         case 188: local_utc_offset =  (string_t::tolower(value[0]) == 'y'); break;
         case 189: js_charts_paths.push_back(value); break;
         case 190: dns_lookups =  (string_t::tolower(value[0]) == 'y'); break;
         case 191: ext_map_url = value; break;
      }
   }

   fclose(fp);
}

///
/// @brief  Adds the output format type to the list.
///
/// No output format type validation is done at this point because the configuration 
/// object doesn't have information about which output genrators are available.
///
void config_t::add_output_format(const string_t& format)
{
   if(!output_formats.isinlist(format))
      output_formats.add_nlist(format);
}

///
/// @brief  A callback function to read the specified configuration file.
///
void config_t::get_config_cb(const char *fname, void *_this)
{
   if(_this != NULL)
      ((config_t*)_this)->get_config(fname);
}

///
/// @brief  Reads configuration files without a host name pattern or those with a 
///         pattern that matches the host name for the current report.
///
/// Included configuration files are collected during two distinct phases - when
/// the primary configuration file is being processed and when command line options
/// are being processed. After each phase the included configuration files that 
/// matched the pattern are removed from the list, so they are not processed more 
/// than once.
///
bool config_t::process_includes(void)
{
   // walk the includes and remove from the list all includes for which the callback was called 
   includes.for_each(hname, get_config_cb, this, true, true);

   // indicate success if there are no configuration errors
   return errors.empty();
}

///
/// @brief  Reports configuration messages collected while processing configuration
///         files and all configuration files used for this run.
///
void config_t::report_config(void) const
{
   unsigned int i;

   if(verbose > 1) {
      // report any messages collected during initialization
      for(i = 0; i < messages.size(); i++)
         printf("%s\n", messages[i].c_str());

      // report processed configuration files
      for(i = 0; i < config_fnames.size(); i++)
         printf("%s %s\n", lang.msg_use_conf, config_fnames[i].c_str());
   }
}

///
/// @brief  Reports configuration errors collected while processing configuration
///         files.
///
void config_t::report_errors(void) const
{
   for(size_t i = 0; i < errors.size(); i++)
      fprintf(stderr, "%s\n", errors[i].c_str());
}

///
/// @brief  Assigns `str` to `path` and ensures that a non-empty path ends with 
///         a forward slash.
///
string_t& config_t::set_url_path(const char *str, string_t& path) const
{
   path.reset();

   if(str == NULL || *str == 0)
      return path;

   path = str;

   if(path[path.length()-1] != '/')
      path += '/';

   return path;
}

///
/// @brief  Returns `true` if the specified URL ends with one of the page extensions
///         or with a forward slash, `false` otherwise.
///
bool config_t::ispage(const string_t& url) const
{
   const char *cp1, *cp2, *cp3;

   // consider empty URL same as `/`
   if(url.isempty())
      return true;

   cp3 = &url[url.length()];
   cp2 = cp3-1;

   // consider a directory as a page (i.e. an index or default page)
   if(*cp2 == '/')
      return true;

   cp1 = url;

   // find the last period or a forward slash
   while(cp2 != cp1) {
      if(*cp2 == '.' || *cp2 == '/')
         break;
      cp2--;
   }

   // if we didn't find a dot, it's the same as directory (e.g. `/abc`)
   if(*cp2 == '/' || cp2 == cp1)
      return true;

   cp2++;

   // look up the file extension in the page extension list
   return page_type.isinlist(string_t::hold(cp2, cp3-cp2)) != NULL;
}

///
/// @brief  Compares the specified port to those in the configuration and returns
///         the port type (HTTP, HTTPS or other).
///
u_char config_t::get_url_type(u_short port) const
{
   // keep the port type unknown if it was not set
   if(port == 0)
      return URL_TYPE_UNKNOWN;

   if(port == http_port)
      return (u_char) URL_TYPE_HTTP;

   if(port == https_port)
      return (u_char) URL_TYPE_HTTPS;

   return (u_char) URL_TYPE_OTHER;
}

///
/// @brief  Returns `true` if URL type is HTTPS or if `UseHTTPS` is set and the URL 
///         was requested at least once over HTTPS or if the port is unknown, `false` 
///         otherwise.
///
bool config_t::is_secure_url(u_char urltype) const
{
   // check for HTTPS only and, if UseHTTPS is set, check if the port is unknown or there was at least one HTTPS request
   return urltype == URL_TYPE_HTTPS || (use_https && (urltype & URL_TYPE_HTTPS || urltype == URL_TYPE_UNKNOWN));
}

///
/// @brief  Evaluates command line arguments and sets corresponding configuration 
///         vaues.
///
/// Command line arguments are processed before configuration files and can be 
/// overwritten by configuration file values.
///
void config_t::proc_cmd_line(int argc, const char * const argv[])
{
   size_t nlen;
   int optind;
   const char *nptr, *vptr;
   bool longopt;

   // start with one to skip the executable path
   for(optind = 1; optind < argc; optind++) {

      // check if a non-option
      if(*argv[optind] != '-') {
         // if there's no option, it's either a database name or the log file path
         if(prep_report || compact_db || db_info)
            report_db_name = argv[optind];
         else {
            // ignore the log file path if --end-month is found (use the default database)
            if(!end_month)
               log_fnames.push_back(string_t(argv[optind]));
         }
         continue;
      }

      // long options begin with a double dash
      if(argv[optind][1] == '-') {
         longopt = true;
         nptr =  &argv[optind][2];
         if((vptr = strchr(nptr, ':')) != NULL || (vptr = strchr(nptr, '=')) != NULL)
            nlen = vptr++ - nptr;
         else
            nlen = strlen(nptr);
      }
      else {
         longopt = false;
         nlen = 1;
         nptr =  &argv[optind][1];

         // check if it's an argument that takes a value
         if(!strchr("aAcCDeEFgIlmMnNoPrRsStuUx", *nptr))
            vptr = NULL;
         else {
            vptr = argv[++optind];

            // error out if the next argument is missing or is another option
            if(!vptr || *vptr == '-')
               throw exception_t(0, string_t::_format("Missing option value for -%c", *nptr));
         }
      }

      // process long options
      if(longopt) {
         if(!string_t::compare_ci(nptr, "help", nlen))
            print_options = true;
         else if(!string_t::compare_ci(nptr, "prepare-report", nlen))
            prep_report = true;
         else if(!string_t::compare_ci(nptr, "last-log", nlen))
            last_log = true;
         else if(!string_t::compare_ci(nptr, "version", nlen))
            print_version = true;
         else if(!string_t::compare_ci(nptr, "warranty", nlen))
            print_warranty = true;
         else if(!string_t::compare_ci(nptr, "batch", nlen))
            batch = true;
         else if(!string_t::compare_ci(nptr, "compact-db", nlen))
            compact_db = true;
         else if(!string_t::compare_ci(nptr, "end-month", nlen))
            end_month = true;
         else if(!string_t::compare_ci(nptr, "db-info", nlen))
            db_info = true;
         else if(!string_t::compare_ci(nptr, "pipe-log-names", nlen))
            pipe_log_names  = true;
         else
            messages.push_back(string_t::_format("WARNING: Unknown option --%s\n", string_t(nptr, nlen).c_str()));

         continue;
      }

      // process classic one-character options
      switch (*nptr) {
          case 'a': hidden_agents.add_nlist(vptr); break;            // Hide agents
          case 'A': ntop_agents=atoi(vptr); break;                   // Top agents
          case 'c': get_config(vptr); break;                         // User Config file
          case 'C': ntop_ctrys=atoi(vptr); break;                    // Top countries
          case 'd': debug_mode=1; break;                             // Debug
          case 'D': set_dns_db_path(vptr); break;                    // DNS Cache filename
          case 'e': ntop_entry=atoi(vptr); break;                    // Top entry pages
          case 'E': ntop_exit=atoi(vptr); break;                     // Top exit pages
          case 'F': log_type=(string_t::tolower(vptr[0])=='s')?
               LOG_SQUID: (string_t::tolower(vptr[0])=='c')?
               LOG_CLF: (string_t::tolower(vptr[0])=='a')?
               LOG_APACHE: (string_t::tolower(vptr[0])=='w')?
               LOG_W3C: LOG_IIS; 
            break;                            // define log type
          case 'g': group_domains=atoi(vptr);break;                  // GroupDomains (0=no)
          case 'G': hourly_graph=false; break;                       // no hourly graph
          case 'h': print_options = true; break;                   // help
          case '?': print_options = true; break;                   // help
          case 'H': hourly_stats=false; break;                       // no hourly stats
          case 'i': ignore_hist=true; break;                         // Ignore history
          case 'I': index_alias.add_nlist(vptr); break;              // Index alias
          case 'l': graph_lines=atoi(vptr); break;                   // Graph Lines
          case 'L': graph_legend=false; break;                           // Graph Legends
          case 'm': visit_timeout=get_interval(vptr, errors); break; // Visit Timeout
          case 'M': mangle_agent=atoi(vptr); break;                  // mangle user agents
          case 'n': hname=vptr; break;                               // Hostname
          case 'N': dns_children=atoi(vptr); break;                  // # of DNS children
          case 'o': out_dir=vptr; break;                             // Output directory
          case 'p': deprecated_p_option(); break;                      // Incremental run (obsolete)
          case 'P': page_type.add_nlist(vptr); break;                // page view types
          case 'q': verbose=1; break;                                // Quiet (verbose=1)
          case 'Q': verbose=0; break;                                // Really Quiet
          case 'r': hidden_refs.add_nlist(vptr); break;              // Hide referrer
          case 'R': ntop_refs=atoi(vptr); break;                     // Top referrers
          case 's': hidden_hosts.add_nlist(vptr); break;             // Hide site
          case 'S': ntop_hosts=atoi(vptr); break;                    // Top sites
          case 't': rpt_title=vptr; break;                           // Report title
          case 'T': time_me=1; break;                                // TimeMe
          case 'u': hidden_urls.add_nlist(vptr); break;              // hide URL
          case 'U': ntop_urls=atoi(vptr); break;                     // Top urls
          case 'v':
          case 'V': print_version = true; break;                     // Version
          case 'w':
          case 'W': print_warranty = true; break;                    // Warranty
          case 'x': html_ext=vptr; break;                            // HTML file extension
          case 'X': hide_hosts=true; break;                             // Hide ind. sites
          case 'Y': ctry_graph=false; break;                         // Suppress ctry graph
      }
   }
}

///
/// @brief  Sets a flag in the configuration to indicate whether a space in an
///         ignore/hide/etc pattern ends the pattern and starts a name or is
///         considered a part of the pattern and a tab serves as a pattern/name
///         separator.
///
void config_t::set_enable_phrase_values(bool enable)
{
   enable_phrase_values = enable;

   search_list.set_enable_phrase_values(enable);
   group_urls.set_enable_phrase_values(enable);
   group_hosts.set_enable_phrase_values(enable);
   group_refs.set_enable_phrase_values(enable);
   group_agents.set_enable_phrase_values(enable);
   group_users.set_enable_phrase_values(enable);
   downloads.set_enable_phrase_values(enable);
   includes.set_enable_phrase_values(enable);
   robots.set_enable_phrase_values(enable);
   group_agent_args.set_enable_phrase_values(enable);
   ignored_urls.set_enable_phrase_values(enable);
}

///
/// @brief  Converts text representation of the state database cache size value 
///         into a number. Suffixes `K` and `M` are interpreted as kilo and mega 
///         multipliers.
///
uint32_t config_t::get_db_cache_size(const char *value) const
{
   uint32_t cachesize;
   char *cp1;

   if(value == NULL)
      return DB_DEF_CACHE_SIZE;

   cachesize = strtoul(value, &cp1, 10);

   if(cachesize == 0 || cachesize == ULONG_MAX)
      return DB_DEF_CACHE_SIZE;

   // skip spaces, if any
   while(*cp1 == ' ') cp1++;

   // process optional suffixes
   if(cp1) {
      switch(toupper(*cp1)) {
         case 'K':
            cachesize *= 1024;
            break;

         case 'M':
            cachesize *= 1024 * 1024;
            break;
      }
   }

   return cachesize < DB_MIN_CACHE_SIZE ? DB_MIN_CACHE_SIZE : cachesize;
}

///
/// @brief  Concatenates all path components to the state database and returns the
///         combined state database path.
///
string_t config_t::get_db_path(void) const
{
   return make_path(db_path, (is_default_db()) ? db_fname : report_db_name) + '.' + db_fname_ext;
}

///
/// @brief  Splits the path to the DNS database onto the file name and the directory
///         path and stores them in the configuration.
///
void config_t::set_dns_db_path(const char *path)
{
   // ignore if empty path
   if(!path || !*path)
      return;

   // skip to the end of the path, so we can find the last path separator
   const char *cp = path;
   for(cp = path; *cp; cp++);

   // hold on to the path length
   size_t plen = cp - path;

   // walk the string back until we find a slash or walk past the first character
   while(cp >= path) {
      if(*cp == '/' || *cp == '\\')
         break;
      cp--;
   }

   // if we didn't find a path separator, use the current directory
   if(cp < path) {
      dns_db_path = get_cur_path();
      dns_db_fname.assign(path, plen);
      dns_cache = make_path(dns_db_path, dns_db_fname);
      return;
   }

   // include the slash in the path
   cp++;

   // split an absolute path onto a directory and a file name
   if(is_abs_path(path)) {
      dns_db_path.assign(path, cp - path);
      dns_db_fname.assign(cp, plen - (cp - path));
      dns_cache.assign(path, plen);
      return;
   }

   // hold onto the relative path in the argument path
   string_t rpath(path, cp - path);

   // set up path components
   dns_db_path = make_path(get_cur_path(), rpath);
   dns_db_fname.assign(cp, plen - (cp - path));
   dns_cache = make_path(dns_db_path, dns_db_fname);
}

///
/// @brief  Returns `true` if the default state database should be used or `false 
///         if a user-supplied database can be used instead.
///
bool config_t::is_default_db(void) const
{
   return !prep_report && !compact_db && !db_info;
}

///
/// @brief  Converts a string representation of a time iterval to the number of 
///         seconds.
/// 
/// Optional suffixes `h`, `m` and `s` are recognized and interpreted as hours, 
/// minutes and seconds. In the latter case no numeric conversion is done, but 
/// the suffix may still be useful to make it clear what units are used. Only 
/// the first character of the suffix is evaluated, so hour, hours, hrs, etc, 
/// are all the same. Intervals may be negative (e.g. -5h) or positive (e.g. 5h 
/// or +5h). 
///
/// There is not special return value for an error, but if a malformed interval 
/// string is passed in, the method returns a zero and the error will be pushed 
/// into the errors vector. 
///
int config_t::get_interval(const char *value, std::vector<string_t>& errors) const
{
   int time;
   const char *cp;

   if(!value || !*value)
      return 0;

   time = (int) str2l(value, &cp);
   
   if(!cp) {
      errors.push_back(string_t::_format("Invalid time interval value (%s)", value));
      return 0;
   }

   // skip spaces
   while(*cp == ' ') cp++;

   if(*cp) {
      // process optional suffixes
      switch (string_t::tolower(*cp)) {
         case 'h':
            return time * 3600;
         case 'm':
            return time * 60;
         case 's':
            return time;
         default:
            errors.push_back(string_t::_format("Invalid time interval suffix (%s)", value));
            return 0;
      }
   }

   return time;
}

///
/// @brief  Returns `true` if running in a maintenance mode (e.g. compacting the 
///         state database or generating a report from a historical database) and
///         `false` otherwise.
///
bool config_t::is_maintenance(void) const
{
   return compact_db || end_month || prep_report || db_info;   // is it a maintenance run?
}

///
/// @brief  Returns `true` if DNS resolver can look up IP addresses in either the
///         GeoIP or DNS databases and `false` otherwise.
///
bool config_t::is_dns_enabled(void) const
{
   return dns_children != 0;
}

///
/// @brief  Saves textual representation of the start and end time stamps of 
///         a daylight saving time (DST) range into the DST range vector.
///
/// Start and end time stamps may be passed individually, leaving the other
/// parameter `NULL`. When both are set in the last DST range, a new range
/// is inserted into the DST range vector. 
///
/// Passing the same parameter multiple times just overwrites the one that was 
/// there before. This means that DST start and end time stamps from the same 
/// range must follow one another in the configuration file. However, DST ranges
/// don't need to be sorted between themselves (i.e. start/end time stamps for 
/// 2017 may be before start/end time stamps for 2016).
///
/// Time stamps are just saved in the DST range vector in this method and not 
/// validated in any way.
///
void config_t::set_dst_range(const char *start, const char *end)
{
   // get the last element (guaranteed to be there)
   dst_pair_t& dst_pair = dst_pairs[dst_pairs.size()-1];
   
   // set the start/end timestamps (overwrite, if more than one of same kind is found)
   if(start)
      dst_pair.dst_start = start;

   if(end)
      dst_pair.dst_end = end;

   // if we don't have a complete range, return
   if(dst_pair.dst_start.isempty() || dst_pair.dst_end.isempty())
      return;

   // otherwise create a new one for the next range
   dst_pairs.push_back(dst_pair_t());
}

///
/// @brief  Reads log file names from the standard input, one per line, and stores
///         them in the file name vector.
///
void config_t::proc_stdin_log_files(void)
{
   string_t::char_buffer_t buffer(BUFSIZE);
   char *cp1 = buffer, *cp2;

   // A single line must contain one file name
   while ( (fgets(buffer, BUFSIZE, stdin)) != NULL) {
      cp2 = cp1;
      
      // skip to the end of line 
      while(*cp2 != '\r' && *cp2 != '\n' && *cp2 != 0) cp2++;
      
      // and if it's not empty, put it into the list
      if(cp2-cp1)
         log_fnames.push_back(string_t(cp1, cp2-cp1));
   }
}

///
/// @brief  Returns a textual representation of the selected log file type.
///
const char *config_t::get_log_type_desc(void) const
{
   switch (log_type) {
      case LOG_CLF:
         return "CLF";
      case LOG_SQUID:
         return "Squid";
      case LOG_APACHE:
         return "Apache";
      case LOG_IIS:
         return "IIS";
      case LOG_W3C:
         return "W3C";
      default:
         return "Unknown";
   }
}

///
/// @brief  Returns a UTC offset for the specieied time stamp.
///
/// If the DST range vector is populated and the time stamp is within a DST range, 
/// DST offset will be added to the retured value. 
///
/// The DST iterator is used as an optimization to avoid repeated traversing of the
/// DST range vector looking for the matching DST range. This optimization assumes 
/// that time stamps passed into this method are in increasing time stamp order.
///
int config_t::get_utc_offset(const tstamp_t& tstamp, tm_ranges_t::iterator& dst_iter) const
{
   if(dst_offset && dst_ranges.is_in_range(tstamp, dst_iter))
      return utc_offset + dst_offset;

   return utc_offset;
}

///
/// @brief  Adds a warning message that the -p option is no longer required because
///         the incremental processing mode is enabled by default.
///
void config_t::deprecated_p_option(void)
{
   messages.emplace_back("Option -p is deprecated. Incremental processing is the default mode now.");
}
