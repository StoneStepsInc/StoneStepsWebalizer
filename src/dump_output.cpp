/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   dump_output.cpp
*/
#include "pch.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/types.h>

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "lang.h"
#include "hashtab.h"
#include "linklist.h"
#include "util_http.h"
#include "util_url.h"
#include "exception.h"
#include "dump_output.h"
#include "preserve.h"

#include <stdexcept>

dump_output_t::dump_output_t(const config_t& config, const state_t& state) : output_t(config, state)
{
}

dump_output_t::~dump_output_t(void)
{
}

bool dump_output_t::init_output_engine(void)
{
   return true;
}

void dump_output_t::cleanup_output_engine(void)
{
}

int dump_output_t::write_main_index()
{
   return 0;
}

int dump_output_t::write_monthly_report(void)
{
   // dump downloads tab file
   if (config.dump_downloads) 
      dump_all_downloads();

   // Dump URLS tab file
   if (config.dump_urls) 
      dump_all_urls();     

   // dump HTTP errors tab file
   if (config.dump_errors) 
      dump_all_errors();    

   // Dump sites tab file
   if (config.dump_hosts) 
      dump_all_hosts();   

   // Dump referrers tab file
   if (config.dump_refs) 
      dump_all_refs();    

   // dump search string tab file
   if (config.dump_search) 
      dump_all_search();  

   // dump usernames tab file
   if (config.dump_users) 
      dump_all_users();  

   // dump user agents tab file
   if (config.dump_agents) 
      dump_all_agents();
      
   if(config.dump_countries)
      dump_all_countries();

   if(config.dump_cities)
      dump_all_cities();
      
   return 0;
}

/*********************************************/
/* DUMP_ALL_SITES - dump sites to tab file   */
/*********************************************/

void dump_output_t::dump_all_hosts()
{
   storable_t<hnode_t> hnode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/site_%04d%02d.%s",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            config.lang.msg_h_hits, config.lang.msg_h_files, config.lang.msg_h_pages, 
            config.lang.msg_h_xfer, config.lang.msg_h_visits, config.lang.msg_h_duration, 
            config.lang.msg_h_duration, config.lang.msg_h_ccode, config.lang.msg_h_ctry, config.lang.msg_h_city, 
            config.lang.msg_h_type, config.lang.msg_h_latitude, config.lang.msg_h_longitude,
            config.lang.msg_h_ipaddr, config.lang.msg_h_hname);
   }

   /* dump 'em */
   database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts("hosts.hits");

   while (iter.prev(hnode, (hnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      if (hnode.flag != OBJ_GRP)
      {
         fprintf(out_fp,
         "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%.0f\t%" PRIu64 "\t%.2f\t%.2f\t%s\t%s\t%s\t%c%.6lf\t%.6lf\t%s\t%s\n",
            hnode.count, hnode.files, hnode.pages, hnode.xfer/1024.,
            hnode.visits, hnode.visit_avg/60., hnode.visit_max/60.,
            hnode.ccode,
            state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str(),
            hnode.city.c_str(),
            hnode.spammer?'*':hnode.robot?'#':' ', 
            hnode.latitude, hnode.longitude,
            hnode.string.c_str(), hnode.hostname().c_str());
      }
   }
   iter.close();

   fclose(out_fp);
   return;
}

/*********************************************/
/* DUMP_ALL_URLS - dump all urls to tab file */
/*********************************************/

void dump_output_t::dump_all_urls()
{
   storable_t<unode_t> unode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/url_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_xfer, config.lang.msg_h_avgtime, config.lang.msg_h_maxtime,config.lang.msg_h_type,config.lang.msg_h_url);
   }

   /* dump 'em */
   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls("urls.hits");

   while (iter.prev(unode, (unode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      if (unode.flag != OBJ_GRP)
      {
         fprintf(out_fp,"%" PRIu64 "\t%.0f\t%.03f\t%.03f\t%c\t%s\n",
            unode.count,unode.xfer/1024.,
            unode.avgtime, unode.maxtime,
            unode.get_url_type_ind(),
            unode.string.c_str());
      }
   }
   iter.close();

   fclose(out_fp);
   return;
}

/*********************************************/
/* DUMP_ALL_REFS - dump all refs to tab file */
/*********************************************/

void dump_output_t::dump_all_refs()
{
   storable_t<rnode_t> rnode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/ref_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_visits,config.lang.msg_h_ref);
   }

   database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.hits");

   /* dump 'em */
   while(iter.prev(rnode, (rnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      if (rnode.flag != OBJ_GRP)
         fprintf(out_fp,"%" PRIu64 "\t%" PRIu64 "\t%s\n", rnode.count, rnode.visits, rnode.string[0] == '-' ? config.lang.msg_ref_dreq : rnode.string.c_str());
   }
   iter.close();

   fclose(out_fp);

   return;
}

void dump_output_t::dump_all_downloads(void)
{
   storable_t<dlnode_t> dlnode;
   const dlnode_t *nptr;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/dl_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", config.lang.msg_h_hits, config.lang.msg_h_xfer, 
            config.lang.msg_h_time, config.lang.msg_h_time, config.lang.msg_h_count, 
            config.lang.msg_h_download, config.lang.msg_h_ccode, config.lang.msg_h_ctry, config.lang.msg_h_city, 
            config.lang.msg_h_latitude, config.lang.msg_h_longitude,
            config.lang.msg_h_ipaddr, config.lang.msg_h_hname);
   }

   // create a reverse state.database iterator (xfer-ordered)
   database_t::reverse_iterator<dlnode_t> iter = state.database.rbegin_downloads("downloads.xfer");

   /* dump 'em */
   storable_t<hnode_t> hnode;
   while(iter.prev(dlnode, state_t::unpack_dlnode_and_host_cb, const_cast<state_t*>(&state), hnode)) {
      dlnode.set_host(&hnode);

      nptr = &dlnode;

      fprintf(out_fp,"%" PRIu64 "\t%8.02f\t%6.02f\t%6.02f\t%" PRIu64 "\t%s\t%s\t%s\t%s\t%.6lf\t%.6lf\t%s\t%s\n", 
         nptr->sumhits, nptr->sumxfer/1024., 
         nptr->avgtime, nptr->sumtime, 
         nptr->count,
         nptr->string.c_str(),
         hnode.ccode, 
         state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str(),
         hnode.city.c_str(),
         hnode.latitude, hnode.longitude,
         hnode.string.c_str(),
         hnode.hostname().c_str());
   }
   iter.close();
   fclose(out_fp);
   return;
}

void dump_output_t::dump_all_errors(void)
{
   storable_t<rcnode_t> rcnode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/err_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\n",config.lang.msg_h_hits, config.lang.msg_h_status, config.lang.msg_h_method, config.lang.msg_h_url);
   }

   // get top tot_num hit-ordered nodes from the state.database
   database_t::reverse_iterator<rcnode_t> iter = state.database.rbegin_errors("errors.hits");

   /* dump 'em */
   while(iter.prev<>(rcnode, (rcnode_t::s_unpack_cb_t<>) nullptr, nullptr))
   {
      fprintf(out_fp,"%" PRIu64 "\t%d\t%s\t%s\n",rcnode.count, rcnode.respcode, rcnode.method.c_str(), rcnode.string.c_str());
   }

   iter.close();
   fclose(out_fp);
   return;
}

/*********************************************/
/* DUMP_ALL_AGENTS - dump agents htab file   */
/*********************************************/

void dump_output_t::dump_all_agents()
{
   storable_t<anode_t> anode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/agent_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_xfer,config.lang.msg_h_type,config.lang.msg_h_agent);
   }

   database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.hits");

   /* dump 'em */
   while(iter.prev(anode, (anode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      if (anode.flag != OBJ_GRP)
         fprintf(out_fp,"%" PRIu64 "\t%.0f\t%c\t%s\n", anode.count, anode.xfer/1024., anode.robot ? '#' : ' ', anode.string.c_str());
   }
   iter.close();

   fclose(out_fp);
   return;
}

/*********************************************/
/* DUMP_ALL_USERS - dump username tab file   */
/*********************************************/

void dump_output_t::dump_all_users()
{
   storable_t<inode_t> inode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/user_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
         config.lang.msg_h_hits,config.lang.msg_h_files,config.lang.msg_h_xfer,config.lang.msg_h_visits, config.lang.msg_h_avgtime, config.lang.msg_h_maxtime, config.lang.msg_h_uname);
   }

   database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.hits");

   /* dump 'em */
   while(iter.prev(inode, (inode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      if (inode.flag != OBJ_GRP) {
         fprintf(out_fp, "%" PRIu64 "\t%" PRIu64 "\t%.0f\t%" PRIu64 "\t%.3f\t%.3f\t%s\n",
            inode.count, inode.files, inode.xfer/1024.,
            inode.visit, 
            inode.avgtime, inode.maxtime,
            inode.string.c_str());
      }
   }
   iter.close();

   fclose(out_fp);
   return;
}

/*********************************************/
/* DUMP_ALL_SEARCH - dump search htab file   */
/*********************************************/

void dump_output_t::dump_all_search()
{
   storable_t<snode_t> snode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/search_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\n",config.lang.msg_h_hits, config.lang.msg_h_visits, config.lang.msg_h_search);
   }

   database_t::reverse_iterator<snode_t> iter = state.database.rbegin_search("search.hits");

   /* dump 'em */
   while(iter.prev(snode, (snode_t::s_unpack_cb_t<>) nullptr, nullptr))
   {
      fprintf(out_fp,"%" PRIu64 "\t%" PRIu64 "\t%s\n", snode.count, snode.visits, snode.string.c_str());
   }
   iter.close();
   fclose(out_fp);
   return;
}

void dump_output_t::dump_all_cities()
{
   storable_t<ctnode_t> ctnode;
   FILE *out_fp;
   char filename[FILENAME_MAX];

   // generate a file name
   if(snprintf(filename, sizeof(filename), "%s/city_%04d%02d.%s",
         !config.dump_path.isempty() ? config.dump_path.c_str() : config.out_dir.c_str(), 
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, config.dump_ext.c_str()) >= sizeof(filename)) {
      throw std::runtime_error("DumpPath is too long");
   }

   // open the file
   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open, filename);
      return;
   }

   // check if we need a header
   if (config.dump_header) {
      // GeoNameID is not localized on purpose because it's a fixed name for a feature on www.geonames.org
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            config.lang.msg_h_hits, config.lang.msg_h_files, config.lang.msg_h_pages, 
            config.lang.msg_h_xfer, config.lang.msg_h_visits, 
            config.lang.msg_h_ccode, config.lang.msg_h_ctry, 
            "GeoNameID", config.lang.msg_h_city);
   }

   // output rows ordered by visit counts, in descending order
   database_t::reverse_iterator<ctnode_t> iter = state.database.rbegin_cities("cities.visits");

   while(iter.prev(ctnode, (ctnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      fprintf(out_fp,
      "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
      "%.0f\t%" PRIu64 "\t"
      "%s\t%s\t"
      "%" PRIu32 "\t%s\n",
         ctnode.hits, ctnode.files, ctnode.pages, 
         ctnode.xfer/1024., ctnode.visits, 
         ctnode.ccode.c_str(), state.cc_htab.get_ccnode(ctnode.ccode).cdesc.c_str(),
         ctnode.geoname_id(), ctnode.city.c_str());
   }
   iter.close();

   fclose(out_fp);
}

void dump_output_t::dump_all_countries()
{
   storable_t<ccnode_t> ctnode;
   FILE *out_fp;
   char filename[FILENAME_MAX];

   // generate a file name
   if(snprintf(filename, sizeof(filename), "%s/country_%04d%02d.%s",
         !config.dump_path.isempty() ? config.dump_path.c_str() : config.out_dir.c_str(), 
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, config.dump_ext.c_str()) >= sizeof(filename)) {
      throw std::runtime_error("DumpPath is too long");
   }

   // open the file
   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open, filename);
      return;
   }

   // check if we need a header
   if (config.dump_header) {
      // GeoNameID is not localized on purpose because it's a fixed name for a feature on www.geonames.org
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            config.lang.msg_h_hits, config.lang.msg_h_files, config.lang.msg_h_pages, 
            config.lang.msg_h_xfer, config.lang.msg_h_visits, 
            config.lang.msg_h_ccode, config.lang.msg_h_ctry);
   }

   // output rows ordered by visit counts, in descending order
   database_t::reverse_iterator<ccnode_t> iter = state.database.rbegin_countries("countries.visits");

   while(iter.prev(ctnode, (ccnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      fprintf(out_fp,
      "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
      "%.0f\t%" PRIu64 "\t"
      "%s\t%s\n",
         ctnode.count, ctnode.files, ctnode.pages, 
         ctnode.xfer/1024., ctnode.visits, 
         ctnode.ccode.c_str(), state.cc_htab.get_ccnode(ctnode.ccode).cdesc.c_str());
   }
   iter.close();

   fclose(out_fp);
}
