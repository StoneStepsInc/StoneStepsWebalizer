/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   preserve.cpp
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
#include "autoptr.h"
#include "history.h"

state_t::state_t(const config_t& config) : v_ended(128), dl_ended(128), config(config), history(config), database(config), response(config.lang.resp_code_count())
{
   stfile = false;
   buffer = new char[BUFSIZE];
}

state_t::~state_t(void)
{
   cc_htab.clear();
   delete [] buffer;
}

bool state_t::is_state_file(void)
{
   // check if there's an old state file (webalizer.current)
   return !access(make_path(config.out_dir, config.state_fname), F_OK);
}

bool state_t::del_state_file(void)
{
   return is_state_file() ? !unlink(make_path(config.out_dir, config.state_fname)) : true;
}

bool state_t::eval_tnode_cb(const tnode_t *tnode, void *arg)
{
   return tnode ? true : false;
}

bool state_t::swap_tnode_cb(tnode_t *tnode, void *arg)
{
   state_t *_this = (state_t*) arg;

   if(tnode->dirty) {
      if(!_this->database.put_tnode(*tnode))
         throw exception_t(0, "Cannot swap out a daily host node to the database");
   }

   return true;
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

   hash_table<tnode_t>::iterator t_iter;
   hash_table<hnode_t>::iterator h_iter;
   hash_table<unode_t>::iterator u_iter;
   hash_table<rnode_t>::iterator r_iter;
   hash_table<anode_t>::iterator a_iter;
   hash_table<snode_t>::iterator s_iter;
   hash_table<inode_t>::iterator i_iter;
   hash_table<rcnode_t>::iterator rc_iter;
   hash_table<dlnode_t>::iterator dl_iter;
   hash_table<ccnode_t>::iterator cc_iter;

   vector_t<u_long>::iterator iter;
   
   u_int  i;

   string_t ccode, hname;

   vnode_t vnode;
   dlnode_t dlnode;

   /* Saving current run data... */
   if (verbose>1)
   {
      sprintf(buffer,"%02d/%02d/%04d %02d:%02d:%02d",cur_month,cur_day,cur_year,cur_hour,cur_min,cur_sec);
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
   while(iter.next(0)) {
      vnode.reset(*iter);
      if(!database.delete_visit(vnode))
         throw exception_t(0, string_t::_format("Cannot detele an ended visit from the database (ID: %d)", vnode.nodeid));
   }
   v_ended.clear();

   // delete stale active downloads
   iter = dl_ended.begin();
   while(iter.next(0)) {
      dlnode.reset(*iter);
      if(!database.delete_download(dlnode))
         throw exception_t(0, string_t::_format("Cannot detele a finished download job from the database (ID: %d)", dlnode.nodeid));
   }
   dl_ended.clear();

   // save totals
   if(!database.put_tgnode(*this))
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
   // node references:
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
   if (t_ref != 0) {
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
   if (t_agent != 0) {
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
   history.update(cur_year, cur_month, t_hit, t_file, t_page, t_visits, t_hosts, t_xfer/1024., f_day, l_day);
   history.put_history();

   //
   // If there's an old state file, delete it
   //
   if (config.incremental) {
      // delete the old state file, if there is one
      if(stfile) {
         if(!del_state_file()) {
            if(verbose)
               fprintf(stderr, "Cannot delete the state file (%s). Delete the file manually before the next run\n", config.state_fname.c_str());
         }
      }
   }

   return 0;            /* successful, return with good return code      */
}

bool state_t::initialize(void)
{
   u_int index;

   // reset sysnode now that we have configuration available 
   sysnode.reset(config);

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

   //
   // initialize the database
   //
   
   if(config.is_maintenance()) {
      // make sure database exists, so database_t::open doesn't create an empty one
      if(access(config.get_db_path(), F_OK)) {
         if(verbose)
            fprintf(stderr, "%s: %s\n", config.lang.msg_nofile, config.get_db_path().c_str());
         return false;
      }
   }

   // enable trickling if trickle rate is not zero (database mode)
   if(config.db_trickle_rate && !config.memory_mode)
      database.set_trickle(true);

   if(!database.open())
      throw exception_t(0, "Cannot open the database");

   // report which database was opened
   if(verbose > 1)
      printf("%s %s\n", config.lang.msg_use_db, database.get_dbpath().c_str());

   //
   // Check if there is a system node. If there is, check if there's 
   // anything to do, given state of the database and current run 
   // parameters.
   //
   if(database.is_sysnode()) {
      if(!database.get_sysnode_by_id(sysnode, NULL, NULL))
         throw exception_t(0, "Cannot read the system node from the database");
         
      if(!sysnode.check_size_of())
         throw exception_t(0, "Incompatible database format (data type sizes)");

      if(!sysnode.check_byte_order())
         throw exception_t(0, "Incompatible database format (byte order)");

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

   // initialize history
   history.initialize();

   //
   // Restore history, if required. History file may be mising, empty or incomplete
   // at this point. History for the current month will be updated when the state
   // is restored.
   //
   if (config.ignore_hist) {
      if (verbose>1) 
         printf("%s\n",config.lang.msg_ign_hist); 
   }
   else
      history.get_history();

   return true;
}

void state_t::cleanup(void)
{
   //
   // History is expected to be up to date at this point. During a log processing run, 
   // it will be updated when the state is restored and saved. WHen a report is being 
   // generated from the database, the history is updated when the state is restored.
   // Note that the state is gone at this point - don't ue it to update the history.
   //
   history.put_history();
   history.cleanup();

   if(!database.close()) {
      if(verbose)
         fprintf(stderr, "Cannot close the database. The database file may be corrupt\n");
   }
}

/*********************************************/
/* RESTORE_STATE - reload internal run data  */
/*********************************************/

int state_t::restore_state_ex(void)
{
   string_t str;
   u_int i;
   void *fixver;
   vnode_t vnode;
   danode_t danode;
   hnode_t hnode;
   dlnode_t dlnode;
   unode_t unode;
   tnode_t tnode;
   rnode_t rnode;
   anode_t anode;
   snode_t snode;
   inode_t inode;
   rcnode_t rcnode;
   ccnode_t ccnode;
   
   // sysnode is not populated if the databases is new or has been truncated
   if(!sysnode.appver)
      return 0;

   //
   // Prior to v3.3.1.5, daily and hourly nodes lacked record version, but
   // stored correct record size, so it was not possible to determine the
   // version from the size (i.e. if 2 bytes short, bad version). Use the
   // application version in the sysnode and the callback object pointer 
   // to instruct these nodes to fix the version. If the callback is needed
   // a new argument would need to be created to include both, unpacking
   // instructions (i.e. fix version) and new callback data.
   //
   fixver = (sysnode.appver < VERSION_3_3_1_5 && !sysnode.fixed_dhv) ? (void*) -1 : NULL;

   // restore current totals
   if(!database.get_tgnode_by_id(*this, NULL, NULL))
      return 3;
   
   //
   // Recover missing record counts, if necessary
   //   
   if(sysnode.appver < VERSION_3_5_1_1) {
      // if there are search string hits, but no record count, get it from the database
      if(t_srchits && !t_search)
         t_search = database.get_scount();
      
      // if there are downloads, but no record count, get it from the database   
      if(t_dlcount && !t_downloads)
         t_downloads = database.get_dlcount();
      
      // set the group counts
      if(!t_grp_hosts)
         t_grp_hosts = database.get_hcount() - t_hosts;
         
      if(!t_grp_urls)
         t_grp_urls = database.get_ucount() - t_url;
         
      if(!t_grp_users)
         t_grp_users = database.get_icount() - t_user;
         
      if(!t_grp_refs)
         t_grp_refs = database.get_rcount() - t_ref;
         
      if(!t_grp_agents)
         t_grp_agents = database.get_acount() - t_agent;

   }
   
   //
   // Some sequence IDs in v3.8.0.4 and before were drawn from the wrong 
   // sequence tables. Adjust the affected sequences so we can start using
   // them without running into duplicate IDs.
   //
   if(sysnode.appver_last <= VERSION_3_8_0_4) {
      if(!database.fix_v3_8_0_4())
         return 25;
   }

   // get daily totals
   for(i = 0; i < 31; i++) {
      // nodeid has already been set in init_counters
      if(!database.get_tdnode_by_id(t_daily[i], NULL, fixver))
         return 5;
   }

   // get hourly totals
   for(i = 0; i < 24; i++) {
      // nodeid has already been set in init_counters
      if(!database.get_thnode_by_id(t_hourly[i], NULL, fixver))
         return 6;
   }
   
   //
   // Update sysnode to indicate that we have fixed daily/hourly version,
   // so the correct record saved by this version of the code will be 
   // processed correctly. Note that the application version in sysnode
   // indicates the version of the application that created the database
   // and we need another Boolean to indicate that the daily/hourly node
   // versions have already been fixed.
   //
   if(fixver)
      sysnode.fixed_dhv = true;

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
   // Update current history. Current history file may be missing or may
   // have old data, in which case we will recover the data for this month 
   // from the current database file. In the worse cae scenario we just 
   // write same data twice (i.e. the line created from the current state).
   //
   history.update(cur_year, cur_month, t_hit, t_file, t_page, t_visits, t_hosts, t_xfer/1024., f_day, l_day);

   //
   // No need to restore the rest in the report-only mode
   //
   if(config.prep_report)
      return 0;

   //
   // Versions prior 3.4 didn't store timestamps in the host nodes. Use
   // the beginning of the current day as a timestamp for all host nodes
   // in the daily hosts table. All other host nodes will have the last
   // hit time stamp set to zero.
   //
   if(sysnode.appver < VERSION_3_4_1_1) {
      // loop through the current daily hosts table
      database_t::iterator<tnode_t> iter = database.begin_dhosts();
      while(iter.next(tnode)) {
         hnode.string = tnode.string;
         
         // do not use callbacks, just get the node,
         if(!database.get_hnode_by_value(hnode, NULL, NULL))
            return 25;
         
         // fix the last hit time stamp
         if(hnode.tstamp == 0)
            hnode.tstamp = cur_tstamp / 86400 * 86400;
         
         // and put it back into the database
         if(!database.put_hnode(hnode))
            return 25;
      }
      iter.close();
      
      // all done - truncate the daily hosts table
      if(!database.clear_daily_hosts())
         return 25;
   }

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
         put_hnode(hnode);
      }}

      {// restore active download jobs 
      database_t::iterator<danode_t> iter = database.begin_active_downloads();
      while(iter.next(danode)) {
         dlnode.nodeid = danode.nodeid;
         if(!database.get_dlnode_by_id(dlnode, unpack_dlnode_cb, this))
            return 21;
         put_dlnode(dlnode);
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
      if(!put_unode(unode))
         return 10;
      unode.reset();
   }
   iter.close();
   }

   {// monthly hosts (unpack_hnode_cb ignores groups)
   database_t::iterator<hnode_t> iter = database.begin_hosts(NULL);
   while(iter.next(hnode, unpack_hnode_cb, this)) {
      if(!put_hnode(hnode))
         return 8;
      hnode.reset();
   }
   iter.close();
   }

   {// referrers table
   database_t::iterator<rnode_t> iter = database.begin_referrers(NULL);
   while(iter.next(rnode)) {
      if(!put_rnode(rnode))
         return 11;
   }
   iter.close();
   }

   {// User agent list
   database_t::iterator<anode_t> iter = database.begin_agents(NULL);
   while(iter.next(anode)) {
      if(!put_anode(anode))
         return 12;
   }
   iter.close();
   }

   {// Search String list
   database_t::iterator<snode_t> iter = database.begin_search(NULL);
   while(iter.next(snode)) {
      if(!put_snode(snode))
         return 13;
   }
   iter.close();
   }

   {// username list
   database_t::iterator<inode_t> iter = database.begin_users(NULL);
   while(iter.next(inode)) {
      if(!put_inode(inode))
         return 14;
   }
   iter.close();
   }

   {// error list
   database_t::iterator<rcnode_t> iter = database.begin_errors(NULL);
   while(iter.next(rcnode)) {
      if(!put_rcnode(rcnode))
         return 15;
   }
   iter.close();
   }

   {// downloads
   database_t::iterator<dlnode_t> iter = database.begin_downloads(NULL);
   while(iter.next(dlnode, unpack_dlnode_cb, this)) {
      if(!put_dlnode(dlnode))
         return 16;
      dlnode.reset();
   }
   iter.close();
   }

   return 0;
}

//
// read the plain-text state file to restore the state
//
int state_t::restore_state(void)
{
   FILE *fp;
   u_int  i;
   hnode_t t_hnode, *hptr;   /* Temporary hash nodes */
   rnode_t t_rnode;
   anode_t t_anode;
   snode_t t_snode;
   inode_t t_inode;
   unode_t t_unode;
   tnode_t t_tnode;
   rcnode_t t_rcnode;
   dlnode_t t_dlnode;
   vnode_t t_vnode;
   danode_t t_danode;

   char   *cptr;

   u_long filever = 0;
   u_long ul_bogus=0;
   u_int  ui, flag, urltype, spammer;

   string_t str2, str3, ccode, fpath, ipaddr;

   // if there's no old state file, restore from the database
   if((stfile = is_state_file()) == false)
      return restore_state_ex();

   fpath = make_path(config.out_dir, config.state_fname);

   if((fp=fopen(fpath,"r")) == NULL) {
      /* Previous run data not found... */
      if (verbose>1) printf("%s\n", config.lang.msg_no_data);
      return 0;   /* return with ok code */
   }

   /* Reading previous run data... */
   if (verbose>1) printf("%s %s\n", config.lang.msg_get_data, fpath.c_str());

   /* get easy stuff */
   if((fgets(buffer,BUFSIZE,fp)) == NULL)                    // Header record
      return 1;                                              // error exit

   //
   // check if the original state file is being read
   //
   if(!strncmp(buffer, "# Stone Steps Webalizer", 23)) {     // bad magic?
      if((cptr = strchr(buffer, 'v')) != NULL) {
         filever = atoi(++cptr) << 24;
         while(*cptr && *cptr++ != '.');
         filever |= atoi(cptr) << 16;
         while(*cptr && *cptr++ != '.');
         filever |= atoi(cptr) << 8;
         while(*cptr && *cptr++ != '.');
         filever |= atoi(cptr);
      }
   }
   else if(!strncmp(buffer, "# Webalizer V2.01", 17)) 
      filever = VERSION_2_1_10_0;
   else
      return 99;

   /* Get current timestamp */
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      sscanf(buffer,"%d %d %d %d %d %d",
       &cur_year, &cur_month, &cur_day,
       &cur_hour, &cur_min, &cur_sec);
   } else return 2;  /* error exit */

   /* calculate current timestamp (seconds since epoch) */
   cur_tstamp=tstamp_t::mktime(cur_year, cur_month, cur_day, cur_hour, cur_min, cur_sec);

   /* Get monthly totals */
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
		if(filever >= VERSION_2_1_10_16) 
			sscanf(buffer,"%lu %lu %lu %lu %lu %lu %lf %lu %lu %lu %lu %lf %lf %lf %lf %lf %lf",
				 &t_hit, &t_file, &t_hosts, &t_url,
				 &t_ref, &t_agent, &t_xfer, &t_page, &t_visits, &t_user, &t_err,
				 &a_hitptime, &m_hitptime, &a_fileptime, &m_fileptime, &a_pageptime, &m_pageptime);
		else if(filever >= VERSION_2_1_10_7) 
			sscanf(buffer,"%lu %lu %lu %lu %lu %lu %lf %lu %lu %lu %lf %lf %lf %lf %lf %lf",
				 &t_hit, &t_file, &t_hosts, &t_url,
				 &t_ref, &t_agent, &t_xfer, &t_page, &t_visits, &t_user,
				 &a_hitptime, &m_hitptime, &a_fileptime, &m_fileptime, &a_pageptime, &m_pageptime);
		else {
			a_hitptime = m_hitptime = a_fileptime = m_fileptime = a_pageptime = m_pageptime = 0;
			sscanf(buffer,"%lu %lu %lu %lu %lu %lu %lf %lu %lu %lu",
				 &t_hit, &t_file, &t_hosts, &t_url,
				 &t_ref, &t_agent, &t_xfer, &t_page, &t_visits, &t_user);
		}
   } else return 3;  /* error exit */
     
   /* Get daily totals */
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      sscanf(buffer,"%lu %lu %lu %d %d",
       &dt_hosts, &ht_hits, &hm_hit, &f_day, &l_day);
   } else return 4;  /* error exit */

   /* get daily totals */
   for (i=0;i<31;i++)
   {
      if ((fgets(buffer,BUFSIZE,fp)) != NULL)
      {
         sscanf(buffer,"%lu %lu %lf %lu %lu %lu",
          &t_daily[i].tm_hits, &t_daily[i].tm_files,&t_daily[i].tm_xfer, &t_daily[i].tm_hosts, &t_daily[i].tm_pages,
          &t_daily[i].tm_visits);
      } else return 5;  /* error exit */
   }

   /* get hourly totals */
   for (i=0;i<24;i++)
   {
      if ((fgets(buffer,BUFSIZE,fp)) != NULL)
      {
         sscanf(buffer,"%lu %lu %lf %lu",
          &t_hourly[i].th_hits,&t_hourly[i].th_files,&t_hourly[i].th_xfer,&t_hourly[i].th_pages);
      } else return 6;  /* error exit */
   }

   /* get response code totals */
   for(i = 0; i < response.size(); i++) {
      if ((fgets(buffer,BUFSIZE,fp)) != NULL)
         sscanf(buffer,"%lu",&response[i].count);
      else 
         return 7;  /* error exit */
   }

   /* Kludge for V2.01-06 TOTAL_RC off by one bug */
   if (!strncmp(buffer,"# -urls- ",9)) 
      response[response.size()-1].count=0;
   else
   {
      /* now do hash tables */

      //
      // url table 
      //
      if ((fgets(buffer,BUFSIZE,fp)) != NULL) {            /* Table header */
         if (strncmp(buffer,"# -urls- ",9))                /* (url)        */ 
            return 10; 
      }  
      else 
         return 10;   /* error exit */
   }

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_unode.pathlen = url_path_len(buffer, &ui);
      if(t_unode.pathlen == ui)
         t_unode.pathlen--;         // '\n'
      ui--;                         // '\n'

      if(t_unode.pathlen)
         t_unode.string.assign(buffer, ui);
      else
         t_unode.string = '/';

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 10;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 10;  /* error exit */

      t_unode.urltype = URL_TYPE_UNKNOWN;
 
		/* load temporary node data */
		if(filever >= VERSION_2_1_10_7) {
			if(filever >= VERSION_2_1_10_9) {
				sscanf(buffer,"%u %lu %lu %lf %lf %lu %lu %u",
					&flag,&t_unode.count,
					&t_unode.files, &t_unode.xfer, &t_unode.avgtime,
					&t_unode.entry, &t_unode.exit, &urltype);
            t_unode.urltype = urltype;
			}
			else {
				sscanf(buffer,"%u %lu %lu %lf %lf %lu %lu",
					&flag,&t_unode.count,
					&t_unode.files, &t_unode.xfer, &t_unode.avgtime,
					&t_unode.entry, &t_unode.exit);
			}
		} else {
			t_unode.avgtime = 0.0;
			sscanf(buffer,"%u %lu %lu %lf %lu %lu",
				&flag,&t_unode.count,
				&t_unode.files, &t_unode.xfer,
				&t_unode.entry, &t_unode.exit);
		}
      t_unode.flag = (u_char) flag;
      
      t_unode.nodeid = database.get_unode_id();

      /* Good record, insert into hash table */
      if(!put_unode(t_unode))
      {
         if (verbose)
         /* Error adding URL node, skipping ... */
         fprintf(stderr,"%s %s\n", config.lang.msg_nomem_u, t_unode.string.c_str());
      }
   }

   //
   // monthly hosts table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL) {               // Table header
      if (strncmp(buffer,"# -sites- ",10))                 // (monthly)
         return 8; 
   }                                                       
   else 
      return 8;                                            // error exit

   str2.reset();
   ccode.reset();
   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      /* Check for end of table */
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_hnode.string.assign(buffer, strlen(buffer)-1);

      // domain name
		if(filever >= VERSION_2_1_10_7) {
			if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 8;  /* error exit */
         if(buffer[0] == '-')
            str2.reset();
         else
            str2.assign(buffer, strlen(buffer)-1);
		}

      // country code
      if(filever >= VERSION_2_5_0_1) {
			if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 8;
         if(buffer[0] == '-')
            ccode.reset();
         else
            ccode.assign(buffer, strlen(buffer)-1);
      }

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) 
         return 8;                                          /* error exit */
      if (!isdigitex(buffer[0])) 
         return 8;                                          /* error exit */

      t_vnode.reset();

      /* load temporary node data */
      if(filever >= VERSION_3_0_1_1) {
         sscanf(buffer,"%u %lu %lu %lu %lf %lu %lu %lu %lf %lu %u %lu %lu %lu %lf",
            &flag,&t_hnode.count,
            &t_hnode.files, &t_hnode.pages, &t_hnode.xfer,
            &t_hnode.visits,
            &t_vnode.end, &t_vnode.start,
            &t_hnode.visit_avg, &t_hnode.visit_max, &spammer,
            &t_vnode.hits, &t_vnode.files, &t_vnode.pages, &t_vnode.xfer);
      }
      else if(filever >= VERSION_2_5_0_1) {
         sscanf(buffer,"%u %lu %lu %lu %lf %lu %lu %lu %lf %lu %u",
            &flag,&t_hnode.count,
            &t_hnode.files, &t_hnode.pages, &t_hnode.xfer,
            &t_hnode.visits, 
            &t_vnode.end, &t_vnode.start, 
            &t_hnode.visit_avg, &t_hnode.visit_max, &spammer);
      }
      else if(filever >= VERSION_2_1_10_17) {
         sscanf(buffer,"%u %lu %lu %lu %lf %lu %lu %lu %lf %lu",
            &flag,&t_hnode.count,
            &t_hnode.files, &t_hnode.pages, &t_hnode.xfer,
            &t_hnode.visits, 
            &t_vnode.end, &t_vnode.start, 
            &t_hnode.visit_avg, &t_hnode.visit_max);
         spammer = false;
      }
      else {
         sscanf(buffer,"%u %lu %lu %lf %lu %lu",
            &flag,&t_hnode.count,
            &t_hnode.files, &t_hnode.xfer,
            &t_hnode.visits, &t_vnode.end);
         t_vnode.start = t_vnode.end;
         spammer = false;
      }
      t_hnode.flag = (u_char) flag;
      t_hnode.spammer = spammer ? true : false;

      /* get last url */
      if ((fgets(buffer,BUFSIZE,fp)) == NULL) 
         return 8;                                          /* error exit */

      if (buffer[0]=='-') 
         t_vnode.set_lasturl(NULL);
      else
         t_vnode.set_lasturl(find_url(str3.hold(buffer, strlen(buffer)-1)));

      // check the last URL to see if the visit is still active
      if(t_vnode.lasturl)
         t_hnode.set_visit(new vnode_t(t_vnode));
      else
         t_hnode.set_visit(NULL);

      if(spammer)
         put_spnode(t_hnode.string);

      t_hnode.nodeid = database.get_hnode_id();

      /* Good record, insert into hash table */
      if(!put_hnode(t_hnode)) {
         /* Error adding host node (monthly), skipping .... */
         if (verbose) 
            fprintf(stderr,"%s %s\n", config.lang.msg_nomem_mh, t_hnode.string.c_str());
      }
   }

   //
   // daily hosts table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
   { if (strncmp(buffer,"# -sites- ",10)) return 9; }    /* (daily)      */
   else return 9;   /* error exit */

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      /* Check for end of table */
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_tnode.string.assign(buffer, strlen(buffer)-1);

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 9;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 9;  /* error exit */

      /* load temporary node data */
      sscanf(buffer,"%u", &flag);
      t_tnode.flag = (u_char) flag;

      if(filever < VERSION_2_1_10_17) {
         /* get last url */
         if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 9;  /* error exit */
      }

      // find the host node in the monthly hosts hash table
      if((hptr = hm_htab.find_node(hash_ex(0, t_tnode.string), t_tnode.string, OBJ_REG)) == NULL)
         return 9;
      
      // and update the last hit timestamp to the start of the current day
      if(hptr->tstamp == 0)
         hptr->tstamp = cur_tstamp / 86400 * 86400;
   }

   //
   // referrers table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
   { if (strncmp(buffer,"# -referrers- ",14)) return 11; } /* (referrers)*/
   else return 11;   /* error exit */

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_rnode.string.assign(buffer, strlen(buffer)-1);

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 11;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 11;  /* error exit */

      /* load temporary node data */
      sscanf(buffer,"%u %lu",&flag,&t_rnode.count);
      t_rnode.flag = (u_char) flag;

      t_rnode.nodeid = database.get_rnode_id();
      
      /* insert node */
      if(!put_rnode(t_rnode)) {
         if (verbose) fprintf(stderr,"%s %s\n", config.lang.msg_nomem_r, t_rnode.string.c_str());
      }
   }

   //
   // user agents table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
   { if (strncmp(buffer,"# -agents- ",11)) return 12; } /* (agents)*/
   else return 12;   /* error exit */

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_anode.string.assign(buffer, strlen(buffer)-1);

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 12;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 12;  /* error exit */

      /* load temporary node data */
      if(filever >= VERSION_2_1_10_1)
         sscanf(buffer,"%u %lu %ul",&flag, &t_anode.count, &t_anode.visits);
      else
         sscanf(buffer,"%u %lu", &flag, &t_anode.count);
      t_anode.flag = (u_char) flag;

      t_anode.nodeid = database.get_anode_id();

      /* insert node */
      if(!put_anode(t_anode)) {
         if (verbose) fprintf(stderr,"%s %s\n", config.lang.msg_nomem_a, t_anode.string.c_str());
      }
   }

   //
   // search strings table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
   { if (strncmp(buffer,"# -search string",16)) return 13; }  /* (search)*/
   else return 13;   /* error exit */

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_snode.string.assign(buffer, strlen(buffer)-1);

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 13;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 13;  /* error exit */

      /* load temporary node data */
      if(filever >= VERSION_2_3_0_5)
         sscanf(buffer,"%lu %hu",&t_snode.count, &t_snode.termcnt);
      else 
         sscanf(buffer,"%lu",&t_snode.count);

      t_snode.nodeid = database.get_snode_id();

      /* insert node */
      if(!put_snode(t_snode)) {
         if (verbose) fprintf(stderr,"%s %s\n", config.lang.msg_nomem_sc, t_snode.string.c_str());
      }
   }

   //
   // user names table
   //
   if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
   { if (strncmp(buffer,"# -usernames- ",10)) return 14; }
   else return 14;   /* error exit */

   while ((fgets(buffer,BUFSIZE,fp)) != NULL)
   {
      /* Check for end of table */
      if (!strncmp(buffer,"# End Of Table ",15)) break;
      t_inode.string.assign(buffer, strlen(buffer)-1);

      if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 14;  /* error exit */
      if (!isdigitex((int)buffer[0])) return 14;  /* error exit */

      /* load temporary node data */
      sscanf(buffer,"%u %lu %lu %lf %lu %lu",
         &flag,&t_inode.count,
         &t_inode.files, &t_inode.xfer,
         &t_inode.visit, &t_inode.tstamp);
      t_inode.flag = (u_char) flag;

      t_inode.nodeid = database.get_inode_id();
      
      /* Good record, insert into hash table */
      if(!put_inode(t_inode)) {
         if (verbose)
         /* Error adding username node, skipping .... */
         fprintf(stderr,"%s %s\n", config.lang.msg_nomem_i, t_inode.string.c_str());
      }
   }

   //
   // errors
   //
	if(filever >= VERSION_2_1_10_16) {
      /* HTTP errors table */
      if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
      { if (strncmp(buffer,"# -errors- ",10)) return 15; }
      else return 15;   /* error exit */

      t_rcnode.method = "GET";
      while ((fgets(buffer,BUFSIZE,fp)) != NULL)
      {
         /* Check for end of table */
         if (!strncmp(buffer,"# End Of Table ",15)) break;
         t_rcnode.string.assign(buffer, strlen(buffer)-1);

         if(filever >= VERSION_2_1_10_24) {
            if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 15;  /* error exit */
               t_rcnode.method.assign(buffer, strlen(buffer)-1);

         }

         if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 15;  /* error exit */
         if (!isdigitex((int)buffer[0])) return 15;  /* error exit */

         /* load temporary node data */
         sscanf(buffer,"%u %hu %lu", &flag, &t_rcnode.respcode, &t_rcnode.count);
         t_rcnode.flag = (u_char) flag;

         t_rcnode.nodeid = database.get_rcnode_id();

         /* Good record, insert into hash table */
         if (!put_rcnode(t_rcnode))
         {
            if (verbose)
            /* Error adding status code node, skipping .... */
            fprintf(stderr,"%s %s\n", config.lang.msg_nomem_rc, t_rcnode.string.c_str());
         }
      }
   }

   //
   // downloads
   //
   if(filever >= VERSION_2_2_0_1) {
      if ((fgets(buffer,BUFSIZE,fp)) != NULL)               /* Table header */
      { if (strncmp(buffer,"# -downloads- ",10)) return 16; }
      else return 16;   /* error exit */

      while ((fgets(buffer,BUFSIZE,fp)) != NULL) {
         /* Check for end of table */
         if (!strncmp(buffer,"# End Of Table ",15)) break;
            t_dlnode.string.assign(buffer, strlen(buffer)-1);

         if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 16;  /* error exit */
            ipaddr.assign(buffer, strlen(buffer)-1);

         if ((fgets(buffer,BUFSIZE,fp)) == NULL) return 16;  /* error exit */
         if (!isdigitex((int)buffer[0])) return 16;  /* error exit */

         /* load temporary node data */
         sscanf(buffer,"%lu %lu %lu %lu %lu %lu %lf %lf %lf %lf\n", 
           &t_danode.hits, &t_danode.tstamp, &t_danode.proctime, &t_danode.xfer,
           &t_dlnode.count, &t_dlnode.sumhits, 
           &t_dlnode.sumxfer, &t_dlnode.avgxfer, 
           &t_dlnode.avgtime, &t_dlnode.sumtime);

         // use the value of hits as an indicator (zero if no active download job)
         if(t_danode.hits)
            t_dlnode.download = new danode_t(t_danode);
         else
            t_dlnode.download = NULL;

         if((hptr = hm_htab.find_node(ipaddr)) != NULL)
            t_dlnode.set_host(hptr);

         t_dlnode.nodeid = database.get_dlnode_id();
         
         /* Good record, insert into hash table */
         if(!put_dlnode(t_dlnode)) {
            if (verbose)
            // Error download job node, skipping .... 
            fprintf(stderr,"%s %s\n", config.lang.msg_nomem_rc, t_dlnode.string.c_str());
         }

         t_dlnode.string.reset();
         ipaddr.reset();
      }
   }

   fclose(fp);
   return 0;                   /* return with ok code       */
}

/*********************************************/
/* INIT_COUNTERS - prep counters for use     */
/*********************************************/

void state_t::init_counters(void)
{
   u_int i;

   // totals
   totals_t::init_counters();

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

hnode_t *state_t::put_hnode(const hnode_t& tmp)
{
   return hm_htab.put_node(hash_ex(0, tmp.string), new hnode_t(tmp));
}

rnode_t *state_t::put_rnode(const rnode_t& rnode)
{
   return rm_htab.put_node(hash_ex(0, rnode.string), new rnode_t(rnode));
}

unode_t *state_t::put_unode(const unode_t& tmp)
{
   return um_htab.put_node(hash_ex(0, tmp.string), new unode_t(tmp));
}

rcnode_t *state_t::put_rcnode(const rcnode_t& rcnode)
{
   return rc_htab.put_node(hash_ex(hash_ex(hash_num(0, rcnode.respcode), rcnode.method), rcnode.string), new rcnode_t(rcnode));
}

anode_t *state_t::put_anode(const anode_t& anode)
{
   return am_htab.put_node(hash_ex(0, anode.string), new anode_t(anode));
}

snode_t *state_t::put_snode(const snode_t& snode)
{
   return sr_htab.put_node(hash_ex(0, snode.string), new snode_t(snode));
}

inode_t *state_t::put_inode(const inode_t& inode)
{
   return im_htab.put_node(hash_ex(0, inode.string), new inode_t(inode));
}

dlnode_t *state_t::put_dlnode(const dlnode_t& dlnode)
{
   return dl_htab.put_node(hash_ex(hash_ex(0, dlnode.hnode ? dlnode.hnode->string.c_str() : ""), dlnode.string), new dlnode_t(dlnode));
}

spnode_t *state_t::put_spnode(const string_t& host) 
{
   return sp_htab.put_node(hash_ex(0, host), new spnode_t(host));
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
   if(cur_tstamp) {
      if(!database.rollover(cur_tstamp))
         throw exception_t(0, "Cannot roll over the current state database");
      
      // it's a new database - reset the system node
      sysnode.reset(config);
   }

   init_counters();            /* reset monthly counters  */
   del_htabs();                /* clear hash tables       */
   cc_htab.reset();
}

template <typename type_t>
void state_t::update_avg_max(double& avgval, type_t& maxval, type_t value, u_long newcnt) const
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
   if(!ht_hits) return;
      
   daily_t& daily = t_daily[cur_day-1];

   // update the number of hours in the current day
   daily.td_hours++;
         
   // update hourly average/maximum stats
   update_avg_max(daily.h_hits_avg, daily.h_hits_max, ht_hits, daily.td_hours);
   update_avg_max(daily.h_files_avg, daily.h_files_max, ht_files, daily.td_hours);
   update_avg_max(daily.h_pages_avg, daily.h_pages_max, ht_pages, daily.td_hours);
   update_avg_max(daily.h_xfer_avg, daily.h_xfer_max, ht_xfer, daily.td_hours);
   update_avg_max(daily.h_visits_avg, daily.h_visits_max, ht_visits, daily.td_hours);
   update_avg_max(daily.h_hosts_avg, daily.h_hosts_max, ht_hosts, daily.td_hours);

   // update hourly maximum hits
   if (ht_hits > hm_hit) hm_hit = ht_hits;
   
   // reset hourly counters
   ht_hits = ht_files = ht_pages = 0;
   ht_xfer = .0;
   ht_visits = ht_hosts = 0;
}

void state_t::set_tstamp(const tstamp_t& tstamp)
{
   if (cur_year != tstamp.year || cur_month != tstamp.month) {
      cur_month = tstamp.month;
      cur_year  = tstamp.year;
      f_day = l_day = tstamp.day;
   }

   /* adjust last day processed if different */
   if (tstamp.day > l_day) l_day = tstamp.day;

   /* update min/sec stuff */
   if (cur_sec != tstamp.sec) cur_sec = tstamp.sec;
   if (cur_min != tstamp.min) cur_min = tstamp.min;

   /* check for hour change  */
   if (cur_hour != tstamp.hour)
   {
      update_hourly_stats();
      
      /* if yes, init hourly stuff */
      cur_hour = tstamp.hour;
   }

   /* check for day change   */
   if (cur_day != tstamp.day)
   {
      /* if yes, init daily stuff */
      t_daily[cur_day-1].tm_hosts = dt_hosts; 
      dt_hosts = 0;
      cur_day = tstamp.day;
   }

   // update current timestamp
   cur_tstamp = tstamp.mktime();
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
void state_t::unpack_dlnode_cb(dlnode_t& dlnode, u_long hostid, bool active, void *arg)
{
   hnode_t hnode, *hptr;
   autoptr_t<danode_t> daptr;
   state_t *_this = (state_t*) arg;

   // for active downloads, lookup the download job descriptor
   if(active) {
      daptr.set(new danode_t(dlnode.nodeid));

      // read the active download node from the database
      if(!_this->database.get_danode_by_id(*daptr, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the active download job (ID: %d)", dlnode.nodeid));

      dlnode.download = daptr.release();
   }

   // if there's a host ID, restore the host node
   if(hostid) {
      hnode.nodeid = hostid;

      // read the active host node from the database
      if(!_this->database.get_hnode_by_id(hnode, unpack_hnode_cb, _this))
         throw exception_t(0, string_t::_format("Cannot find the host node (ID: %d) for the download job (ID: %d)", hostid, dlnode.nodeid));

      // check if the node is already in the hash table
      if((hptr = _this->hm_htab.find_node(hnode.string)) != NULL)
         dlnode.set_host(hptr);
      else
         dlnode.set_host(_this->put_hnode(hnode));
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
// void unpack_dlnode_const_cb(dlnode_t& dlnode, u_long hostid, bool active, const void *_this)
//
// This, however, would require all node unpack callbacks duplicated. 
// Instead, just make sure unpack_dlnode_const_cb is used for reporting 
// purposes only.
//
void state_t::unpack_dlnode_const_cb(dlnode_t& dlnode, u_long hostid, bool active, void *arg)
{
   autoptr_t<hnode_t> hptr;
   const state_t *_this = (const state_t*) arg;

   if(hostid) {
      hptr.set(new hnode_t);
      hptr->nodeid = hostid;

      // look up the host node in the database
      if(!_this->database.get_hnode_by_id(*hptr, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the host node (ID: %d) for the download job (ID: %d)", hostid, dlnode.nodeid));

       dlnode.set_host(hptr.release());
       dlnode.ownhost = true;
   }
}

void state_t::unpack_vnode_cb(vnode_t& vnode, u_long urlid, void *arg)
{
   unode_t unode, *uptr;
   state_t *_this = (state_t*) arg;

   if(urlid) {
      unode.nodeid = urlid;

      // look up a URL node in the database
      if(!_this->database.get_unode_by_id(unode, NULL, NULL))
         throw exception_t(0, string_t::_format("Cannot find the last URL (ID: %d) of an active visit (ID: %d)", urlid, vnode.nodeid));

      // find the URL node or create a new one
      if((uptr = _this->find_url(unode.string)) != NULL)
         vnode.set_lasturl(uptr);
      else
         vnode.set_lasturl(_this->put_unode(unode));
   }
}

//
// see comment above state_t::unpack_dlnode_const_cb
//
void state_t::unpack_hnode_const_cb(hnode_t& hnode, bool active, void *arg)
{
   state_t *_this = (state_t*) arg;
   autoptr_t<vnode_t> vptr;
   string_t str;

   if(hnode.flag == OBJ_GRP)
      return;

   // read the active visit, if there's one
   if(active) {
      vptr.set(new vnode_t(hnode.nodeid));

      // look up the active visit in the database
      if(!_this->database.get_vnode_by_id(*vptr, unpack_vnode_cb, _this))
         throw exception_t(0, string_t::_format("Cannot find the active visit of a host (ID: %d)", hnode.nodeid));

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
      _this->put_spnode(hnode.string);
}

//
// insteantiate templates
//
template void state_t::update_avg_max<u_long>(double& avgval, u_long& maxval, u_long value, u_long newcnt) const;
template void state_t::update_avg_max<double>(double& avgval, double& maxval, double value, u_long newcnt) const;
