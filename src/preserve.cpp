/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   preserve.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#ifndef _WIN32
#include <unistd.h>                           /* normal stuff             */
#else
#include <io.h>
#endif

/* ensure sys/types */
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "lang.h"
#include "hashtab.h"
#include "preserve.h"
#include "dns_resolv.h"
#include "util.h"
#include "linklist.h"
#include "database.h"
#include "lang.h"
#include "exception.h"
#include "history.h"

#include <memory>

state_t::state_t(const config_t& config) : config(config), history(config), database(config), response(config.lang.resp_code_count())
{
   buffer = new char[BUFSIZE];

   v_ended.reserve(128);
   dl_ended.reserve(128); 
}

state_t::~state_t(void)
{
   // these hash tables cannot be deleted in the declaration order (see save_state)
   dl_htab.clear();
   hm_htab.clear();
   um_htab.clear();

   cc_htab.clear();
   delete [] buffer;
}

bool state_t::eval_hnode_cb(const hnode_t *hnode, void *arg)
{
   // do not swap out node if there are active visits or downloads
   return (!hnode || hnode->visit || hnode->grp_visit || hnode->dlref) ? false : true;
}

bool state_t::swap_hnode_cb(hnode_t *hnode, void *arg)
{
   state_t *_this = (state_t*) arg;

   if(hnode->dirty) {
      if(!_this->database.put_hnode(*hnode))
         throw exception_t(0, "Cannot swap out a monthly host node to the database");
   }

   return true;
}

bool state_t::eval_unode_cb(const unode_t *unode, void *arg)
{
   // do not swap out node is there are visits referring to this node
   return (!unode || unode->vstref) ? false : true;
}

bool state_t::swap_unode_cb(unode_t *unode, void *arg)
{
   state_t *_this = (state_t*) arg;

   if(unode->dirty) {
      if(!_this->database.put_unode(*unode))
         throw exception_t(0, "Cannot swap out a URL node to the database");
   }

   return true;
}

void state_t::swap_out(void)
{
   //
   // monthly hosts
   //
   if(!hm_htab.swap_out())
      throw exception_t(0, "Cannot swap out the monthly hosts table");

   //
   // monthly URLs
   //
   if(!um_htab.swap_out())
      throw exception_t(0, "Cannot swap out the monthly URL table");

}

/*********************************************/
/* SAVE_STATE - save internal data structs   */
/*********************************************/

int state_t::save_state(void)
{
   const hnode_t *hptr;
   const unode_t *uptr;
   const rnode_t *rptr;
   const anode_t *aptr;
   const snode_t *sptr;
   const inode_t *iptr;
   const rcnode_t *rcptr;
   const dlnode_t *dlptr;
   const ccnode_t *ccptr;

   hash_table<hnode_t>::iterator h_iter;
   hash_table<unode_t>::iterator u_iter;
   hash_table<rnode_t>::iterator r_iter;
   hash_table<anode_t>::iterator a_iter;
   hash_table<snode_t>::iterator s_iter;
   hash_table<inode_t>::iterator i_iter;
   hash_table<rcnode_t>::iterator rc_iter;
   hash_table<dlnode_t>::iterator dl_iter;
   hash_table<ccnode_t>::iterator cc_iter;

   std::vector<uint64_t>::iterator iter;
   
   u_int  i;

   string_t ccode, hname;

   vnode_t vnode;
   dlnode_t dlnode;

   /* Saving current run data... */
   if (config.verbose>1)
   {
      sprintf(buffer,"%02d/%02d/%04d %02d:%02d:%02d",totals.cur_tstamp.month,totals.cur_tstamp.day,totals.cur_tstamp.year,totals.cur_tstamp.hour,totals.cur_tstamp.min,totals.cur_tstamp.sec);
      printf("%s [%s]\n", config.lang.msg_put_data,buffer);
   }

   //
   // Application version is immutable and reflects the version of the 
   // application that created the database. Set it only if it's zero.
   //
   if(sysnode.appver == 0)
      sysnode.appver = VERSION;
   
   // always save the current version
   sysnode.appver_last = VERSION;

   // update the runtime part of sysnode only if we processed a log file
   if(!config.is_maintenance()) {
      sysnode.incremental = config.incremental;
      sysnode.batch = config.batch;
   }

   if(!database.put_sysnode(sysnode))
      throw exception_t(0, "Cannot write the system node to the database");

   // delete stale active visits
   iter = v_ended.begin();
   while(iter != v_ended.end()) {
      vnode.reset(*iter++);
      if(!database.delete_visit(vnode))
         throw exception_t(0, string_t::_format("Cannot delete an ended visit from the database (ID: %" PRIu64 ")", vnode.nodeid));
   }
   v_ended.clear();

   // delete stale active downloads
   iter = dl_ended.begin();
   while(iter != dl_ended.end()) {
      dlnode.reset(*iter++);
      if(!database.delete_download(dlnode))
         throw exception_t(0, string_t::_format("Cannot delete a finished download job from the database (ID: %" PRIu64 ")", dlnode.nodeid));
   }
   dl_ended.clear();

   // save totals
   if(!database.put_tgnode(totals))
      return 1;

   /* Monthly (by day) total array */
   for(i = 0; i < 31; i++) {
      if(!database.put_tdnode(t_daily[i]))
         return 1;
   }

   /* Daily (by hour) total array */
   for (i=0;i<24;i++) {
      if(!database.put_thnode(t_hourly[i]))
         return 1;
   }

   /* Response codes */
   for(i = 0; i < response.size(); i++) {
      if(!database.put_scnode(response[i]))
         return 1;
   }

   // country codes
   cc_iter = cc_htab.begin();
   while(cc_iter.next()) {
      ccptr = cc_iter.item();
      // save only those that have any activity
      if(ccptr && ccptr->count) {
         if(!database.put_ccnode(*ccptr))
            return 22;
      }
   }

   /* now we need to save our hash tables */

   //
   // Nodes must be deleted in the left-to-right order, so we do not leave any 
   // dangling references:
   //
   // dlnode_t > hnode_t > vnode_t > unode_t
   //          > danode_t
   //

   // downloads
   dl_iter = dl_htab.begin();
   while(dl_iter.next()) {
      dlptr = dl_iter.item();
      if(dlptr->download && dlptr->download->dirty) {
         // save first to keep referencial integrity in case of an error
         if(!database.put_danode(*dlptr->download))
            return 1;
      }

      if(dlptr->dirty) {
         if(!database.put_dlnode(*dlptr))
            return 1;
      }
   }
   dl_htab.clear();

   // monthly hosts
   h_iter = hm_htab.begin();
   while(h_iter.next()) {
      hptr = h_iter.item();
      if(hptr->visit && hptr->visit->dirty) {
         // save first to keep referencial integrity in case of an error
         if(!database.put_vnode(*hptr->visit))
            return 1;
      }

      if(hptr->dirty) {
         if(!database.put_hnode(*hptr))
            return 1;
      }
   }
   hm_htab.clear();

   /* URL list */
   u_iter = um_htab.begin();
   while(u_iter.next()) {
      uptr = u_iter.item();
      if(uptr->dirty) {
         if(!database.put_unode(*uptr))
            return 1;
      }
   }
   um_htab.clear();

   /* Referrer list */
   if (totals.t_ref != 0) {
      r_iter = rm_htab.begin();
      while(r_iter.next()) {
         rptr = r_iter.item();
         if(rptr->dirty) {
            if(!database.put_rnode(*rptr))
               return 1;
         }
      }
   }
   rm_htab.clear();

   /* User agent list */
   if (totals.t_agent != 0) {
      a_iter = am_htab.begin();
      while(a_iter.next()) {
         aptr = a_iter.item();
         if(aptr->dirty) {
            if(!database.put_anode(*aptr))
               return 1;
         }
      }
   }
   am_htab.clear();

   /* Search String list */
   s_iter = sr_htab.begin();
   while(s_iter.next()) {
      sptr = s_iter.item();
      if(sptr->dirty) {
         if(!database.put_snode(*sptr))
            return 1;
      }
   }
   sr_htab.clear();

   /* username list */
   i_iter = im_htab.begin();
   while(i_iter.next()) {
      iptr = i_iter.item();
      if(iptr->dirty) {
         if(!database.put_inode(*iptr))
            return 1;
      }
   }
   im_htab.clear();

   /* error list */
   rc_iter = rc_htab.begin();
   while(rc_iter.next()) {
      rcptr = rc_iter.item();
      if(rcptr->dirty) {
         if(!database.put_rcnode(*rcptr))
            return 1;
      }
   }
   rc_htab.clear();

   //
   // Update history for the current month. If the history file was missing, 
   // a new one will be created with this data. 
   //
   history.update(totals.cur_tstamp.year, totals.cur_tstamp.month, totals.t_hit, totals.t_file, totals.t_page, totals.t_visits, totals.t_hosts, totals.t_xfer, totals.f_day, totals.l_day);
   history.put_history();

   return 0;            /* successful, return with good return code      */
}

bool state_t::initialize(void)
{
   u_int index;

   // reset sysnode now that we have configuration available 
   sysnode.reset(config);

   //
   // initialize the database
   //
   
   if(config.is_maintenance()) {
      // make sure database exists, so database_t::open doesn't create an empty one
      if(access(config.get_db_path(), F_OK)) {
         if(config.verbose)
            fprintf(stderr, "%s: %s\n", config.lang.msg_nofile, config.get_db_path().c_str());
         return false;
      }
   }
   else {
      // enable trickling if trickle rate is not zero (database mode)
      if(config.db_trickle_rate && !config.memory_mode)
         database.set_trickle(true);
   }

   if(!database.open())
      throw exception_t(0, string_t::_format("Cannot open the database %s", config.get_db_path().c_str()));

   // report which database was opened
   if(config.verbose > 1)
      printf("%s %s\n", config.lang.msg_use_db, config.get_db_path().c_str());

   //
   // If there is a system node, check if we have anything to do, given state of 
   // the database and current run parameters.
   //
   if(database.is_sysnode()) {
      if(!database.get_sysnode_by_id(sysnode, NULL, NULL))
         throw exception_t(0, "Cannot read the system node from the database");

      // cannot read any data if byte order isn't the same
      if(!sysnode.check_byte_order())
         throw exception_t(0, "Incompatible database format (byte order)");

      //
      // Time stamps in the databases prior to v4 were saved without UTC offsets and
      // cannot be interpreted without having to propagate current UTC offset from 
      // the configuration to all the nodes, which would require a significant effort. 
      // Instead, let's just cut off access to old databases here. In addition to this,
      // all data counters in v4 were changed to 64-bit integers. Only continue if we
      // need to read just the sysnode and query the database directly.
      //
      if(sysnode.appver_last < MIN_APP_DB_VERSION && !config.db_info)
         throw exception_t(0, string_t::_format("Cannot open a database with a version prior to v%s", state_t::get_version(MIN_APP_DB_VERSION).c_str()));
         
      if(!sysnode.check_size_of())
         throw exception_t(0, "Incompatible database format (data type sizes)");

      // do not enforce time settings if we just need to print database information
      if(!config.db_info && !sysnode.check_time_settings(config))
         throw exception_t(0, "Incompatible database format (time settings)");

      // nothing to do if just compacting the database or printing information
      if(!config.compact_db && !config.db_info) {
         // attach indexes to generate a report or to end the current month
         if(config.prep_report || config.end_month) {
            // if the last run was in the batch mode, rebuild indexes
            if(!database.attach_indexes(sysnode.batch ? true : false))
               throw exception_t(0, "Cannot activate secondary database indexes");
         }
         else {
            // do not truncate incremental database for a non-incremental run
            if(!config.incremental && sysnode.incremental)
               throw exception_t(0, "Cannot truncate an incremental database for a non-incremental run");

            //
            // Truncate the database for 
            //   a) a non-incremental run or
            //   b) an incremental run following a non-incremental one
            //
            if(!config.incremental || !sysnode.incremental) {
               if(!database.truncate())
                  throw exception_t(0, "Cannot truncate the database\n");
               sysnode.reset(config);
            }
         }
      }
   }

   // no need to initialize the rest if we just need database information
   if(config.db_info)
      return true;

   // fix any issues with the database to make it compatible with the latest version
   if(sysnode.appver_last != VERSION) {
      if(upgrade_database())
         throw exception_t(0, "Cannot upgrade the database to the latest version");
   }

   //
   // Initialize history
   //
   history.initialize();

   //
   // Initialize runtime structures
   //

   // add response codes for which we have localized descriptions
   for(index = 0; index < config.lang.resp_code_count(); index++)
      response.add_status_code(config.lang.get_resp_code_by_index(index).code);

   // add localized country codes and names to the hash table
   for(index = 0; config.lang.ctry[index].desc; index++)
      cc_htab.put_ccnode(config.lang.ctry[index].ccode, config.lang.ctry[index].desc);

   // indicate that hash tables are in sync with the database
   hm_htab.set_cleared(false);
   um_htab.set_cleared(false);
   rm_htab.set_cleared(false);
   am_htab.set_cleared(false);
   sr_htab.set_cleared(false);
   im_htab.set_cleared(false);
   rc_htab.set_cleared(false);
   dl_htab.set_cleared(false);

   // indicate that none of data is swapped out
   hm_htab.set_swapped_out(false);
   um_htab.set_swapped_out(false);
   rm_htab.set_swapped_out(false);
   am_htab.set_swapped_out(false);
   sr_htab.set_swapped_out(false);
   im_htab.set_swapped_out(false);
   rc_htab.set_swapped_out(false);
   dl_htab.set_swapped_out(false);

   // initalize main counters
   init_counters();                      

   // initalize hash tables
   del_htabs();

   if(!config.memory_mode) {
      // set swap-out callbacks
      hm_htab.set_swap_out_cb(eval_hnode_cb, swap_hnode_cb, this);
      um_htab.set_swap_out_cb(eval_unode_cb, swap_unode_cb, this);
   }

   return true;
}

void state_t::cleanup(void)
{
   if(!config.db_info)
      history.cleanup();

   if(!database.close()) {
      if(config.verbose)
         fprintf(stderr, "Cannot close the database. The database file may be corrupt\n");
   }
}

void state_t::database_info(void) const
{
   printf("\n");
   
   printf("Database        : %s\n", config.get_db_path().c_str());
   printf("Created by      : %s\n", state_t::get_version(get_sysnode().appver).c_str());
   printf("Last updated by : %s\n", state_t::get_version(get_sysnode().appver_last).c_str());
   
   // cannot read totals ifrom a database created prior to v4
   if(sysnode.appver_last >= MIN_APP_DB_VERSION) {
      // output the first day of the month and the last timestamp
      printf("First day       : %04d/%02d/%02d\n", totals.cur_tstamp.year, totals.cur_tstamp.month, totals.f_day);
      printf("Log time        : %04d/%02d/%02d %02d:%02d:%02d\n", totals.cur_tstamp.year, totals.cur_tstamp.month, totals.cur_tstamp.day, totals.cur_tstamp.hour, totals.cur_tstamp.min, totals.cur_tstamp.sec);
   }

   // output active visits and downloads
   printf("Active visits   : %" PRIu64 "\n", database.get_vcount());
   printf("Active downloads: %" PRIu64 "\n", database.get_dacount());

   printf("Incremental     : %s\n", get_sysnode().incremental ? "yes" : "no");
   printf("Batch           : %s\n", get_sysnode().batch ? "yes" : "no");

   printf("Local time      : %s\n", get_sysnode().utc_time ? "no" : "yes");

   if(!get_sysnode().utc_time)
      printf("UTC offset      : %d min\n", get_sysnode().utc_offset);

   // output numeric storage sizes and byte order in debug mode
   if(config.debug_mode) {
      printf("Numeric storage : c=%hd s=%hd i=%hd l=%hd d=%hd\n", get_sysnode().sizeof_char, get_sysnode().sizeof_short, get_sysnode().sizeof_int, get_sysnode().sizeof_long, get_sysnode().sizeof_double);
      printf("Byte order      : %02x%02x%02x%02x\n", (u_int)*(u_char*)&get_sysnode().byte_order, (u_int)*((u_char*)&get_sysnode().byte_order+1), (u_int)*((u_char*)&get_sysnode().byte_order+2), (u_int)*((u_char*)&get_sysnode().byte_order+3));
   }

   printf("\n");
}

/*********************************************/
/* RESTORE_STATE - reload internal run data  */
/*********************************************/

int state_t::restore_state(void)
{
   string_t str;
   u_int i;
   vnode_t vnode;
   danode_t danode;
   hnode_t hnode;
   dlnode_t dlnode;
   unode_t unode;
   rnode_t rnode;
   anode_t anode;
   snode_t snode;
   inode_t inode;
   rcnode_t rcnode;
   ccnode_t ccnode;
   
   // sysnode is not populated if the databases is new or has been truncated
   if(!sysnode.appver)
      return 0;

   // cannot read any data nodes from old databases
   if(config.db_info && sysnode.appver_last < MIN_APP_DB_VERSION)
      return 0;

   // restore current totals
   if(!database.get_tgnode_by_id(totals, NULL, NULL))
      return 3;

   // no need to restore the rest if we just need database information
   if(config.db_info)
      return 0;
   
   // get daily totals
   for(i = 0; i < 31; i++) {
      // nodeid has already been set in init_counters
      if(!database.get_tdnode_by_id(t_daily[i], NULL, NULL))
         return 5;
   }

   // get hourly totals
   for(i = 0; i < 24; i++) {
      // nodeid has already been set in init_counters
      if(!database.get_thnode_by_id(t_hourly[i], NULL, NULL))
         return 6;
   }
   
   // get response code totals
   for(i = 0; i < response.size(); i++) {
      // nodeid has already been set in the constructor
      if(!database.get_scnode_by_id(response[i], NULL, NULL))
         return 7;
   }

   // restore country code data
   {database_t::iterator<ccnode_t> iter = database.begin_countries();
   while(iter.next(ccnode)) {
      cc_htab.update_ccnode(ccnode);
   }
   iter.close();
   }

   //
   // Restore history, unless we are told otherwise
   //
   if (config.ignore_hist) {
      if (config.verbose>1) 
         printf("%s\n",config.lang.msg_ign_hist); 
   }
   else
      history.get_history();

   //
   // If the history file is missing or there is no record of this month, update 
   // history from the state database for this month.
   //
   if(!history.find_month(totals.cur_tstamp.year, totals.cur_tstamp.month))
      history.update(totals.cur_tstamp.year, totals.cur_tstamp.month, totals.t_hit, totals.t_file, totals.t_page, totals.t_visits, totals.t_hosts, totals.t_xfer, totals.f_day, totals.l_day);

   //
   // No need to restore the rest in the report-only mode
   //
   if(config.prep_report)
      return 0;

   //
   // In the database mode, just read those nodes that must be in memory
   // or could improve performance if read earlier (e.g. active visits 
   // and downloads).
   //

   if(!config.memory_mode) {
      {// restore active visits and associated hosts
      database_t::iterator<vnode_t> iter = database.begin_visits();
      while(iter.next(vnode)) {
         hnode.nodeid = vnode.nodeid;
         if(!database.get_hnode_by_id(hnode, unpack_hnode_cb, this))
            return 20;
         hm_htab.put_node(new hnode_t(hnode));
         hnode.reset();
      }}

      {// restore active download jobs 
      database_t::iterator<danode_t> iter = database.begin_active_downloads();
      while(iter.next(danode)) {
         dlnode.nodeid = danode.nodeid;
         if(!database.get_dlnode_by_id(dlnode, unpack_dlnode_cb, this))
            return 21;
         dl_htab.put_node(new dlnode_t(dlnode));
         dlnode.reset();
      }}

      // indicate that hash tables contain only some database data
      hm_htab.set_swapped_out(true);
      um_htab.set_swapped_out(true);
      rm_htab.set_swapped_out(true);
      am_htab.set_swapped_out(true);
      sr_htab.set_swapped_out(true);
      im_htab.set_swapped_out(true);
      rc_htab.set_swapped_out(true);
      dl_htab.set_swapped_out(true);

      return 0;
   }

   //
   // In the memory mode, read all nodes into memory
   //

   {// start with URLs, as they may be referenced by visit nodes (see unpack_vnode_cb)
   database_t::iterator<unode_t> iter = database.begin_urls(NULL);
   while(iter.next(unode)) {
      if(!um_htab.put_node(new unode_t(unode)))
         return 10;
      unode.reset();
   }
   iter.close();
   }

   {// monthly hosts (unpack_hnode_cb ignores groups)
   database_t::iterator<hnode_t> iter = database.begin_hosts(NULL);
   while(iter.next(hnode, unpack_hnode_cb, this)) {
      if(!hm_htab.put_node(new hnode_t(hnode)))
         return 8;
      hnode.reset();
   }
   iter.close();
   }

   {// referrers table
   database_t::iterator<rnode_t> iter = database.begin_referrers(NULL);
   while(iter.next(rnode)) {
      if(!rm_htab.put_node(new rnode_t(rnode)))
         return 11;
   }
   iter.close();
   }

   {// User agent list
   database_t::iterator<anode_t> iter = database.begin_agents(NULL);
   while(iter.next(anode)) {
      if(!am_htab.put_node(new anode_t(anode)))
         return 12;
   }
   iter.close();
   }

   {// Search String list
   database_t::iterator<snode_t> iter = database.begin_search(NULL);
   while(iter.next(snode)) {
      if(!sr_htab.put_node(new snode_t(snode)))
         return 13;
   }
   iter.close();
   }

   {// username list
   database_t::iterator<inode_t> iter = database.begin_users(NULL);
   while(iter.next(inode)) {
      if(!im_htab.put_node(new inode_t(inode)))
         return 14;
   }
   iter.close();
   }

   {// error list
   database_t::iterator<rcnode_t> iter = database.begin_errors(NULL);
   while(iter.next(rcnode)) {
      if(!rc_htab.put_node(new rcnode_t(rcnode)))
         return 15;
   }
   iter.close();
   }

   {// downloads
   database_t::iterator<dlnode_t> iter = database.begin_downloads(NULL);
   while(iter.next(dlnode, unpack_dlnode_cb, this)) {
      if(!dl_htab.put_node(new dlnode_t(dlnode)))
         return 16;
      dlnode.reset();
   }
   iter.close();
   }

   return 0;
}

//
// Checks the database version and updates all fields that may need to be changed
// to be compatible with the current application version. Always updates the last
// application version in the system node, even if no other work was done.
//
int state_t::upgrade_database(void)
{
   // sysnode is not populated if the databases is new or has been truncated
   if(!sysnode.appver)
      return 0;
   
   // update the last application version and save sysnode
   sysnode.appver_last = VERSION;

   if(!database.put_sysnode(sysnode))
      throw exception_t(0, "Cannot write the system node to the database");

   return 0;
}

/*********************************************/
/* INIT_COUNTERS - prep counters for use     */
/*********************************************/

void state_t::init_counters(void)
{
   u_int i;

   // totals
   totals.init_counters();

   // status codes
   for (i=0; i < response.size(); i++) 
      response[i].count = 0;

   // daily totals
   for(i = 0; i < sizeof(t_daily)/sizeof(t_daily[0]); i++) 
      t_daily[i].reset(i+1);

   // hourly totals 
   for(i=0; i < 24; i++) 
      t_hourly[i].reset(i);

   // country codes
   cc_htab.reset();
}

/*********************************************/
/* DEL_HTABS - clear out our hash tables     */
/*********************************************/

void state_t::del_htabs()
{
   // Clear out our various hash tables here
   dl_htab.clear();
   hm_htab.clear();
   um_htab.clear();
   rm_htab.clear();
   am_htab.clear();
   sr_htab.clear();
   im_htab.clear();
   rc_htab.clear();
   sp_htab.clear();
}

/*********************************************/
/* CLEAR_MONTH - initalize monthly stuff     */
/*********************************************/

void state_t::clear_month()
{
   // if there's any data in the database, rename the file
   if(!totals.cur_tstamp.null) {
      if(!database.rollover(totals.cur_tstamp))
         throw exception_t(0, "Cannot roll over the current state database");
      
      // it's a new database - reset the system node
      sysnode.reset(config);
   }

   init_counters();            /* reset monthly counters  */
   del_htabs();                /* clear hash tables       */
   cc_htab.reset();
}

template <typename type_t>
void state_t::update_avg_max(double& avgval, type_t& maxval, type_t value, uint64_t newcnt) const
{
   avgval = AVG(avgval, value, newcnt);
   if(value > maxval)
      maxval = value;
}

//
// Increments the number of hours processed in the current day and updates
// hourly stats, such as hourly averages and maximums for this day. This
// method may be called 
//
//    * when a timestamp from the next hour is seen (from set_tstamp)
//    * at the end of the month, before set_tstamp is called
//    * when there is no more data expected (e.g. end of month or the last log).
//
void state_t::update_hourly_stats(void)
{
   //
   // ht_hits indicates whether there is any unprocessed hourly data or
   // not. If there's no data, skip updating hourly stats (e.g. first 
   // hit of the month).
   //
   if(!totals.ht_hits) return;
      
   daily_t& daily = t_daily[totals.cur_tstamp.day-1];

   // update the number of hours in the current day
   daily.td_hours++;
         
   // update hourly average/maximum stats
   update_avg_max(daily.h_hits_avg, daily.h_hits_max, totals.ht_hits, daily.td_hours);
   update_avg_max(daily.h_files_avg, daily.h_files_max, totals.ht_files, daily.td_hours);
   update_avg_max(daily.h_pages_avg, daily.h_pages_max, totals.ht_pages, daily.td_hours);
   update_avg_max(daily.h_xfer_avg, daily.h_xfer_max, totals.ht_xfer, daily.td_hours);
   update_avg_max(daily.h_visits_avg, daily.h_visits_max, totals.ht_visits, daily.td_hours);
   update_avg_max(daily.h_hosts_avg, daily.h_hosts_max, totals.ht_hosts, daily.td_hours);

   // update hourly maximum hits
   if (totals.ht_hits > totals.hm_hit) totals.hm_hit = totals.ht_hits;
   
   // reset hourly counters
   totals.ht_hits = totals.ht_files = totals.ht_pages = 0;
   totals.ht_xfer = 0;
   totals.ht_visits = totals.ht_hosts = 0;
}

void state_t::set_tstamp(const tstamp_t& tstamp)
{
   if (totals.cur_tstamp.year != tstamp.year || totals.cur_tstamp.month != tstamp.month) {
      totals.f_day = totals.l_day = tstamp.day;
   }

   /* adjust last day processed if different */
   if (tstamp.day > totals.l_day) totals.l_day = tstamp.day;

   /* check for hour change  */
   if (totals.cur_tstamp.year != tstamp.year || totals.cur_tstamp.month != tstamp.month ||
         totals.cur_tstamp.day != tstamp.day || totals.cur_tstamp.hour != tstamp.hour)
   {
      update_hourly_stats();
   }

   // update current timestamp
   totals.cur_tstamp = tstamp;
}

/*********************************************/
/* FIND_URL - Find URL in hash table         */
/*********************************************/

unode_t *state_t::find_url(const string_t& url)
{
   return (url.length()) ? um_htab.find_node(url) : NULL;
}

// -----------------------------------------------------------------------
//
// Serialization callbacks
//
// -----------------------------------------------------------------------

//
//
//
void state_t::unpack_dlnode_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *arg)
{
   hnode_t hnode, *hptr;
   std::unique_ptr<danode_t> daptr;
   state_t *_this = (state_t*) arg;

   // for active downloads, lookup the download job descriptor
   if(active) {
      daptr.reset(new danode_t(dlnode.nodeid));

      // read the active download node from the database
      if(!_this->database.get_danode_by_id(*daptr, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the active download job (ID: %" PRIu64 ")", dlnode.nodeid));

      dlnode.download = daptr.release();
   }

   // if there's a host ID, restore the host node
   if(hostid) {
      hnode.nodeid = hostid;

      // read the active host node from the database
      if(!_this->database.get_hnode_by_id(hnode, unpack_hnode_cb, _this))
         throw exception_t(0, string_t::_format("Cannot find the host node (ID: %d) for the download job (ID: %" PRIu64 ")", hostid, dlnode.nodeid));

      // check if the node is already in the hash table
      if((hptr = _this->hm_htab.find_node(hnode.string)) != NULL)
         dlnode.set_host(hptr);
      else
         dlnode.set_host(_this->hm_htab.put_node(new hnode_t(hnode)));
   }
}

//
// unpack_dlnode_const_cb provides a way of loading the host node without
// having to load any of the host's dependencies (e.g. a visit and a URL).
//
// The loaded host node is not inserted into any of the hash tables and
// is owned and will be destroyed by the instance of dlnode_t that is 
// being loaded.
//
// A better declaration for this method would be to take a const pointer:
//
// void unpack_dlnode_const_cb(dlnode_t& dlnode, uint64_t hostid, bool active, const void *_this)
//
// This, however, would require all node unpack callbacks duplicated. 
// Instead, just make sure unpack_dlnode_const_cb is used for reporting 
// purposes only.
//
void state_t::unpack_dlnode_const_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *arg)
{
   std::unique_ptr<hnode_t> hptr;
   const state_t *_this = (const state_t*) arg;

   if(hostid) {
      hptr.reset(new hnode_t);
      hptr->nodeid = hostid;

      // look up the host node in the database
      if(!_this->database.get_hnode_by_id(*hptr, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the host node (ID: %d) for the download job (ID: %" PRIu64 ")", hostid, dlnode.nodeid));

       dlnode.set_host(hptr.release());
       dlnode.ownhost = true;
   }
}

void state_t::unpack_vnode_cb(vnode_t& vnode, uint64_t urlid, void *arg)
{
   unode_t unode, *uptr;
   state_t *_this = (state_t*) arg;

   if(urlid) {
      unode.nodeid = urlid;

      // look up a URL node in the database
      if(!_this->database.get_unode_by_id(unode, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the last URL (ID: %d) of an active visit (ID: %" PRIu64 ")", urlid, vnode.nodeid));

      // find the URL node or create a new one
      if((uptr = _this->find_url(unode.string)) != NULL)
         vnode.set_lasturl(uptr);
      else
         vnode.set_lasturl(_this->um_htab.put_node(new unode_t(unode)));
   }
}

//
// see comment above state_t::unpack_dlnode_const_cb
//
void state_t::unpack_hnode_const_cb(hnode_t& hnode, bool active, void *arg)
{
   state_t *_this = (state_t*) arg;
   std::unique_ptr<vnode_t> vptr;
   string_t str;

   if(hnode.flag == OBJ_GRP)
      return;

   // read the active visit, if there's one
   if(active) {
      vptr.reset(new vnode_t(hnode.nodeid));

      // look up the active visit in the database
      if(!_this->database.get_vnode_by_id(*vptr, unpack_vnode_cb, _this))
         throw exception_t(0, string_t::_format("Cannot find the active visit of a host (ID: %" PRIu64 ")", hnode.nodeid));

      hnode.set_visit(vptr.release());
   }
}

//
// hnode unpack callback
//
void state_t::unpack_hnode_cb(hnode_t& hnode, bool active, void *arg)
{
   state_t *_this = (state_t*) arg;
   string_t str;

   if(hnode.flag == OBJ_GRP)
      return;

   unpack_hnode_const_cb(hnode, active, arg);

   // remember spammers
   if(hnode.spammer)
      _this->sp_htab.put_node(new spnode_t(hnode.string));
}

string_t state_t::get_app_version(void)
{
   return string_t::_format("%u.%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, EDITION_LEVEL, BUILD_NUMBER);
}

string_t state_t::get_version(u_int version)
{
   return string_t::_format("%u.%u.%u.%u", VERSION_MAJOR_EX(version), VERSION_MINOR_EX(version), EDITION_LEVEL_EX(version), BUILD_NUMBER_EX(version));
}

//
// insteantiate templates
//
template void state_t::update_avg_max<uint64_t>(double& avgval, uint64_t& maxval, uint64_t value, uint64_t newcnt) const;
template void state_t::update_avg_max<double>(double& avgval, double& maxval, double value, uint64_t newcnt) const;
