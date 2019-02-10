/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   preserve.cpp
*/
#include "pch.h"

#ifndef _WIN32
#include <unistd.h>                           /* normal stuff             */
#else
#include <io.h>
#endif

#include "lang.h"
#include "hashtab.h"
#include "preserve.h"
#include "dns_resolv.h"
#include "util_path.h"
#include "util_math.h"
#include "linklist.h"
#include "database.h"
#include "lang.h"
#include "exception.h"
#include "history.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <memory>
#include <algorithm>

state_t::state_t(const config_t& config, end_visit_cb_t end_visit_cb, end_download_cb_t end_download_cb, void *end_cb_arg) : 
   config(config), history(config), database(config),
   end_visit_cb(end_visit_cb), end_download_cb(end_download_cb), end_cb_arg(end_cb_arg)
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
   ct_htab.clear();

   delete [] buffer;
}

///
/// @brief  Returns `true` if the host node can be swapped out and `false`
///         otherwise.
///
bool state_t::eval_hnode_cb(const hnode_t *hnode, void *arg)
{
   // check if there are any active download jobs referencing this host node
   if(hnode->dlref)
      return false;

   // cannot swap out if there are any visit nodes queued up for name grouping
   if(hnode->grp_visit)
      return false;

   // check if the host is still in the DNS resolver pipeline
   if(((state_t*)arg)->config.is_dns_enabled() && !hnode->resolved)
      return false;

   return true;
}

///
/// @brief  Returns `true` if the URL node can be swapped out and `false`
///         otherwise.
///
bool state_t::eval_unode_cb(const unode_t *unode, void *arg)
{
   // cannot swap out if there are any visits referencing this URL node
   return (unode->vstref) == 0;
}

///
/// @brief  Stores a node without any special dependency considerations in
///         the database.
///
template <typename node_t, bool (database_t::*put_node)(const node_t& node, storage_info_t& strg_info)>
void state_t::swap_out_node_cb(storable_t<node_t> *node, void *arg)
{
   state_t *_this = (state_t*) arg;

   if(node->storage_info.dirty) {
      // save the node to the database
      if(!(_this->database.*put_node)(*node, node->storage_info))
         throw exception_t(0, string_t::_format("Cannot store a node to the database (%s)", typeid(node).name()));
   }
}

///
/// @brief  Stores a host node in the database.
///
/// If a host node has an associated visit, this visit node will be detached and
/// factored into the host data.
///
/// @warning   This callback may only be called for hosts whose visits have been
///            inactive for at least visit timeout time, so these visits may be
///            ended without any additional checks because this class does not
///            control log processing logic, including visit timeouts.
///
template <>
void state_t::swap_out_node_cb<hnode_t, &database_t::put_hnode>(storable_t<hnode_t> *hnode, void *arg)
{
   state_t *_this = (state_t*) arg;

   //
   // If a host node was stored in the database with a active visit, but wasn't
   // seen in the new logs until it rolled off the left side of the swap-out range,
   // the host node will not be dirty at this point and we may end up in a bad
   // state with an active visit in the database, but not in memory, unless we end
   // the visit now. 
   //
   if(hnode->visit) {
      // the callback must update storage flags in the host and visit nodes
      delete _this->end_visit_cb(hnode, _this->end_cb_arg);
   }

   //
   // This check may be an overkill because the only non-dirty nodes in the hash
   // table will be those with active visits that haven't been updated recently,
   // which are updated above. All other host nodes would be marked as dirty when
   // they receive new traffic. TODO: consider reporting an error if we find a
   // non-dirty node here. 
   //
   if(hnode->storage_info.dirty) {
      // save the host node to the database
      if(!_this->database.put_hnode(*hnode, hnode->storage_info))
         throw exception_t(0, "Cannot store a host node to the database (hnode)");
   }
}

///
/// @brief  Stores a download node in the database.
///
/// If there is an active download associated with this download job node, it
/// will be detached and factored into the download job data.
///
/// @warning   This callback may only be called for download jobs whose active
///            downloads have been inactive for at least download timeout time,
///            so these downloads may be ended without any additional checks 
///            because this class does not control log processing logic, including
///            download timeouts.
///
template <>
void state_t::swap_out_node_cb<dlnode_t, &database_t::put_dlnode>(storable_t<dlnode_t> *dlnode, void *arg)
{
   state_t *_this = (state_t*) arg;

   // see the comment in the hnode specialization above
   if(dlnode->download) {
      // the callback must update storage flags in the download and active download nodes
      delete _this->end_download_cb(dlnode, _this->end_cb_arg);
   }

   // see the comment in the hnode specialization above
   if(dlnode->storage_info.dirty) {
      // save the download node to the database
      if(!_this->database.put_dlnode(*dlnode, dlnode->storage_info))
         throw exception_t(0, "Cannot store a download node to the database");
   }
}

///
/// @brief  Saves the current monthly state to the database.
///
/// This method should report all errors to the standard error stream and return a 
/// non-zero value to indicate an error. Historically, however, this method did not
/// reporte errors to the standard error stream when it returned a non-zero value. 
/// This eventually should be fixed, but in the meantime all new code in this method 
/// should be written to report errors properly.
///
int state_t::save_state(void)
{
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

   if(!database.put_sysnode(sysnode, sysnode.storage_info)) {
      fprintf(stderr, "Cannot write the system node to the database");
      return 1;
   }

   // delete stale active visits
   iter = v_ended.begin();
   while(iter != v_ended.end()) {
      vnode.reset(*iter++);
      if(!database.delete_visit(vnode)) {
         fprintf(stderr, "Cannot delete an ended visit from the database (ID: %" PRIu64 ")", vnode.nodeid);
         return 1;
      }
   }
   v_ended.clear();

   // delete stale active downloads
   iter = dl_ended.begin();
   while(iter != dl_ended.end()) {
      dlnode.reset(*iter++);
      if(!database.delete_download(dlnode)) {
         fprintf(stderr, "Cannot delete a finished download job from the database (ID: %" PRIu64 ")", dlnode.nodeid);
         return 1;
      }
   }
   dl_ended.clear();

   // save totals
   if(!database.put_tgnode(totals, totals.storage_info))
      return 1;

   /* Monthly (by day) total array */
   for(i = 0; i < 31; i++) {
      if(!database.put_tdnode(t_daily[i], t_daily[i].storage_info))
         return 1;
   }

   /* Daily (by hour) total array */
   for (i=0;i<24;i++) {
      if(!database.put_thnode(t_hourly[i], t_hourly[i].storage_info))
         return 1;
   }

   /* Response codes */
   for(i = 0; i < response.size(); i++) {
      if(!database.put_scnode(response[i], response[i].storage_info))
         return 1;
   }

   // country codes
   hash_table<storable_t<ccnode_t>>::iterator cc_iter = cc_htab.begin();
   while(cc_iter.next()) {
      ccnode_t *ccptr = cc_iter.item();
      // save only those that have any activity
      if(ccptr && ccptr->count) {
         if(!database.put_ccnode(*ccptr, cc_iter.item()->storage_info))
            return 22;
      }
   }

   // cities
   hash_table<storable_t<ctnode_t>>::iterator ct_iter = ct_htab.begin();
   while(ct_iter.next()) {
      ctnode_t *ctnode = ct_iter.item();
      if(!database.put_ctnode(*ctnode, ct_iter.item()->storage_info))
         return 23;
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
   hash_table<storable_t<dlnode_t>>::iterator dl_iter = dl_htab.begin();
   while(dl_iter.next()) {
      storable_t<dlnode_t> *dlptr = dl_iter.item();
      if(dlptr->download && dlptr->download->storage_info.dirty) {
         // save first to keep referencial integrity in case of an error
         if(!database.put_danode(*dlptr->download, dlptr->download->storage_info))
            return 1;
      }

      if(dlptr->storage_info.dirty) {
         if(!database.put_dlnode(*dlptr, dlptr->storage_info))
            return 1;
      }
   }
   dl_htab.clear();

   // monthly hosts
   hash_table<storable_t<hnode_t>>::iterator h_iter = hm_htab.begin();
   while(h_iter.next()) {
      storable_t<hnode_t> *hptr = h_iter.item();
      if(hptr->visit && hptr->visit->storage_info.dirty) {
         // save first to keep referencial integrity in case of an error
         if(!database.put_vnode(*hptr->visit, hptr->visit->storage_info))
            return 1;
      }

      if(hptr->storage_info.dirty) {
         if(!database.put_hnode(*hptr, hptr->storage_info))
            return 1;
      }
   }
   hm_htab.clear();

   /* URL list */
   hash_table<storable_t<unode_t>>::iterator u_iter = um_htab.begin();
   while(u_iter.next()) {
      storable_t<unode_t> *uptr = u_iter.item();
      if(uptr->storage_info.dirty) {
         if(!database.put_unode(*uptr, uptr->storage_info))
            return 1;
      }
   }
   um_htab.clear();

   /* Referrer list */
   if (totals.t_ref != 0) {
      hash_table<storable_t<rnode_t>>::iterator r_iter = rm_htab.begin();
      while(r_iter.next()) {
         storable_t<rnode_t> *rptr = r_iter.item();
         if(rptr->storage_info.dirty) {
            if(!database.put_rnode(*rptr, rptr->storage_info))
               return 1;
         }
      }
   }
   rm_htab.clear();

   /* User agent list */
   if (totals.t_agent != 0) {
      hash_table<storable_t<anode_t>>::iterator a_iter = am_htab.begin();
      while(a_iter.next()) {
         storable_t<anode_t> *aptr = a_iter.item();
         if(aptr->storage_info.dirty) {
            if(!database.put_anode(*aptr, aptr->storage_info))
               return 1;
         }
      }
   }
   am_htab.clear();

   /* Search String list */
   hash_table<storable_t<snode_t>>::iterator s_iter = sr_htab.begin();
   while(s_iter.next()) {
      storable_t<snode_t> *sptr = s_iter.item();
      if(sptr->storage_info.dirty) {
         if(!database.put_snode(*sptr, sptr->storage_info))
            return 1;
      }
   }
   sr_htab.clear();

   /* username list */
   hash_table<storable_t<inode_t>>::iterator i_iter = im_htab.begin();
   while(i_iter.next()) {
      storable_t<inode_t> *iptr = i_iter.item();
      if(iptr->storage_info.dirty) {
         if(!database.put_inode(*iptr, iptr->storage_info))
            return 1;
      }
   }
   im_htab.clear();

   /* error list */
   hash_table<storable_t<rcnode_t>>::iterator rc_iter = rc_htab.begin();
   while(rc_iter.next()) {
      storable_t<rcnode_t> *rcptr = rc_iter.item();
      if(rcptr->storage_info.dirty) {
         if(!database.put_rcnode(*rcptr, rcptr->storage_info))
            return 1;
      }
   }
   rc_htab.clear();

   //
   // Update history for the current month. If the history file was missing, 
   // a new one will be created with this data. 
   //
   history.update(totals.cur_tstamp.year, totals.cur_tstamp.month, totals.t_hit, totals.t_file, totals.t_page, totals.t_visits, totals.t_hosts, totals.t_xfer, totals.f_day, totals.l_day);

   // put_history will report the error
   if(!history.put_history())
      return 1;

   return 0;            /* successful, return with good return code      */
}

///
/// @brief  Initializes the montly state database.
///
/// This method reports all errors to the stderr stream and return `false` if the 
/// monthly state database could not be initialized.
///
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
         fprintf(stderr, "%s: %s\n", config.lang.msg_nofile, config.get_db_path().c_str());
         return false;
      }
   }

   database_t::status_t status;
   system_database_t sysdb(config);

   if(!(status = sysdb.open()).success()) {
      if(status.err_num() != ENOENT) {
         fprintf(stderr, "Cannot open the sysdb %s (%s)", config.get_db_path().c_str(), status.err_msg().c_str());
         return false;
      }
   }
   else {
      //
      // If there is a system node, check if we have anything to do, given state of 
      // the database and current run parameters.
      //
      if(sysdb.is_sysnode()) {
         if(!sysdb.get_sysnode_by_id(sysnode, NULL, NULL))
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

         // upgrade older databases to make them compatible with the latest version
         if(sysnode.appver && sysnode.appver_last != VERSION)
            upgrade_database(sysnode, sysdb);

         // make sure the database base is closed properly to ensure schema upgrades
         if(!(status = sysdb.close()).success()) {
            fprintf(stderr, "Cannot close the database %s (%s)", config.get_db_path().c_str(), status.err_msg().c_str());
            return false;
         }
      }
   }

   // set up background writing for log processing
   if(!config.is_maintenance()) {
      // enable trickling for log processing
      database.set_trickle(true);
   }

   // open the full state database (sysnode is already up to date)
   if(!(status = database.open()).success()) {
      fprintf(stderr, "Cannot open the database %s (%s)", config.get_db_path().c_str(), status.err_msg().c_str());
      return false;
   }

   // report which database was opened
   if(config.verbose > 1)
      printf("%s %s\n", config.lang.msg_use_db, config.get_db_path().c_str());

   // no need to initialize the rest if we just need database information
   if(config.db_info)
      return true;

   // nothing to do if just compacting the database or printing information
   if(!config.compact_db && !config.db_info) {
      // attach indexes to generate a report or to end the current month
      if(config.prep_report || config.end_month) {
         // if the last run was in the batch mode, rebuild indexes
         if(!(status = database.attach_indexes(sysnode.batch ? true : false)).success())
            throw exception_t(0, string_t::_format("Cannot activate secondary database indexes (%s)", status.err_msg().c_str()));
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
            database_t::status_t status;
            if(!(status = database.truncate()).success())
               throw exception_t(0, string_t::_format("Cannot truncate the database (%s)\n", status.err_msg().c_str()));
            sysnode.reset(config);
         }
      }
   }

   //
   // Initialize history
   //
   history.initialize();

   //
   // Initialize runtime structures
   //

   // add response codes for which we have localized descriptions
   for(index = 0; index < config.lang.response.size(); index++)
      response.add_status_code(config.lang.response[index].code);

   // all country nodes are always kept in memory and are never swapped out
   for(index = 0; index < (u_int) config.lang.ctry.size(); index++)
      cc_htab.put_ccnode(config.lang.ctry[index].ccode, config.lang.ctry[index].desc, 0);

   // initalize main counters
   init_counters();                      

   // initalize hash tables
   del_htabs();

   dl_htab.set_swap_out_cb(&swap_out_node_cb<dlnode_t, &database_t::put_dlnode>, this);
   hm_htab.set_swap_out_cb(&swap_out_node_cb<hnode_t, &database_t::put_hnode>, this, eval_hnode_cb);
   um_htab.set_swap_out_cb(&swap_out_node_cb<unode_t, &database_t::put_unode>, this, eval_unode_cb);
   rm_htab.set_swap_out_cb(&swap_out_node_cb<rnode_t, &database_t::put_rnode>, this);
   am_htab.set_swap_out_cb(&swap_out_node_cb<anode_t, &database_t::put_anode>, this);
   sr_htab.set_swap_out_cb(&swap_out_node_cb<snode_t, &database_t::put_snode>, this);
   im_htab.set_swap_out_cb(&swap_out_node_cb<inode_t, &database_t::put_inode>, this);

   return true;
}

void state_t::cleanup(void)
{
   if(!config.db_info)
      history.cleanup();

   if(!database.close().success()) {
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
      printf("Numeric storage : c=%hd s=%hd i=%hd l=%hd ll=%hd d=%hd\n", get_sysnode().sizeof_char, get_sysnode().sizeof_short, get_sysnode().sizeof_int, get_sysnode().sizeof_long, get_sysnode().sizeof_longlong, get_sysnode().sizeof_double);
      printf("Byte order      : %02X%02X%02X%02X\n", 
                        (u_int)*(u_char*)&get_sysnode().byte_order, (u_int)*((u_char*)&get_sysnode().byte_order+1), (u_int)*((u_char*)&get_sysnode().byte_order+2), (u_int)*((u_char*)&get_sysnode().byte_order+3));
      printf("Byte order x64  : %02X%02X%02X%02X%02X%02X%02X%02X\n", 
                        (u_int)*(u_char*)&get_sysnode().byte_order_x64, (u_int)*((u_char*)&get_sysnode().byte_order_x64+1), (u_int)*((u_char*)&get_sysnode().byte_order_x64+2), (u_int)*((u_char*)&get_sysnode().byte_order_x64+3),
                        (u_int)*((u_char*)&get_sysnode().byte_order_x64+4), (u_int)*((u_char*)&get_sysnode().byte_order_x64+5), (u_int)*((u_char*)&get_sysnode().byte_order_x64+6), (u_int)*((u_char*)&get_sysnode().byte_order_x64+7));
   }

   printf("\n");
}

///
/// @brief  Restores the monthly database to the last run state.
///
/// This method reports errors to the stderr stream and returns a non-zero value if 
/// the monthly state could not be initialized. Unlike other initialization methods, 
/// this method may just return a non-zero value other than `1` if some particular 
/// state value could not be initialized and the caller will report the returned value 
/// to the standard error stream within some error message. The return value `1` 
/// indicates a generic failure and must be accompanied by an error message reported 
/// to the standar error stream. 
///
int state_t::restore_state(void)
{
   u_int i;

   // restore history, unless we are told otherwise
   if(!config.ignore_hist) {
      // the error is reported inside get_history, so just return
      if(!history.get_history())
         return 1;
   }
   else {
      if(config.verbose>1) 
         printf("%s\n",config.lang.msg_ign_hist);
   }

   // sysnode is not populated if the databases is new or has been truncated
   if(!sysnode.appver)
      return 0;

   // cannot read any data nodes from old databases
   if(config.db_info && sysnode.appver_last < MIN_APP_DB_VERSION)
      return 0;

   // restore current totals
   if(!database.get_tgnode_by_id(totals, NULL, NULL))
      return 3;

   // keep the serial time stamp to avoid doing math in every put_node call
   int64_t htab_tstamp = totals.cur_tstamp.mktime();

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
   {database_t::iterator<ccnode_t> iter = database.begin_countries("countries");
   storable_t<ccnode_t> ccnode;
   while(iter.next(ccnode, (ccnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      cc_htab.update_ccnode(ccnode, 0);
   }
   iter.close();
   }

   // restore city data
   {database_t::iterator<ctnode_t> iter = database.begin_cities("cities");
   storable_t<ctnode_t> ccnode;
   while(iter.next(ccnode, (ctnode_t::s_unpack_cb_t<>) nullptr, nullptr)) {
      ct_htab.put_node(new storable_t<ctnode_t>(std::move(ccnode)), 0);
   }
   iter.close();
   }

   //
   // If there is no record of the current month, set the initial history using 
   // values from the state database.
   //
   if(!history.find_month(totals.cur_tstamp.year, totals.cur_tstamp.month))
      history.update(totals.cur_tstamp.year, totals.cur_tstamp.month, totals.t_hit, totals.t_file, totals.t_page, totals.t_visits, totals.t_hosts, totals.t_xfer, totals.f_day, totals.l_day);

   //
   // No need to restore the rest in the report-only mode
   //
   if(config.prep_report)
      return 0;

   {// restore active visits and the associated host and URL nodes
   storable_t<vnode_t> vnode;
   database_t::iterator<vnode_t> iter = database.begin_visits();
   storable_t<hnode_t> hnode;
   storable_t<unode_t> unode, *uptr;
   hnode_t *hptr;
   while(iter.next(vnode, unpack_vnode_cb, this, hnode, unode)) {
      // check if the visit has a reference to the last visited URL
      if(unode.nodeid) {
         //
         // The visit doesn't own the URL node, so we need to find a URL node in the 
         // hash table or insert a new one, so it's deleted properly. 
         //
         if((uptr = um_htab.find_node(unode.string, OBJ_REG, htab_tstamp)) != NULL)
            vnode.set_lasturl(uptr);
         else
            vnode.set_lasturl(um_htab.put_node(new storable_t<unode_t>(std::move(unode)), htab_tstamp));
      }

      hnode.set_visit(new storable_t<vnode_t>(std::move(vnode)));

      // remember spammers
      if(hnode.spammer)
         sp_htab.insert(hnode.string);

      // now we can move the host node into the new instance in the hash table
      hptr = hm_htab.put_node(new storable_t<hnode_t>(std::move(hnode)), htab_tstamp);

      hnode.reset();
      unode.reset();
      vnode.reset();
   }}

   {// restore active download jobs and the associated host and download nodes
   database_t::iterator<danode_t> iter = database.begin_active_downloads();
   storable_t<danode_t> danode;
   storable_t<dlnode_t> dlnode;
   storable_t<hnode_t> hnode;
   hnode_t *hptr;
   while(iter.next(danode, unpack_danode_cb, this, dlnode, hnode)) {
      // the host must be in the hosts table because active downloads are a subset of active visits
      if((hptr = hm_htab.find_node(hnode.string, OBJ_REG, htab_tstamp)) == nullptr)
         throw std::runtime_error(string_t::_format("Cannot find the host (%s) for the download (ID: %" PRIu64 ")", hnode.string.c_str(), dlnode.nodeid));

      // make a copy of the active download and link it to the kdownload node
      dlnode.download = new storable_t<danode_t>(danode);

      // associate the download with the host node
      dlnode.set_host(hptr);

      // finish up and insert the download node into the hash table
      dl_htab.put_node(new storable_t<dlnode_t>(std::move(dlnode)), htab_tstamp);

      dlnode.reset();
      danode.reset();
      hnode.reset();
   }}

   return 0;
}

///
/// @brief  Makes the state database schema compatible with the current application
///         version.
///
/// This method deals with all state database schema compatibility issues, which can
/// be applied only if database tables that are being renamed, copied, etc, are not
/// opened.
///
/// The system database passed into this method must be the most derived class to
/// ensure that no tables except the system table are opened.
///
void state_t::upgrade_database(storable_t<sysnode_t>& sysnode, system_database_t& sysdb)
{
   // make sure this method is called in the right context
   if(!sysnode.appver)
      throw std::logic_error("Cannot upgrade a new or truncated database");
   
   // schema upgrades go here...

   // update the last application version and save sysnode
   sysnode.appver_last = VERSION;

   if(!sysdb.put_sysnode(sysnode, sysnode.storage_info))
      throw exception_t(0, "Cannot write the system node to the database");
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
   database_t::status_t status;

   // if there's any data in the database, rename the file
   if(!totals.cur_tstamp.null) {
      if(!(status = database.rollover(totals.cur_tstamp)).success())
         throw exception_t(0, string_t::_format("Cannot roll over the current state database (%s)", status.err_msg().c_str()));
      
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

///
/// @brief  Stores all nodes with last access time stamps that are less than or
///         equal to `tstamp` in the database, up to `maxmem` in hash table memory
///         size.
///
void state_t::swap_out(int64_t tstamp, size_t maxmem)
{
   //
   // Use 2/3 of the memory for the large hash tables and 1/3 for small ones.
   // If the resulting size is zero, then it will be ignored by the swap_out
   // call.
   //
   size_t lg_htab_sz = (maxmem * 2) / 9;     // ((x / 3) * 2) / 3
   size_t sm_htab_sz = (maxmem * 2) / 12;    // ((x / 3) * 2) / 4

   //
   // The order of swap_out calls is important because nodes may reference one 
   // another. These are current dependencies:
   //
   //    dlnode_t > hnode_t > vnode_t > unode_t
   //             > danode_t
   //
   dl_htab.swap_out(tstamp, lg_htab_sz);
   hm_htab.swap_out(tstamp, lg_htab_sz);
   um_htab.swap_out(tstamp, lg_htab_sz);

   // these nodes currently do not reference other nodes
   rm_htab.swap_out(tstamp, sm_htab_sz);
   am_htab.swap_out(tstamp, sm_htab_sz);
   sr_htab.swap_out(tstamp, sm_htab_sz);
   im_htab.swap_out(tstamp, sm_htab_sz);
}

// -----------------------------------------------------------------------
//
// Serialization callbacks
//
// -----------------------------------------------------------------------

///
/// This method does not read anything from the database and just ensures that the cached 
/// host matches the download node read from the database.
///
/// The method expects `hnode` come from the host hash table and not have an associated 
/// active download because active downloads are read into memory when the `state_t` 
/// instance is initialized and are not swapped out during log processing while downloads 
/// are in progress, so this method is only called when an existing inactive download node 
/// is loaded from the database and a new active download node will be created after this 
/// method returns. Based on this, the method will enforce that the `active` argument is 
/// never `true`.
///
void state_t::unpack_dlnode_cached_host_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *arg, const storable_t<hnode_t>& hnode)
{
   const state_t *_this = (const state_t*) arg;

   // a download node must have a valid host node ID
   if(!hostid)
      throw std::runtime_error(string_t::_format("Invalid host node for the download (ID: %" PRIu64 ")", dlnode.nodeid));

   // the host ID in the host node must match the host ID in the download node from the database
   if(hnode.nodeid != hostid)
      throw std::runtime_error(string_t::_format("Supplied host node (ID:%" PRIu64 ") doesn't match the reference in the download node (ID: %PRIu64)", hostid, dlnode.nodeid));

   // this method is never called for active downloads
   if(active)
      throw std::runtime_error(string_t::_format("Invalid active download state for the download (ID: %" PRIu64 ")", dlnode.nodeid));
}

///
/// This method is intended for callers that need just the host node associated with the
/// download node being loaded, regardless whether there is an active visit or not, such
/// as report writers and `state_t` initialization code that needs just the IP address.
///
/// If the host associated with this download has an active visit or an active download,
/// neither will be read from the database.
///
/// The host node is not associated with the download node in this call. This ensures 
/// that there are no dangling pointers because this methods cannot make any assumptions 
/// about how node arguments are defined.
///
void state_t::unpack_dlnode_and_host_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *arg, storable_t<hnode_t>& hnode)
{
   const state_t *_this = (const state_t*) arg;

   // a download node must have a valid host node ID
   if(!hostid)
      throw std::runtime_error(string_t::_format("Invalid host node for the download (ID: %" PRIu64 ")", dlnode.nodeid));

   // look up the host node in the database and ignore a possible active visit
   hnode.nodeid = hostid;
   if(!_this->database.get_hnode_by_id(hnode, (hnode_t::s_unpack_cb_t<>) nullptr, nullptr))
      throw std::runtime_error(string_t::_format("Cannot find the host node (ID: %" PRIu64 ") for the download (ID: %" PRIu64 ")", hostid, dlnode.nodeid));
}

///
/// This method is intended for loading downloads and their associated hosts during 
/// `state_t` initialization using active downloads as input.
///
/// All host nodes loaded by this method must already be in the host hash table because
/// active visits and their associated hosts are loaded before downloads and a download 
/// can be active only within an active visit. Consequently, only the host IP address 
/// from `hnode` is needed, so the host can be found in the host hash table after this
/// call returns, along with the active visit and the associated URL.
///
/// Neither of the node arguments is linked to one another. See `unpack_dlnode_and_host_cb`.
///
void state_t::unpack_danode_cb(danode_t& danode, void *arg, storable_t<dlnode_t>& dlnode, storable_t<hnode_t>& hnode)
{
   std::unique_ptr<storable_t<dlnode_t>> dlptr;
   const state_t *_this = (const state_t*) arg;

   // read the download and the host from the database
   dlnode.nodeid = danode.nodeid;
   if(!_this->database.get_dlnode_by_id(dlnode, unpack_dlnode_and_host_cb, arg, hnode))
      throw std::runtime_error(string_t::_format("Cannot find the download node (ID: %" PRIu64 ")", dlnode.nodeid));
}

///
/// This method reads from the database a host and a URL node associated with the active 
/// visit identified by `vnode` and is intended to be used during `state_t` initialization, 
/// when active visits are being loaded from the database.
///
/// Neither of the node arguments is linked to one another. See `unpack_dlnode_and_host_cb`.
///
void state_t::unpack_vnode_cb(vnode_t& vnode, uint64_t urlid, void *arg, storable_t<hnode_t>& hnode, storable_t<unode_t>& unode)
{
   const state_t *_this = (state_t*) arg;

   if(urlid) {
      unode.nodeid = urlid;

      // look up a URL node in the database
      if(!_this->database.get_unode_by_id(unode, nullptr, nullptr))
         throw std::runtime_error(string_t::_format("Cannot find the last URL (ID: %d) of an active visit (ID: %" PRIu64 ")", urlid, vnode.nodeid));
   }

   hnode.nodeid = vnode.nodeid;
   if(!_this->database.get_hnode_by_id(hnode, unpack_active_hnode_cb, arg))
      throw std::runtime_error(string_t::_format("Cannot find the host node (ID: %" PRIu64 ") for visit (ID: %" PRIu64 ")", hnode.nodeid, vnode.nodeid));
}

///
/// This method does not read anything from the database and just ensures that there is 
/// an active visit associated with the host being loaded, but the caller will verify on 
/// its own that the visit node matches the host node. 
///
/// This method is intended ro be used during `state_t` initialization, when active 
/// downloads are being loaded.
///
void state_t::unpack_active_hnode_cb(hnode_t& hnode, bool active, void *arg)
{
   // group host nodes cannot have active visits
   if(hnode.flag == OBJ_GRP && active)
      throw std::runtime_error(string_t::_format("A group host node (ID: %" PRIu64 ") cannot have an active visit", hnode.nodeid));

   // this callback is only called for hosts with active visits
   if(!active)
      throw std::runtime_error(string_t::_format("A host node (ID: %" PRIu64 ") has no indicator of an active visit", hnode.nodeid));
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
