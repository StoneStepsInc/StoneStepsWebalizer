/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   dump_output.cpp
*/
#include "pch.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
#include "util.h"
#include "exception.h"
#include "dump_output.h"

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
      
   return 0;
}

/*********************************************/
/* DUMP_ALL_SITES - dump sites to tab file   */
/*********************************************/

void dump_output_t::dump_all_hosts()
{
   hnode_t hnode;
   FILE     *out_fp;
   char     filename[256];
   string_t ccode;

   /* generate file name */
   sprintf(filename,"%s/site_%04d%02d.%s",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
       config.lang.msg_h_hits,config.lang.msg_h_files,config.lang.msg_h_pages,config.lang.msg_h_xfer,config.lang.msg_h_visits,config.lang.msg_h_duration,config.lang.msg_h_duration,config.lang.msg_h_ctry,config.lang.msg_h_type,config.lang.msg_h_hname,config.lang.msg_h_hname);
   }

   /* dump 'em */
   database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts("hosts.hits");

   while (iter.prev(hnode)) {
      if (hnode.flag != OBJ_GRP)
      {
         fprintf(out_fp,
         "%lu\t%lu\t%lu\t%.0f\t%lu\t%.2f\t%.2f\t%s\t%c\t%s\t%s\n",
            hnode.count, hnode.files, hnode.pages, hnode.xfer/1024.,
            hnode.visits, hnode.visit_avg/60., hnode.visit_max/60.,
            state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str(),
            hnode.spammer?'*':hnode.robot?'#':' ', hnode.string.c_str(), hnode.hostname().c_str());
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
   unode_t  unode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/url_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_xfer, config.lang.msg_h_avgtime, config.lang.msg_h_maxtime,config.lang.msg_h_type,config.lang.msg_h_url);
   }

   /* dump 'em */
   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls("urls.hits");

   while (iter.prev(unode)) {
      if (unode.flag != OBJ_GRP)
      {
         fprintf(out_fp,"%lu\t%.0f\t%.03f\t%.03f\t%c\t%s\n",
            unode.count,unode.xfer/1024.,
            unode.avgtime, unode.maxtime,
            (unode.urltype == URL_TYPE_HTTPS) ? '*' : (unode.urltype == URL_TYPE_MIXED) ? '-' : ' ',
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
   rnode_t  rnode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/ref_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_visits,config.lang.msg_h_ref);
   }

   database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.hits");

   /* dump 'em */
   while(iter.prev(rnode)) {
      if (rnode.flag != OBJ_GRP)
         fprintf(out_fp,"%lu\t%lu\t%s\n", rnode.count, rnode.visits, rnode.string[0] == '-' ? config.lang.msg_ref_dreq : rnode.string.c_str());
   }
   iter.close();

   fclose(out_fp);

   return;
}

void dump_output_t::dump_all_downloads(void)
{
   dlnode_t dlnode;
   const dlnode_t *nptr;
   FILE     *out_fp;
   char     filename[256];
   const char *cdesc;
   string_t str;

   /* generate file name */
   sprintf(filename,"%s/dl_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", config.lang.msg_h_hits, config.lang.msg_h_xfer, config.lang.msg_h_time, config.lang.msg_h_time, config.lang.msg_h_count, config.lang.msg_h_download, config.lang.msg_h_ctry, config.lang.msg_h_hname, config.lang.msg_h_hname);
   }

   // create a reverse state.database iterator (xfer-ordered)
   database_t::reverse_iterator<dlnode_t> iter = state.database.rbegin_downloads("downloads.xfer");

   /* dump 'em */

   // state_t::unpack_dlnode_const_cb does not modify state
   while(iter.prev(dlnode, state_t::unpack_dlnode_const_cb, const_cast<state_t*>(&state))) {
      nptr = &dlnode;

      if(!nptr->hnode)
         throw exception_t(0, string_t::_format("Missing host in a download node (ID: %d)", nptr->nodeid));

      if(!nptr->hnode)
         cdesc = "";
      else
         cdesc = state.cc_htab.get_ccnode(nptr->hnode->get_ccode()).cdesc;

      fprintf(out_fp,"%lu\t%8.02f\t%6.02f\t%6.02f\t%lu\t%s\t%s\t%s\t%s\n", 
         nptr->sumhits, nptr->sumxfer, 
         nptr->avgtime, nptr->sumtime, 
         nptr->count,
         nptr->string.c_str(),
         cdesc,
         nptr->hnode->string.c_str(),
         nptr->hnode->hostname().c_str());
   }
   iter.close();
   fclose(out_fp);
   return;
}

void dump_output_t::dump_all_errors(void)
{
   rcnode_t rcnode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/err_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

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
   while(iter.prev(rcnode))
   {
      fprintf(out_fp,"%lu\t%d\t%s\t%s\n",rcnode.count, rcnode.respcode, rcnode.method.c_str(), rcnode.string.c_str());
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
   anode_t  anode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/agent_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\t%s\n",config.lang.msg_h_hits,config.lang.msg_h_xfer,config.lang.msg_h_type,config.lang.msg_h_agent);
   }

   database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.hits");

   /* dump 'em */
   while(iter.prev(anode)) {
      if (anode.flag != OBJ_GRP)
         fprintf(out_fp,"%lu\t%.0f\t%c\t%s\n", anode.count, anode.xfer/1024., anode.robot ? '#' : ' ', anode.string.c_str());
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
   inode_t  inode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/user_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

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
   while(iter.prev(inode)) {
      if (inode.flag != OBJ_GRP) {
         fprintf(out_fp, "%lu\t%lu\t%.0f\t%lu\t%.3f\t%.3f\t%s\n",
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
   snode_t snode;
   FILE     *out_fp;
   char     filename[256];

   /* generate file name */
   sprintf(filename,"%s/search_%04d%02d.%s",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.cur_year,state.cur_month,config.dump_ext.c_str());

   /* open file */
   if ( (out_fp=open_out_file(filename))==NULL ) return;

   /* need a header? */
   if (config.dump_header)
   {
      fprintf(out_fp,"%s\t%s\t%s\n",config.lang.msg_h_hits, config.lang.msg_h_visits, config.lang.msg_h_search);
   }

   database_t::reverse_iterator<snode_t> iter = state.database.rbegin_search("search.hits");

   /* dump 'em */
   while(iter.prev(snode))
   {
      fprintf(out_fp,"%lu\t%lu\t%s\n", snode.count, snode.visits, snode.string.c_str());
   }
   iter.close();
   fclose(out_fp);
   return;
}

