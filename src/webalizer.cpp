/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   webalizer.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include "platform/sys/utsname.h"
#include <psapi.h>
#else
#include <signal.h>
#include <unistd.h>                           /* normal stuff             */
#include <sys/utsname.h>
#endif

/* ensure sys/types */
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#endif 

#include "webalizer.h"                         /* main header              */
#include "output.h"
#include "parser.h"
#include "preserve.h"
#include "hashtab.h"
#include "linklist.h"
#include "lang.h"
#include "dns_resolv.h"
#include "util.h"
#include "tstring.h"
#include "thread.h"
#include "exception.h"
#include "dump_output.h"
#include "html_output.h"
#include "xml_output.h"
#include "autoptr.h"

//
//
//
#define GZ_BUFSIZE BUFSIZE                    /* our_getfs buffer size    */

/* internal function prototypes */

#ifdef _WIN32
static BOOL WINAPI console_ctrl_handler(DWORD type);
void __cdecl _win32_se_handler(unsigned int excode, _EXCEPTION_POINTERS *exinfo);
void print_loaded_modules(void);
#else
static void console_ctrl_handler(int sig);
#endif

/*********************************************/
/* GLOBAL VARIABLES                          */
/*********************************************/

const char *moddate     = __DATE__;										      /* modification date        */
const char *copyright   = "Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)";

static bool abort_signal = false;   // true if Ctrl-C was pressed

int     verbose      = 2;                     /* 2=verbose,1=err, 0=none  */ 
int     debug_mode   = 0;                     /* debug mode flag          */

//
//
//
webalizer_t::webalizer_t(void) : config(_config), parser(config), state(config), dns_resolver(config)
{
   check_dup = false;                         /* check for dup flag       */

   total_rec   =0;                            /* Total Records Processed     */
   total_ignore=0;                            /* Total Records Ignored       */
   total_bad   =0;                            /* Total Bad Records           */

   start_ts = 0;                              /* program timers              */
   end_ts = 0;

   dns_time = 0;
   mnt_time = 0;
   rpt_time = 0;

   f_buf = new char[GZ_BUFSIZE];              /* our_getfs buffer         */
   f_cp = f_buf+GZ_BUFSIZE;                   /* pointer into the buffer  */
   f_end = 0;

   buffer = new char[BUFSIZE];
}

webalizer_t::~webalizer_t(void)
{
   delete [] f_buf;
   delete [] buffer;
}

bool webalizer_t::initialize(int argc, const char * const argv[])
{
   u_int i;
   utsname system_info;

   //
   // Read configuration files
   //
   _config.initialize(get_cur_path(), argc, argv);

   // check if version is requested
   if(config.print_version) {
      print_version();
      exit(1);
   }

   // be polite and announce yourself...
   if(verbose > 1) {
      uname(&system_info);
      printf("\nStone Steps Webalizer v%s (%s %s)\n\n", state_t::get_app_version().c_str(), system_info.sysname, system_info.release);
   }

   // check if help is requested
   if(config.print_options) {
      print_options(argv[0]);
      exit(1);
   }

   // check if warranty is requested
   if(config.print_warranty) {
      print_warranty();
      exit(1);
   }

   // report procesed language file
   if(verbose > 1)
      config.lang.report_lang_fname();

   // report processed configuration files
   if(verbose > 1)
      config.report_config();

   // check if the output directory has write access
   if(access(config.out_dir, R_OK | W_OK)) {
      /* Error: Can't change directory to ... */
      if(verbose)
         fprintf(stderr, "%s %s\n", config.lang.msg_dir_err,config.out_dir.c_str());
      exit(1);
   }

   // check if the database directory has write access
   if(access(config.db_path, R_OK | W_OK)) {
      /* Error: Can't change directory to ... */
      if(verbose)
         fprintf(stderr, "%s %s\n", config.lang.msg_dir_err, config.db_path.c_str());
      exit(1);
   }

   // initialize components we need for log file processing
   if(!config.is_maintenance()) {
      // initialize DNS resolver if the number of workers is set to a non-zero value
      if(config.is_dns_enabled()) {
         if(dns_resolver.dns_init() == false) {
            if(verbose)
               printf("%s\n", lang_t::msg_dns_init);
         }
      }

      // initialize the log file parser
      if(!parser.init_parser(config.log_type)) {
         if(verbose)
            fprintf(stderr, "%s\n", config.lang.msg_pars_err);
         exit(1);
      }
   }

   //
   // Initialize report generator
   //
   if(!config.compact_db && !config.end_month && !config.db_info) {
      if(!init_output_engines()) {
         if(verbose)
            fprintf(stderr, "Cannot initialize output engine\n");
         exit(1);
      }
   }

   //
   // Initialize the state engine
   //
   if(!state.initialize()) {
      if(verbose)
         fprintf(stderr, "Cannot initialize the state engine\n");
      exit(1);
   }

   //
   // restore state, if required
   //
   if(config.prep_report || config.end_month || config.incremental || config.db_info) {
      if((i=state.restore_state()) != 0) {
         /* Error: Unable to restore run data (error num) */
         throw exception_t(0, string_t::_format("%s (%d)\n", config.lang.msg_bad_data, i));
      }

      // enable duplicate time stamp checking if in the incremental mode
      if(config.incremental)
         check_dup = true;
   }

   if(!config.db_info) {
      /* Creating output in ... */
      if(verbose > 1)
         printf("%s %s\n", config.lang.msg_dir_use, !config.out_dir.isempty() ? config.out_dir.c_str() : config.lang.msg_cur_dir);

      /* Hostname for reports is ... */
      if(verbose > 1 && !config.db_info) 
         printf("%s '%s'\n",config.lang.msg_hostname,config.hname.c_str());
   }

   return true;
}

bool webalizer_t::cleanup(void)
{
   if(config.is_dns_enabled())
      dns_resolver.dns_clean_up();

   parser.cleanup_parser();
   
   cleanup_output_engines();

   state.cleanup();

   return true;
}

bool webalizer_t::init_output_engines(void)
{
   output_t::graphinfo_t *graphinfo = NULL;
   bool makeimgs = false;
   output_t *optr;
   nlist::const_iterator iter = config.output_formats.begin();
   
   while(iter.next()) {
      // allocate an output engine of the requested type
      if(iter.item()->string == "html") 
         optr = new html_output_t(config, state);
      else if(iter.item()->string == "tsv") 
         optr = new dump_output_t(config, state);
      else if(iter.item()->string == "xml") 
         optr = new xml_output_t(config, state);
      else {
         fprintf(stderr, "Unrecognized output format (%s)\n", iter.item()->string.c_str());
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
         delete optr;
         if(verbose)
            fprintf(stderr, "Cannot initialize output engine (%s)\n", iter.item()->string.c_str());
         continue;
      }
      
      output.push(optr);
   }
   
   if(!output.size())
      fprintf(stderr, "At least one output format must be specified\n");
   
   return output.size() != 0;
}

void webalizer_t::cleanup_output_engines(void)
{
   output_t *optr;
   vector_t<output_t*>::iterator iter = output.begin();
   
   while(iter.next(NULL)) {
      optr = iter.item();
      optr->cleanup_output_engine();
      delete optr;
   }
   output.clear();
}

void webalizer_t::write_main_index(void)
{
   output_t *optr;
   vector_t<output_t*>::iterator iter = output.begin();
   
   while(iter.next(NULL)) {
      optr = iter.item();
      
      if(optr->is_main_index()) {
         if (verbose>1) 
            printf("%s (%s)\n", config.lang.msg_gen_sum, optr->get_output_type());
         
         optr->write_main_index();
      }
   }
}

void webalizer_t::write_monthly_report(void)
{
   output_t *optr;
   vector_t<output_t*>::iterator iter = output.begin();
   
   while(iter.next(NULL)) {
      optr = iter.item();
      
      if(verbose > 1)
         printf("%s %s %d (%s)\n", config.lang.msg_gen_rpt, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year, optr->get_output_type());
      
      optr->write_monthly_report();
   }
}

/*********************************************/
/* PRINT_OPTS - print command line options   */
/*********************************************/

void webalizer_t::print_options(const char *pname)
{
   int i;

   printf("%s: %s %s\n", lang_t::h_usage1, pname, lang_t::h_usage2);
   for (i=0; lang_t::h_msg[i]; i++) printf("%s\n", lang_t::h_msg[i]);
}

/*********************************************/
/* PRINT_WARRANTY                            */
/*********************************************/

void webalizer_t::print_warranty(void)
{
	printf("THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL,\n"
			 "BUT WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED WARRANTY OF\n"
			 "MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. SEE THE\n"
			 "GNU GENERAL PUBLIC LICENSE V2 FOR MORE DETAILS.\n");
}

/*********************************************/
/* PRINT_VERSION                             */
/*********************************************/

void webalizer_t::print_version()
{
   utsname system_info;

   if(verbose == 0) {
      printf("%s\n", state_t::get_app_version().c_str());
      return;
   }

   uname(&system_info);
   printf("\nStone Steps Webalizer v%s (%s %s) %s\n%s\n",   
               state_t::get_app_version().c_str(),
               system_info.sysname,system_info.release,
               config.lang.language,copyright);
   printf("\nThis program is based on The Webalizer v2.01-10\nCopyright 1997-2001 by Bradford L. Barrett (www.webalizer.com)\n\n");

   if (debug_mode) {
      printf("Mod date: %s\n", moddate);
      printf("\nDefault config dir: %s\n\n", ETCDIR);
   }
}

//
// group_host_by_name
//
//
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
	if(config.group_sites.size() && ((hostname && (group = config.group_sites.isinglist(*hostname)) != NULL) ||
                                                ((group = config.group_sites.isinglist(hnode.string)) != NULL))) {
		if(!put_hnode(*group, vnode.hits, vnode.files, vnode.pages, vnode.xfer, vlen, newhgrp)) {
			if (verbose)
				/* Error adding Site node, skipping ... */
				fprintf(stderr,"%s %s\n", config.lang.msg_nomem_mh, group->c_str());
		}
	}
	else
	{
		/* Domain Grouping */
		if (config.group_domains)
		{
			const char *domain = get_domain((hostname) ? *hostname : hnode.string, config.group_domains);
			if (domain) {
				if(!put_hnode(string_t::hold(domain), vnode.hits, vnode.files, vnode.pages, vnode.xfer, vlen, newhgrp)) {
					if (verbose)
						/* Error adding Site node, skipping ... */
						fprintf(stderr,"%s %s\n", config.lang.msg_nomem_mh, domain);
				}
			}
		}
	}
	
	// update the host group count
	if(newhgrp)
	   state.totals.t_grp_hosts++;

   //
   // Update country counters, ignoring robot and spammer activity
   //
   if(!hnode.robot && !hnode.spammer) {
      ccptr = &state.cc_htab.get_ccnode(hnode.get_ccode());

      ccptr->count += vnode.hits;
      ccptr->files += vnode.files;
      ccptr->xfer += vnode.xfer;
      ccptr->visits++;
   }
}

//
// group_hosts_by_name should be called only when DNS resolution is enabled
//
void webalizer_t::group_hosts_by_name(void)
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

void webalizer_t::proc_index_alias(string_t& url)
{
   const char *cp1;
   const nnode_t *lptr;
   list_t<nnode_t>::iterator iter = config.index_alias.begin();

   while((lptr = iter.next()) != NULL) {
      if ((cp1 = strstr(url, lptr->string)) != NULL) {
         if(cp1 == url.c_str() || *(cp1-1) == '/') {
            url.truncate(cp1-url.c_str());
            if(url.isempty())
               url = "/"; 
            break;
         }
      }
      lptr=lptr->next;
   }
}

void webalizer_t::mangle_user_agent(string_t& agent)
{
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

void webalizer_t::filter_user_agent(string_t& agent, const string_t *ragent)
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
   
   ua_token_t token;
   const char *delims = str_delims;             // active delimiters
   char *cp1, *cp2;
   string_t::char_buffer_t ua;
   size_t ualen, arglen, t_arglen = 0;
   const string_t *str;
   bool skiptok = false;
   
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
            if(!strncasecmp(++cp2, "ttp://", 6)) {
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
                     ua_groups.push(ua_args.size());
                  
                     // re-write the token
                     token.start = str->c_str();
                     token.arglen = str->length();
                     
                     // and update the total length (arg's output delimiters still included)
                     t_arglen += token.arglen;
                     t_arglen -= arglen;
                  }
               }
            }
            
            if(!skiptok)
               ua_args.push(token);             // keep the token
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

      // allocate a new memory block only if the new string is greater
      if(t_arglen > ualen)
         ua.resize(t_arglen+1);
      
      // form a new user agent string
      cp1 = ua;
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
         memcpy(cp1, ua_args[i].start, arglen);
         cp1 += arglen;
      }
      *cp1 = 0;
   }

   // clear both vectors
   ua_args.clear();
   ua_groups.clear();
   
   if(ua)
      agent.attach(ua, cp1-ua.get_buffer());    // attach the new string
   else
      agent.reset();                            // reset to an empty agent string
}

//
// --end-month option handler
//
// When called, no record processing is being done - just end active visits and 
// downloads and then save the state.
//
int webalizer_t::end_month(void)
{
   uint64_t stime = msecs();
   
   update_visits(tstamp_t());
   update_downloads(tstamp_t());
   
   state.update_hourly_stats();
   
   if (state.save_state()) {
      /* Error: Unable to save current run data */
      if (verbose) 
         fprintf(stderr,"%s\n",config.lang.msg_data_err);
      return 1;
   }
   
   state.clear_month();
   
   rpt_time += elapsed(stime, msecs());

   return 0;
}

int webalizer_t::database_info(void)
{
   uint64_t stime = msecs();

   state.database_info();

   mnt_time += elapsed(stime, msecs());

   return 0;
}

int webalizer_t::prep_report(void)
{
   uint64_t stime = msecs();

   write_monthly_report();
   write_main_index();

   rpt_time += elapsed(stime, msecs());

   return 0;
}

int webalizer_t::compact_database(void)
{
   u_int bytes;
   int error;

   uint64_t stime = msecs();
   
   if((error = state.database.compact(bytes)) == 0) {
      if(verbose > 1)
         printf("%s: %d KB\n", config.lang.msg_cmpctdb, bytes/1024);
   }
   else
      fprintf(stderr, "Cannot compact the database (%d)\n", error);
   
   mnt_time += elapsed(stime, msecs());
      
   return 0;
}

int webalizer_t::run(void)
{
   uint64_t tot_time, proc_time;                    /* temporary time storage      */
   u_int rps;
   int retcode;

   start_ts = msecs();

   if(config.prep_report)
      retcode = prep_report();
   else if(config.compact_db)
      retcode = compact_database();
   else if(config.end_month)
      retcode = end_month();
   else if(config.db_info)
      retcode = database_info();
   else
      retcode = proc_logfile();

   end_ts = msecs();              
   
   /* display timing totals?   */
   if (config.time_me || verbose > 1) {
      /* get total (end-start) and processing time */
      tot_time = elapsed(start_ts, end_ts);
      proc_time = tot_time-dns_time-mnt_time-rpt_time;

      if(!config.prep_report && !config.compact_db && !config.end_month && !config.db_info) {
         // output number of processed, ignored and bad records
         printf("%s %" PRIu64 " %s ", config.lang.msg_processed, total_rec, config.lang.msg_records);
         if (total_ignore) {
            printf("(%" PRIu64 " %s",total_ignore, config.lang.msg_ignored);
            if (total_bad) 
               printf(", %" PRIu64 " %s) ",total_bad, config.lang.msg_bad);
            else
               printf(") ");
         }
         else if (total_bad) 
            printf("(%" PRIu64 " %s) ",total_bad, config.lang.msg_bad);

         // output processing time
         printf("%s %.2f %s", config.lang.msg_in, proc_time/1000., config.lang.msg_seconds);

         /* calculate records per second */
         if (tot_time)
            rps = ((u_int)((double)total_rec/(proc_time/1000.)));
         else 
            rps = 0;

         if(rps > 0 && rps <= total_rec)
            printf(", %d/sec\n", rps);
         else  
            printf("\n");

         if(verbose && config.is_dns_enabled()) {
            if(dns_resolver.dns_cached || dns_resolver.dns_resolved)
               printf("%s: %" PRIu64 "%% (%" PRIu64 ":%" PRIu64 ")\n", lang_t::msg_dns_htrt, (uint64_t) (dns_resolver.dns_cached * 100. / (dns_resolver.dns_cached + dns_resolver.dns_resolved)), dns_resolver.dns_cached, dns_resolver.dns_resolved);
         }

         // report total DNS time
         printf("%s %.2f %s\n", config.lang.msg_dnstime, dns_time/1000., config.lang.msg_seconds);
      }

      // report total report generation time
      if(!config.batch && !config.compact_db && !config.end_month && !config.db_info)
         printf("%s %.2f %s\n", config.lang.msg_rpttime, rpt_time/1000., config.lang.msg_seconds);

      // report maintenance and total run time
      printf("%s %.2f %s\n", config.lang.msg_mnttime, mnt_time/1000., config.lang.msg_seconds);
      printf("%s %.2f %s\n", config.lang.msg_runtime, tot_time/1000., config.lang.msg_seconds);
   }

   return retcode;
}

// -----------------------------------------------------------------------
//
// webalizer_t::proc_logfile
//
// -----------------------------------------------------------------------

int webalizer_t::proc_logfile(void)
{
	int parse_code;
   char *cp1;                            /* generic char pointers       */
   hnode_t *hptr;
   unode_t *uptr;
   bool newvisit, newhost, newthost, newurl, newagent, newuser, newerr, newref, newdl;
   bool newrgrp, newugrp, newagrp, newigrp;
   bool pageurl, fileurl, httperr, robot, target, spammer, goodurl, entryurl, exiturl;
   size_t reclen;
   const string_t *sptr, empty, *ragent;
   uint64_t stime;
   bool gz_log = false;                  // flag for zipped log
   u_int newsrch = 0;

   tstamp_t rec_tstamp;  

   uint64_t total_good = 0;

   bool good_rec = false;                /* true if we had a good record*/

   int retcode = 0, errnum = 0;

   lfp_state_t wlfs;                   // working log file state
   vector_t<lfp_state_t> lfp_state;    // log file states ordered by log time (TODO: change to a list)
   vector_t<logfile_t*> logfiles;      // owns log files
   vector_t<log_struct*> logrecs;      // owns log records
   
   tm_ranges_t::iterator dst_iter = config.dst_ranges.begin();
   
   //
   // Walk through the list of log files and test if each one is radable. Do not 
   // open files at this point, so we don't exceed the limit on the total number 
   // of open files.
   //
   vector_t<string_t>::const_iterator iter = config.log_fnames.begin();
   while(iter.more()) {
      const string_t& fname = iter.next();
      autoptr_t<logfile_t> logfile(new logfile_t(fname.length() && !is_abs_path(fname) ? make_path(config.cur_dir, fname) : fname));
      
      // check if we can read the file
      if(!logfile->is_readable()) {
         /* Error: Can't open log file ... */
         throw exception_t(0, string_t::_format("%s %s\n",config.lang.msg_log_err, fname.c_str()));
      }
      
      /* Using logfile ... */
      if (verbose>1)
      {
         printf("%s %s (",config.lang.msg_log_use, !fname.isempty() ? make_path(config.cur_dir, fname).c_str() : "STDIN");
         if (logfile->is_gzip()) printf("gzip-");
         printf("%s)\n", config.get_log_type_desc());
      }

      logfiles.push(logfile.release());
   }

   //
   // Loop through the log files and populate the log file state list. After
   // this loop we will have same number of log files and states in the lists 
   // and the working log file state structure (wlfs) will be empty.
   //
   for(size_t i = 0; i < logfiles.size();) {
      // the file may be open if we skipped any lines
      if(!logfiles[i]->is_open() && (errnum = logfiles[i]->open()) != 0) {
         /* Error: Can't open log file ... */
         throw exception_t(0, string_t::_format("%s %s (%d)\n", config.lang.msg_log_err, logfiles[i]->get_fname().c_str(), errnum));
      }

      // read the log record into the buffer
      if((reclen = read_log_line(*logfiles[i])) == 0) {
         // if there's no more data, remove the log file from the list and clean up
         printf("%s %s\n", config.lang.msg_log_done, logfiles[i]->get_fname().c_str());

         logfiles[i]->close();
         delete logfiles[i];
         logfiles.remove(i);
         
         continue;
      }
      
      // allocate a log record and set up the state structure
      if(!wlfs.logrec) {
         wlfs.logfile = logfiles[i];
         wlfs.logrec = new log_struct;
         logrecs.push(wlfs.logrec);
      }

      // parse the log line
      if((parse_code = parse_log_record(buffer, reclen, *wlfs.logrec)) == PARSE_CODE_ERROR) {
         total_bad++;
         continue;
      }
      
      // skip log file directives, etc
      if(parse_code == PARSE_CODE_IGNORE) {
         total_ignore++;
         continue;
      }
      
      //
      // There's a limit on how many files can be opened simulteneously. If we 
      // reached the maximum, close the file here and it will be opened again 
      // when the next line is read in the main loop.
      //
      if(logfiles.size() >= FOPEN_MAX) {
         if(logfiles[i]->set_reopen_offset() == -1L)
            throw exception_t(0, string_t::_format("%s %s (%d)\n", config.lang.msg_fpos_err, logfiles[i]->get_fname().c_str(), errnum));
            
         logfiles[i]->close();
      }
      
      // find the spot in the list and insert the record (earlier timestamps first)
      size_t j;
      for(j = 0; j < lfp_state.size() && wlfs.logrec->tstamp > lfp_state[j].logrec->tstamp; j++);
      lfp_state.insert(j, wlfs);

      // reset the working state and move onto the next log file
      wlfs.reset();
      i++;
   }

   //
   // Main processing loop - go through the log files until we run out of them.
   //
   // The previous loop guarantees that at that this point the number of states 
   // matches the number of log files and that wlfs is empty.
   //
   // Within this loop, the first state is moved to wlfs, the log record inside 
   // is processed and then the log file inside is used to read a new log record 
   // from that log file, creating a new state, which is then inserted into the 
   // state list according to the log record timestamp.
   //
   // Each file is opened only once, so the total number of open files will be 
   // determied by how many of them have log records in approximately same range
   // and how close log records are to each other. 
   //
   while(logfiles.size()) {
      
      // if we detected a Ctrl-C, break out right away 
      if(abort_signal) {
         fprintf(stderr, "%s\n", lang_t::msg_ctrl_c);
         break;
      }
   
      //
      // If we have fewer states than log files, then we either need to add a 
      // state or remove the log file identified by wlfs from the list.
      //
      while(lfp_state.size() < logfiles.size()) {
         // the log file in wlfs may be closed if wlfs is returned to the state vector first time
         if(!wlfs.logfile->is_open() && (errnum = wlfs.logfile->open()) != 0) {
            /* Error: Can't open log file ... */
            throw exception_t(0, string_t::_format("%s %s (%d)\n", config.lang.msg_log_err, wlfs.logfile->get_fname().c_str(), errnum));
         }

         // use logfile from wlfs, which was populated in the previous iteration
         if((reclen = read_log_line(*wlfs.logfile)) == 0) {
            size_t i;
            
            // report that we are done with this log file
            printf("%s %s\n", config.lang.msg_log_done, wlfs.logfile->get_fname().c_str());

            // find it in the list, remove it and clean up
            for(i = 0; i < logfiles.size() && wlfs.logfile != logfiles[i]; i++);
            logfiles[i]->close();
            delete logfiles[i];
            logfiles.remove(i);
            
            break;
         }
         
         // parse the log line
         if((parse_code = parse_log_record(buffer, reclen, *wlfs.logrec)) == PARSE_CODE_ERROR) {
            total_bad++;
            continue;
         }
         
         if(parse_code == PARSE_CODE_IGNORE) {
            total_ignore++;
            continue;
         }

         // find the spot in the list and insert the record (earlier timestamps first)
         size_t j;
         for(j = 0; j < lfp_state.size() && wlfs.logrec->tstamp > lfp_state[j].logrec->tstamp; j++);
         lfp_state.insert(j, wlfs);
         wlfs.reset();
      }
   
      // check if there are unprocessed states
      if(lfp_state.size()) {
         //
         // Get the first state and remove it from the list. The log file in the  
         // working state lets us keep track of which log file needs to be read from.
         //
         wlfs = lfp_state[0];
         lfp_state.remove(0);
         
         log_struct& log_rec = *wlfs.logrec;
         
         newvisit = false;

         /*********************************************/
         /* GOOD RECORD, CHECK INCREMENTAL/TIMESTAMPS */
         /*********************************************/

         /* Flag as a good one */
         good_rec = true;

         // check if need to convert log time stamp to local time
         if(config.local_time)
            log_rec.tstamp.tolocal(config.get_utc_offset(log_rec.tstamp, dst_iter));

         /* get current records timestamp (seconds since epoch) */
         rec_tstamp = log_rec.tstamp;

         /* Do we need to check for duplicate records? (incremental mode)   */
         if (check_dup)
         {
            /* check if less than/equal to last record processed            */
            if ( rec_tstamp <= state.totals.cur_tstamp )
            {
               /* if it is, assume we have already processed and ignore it  */
               total_ignore++;
               continue;
            }

            /* if it isn't.. disable any more checks this run            */
            check_dup = false;

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
                     state.totals.cur_tstamp.sec   = log_rec.tstamp.sec;
                     state.totals.cur_tstamp.min   = log_rec.tstamp.min;
                     state.totals.cur_tstamp.hour  = log_rec.tstamp.hour;
                     state.totals.cur_tstamp.day   = log_rec.tstamp.day;
                     state.totals.cur_tstamp.month = log_rec.tstamp.month;
                     state.totals.cur_tstamp.year  = log_rec.tstamp.year;
                     state.totals.cur_tstamp= rec_tstamp;
                     state.totals.f_day=state.totals.l_day=log_rec.tstamp.day;
               }
            }
         }

         /* check for out of sequence records */
         if (rec_tstamp < state.totals.cur_tstamp) {
            total_ignore++; 
            continue; 
         }

         total_good++;

         /********************************************/
         /* PROCESS RECORD                           */
         /********************************************/

         //
         // Update state timestamp parts and related data items
         //

         /* first time through? */
         if (state.totals.cur_tstamp.month == 0)
         {
             /* if yes, init our date vars */
             state.totals.cur_tstamp.month=log_rec.tstamp.month; state.totals.cur_tstamp.year=log_rec.tstamp.year;
             state.totals.cur_tstamp.day=log_rec.tstamp.day; state.totals.cur_tstamp.hour=log_rec.tstamp.hour;
             state.totals.cur_tstamp.min=log_rec.tstamp.min; state.totals.cur_tstamp.sec=log_rec.tstamp.sec;
             state.totals.f_day=log_rec.tstamp.day;
         }

         //
         // Check for month change. The state timestamp must not be updated 
         // until reports are generated and the database is rolled over.
         //
         if (state.totals.cur_tstamp.year != log_rec.tstamp.year || state.totals.cur_tstamp.month != log_rec.tstamp.month)
         {
            if(config.is_dns_enabled()) {
               stime = msecs();
				   dns_resolver.dns_wait();
               dns_time += elapsed(stime, msecs());
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
				   group_hosts_by_name();

            // save run data for the report generator
            stime = msecs();
            if (state.save_state()) {
               /* Error: Unable to save current run data */
               if (verbose) 
                  fprintf(stderr,"%s\n",config.lang.msg_data_err);
               // report generator uses saved state data
               return 1;
            }
            mnt_time += elapsed(stime, msecs());

            // generate monthly reports if not in batch mode
            if(!config.batch) {
               stime = msecs();
               if(!state.database.attach_indexes(true))
                  throw exception_t(0, "Cannot create secondary database indexes");
               write_monthly_report();                /* generate HTML for month */
               rpt_time += elapsed(stime, msecs());
            }

            stime = msecs();
            state.clear_month();
            mnt_time += elapsed(stime, msecs());
         }

         state.set_tstamp(log_rec.tstamp);            // update current timestamp

         /*********************************************/
         /* DO SOME PRE-PROCESS FORMATTING            */
         /*********************************************/

         // remember spammers
         if((spammer = (config.spam_refs.isinlist(log_rec.refer) != NULL)))
            put_spnode(log_rec.hostname);

         // run search arguments through the filters
         filter_srchargs(log_rec.srchargs);

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
         // 3. Hosts are marked as robots when user agent matches one of 
         // the Robot entries and only when an hnode is created. If a human
         // and a robot share the same IP address, this address will be 
         // marked as robot or non-robot depending on which user agent was 
         // active when the first hit was made.
         //
         // 4. A visit is marked as a robot visit when the user agent 
         // matches one of the Robot entries and a vnode is created, 
         // regardless whether the host is marked as a robot or not.
         // The robot flag in the visit is purely informational. See
         // vnode_t for details.
         //
         // 5. A user agent is marked as a robot if the current visit is 
         // marked as a robot.
         //
         // 7. Country totals do not include robot activity.
         //
         
         // if robots can be ignored, set ragent now, otherwise, do it after the ignore check
         ragent = config.ignore_robots ? config.robots.isinglist(log_rec.agent) : NULL;

         //
         // Ignore/Include check
         //

         if ( (config.include_sites.isinlist(log_rec.hostname)==NULL) &&
              (config.include_urls.isinlist(log_rec.url)==NULL)       &&
              (config.include_refs.isinlist(log_rec.refer)==NULL)     &&
              (config.include_agents.isinlist(log_rec.agent)==NULL)   &&
              (config.include_users.isinlist(log_rec.ident)==NULL)    )
         {
            if(ragent && config.ignore_robots)
              { total_ignore++; continue; }
            if (config.ignored_hosts.isinlist(log_rec.hostname) != NULL)
              { total_ignore++; continue; }
            if (config.ignored_urls.isinlist(log_rec.url)!=NULL)
              { total_ignore++; continue; }
            if (config.ignored_agents.isinlist(log_rec.agent)!=NULL)
              { total_ignore++; continue; }
            if (config.ignored_refs.isinlist(log_rec.refer)!=NULL)
              { total_ignore++; continue; }
            if (config.ignored_users.isinlist(log_rec.ident)!=NULL)
              { total_ignore++; continue; }

            /* 
            // If IgnoreHost contains domain patterns, wait for DNS results
            */
            if (config.is_dns_enabled() && config.ignored_domain_names) {
               stime = msecs();
               dns_resolver.dns_wait();
               dns_time += elapsed(stime, msecs());

               if(config.ignored_hosts.isinlist(dns_resolver.dns_resolve_name(log_rec.hostname, buffer, BUFSIZE)) != NULL) {
                  total_ignore++; 
                  continue; 
               }
            }
         }

         // if not ignored, check if a robot and set ragent (ignore spammers)
         if(!config.ignore_robots)
            ragent = (!spammer) ? config.robots.isinglist(log_rec.agent) : NULL;

         /* Do we need to mangle? */
         if(config.mangle_agent) {
            if (config.use_classic_mangler)
               mangle_user_agent(log_rec.agent);
            else
               filter_user_agent(log_rec.agent, ragent);
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
         
         // if appears to be not a spammer, check their past
         if(!spammer)
            spammer = state.sp_htab.find_key(log_rec.hostname);
         
         // initialize those that may be not set otherwise (e.g. if URL is not added)
         newurl = newagent = newuser = newerr = newref = newdl = newrgrp = newugrp = newagrp = newigrp = false;

         //
         // host name hash table - monthly
         //
         // put_hnode sets newvisit and must be called before any other put_xnode 
         // function.
         //
         if((hptr = put_hnode(log_rec.hostname, rec_tstamp, log_rec.xfer_size, fileurl, pageurl, 
            spammer, ragent != NULL, target, newvisit, newhost, newthost)) == NULL)
         {
            if (verbose)
               /* Error adding host node (monthly), skipping .... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_mh, log_rec.hostname.c_str());
         }

         // add new hosts to the resolver queue
         if(config.is_dns_enabled() && newhost)
			   dns_resolver.put_hnode(hptr);

         // use the host robot flag (ignore mid-visit ragent)
         robot = hptr->robot;

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
            if((uptr = put_unode(log_rec.url, log_rec.srchargs, OBJ_REG,
                log_rec.xfer_size, log_rec.proc_time/1000., log_rec.port, entryurl, target, newurl)) == NULL)
            {
               if (verbose)
               /* Error adding URL node, skipping ... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_u, log_rec.url.c_str());
            }
            
            // update the last URL for the current visit
            if(hptr && exiturl) hptr->set_last_url(uptr);

            /* ident (username) hash table */
            if(!log_rec.ident.isempty()) {
               if(!put_inode(log_rec.ident, OBJ_REG, fileurl, log_rec.xfer_size, rec_tstamp, log_rec.proc_time/1000., newuser))
               {
                  if (verbose)
                  /* Error adding ident node, skipping .... */
                  fprintf(stderr,"%s %s\n", config.lang.msg_nomem_i, log_rec.ident.c_str());
               }
            }
         }

         //
         // Downloads
         //
         if(config.ntop_downloads || config.dump_downloads) {
            if((sptr = config.downloads.isinglist(log_rec.url)) != NULL) {
               if(log_rec.resp_code == RC_OK || log_rec.resp_code == RC_PARTIALCONTENT) {
                  if(!put_dlnode(*sptr, log_rec.resp_code, rec_tstamp, log_rec.proc_time, log_rec.xfer_size, *hptr, newdl)) {
                     if (verbose)
                        /* Error adding a download node, skipping .... */
                        fprintf(stderr,"%s %s\n", config.lang.msg_nomem_dl, log_rec.url.c_str());
                  }
               }
            }
         }

         //
         // error HTTP response codes
         //
         if(httperr && (config.ntop_errors || config.dump_errors)) {
            if(!put_rcnode(log_rec.method, log_rec.url, log_rec.resp_code, false, 1, &newerr)) {
               if (verbose)
                  /* Error adding response code node, skipping .... */
                  fprintf(stderr,"%s %d %s\n", config.lang.msg_nomem_rc, log_rec.resp_code, log_rec.url.c_str());
            }
         }

         //
         // referrer hash table
         //
         if (config.ntop_refs && !log_rec.refer.isempty())
         {
            // filter out spammers
            if(!spammer) {
               // check if it's a partial request and ignore the referrer if requested
               if(!config.ignore_referrer_partial || log_rec.resp_code != RC_PARTIALCONTENT) {
                  if(!put_rnode(log_rec.refer, OBJ_REG, (uint64_t)1, newvisit, newref)) {
                     if (verbose) 
                        fprintf(stderr,"%s %s\n", config.lang.msg_nomem_r, log_rec.refer.c_str());
                  }
               }
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
            if(!log_rec.agent.isempty()) {
               if(!put_anode(log_rec.agent, OBJ_REG, log_rec.xfer_size, newvisit, !config.use_classic_mangler && robot, newagent)) {
                  if (verbose)
                     fprintf(stderr,"%s %s\n", config.lang.msg_nomem_a, log_rec.agent.c_str());
               }
            }
         }

         /* do search string stuff if needed     */
         if(config.ntop_search) {
            // reset the new search node count
            newsrch = 0;
            if(config.log_type == LOG_SQUID) {
               // count only successful requests (unlike with referrers)
               if(log_rec.resp_code == RC_OK)
                  srch_string(log_rec.url, log_rec.srchargs, newsrch, newvisit);
            }
            else {
               // check if it's a partial request and ignore the referrer if requested
               if(!config.ignore_referrer_partial || log_rec.resp_code != RC_PARTIALCONTENT) {
                  srch_string(log_rec.refer, log_rec.xsrchstr, newsrch, newvisit);
               }
            }
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

         if(config.ntop_search) state.totals.t_search += newsrch;

			/* 
			// Monthly average and maximum hit/file/page processing time 
			*/
			state.totals.a_hitptime = AVG(state.totals.a_hitptime, ((double) log_rec.proc_time / 1000.), state.totals.t_hit);
			state.totals.m_hitptime = MAX(state.totals.m_hitptime, ((double) log_rec.proc_time / 1000.));

         /* if RC_OK, increase file counters */
         if (fileurl)
         {
            state.totals.t_file++; state.totals.ht_files++;
            state.t_daily[log_rec.tstamp.day-1].tm_files++;
            state.t_hourly[log_rec.tstamp.hour].th_files++;

				state.totals.a_fileptime = AVG(state.totals.a_fileptime, ((double) log_rec.proc_time / 1000.), state.totals.t_file);
				state.totals.m_fileptime = MAX(state.totals.m_fileptime, ((double) log_rec.proc_time / 1000.));

            if(robot) state.totals.t_rfiles++;
         }

         /* Pages (pageview) calculation */
         if (pageurl) {
            state.totals.t_page++; state.totals.ht_pages++;
            state.t_daily[log_rec.tstamp.day-1].tm_pages++;
            state.t_hourly[log_rec.tstamp.hour].th_pages++;

				state.totals.a_pageptime = AVG(state.totals.a_pageptime, ((double) log_rec.proc_time / 1000.), state.totals.t_page);
				state.totals.m_pageptime = MAX(state.totals.m_pageptime, ((double) log_rec.proc_time / 1000.));

            if(robot) state.totals.t_rpages++;
         }

         /*********************************************/
         /* RECORD PROCESSED - DO GROUPS HERE         */ 
         /*********************************************/

         /* URL Grouping */
         if((sptr = config.group_urls.isinglist(log_rec.url))!=NULL)
         {
            if(!put_unode(*sptr, empty, OBJ_GRP, log_rec.xfer_size, log_rec.proc_time/1000., 0, false, false, newugrp)) {
               if (verbose)
                  /* Error adding URL node, skipping ... */
                  fprintf(stderr,"%s %s\n", config.lang.msg_nomem_u, sptr->c_str());
            }
         }

         // URL domain grouping
         if(config.group_url_domains && (cp1 = get_url_domain(log_rec.url, buffer, BUFSIZE)) != NULL) {
            const char *domain = get_domain(cp1, config.group_url_domains);
            if(!put_unode(string_t::hold(domain), empty, OBJ_GRP, log_rec.xfer_size, log_rec.proc_time/1000., 0, false, false, newugrp)) {
               if (verbose)
                  /* Error adding URL node, skipping ... */
                  fprintf(stderr,"%s %s\n", config.lang.msg_nomem_u, domain);
            }
         }

         /* Referrer Grouping */
         if((sptr = config.group_refs.isinglist(log_rec.refer))!=NULL)
         {
            if(!put_rnode(*sptr, OBJ_GRP, 1ul, newvisit, newrgrp)) {
               if (verbose)
               /* Error adding Referrer node, skipping ... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_r, sptr->c_str());
            }
         }

         /* User Agent Grouping */
         if((sptr = config.group_agents.isinglist(log_rec.agent))!=NULL)
         {
            if(!put_anode(*sptr, OBJ_GRP, log_rec.xfer_size, newvisit, false, newagrp)) {
               if (verbose)
               /* Error adding User Agent node, skipping ... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_a, sptr->c_str());
            }
         }

         // group robots
         if(robot && ragent && config.group_robots)
         {
            if(!put_anode(*ragent, OBJ_GRP, log_rec.xfer_size, newvisit, true, newagrp)) {
               if (verbose)
               /* Error adding User Agent node, skipping ... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_a, ragent->c_str());
            }
         }

         /* Ident (username) Grouping */
         if((sptr = config.group_users.isinglist(log_rec.ident))!=NULL)
         {
            if(!put_inode(*sptr, OBJ_GRP, fileurl, log_rec.xfer_size, rec_tstamp, log_rec.proc_time/1000., newigrp))
            {
               if (verbose)
               /* Error adding Username node, skipping ... */
               fprintf(stderr,"%s %s\n", config.lang.msg_nomem_i, sptr->c_str());
            }
         }

         // update group counts (host counts are updated in group_hosts_by_name)
         if(newugrp) state.totals.t_grp_urls++;
         if(newigrp) state.totals.t_grp_users++;
         if(newrgrp) state.totals.t_grp_refs++;
         if(newagrp) state.totals.t_grp_agents++;

         //
         // swap out hash tables in the database mode
         //
         if(!config.memory_mode && total_good >= config.swap_first_record) {
            if(total_rec && total_good % config.swap_frequency == 0) {
               stime = msecs();
               state.swap_out();
               mnt_time += elapsed(stime, msecs());
            }
         }
      }
   }

   /*********************************************/
   /* DONE READING LOG FILES - final processing */
   /*********************************************/
   
   // if there are any unprocessed log files, close them (e.g. Ctrl-C was pressed)
   for(size_t i = 0; i < logfiles.size(); i++) {
      if(logfiles[i] && logfiles[i]->is_open())
         logfiles[i]->close(); 
   }
   logfiles.clear();
   
   // deallocate log records we created earlier
   for(size_t i = 0; i < logrecs.size(); i++)
      delete logrecs[i];
   logrecs.clear();

   if(good_rec)                                                         /* were any good records?   */
   {
      if (state.totals.ht_hits > state.totals.hm_hit) state.totals.hm_hit = state.totals.ht_hits;

      if (total_rec > (total_ignore+total_bad))                         /* did we process any?   */
      {
         if(config.is_dns_enabled()) {
            stime = msecs();
			   dns_resolver.dns_wait();
            dns_time += elapsed(stime, msecs());
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
            update_visits(rec_tstamp);

         update_downloads(rec_tstamp);

         if(config.is_dns_enabled())
			   group_hosts_by_name();

         // save run data for the report generator
         stime = msecs();
         if (state.save_state()) {
            /* Error: Unable to save current run data */
            if (verbose) 
               fprintf(stderr,"%s\n",config.lang.msg_data_err);
         }
         mnt_time += elapsed(stime, msecs());

         // generate intermediate reports if not in batch mode
         if(!config.batch) {
            stime = msecs();
            if(!state.database.attach_indexes(true))
               throw exception_t(0, "Cannot create secondary database indexes");
            write_monthly_report();             /* write monthly HTML file  */
            write_main_index();                 /* write main HTML file     */
            rpt_time += elapsed(stime, msecs());
         }

         // if it's the last log for the month, roll over the database
         if(config.last_log) {
            stime = msecs();
            state.clear_month();
            mnt_time += elapsed(stime, msecs());
         }

      }

      /* Whew, all done! Exit with completion status (0) */
      retcode = 0;
   }
   else
   {
      /* No valid records found... exit with error (1) */
      if (verbose) 
         printf("%s\n",config.lang.msg_no_vrec);

      retcode = 1;
   }

   return retcode;
}

//
//
//

int webalizer_t::qs_srcharg_cmp(const arginfo_t *e1, const arginfo_t *e2)
{
   if(!e1 || !e2)
      return e1 ? 1 : -1;

   if(!e1->name && !e2->name)
      return 0;

   if(!e1->name || !e2->name)
      return e1->name ? 1 : -1;

   return strncmp(e1->name, e2->name, MIN(e1->namelen, e2->namelen));
}

void webalizer_t::filter_srchargs(string_t& srchargs)
{
   arginfo_t arginfo;
   char *cptr;
   string_t::char_buffer_t sa;

   if(srchargs.isempty())
      return; 

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
   while (*cptr) {
      while(*cptr == '&') cptr++;
      arginfo.name = cptr;
      while(*cptr && *cptr != '=') cptr++;
      arginfo.namelen = cptr - arginfo.name;
      while(*cptr && *cptr != '&') cptr++;
      arginfo.arglen = cptr - arginfo.name;

      // 
      // [1] everything is included
      // [2] argument has no name and there's no wildcard exclusion
      // [3] argument has a name and it's either included or not excluded
      //
      if(config.incl_srch_args.iswildcard() ||
         !arginfo.namelen && !config.excl_srch_args.iswildcard() ||
         (arginfo.namelen && 
            (config.incl_srch_args.isinlistex(arginfo.name, arginfo.namelen, false) || 
            !config.excl_srch_args.isinlistex(arginfo.name, arginfo.namelen, false)))) {

         if(sr_args.size() >= sr_args.capacity())
            sr_args.reserve(sr_args.capacity() << 1);
         sr_args.push(arginfo);
      }
   }

   // if none remaining, return
   if(!sr_args.size()) {
      srchargs.reset();
      return;
   }

   // sort the resulting vector, if requested
   if(config.sort_srch_args && sr_args.size() > 1)
      qsort(&sr_args[0], sr_args.size(), sizeof(arginfo_t), (int (*)(const void*, const void*)) qs_srcharg_cmp);

   // copy remaining search arguments to the buffer
   cptr = buffer;
   for(size_t index = 0; index < sr_args.size(); index++) {
      if(index > 0)
         *cptr++ = '&';
      cptr += strncpy_ex(cptr, BUFSIZE - (cptr-buffer), sr_args[index].name, sr_args[index].arglen);
   }

   // copy to the original memory block
   memcpy(sa, buffer, cptr-buffer);
   sa[cptr-buffer] = 0;

   // attach the memory block to the string
   srchargs.attach(sa, cptr-buffer);

   sr_args.clear();
}

/*********************************************/
/* SRCH_STRING - get search strings from ref */
/*********************************************/

void webalizer_t::srch_string(const string_t& refer, const string_t& srchargs, u_int& scount, bool newvisit)
{
   const gnode_t *nptr = NULL;
   const char *cp1, *cp3, *cps = NULL;
   char *cp2, *bptr;
   int  sp_flg = 0, termcnt = 0;
   bool newsrch = false;
   size_t slen = 0, qlen = 0;
   string_t str;
   glist::const_iterator iter = config.search_list.begin();

   // reset the new node count
   scount = 0;

   // ignore empty referrers and empty search strings
   if(srchargs.isempty() || refer.isempty())
      return;

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
         while(*cp1 && *cp3 && *cp1 == *cp3) *cp1++, *cp3++;
         if(*cp3 == 0)
            break;

         // move to the next query variable
         while(*cp1 && *cp1++ != '&');
      } while(*cp1);

      if(*cp1 == 0) continue;                         // If not found, continue  

      // copy and decode the search string 
      cp2 = buffer;
      while(*cp1 && *cp1!='&')
      {
         if(*cp1 == '%')                              // decode URL-encoded chars
            cp1 = from_hex(++cp1, cp2);
         else if(*cp1 == '+') 
            *cp2 = ' ', cp1++;                        // change + to space       
         else if(*cp1 < '\x20') 
            *cp2='_', cp1++;                          // strip invalid chars     
         else
            *cp2 = tolower(*cp1), cp1++;              // normal character        

         if (sp_flg && *cp2==' ') continue;           // compress spaces         
         if(*cp2==' ') sp_flg=1; else sp_flg=0;       // (flag spaces here)      
         cp2++;
      }

      // trim trailing spaces
      cp3 = buffer;                                       
      while(cp2 > cp3 && isspaceex(*(cp2-1))) cp2--;
      *cp2 = 0;

      // trim leading spaces
      cp1 = buffer;
      while(*cp1 && isspaceex(*cp1)) cp1++;           

      // hold on to the query length
      slen = cp2 - cp1;

      // store in the hash table if not empty
      if(slen) {                                      
         termcnt++;
         cp3 = bptr = &buffer[HALFBUFSIZE];

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
         str.append(cp3, bptr-cp3);
      }
   }

   if(termcnt && !str.isempty()) {
      if(!put_snode(str, termcnt, newvisit, newsrch)) {
         if (verbose)
            // Error adding search string node, skipping .... 
            fprintf(stderr, "%s %s\n", config.lang.msg_nomem_sc, str.c_str());
      }
      
      // update the count if it's a new node
      if(newsrch) scount++;
   }
}

// -----------------------------------------------------------------------
//
// hosts
//
// -----------------------------------------------------------------------

hnode_t *webalizer_t::put_hnode(
               const string_t& ipaddr,          // IP address
               const tstamp_t& tstamp,          // timestamp 
               uint64_t xfer,                   // xfer size 
               bool     fileurl,                // file count
               bool     pageurl,
               bool     spammer,
               bool     robot,
               bool     target,
					bool&		newvisit,               // new visit?
               bool&    newnode,                // new host node?
               bool&    newthost                // new host today?
               )
{
   bool found = true;
   uint64_t hashval;
   hnode_t *cptr;
   vnode_t *visit;

   newnode = newvisit = newthost = false;

   hashval = hash_ex(0, ipaddr);

   /* check if hashed */
   if((cptr = state.hm_htab.find_node(hashval, ipaddr, OBJ_REG)) == NULL) {
      /* not hashed */
      if((cptr = new hnode_t(ipaddr)) != NULL) {
         if(!state.hm_htab.is_swapped_out() || !state.database.get_hnode_by_value(*cptr, state_t::unpack_hnode_cb, &state)) {
            cptr->nodeid = state.database.get_hnode_id();
            cptr->flag = OBJ_REG;

            cptr->spammer = spammer;
            cptr->robot = robot;

            cptr->set_visit(new vnode_t(cptr->nodeid));
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
            
            found = false;
         }
         state.hm_htab.put_node(hashval, cptr);
      }
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
		      if(!cptr->visit->dirty)
		         cptr->visit->dirty = true;
		   }
      }
      else {
         // no active visit - create one
         cptr->set_visit(new vnode_t(cptr->nodeid));
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
   }

   // spam request may be not the first one, so check every time
   if(spammer && !cptr->spammer)
      cptr->spammer = spammer;

   // update the last hit time stamp
   cptr->tstamp = tstamp;

   // if a non-robot requested a target URL, mark the visit as converted
   if(target && !robot && !spammer && !cptr->visit->converted)
      cptr->visit->converted = true;

   if(!cptr->dirty)
      cptr->dirty = true;

   return cptr;
}               

hnode_t *webalizer_t::put_hnode(
               const string_t& grpname,         // Hostname  
               uint64_t    hits,                  // hit count 
               uint64_t    files,                 // file count
               uint64_t    pages,
               uint64_t    xfer,                  // xfer size 
               uint64_t    visitlen,              // visit length
               bool&     newnode)
{
   bool found = true;
   uint64_t hashval;
   hnode_t *cptr;

   newnode = false;

   hashval = hash_ex(0, grpname);

   /* check if hashed */
   if((cptr = state.hm_htab.find_node(hashval, grpname, OBJ_GRP)) == NULL) {
      /* not hashed */
      if((cptr = new hnode_t(grpname)) != NULL) {
         if(!state.hm_htab.is_swapped_out() || !state.database.get_hnode_by_value(*cptr, state_t::unpack_hnode_cb, &state)) {
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
         state.hm_htab.put_node(hashval, cptr);
      }
   }
   
   if(found) {
      /* found... bump counter */
      cptr->count+=hits;
      cptr->files+=files;
      cptr->pages+=pages;
      cptr->xfer +=xfer;

      cptr->visits++;
      cptr->visit_avg = AVG(cptr->visit_avg, visitlen, cptr->visits);
      cptr->visit_max = MAX(cptr->visit_max, visitlen);
   }

   if(!cptr->dirty)
      cptr->dirty = true;

   return cptr;
}

rnode_t *webalizer_t::put_rnode(const string_t& str, nodetype_t type, uint64_t count, bool newvisit, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   rnode_t *nptr;

   newnode = false;

   hashval = hash_ex(0, str);

   /* check if hashed */
   if((nptr = state.rm_htab.find_node(hashval, str, type)) == NULL) {
      /* not hashed */
      if((nptr = new rnode_t(str)) != NULL) {
         if(!state.rm_htab.is_swapped_out() || !state.database.get_rnode_by_value(*nptr, NULL, NULL)) {
            nptr->nodeid = state.database.get_rnode_id();
            nptr->flag  = type;
            nptr->count = count;
            
            if(newvisit)
               nptr->visits++;

            newnode = true;
            found = false;
         }

         state.rm_htab.put_node(hashval, nptr);
      }
   }

   if(!nptr->dirty)
      nptr->dirty = true;

   if(found) {
      /* found... bump counter */
      nptr->count += count;
      
      if(newvisit)
         nptr->visits++;
         
      return nptr;
   }

   return nptr;
}

/*********************************************/
/* PUT_UNODE - insert/update URL node        */
/*********************************************/

unode_t *webalizer_t::put_unode(const string_t& str, const string_t& srchargs, nodetype_t type, uint64_t xfer, double proctime, u_short port, bool entryurl, bool target, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   unode_t *cptr;
   u_hash_table::param_block param;

   newnode = false;

   param.type = type;
   param.url = &str;
   param.srchargs = !srchargs.isempty() ? &srchargs : NULL;

   // hash pieces as if the entire URL was hashed
   hashval = (srchargs.isempty()) ? hash_ex(0, str) : hash_ex(hash_byte(hash_ex(0, str), '?'), srchargs);

   /* check if hashed */
   if((cptr = state.um_htab.find_node(hashval, &param)) == NULL) {
      /* not hashed */
      if((cptr = new unode_t(str, srchargs)) != NULL) {
         // check if in the database
         if(!state.um_htab.is_swapped_out() || !state.database.get_unode_by_value(*cptr)) {
            cptr->nodeid = state.database.get_unode_id();
            cptr->flag = type;
            cptr->count= 1;
            cptr->xfer = xfer;
			   cptr->avgtime = cptr->maxtime = proctime;
			   cptr->urltype = config.get_url_type(port, cptr->urltype);
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
         state.um_htab.put_node(hashval, cptr);
      }
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
		cptr->urltype = config.get_url_type(port, cptr->urltype);
		cptr->avgtime = AVG(cptr->avgtime, proctime, cptr->count);
		cptr->maxtime = MAX(cptr->maxtime, proctime);
   }

   if(!cptr->dirty)
      cptr->dirty = true;

   return cptr;
}

rcnode_t *webalizer_t::put_rcnode(const string_t& method, const string_t& url, u_short respcode, bool restore, uint64_t count, bool *newnode)
{
   bool found = true;
   rcnode_t *nptr;
   uint64_t hashval;
   rc_hash_table::param_block param;

   if(newnode)
      *newnode = false;
      
   if(method.isempty() || url.isempty())
      return NULL;

   param.respcode = respcode;
   param.url = url;
   param.method = method;

   // respcode, method, url
   hashval = hash_ex(hash_ex(hash_num(0, respcode), method), url);

   /* check if hashed */
   if((nptr = state.rc_htab.find_node(hashval, &param)) == NULL) {
      /* not hashed */
      if((nptr = new rcnode_t(method, url, respcode)) == NULL) 
         return NULL;

      if(!state.rc_htab.is_swapped_out() || !state.database.get_rcnode_by_value(*nptr, NULL, NULL)) {
         nptr->nodeid = state.database.get_rcnode_id();
         nptr->flag = OBJ_REG;
         nptr->count = count;
         if(newnode) *newnode = true;
         found = false;
      }
      state.rc_htab.put_node(hashval, nptr);
   }
   
   if(found) {
      /* found... bump counter */
      nptr->count += count;
   }

   if(!nptr->dirty)
      nptr->dirty = true;

   return nptr;
}

/*********************************************/
/* PUT_ANODE - insert/update user agent node */
/*********************************************/

anode_t *webalizer_t::put_anode(const string_t& str, nodetype_t type, uint64_t xfer, bool newvisit, bool robot, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   anode_t *cptr;

   newnode = false;
      
   hashval = hash_ex(0, str);

   /* check if hashed */
   if((cptr = state.am_htab.find_node(hashval, str, type)) == NULL) {
      /* not hashed */
      if((cptr=new anode_t(str, robot)) != NULL) {
         if(!state.am_htab.is_swapped_out() || !state.database.get_anode_by_value(*cptr)) {
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

         state.am_htab.put_node(hashval, cptr);
      }
   }

   if(found) {
      /* found... bump counter */
      cptr->count++;
      cptr->xfer += xfer;
		if(newvisit)
			cptr->visits++;
   }

   if(!cptr->dirty)
      cptr->dirty = true;

   return cptr;
}

/*********************************************/
/* PUT_SNODE - insert/update search str node */
/*********************************************/

snode_t *webalizer_t::put_snode(const string_t& str, u_int termcnt, bool newvisit, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   snode_t *nptr;

   newnode = false;

   if(str.isempty())     /* skip bad search strs */
      return NULL;

   hashval = hash_ex(0, str);

   /* check if hashed */
   if((nptr = state.sr_htab.find_node(hashval, str)) == NULL) {
      /* not hashed */
      if((nptr = new snode_t(str)) != NULL) {
         if(!state.sr_htab.is_swapped_out() || !state.database.get_snode_by_value(*nptr)) {
            nptr->nodeid = state.database.get_snode_id();
            nptr->count = 1;
            nptr->termcnt = termcnt;
            
            if(newvisit)
               nptr->visits++;
            
            found = false;
            newnode = true;
         }
         state.sr_htab.put_node(hashval, nptr);
      }
   }

   if(found) {
      /* found... bump counter */
      nptr->count++;

      if(newvisit)
         nptr->visits++;
   }

   state.totals.t_srchits++;

   if(!nptr->dirty)
      nptr->dirty = true;

   return nptr;
}

/*********************************************/
/* PUT_INODE - insert/update ident node      */
/*********************************************/

inode_t *webalizer_t::put_inode(const string_t& str,   /* ident str */
               nodetype_t    type,       /* obj type  */
               bool     fileurl,    /* File flag */
               uint64_t xfer,       /* xfer size */
               const tstamp_t& tstamp, /* timestamp */
               double   proctime,
               bool&    newnode)
{
   bool found = true;
   uint64_t hashval;
   inode_t *nptr;

   newnode = false;
   
   if(str.isempty()) return NULL;  /* skip if no username */

   hashval = hash_ex(0, str);

   /* check if hashed */
   if((nptr = state.im_htab.find_node(hashval, str, type)) == NULL) {
      /* not hashed */
      if ( (nptr=new inode_t(str)) != NULL) {
         if(!state.im_htab.is_swapped_out() || !state.database.get_inode_by_value(*nptr)) {
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
         state.im_htab.put_node(hashval, nptr);
      }
   }

   if(!nptr->dirty)
      nptr->dirty = true;
   
   if(found) {
      /* hashed */
      /* found... bump counter */
      nptr->count++;
      if(fileurl) nptr->files++;
      nptr->xfer +=xfer;
		nptr->avgtime = AVG(nptr->avgtime, proctime, nptr->count);
		nptr->maxtime = MAX(nptr->maxtime, proctime);

      if(tstamp.elapsed(nptr->tstamp) >= config.visit_timeout)
         nptr->visit++;
      nptr->tstamp=tstamp;
   }

   return nptr;
}

//
//
//
dlnode_t *webalizer_t::put_dlnode(const string_t& name, u_int respcode, const tstamp_t& tstamp, uint64_t proctime, uint64_t xfer, hnode_t& hnode, bool& newnode)
{
   bool found = true;
   uint64_t hashval;
   danode_t *download;
   dlnode_t *nptr;
   dl_hash_table::param_block params;

   newnode = false;

   if(name.isempty() || hnode.string.isempty())
      return NULL;

   if(respcode != RC_OK && respcode != RC_PARTIALCONTENT)
      return NULL;

   params.name = name;
   params.ipaddr = hnode.string;

   hashval = hash_ex(hash_ex(0, hnode.string), name);

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
   if((nptr = state.dl_htab.find_node(hashval, &params)) == NULL) {
      nptr = new dlnode_t(name, &hnode);
      if(!state.dl_htab.is_swapped_out() || !state.database.get_dlnode_by_value(*nptr, state_t::unpack_dlnode_cb, &state)) {
         nptr->nodeid = state.database.get_dlnode_id();
         nptr->download = new danode_t(nptr->nodeid);

         nptr->download->hits = 1;
         nptr->download->tstamp = tstamp;
         nptr->download->xfer = xfer;
         nptr->download->proctime = proctime;

         newnode = true;
         found = false;
      }

      state.dl_htab.put_node(hashval, nptr);
   }
   
   if(found) {
      if(nptr->download) {
         if((download = update_download(nptr, tstamp)) != NULL) {
            download->reset(nptr->nodeid);
            nptr->download = download;
         }
         else {
            if(!nptr->download->dirty)
               nptr->download->dirty = true;
         }
      }
      else
         nptr->download = new danode_t(nptr->nodeid);

      nptr->download->hits++;
      nptr->download->tstamp = tstamp;
      nptr->download->xfer += xfer;
      nptr->download->proctime += proctime;
   }

   if(!nptr->dirty)
      nptr->dirty = true;

   return nptr;
}

//
//
//
spnode_t *webalizer_t::put_spnode(const string_t& host) 
{
   spnode_t *spnode;
   uint64_t hashval;

   hashval = hash_ex(0, host);

   /* check if hashed */
   if((spnode = state.sp_htab.find_node(hashval, host)) != NULL)
      return spnode;

   if((spnode = new spnode_t(host)) != NULL)
      state.sp_htab.put_node(hashval, spnode);

   return spnode;
}

//
// update_visit
//
// If the active visit has ended, update the state and detach the
// visit from the host node. Return the detached and reset visit 
// to the caller, which is expected to dispose of the visit node.
//
vnode_t *webalizer_t::update_visit(hnode_t *hptr, const tstamp_t& tstamp)
{
   vnode_t *visit;
   uint64_t vlen;

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

   // detach the visit from the host
   hptr->set_visit(NULL);
   hptr->dirty = true;

   // update host counters
   hptr->count+=visit->hits;
   hptr->files+=visit->files;
   hptr->pages+=visit->pages;
   hptr->xfer +=visit->xfer;

   // get visit length
   vlen = (uint64_t) visit->end.elapsed(visit->start);

   // update maximum and average visit duration for this host
   hptr->visit_avg = AVG(hptr->visit_avg, vlen, hptr->visits);
   hptr->visit_max = MAX(hptr->visit_max, vlen);

   // update maximum hits, pages, files and transfer
   hptr->max_v_hits = MAX(visit->hits, hptr->max_v_hits);
   hptr->max_v_files = MAX(visit->files, hptr->max_v_files);
   hptr->max_v_pages = MAX(visit->pages, hptr->max_v_pages);
   hptr->max_v_xfer = MAX(visit->xfer, hptr->max_v_xfer);

   // update total maximum hits, pages, files and transfer
   state.totals.max_v_hits = MAX(hptr->max_v_hits, state.totals.max_v_hits);
   state.totals.max_v_files = MAX(hptr->max_v_files, state.totals.max_v_files);
   state.totals.max_v_pages = MAX(hptr->max_v_pages, state.totals.max_v_pages);
   state.totals.max_v_xfer = MAX(hptr->max_v_xfer, state.totals.max_v_xfer);

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
      state.totals.max_hv_hits = MAX(hptr->max_v_hits, state.totals.max_hv_hits);
      state.totals.max_hv_files = MAX(hptr->max_v_files, state.totals.max_hv_files);
      state.totals.max_hv_pages = MAX(hptr->max_v_pages, state.totals.max_hv_pages);
      state.totals.max_hv_xfer = MAX(hptr->max_v_xfer, state.totals.max_hv_xfer);
   
      // update total maximum and average visit durations
      state.totals.t_visit_avg = AVG(state.totals.t_visit_avg, vlen, state.totals.t_hvisits_end);
      state.totals.t_visit_max = MAX(hptr->visit_max, state.totals.t_visit_max);

      // update counters for converted visits
      if(visit->converted) {
         // if it's the first converted visit, increment converted host count
         if(!hptr->visits_conv)
            state.totals.t_hosts_conv++;            // total converted hosts
         
         state.totals.t_visits_conv++;              // total converted visits
         hptr->visits_conv++;                // and for this host

         state.totals.t_vconv_avg = AVG(state.totals.t_vconv_avg, vlen, state.totals.t_visits_conv);
         state.totals.t_vconv_max = MAX(hptr->visit_max, state.totals.t_vconv_max);
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
         visit->lasturl->dirty = true;
      }
      visit->set_lasturl(NULL);
   }

   // if the visit node was read from the database, queue it for deletion
   if(visit->storage) {
      state.v_ended.push(visit->nodeid);
      visit->storage = false;
   }

   // if DNS resolution is disabled, update host groups now
   if(!config.is_dns_enabled())
      group_host_by_name(*hptr, *visit);
   else {
      // otherwise, queue a copy of the visit for grouping when the host name is available
      hptr->add_grp_visit(new vnode_t(*visit));
   }

   return visit;
}

//
// update_visits
//
// Scan all host nodes and check each whether it has an active visit that can
// be ended. Delete returned ended visits, as we have no use for them at
// this point. 
//
void webalizer_t::update_visits(const tstamp_t& tstamp)
{
   vnode_t *visit;
   hash_table<hnode_t>::iterator h_iter = state.hm_htab.begin();
   
   while(h_iter.next()) {
      if((visit = update_visit(h_iter.item(), tstamp)) != NULL)
         delete visit;
   }
}

danode_t *webalizer_t::update_download(dlnode_t *dlnode, const tstamp_t& tstamp)
{
   danode_t *download;
   double value;

   if(!dlnode || dlnode->flag == OBJ_GRP || tstamp.null)
      return NULL;

   if((download = dlnode->download) == NULL)
      return NULL;

   // the elapsed time is always positive, so usual arithmetic conversions work
   if(tstamp.elapsed(download->tstamp) < config.download_timeout)
      return NULL;

   dlnode->download = NULL;
   dlnode->dirty = true;

   dlnode->count++;
   dlnode->sumhits += download->hits;

   dlnode->sumxfer += download->xfer;
   dlnode->avgxfer = AVG(dlnode->avgxfer, download->xfer, dlnode->count);

   value = (double) download->proctime/60000.;
   dlnode->sumtime += value;
   dlnode->avgtime = AVG(dlnode->avgtime, value, dlnode->count);

   state.totals.t_dlcount++;

   // if the download node was read from the database, queue it for deletion
   if(download->storage) {
      state.dl_ended.push(download->nodeid);
      download->storage = false;
   }

   return download;
}

//
// update_downloads
//
// Scan all download nodes and check each whether it has an active job that can
// be ended. Delete returned ended download jobs, as we have no use for them at
// this point. 
//
void webalizer_t::update_downloads(const tstamp_t& tstamp)
{
   danode_t *download;
   dlnode_t *nptr;
   dl_hash_table::iterator iter = state.dl_htab.begin();

   while((nptr = iter.next()) != NULL) {
      if((download = update_download(nptr, tstamp)) != NULL)
         delete download;
   }
}

/*********************************************/
/* OUR_GZGETS - enhanced gzgets for log only */
/*********************************************/

char *webalizer_t::our_gzgets(gzFile fp, char *buf, int size)
{
   char *out_cp=buf;      /* point to output */
   while (1)
   {
      if(f_cp > (f_buf+f_end-1))     /* load? */
      {
         f_end=gzread(fp, f_buf, GZ_BUFSIZE);
         if(f_end <= 0) 
            return Z_NULL;
         f_cp = f_buf;
      }

      if (--size)                   /* more? */
      {
         *out_cp++ = *f_cp;
         if (*f_cp++ == '\n') { *out_cp='\0'; return buf; }
      }
      else { 
         *out_cp='\0'; 
         return buf; 
      }
   }
}

/*********************************************/
/* MAIN - start here                         */
/*********************************************/
int main(int argc, char *argv[])
{
   int retcode;

#ifdef _WIN32
      //
      // Set a callback to translate Win32 structured exceptions (memory access
      // violations, etc) to C++ exceptins as early as possible.
      //
      _set_se_translator(_win32_se_handler);    // $$$ check if has to be reset to the default callback !!!
#endif

   try {
      webalizer_t logproc;

      try {
         // set the Ctrl-C handler
#ifdef _WIN32
         SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
         signal(SIGINT, console_ctrl_handler);
#endif 

         // initialize the log processor
         logproc.initialize(argc, argv);

         // run the log processor 
         retcode = logproc.run();

         // clean up
         logproc.cleanup();

         // remove the Ctrl-C handler
#ifndef _WIN32
         signal(SIGINT, NULL);   // $$$ check if setting it to NULL is the right way to reset the handler
#endif

         return retcode;
      }
#ifdef _WIN32
      catch(ex_win32_se_t& err) {
         if(verbose) {
            fprintf(stderr, "%s\n", err.desc().c_str());
            print_loaded_modules();
         }
      }
#endif      
      catch (DbException &err) {
         if(verbose)
            fprintf(stderr, "[%d] %s\n", err.get_errno(), err.what());
      }
      catch (exception_t &err) {
         if(verbose)
            fprintf(stderr, "%s\n", err.desc().c_str());
      }

      //
      // If an exception was caught, try to clean up the log processor. If
      // any exception is thrown at this point, report it and just exit.
      //

      logproc.cleanup();
   }
#ifdef _WIN32
   catch(ex_win32_se_t& err) {
      if(verbose) {
         fprintf(stderr, "%s\n", err.desc().c_str());
         print_loaded_modules();
      }
   }
#endif      
   catch (DbException &err) {
      if(verbose)
         fprintf(stderr, "[%d] %s\n", err.get_errno(), err.what());
   }
   catch (exception_t &err) {
      if(verbose)
         fprintf(stderr, "%s\n", err.desc().c_str());
   }

   return -1;
}

//
// Win32 detailed exception reporting
//
#ifdef _WIN32
void __cdecl _win32_se_handler(unsigned int excode, _EXCEPTION_POINTERS *exinfo)
{
   throw ex_win32_se_t(excode, exinfo);
}

void print_loaded_modules(void)
{
   DWORD sizein = 0, sizeout = 0, modulecnt = 0;
   HMODULE *modules = NULL;
   MODULEINFO moduleinfo;
   char modulename[256];

   if(!EnumProcessModules(GetCurrentProcess(), NULL, 0, &sizein)) {
      fprintf(stderr, "Cannot enumerate modules");
      return;
   }
   
   // WinXP doesn't null-terminate the returned string if the buffer is too small
   modulename[sizeof(modulename)-sizeof(char)] = 0;
   
   // double the size to account for modules loaded after the first call
   sizein *= 2;
   
   modules = new HMODULE[sizein / sizeof(HMODULE)];
   
   if(!EnumProcessModules(GetCurrentProcess(), modules, sizein, &sizeout)) {
      fprintf(stderr, "Cannot enumerate modules");
      goto funcexit;
   }
   
   // the number of modules we actually have is the minimum of the two sizes
   modulecnt = MIN(sizein/sizeof(HMODULE), sizeout/sizeof(HMODULE));
   
   // print the total just so we know if we missed anything
   fprintf(stderr, "Loaded modules (%lu of %lu)\n", modulecnt, static_cast<DWORD>(sizeout / sizeof(HMODULE)));
   
   for(DWORD i = 0; i < modulecnt; i++) {
      if(!GetModuleFileNameA(modules[i], modulename, sizeof(modulename)-sizeof(char)))
         strcpy(modulename, "*unknown*");
      
      if(!GetModuleInformation(GetCurrentProcess(), modules[i], &moduleinfo, sizeof(MODULEINFO)))
         memset(&moduleinfo, 0, sizeof(MODULEINFO));
      
      fprintf(stderr, "%s (%p-%p)\n", modulename, moduleinfo.lpBaseOfDll, (unsigned char*) moduleinfo.lpBaseOfDll + moduleinfo.SizeOfImage);
   }
   
funcexit:
   delete [] modules;   
}
#endif 

//
// If Ctrl-C is not handled properly, both Berkeley databases may get damaged 
//
#ifdef _WIN32
static BOOL WINAPI console_ctrl_handler(DWORD type)
{
   switch(type) {
      case CTRL_C_EVENT:
      case CTRL_BREAK_EVENT:
      case CTRL_CLOSE_EVENT:
      case CTRL_LOGOFF_EVENT:
      case CTRL_SHUTDOWN_EVENT:
         abort_signal = true;
         return TRUE;
   }

   return FALSE;
}
#else
static void console_ctrl_handler(int sig)
{
   switch(sig) {
      case SIGINT:
         signal(SIGINT, SIG_IGN);
         abort_signal = true;
         break;
   }
}
#endif

int webalizer_t::read_log_line(logfile_t& logfile)
{
   int reclen = 0, errnum = 0;

   // read the line ad check if there's no more data; EOF is checked in logfile_t::get_line
   while((reclen = logfile.get_line(buffer, BUFSIZE, &errnum)) != 0) {
      
      // in case of an error, throw an exception - errors cannot be just reported as bad log lines
      if(reclen == -1)
         throw exception_t(0, string_t::_format("%s: %s (%d)", config.lang.msg_file_err, logfile.get_fname().c_str(), errnum));
         
      // live IIS log files are zero-padded to a 64K boundary
      if(config.log_type == LOG_IIS && *buffer == 0)
         return 0;

      total_rec++;
      
      // if the buffer is not full, return
      if(reclen < BUFSIZE-1 || buffer[BUFSIZE-1] == '\n')
         return reclen;
      
      total_bad++;                     /* bump bad record counter      */

      // full buffer - report record number, file name and the record if running in debug mode
      if (verbose) {
         
         fprintf(stderr,"%s (%" PRIu64 " - %s)",config.lang.msg_big_rec, total_rec, logfile.get_fname().c_str());
         if (debug_mode) 
            fprintf(stderr,":\n%s",buffer);
         else 
            fprintf(stderr,"\n");
      }

      /* get the rest of the record */
      while((reclen = logfile.get_line(buffer, BUFSIZE, &errnum)) != 0) {
         // handle errors
         if(reclen == -1)
            throw exception_t(0, string_t::_format("%s: %s (%d)", config.lang.msg_file_err, logfile.get_fname().c_str(), errnum));

         // print this buffer
         if (debug_mode && verbose) 
            fprintf(stderr,"%s",buffer);

         // if at the end of the oversized record, print EOL and move on to the next line
         if(reclen < BUFSIZE-1) {
            if (debug_mode && verbose)
               fprintf(stderr,"\n");
            break;
         }
      }
   }
   
   return reclen;
}

int webalizer_t::parse_log_record(char *buffer, size_t reclen, log_struct& logrec)
{
   int parse_code;
   string_t lrecstr;

   //
   // parser_t::parse_record modifies the buffer, so we need to save the original
   // record in case we need to report an error. This is expensive - do it only
   // in debug mode.
   //
   if(debug_mode)
	   lrecstr = buffer;

   if((parse_code = parser.parse_record(buffer, reclen, logrec)) == PARSE_CODE_ERROR) {

      /* really bad record... */
      if (verbose)
      {
         fprintf(stderr,"%s (%" PRIu64 ")", config.lang.msg_bad_rec, total_rec);
         if (debug_mode) fprintf(stderr,":\n%s\n", lrecstr.c_str());
         else fprintf(stderr,"\n");
      }
   }
   
   return parse_code;
}

