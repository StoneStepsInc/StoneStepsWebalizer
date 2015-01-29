/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   config.cpp
*/
#include "pch.h"

#include <stdio.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#include "platform/sys/utsname.h"
#else
#include <sys/utsname.h>
#endif

#include <limits.h>

#include "config.h"
#include "linklist.h"
#include "lang.h"
#include "exception.h"

struct kwinfo {
   const char *keyword;
   u_int key;
};

config_t::config_t(void) : config_fnames(1)
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
   memory_mode = true;
   ignored_domain_names = false;
   user_config = false;

   pipe_log_names  = false;

   //
   // public values
   //
   conv_url_lower_case = false;		          // if true, URL stems will be converted lower case
   bundle_groups = true;				          // bundle groups together?
   no_def_index_alias = false;		          // ignore default index alias?
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
   incremental = false;                       /* incremental mode 1=yes   */
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

   swap_first_record = 100;
   swap_frequency = 100;

   http_port = DEF_HTTP_PORT;                 // HTTP port number
   https_port = DEF_HTTPS_PORT;	             // HTTPS port number

   mangle_agent = 0;                          /* mangle user agents       */
   group_domains= 0;                          /* Group domains 0=none     */
   group_url_domains = 0;                     /* Group URL domains 0=none */
   graph_lines  = 2;                          /* graph lines (0=none)     */
   log_type = LOG_IIS;                        // (0=clf, 1=ftp, 2=squid, 3=iis, 4=apache, 5=w3c)

   graph_border_width = 0;

   graph_background_alpha = 0;                // percent: opaque=0, transparent=100

   graph_background = DEFCOLOR;
   graph_gridline = DEFCOLOR;
   graph_shadow = DEFCOLOR;
   graph_title_color = DEFCOLOR;
   graph_hits_color = DEFCOLOR;
   graph_files_color = DEFCOLOR;
   graph_hosts_color = DEFCOLOR;
   graph_pages_color = DEFCOLOR;
   graph_visits_color = DEFCOLOR;
   graph_volume_color = DEFCOLOR;
   graph_outline_color = DEFCOLOR;
   graph_legend_color = DEFCOLOR;
   graph_weekend_color = DEFCOLOR;
   
   graph_type = "png";

   max_hist_length = 24;

   visit_timeout = 1800;                      /* visit timeout (seconds)  */
   max_visit_length = 0;
   download_timeout = 180;                    // download job timeout (seconds)

   ntop_sites = 30;                           /* top n sites to display   */
   ntop_sitesK = 10;                          /* top n sites (by kbytes)  */
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
   max_hosts_kb = ntop_sitesK; 
   max_urls_kb = ntop_urlsK;

   font_size_small = FONT_SIZE_SMALL;
   font_size_medium = FONT_SIZE_MEDIUM;

   rpt_title = lang.msg_title;
   html_charset = "utf-8";
   state_fname = "webalizer.current";         /* run state file name      */
   hist_fname = "webalizer.hist";             /* name of history file     */
   html_ext = "html";                         /* HTML file prefix         */
   dump_ext = "tab";                          /* Dump file prefix         */
   db_fname = "webalizer";                    // database file name
   db_fname_ext = "db";                       // database file extension
   report_db_name = "webalizer";              // report database file name

   dns_children = 0;                          /* DNS children (0=don't do)*/
   dns_cache_ttl= DNS_CACHE_TTL;              /* Default TTL of a DNS cache entry */

   dst_offset = utc_offset = 0;

   use_classic_mangler = false;
   target_downloads = true;
   page_entry = true;
   
   // most browsers ignore external XML entity declarations
   use_ext_ent = false;
   
   accept_host_names = false;

   // do not use the local machine's UTC offset by default
   local_utc_offset = false;

   geoip_city = true;

   // push the initial DST pair into the vector
   dst_pairs.push(dst_pair_t());
}

config_t::~config_t(void)
{
   lang.cleanup_lang_data();
}

/*********************************************/
/* GET_CONFIG - get configuration file info  */
/*********************************************/

void config_t::initialize(const string_t& basepath, int argc, const char * const argv[])
{
   const char *cp1;
   uint32_t max_ctry;                      /* max countries defined       */
   uint32_t i;
   string_t fpath;
   utsname system_info;

   //
   // Store the current directory to resolve relative paths
   //
   cur_dir = basepath;

   //
   // Process command line arguments
   //
   proc_cmd_line(argc, argv);

   // check stdin for log file names
   if(pipe_log_names )
      proc_stdin_log_files();

   // if log_fnames is empty, insert one empty entry to force stdin (i.e. at most one)
   if(!log_fnames.size())
      log_fnames.push(string_t());
      
   // treat a single dash as stdin
   vector_t<string_t>::iterator iter = log_fnames.begin();
   while(iter.more()) {
      string_t& fname = iter.next();
      if(fname.length() == 1 && fname[0] == '-')
         fname.clear();
   }

   //
   // If no user configuration file was provided, try to locate the
   // default one in the
   //
   //    - current directory
   //    - common system directory
   //    - directory where the executable is
   //
   if(!user_config) {
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
   }

   //
   // Process optional domain-specific include files
   //
   process_includes();

   //
   // Additional initializations and validations
   //
   
   //
   // Do not print any messages at this point, as they will appear before
   // version text. Instead, add them to the messages vector, which is
   // dumped to the standard output at a later time.
   //

   // turn off batch mode if incremental is not specified
   if(batch && (!incremental || prep_report)) {
      messages.push(string_t("WARNING: Batch processing only works in the incremental mode"));
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
   // Make sure group_sites does not contain bare IP addresses, as it
   // would cause collisions in hm_htab and the database. In order to
   // group by a single address, give this group a name that is not an
   // IP address:
   //
   // GroupHost   127.0.0.1   localhost
   //
   glist::iterator grph_iter = group_sites.begin();

   while(grph_iter.next()) {
      if(is_ip4_address(grph_iter.item()->name)) {
         grph_iter.item()->name = string_t::_format("group_%s", grph_iter.item()->name.c_str());
         messages.push(string_t::_format("WARNING: A bare IP address in GroupHost is changed to %s\n", grph_iter.item()->name.c_str()));
      }
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
      vector_t<string_t>::iterator iter = log_fnames.begin();
      while(iter.more()) {
         string_t& fname = iter.next();
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
   // If we are not allowed to use external XML entities, read the help
   // file, so we can embed it into XML reports.
   //
   if(!use_ext_ent && !help_file.isempty())
      read_help_xml(make_path(out_dir, help_file));
   
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

   if(dns_children > DNS_MAX_THREADS)
		dns_children = DNS_MAX_THREADS;

   enable_js = !html_js_path.isempty();

   // process all DST ranges we collected
   vector_t<dst_pair_t>::iterator dst_iter = dst_pairs.begin();
   
   while(dst_iter.more()) {
      const dst_pair_t& dst_pair = dst_iter.next();
      
      // ignore the range if both timestamps are empty (the top one)
      if(dst_pair.dst_start.isempty() && dst_pair.dst_end.isempty())
         continue;
      
      // report if either start or end timestamp is missing
      if(dst_pair.dst_start.isempty())
         throw exception_t(0, string_t::_format("Missing DST start time this end time: %s", dst_pair.dst_end.c_str()));

      if(dst_pair.dst_end.isempty())
         throw exception_t(0, string_t::_format("Missing DST end time for this start time: %s", dst_pair.dst_start.c_str()));
      
      // create actual timestamps (always in local time)
      tstamp_t dst_start, dst_end;

      if(!dst_start.parse(dst_pair.dst_start, utc_offset))
         throw exception_t(0, string_t::_format("Ivalid DST start timestamp: %s", dst_pair.dst_start.c_str()));

      if(!dst_end.parse(dst_pair.dst_end, utc_offset))
         throw exception_t(0, string_t::_format("Ivalid DST end timestamp: %s", dst_pair.dst_end.c_str()));

      // and make sure the end time stamp has the DST offset from UTC
      if(local_time && dst_offset)
         dst_end.tolocal(utc_offset+dst_offset);

      // add the range if there's nothing wrong with it
      if(!dst_ranges.add_range(dst_start, dst_end))
         throw exception_t(0, string_t::_format("Cannot add a DST range from %s to %s", dst_pair.dst_start.c_str(), dst_pair.dst_end.c_str()));
   }
   
   // clean up a bit and clear the text timestamps
   dst_pairs.clear();

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

   for (max_ctry=0; lang.ctry[max_ctry].desc; max_ctry++);
   if (ntop_ctrys > max_ctry) ntop_ctrys = max_ctry;   /* force upper limit */
   if (graph_lines> 20)       graph_lines= 20;         /* keep graphs sane! */

   /* ensure entry/exits don't exceed urls */
   i = (ntop_urls > ntop_urlsK) ? ntop_urls : ntop_urlsK;
   if(ntop_entry > i) ntop_entry=i;
   if(ntop_exit > i)  ntop_exit=i;
   
   // make sure maximums are not less than tops
   if(max_hosts < ntop_sites) max_hosts = ntop_sites;
   if(max_hosts_kb < ntop_sitesK) max_hosts_kb = ntop_sitesK;
   if(max_urls < ntop_urls) max_urls = ntop_urls;
   if(max_urls_kb < ntop_urlsK) max_urls_kb = ntop_urlsK;
   if(max_refs < ntop_refs) max_refs = ntop_refs;
   if(max_agents < ntop_agents) max_agents = ntop_agents;
   if(max_search < ntop_search) max_search = ntop_search;
   if(max_users < ntop_users) max_users = ntop_users;
   if(max_errors < ntop_errors) max_errors = ntop_errors;
   if(max_downloads < ntop_downloads) max_downloads = ntop_downloads;

   if(search_list.isempty()) {
      /* If no search engines defined, define some :) */
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

   if(!use_classic_mangler) {

      //
      // If the user agent argument exclusion and group lists are empty
      // and mangle_agent is a non-zero value, add some filters.
      //
      if(excl_agent_args.isempty() && group_agent_args.isempty()) {
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
      else {
         //
         // If mangle_agent is zero, set it to one to avoid having to
         // check all user agent list sizes in the performance-critical
         // main loop. This mangle level leaves argument version numbers
         // unmodified.
         //
         if(!mangle_agent)
            mangle_agent = 1;
      }
   }

   //
   // prepare and report the hostname
   //
   if (hname.isempty()) {
      if (uname(&system_info))
         hname = "localhost";
      else
         hname = system_info.nodename;
   }

}

static int cmp_conf_kw(const void *e1, const void *e2)
{
   return strcasecmp(((kwinfo*)e1)->keyword, ((kwinfo*)e2)->keyword);
}

void config_t::get_config(const char *fname)
{
   static kwinfo kwords[] = {
                     //
                     // This array *must* be sorted alphabetically
                     //
                     // max key: 188; empty slots: 
                     //
                     {"AcceptHostNames",     186},       // Accept host names instead of IP addresses?
                     {"AllAgents",		      67},			// List all User Agents?
                     {"AllDownloads",        123},       // List all downloads
                     {"AllErrors",		      114},			// List All HTTP Errors?
                     {"AllHosts",            64},
                     {"AllReferrers",	      66},			// List all Referrers?
                     {"AllSearchStr",	      68},			// List all Search Strings?
                     {"AllSites",		      64},			// List all sites?
                     {"AllURLs",		         65},			// List all URLs?
                     {"AllUsers",		      69},			// List all Users?
                     {"ApacheLogFormat",     95},			// Apache LogFormat Directive
                     {"Batch",               156},       // Batch processing?
                     {"BatchProcessing",     156},       // Batch processing?
                     {"BundleGroups",	      91},			// Bundle groups together?
                     {"ConvURLsLowerCase",   89},			// Convert URL's to lower case
                     {"CountryGraph",	      54},			// Display ctry graph (0=no)
                     {"DailyGraph",		      86},			// Daily Graph (0=no)
                     {"DailyStats",		      87},			// Daily Stats (0=no)
                     {"DbCacheSize",         146},       // State database cache size
                     {"DbDirect",            153},       // Use OS buffering?
                     {"DbDSync",             154},       // Use write-through?
                     {"DbExt",               148},       // State database file extension
                     {"DbName",              145},       // State database file name
                     {"DbPath",              144},       // State database path
                     {"DbSeqCacheSize",      149},       // Database sequence cache size
                     {"DbTrickleRate",       150},       // Database trickle rate
                     {"Debug",		         8},		   // Produce debug information
                     {"DNSCache",		      84},			// DNS Cache file name
                     {"DNSCacheTTL",	      93},			// TTL of a DNS cache entry (days)
                     {"DNSChildren",	      85},			// DNS Children (0=no DNS)
                     {"DownloadPath",        119},       // Download path
                     {"DownloadTimeout",     120},       // Download job timeout
                     {"DSTEnd",              162},       // Daylight saving end date/time
                     {"DSTOffset",           163},       // Daylight saving offset
                     {"DSTStart",            161},       // Daylight saving start date/time
                     {"DumpAgents",		      81},			// Dump user agents tab file
                     {"DumpDownloads",       121},       // Dump downloads?
                     {"DumpErrors",		      112},			// Dump HTTP errors?
                     {"DumpExtension",	      76},			// Dump filename extension
                     {"DumpHeader",		      77},			// Dump header as first rec?
                     {"DumpHosts",           78},
                     {"DumpPath",		      75},			// Path for dump files
                     {"DumpReferrers",	      80},			// Dump referrers tab file
                     {"DumpSearchStr",	      83},			// Dump search str tab file
                     {"DumpSites",		      78},			// Dump sites tab file
                     {"DumpURLs",		      79},			// Dump urls tab file
                     {"DumpUsers",		      82},			// Dump usernames tab file
                     {"EnablePhraseValues",  117},		   // Enable phrases in configuration values
                     {"ExcludeAgentArgs",    164},       // Exclude user agent arguments
                     {"ExcludeSearchArg",	   109},			// Exclude a search argument
                     {"GeoIPCity",           53},        // Output city name in reports?
                     {"GeoIPDBPath",         141},       // Path to the GeoIP database file
                     {"GMTTime",		         30},			// Local or UTC time?
                     {"GraphBackgroundAlpha",130},       // Graph background transparency
                     {"GraphBackgroundColor",105},		   // Graph background color
                     {"GraphBorderWidth",    129},		   // Graph border width
                     {"GraphFilesColor",     133},       // Graph files color
                     {"GraphFontBold",	      101},			// True Type font path
                     {"GraphFontMedium",     103},			// Medium font size in points
                     {"GraphFontNormal",     100},			// True Type font path
                     {"GraphFontSmall",      102},			// Small font size in points
                     {"GraphFontSmoothing",  104},		   // Use font anti-aliasing?
                     {"GraphGridlineColor",  124},       // Graph griline color
                     {"GraphHitsColor",      132},       // Graph hits color
                     {"GraphHostsColor",     134},       // Graph hosts color
                     {"GraphLegend",	      51},			// Graph Legends (yes/no)
                     {"GraphLegendColor",	   139},			// Graph legend color
                     {"GraphLines",		      52},			// Graph Lines (0=none)
                     {"GraphOutlineColor",   138},       // Graph bar outline color
                     {"GraphPagesColor",     135},       // Graph pages color
                     {"GraphShadowColor",	   106},			// Graph legend shadow color
                     {"GraphTitleColor",     131},       // Graph title color
                     {"GraphTrueColor",	   111},			// Create true-color images?
                     {"GraphType",           115},       // Graph type (PNG, Flash-OFC, etc)
                     {"GraphVisitsColor",    136},       // Graph visits color
                     {"GraphVolumeColor",    137},       // Graph volume (transfer) color
                     {"GraphWeekendColor",   140},       // Graph weekend color
                     {"GroupAgent",		      34},			// Group Agents
                     {"GroupAgentArgs",      167},       // Group agent arguments
                     {"GroupDomains",	      62},			// Group domains (n=level)
                     {"GroupHighlight",      36},			// BOLD Grouped entries
                     {"GroupHost",           32},
                     {"GroupReferrer",	      33},			// Group Referrers
                     {"GroupRobots",         159},       // Group Robots?
                     {"GroupShading",	      35},			// Shade Grouped entries
                     {"GroupSite",		      32},			// Group Sites
                     {"GroupURL",		      31},			// Group URL's
                     {"GroupURLDomains",     126},			// Group URL domains (proxy)
                     {"GroupUser",		      74},			// Usernames to group
                     {"HelpFile",            184},       // XML help file
                     {"HideAgent",		      19},			// User Agents to hide
                     {"HideAllHosts",        63},
                     {"HideAllSites",	      63},			// Hide ind. sites (0=no)
                     {"HideHost",            16},
                     {"HideReferrer",	      18},			// Referrers to hide
                     {"HideRobots",          157},       // Hide robots?
                     {"HideSite",		      16},			// Sites to hide
                     {"HideURL",		         17},			// URL's to hide
                     {"HideUser",		      71},			// Usernames to hide
                     {"HistoryLength",	      143},			// Number of months in history
                     {"HistoryName",	      39},			// Filename for history data
                     {"HostName",		      4},		   // Hostname to use
                     {"HourlyGraph",	      9},		   // Hourly stats graph
                     {"HourlyStats",	      10},		   // Hourly stats table
                     {"HTMLBody",		      42},			// HTML body code
                     {"HTMLCharset",	      99},			// HTML charset
                     {"HTMLCssPath",	      98},			// URL path to webalizer.css
                     {"HTMLEnd",		         43},			// HTML code at end
                     {"HTMLExtension",	      40},			// HTML filename extension
                     {"HTMLExtensionLang",   94},        // HTMLExtensionLang
                     {"HTMLHead",		      21},			// HTML Top1 code
                     {"HTMLJsPath",          128},       // HTML JavaScript Path
                     {"HTMLMetaNoIndex",	   110},			// Add noindex, nofollow?
                     {"HTMLPost",		      22},			// HTML Top2 code
                     {"HTMLPre",		         41},			// HTML code at beginning
                     {"HTMLTail",		      23},			// HTML Tail code
                     {"HttpPort",		      96},			// HTTP port number
                     {"HttpsPort",		      97},			// HTTPS port number
                     {"IgnoreAgent",	      28},			// User Agents to ignore
                     {"IgnoreHist",		      5},		   // Ignore history file
                     {"IgnoreHost",		      25},			// Synonym for IgnoreSite
                     {"IgnoreReferrer",      27},			// Referrers to ignore
                     {"IgnoreReferrerPartial",	125},	   // Partial request (206)
                     {"IgnoreRobots",        158},       // Ignore robots altogether?
                     {"IgnoreSite",		      25},			// Sites to ignore
                     {"IgnoreURL",		      26},			// Url's to ignore
                     {"IgnoreUser",		      72},			// Usernames to ignore
                     {"Include",		         118},			// Include a config file
                     {"IncludeAgent",	      48},			// User Agents to include
                     {"IncludeAgentArgs",    165},       // User agent argument to include
                     {"IncludeHost",         45},
                     {"IncludeReferrer",     47},			// Referrers to include
                     {"IncludeSearchArg",	   108},			// Include a search argument
                     {"IncludeSite",	      45},			// Sites to always include
                     {"IncludeURL",		      46},			// URL's to always include
                     {"IncludeUser",	      73},			// Usernames to include
                     {"Incremental",	      37},			// Incremental runs
                     {"IncrementalName",     38},			// Filename for state data
                     {"IndexAlias",		      20},			// Aliases for index.html
                     {"LanguageFile",	      90},			//	Language file
                     {"LocalUTCOffset",      188},       // Do not use local UTC offset?
                     {"LogDir",              183},       // Log directory
                     {"LogFile",		         2},		   // Log file to use for input
                     {"LogType",		         60},			// Log Type (clf/ftp/squid/iis)
                     {"MangleAgents",	      24},			// Mangle User Agents
                     {"MaxAgents",		      176},			// Maximum User Agents
                     {"MaxDownloads",        180},       // Maximum downloads
                     {"MaxErrors",		      179},			// Maximum HTTP Errors
                     {"MaxHosts",            173},       // Maximum hosts
                     {"MaxKHosts",           181},       // Maximum hosts (transfer)
                     {"MaxKURLs",		      182},			// Maximum URLs (transfer)
                     {"MaxReferrers",	      175},			// Maximum Referrers
                     {"MaxSearchStr",	      177},			// Maximum Search Strings
                     {"MaxURLs",		         174},			// Maximum URLs
                     {"MaxUsers",		      178},			// Maximum Users
                     {"MaxVisitLength",      187},       // Maximum visit length
                     {"MemoryMode",          147},       // Memory or database mode?
                     {"MonthlyTotals",       127},       // Output monthly totals report?
                     {"NoDefaultIndexAlias", 92},		   // Ignore default index alias?
                     {"OutputDir",		      1},		   // Output directory
                     {"OutputFormat",        171},       // Output format
                     {"PageEntryURL",        170},       // Show only pages in the entry report?
                     {"PageType",		      49},			// Page Type (pageview)
                     {"Quiet",		         6},		   // Run in quiet mode
                     {"ReallyQuiet",	      29},			// Dont display ANY messages
                     {"ReportTitle",	      3},		   // Title for reports
                     {"Robot",               155},       // Robot user agent filter
                     {"SearchEngine",	      61},			// SearchEngine strings
                     {"SortSearchArgs",	   107},			// Sort search arguments ?
                     {"SpamReferrer",        142},       // Spam referrer
                     {"SwapFirstRecord",     151},       // First record # to start DB swapping
                     {"SwapFrequency",       152},       // DB-swap each N record
                     {"TargetDownloads",     169},       // Treat download URLs as targets?
                     {"TargetURL",           168},       // Target URL pattern
                     {"TimeMe",		         7},		   // Produce timing results
                     {"TopAgents",		      14},			// Top User Agents
                     {"TopCountries",	      15},			// Top Countries
                     {"TopDownloads",        122},       // Top downloads
                     {"TopEntry",		      57},			// Top Entry Pages
                     {"TopErrors",		      113},			// Top HTTP errors
                     {"TopExit",		         58},			// Top Exit Pages
                     {"TopHosts",            11},
                     {"TopKHosts",           55},
                     {"TopKSites",		      55},			// Top sites (by KBytes)
                     {"TopKURLs",		      56},			// Top URL's (by KBytes)
                     {"TopReferrers",	      13},		   // Top Referrers
                     {"TopSearch",		      59},			// Top Search Strings
                     {"TopSites",		      11},		   // Top sites
                     {"TopURLs",		         12},		   // Top URL's
                     {"TopUsers",		      70},			// Top Usernames to show
                     {"UpstreamTraffic",     88},        // Track upstream traffic?
                     {"UseClassicMangleAgents",166},     // Use classic MangleAgents?
                     {"UseExternalEntities", 185},       // Use external XML entities?
                     {"UseHTTPS",		      44},			// Use https:// on URL's
                     {"UTCOffset",           160},       // UTC/local time difference
                     {"UTCTime",		         30},			// Local or UTC time?
                     {"VisitTimeout",	      50},			// Visit timeout (seconds)
                     {"XslPath",             172}        // XSL file path
                   };

   FILE *fp;

   string_t keyword, value;
   const char *cp1, *cp2;
   u_int num_kwords = sizeof(kwords)/sizeof(kwords[0]);
   kwinfo *kptr, key = {NULL, 0};
   char *buffer;

   config_fnames.push(string_t(fname));

   if ( (fp=fopen(fname,"r")) == NULL)
   {
      if (verbose)
      fprintf(stderr,"%s %s\n", lang.msg_bad_conf, fname);
      return;
   }

   buffer = new char[BUFSIZE];

   while ( (fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      /* skip comments and blank lines */
      if(*buffer =='#' || iswspaceex(*buffer) ) continue;

      // skip to the first non-alphanum character
      cp1 = cp2 = buffer;
      while(isalnum(*cp2)) *cp2++;
      
      // hold onto the keyword
      keyword.assign(cp1, cp2-cp1);

      // skip leading whitespace before the value
      cp1 = cp2;
      while(*cp1 != '\n' && *cp1 != '\0' && isspaceex(*cp1)) cp1++;
      
      // find the end of the line
      cp2 = cp1;
      while(*cp2 != '\n' && *cp2 != '\0') *cp2++;
      
      // move backwards until we find a non-whitespace character
      while(cp2 != cp1 && iswspaceex(*cp2)) *cp2--;
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
         messages.push(string_t::_format("%s \"%s\" (%s)\n", lang.msg_bad_key, keyword.c_str(), fname));
         continue;
      }

      switch (kptr->key) {
         case 1:  out_dir=value; break;                           // OutputDir
         case 2:  log_fnames.push(value);break;                   // LogFile
         case 3:  rpt_title = value; break;                       // ReportTitle
         case 4:  hname=value; break;                             // HostName
         case 5:  ignore_hist=(tolower(value[0])=='y'); break;    // IgnoreHist
         case 6:  verbose=(value[0]=='n')?2:1; break;             // Quiet
         case 7:  time_me=(value[0]=='n')?0:1; break;             // TimeMe
         case 8:  debug_mode=(value[0]=='n')?0:1; break;          // Debug
         case 9:  hourly_graph=(value[0]=='n')?false:true; break; // HourlyGraph
         case 10: hourly_stats=(value[0]=='n')?false:true; break; // HourlyStats
         case 11: ntop_sites = atoi(value); break;                // TopSites
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
         case 25: add_ignored_host(value); break;                 // IgnoreSite, IgnoreHost
         case 26: ignored_urls.add_nlist(value); break;           // IgnoreURL
         case 27: ignored_refs.add_nlist(value); break;           // IgnoreReferrer
         case 28: ignored_agents.add_nlist(value); break;         // IgnoreAgent
         case 29: if (value[0]=='y') verbose=0; break;            // ReallyQuiet
         case 30: local_time=(tolower(*value)=='y') ? false : true; break; // GMTTime
         case 31: group_urls.add_glist(value); break;             // GroupURL
         case 32: group_sites.add_glist(value); break;            // GroupSite
         case 33: group_refs.add_glist(value); break;             // GroupReferrer
         case 34: group_agents.add_glist(value); break;           // GroupAgent
         case 35: shade_groups=(tolower(value[0])=='y'); break;   // GroupShading
         case 36: hlite_groups=(tolower(value[0])=='y'); break;   // GroupHighlight
         case 37: incremental=(value[0]=='y')?true:false; break;  // Incremental
         case 38: state_fname=value; break;                       // State FName
         case 39: hist_fname=value; break;                        // History FName
         case 40: html_ext=value; break;                          // HTML extension
         case 41: html_pre.add_nlist(value); break;               // HTML Pre code
         case 42: html_body.add_nlist(value); break;              // HTML Body code
         case 43: html_end.add_nlist(value); break;               // HTML End code
         case 44: use_https=(tolower(value[0])=='y'); break;      // Use https://
         case 45: include_sites.add_nlist(value); break;          // IncludeSite
         case 46: include_urls.add_nlist(value); break;           // IncludeURL
         case 47: include_refs.add_nlist(value); break;           // IncludeReferrer
         case 48: include_agents.add_nlist(value); break;         // IncludeAgent
         case 49: page_type.add_nlist(value); break;              // PageType
         case 50: visit_timeout=get_interval(value); break;       // VisitTimeout
         case 51: graph_legend=(tolower(value[0])=='y'); break;   // GraphLegend
         case 52: graph_lines = atoi(value); break;               // GraphLines
         case 53: geoip_city = (tolower(value[0])=='y'); break;   // GeoIPCity
         case 54: ctry_graph=(tolower(value[0])=='y'); break;     // CountryGraph
         case 55: ntop_sitesK = atoi(value); break;               // TopKSites (KB)
         case 56: ntop_urlsK  = atoi(value); break;               // TopKUrls (KB)
         case 57: ntop_entry  = atoi(value); break;               // Top Entry pgs
         case 58: ntop_exit   = atoi(value); break;               // Top Exit pages
         case 59: ntop_search = atoi(value); break;               // Top Search pgs
         case 60: log_type=(tolower(value[0])=='f')?
              LOG_SQUID:(tolower(value[0])=='c')?
              LOG_CLF: (tolower(value[0])=='a')?
              LOG_APACHE : (tolower(value[0])=='w')?
              LOG_W3C : LOG_IIS;
              break;                                              // LogType
         case 61: search_list.add_glist(value, true); break;      // SearchEngine
         case 62: group_domains=atoi(value); break;               // GroupDomains
         case 63: hide_hosts=(tolower(value[0])=='y'); break;     // HideAllSites
         case 64: all_hosts=(tolower(value[0])=='y'); break;      // All Sites?
         case 65: all_urls=(tolower(value[0])=='y'); break;       // All URL's?
         case 66: all_refs=(tolower(value[0])=='y'); break;       // All Refs
         case 67: all_agents=(tolower(value[0])=='y'); break;     // All Agents?
         case 68: all_search=(tolower(value[0])=='y'); break;     // All Srch str
         case 69: all_users=(tolower(value[0])=='y'); break;      // All Users?
         case 70: ntop_users=atoi(value); break;                  // TopUsers
         case 71: hidden_users.add_nlist(value); break;           // HideUser
         case 72: ignored_users.add_nlist(value); break;          // IgnoreUser
         case 73: include_users.add_nlist(value); break;          // IncludeUser
         case 74: group_users.add_glist(value); break;            // GroupUser
         case 75: dump_path=value; break;                         // DumpPath
         case 76: dump_ext=value; break;                          // Dumpfile ext
         case 77: dump_header=(tolower(value[0])=='y'); break;    // DumpHeader?
         case 78: dump_hosts=(tolower(value[0])=='y'); break;     // DumpSites?
         case 79: dump_urls=(tolower(value[0])=='y'); break;      // DumpURLs?
         case 80: dump_refs=(tolower(value[0])=='y'); break;      // DumpReferrers?
         case 81: dump_agents=(tolower(value[0])=='y'); break;    // DumpAgents?
         case 82: dump_users=(tolower(value[0])=='y'); break;     // DumpUsers?
         case 83: dump_search=(tolower(value[0])=='y'); break;    // DumpSrchStrs?
         case 84: dns_cache=value; break;                         // DNSCache fname
         case 85: dns_children=atoi(value); break;                // DNSChildren
         case 93: dns_cache_ttl = atoi(value) * 86400; if(dns_cache_ttl == 0) dns_cache_ttl = DNS_CACHE_TTL; break;
         case 86: daily_graph=(tolower(value[0])=='y'); break;    // HourlyGraph
         case 87: daily_stats=(tolower(value[0])=='y'); break;    // HourlyStats
         case 88: upstream_traffic = (tolower(value[0]) == 'y') ? true : false; break;
         case 89: conv_url_lower_case = (tolower(value[0]) == 'y') ? true : false; break;
         case 90: lang.proc_lang_file(value); break;
         case 91: bundle_groups = (tolower(value[0]) == 'y') ? true : false; break;
         case 92: no_def_index_alias = (tolower(value[0]) == 'y') ? true : false; break;
         case 94: html_ext_lang = (tolower(value[0]) == 'y') ? true : false; break;
         case 95: apache_log_format = value; break;
         case 96: http_port = atoi(value); break;
         case 97: https_port = atoi(value); break;
         case 98: save_path_opt(value, html_css_path); break;
         case 99: html_charset = value; break;
         case 100: font_file_normal = value; break;
         case 101: font_file_bold = value; break;
         case 102: font_size_small = atof(value); break;
         case 103: font_size_medium = atof(value); break;
         case 104: font_anti_aliasing = (tolower(value[0]) == 'y') ? true : false; break;
         case 105: graph_background = graph_t::make_color(value); break;
         case 106: graph_shadow = graph_t::make_color(value); break;
         case 107: sort_srch_args = (tolower(value[0]) == 'y') ? true : false; break;
         case 108: incl_srch_args.add_nlist(value);   break;
         case 109: excl_srch_args.add_nlist(value);   break;
         case 110: html_meta_noindex = (tolower(value[0]) == 'y') ? true : false; break;
         case 111: graph_true_color = (tolower(value[0]) == 'y') ? true : false; break;
         case 112: dump_errors = (tolower(value[0]) == 'y') ? true : false; break;
         case 113: ntop_errors = atoi(value); break;
         case 114: all_errors=(tolower(value[0])=='y'); break;    // List all errors?
         case 115: graph_type = value.tolower(); break;
         case 117: set_enable_phrase_values(tolower(value[0]) == 'y'); break;
         case 118: includes.add_glist(value); break;
         case 119: downloads.add_glist(value); break;
         case 120: download_timeout = get_interval(value); break;
         case 121: dump_downloads = (tolower(value[0]) == 'y') ? true : false; break;
         case 122: ntop_downloads = atoi(value); break;
         case 123: all_downloads=(tolower(value[0]) == 'y'); break;
         case 124: graph_gridline = graph_t::make_color(value); break;
         case 125: ignore_referrer_partial = (tolower(value[0]) == 'y') ? true : false; break;
         case 126: group_url_domains=atoi(value); break;
         case 127: monthly_totals_stats=(tolower(value[0]) == 'y') ? true : false; break;
         case 128: save_path_opt(value, html_js_path); break;
         case 129: graph_border_width = atoi(value); if(graph_border_width > 7) graph_border_width = 7; break;
         case 130: graph_background_alpha = atoi(value); if(graph_background_alpha > 100) graph_background_alpha = 100; break;
         case 131: graph_title_color = graph_t::make_color(value); break;
         case 132: graph_hits_color = graph_t::make_color(value); break;
         case 133: graph_files_color = graph_t::make_color(value); break;
         case 134: graph_hosts_color = graph_t::make_color(value); break;
         case 135: graph_pages_color = graph_t::make_color(value); break;
         case 136: graph_visits_color = graph_t::make_color(value); break;
         case 137: graph_volume_color = graph_t::make_color(value); break;
         case 138: graph_outline_color = graph_t::make_color(value); break;
         case 139: graph_legend_color = graph_t::make_color(value); break;
         case 140: graph_weekend_color = graph_t::make_color(value); break;
         case 141: geoip_db_path = value; break;
         case 142: spam_refs.add_nlist(value); break;
         case 143: max_hist_length = atoi(value); break;
         case 144: db_path = value; break;
         case 145: db_fname = value; break;
         case 146: db_cache_size = get_db_cache_size(value); break;
         case 147: memory_mode = (tolower(value[0]) == 'y') ? true : false; break;
         case 148: db_fname_ext = value; break;
         case 149: db_seq_cache_size = atoi(value); break;
         case 150: db_trickle_rate = atoi(value); break;
         case 151: swap_first_record = atoi(value) * 1000; break;
         case 152: swap_frequency = atoi(value) * 1000; break;
         case 153: db_direct = (tolower(value[0]) == 'y') ? true : false; break;
         case 154: db_dsync = (tolower(value[0]) == 'y') ? true : false; break;
         case 155: robots.add_glist(value); break;
         case 156: batch = (tolower(value[0]) == 'y') ? true : false; break;
         case 157: hide_robots = (tolower(value[0]) == 'y') ? true : false; break;
         case 158: ignore_robots = (tolower(value[0]) == 'y') ? true : false; break;
         case 159: group_robots = (tolower(value[0]) == 'y') ? true : false; break;
         case 160: utc_offset = get_interval(value, 60); break;
         case 161: set_dst_range(&value, NULL); break;
         case 162: set_dst_range(NULL, &value); break;
         case 163: dst_offset = get_interval(value, 60); break;
         case 164: excl_agent_args.add_nlist(value); break;
         case 165: incl_agent_args.add_nlist(value); break;
         case 166: use_classic_mangler = (tolower(value[0]) == 'y'); break;
         case 167: group_agent_args.add_glist(value); break;
         case 168: target_urls.add_nlist(value); break;
         case 169: target_downloads = (tolower(value[0]) == 'y'); break;
         case 170: page_entry = (tolower(value[0]) == 'y'); break;
         case 171: add_output_format(value.tolower()); break;
         case 172: save_path_opt(value, xsl_path); break;
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
         case 184: help_file = value; break;
         case 185: use_ext_ent = (tolower(value[0]) == 'y'); break;
         case 186: accept_host_names = (tolower(value[0]) == 'y'); break;
         case 187: max_visit_length = get_interval(value); break;
         case 188: local_utc_offset =  (tolower(value[0]) == 'y'); break;
      }
   }
   fclose(fp);
   delete [] buffer;
}

void config_t::add_output_format(const string_t& format)
{
   if(!output_formats.isinlist(format))
      output_formats.add_nlist(format);
}

void config_t::get_config_cb(const char *fname, void *_this)
{
   if(_this != NULL)
      ((config_t*)_this)->get_config(fname);
}

void config_t::process_includes(void)
{
   includes.for_each(hname, get_config_cb, this);
}

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

void config_t::add_ignored_host(const char *value)
{
   const char *cptr;

   if(value == NULL)
      return;

   //
   // If the pattern contains a domain name, set ignored_domain_names
   // to true (this will slow down log processing)
   //
   for(cptr = value; !ignored_domain_names && *cptr != 0; cptr++) {
      if(strchr("0123456789.*", *cptr) == NULL) {
         ignored_domain_names = true;
         break;
      }
   }

   ignored_hosts.add_nlist(value);
}

string_t& config_t::save_path_opt(const char *str, string_t& path) const
{
   path.reset();

	if(str == NULL || *str == 0)
		return path;

   path = str;

	if(path[path.length()-1] != '/')
		path += '/';

   return path;
}

/*********************************************/
/* ISPAGE - determine if an HTML page or not */
/*********************************************/

bool config_t::ispage(const string_t& url) const
{
   const char *cp1, *cp2, *cp3;

   if(url.isempty())
      return true;

   cp3 = &url[url.length()];
   cp2 = cp3-1;

   if(*cp2 == '/')
      return true;

   cp1 = url;

   while(cp2 != cp1) {
      if(*cp2 == '.' || *cp2 == '/')
         break;
      cp2--;
   }

   if(*cp2 == '/' || cp2 == cp1)
      return true;

   cp2++;

   return page_type.isinlist(string_t().hold(const_cast<char*>(cp2), cp3-cp2, true, cp3-cp2+1)) != NULL;
}

u_int config_t::get_url_type(u_short port, u_int urltype) const
{
	if(port == PORT_UNKNOWN)
		return URL_TYPE_UNKNOWN;

	if(port == http_port)
		return urltype | URL_TYPE_HTTP;

	if(port = https_port)
		return urltype | URL_TYPE_HTTPS;

	return URL_TYPE_UNKNOWN;
}

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
               log_fnames.push(string_t(argv[optind]));
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
         if((vptr = (strchr("aAcCDeEFgIlmMnNoPrRsStuUx", *nptr)) ? argv[++optind] : NULL)) {
            if(*vptr == '-') {
               if(verbose)
                  fprintf(stderr, "Missing option value for -%s\n", nptr);
               exit(1);
            }
         }
      }

      // process long options
      if(longopt) {
         if(!strncasecmp(nptr, "help", nlen))
            print_options = true;
         else if(!strncasecmp(nptr, "prepare-report", nlen))
            prep_report = true;
         else if(!strncasecmp(nptr, "last-log", nlen))
            last_log = true;
         else if(!strncasecmp(nptr, "version", nlen))
            print_version = true;
         else if(!strncasecmp(nptr, "warranty", nlen))
            print_warranty = true;
         else if(!strncasecmp(nptr, "batch", nlen))
            batch = true;
         else if(!strncasecmp(nptr, "compact-db", nlen))
            compact_db = true;
         else if(!strncasecmp(nptr, "end-month", nlen))
            end_month = true;
         else if(!strncasecmp(nptr, "db-info", nlen))
            db_info = true;
         else if(!strncasecmp(nptr, "pipe-log-names", nlen))
            pipe_log_names  = true;
         else
            messages.push(string_t::_format("WARNING: Unknown option --%s\n", string_t(nptr, nlen).c_str()));

         continue;
      }

      // process classic one-character options
      switch (*nptr) {
          case 'a': hidden_agents.add_nlist(vptr); break;            // Hide agents
          case 'A': ntop_agents=atoi(vptr); break;                   // Top agents
          case 'c': user_config = true; get_config(vptr); break;     // User Config file
          case 'C': ntop_ctrys=atoi(vptr); break;                    // Top countries
          case 'd': debug_mode=1; break;                             // Debug
          case 'D': dns_cache=vptr; break;                           // DNS Cache filename
          case 'e': ntop_entry=atoi(vptr); break;                    // Top entry pages
          case 'E': ntop_exit=atoi(vptr); break;                     // Top exit pages
          case 'F': log_type=(tolower(vptr[0])=='f')?
               LOG_SQUID: (tolower(vptr[0])=='c')?
               LOG_CLF: (tolower(vptr[0])=='a')?
               LOG_APACHE: (tolower(vptr[0])=='w')?
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
          case 'm': visit_timeout=get_interval(vptr); break;         // Visit Timeout
          case 'M': mangle_agent=atoi(vptr); break;                  // mangle user agents
          case 'n': hname=vptr; break;                               // Hostname
          case 'N': dns_children=atoi(vptr); break;                  // # of DNS children
          case 'o': out_dir=vptr; break;                             // Output directory
          case 'p': incremental=true; break;                         // Incremental run
          case 'P': page_type.add_nlist(vptr); break;                // page view types
          case 'q': verbose=1; break;                                // Quiet (verbose=1)
          case 'Q': verbose=0; break;                                // Really Quiet
          case 'r': hidden_refs.add_nlist(vptr); break;              // Hide referrer
          case 'R': ntop_refs=atoi(vptr); break;                     // Top referrers
          case 's': hidden_hosts.add_nlist(vptr); break;             // Hide site
          case 'S': ntop_sites=atoi(vptr); break;                    // Top sites
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

void config_t::set_enable_phrase_values(bool enable)
{
   enable_phrase_values = enable;

   search_list.set_enable_phrase_values(enable);
   group_urls.set_enable_phrase_values(enable);
   group_sites.set_enable_phrase_values(enable);
   group_refs.set_enable_phrase_values(enable);
   group_agents.set_enable_phrase_values(enable);
   group_users.set_enable_phrase_values(enable);
   downloads.set_enable_phrase_values(enable);
   includes.set_enable_phrase_values(enable);
   robots.set_enable_phrase_values(enable);
   group_agent_args.set_enable_phrase_values(enable);
}

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

string_t config_t::get_db_path(void) const
{
   return make_path(db_path, (is_default_db()) ? db_fname : report_db_name) + '.' + db_fname_ext;
}

bool config_t::is_default_db(void) const
{
   return !prep_report && !compact_db && !db_info;
}

int config_t::get_interval(const char *value, int div) const
{
   int time;
   const char *cp;

   if(!value)
      return 0;

   time = (int) str2l(value, &cp);
   
   if(!cp)
      throw exception_t(0, string_t::_format("Invalid time interval value (%s)", value));

   // skip spaces
   while(*cp == ' ') cp++;

   // process optional suffixes
   switch (tolower(*cp)) {
      case 'h':
         if(div == 3600)
            return time;

         time *= 3600;
         break;

      case 'm':
         if(div == 60)
            return time;

         time *= 60;
         break;
   }

   return div > 1 ? time / div : time;
}

void config_t::read_help_xml(const char *fname)
{
	long fsize;
   FILE *file = NULL;
   char *buffer = NULL;
   size_t cnt1 = 0, cnt2;

   // check if it's a bad name
	if(fname == NULL || *fname == 0)
		return;

   // open read-only
	if((file = fopen(fname, "r")) == NULL)
      goto errexit;

   // seek to the end to determine file size
	if(fseek(file, 0, SEEK_END) == -1 || (fsize = ftell(file)) == -1L)
      goto errexit;

   // reset file position to the beginning
	if(fseek(file, 0, SEEK_SET) == -1)
      goto errexit;
      
   // allocate a block to hold the entire file and a zero terminator
	if((buffer = (char*) malloc(fsize+1)) == NULL)
      goto errexit;

   // read first three characters
   if((cnt2 = fread(buffer, sizeof(char), 3, file)) != 3)
      goto errexit;

   // check if it's a UTF-8 byte order mark (BOM)
   if(buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
      cnt1 = 3;

   // and read the rest
   if((cnt2 = fread(buffer+cnt1, sizeof(char), fsize-cnt1, file)) == 0)
      goto errexit;

	fclose(file);

   // truncate the buffer if it's too large (e.g. \r\n translation on Windows)
   if((long) (cnt1+cnt2) < fsize)
      buffer = (char*) realloc(buffer, cnt1+cnt2+1);

   // terminate the string and attach to help_xml
	buffer[cnt1+cnt2] = 0;
	help_xml.attach(buffer, cnt1+cnt2);

   // report the file path
   messages.push(string_t::_format("%s %s", lang.msg_use_help, fname));
	
	return;

errexit:
   // report that we couldn't read the file
   messages.push(string_t::_format("%s (%s)", lang.msg_file_err, fname));

	if(buffer)
	   free(buffer);

   if(file)
	   fclose(file);
}

bool config_t::is_maintenance(void) const
{
   return compact_db || end_month || prep_report || db_info;   // is it a maintenance run?
}

bool config_t::is_dns_enabled(void) const
{
   return dns_children != 0;
}

void config_t::set_dst_range(const string_t *start, const string_t *end)
{
   // get the last element (guaranteed to be there)
   dst_pair_t& dst_pair = dst_pairs[dst_pairs.size()-1];
   
   // set the start/end timestamps (overwrite, if more than one of same kind is found)
   if(start)
      dst_pair.dst_start = *start;

   if(end)
      dst_pair.dst_end = *end;

   // if we don't have a complete range, return
   if(dst_pair.dst_start.isempty() || dst_pair.dst_end.isempty())
      return;

   // otherwise create a new one for the next range
   dst_pairs.push(dst_pair_t());
}

void config_t::proc_stdin_log_files(void)
{
   char *buffer = new char[BUFSIZE];
   char *cp1 = buffer, *cp2;

   // A single line must contain one file name
   while ( (fgets(buffer, BUFSIZE, stdin)) != NULL) {
      cp2 = cp1;
      
      // skip to the end of line 
      while(*cp2 != '\r' && *cp2 != '\n' && *cp2 != 0) cp2++;
      
      // and if it's not empty, put it into the list
      if(cp2-cp1)
         log_fnames.push(string_t(cp1, cp2-cp1));
   }
   
   delete [] buffer;
}

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

int config_t::get_utc_offset(const tstamp_t& tstamp, tm_ranges_t::iterator& dst_iter) const
{
   if(dst_offset && dst_ranges.is_in_range(tstamp, dst_iter))
      return utc_offset + dst_offset;

   return utc_offset;
}
