/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   webalizer.cpp
*/
#include "pch.h"

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include "platform/sys/utsname.h"
#else
#include <unistd.h>                           /* normal stuff             */
#include <sys/utsname.h>
#endif

#include "webalizer.h"                         /* main header              */
#include "output.h"
#include "parser.h"
#include "preserve.h"
#include "hashtab.h"
#include "linklist.h"
#include "lang.h"
#include "dns_resolv.h"
#include "util_path.h"
#include "util_http.h"
#include "util_url.h"
#include "util_time.h"
#include "util_math.h"
#include "tstring.h"
#include "exception.h"
#include "dump_output.h"
#include "html_output.h"
#include "console.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <list>
#include <memory>
#include <exception>
#include <algorithm>
#include <stdexcept>

/*********************************************/
/* GLOBAL VARIABLES                          */
/*********************************************/

static const char *copyright   = "Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)";

bool webalizer_t::abort_signal = false;   // true if Ctrl-C was pressed

///
/// @brief  Constructs an instance of a log processor.
///
webalizer_t::webalizer_t(const config_t& config) : config(config), parser(config), state(config, &end_visit_cb, &end_download_cb, this), dns_resolver(config)
{
   // preallocate all character buffers we need for log processing
   buffer_allocator.release_buffer(string_t::char_buffer_t(BUFSIZE));
   buffer_allocator.release_buffer(string_t::char_buffer_t(BUFSIZE));
}

///
/// @brief  Destroys an instance of a log processor.
///
webalizer_t::~webalizer_t(void)
{
}

///
/// @brief  Initializes a log processor instance using a fully initialized 
///         configuration object.
///
/// All initialization methods called from this method must report unrecoverable errors to 
/// the standard error stream and return `false`. If any initialization throws an exception, 
/// this exception must be self-explanatory because it will be reported on its own. This 
/// alows us to keep this method simpler because otherwise each initialization method call 
/// would have to be wrapped in its own exception handler to keep the same level of detail 
/// (e.g. that a DNS resolver failed with a particular error).
///
void webalizer_t::initialize(void)
{
   u_int i;

   // check if the output directory has write access
   if(access(config.out_dir, R_OK | W_OK)) {
      /* Error: Can't change directory to ... */
      throw exception_t(0, string_t::_format("%s %s", config.lang.msg_dir_err,config.out_dir.c_str()));
   }

   // check if the database directory has write access
   if(access(config.db_path, R_OK | W_OK)) {
      /* Error: Can't change directory to ... */
      throw exception_t(0, string_t::_format("%s %s", config.lang.msg_dir_err, config.db_path.c_str()));
   }

   // initialize components we need for log file processing
   if(!config.is_maintenance()) {
      // initialize DNS resolver if the number of workers is set to a non-zero value
      if(config.is_dns_enabled()) {
         if(!dns_resolver.dns_init())
            throw exception_t(0, config.lang.msg_dns_init);
      }

      // initialize the log file parser
      if(!parser.init_parser(config.log_type)) {
         throw exception_t(0, string_t::_format("%s", config.lang.msg_pars_err));
      }
   }

   //
   // Initialize report generator
   //
   if(!config.compact_db && !config.end_month && !config.db_info) {
      if(!init_output_engines()) {
         throw exception_t(0, "Cannot initialize output engine");
      }
   }

   //
   // Initialize the state engine
   //
   if(!state.initialize()) {
      throw exception_t(0, "Cannot initialize the state engine");
   }

   //
   // restore state, if required
   //
   if(config.prep_report || config.end_month || config.incremental || config.db_info) {
      if((i=state.restore_state()) != 0) {
         /* Error: Unable to restore run data (error num) */
         throw exception_t(0, string_t::_format("%s (%d)\n", config.lang.msg_bad_data, i));
      }
   }

   if(!config.db_info) {
      /* Creating output in ... */
      if(config.verbose > 1)
         printf("%s %s\n", config.lang.msg_dir_use, !config.out_dir.isempty() ? config.out_dir.c_str() : config.lang.msg_cur_dir);

      /* Hostname for reports is ... */
      if(config.verbose > 1 && !config.db_info) 
         printf("%s '%s'\n",config.lang.msg_hostname,config.hname.c_str());
   }
}

///
/// @brief  Cleans up an instance of a log processor.
///
void webalizer_t::cleanup(void)
{
   if(!config.is_maintenance()) {
      if(config.is_dns_enabled())
         dns_resolver.dns_clean_up();

      parser.cleanup_parser();
   }
   
   if(!config.compact_db && !config.end_month && !config.db_info)
      cleanup_output_engines();

   state.cleanup();
}

///
/// @brief  Initializes all output engines for each selected report type.
///
/// This method reports all errors to the stderr stream and returns `false` if 
/// any output engine could not be initialized.
///
bool webalizer_t::init_output_engines(void)
{
   output_t::graphinfo_t *graphinfo = NULL;
   std::unique_ptr<output_t> optr;
   nlist::const_iterator iter = config.output_formats.begin();
   
   for(nlist::const_iterator iter = config.output_formats.begin(); iter != config.output_formats.end(); iter++) {
      // allocate an output engine of the requested type
      if(iter->string == "html") 
         optr.reset(new html_output_t(config, state));
      else if(iter->string == "tsv") 
         optr.reset(new dump_output_t(config, state));
      else {
         fprintf(stderr, "Unrecognized output format (%s)\n", iter->string.c_str());
         continue;
      }
      
      // make sure only one set of images is generated
      if(optr->graph_support()) {
         if(!graphinfo)
            graphinfo = optr->alloc_graphinfo();
         else
            optr->set_graphinfo(graphinfo);
      }
      
      // and initialize this output engine
      if(!optr->init_output_engine()) {
         fprintf(stderr, "Cannot initialize output engine (%s)\n", iter->string.c_str());
         return false;
      }
      
      output.push_back(optr.release());
   }
   
   if(!output.size())
      fprintf(stderr, "At least one output format must be specified\n");
   
   return output.size() != 0;
}

///
/// @brief  Cleans up all selected output engines.
///
void webalizer_t::cleanup_output_engines(void)
{
   output_t *optr;
   std::vector<output_t*>::iterator iter = output.begin();
   
   while(iter != output.end()) {
      optr = *iter++;
      optr->cleanup_output_engine();
      delete optr;
   }
   output.clear();
}

///
/// @brief  Updates an index document for each report type with the usage data for 
///         the current month in the state database.
///
void webalizer_t::write_main_index(void)
{
   output_t *optr;
   std::vector<output_t*>::iterator iter = output.begin();
   
   while(iter != output.end()) {
      optr = *iter++;
      
      if(optr->is_main_index()) {
         if (config.verbose>1) 
            printf("%s (%s)\n", config.lang.msg_gen_sum, optr->get_output_type());
         
         optr->write_main_index();
      }
   }
}

///
/// @brief  Creates a monthly usage report document for each report type for the 
///         current month in the state database.
///
void webalizer_t::write_monthly_report(void)
{
   output_t *optr;
   std::vector<output_t*>::iterator iter = output.begin();
   
   while(iter != output.end()) {
      optr = *iter++;
      
      if(config.verbose > 1)
         printf("%s %s %d (%s)\n", config.lang.msg_gen_rpt, config.lang.l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year, optr->get_output_type());
      
      optr->write_monthly_report();
   }
}

///
/// @brief  Prints application command line options in the selected language.
///
void webalizer_t::print_options(const char *pname)
{
   int i;

   printf("%s: %s %s\n", config.lang.h_usage1, pname, config.lang.h_usage2);
   for (i=0; config.lang.h_msg[i]; i++) printf("%s\n", config.lang.h_msg[i]);
}

///
/// @brief  Prints warranty information.
///
void webalizer_t::print_warranty(void)
{
   printf("THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL,\n"
          "BUT WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED WARRANTY OF\n"
          "MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. SEE THE\n"
          "GNU GENERAL PUBLIC LICENSE V2 FOR MORE DETAILS.\n");
}

///
/// @brief  Prints application name, version and some basic system information
///         if the verbosity level is greater than Quiet.
///
void webalizer_t::print_intro(void)
{
   utsname system_info;

   if(config.verbose > 1) {
      uname(&system_info);

      printf("\nStone Steps Webalizer v%s (%s %s)\n\n", 
               state_t::get_app_version().c_str(), 
               system_info.sysname, 
               system_info.release);
   }
}

///
/// @brief  Prints application name, version and some basic system information.
///
/// The amount of printed information depends on the verbosity level. In Really Quiet 
/// mode, only the version is printed.
///
void webalizer_t::print_version()
{
   utsname system_info;

   // for a quiet run just print the version
   if(config.verbose == 0) {
      printf("%s\n", state_t::get_app_version().c_str());
      return;
   }

   uname(&system_info);

   printf("\nStone Steps Webalizer v%s (%s %s) %s\n%s\n",   
               state_t::get_app_version().c_str(),
               system_info.sysname,system_info.release,
               config.lang.language,copyright);
   printf("\nThis program is based on The Webalizer v2.01-10\nCopyright 1997-2001 by Bradford L. Barrett (www.webalizer.com)\n\n");

   if (config.debug_mode) {
      printf("Mod date: %s\n", __DATE__);
      printf("\nDefault config dir: %s\n\n", ETCDIR);
   }
}

///
/// @brief  Prints the selected language file and all configuration file names.
///
void webalizer_t::print_config(void)
{
   if(config.verbose > 1) {
      // report procesed language file
      config.lang.report_lang_fname();

      // report processed configuration files
      config.report_config();
   }
}

///
/// @brief  Adds provided visit data to the relevant host groups and countries.
///
/// This method is called when the specified host node has gone through the DNS 
/// resolver, so host properties provided by the DNS resolver, such as country
/// information or whether it's a spamer or not, are available in the host node. 
///
void webalizer_t::group_host_by_name(const hnode_t& hnode, const vnode_t& vnode)
{
   ccnode_t *ccptr;
   const string_t *group;
   const string_t *hostname;
   bool newhgrp = false;
   uint64_t vlen = (uint64_t) vnode.end.elapsed(vnode.start);

   if(hnode.flag == OBJ_GRP)
      return;

   hostname = &hnode.hostname();

   //
   // Try grouping by IP address or host name first
   //
   if(config.group_hosts.size() && ((hostname && (group = config.group_hosts.isinglist(*hostname)) != NULL) ||
                                                ((group = config.group_hosts.isinglist(hnode.string)) != NULL))) {
      put_hnode(*group, 0, vnode.hits, vnode.files, vnode.pages, vnode.xfer, vlen, newhgrp);
   }
   else
   {
      /* Domain Grouping */
      if (config.group_domains)
      {
         const char *domain = get_domain((hostname) ? *hostname : hnode.string, config.group_domains);
         if (domain)
            put_hnode(string_t::hold(domain), 0, vnode.hits, vnode.files, vnode.pages, vnode.xfer, vlen, newhgrp);
      }
   }
   
   // update the host group count
   if(newhgrp)
      state.totals.t_grp_hosts++;

   //
   // Update country and city counters, ignoring robot and spammer activity
   //
   if(!hnode.robot && !hnode.spammer) {
      ccptr = &state.cc_htab.get_ccnode(hnode.get_ccode(), 0);

      ccptr->count += vnode.hits;
      ccptr->files += vnode.files;
      ccptr->pages += vnode.pages;
      ccptr->xfer += vnode.xfer;
      ccptr->visits++;

      ctnode_t& ctnode = state.ct_htab.get_ctnode(hnode.geoname_id, hnode.city, 0);

      ctnode.hits += vnode.hits;
      ctnode.files += vnode.files;
      ctnode.pages += vnode.pages;
      ctnode.xfer += vnode.xfer;
      ctnode.visits++;
   }
}

///
/// @brief  Retrieves host nodes from the DNS resolver and aggregates visits 
///         collected for each host while host name and country information 
///         were not available.
///
void webalizer_t::process_resolved_hosts(void)
{
   vnode_t *vptr;
   hnode_t *hptr;

   // go over all resolved host nodes
   while((hptr = dns_resolver.get_hnode()) != NULL) {
   
      // factor host visits into host data
      while((vptr = hptr->get_grp_visit()) != NULL) {
         group_host_by_name(*hptr, *vptr);
         delete vptr;
      }
   }
}

///
/// @brief  Removes index alias file names, such as `index.html`, from the URL.
///
void webalizer_t::proc_index_alias(string_t& url)
{
   const char *cp1;

   for(nlist::const_iterator lptr = config.index_alias.begin(); lptr != config.index_alias.end(); lptr++) {
      // use strstr becase index files can have search arguments (e.g. /index.html?n=v)
      if ((cp1 = strstr(url, lptr->string)) != NULL) {
         if(cp1 == url.c_str() || *(cp1-1) == '/') {
            url.truncate(cp1-url.c_str());
            if(url.isempty())
               url = "/"; 
            break;
         }
      }
   }
}

///
/// @brief  Removes various parts of the user agent string based on configured
///         level of mangling. 
///
/// This is the original implementation. It is no longer maintained and will
/// eventually be removed. 
///
void webalizer_t::mangle_user_agent(string_t& agent)
{
   string_t::char_buffer_t&& buffer = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;
   const char *str, *cp1;
   char *cp2;

   str = agent;
   cp2 = buffer;
 
   *cp2 = 0;

   cp1 = strstr(str,"ompatible"); /* check known fakers */
   if (cp1!=NULL) {
      while (*cp1!=';'&&*cp1!='\0') cp1++;
      /* kludge for Mozilla/3.01 (compatible;) */
      if (*cp1++==';' && strcmp(cp1,")\"")) { /* success! */
         while (*cp1 == ' ') cp1++; /* eat spaces */
         while (*cp1!='.'&&*cp1!='\0'&&*cp1!=';') *cp2++=*cp1++;
         
         if (config.mangle_agent < 5) {
            while (*cp1!='.'&&*cp1!=';'&&*cp1!='\0') *cp2++=*cp1++;
            if (*cp1!=';'&&*cp1!='\0') {
               *cp2++=*cp1++;
               *cp2++=*cp1++;
            }
         }
         
         if (config.mangle_agent<4)
            if (*cp1>='0'&&*cp1<='9') *cp2++=*cp1++;

         if (config.mangle_agent<3)
            while (*cp1!=';'&&*cp1!='\0'&&*cp1!='(') *cp2++=*cp1++;

         if (config.mangle_agent<2) {
            /* Level 1 - try to get OS */
            cp1=strstr(str,")");
            if (cp1!=NULL) {
               *cp2++=' ';
               *cp2++='(';
               while (*cp1!=';'&&*cp1!='('&&cp1!=str) cp1--;
               if (cp1!=str&&*cp1!='\0') cp1++;
               while (*cp1==' '&&*cp1!='\0') cp1++;
               while (*cp1!=')'&&*cp1!='\0') *cp2++=*cp1++;
               *cp2++=')';
            }
         }
         *cp2='\0';
      } else { /* nothing after "compatible", should we mangle? */
         /* not for now */
      }
   } else {
      cp1=strstr(str,"Opera");  /* Opera flavor         */
      if (cp1!=NULL) {
         while (*cp1!='/'&&*cp1!=' '&&*cp1!='\0') *cp2++=*cp1++;
         while (*cp1!='.'&&*cp1!='\0') *cp2++=*cp1++;
         if (config.mangle_agent<5) {
            while (*cp1!='.'&&*cp1!='\0') *cp2++=*cp1++;
            *cp2++=*cp1++;
            *cp2++=*cp1++;
         }
         if (config.mangle_agent<4)
            if (*cp1>='0'&&*cp1<='9') *cp2++=*cp1++;

         if (config.mangle_agent<3)
            while (*cp1!=' '&&*cp1!='\0'&&*cp1!='(') 
               *cp2++=*cp1++;

         if (config.mangle_agent<2) {
            cp1=strstr(str,"(");
            if (cp1!=NULL) {
             cp1++;
             *cp2++=' ';
             *cp2++='(';
             while (*cp1!=';'&&*cp1!=')'&&*cp1!='\0') 
            *cp2++=*cp1++;
             *cp2++=')';
            }
         }
         *cp2='\0';
      } else { 
         cp1=strstr(str,"Mozilla");  /* Netscape flavor      */
         if (cp1!=NULL) {
            while (*cp1!='/'&&*cp1!=' '&&*cp1!='\0') *cp2++=*cp1++;
            if (*cp1==' ') {*cp2++='/'; cp1++;}
            while (*cp1!='.'&&*cp1!='\0') *cp2++=*cp1++;

            if (config.mangle_agent<5) {
             while (*cp1!='.'&&*cp1!='\0') *cp2++=*cp1++;
             *cp2++=*cp1++;
             *cp2++=*cp1++;
            }

            if (config.mangle_agent<4)
               if (*cp1>='0'&&*cp1<='9') *cp2++=*cp1++;

            if (config.mangle_agent<3)
               while (*cp1!=' '&&*cp1!='\0'&&*cp1!='(') 
                  *cp2++=*cp1++;

            if (config.mangle_agent<2) {
               /* Level 1 - Try to get OS */
               cp1=strstr(str,"(");
               if (cp1!=NULL) {
                  cp1++;
                  *cp2++=' ';
                  *cp2++='(';
                  while (*cp1!=';'&&*cp1!=')'&&*cp1!='\0') 
                      *cp2++=*cp1++;
                  *cp2++=')';
               }
            }
            *cp2='\0';
         }
      }
   }

   agent = buffer;
}

///
/// @brief  Removes various parts of the user agent string based on the configured
///         level of mangling.
///
void webalizer_t::filter_user_agent(string_t& agent)
{
   //
   // Typical user agent strings:
   //
   // Opera/9.25 (Windows NT 5.1; U; de)
   // Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.7) Gecko/20070914 Firefox/2.0.0.7
   // Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322; .NET CLR 2.0.50727)
   // Mozilla/5.0 (compatible; Googlebot/2.1;  http://www.google.com/bot.html)
   // Mozilla/5.0 (compatible; Yahoo! Slurp; http://help.yahoo.com/help/us/ysearch/slurp)
   //
   // The parser interprets various delimiters as follows:
   //
   // "();"    - end all tokens
   // '/'      - changes string token into a version token
   // 'h'      - if the following string is "http://", changes token into a URL token
   // '.'      - the first dot of a version token marks the end of the major version number
   // " \t"    - ends version and URL tokens
   //
   static const char *str_delims = "h/();";     // string token delimiters
   static const char *ver_delims = ". \t();";   // version token end delimiters
   static const char *url_delims = " \t();";    // URL token end delimiters
   static const char *wht_delims = " \t();";    // whitespace delimiters
   
   static const char o_arg[] = {';', ' '};      // argument's output delimiters
   
   std::vector<ua_token_t, ua_token_alloc_t> ua_args(ua_token_alloc);      // user agent argument tokens
   std::vector<size_t, ua_grp_idx_alloc_t>   ua_groups(ua_grp_idx_alloc);  // grouped tokens within ua_args (indexes)

   ua_token_t token;
   const char *delims = str_delims;             // active delimiters
   char *cp1, *cp2;
   string_t::char_buffer_t ua, ua2;
   size_t ualen, arglen, t_arglen = 0;
   const string_t *str;
   bool skiptok = false;
   bool cpinplace = true;                       // copy arguments in place
   
   // return if user agent string is empty
   if(agent.isempty())
      return;
   
   // if all included, return - nothing to do
   if(config.incl_agent_args.iswildcard())
      return;
   
   // if all excluded and none included, reset the string and return   
   if(config.incl_agent_args.isempty() && config.excl_agent_args.iswildcard()) {
      agent.reset();
      return;
   }

   // most user agents will have less than 16 tokens
   ua_args.reserve(16);
   ua_groups.reserve(16);
   
   // save the initial string length   
   ualen = agent.length();
   
   // detach the string so we can manipulate it directly   
   ua = agent.detach();

   cp1 = cp2 = ua.get_buffer();

   token.reset(cp1, strtok);

   while(*cp2) {
      // find the next delimiter based on the current token type
      while(*cp2 && !strchr(delims, *cp2)) cp2++;
      
      // if the token type is still a string, check if we need to change it
      if(token.argtype == strtok) {
         if(*cp2 == '/') {
            // make it a version token
            token.namelen = cp2 - cp1;
            token.argtype = vertok;
            delims = ver_delims;
            cp2++;
            continue;
         }
         else if(*cp2 == 'h') {
            if(!string_t::compare_ci(++cp2, "ttp://", 6)) {
               // make it a URL token
               token.argtype = urltok;
               delims = url_delims;
               cp2 += 6;
            }
            continue;
         }
      }
      else if(token.argtype == vertok) {
         // compute major version length if it's not set yet
         if(!token.mjverlen)
            token.mjverlen = cp2 - cp1;         // Firefox/2
         else if(!token.mnverlen)
            token.mnverlen = cp2 - cp1;         // Firefox/2.0
            
         // if it's a dot, continue parsing
         if(*cp2 == '.') {
            cp2++;
            continue;
         }
      }
      
      // got the token, set argument length
      arglen = cp2 - cp1;
      
      // skip trailing spaces
      while(arglen && cp1[arglen-1] == ' ') arglen--;
      
      if(arglen) {
         // push a copy of a non-empty argument into the vector
         //    if the exclude list is empty OR
         //    if the argument is in the include list OR
         //    if the argument is not in the exclude list
         if(config.excl_agent_args.isempty() || 
               config.incl_agent_args.isinlistex(cp1, arglen, false) || 
               !config.excl_agent_args.isinlistex(cp1, arglen, false)) {
            
            // if there's no minor version, use the major version length
            if(!token.mnverlen)
               token.mnverlen = token.mjverlen;

            token.arglen = arglen;

            // update the total length
            t_arglen += arglen + sizeof(o_arg); // account for ';' and ' ' in the output

            // check if there's a matching group pattern for string tokens
            if(token.argtype == strtok && config.group_agent_args.size()) {
               if((str = config.group_agent_args.isinglist(cp1, arglen, false)) != NULL) {
                  // make sure there are no duplicates
                  for(size_t i = 0; i < ua_groups.size(); i++) {
                     // check if lengths are the same
                     if(ua_args[ua_groups[i]].arglen == str->length()) {
                        // and compare the strings
                        if(!strncmp(ua_args[ua_groups[i]].start, str->c_str(), str->length())) {
                           skiptok = true;      // already got it - skip the token
                           break;
                        }
                     }
                  }
               
                  if(!skiptok) {
                     // remember grouped item index
                     ua_groups.push_back(ua_args.size());
                  
                     // re-write the token
                     token.start = str->c_str();
                     token.arglen = str->length();
                     
                     // and update the total length (arg's output delimiters still included)
                     t_arglen += token.arglen;
                     t_arglen -= arglen;

                     // copy in-place only if the aliased name is not longer than the argument
                     if(cpinplace && str->length() > arglen)
                        cpinplace = false;
                  }
               }
            }
            
            if(!skiptok)
               ua_args.push_back(token);             // keep the token
            else
               skiptok = false;                 // skip and reset skiptok
         }
      }
      
      // skip whitespace
      while(*cp2 && strchr(wht_delims, *cp2)) cp2++;
      
      // check if at the end of the string
      if(*cp2) {
         // if not, point cp1 to the next token
         cp1 = cp2;
         
         // reset the token
         token.reset(cp1, strtok);
         delims = str_delims;
      }
   }
   
   if(t_arglen) {
      // there are extra ';' and ' ' at the end
      t_arglen -= sizeof(o_arg);

      //
      // If the final user agent string is shorter than or equal to the original 
      // and there are no replacement tokens, we can move existing tokens within 
      // the existing buffer because tokens can only be shortened or filtered out. 
      // Otherwise, if the final user agent string is longer or any of its tokens
      // come from the replacement list, allocate a new buffer.
      //
      if(cpinplace && t_arglen <= ualen)
         ua2 = std::move(ua);
      else
         ua2.resize(t_arglen+1, 0);
      
      // form a new user agent string
      cp1 = ua2;
      for(size_t i = 0; i < ua_args.size(); i++) {
         if(i) {
            for(size_t j = 0; j < sizeof(o_arg); j++) 
               *cp1++ = o_arg[j];
         }
         
         // if a version token, use mangle level to output version length
         if(ua_args[i].argtype == vertok) {
            if(config.mangle_agent >= 4)
               arglen = ua_args[i].namelen;     // name
            else if(config.mangle_agent == 3)
               arglen = ua_args[i].mjverlen;    // name/major
            else if(config.mangle_agent == 2)
               arglen = ua_args[i].mnverlen;    // name/major.minor
            else
               arglen = ua_args[i].arglen;      // output the entire argument
         }
         else
            arglen = ua_args[i].arglen;
         
         // copy the token and advance the pointer   
         if(cp1 != ua_args[i].start)
            memmove(cp1, ua_args[i].start, arglen);
         cp1 += arglen;
      }
      *cp1 = 0;
   }

   // clear both vectors
   ua_args.clear();
   ua_groups.clear();
   
   if(ua2)
      agent.attach(std::move(ua2), cp1-ua2.get_buffer());    // attach the new string
   else
      agent.reset();                            // reset to an empty agent string
}

///
/// @brief  Ends all active visits and downloads, saves the state and rolls over
///         the database.
///
int webalizer_t::end_month(void)
{
   update_visits(tstamp_t());
   update_downloads(tstamp_t());
   
   state.update_hourly_stats();
   
   if (state.save_state()) {
      /* Error: Unable to save current run data */
      fprintf(stderr,"%s\n",config.lang.msg_data_err);
      return 1;
   }
   
   state.clear_month();

   return 0;
}

///
/// @brief  Prints summary information about the selected database.
///
int webalizer_t::database_info(void)
{
   state.database_info();

   return 0;
}

///
/// @brief  Generates a report using data from the selected database.
///
int webalizer_t::prep_report(void)
{
   write_monthly_report();
   write_main_index();

   return 0;
}

///
/// @brief  Reclaims some of the unused space within the selected state database 
///         and reduces the overall database size. 
///
int webalizer_t::compact_database(void)
{
   u_int bytes;
   database_t::status_t status;

   if((status = state.database.compact(bytes)).success()) {
      if(config.verbose > 1)
         printf("%s: %d KB\n", config.lang.msg_cmpctdb, bytes/1024);
   }
   else
      fprintf(stderr, "Cannot compact the database (%s)\n", status.err_msg().c_str());
   
   return 0;
}

///
/// @brief  Runs the appropriate handler for the active command.
///
int webalizer_t::run(void)
{
   uint64_t start_ts;                        // start of the run
   uint64_t end_ts;                          // end of the run
   uint64_t tot_time, proc_time;                    /* temporary time storage      */
   proc_times_t ptms;
   logrec_counts_t lrcnt;
   u_int rps;
   int retcode;

   start_ts = msecs();

   if(config.prep_report) {
      retcode = prep_report();
      ptms.rpt_time += elapsed(start_ts, msecs());
   }
   else if(config.compact_db) {
      retcode = compact_database();
      ptms.mnt_time += elapsed(start_ts, msecs());
   }
   else if(config.end_month) {
      retcode = end_month();
      ptms.mnt_time += elapsed(start_ts, msecs());
   }
   else if(config.db_info) {
      retcode = database_info();
      ptms.mnt_time += elapsed(start_ts, msecs());
   }
   else
      retcode = proc_logfile(ptms, lrcnt);

   end_ts = msecs();              
   
   /* display timing totals?   */
   if (config.time_me || config.verbose > 1) {
      /* get total (end-start) and processing time */
      tot_time = elapsed(start_ts, end_ts);
      proc_time = tot_time - ptms.dns_time - ptms.mnt_time - ptms.rpt_time;

      if(!config.prep_report && !config.compact_db && !config.end_month && !config.db_info) {
         // output number of processed, ignored and bad records
         printf("%s %" PRIu64 " %s ", config.lang.msg_processed, lrcnt.total_rec, config.lang.msg_records);
         if (lrcnt.total_ignore) {
            printf("(%" PRIu64 " %s", lrcnt.total_ignore, config.lang.msg_ignored);
            if (lrcnt.total_bad) 
               printf(", %" PRIu64 " %s) ", lrcnt.total_bad, config.lang.msg_bad);
            else
               printf(") ");
         }
         else if (lrcnt.total_bad) 
            printf("(%" PRIu64 " %s) ", lrcnt.total_bad, config.lang.msg_bad);

         // output processing time
         printf("%s %.2f %s", config.lang.msg_in, proc_time/1000., config.lang.msg_seconds);

         /* calculate records per second */
         if (tot_time)
            rps = ((u_int)((double) lrcnt.total_rec / (proc_time / 1000.)));
         else 
            rps = 0;

         if(rps > 0 && rps <= lrcnt.total_rec)
            printf(", %d/sec\n", rps);
         else  
            printf("\n");

         if(config.verbose && config.is_dns_enabled()) {
            if(dns_resolver.dns_cached || dns_resolver.dns_resolved)
               printf("%s: %" PRIu64 "%% (%" PRIu64 ":%" PRIu64 ")\n", config.lang.msg_dns_htrt, (uint64_t) (dns_resolver.dns_cached * 100. / (dns_resolver.dns_cached + dns_resolver.dns_resolved)), dns_resolver.dns_cached, dns_resolver.dns_resolved);
         }

         // report total DNS time
         printf("%s %.2f %s\n", config.lang.msg_dnstime, ptms.dns_time/1000., config.lang.msg_seconds);
      }

      // report total report generation time
      if(!config.batch && !config.compact_db && !config.end_month && !config.db_info)
         printf("%s %.2f %s\n", config.lang.msg_rpttime, ptms.rpt_time/1000., config.lang.msg_seconds);

      // report maintenance and total run time
      printf("%s %.2f %s\n", config.lang.msg_mnttime, ptms.mnt_time/1000., config.lang.msg_seconds);
      printf("%s %.2f %s\n", config.lang.msg_runtime, tot_time/1000., config.lang.msg_seconds);
   }

   return retcode;
}

///
/// @brief  Checks that each log file specified in the configuration is readable 
///         and populates the log file list parameter with instances of `logfile_t`.
///
void webalizer_t::prep_logfiles(logfile_list_t& logfiles)
{
   //
   // Walk through the list of log files and test if each one is readable. Do not 
   // open files at this point, so we don't exceed the limit on the total number 
   // of open files.
   //
   std::vector<string_t>::const_iterator iter = config.log_fnames.begin();
   while(iter != config.log_fnames.end()) {
      const string_t& fname = *iter++;
      // VC++ Intellisense erroneously highlights make_path as trying to create a string with `const string_t&&`
      std::unique_ptr<logfile_t> logfile(new logfile_t(fname.length() && !is_abs_path(fname) ? (const string_t&) make_path(config.cur_dir, fname) : fname));
      
      // check if we can read the file
      if(!logfile->is_readable()) {
         /* Error: Can't open log file ... */
         throw exception_t(0, string_t::_format("%s %s\n",config.lang.msg_log_err, fname.c_str()));
      }

      // set a one-based log file ID, so we can identify log files when we report bad log records
      logfile->set_id((u_int) (logfiles.size() + 1));
      
      /* Using logfile ... */
      if (config.verbose>1)
      {
         printf("%s %s (",config.lang.msg_log_use, !fname.isempty() ? make_path(config.cur_dir, fname).c_str() : "STDIN");
         if (logfile->is_gzip()) printf("gzip-");
         printf("%s)\n", config.get_log_type_desc());
      }

      logfiles.push_back(logfile.release());
   }
}

///
/// @brief  Prepares a log file processing state for each of the log files in the 
///         log file list. 
///
/// This method reads the first valid log record from each log file and creates a 
/// log file state instance containing a populated log record and the corresponding 
/// log file. Each log file state instance is inserted into the state list in the 
/// ascending order of log record time stamps. 
///
/// Those log files that do not have valid log records are removed from the log file 
/// list, so when the function returns, the number of log file states matches the 
/// number of log files. 
///
void webalizer_t::prep_lfstates(logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs, logrec_counts_t& lrcnt)
{
   string_t::char_buffer_t&& buffer = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;
   int parse_code;
   int errnum = 0;
   size_t reclen;
   lfp_state_t wlfs;                   // working log file state

   //
   // Loop through the log lines of each log file until we find a valid log
   // record. Store the first good log record and the file pointer in a state
   // and insert the state into the state list, ordered by the time stamp.
   // Repeat the process for each log file. After this loop we will have same 
   // number of log files, log records and states in the lists and the working 
   // log file state structure (wlfs) will be empty.
   //
   for(logfile_list_t::iterator i = logfiles.begin(); i != logfiles.end();) {
      // the file may be open if we skipped any lines
      if(!(*i)->is_open() && (errnum = (*i)->open()) != 0) {
         /* Error: Can't open log file ... */
         throw exception_t(0, string_t::_format("%s %s (%d)", config.lang.msg_log_err, (*i)->get_fname().c_str(), errnum));
      }

      // read the log record into the buffer
      if((reclen = read_log_line(buffer, **i, lrcnt)) == 0) {
         // report if there's no more data
         printf("%s %s\n", config.lang.msg_log_done, (*i)->get_fname().c_str());

         // close the log file and clean up
         (*i)->close();
         delete (*i);
         i = logfiles.erase(i);
         
         // delete the last log record and remove it from the list
         if(wlfs.logrec) {
            delete logrecs.back();
            logrecs.pop_back();
         }

         // do not leave dangling poiters behind
         wlfs.reset();

         continue;
      }
      
      // allocate a log record and set up the state structure
      if(!wlfs.logrec) {
         wlfs.logfile = *i;
         wlfs.logrec = new log_struct;
         logrecs.push_back(wlfs.logrec);
      }

      // parse the log line
      if((parse_code = parse_log_record(buffer, reclen, *wlfs.logrec, wlfs.logfile->get_id(), lrcnt.total_rec)) == PARSE_CODE_ERROR) {
         lrcnt.total_bad++;
         continue;
      }
      
      // skip log file directives, etc
      if(parse_code == PARSE_CODE_IGNORE) {
         lrcnt.total_ignore++;
         continue;
      }
      
      //
      // There's a limit on how many files can be opened simulteneously. If we 
      // reached the maximum, close the file here and it will be opened again 
      // when the next line is read in the main loop.
      //
      if(logfiles.size() >= FOPEN_MAX) {
         if((*i)->set_reopen_offset() == -1L)
            throw exception_t(0, string_t::_format("%s %s (%d)\n", config.lang.msg_fpos_err, (*i)->get_fname().c_str(), errnum));
            
         (*i)->close();
      }
      
      // find the spot in the list and insert the record (earlier timestamps first)
      lfp_state_list_t::iterator j = lfp_states.begin();
      while(j != lfp_states.end() && wlfs.logrec->tstamp > j->logrec->tstamp) j++;
      lfp_states.insert(j, wlfs);

      // reset the working state and move onto the next log file
      wlfs.reset();
      i++;
   }
}

///
/// @brief  Returns a working log file state with a log record with the oldest
///         time stamp across all log file states. 
///
/// If `get_logrec` is called when the number of log file states is the same as 
/// the number of files, the first state is moved to `wlfs` and the function 
/// returns `true`, so the log record inside `wlfs` may be processed by the 
/// caller. 
///
/// Otherwise, if the number of log file states is less than the number of log 
/// files and the log file in `wlfs` has at least one valid log record, then a 
/// new log record is read and the new log file state is inserted into the state
/// list according to the log record time stamp. The first state from the list 
/// is then returned to the caller in `wlfs`, as described in the first paragraph.
///
/// If there are no more valid records in the log file in `wlfs`, the matching 
/// log file is removed from the log file list and the first state is returned 
/// to the caller, as described in the first paragraph.
///
/// Each file is opened only once, so the total number of open files will be 
/// determied by how many of them have log records in approximately same range
/// and how close log records are to each other. 
///
bool webalizer_t::get_logrec(lfp_state_t& wlfs, logfile_list_t& logfiles, lfp_state_list_t& lfp_states, logrec_list_t& logrecs, logrec_counts_t& lrcnt)
{
   string_t::char_buffer_t&& buffer = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;
   int parse_code;
   int errnum = 0;
   size_t reclen;

   //
   // If we have fewer states than log files, then we either need to add a 
   // state or remove the log file identified by wlfs from the list.
   //
   while(lfp_states.size() < logfiles.size()) {
      // the log file in wlfs may be closed if wlfs is returned to the state vector first time
      if(!wlfs.logfile->is_open() && (errnum = wlfs.logfile->open()) != 0) {
         /* Error: Can't open log file ... */
         throw exception_t(0, string_t::_format("%s %s (%d)\n", config.lang.msg_log_err, wlfs.logfile->get_fname().c_str(), errnum));
      }

      // use logfile from wlfs, which was populated in the previous iteration
      if((reclen = read_log_line(buffer, *wlfs.logfile, lrcnt)) == 0) {
         logfile_list_t::iterator i;
            
         // report that we are done with this log file
         printf("%s %s\n", config.lang.msg_log_done, wlfs.logfile->get_fname().c_str());

         // find it in the list, remove it and clean up
         for(i = logfiles.begin(); i != logfiles.end() && wlfs.logfile != *i; i++);
         (*i)->close();
         delete *i;
         logfiles.erase(i);

         // log records are cleaned up after all log files are processed
         wlfs.reset();
            
         break;
      }
         
      // parse the log line
      if((parse_code = parse_log_record(buffer, reclen, *wlfs.logrec, wlfs.logfile->get_id(), lrcnt.total_rec)) == PARSE_CODE_ERROR) {
         lrcnt.total_bad++;
         continue;
      }
         
      if(parse_code == PARSE_CODE_IGNORE) {
         lrcnt.total_ignore++;
         continue;
      }

      // find the spot in the list and insert the record (earlier timestamps first)
      lfp_state_list_t::iterator j = lfp_states.begin();
      while(j != lfp_states.end() && wlfs.logrec->tstamp > j->logrec->tstamp) j++;
      lfp_states.insert(j, wlfs);
      wlfs.reset();
   }
   
   // if there are no unprocessed states, we've gone through all log files
   if(lfp_states.empty())
      return false;

   //
   // Get the first state and remove it from the list. The log file in the  
   // working state lets us keep track of which log file needs to be read from.
   //
   wlfs = lfp_states.front();
   lfp_states.pop_front();

   return true;
}

///
/// @brief  Reads log records from specified log files and aggregates counts
///         for each log record entity, such as an IP address or a URL, in 
///         the state database.
///
int webalizer_t::proc_logfile(proc_times_t& ptms, logrec_counts_t& lrcnt)
{
   storable_t<hnode_t> *hptr;
   storable_t<unode_t> *uptr;
   bool newvisit, newhost, newthost, newurl, newagent, newuser, newerr, newref, newdl, newspammer;
   bool newrgrp, newugrp, newagrp, newigrp;
   bool pageurl, fileurl, httperr, robot = false, target, spammer = false, goodurl, entryurl, exiturl;
   const string_t *sptr, empty, *ragent = NULL;
   uint64_t stime;
   bool newsrch = false;
   u_short termcnt = 0;
   string_t srchterms;
   string_t urlhost;

   int64_t htab_tstamp = 0;            ///< A time stamp for all hash tables

   uint64_t total_good = 0;

   int retcode = 0;

   bool check_dup = false;             // check for duplicate time stamps for initial log records?

   lfp_state_t wlfs;                   // working log file state
   lfp_state_list_t lfp_states;        // log file states ordered by log time
   logfile_list_t logfiles;            // owns log files
   logrec_list_t logrecs;              // contains one log record per log file; owns log records
   
   tm_ranges_t::iterator dst_iter = config.dst_ranges.begin();

   // check for duplicates only if we processed any log records in the past
   check_dup = config.incremental && !state.totals.cur_tstamp.null;

   // reserve enough memory for most URL hosts
   urlhost.reserve(128);

   // populate the list of log files and make sure they are readable
   prep_logfiles(logfiles);

   // populate log file states, so we have one log record per log file, ordered by time
   prep_lfstates(logfiles, lfp_states, logrecs, lrcnt);

   //
   // Main processing loop - go through the log files until we run out of them.
   //
   while(logfiles.size()) {
      
      // if we detected a Ctrl-C, break out right away 
      if(abort_signal) {
         fprintf(stderr, "%s\n", config.lang.msg_ctrl_c);
         break;
      }
   
      // get the next record in the working log file state structure
      if(get_logrec(wlfs, logfiles, lfp_states, logrecs, lrcnt)) {
         log_struct& log_rec = *wlfs.logrec;
         
         newspammer = newthost = newvisit = false;

         /*********************************************/
         /* GOOD RECORD, CHECK INCREMENTAL/TIMESTAMPS */
         /*********************************************/

         //
         // Convert the log record time zone only if local time setting in the configuration 
         // doesn't match the log record time zone setting. This ensures that local time 
         // stamps in Apache and CLF logs are not converted to a different time zone using 
         // UTCOffset, which may not even be set, in which case it will appear that the time 
         // stamp is adjusted to UTC, while it is actually local time and may be DST adjusted.
         //
         if(config.local_time != log_rec.tstamp.islocal()) {
            if(config.local_time)
               log_rec.tstamp.tolocal(config.get_utc_offset(log_rec.tstamp, dst_iter));
            else
               log_rec.tstamp.toutc();
         }

         /* get current records timestamp (seconds since epoch) */
         tstamp_t& rec_tstamp = log_rec.tstamp;

         // hold onto the time stamp value, so we don't have to do time math more than we need
         htab_tstamp = rec_tstamp.mktime();

         //
         // Skip log records that we processed in the past, but not the first few that have 
         // the same time stamp (i.e. the first good log record sets cur_tstamp in the state 
         // and the following few with the same time stamp are skipped while check_dup is 
         // true).
         //
         if (check_dup && !total_good)
         {
            /* check if less than/equal to last record processed            */
            if ( rec_tstamp <= state.totals.cur_tstamp )
            {
               /* if it is, assume we have already processed and ignore it  */
               lrcnt.total_ignore++;
               continue;
            }

            /* if it isn't.. disable any more checks this run            */
            check_dup = false;

            //
            // If the previous run ended on the last record for the current month,
            // we might be able to keep the report generated at the end of that
            // run instead of generating a new report.
            //
            // Original code used to reset the state timestamp, based on the 
            // assumption that the report generated in the previous run was 
            // a valid report, which was the case at the time because at the 
            // end of a run the state was saved first and then all active 
            // visits terminated in memory before the report was generated.
            //
            // In the current incremental model, visits are no longer terminated  
            // before generating a report, except at the end of the month.  
            // Consequently, the report generated after the previous run is not 
            // usable if there are any active visits at the end of the log file 
            // and a new report must be generated at the end of the month.
            //
            if(state.totals.cur_tstamp.year != log_rec.tstamp.year || state.totals.cur_tstamp.month != log_rec.tstamp.month) {
               // check if there are active visits
               if(state.totals.t_visits == state.totals.t_visits_end) {
                     // if there are none, clear the month and
                     state.clear_month();

                     // reset the current state timestamp
                     state.totals.cur_tstamp = log_rec.tstamp;
                     state.totals.f_day=state.totals.l_day=log_rec.tstamp.day;
               }
            }
         }

         // check for out of sequence records
         if (rec_tstamp < state.totals.cur_tstamp) {
            lrcnt.total_ignore++; 
            continue; 
         }

         total_good++;

         /********************************************/
         /* PROCESS RECORD                           */
         /********************************************/

         //
         // Check for month change. The state timestamp must not be updated until 
         // reports are generated and the database is rolled over, so each database
         // contains the right start and end time stamps for its month.
         //
         if (!state.totals.cur_tstamp.null) {
            if (state.totals.cur_tstamp.year != log_rec.tstamp.year || state.totals.cur_tstamp.month != log_rec.tstamp.month)
            {
               if(config.is_dns_enabled()) {
                  stime = msecs();
                  dns_resolver.dns_wait();
                  ptms.dns_time += elapsed(stime, msecs());
               }

               //
               // Terminate all visits for the current month. This operation splits 
               // active visits at the month boundary, which is by design. The sum 
               // of all hits of all visits must match the total number of hits. If 
               // active visits are moved to either of the months, hit counts will 
               // no longer match for both months. 
               //
               update_visits(tstamp_t());
               update_downloads(state.totals.cur_tstamp);
            
               // state_t::set_tstamp is called later, so update hourly stats now
               state.update_hourly_stats();

               if(config.is_dns_enabled())
                  process_resolved_hosts();

               // save run data for the report generator
               stime = msecs();
               if (state.save_state()) {
                  /* Error: Unable to save current run data */
                  fprintf(stderr,"%s\n",config.lang.msg_data_err);
                  // report generator uses saved state data
                  return 1;
               }
               ptms.mnt_time += elapsed(stime, msecs());

               // generate monthly reports if not in batch mode
               if(!config.batch) {
                  database_t::status_t status;
                  stime = msecs();
                  if(!(status = state.database.attach_indexes(true)).success())
                     throw exception_t(0, string_t::_format("Cannot create secondary database indexes (%s)", status.err_msg().c_str()));
                  write_monthly_report();                /* generate HTML for month */
                  ptms.rpt_time += elapsed(stime, msecs());
               }

               stime = msecs();
               state.clear_month();
               ptms.mnt_time += elapsed(stime, msecs());
            }
         }

         //
         // Update the state with the new time stamp
         //
         state.set_tstamp(log_rec.tstamp);

         /*********************************************/
         /* DO SOME PRE-PROCESS FORMATTING            */
         /*********************************************/

         // check non-proxy requests against the spam referrers list
         if(config.log_type != LOG_SQUID)
            spammer = config.spam_refs.isinlist(log_rec.refer) != NULL;

         // reset search terms
         termcnt = 0;
         srchterms.clear();

         // check if the query string contains any search terms
         if(config.log_type == LOG_SQUID) {
            // count only successful requests (unlike with referrers)
            if(log_rec.resp_code == RC_OK)
               srch_string(log_rec.url, log_rec.srchargs, termcnt, srchterms, false);
         }
         else {
            if(!spammer) {
               // check if it's a partial request and ignore the referrer if requested
               if(!config.ignore_referrer_partial || log_rec.resp_code != RC_PARTIALCONTENT) {
                  if(srch_string(log_rec.refer, log_rec.xsrchstr, termcnt, srchterms, true))
                     spammer = true;
               }
            }
         }

         // remember spammers
         if(spammer)
            state.sp_htab.insert(log_rec.hostname);

         //
         // Filter and, optionally, sort search arguments. Afer filter_srchargs returns,
         // the search argument string in tthe log record will be rearranged according to
         // the filters and sorting settings. Individual search arguments may be accessed
         // via pointers in sr_args, which point to the search argument string in the the 
         // log record and must be maintained together to avoid dangling pointers.
         //
         std::vector<arginfo_t, srch_arg_alloc_t> sr_args(srch_arg_alloc);
         filter_srchargs(log_rec.srchargs, sr_args);

         /* strip off index.html (or any aliases) */
         if(config.index_alias.size())
            proc_index_alias(log_rec.url);

         //
         // Robot policies:
         //
         // 1. Robots are identified before user agents are mangled
         //
         // 2. Robot hits are ignored based just on the user agent string
         //
         // 3. Hosts are marked as robots when user agent matches one of the Robot entries 
         // and only when an hnode is created. If a human and a robot share the same IP 
         // address, this address will be marked as robot or non-robot depending on which 
         // user agent was active when the first request was made.
         //
         // 4. A visit is marked as a robot visit when the user agent matches one of the 
         // Robot entries and a vnode is created, regardless whether the host is marked 
         // as a robot or not. The robot flag in the visit is purely informational. See
         // note #4 in vnode_t for details.
         //
         // 5. A user agent is marked as a robot if the current visit is marked as a robot.
         //
         // 7. Country totals do not include robot activity.
         //
         
         // do not look up robot agent for proxy requests
         if(config.log_type != LOG_SQUID) {
            //
            // If robots can be ignored, set ragent now, otherwise, do it after the ignore 
            // check, so we avoid a look-up if matches some other ignore criteria.
            //
            if(config.ignore_robots)
               ragent = (!spammer) ? config.robots.isinglist(log_rec.agent) : NULL;
         }

         //
         // Ignore/Include check
         //

         if ( (config.include_hosts.isinlist(log_rec.hostname)==NULL) &&
              (config.include_urls.isinlist(log_rec.url)==NULL)       &&
              (config.include_refs.isinlist(log_rec.refer)==NULL)     &&
              (config.include_agents.isinlist(log_rec.agent)==NULL)   &&
              (config.include_users.isinlist(log_rec.ident)==NULL)    )
         {
            if(ragent && config.ignore_robots)
              { lrcnt.total_ignore++; continue; }
            if (config.ignored_hosts.isinlist(log_rec.hostname) != NULL)
              { lrcnt.total_ignore++; continue; }
            if (config.ignored_agents.isinlist(log_rec.agent)!=NULL)
              { lrcnt.total_ignore++; continue; }
            if (config.ignored_refs.isinlist(log_rec.refer)!=NULL)
              { lrcnt.total_ignore++; continue; }
            if (config.ignored_users.isinlist(log_rec.ident)!=NULL)
              { lrcnt.total_ignore++; continue; }

            // check the ignore URL filter, which may contain optional search argument names
            if(check_ignore_url_list(log_rec.url, log_rec.srchargs, sr_args)) {
               lrcnt.total_ignore++; 
               continue;
            }
         }

         // done with individual search arguments and can dispose of them
         sr_args.clear();

         // do not look up robot agent for proxy requests
         if(config.log_type != LOG_SQUID) {
            // if not ignored, check if a robot and set ragent (ignore spammers)
            if(!config.ignore_robots)
               ragent = (!spammer) ? config.robots.isinglist(log_rec.agent) : NULL;
         }

         /* Do we need to mangle? */
         if(config.mangle_agent) {
            if (config.use_classic_mangler)
               mangle_user_agent(log_rec.agent);
            else
               filter_user_agent(log_rec.agent);
         }
            
         /* Bump response code totals */
         state.response.get_status_code(log_rec.resp_code).count++;

         //
         // now save in the various hash tables...
         //
         
         goodurl = log_rec.resp_code==RC_OK || log_rec.resp_code==RC_NOMOD || log_rec.resp_code==RC_PARTIALCONTENT;
         fileurl = log_rec.resp_code==RC_OK || log_rec.resp_code==RC_PARTIALCONTENT;
         pageurl = config.ispage(log_rec.url);
         httperr = is_http_error(log_rec.resp_code);
         target = goodurl ? config.target_urls.isinlist(log_rec.url) ? true : config.target_downloads && config.downloads.isinlist(log_rec.url) ? true : false : false;
         
         // do not check proxy requests for spammers
         if(config.log_type != LOG_SQUID) {
            // if appears to be not a spammer, check their past
            if(!spammer)
               spammer = state.sp_htab.find(log_rec.hostname) != state.sp_htab.end();
         }
         
         // initialize those that may be not set otherwise (e.g. if URL is not added)
         newurl = newagent = newuser = newerr = newref = newdl = newrgrp = newugrp = newagrp = newigrp = false;

         //
         // host name hash table - monthly
         //
         // put_hnode sets newvisit and must be called before any other put_xnode 
         // function.
         //
         hptr = put_hnode(log_rec.hostname, rec_tstamp, htab_tstamp, log_rec.xfer_size, fileurl, pageurl, 
            spammer, ragent != NULL, target, newvisit, newhost, newthost, newspammer);

         // 
         // Host nodes need to be resolved before their visits can be attributed to various 
         // groups (i.e. IP address, domain name and country groups) and marked as spammers 
         // or not spammers, based on historical host address data from the DNS resolver 
         // database. Queue a new host or a host that was just identfied as a spammer to the 
         // DNS resolver.
         //
         if(config.is_dns_enabled() && (newhost || newspammer))
            dns_resolver.put_hnode(hptr);

         // proxy logs do not contain any robot activity
         if(config.log_type != LOG_SQUID) {
            // use the host robot flag (ignore mid-visit ragent)
            robot = hptr->robot;
         }

         //
         // Entry URL may be not the first URL in the visit either because the
         // entry request was logged later than the URL requested after the entry 
         // URL or because the initial entry URL was redirected to the actual
         // entry URL. Consider a URL as an entry URL if there wasn't one before 
         // for the current visit.
         //
         // IMPORTANT: entry URL criteria must not be less restrictive than the
         // one used for put_unode.
         //
         entryurl = goodurl && !robot && !spammer && (!config.page_entry || pageurl) && (newvisit || hptr && !hptr->entry_url_set());
         exiturl = goodurl && !robot && !spammer && (!config.page_entry || pageurl);
         
         // update the visit to indicate that the entry URL has been seen
         if(entryurl && hptr)
            hptr->set_entry_url();

         //
         // URL/ident hash table (only if valid response code)
         //
         if(goodurl) {
            /* URL hash table */
            uptr = put_unode(log_rec.url, htab_tstamp, log_rec.srchargs, OBJ_REG,
                log_rec.xfer_size, log_rec.proc_time/1000., log_rec.port, entryurl, target, newurl);
            
            // update the last URL for the current visit
            if(hptr && exiturl) hptr->set_last_url(uptr);

            /* ident (username) hash table */
            if(!log_rec.ident.isempty())
               put_inode(log_rec.ident, htab_tstamp, OBJ_REG, fileurl, log_rec.xfer_size, rec_tstamp, log_rec.proc_time/1000., newuser);
         }

         //
         // Downloads
         //
         if(config.ntop_downloads || config.dump_downloads) {
            if((sptr = config.downloads.isinglist(log_rec.url)) != NULL) {
               if(log_rec.resp_code == RC_OK || log_rec.resp_code == RC_PARTIALCONTENT)
                  put_dlnode(*sptr, htab_tstamp, log_rec.resp_code, rec_tstamp, log_rec.proc_time, log_rec.xfer_size, *hptr, newdl);
            }
         }

         //
         // error HTTP response codes
         //
         if(httperr && (config.ntop_errors || config.dump_errors))
            put_rcnode(log_rec.method, htab_tstamp, log_rec.url, log_rec.resp_code, false, 1, &newerr);

         //
         // referrer hash table
         //
         if (config.ntop_refs && !log_rec.refer.isempty())
         {
            // filter out spammers
            if(!spammer) {
               // check if it's a partial request and ignore the referrer if requested
               if(!config.ignore_referrer_partial || log_rec.resp_code != RC_PARTIALCONTENT)
                  put_rnode(log_rec.refer, htab_tstamp, OBJ_REG, (uint64_t)1, newvisit, newref);
            }
         }

         //
         // User agents must be processed after client host addresses
         // because visits for a user agent are counted based on the
         // value of newvisit populated by put_hnode.
         //
         // Ignore the robot flag if user agents are mangled to avoid  
         // mixing up robots and non-robots (i.e. if the mangled agent
         // string matches both).
         //
         /* user agent hash table */
         if (config.ntop_agents)
         {
            if(!log_rec.agent.isempty())
               put_anode(log_rec.agent, htab_tstamp, OBJ_REG, log_rec.xfer_size, newvisit, !config.use_classic_mangler && robot, newagent);
         }

         /* do search string stuff if needed     */
         if(config.ntop_search) {
            newsrch = false;
            if(termcnt && !srchterms.isempty())
               put_snode(srchterms, htab_tstamp, termcnt, newvisit, newsrch);
         }

         //
         // bump monthly/daily/hourly totals
         //
         state.totals.t_hit++; state.totals.ht_hits++;                        /* daily/hourly hits    */
         state.totals.t_xfer += log_rec.xfer_size;                            // total xfer size
         state.totals.ht_xfer += log_rec.xfer_size;                           // hourly xfer size
         state.t_daily[log_rec.tstamp.day-1].tm_xfer += log_rec.xfer_size;    /* daily xfer total     */
         state.t_daily[log_rec.tstamp.day-1].tm_hits++;                       /* daily hits total     */
         state.t_hourly[log_rec.tstamp.hour].th_xfer += log_rec.xfer_size;    /* hourly xfer total    */
         state.t_hourly[log_rec.tstamp.hour].th_hits++;                       /* hourly hits total    */

         // update total host/URL/etc counters (htab size, minus group count)
         if(newhost) {state.totals.t_hosts++; state.totals.ht_hosts++;}
         if(newthost) state.t_daily[state.totals.cur_tstamp.day-1].tm_hosts++;
         if(newurl) state.totals.t_url++;
         if(newagent) state.totals.t_agent++;
         if(newuser) state.totals.t_user++;
         if(newerr) state.totals.t_err++;
         if(newref) state.totals.t_ref++;
         if(newdl) state.totals.t_downloads++;
         
         if(robot) {
            state.totals.t_rhits++; 
            state.totals.t_rxfer += log_rec.xfer_size;
            if(httperr) state.totals.t_rerrors++;
            if(newhost) state.totals.t_rhosts++;
         }

         if(newvisit) {
            state.t_daily[state.totals.cur_tstamp.day-1].tm_visits++;
            state.totals.t_visits++;
            if(robot) state.totals.t_rvisits++;
            state.totals.ht_visits++;
         }

         if(config.ntop_search && newsrch) state.totals.t_search++;

         /* 
         // Monthly average and maximum hit/file/page processing time 
         */
         state.totals.a_hitptime = AVG(state.totals.a_hitptime, ((double) log_rec.proc_time / 1000.), state.totals.t_hit);
         state.totals.m_hitptime = std::max(state.totals.m_hitptime, ((double) log_rec.proc_time / 1000.));

         /* if RC_OK, increase file counters */
         if (fileurl)
         {
            state.totals.t_file++; state.totals.ht_files++;
            state.t_daily[log_rec.tstamp.day-1].tm_files++;
            state.t_hourly[log_rec.tstamp.hour].th_files++;

            state.totals.a_fileptime = AVG(state.totals.a_fileptime, ((double) log_rec.proc_time / 1000.), state.totals.t_file);
            state.totals.m_fileptime = std::max(state.totals.m_fileptime, ((double) log_rec.proc_time / 1000.));

            if(robot) state.totals.t_rfiles++;
         }

         /* Pages (pageview) calculation */
         if (pageurl) {
            state.totals.t_page++; state.totals.ht_pages++;
            state.t_daily[log_rec.tstamp.day-1].tm_pages++;
            state.t_hourly[log_rec.tstamp.hour].th_pages++;

            state.totals.a_pageptime = AVG(state.totals.a_pageptime, ((double) log_rec.proc_time / 1000.), state.totals.t_page);
            state.totals.m_pageptime = std::max(state.totals.m_pageptime, ((double) log_rec.proc_time / 1000.));

            if(robot) state.totals.t_rpages++;
         }

         /*********************************************/
         /* RECORD PROCESSED - DO GROUPS HERE         */ 
         /*********************************************/

         /* URL Grouping */
         if((sptr = config.group_urls.isinglist(log_rec.url))!=NULL)
            put_unode(*sptr, 0, empty, OBJ_GRP, log_rec.xfer_size, log_rec.proc_time/1000., 0, false, false, newugrp);

         // group URL domains for proxy requests
         if(config.log_type == LOG_SQUID) {
            if(config.group_url_domains && !get_url_host(log_rec.url, urlhost).isempty()) {
               const char *domain = get_domain(urlhost.c_str(), config.group_url_domains);
               put_unode(string_t::hold(domain), 0, empty, OBJ_GRP, log_rec.xfer_size, log_rec.proc_time/1000., 0, false, false, newugrp);
            }
         }

         /* Referrer Grouping */
         if((sptr = config.group_refs.isinglist(log_rec.refer))!=NULL)
            put_rnode(*sptr, 0, OBJ_GRP, 1ul, newvisit, newrgrp);

         /* User Agent Grouping */
         if((sptr = config.group_agents.isinglist(log_rec.agent))!=NULL)
            put_anode(*sptr, 0, OBJ_GRP, log_rec.xfer_size, newvisit, false, newagrp);

         // group robots
         if(robot && ragent && config.group_robots)
            put_anode(*ragent, 0, OBJ_GRP, log_rec.xfer_size, newvisit, true, newagrp);

         /* Ident (username) Grouping */
         if((sptr = config.group_users.isinglist(log_rec.ident))!=NULL)
            put_inode(*sptr, 0, OBJ_GRP, fileurl, log_rec.xfer_size, rec_tstamp, log_rec.proc_time/1000., newigrp);

         // update group counts (host counts are updated in process_resolved_hosts)
         if(newugrp) state.totals.t_grp_urls++;
         if(newigrp) state.totals.t_grp_users++;
         if(newrgrp) state.totals.t_grp_refs++;
         if(newagrp) state.totals.t_grp_agents++;

         //
         // Retrieve all resolved host nodes and group host names and countries
         //
         if(config.is_dns_enabled())
            process_resolved_hosts();

         //
         // Every thousand records swap out nodes older than two visit timeouts. This
         // number is arbitrary and seems like a reasonable balance between how many
         // nodes are saved at one time and the size of the hash tables.
         //
         if(total_good % 1000 == 0) {
            stime = msecs();
            //
            // Use the database cache size as a guiding number for the combined size of
            // our hash tables or 256 MB if the former was not set.
            //
            state.swap_out(htab_tstamp - config.visit_timeout * 2, std::max(config.db_cache_size, 256u * 1024u * 1024u));
            ptms.mnt_time += elapsed(stime, msecs());
         }
      }
   }

   /*********************************************/
   /* DONE READING LOG FILES - final processing */
   /*********************************************/
   
   // if there are any unprocessed log files, close them (e.g. Ctrl-C was pressed)
   for(logfile_list_t::iterator i = logfiles.begin(); i != logfiles.end(); i++) {
      if(*i && (*i)->is_open())
         (*i)->close(); 
   }
   logfiles.clear();
   
   // deallocate log records we created earlier
   for(logrec_list_t::iterator i = logrecs.begin(); i != logrecs.end(); i++)
      delete *i;
   logrecs.clear();

   //
   // If Ctrl-C was pressed, stop the DNS resolver without waiting for all 
   // IP addresses being resolved. Unresolved addresses will be saved in the 
   // state database without a country code and will not be retried. 
   //
   if(abort_signal)
      dns_resolver.dns_abort();

   if(total_good)                                                       /* were any good records?   */
   {
      if (state.totals.ht_hits > state.totals.hm_hit) state.totals.hm_hit = state.totals.ht_hits;

      if (lrcnt.total_rec > (lrcnt.total_ignore + lrcnt.total_bad))  /* did we process any?   */
      {
         if(config.is_dns_enabled()) {
            stime = msecs();
            dns_resolver.dns_wait();
            ptms.dns_time += elapsed(stime, msecs());
         }

         //
         // In the non-incremental mode or if processing the last log for
         // the current month, terminate all visits before generating the 
         // report.
         //
         if(!config.incremental || config.last_log) {
            update_visits(tstamp_t());
            state.update_hourly_stats();
         }
         else
            update_visits(state.totals.cur_tstamp);

         update_downloads(state.totals.cur_tstamp);

         if(config.is_dns_enabled())
            process_resolved_hosts();

         // save run data for the report generator
         stime = msecs();
         if (state.save_state()) {
            /* Error: Unable to save current run data */
            fprintf(stderr,"%s\n",config.lang.msg_data_err);
         }
         ptms.mnt_time += elapsed(stime, msecs());

         // do not generate reports or roll over the state database if Ctrl-C was pressed
         if(abort_signal)
            return EXIT_FAILURE;

         // generate intermediate reports if not in batch mode
         if(!config.batch) {
            database_t::status_t status;
            stime = msecs();
            if(!(status = state.database.attach_indexes(true)).success())
               throw exception_t(0, string_t::_format("Cannot create secondary database indexes (%s)", status.err_msg().c_str()));
            write_monthly_report();             /* write monthly HTML file  */
            write_main_index();                 /* write main HTML file     */
            ptms.rpt_time += elapsed(stime, msecs());
         }

         // if it's the last log for the month, roll over the database
         if(config.last_log) {
            stime = msecs();
            state.clear_month();
            ptms.mnt_time += elapsed(stime, msecs());
         }

      }

      /* Whew, all done! Exit with completion status (0) */
      retcode = EXIT_SUCCESS;
   }
   else
   {
      // No valid records found...
      if (config.verbose) 
         printf("%s\n",config.lang.msg_no_vrec);

      //
      // This used to be a failure exit code, but it often interfered with
      // various automation scripts because not having any site traffic is
      // not an error, but it was reported as such based on the exit code. 
      //
      retcode = EXIT_SUCCESS;
   }

   return retcode;
}

///
/// @brief  Compares names of the two search arguments and returns a zero if they 
///         are equal, a negative value if the `e1` name is less than the `e2` name
///         or a positive value if the `e1` name is greater than the `e2` name.
///
int webalizer_t::qs_srcharg_name_cmp(const arginfo_t *e1, const arginfo_t *e2)
{
   if(!e1 && !e2)
      return 0;

   if(!e1 || !e2)
      return e1 ? 1 : -1;

   return strncmp_ex(e1->name(), e1->namelen, e2->name(), e2->namelen);
}

///
/// @brief  Compares the two search arguments and returns a zero if they are equal, 
///         a negative value if `e1` is less than `e2` or a positive value if `e1` 
///         is greater than `e2`.
///
int webalizer_t::qs_srcharg_cmp(const arginfo_t *e1, const arginfo_t *e2)
{
   if(!e1 && !e2)
      return 0;

   if(!e1 || !e2)
      return e1 ? 1 : -1;

   if(!e1->arg && !e2->arg)
      return 0;

   if(!e1->arg || !e2->arg)
      return e1->arg ? 1 : -1;

   return strncmp_ex(e1->arg, e1->arglen, e2->arg, e2->arglen);
}

///
/// @brief  Checks if the specified URL, including its search argments, matches 
///         any pattern in the IgnoreURL list.
///
bool webalizer_t::check_ignore_url_list(const string_t& url, const string_t& srchargs, std::vector<arginfo_t, srch_arg_alloc_t>& sr_args) const
{
   // check the ignore URL filter, which may contain optional search argument names
   const gnode_t *upat = NULL;
   glist::const_iterator upat_it = config.ignored_urls.begin();

   // find the first URL pattern and then loop through duplicates to see if we have a matching search argument name
   while((upat = config.ignored_urls.find_node(url, upat_it, upat != NULL)) != NULL) {
      // if there is no search argument name in this URL pattern, then we found a match
      if(upat->noname || upat->name.isempty()) 
         return true;

      // otherwise search for the pattern name in the sorted search argument vector
      if(!sr_args.empty()) {
         // if the pattern doesn't have a name qualifier, just do a binary search
         if(upat->qualifier.isempty()) {
            // if there's no name qualifier, make a search argument descriptor for a bare name (i.e as if without an equal sign)
            arginfo_t sa_key(upat->name.c_str(), upat->name.length(), upat->name.length());

            // use binary search to check if any of the actual argument names matches
            if(bsearch(&sa_key, &*sr_args.begin(), sr_args.size(), sizeof(arginfo_t), (int (*)(const void*, const void*)) qs_srcharg_name_cmp))
               return true;
         }
         else {
            // otherwise make a search argument descriptor for a name that excludes the equal sign in the pattern name
            arginfo_t sa_key(upat->name.c_str(), upat->name.length() - 1, upat->name.length());

            std::vector<arginfo_t, srch_arg_alloc_t>::const_iterator sr_it;
            
            // cannot use bsearch because we need to traverse all search argument names in sequence
            sr_it = std::lower_bound(sr_args.begin(), sr_args.end(), sa_key, [] (const arginfo_t& e1, const arginfo_t& e2) {return qs_srcharg_cmp(&e1, &e2) < 0;});

            // iterate through all search arguments with the same name and compare values
            while(sr_it != sr_args.end() && !strncmp_ex(sr_it->name(), sr_it->namelen, upat->name, upat->name.length() - 1)) {
               // compare the argument value and the pattern qualifier
               if(!strncmp_ex(sr_it->value(), sr_it->value_length(), upat->qualifier, upat->qualifier.length()))
                  return true;

               sr_it++;
            }
         }
      }
   }

   return false;
}

///
/// @brief  Removes excluded search arguments from the URL and optionally sorts
///         them by name.
///
void webalizer_t::filter_srchargs(string_t& srchargs, std::vector<arginfo_t, srch_arg_alloc_t>& sr_args)
{
   arginfo_t arginfo;
   char *cptr;
   string_t::char_buffer_t sa;

   if(srchargs.isempty())
      return; 

   // there are rarely more than 4-5 search arguments
   sr_args.reserve(8);

   // if no sorting or filtering is requested, return
   if(!config.sort_srch_args && config.incl_srch_args.isempty() && config.excl_srch_args.isempty())
      return;

   // if all are excluded and none included, return
   if(config.incl_srch_args.isempty() && config.excl_srch_args.iswildcard()) {
      srchargs.reset();
      return;
   }

   // detach the storage, so we can manipulate the characters directly
   sa = srchargs.detach();
   cptr = sa.get_buffer();

   // walk search arguments and create descriptors for those that aren't filtered out
   while (*cptr && *cptr != '#') {
      while(*cptr == '&') cptr++;
      arginfo.arg = cptr;
      while(*cptr && *cptr != '=' && *cptr != '&' && *cptr != '#') cptr++;
      arginfo.namelen = cptr - arginfo.arg;
      while(*cptr && *cptr != '&' && *cptr != '#') cptr++;
      arginfo.arglen = cptr - arginfo.arg;

      // 
      // [1] everything is included
      // [2] argument has no name and there's no wildcard exclusion
      // [3] argument has a name and it's either included or not excluded
      //
      if(config.incl_srch_args.iswildcard() ||
         !arginfo.namelen && !config.excl_srch_args.iswildcard() ||
         (arginfo.namelen && 
            (config.incl_srch_args.isinlistex(arginfo.name(), arginfo.namelen, false) || 
            !config.excl_srch_args.isinlistex(arginfo.name(), arginfo.namelen, false)))) {

         if(sr_args.size() >= sr_args.capacity())
            sr_args.reserve(sr_args.capacity() << 1);
         sr_args.push_back(arginfo);
      }
   }

   // if none remaining, return
   if(!sr_args.size()) {
      srchargs.reset();
      return;
   }

   // sort the resulting vector, if requested
   if(config.sort_srch_args && sr_args.size() > 1)
      qsort(&sr_args[0], sr_args.size(), sizeof(arginfo_t), (int (*)(const void*, const void*)) qs_srcharg_name_cmp);

   // get a buffer to copy filtered and possibly sorted search arguments
   string_t::char_buffer_t&& buffer = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;

   // form a new search argument string in the buffer
   cptr = buffer;
   for(size_t index = 0; index < sr_args.size(); index++) {
      if(index > 0)
         *cptr++ = '&';

      // hold onto the new name position in the buffer
      const char *argname = cptr;

      cptr += strncpy_ex(cptr, BUFSIZE - (cptr-buffer), sr_args[index].arg, sr_args[index].arglen);

      // make sure we copied the entire argument
      if(cptr - argname != sr_args[index].arglen)
         throw std::runtime_error("Cannot filter search arguments because the buffer is too small");

      // re-point the argument name within the original character buffer (will contain garbage until memcpy below)
      sr_args[index].arg = sa.get_buffer() + (argname - buffer);
   }

   // copy to the original character buffer (sr_args can be used again after this)
   memcpy(sa, buffer, cptr-buffer);
   sa[cptr-buffer] = 0;

   // attach the memory block to the string
   srchargs.attach(std::move(sa), cptr-buffer);

   // clear the vector if the ignore URL list doesn't have search argument names and make sure it's sorted otherwise
   if(!config.ignored_urls.get_has_names())
      sr_args.clear();
   else {
      // if the vector was not sorted for output, sort it for the ignore filter
      if(!config.sort_srch_args && sr_args.size() > 1)
         qsort(&sr_args[0], sr_args.size(), sizeof(arginfo_t), (int (*)(const void*, const void*)) qs_srcharg_name_cmp);
   }
}

///
/// @brief  Checks if the specified search engine search string contains a domain 
///         name that is not listed in website aliaces and if it does, identifies
///         this string as a spam attempt.
///
bool webalizer_t::check_for_spam_urls(const char *str, size_t slen) const
{
   //
   // Move one character at a time through the search string to see if we have any full 
   // URLs. Note that while we could speed it up by skipping parts of the string we have
   // identified, parsing would have to rely on well-formed URLs and spammers don't exactly 
   // follow the rules (e.g. http://http://spamsite.com). 
   //
   for(const char *cp1 = str; (size_t) (cp1 - str) < slen; cp1++) {
      // check if there's a host name at this offset
      string_t::const_char_buffer_t host = get_url_host(cp1, slen - (cp1 - str));

      if(!host.isempty()) {
         //
         // People often mix up search engine input with the browser's URL input and
         // some search engines generate search results with site's full URL in the 
         // search strings. Check against the site aliases list to avoid flagging 
         // legitimate users as spammers, but if this host is not found in the list, 
         // consider it a spammer.
         //
         if(!config.site_aliases.isinlistex(host, host.capacity(), false, true))
            return true;
      }
   }

   // not a spammer
   return false;
}

///
/// @brief  Extracts and decodes search engine search terms from the referrer URL.
///
bool webalizer_t::srch_string(const string_t& refer, const string_t& srchargs, u_short& termcnt, string_t& srchterms, bool spamcheck)
{
   string_t::char_buffer_t&& buffer = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;
   string_t::char_buffer_t&& termbuf = buffer_holder_t(buffer_allocator, BUFSIZE).buffer;
   const gnode_t *nptr = NULL;
   const char *cp1, *cp3;
   char *cp2, *bptr;
   int  sp_flg = 0;
   bool newsrch = false;
   size_t slen = 0, qlen = 0;
   u_int dblscnt = 0;              // double slash count
   glist::const_iterator iter = config.search_list.begin();

   // reset the search term string and count
   srchterms.clear();
   termcnt = 0;

   // ignore empty referrers and empty search strings
   if(srchargs.isempty() || refer.isempty())
      return false;

   //
   // Check if the referrer domain name matches any search engine in the list 
   // and if it does, find the first non-empty search string name matching the
   // one in the search list entry.
   //
   // Search engines are expected to be grouped by the domain name, so that 
   // once the first one is found, only those following the first match will 
   // be evaluated for performance reasons, to avoid traversing the entire 
   // list every time.
   //
   // $$$ search_list will find the domain if it's mentioned anywhere in the URL !!!
   //
   while((nptr = config.search_list.find_node(refer, iter, (nptr != NULL))) != NULL) {

      // walk the query and look for the name we found for this domain
      cp1 = srchargs;
      do {
         // nptr->name includes the equal sign (e.g. "as_q=")
         cp3 = nptr->name;

         // walk both strings (strncmp would require two passes if not found)
         while(*cp1 && *cp3 && *cp1 == *cp3) cp1++, cp3++;
         if(*cp3 == 0)
            break;

         // move to the next query variable
         while(*cp1 && *cp1++ != '&');
      } while(*cp1);

      if(*cp1 == 0) continue;                         // If not found, continue  

      dblscnt = 0;

      //
      // The entire search string is normalized, so all control characters and reserved 
      // URL characters are URL-encoded, but ASCII characters are not. All characters 
      // are represented by well-formed UTF-8 sequences. We can now safely decode all 
      // URL-encoded sequences and transform some characters in the process, as described 
      // in the comments below. 
      //
      cp2 = buffer;
      while(*cp1 && *cp1!='&')
      {
         if(*cp1 == '%')                              // decode URL-encoded chars
            cp1 = from_hex(++cp1, cp2);
         else if(*cp1 == '+') 
            *cp2 = ' ', cp1++;                        // change + to space       
         else if(*cp1 >= 'A' && *cp1 <= 'Z')     
            *cp2 = string_t::tolower(*cp1), cp1++;    // lowercase ASCII characters
         else
            *cp2 = *cp1, cp1++;                       // copy other characters

         // replace control characters with underscores
         if((unsigned char) *cp2 < '\x20')
            *cp2 = '_';

         // track if we saw at least one double-slash sequence
         if(spamcheck && dblscnt != 2) {
            if(*cp2 == '/') dblscnt++;
            else if(dblscnt) dblscnt = 0;
         }

         if (sp_flg && *cp2==' ') continue;           // compress spaces         
         if(*cp2==' ') sp_flg=1; else sp_flg=0;       // (flag spaces here)      
         cp2++;
      }

      // trim trailing spaces
      cp3 = buffer;                                       
      while(cp2 > cp3 && string_t::isspace(*(cp2-1))) cp2--;
      *cp2 = 0;

      // trim leading spaces
      cp1 = buffer;
      while(*cp1 && string_t::isspace(*cp1)) cp1++;           

      // hold on to the query length
      slen = cp2 - cp1;

      // store in the hash table if not empty
      if(slen) {
         // check for spam URLs, if requested, but only if we saw any double-slash sequences
         if(spamcheck && dblscnt == 2) {
            if(check_for_spam_urls(cp1, slen))
               return true;
         }
                                            
         termcnt++;

         // build the search terms string in the the search term buffer
         cp3 = bptr = termbuf;

         // [9]All Words
         *bptr++ = '[';
         qlen = nptr->qualifier.length();
         bptr += ul2str(qlen, bptr);
         *bptr++ = ']';
         if(qlen) {
            memcpy(bptr, nptr->qualifier.c_str(), qlen);
            bptr += qlen;
         }

         // [17]webalizer windows
         *bptr++ = '[';
         bptr += ul2str(slen, bptr);
         *bptr++ = ']';
         memcpy(bptr, cp1, slen);
         bptr += slen;
         *bptr = 0;

         // [9]All Words[17]webalizer windows
         srchterms.append(cp3, bptr-cp3);
      }
   }

   return false;
}

///
/// This method just ensures that there is no active visit associated with the host 
/// being loaded from the database. Hosts with active visits are loaded when the 
/// state is restored and then maintained in memory, same as new hosts, until their 
/// visits end, at which point they are saved to the database. If we found an active
/// visit in the `put_hnode` call, something went wrong and we cannot proceed.
///
void webalizer_t::unpack_inactive_hnode_cb(hnode_t& hnode, bool active, void *arg)
{
   if(hnode.flag == OBJ_GRP)
      return;

   // make sure there is no active visit for this host
   if(active)
      throw std::runtime_error(string_t::_format("A new host node (ID: %" PRIu64 ") cannot have an active visit", hnode.nodeid));
}

///
/// @brief  Adds or updates a host node in the state database.
///
storable_t<hnode_t> *webalizer_t::put_hnode(
               const string_t& ipaddr,          // IP address
               const tstamp_t& tstamp,          // timestamp 
               int64_t  htab_tstamp,            // serial time stamp
               uint64_t xfer,                   // xfer size 
               bool     fileurl,                // file count
               bool     pageurl,
               bool     spammer,
               bool     robot,
               bool     target,
               bool&    newvisit,               // new visit?
               bool&    newnode,                // new host node?
               bool&    newthost,               // new host today?
               bool&    newspammer              // spammer status changed?
               )
{
   bool found = true;
   uint64_t hashval;
   storable_t<hnode_t> *cptr;
   storable_t<vnode_t> *visit;

   newnode = newvisit = newthost = newspammer = false;

   hashval = hnode_t::hash(ipaddr);

   /* check if hashed */
   if((cptr = state.hm_htab.find_node(hashval, ipaddr, OBJ_REG, htab_tstamp)) == NULL) {
      /* not hashed */
      cptr = new storable_t<hnode_t>(ipaddr);
      if(!state.database.get_hnode_by_value(*cptr, &unpack_inactive_hnode_cb, this)) {
         cptr->nodeid = state.database.get_hnode_id();
         cptr->flag = OBJ_REG;

         cptr->spammer = spammer;
         cptr->robot = robot;

         cptr->set_visit(new storable_t<vnode_t>(cptr->nodeid));
         cptr->visit->hits = 1;
         cptr->visit->files = fileurl ? 1 : 0;
         cptr->visit->pages = pageurl ? 1 : 0;
         cptr->visit->xfer = xfer;
         cptr->visit->robot = robot;
         cptr->visit->start = cptr->visit->end = tstamp;
            
         cptr->visits = 1;
            
         newvisit = true;

         newnode = true;
         newthost = true;
         newspammer = spammer;
            
         found = false;
      }
      state.hm_htab.put_node(hashval, cptr, htab_tstamp);
   }
   
   if(found) {
      if(cptr->visit) {
         // check if the active visit has ended
         if((visit = update_visit(cptr, tstamp)) != NULL) {
            newvisit = true;

            // reuse the visit node to minimize memory allocations
            visit->reset(cptr->nodeid);
            cptr->set_visit(visit);

            cptr->visit->start = tstamp;
            cptr->visit->robot = robot;
            cptr->visits++;
         }
         else {
            if(cptr->visit->storage_info.storage)
               cptr->visit->storage_info.set_modified();
         }
      }
      else {
         // no active visit - create one
         cptr->set_visit(new storable_t<vnode_t>(cptr->nodeid));
         cptr->visit->start = tstamp;
         cptr->visit->robot = robot;
         cptr->visits++;

         newvisit = true;
      }

      cptr->visit->hits++;
      if(fileurl) cptr->visit->files++;
      if(pageurl) cptr->visit->pages++;
      cptr->visit->xfer += xfer;
      cptr->visit->end = tstamp;
      
      // check if the last hit was in the same day
      if(tstamp.compare(cptr->tstamp, tstamp_t::tm_parts::DATE) > 0)
         newthost = true;

      // spam request may be not the first one, so check every time
      if(spammer && !cptr->spammer) {
         cptr->spammer = spammer;
         newspammer = true;
      }

   }

   // update the last hit time stamp
   cptr->tstamp = tstamp;

   // if a non-robot requested a target URL, mark the visit as converted
   if(target && !robot && !spammer && !cptr->visit->converted)
      cptr->visit->converted = true;

   if(cptr->storage_info.storage)
      cptr->storage_info.set_modified();

   return cptr;
}               

///
/// @brief  Adds or updates a host group node in the state database.
///
storable_t<hnode_t> *webalizer_t::put_hnode(
               const string_t& grpname,         // Hostname  
               int64_t     htab_tstamp,
               uint64_t    hits,                  // hit count 
               uint64_t    files,                 // file count
               uint64_t    pages,
               uint64_t    xfer,                  // xfer size 
               uint64_t    visitlen,              // visit length
               bool&     newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<hnode_t> *cptr;

   newnode = false;

   hashval = hnode_t::hash(grpname);

   /* check if hashed */
   if((cptr = state.hm_htab.find_node(hashval, grpname, OBJ_GRP, htab_tstamp)) == NULL) {
      /* not hashed */
      cptr = new storable_t<hnode_t>(grpname);
      if(!state.database.get_hnode_by_value(*cptr, (hnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
         cptr->nodeid = state.database.get_hnode_id();
         cptr->flag  = OBJ_GRP;

         cptr->spammer = false;     // groups are never spammers
         cptr->robot = false;       // or robots

         cptr->count = hits;
         cptr->files = files;
         cptr->pages = pages;
         cptr->xfer  = xfer;

         cptr->visit_avg = (double) visitlen;
         cptr->visit_max = visitlen;
         cptr->visits = 1;

         newnode = true;
         found = false;
      }
      state.hm_htab.put_node(hashval, cptr, htab_tstamp);
   }
   
   if(found) {
      /* found... bump counter */
      cptr->count+=hits;
      cptr->files+=files;
      cptr->pages+=pages;
      cptr->xfer +=xfer;

      cptr->visits++;
      cptr->visit_avg = AVG(cptr->visit_avg, visitlen, cptr->visits);
      cptr->visit_max = std::max(cptr->visit_max, visitlen);
   }

   if(cptr->storage_info.storage)
      cptr->storage_info.set_modified();

   return cptr;
}

///
/// @brief  Adds or updates a referrer node in the state database.
///
rnode_t *webalizer_t::put_rnode(const string_t& str, int64_t htab_tstamp, nodetype_t type, uint64_t count, bool newvisit, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<rnode_t> *nptr;

   newnode = false;

   hashval = rnode_t::hash(str);

   /* check if hashed */
   if((nptr = state.rm_htab.find_node(hashval, str, type, htab_tstamp)) == NULL) {
      /* not hashed */
      nptr = new storable_t<rnode_t>(str);
      if(!state.database.get_rnode_by_value(*nptr, NULL, NULL)) {
         nptr->nodeid = state.database.get_rnode_id();
         nptr->flag  = type;
         nptr->count = count;
            
         if(newvisit)
            nptr->visits++;

         newnode = true;
         found = false;
      }

      state.rm_htab.put_node(hashval, nptr, htab_tstamp);
   }

   if(nptr->storage_info.storage)
      nptr->storage_info.set_modified();

   if(found) {
      /* found... bump counter */
      nptr->count += count;
      
      if(newvisit)
         nptr->visits++;
         
      return nptr;
   }

   return nptr;
}

///
/// @brief  Adds or updates a URL node in the state database.
///
storable_t<unode_t> *webalizer_t::put_unode(const string_t& str, int64_t htab_tstamp, const string_t& srchargs, nodetype_t type, uint64_t xfer, double proctime, u_short port, bool entryurl, bool target, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<unode_t> *cptr;
   unode_t::param_block param;

   newnode = false;

   param.type = type;
   param.url = &str;
   param.srchargs = !srchargs.isempty() ? &srchargs : NULL;

   hashval = unode_t::hash(str, srchargs);

   /* check if hashed */
   if((cptr = state.um_htab.find_node(hashval, &param, type, htab_tstamp)) == NULL) {
      /* not hashed */
      cptr = new storable_t<unode_t>(str, srchargs);
      // check if in the database
      if(!state.database.get_unode_by_value(*cptr)) {
         cptr->nodeid = state.database.get_unode_id();
         cptr->flag = type;
         cptr->count= 1;
         cptr->xfer = xfer;
         cptr->avgtime = cptr->maxtime = proctime;
         cptr->update_url_type(config.get_url_type(port));
         cptr->exit = 0;

         if(target && !cptr->target)
            cptr->target = true;

         if(type == OBJ_GRP) 
            cptr->flag=OBJ_GRP;

         if(entryurl) {
            state.totals.u_entry++;
            state.totals.t_entry++;
            cptr->entry = 1;
         }
            
         newnode = true;
         found = false;
      }
      state.um_htab.put_node(hashval, cptr, htab_tstamp);
   }

   if(found) {
      if(entryurl) {
         if(!cptr->entry)
            state.totals.u_entry++;
         state.totals.t_entry++;
         cptr->entry++;
      }
      cptr->count++;
      cptr->xfer += xfer;
      cptr->update_url_type(config.get_url_type(port));
      cptr->avgtime = AVG(cptr->avgtime, proctime, cptr->count);
      cptr->maxtime = std::max(cptr->maxtime, proctime);
   }

   if(cptr->storage_info.storage)
      cptr->storage_info.set_modified();

   return cptr;
}

///
/// @brief  Adds or updates an HTTP response code node in the state database.
///
rcnode_t *webalizer_t::put_rcnode(const string_t& method, int64_t htab_tstamp, const string_t& url, u_short respcode, bool restore, uint64_t count, bool *newnode)
{
   bool found = true;
   storable_t<rcnode_t> *nptr;
   uint64_t hashval;
   rcnode_t::param_block param;

   if(newnode)
      *newnode = false;
      
   if(method.isempty() || url.isempty())
      return NULL;

   param.respcode = respcode;
   param.url = url;
   param.method = method;

   // respcode, method, url
   hashval = rcnode_t::hash(respcode, method, url);

   /* check if hashed */
   if((nptr = state.rc_htab.find_node(hashval, &param, OBJ_REG, htab_tstamp)) == NULL) {
      /* not hashed */
      nptr = new storable_t<rcnode_t>(method, url, respcode);

      if(!state.database.get_rcnode_by_value(*nptr, NULL, NULL)) {
         nptr->nodeid = state.database.get_rcnode_id();
         nptr->flag = OBJ_REG;
         nptr->count = count;
         if(newnode) *newnode = true;
         found = false;
      }
      state.rc_htab.put_node(hashval, nptr, htab_tstamp);
   }
   
   if(found) {
      /* found... bump counter */
      nptr->count += count;
   }

   if(nptr->storage_info.storage)
      nptr->storage_info.set_modified();

   return nptr;
}

///
/// @brief  Adds or updates a user agent node in the state database.
///
anode_t *webalizer_t::put_anode(const string_t& str, int64_t htab_tstamp, nodetype_t type, uint64_t xfer, bool newvisit, bool robot, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<anode_t> *cptr;

   newnode = false;
      
   hashval = anode_t::hash(str);

   /* check if hashed */
   if((cptr = state.am_htab.find_node(hashval, str, type, htab_tstamp)) == NULL) {
      /* not hashed */
      cptr = new storable_t<anode_t>(str, robot);
      if(!state.database.get_anode_by_value(*cptr)) {
         cptr->nodeid = state.database.get_anode_id();
         cptr->flag = type;
         cptr->count = 1;
         cptr->visits = 1;
         cptr->xfer = xfer;

         if(type==OBJ_GRP) 
            cptr->flag=OBJ_GRP;

         newnode = true;
         found = false;
      }

      state.am_htab.put_node(hashval, cptr, htab_tstamp);
   }

   if(found) {
      /* found... bump counter */
      cptr->count++;
      cptr->xfer += xfer;
      if(newvisit)
         cptr->visits++;
   }

   if(cptr->storage_info.storage)
      cptr->storage_info.set_modified();

   return cptr;
}

///
/// @brief  Adds or updates a search engine search ters node in the state database.
///
snode_t *webalizer_t::put_snode(const string_t& str, int64_t htab_tstamp, u_short termcnt, bool newvisit, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<snode_t> *nptr;

   newnode = false;

   if(str.isempty())     /* skip bad search strs */
      return NULL;

   hashval = snode_t::hash(str);

   /* check if hashed */
   if((nptr = state.sr_htab.find_node(hashval, str, OBJ_REG, htab_tstamp)) == NULL) {
      /* not hashed */
      nptr = new storable_t<snode_t>(str);
      if(!state.database.get_snode_by_value(*nptr)) {
         nptr->nodeid = state.database.get_snode_id();
         nptr->count = 1;
         nptr->termcnt = termcnt;
            
         if(newvisit)
            nptr->visits++;
            
         found = false;
         newnode = true;
      }
      state.sr_htab.put_node(hashval, nptr, htab_tstamp);
   }

   if(found) {
      /* found... bump counter */
      nptr->count++;

      if(newvisit)
         nptr->visits++;
   }

   state.totals.t_srchits++;

   if(nptr->storage_info.storage)
      nptr->storage_info.set_modified();

   return nptr;
}

///
/// @brief  Adds or updates a user node in the state database.
///
inode_t *webalizer_t::put_inode(const string_t& str,   /* ident str */
               int64_t htab_tstamp,
               nodetype_t    type,       /* obj type  */
               bool     fileurl,    /* File flag */
               uint64_t xfer,       /* xfer size */
               const tstamp_t& tstamp, /* timestamp */
               double   proctime,
               bool&    newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<inode_t> *nptr;

   newnode = false;
   
   if(str.isempty()) return NULL;  /* skip if no username */

   hashval = inode_t::hash(str);

   /* check if hashed */
   if((nptr = state.im_htab.find_node(hashval, str, type, htab_tstamp)) == NULL) {
      /* not hashed */
      nptr = new storable_t<inode_t>(str);
      if(!state.database.get_inode_by_value(*nptr)) {
         nptr->nodeid = state.database.get_inode_id();
         nptr->flag  = type;
         nptr->count = 1;
         nptr->files = fileurl ? 1 : 0;
         nptr->xfer  = xfer;
         nptr->tstamp=tstamp;
         nptr->avgtime = nptr->maxtime = proctime;

         /* set object type */
         if (type==OBJ_GRP) 
            nptr->flag=OBJ_GRP;            /* is it a grouping? */

         newnode = true;
         found = false;
      }
      state.im_htab.put_node(hashval, nptr, htab_tstamp);
   }

   if(nptr->storage_info.storage)
      nptr->storage_info.set_modified();
   
   if(found) {
      /* hashed */
      /* found... bump counter */
      nptr->count++;
      if(fileurl) nptr->files++;
      nptr->xfer +=xfer;
      nptr->avgtime = AVG(nptr->avgtime, proctime, nptr->count);
      nptr->maxtime = std::max(nptr->maxtime, proctime);

      if(tstamp.elapsed(nptr->tstamp) >= config.visit_timeout)
         nptr->visit++;
      nptr->tstamp=tstamp;
   }

   return nptr;
}

///
/// @brief  Adds or updates a download node in the state database.
///
dlnode_t *webalizer_t::put_dlnode(const string_t& name, int64_t htab_tstamp, u_int respcode, const tstamp_t& tstamp, uint64_t proctime, uint64_t xfer, storable_t<hnode_t>& hnode, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   storable_t<danode_t> *download;
   storable_t<dlnode_t> *nptr;
   dlnode_t::param_block params;

   newnode = false;

   if(name.isempty() || hnode.string.isempty())
      return NULL;

   if(respcode != RC_OK && respcode != RC_PARTIALCONTENT)
      return NULL;

   params.name = name;
   params.ipaddr = hnode.string;

   hashval = dlnode_t::hash(hnode.string, name);

   //
   // Sometimes download requests may come in severely shuffled. For example, 
   // all these requests came from the same IP address. The first one failed
   // (sc-bytes is zero), the other two retrieved the file, but the initial 
   // request (200) took a bit longer and was logged after the second one (206)
   // was finished. 
   //
   // time cs-method cs-uri-stem cs(User-Agent) sc-status sc-bytes cs-bytes time-taken 
   // 12:29:09 GET /.../webalizer_win.zip Mozilla/5.0+(...)+Firefox/1.5.0.1 200 0      710 15234
   // 12:36:43 GET /.../webalizer_win.zip Download+Master                   206 530125 360 436546
   // 12:36:48 GET /.../webalizer_win.zip Download+Master                   200 524613 338 448765
   //
   if((nptr = state.dl_htab.find_node(hashval, &params, OBJ_REG, htab_tstamp)) == NULL) {
      nptr = new storable_t<dlnode_t>(name, &hnode);
      if(!state.database.get_dlnode_by_value(*nptr, &state_t::unpack_dlnode_cached_host_cb, &state, (const storable_t<hnode_t>&) hnode)) {
         nptr->set_host(&hnode);

         nptr->nodeid = state.database.get_dlnode_id();
         nptr->download = new storable_t<danode_t>(nptr->nodeid);

         nptr->download->hits = 1;
         nptr->download->tstamp = tstamp;
         nptr->download->xfer = xfer;
         nptr->download->proctime = proctime;

         newnode = true;
         found = false;
      }

      state.dl_htab.put_node(hashval, nptr, htab_tstamp);
   }
   
   if(found) {
      if(nptr->download) {
         if((download = update_download(nptr, tstamp)) != NULL) {
            download->reset(nptr->nodeid);
            nptr->download = download;
         }
         else {
            if(nptr->download->storage_info.storage)
               nptr->download->storage_info.set_modified();
         }
      }
      else
         nptr->download = new storable_t<danode_t>(nptr->nodeid);

      nptr->download->hits++;
      nptr->download->tstamp = tstamp;
      nptr->download->xfer += xfer;
      nptr->download->proctime += proctime;
   }

   if(nptr->storage_info.storage)
      nptr->storage_info.set_modified();

   return nptr;
}

///
/// @brief  Checks if the visit in the specified host has ended and if it has, 
///         detaches it from the host and factors visit data into host and total 
///         counters.
///
/// The time stamp is either the current log time or a null time stamp if the
/// current month is being ended. 
///
/// The caller is expected to dispose of the returned visit node.
///
storable_t<vnode_t> *webalizer_t::update_visit(storable_t<hnode_t> *hptr, const tstamp_t& tstamp)
{
   storable_t<vnode_t> *visit;

   if(!hptr || hptr->flag == OBJ_GRP || !hptr->visits)
      return NULL;

   if((visit = hptr->visit) == NULL)
      return NULL;

   // end the visit if tstamp is zero (e.g. end of month)
   if(!tstamp.null) {
      // or the time since the last request exceeds the visit timeout
      if(tstamp.elapsed(visit->end) < config.visit_timeout) {
         // or visit length exceeds the allowed maximum (e.g. site pinging)
         if(!config.max_visit_length || visit->end.elapsed(visit->start) < config.max_visit_length)
            return NULL;
      }
   }

   return end_visit(hptr);
}

///
/// @brief  Ends the visit within the host, if one exists and returns the visit
///         node, so it can be reused ot deleted.
///
/// This method does not check whether the last visit request is outside of the
/// visit timeout window and should be called only when it is known that the visit
/// can be ended safely. 
///
storable_t<vnode_t> *webalizer_t::end_visit(storable_t<hnode_t> *hptr)
{
   storable_t<vnode_t> *visit;
   uint64_t vlen;

   if((visit = hptr->visit) == nullptr)
      return nullptr;

   // detach the visit from the host
   hptr->set_visit(NULL);

   // indicate that the host node has been updated
   if(hptr->storage_info.storage)
      hptr->storage_info.set_modified();

   // update host counters
   hptr->count += visit->hits;
   hptr->files += visit->files;
   hptr->pages += visit->pages;
   hptr->xfer += visit->xfer;

   // get visit length
   vlen = (uint64_t) visit->end.elapsed(visit->start);

   // update maximum and average visit duration for this host
   hptr->visit_avg = AVG(hptr->visit_avg, vlen, hptr->visits);
   hptr->visit_max = std::max(hptr->visit_max, vlen);

   // update maximum hits, pages, files and transfer
   hptr->max_v_hits = std::max(visit->hits, hptr->max_v_hits);
   hptr->max_v_files = std::max(visit->files, hptr->max_v_files);
   hptr->max_v_pages = std::max(visit->pages, hptr->max_v_pages);
   hptr->max_v_xfer = std::max(visit->xfer, hptr->max_v_xfer);

   // update total maximum hits, pages, files and transfer
   state.totals.max_v_hits = std::max(hptr->max_v_hits, state.totals.max_v_hits);
   state.totals.max_v_files = std::max(hptr->max_v_files, state.totals.max_v_files);
   state.totals.max_v_pages = std::max(hptr->max_v_pages, state.totals.max_v_pages);
   state.totals.max_v_xfer = std::max(hptr->max_v_xfer, state.totals.max_v_xfer);

   // update ended visits counters
   state.totals.t_visits_end++;

   // update the counters for each visitor type (robots, spammers and humans)
   if(hptr->spammer) {
      visit->converted = false;              // clear the converted flag

      state.totals.t_svisits_end++;
      
      //
      // Spammer totals can only be updated at the end of a visit to make
      // sure that spam visits that don't start with a spamming request 
      // are still counted as spammers.
      //
      
      // if it's the first visit, increment spammer host count
      if(hptr->visits == 1)
         state.totals.t_shosts++;

      state.totals.t_spmhits += visit->hits;
      state.totals.t_sfiles += visit->files;
      state.totals.t_spages += visit->pages;
      state.totals.t_sxfer += visit->xfer;
   }
   else if(hptr->robot) {
      visit->converted = false;              // clear the converted flag
      state.totals.t_rvisits_end++;
   }
   else { 
      // update ended human visits
      state.totals.t_hvisits_end++;

      // update total maximum hits, pages, files and transfer
      state.totals.max_hv_hits = std::max(hptr->max_v_hits, state.totals.max_hv_hits);
      state.totals.max_hv_files = std::max(hptr->max_v_files, state.totals.max_hv_files);
      state.totals.max_hv_pages = std::max(hptr->max_v_pages, state.totals.max_hv_pages);
      state.totals.max_hv_xfer = std::max(hptr->max_v_xfer, state.totals.max_hv_xfer);
   
      // update total maximum and average visit durations
      state.totals.t_visit_avg = AVG(state.totals.t_visit_avg, vlen, state.totals.t_hvisits_end);
      state.totals.t_visit_max = std::max(hptr->visit_max, state.totals.t_visit_max);

      // update counters for converted visits
      if(visit->converted) {
         // if it's the first converted visit, increment converted host count
         if(!hptr->visits_conv)
            state.totals.t_hosts_conv++;            // total converted hosts
         
         state.totals.t_visits_conv++;              // total converted visits
         hptr->visits_conv++;                // and for this host

         state.totals.t_vconv_avg = AVG(state.totals.t_vconv_avg, vlen, state.totals.t_visits_conv);
         state.totals.t_vconv_max = std::max(hptr->visit_max, state.totals.t_vconv_max);
      }
   }

   // update the last URL, if any
   if(visit->lasturl) {
      // update exit URL for non-robots
      if(!hptr->robot) {
         if(!visit->lasturl->exit)
            state.totals.u_exit++;                  // unique exit URLs
         state.totals.t_exit++;                     // total exit URLs

         visit->lasturl->exit++;
         if(visit->lasturl->storage_info.storage)
            visit->lasturl->storage_info.set_modified();
      }
      visit->set_lasturl(NULL);
   }

   // if the visit node was read from the database, queue it for deletion
   if(visit->storage_info.storage) {
      state.v_ended.push_back(visit->nodeid);
      visit->storage_info.set_deleted();
   }

   // if DNS resolution is disabled or if the host name has been resolved, update host groups now
   if(!config.is_dns_enabled() || hptr->resolved)
      group_host_by_name(*hptr, *visit);
   else {
      // otherwise, queue a copy of the visit for grouping when the host name is available
      hptr->add_grp_visit(new storable_t<vnode_t>(std::move(*visit)));
   }

   return visit;
}

///
/// @brief  A callback method that ends a visit and factors its data into the
///         host.
///
storable_t<vnode_t> *webalizer_t::end_visit_cb(storable_t<hnode_t> *hnode, void *arg)
{
   return ((webalizer_t*) arg)->end_visit(hnode);
}

///
/// @brief  Checks all hosts for active visits and ends those that exceed the
///         maximum visit time or all if we are ending a month.
///
/// Deletes all returned ended visits, as we have no use for them at this point.
///
void webalizer_t::update_visits(const tstamp_t& tstamp)
{
   vnode_t *visit;
   hash_table<storable_t<hnode_t>>::iterator h_iter = state.hm_htab.begin();
   
   while(h_iter.next()) {
      if((visit = update_visit(h_iter.item(), tstamp)) != NULL)
         delete visit;
   }
}

///
/// @brief  Checks if the active download in the specified has ended and if it has, 
///         detaches it from the download node and factors download data into download 
///         and total counters.
///
storable_t<danode_t> *webalizer_t::update_download(storable_t<dlnode_t> *dlnode, const tstamp_t& tstamp)
{
   storable_t<danode_t> *download;

   if(!dlnode || dlnode->flag == OBJ_GRP || tstamp.null)
      return NULL;

   if((download = dlnode->download) == NULL)
      return NULL;

   // the elapsed time is always positive, so usual arithmetic conversions work
   if(tstamp.elapsed(download->tstamp) < config.download_timeout)
      return NULL;

   return end_download(dlnode);
}

///
/// @brief  Ends the active download within the download job, if one exists,
///         and returns the active download node, so it can be reused ot
///         deleted.
///
/// This method does not check whether the last download request is outside of
/// the download timeout window and should be called only when it is known that
/// the download can be ended safely.
///
storable_t<danode_t> *webalizer_t::end_download(storable_t<dlnode_t> *dlnode)
{
   storable_t<danode_t> *download;
   double value;

   if((download = dlnode->download) == nullptr)
      return nullptr;

   dlnode->download = NULL;
   if(dlnode->storage_info.storage)
      dlnode->storage_info.set_modified();

   dlnode->count++;
   dlnode->sumhits += download->hits;

   dlnode->sumxfer += download->xfer;
   dlnode->avgxfer = AVG(dlnode->avgxfer, download->xfer, dlnode->count);

   value = (double) download->proctime/60000.;
   dlnode->sumtime += value;
   dlnode->avgtime = AVG(dlnode->avgtime, value, dlnode->count);

   state.totals.t_dlcount++;

   // if the download node was read from the database, queue it for deletion
   if(download->storage_info.storage) {
      state.dl_ended.push_back(download->nodeid);
      download->storage_info.set_deleted();
   }

   return download;
}

///
/// @brief  A callback method that ends a download and factors its data into the
///         download job.
///
storable_t<danode_t> *webalizer_t::end_download_cb(storable_t<dlnode_t> *dlnode, void *arg)
{
   return ((webalizer_t*) arg)->end_download(dlnode);
}

///
/// @brief  Checks all download nodes and ends all active downloads that exceed the 
///         maximum download time.
///
/// Deletes all returned ended downloads, as we have no use for them at this point. 
///
void webalizer_t::update_downloads(const tstamp_t& tstamp)
{
   storable_t<danode_t> *download;
   storable_t<dlnode_t> *nptr;
   dl_hash_table::iterator iter = state.dl_htab.begin();

   while((nptr = iter.next()) != NULL) {
      if((download = update_download(nptr, tstamp)) != NULL)
         delete download;
   }
}

///
/// @brief  Application entry point.
///
int main(int argc, char *argv[])
{
   config_t config;
   int retcode = EXIT_SUCCESS;

   set_os_ex_translator();

   try {
      // read configuration files
      config.initialize(get_cur_path(), argc, argv);
      
      webalizer_t logproc(config);

      //
      // Route commands that don't require webalizer_t initialized right away. Note
      // that these commands exited with code one historically, which indicated that 
      // no log processing was done. However, the failure exit code breaks scripts
      // that check for failures (e.g. printing a version after a build) and was 
      // changed to return success in v4.4.0.
      //

      // check if version is requested
      if(config.print_version) {
         logproc.print_version();
         return EXIT_SUCCESS;
      }

      // check if warranty is requested
      if(config.print_warranty) {
         logproc.print_warranty();
         return EXIT_SUCCESS;
      }

      // check if help is requested
      if(config.print_options) {
         logproc.print_options(argv[0]);
         return EXIT_SUCCESS;
      }

      // be polite and announce yourself...
      logproc.print_intro();

      if(config.is_bad()) {
         config.report_errors();
         return EXIT_FAILURE;
      }

      // print current configuration
      logproc.print_config();

      try {
         // set the Ctrl-C handler
         set_ctrl_c_handler(webalizer_t::ctrl_c_handler);

         // initialize the log processor
         logproc.initialize();

         // run the log processor 
         retcode = logproc.run();

         // clean up
         logproc.cleanup();

         // remove the Ctrl-C handler
         reset_ctrl_c_handler();

         return retcode;
      }
      catch(const os_ex_t& err) {
         fprintf(stderr, "%s\n", err.desc().c_str());
       }
      catch (const DbException &err) {
         fprintf(stderr, "[%d] %s\n", err.get_errno(), err.what());
      }
      catch (const exception_t &err) {
         fprintf(stderr, "%s\n", err.desc().c_str());
      }
      catch (const std::exception &err) {
         fprintf(stderr, "%s\n", err.what());
      }

      //
      // If an exception was caught, try to clean up the log processor. If
      // any exception is thrown at this point, report it and just exit.
      //

      logproc.cleanup();
   }
   catch(const os_ex_t& err) {
      fprintf(stderr, "%s\n", err.desc().c_str());
   }
   catch (const DbException &err) {
      fprintf(stderr, "[%d] %s\n", err.get_errno(), err.what());
   }
   catch (const exception_t &err) {
      fprintf(stderr, "%s\n", err.desc().c_str());
   }
   catch (const std::exception &err) {
      fprintf(stderr, "%s\n", err.what());
   }

   return EXIT_FAILURE;
}

///
/// @brief  Ctrl-C handler callback function.
///
void webalizer_t::ctrl_c_handler(void)
{
   abort_signal = true;
}

///
/// @brief  Reads the next log record into the supplied buffer and updates log 
///         record counts.
///
int webalizer_t::read_log_line(string_t::char_buffer_t& buffer, logfile_t& logfile, logrec_counts_t& lrcnt)
{
   int reclen = 0, errnum = 0;

   // read the line ad check if there's no more data; EOF is checked in logfile_t::get_line
   while((reclen = logfile.get_line(buffer, (u_int) buffer.capacity(), &errnum)) != 0) {
      
      // in case of an error, throw an exception - errors cannot be just reported as bad log lines
      if(reclen == -1)
         throw exception_t(0, string_t::_format("%s: %s (%d)", config.lang.msg_file_err, logfile.get_fname().c_str(), errnum));
         
      // live IIS log files are zero-padded to a 64K boundary
      if(config.log_type == LOG_IIS && *buffer == 0)
         return 0;

      lrcnt.total_rec++;
      
      // if the buffer is not full, return
      if((size_t) reclen < buffer.capacity()-1 || buffer[buffer.capacity()-1] == '\n')
         return reclen;
      
      lrcnt.total_bad++;              /* bump bad record counter      */

      // full buffer - report record number, file name and the record if running in debug mode
      if (config.verbose) {
         fprintf(stderr,"%s (%" PRIu64 " - %s)",config.lang.msg_big_rec, lrcnt.total_rec, logfile.get_fname().c_str());
         if (config.debug_mode) 
            fprintf(stderr,":\n%s",buffer.get_buffer());
         else 
            fprintf(stderr,"\n");
      }

      /* get the rest of the record */
      while((reclen = logfile.get_line(buffer, (u_int) buffer.capacity(), &errnum)) != 0) {
         // handle errors
         if(reclen == -1)
            throw exception_t(0, string_t::_format("%s: %s (%d)", config.lang.msg_file_err, logfile.get_fname().c_str(), errnum));

         // print this buffer
         if (config.debug_mode && config.verbose) 
            fprintf(stderr,"%s",buffer.get_buffer());

         // if at the end of the oversized record, print EOL and move on to the next line
         if((size_t) reclen < buffer.capacity()-1) {
            if (config.debug_mode && config.verbose)
               fprintf(stderr,"\n");
            break;
         }
      }

      // check if this was the last line in the file
      if(reclen == 0)
         break;
   }
   
   return reclen;
}

///
/// @brief  Parses the log record text in the buffer and, if it's valid, populates
///         the log record structure.
///
int webalizer_t::parse_log_record(string_t::char_buffer_t& buffer, size_t reclen, log_struct& logrec, u_int fileid, uint64_t recnum)
{
   int parse_code;
   string_t lrecstr;

   //
   // parser_t::parse_record modifies the buffer, so we need to save the original
   // record in case we need to report an error. This is expensive - do it only
   // in debug mode.
   //
   if(config.debug_mode)
      lrecstr = buffer;

   if((parse_code = parser.parse_record(buffer, reclen, logrec)) == PARSE_CODE_ERROR) {

      /* really bad record... */
      if (config.verbose)
      {
         fprintf(stderr,"%s (%u:%" PRIu64 ")", config.lang.msg_bad_rec, fileid, recnum);
         if (config.debug_mode) fprintf(stderr,":\n%s\n", lrecstr.c_str());
         else fprintf(stderr,"\n");
      }
   }
   
   return parse_code;
}

