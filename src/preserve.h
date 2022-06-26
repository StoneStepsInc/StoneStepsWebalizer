/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   preserve.h
*/

#ifndef PRESERVE_H
#define PRESERVE_H

#include "types.h"
#include "scnode.h"
#include "daily.h"
#include "hourly.h"
#include "tstring.h"
#include "history.h"
#include "database.h"
#include "hashtab_nodes.h"
#include "storable.h"

#include <vector>
#include <unordered_set>

class config_t;
class lang_t;

///
/// @brief  Keeps the entire processing state for the current month
///
/// The state consists of all time totals (total hits, files, etc), daily and hourly 
/// totals in the current month and lookup hash tables contaning totals for individual 
/// items, such as URLs or hosts. The state is stored in the database between runs.
///
class state_t {
   public:
      typedef storable_t<vnode_t> *(*end_visit_cb_t)(storable_t<hnode_t> *hnode, void *arg);
      typedef storable_t<danode_t> *(*end_download_cb_t)(storable_t<dlnode_t> *dlnode, void *arg);

   public:
      storable_t<totals_t> totals;

      storable_t<daily_t> t_daily[31];          // daily totals

      storable_t<hourly_t> t_hourly[24];        // hourly totals

      sc_table_t response;                      // HTTP status codes

      ///
      /// @name   Monthly state hash tables
      ///
      /// These hash tables serve as a cache between Berkeley DB and
      /// the log analyzer. Each value from a log file is looked up
      /// first in the database and then is inserted into one of the
      /// hash tables, which is either the one found in the database
      /// or the new one created from the log item value.
      /// 
      /// Each hash table is saved to the database either at the end
      /// of the run or, for select hash tables, when they become too
      /// large to be kept in memory. The latter is performed via the
      /// swap-out interface.
      /// 
      /// When the state is being initialized, only data items that
      /// are likely to be referenced are loaded from the database,
      /// such as hosts and URLs associated with active visits.
      /// 
      /// Historically, these hash tables were loaded from the state
      /// file and contained all entries for the current month, which
      /// were traversed by report generators to output hosts, URLs
      /// and other items. Because of this pattern, the state was
      /// saved first in the state file, then reports were generated,
      /// and then hash tables were cleared before the next month
      /// could be processed. All current report generators traverse
      /// state database tables instead and most hash tables may be
      /// cleared when the state is being saved to the Berkeley DB
      /// database. Country, city and ASN hash tables are still being
      /// accessed during report generation and can only be cleared
      /// after reports have been generated.
      /// 
      /// @{

      h_hash_table hm_htab;                      ///< A hash table for hosts.
      u_hash_table um_htab;                      ///< A hash table for URLS.
      r_hash_table rm_htab;                      ///< A hash table for referrers.
      a_hash_table am_htab;                      ///< A hash table for user agents.
      s_hash_table sr_htab;                      ///< A hash table for search string table.
      i_hash_table im_htab;                      ///< A hash table for user names.
      rc_hash_table rc_htab;                     ///< A hash table for HTTP status codes.
      dl_hash_table dl_htab;                     ///< A hash table for active download jobs.

      cc_hash_table cc_htab;                     ///< A hash table for countries.
      ct_hash_table ct_htab;                     ///< A hash table for cities.

      as_hash_table as_htab;                     ///< A hash table for ASN data (autonomous system numbers).

      /// @}

      std::unordered_set<string_t, hash_string> sp_htab; ///< Spammer hosts

      std::vector<hash_table_base*> cleared_htabs; ///< A vector or hash tables to clear on month switch (order is important).

      std::vector<uint64_t> v_ended;             // ended active visit node IDs
      std::vector<uint64_t> dl_ended;            // ended active download node IDs

      history_t   history;
      database_t  database;

   private:
      const config_t&   config;

      char              *buffer;

      storable_t<sysnode_t> sysnode;

      end_visit_cb_t       end_visit_cb;
      end_download_cb_t    end_download_cb;
      void                 *end_cb_arg;

   private:
      template <typename type_t>
      void update_avg_max(double& avg, type_t& max, type_t value, uint64_t newcnt) const;

      void init_counters(void);

      /// Closes and renamed the current database file and opens a new empty one.
      void rollover_database(const tstamp_t& tstamp);

      ///
      /// @name   Serialization callbacks
      ///
      /// @{

      static void unpack_danode_cb(danode_t& danode, void *arg, storable_t<dlnode_t>& dlnode, storable_t<hnode_t>& hnode);

      static void unpack_vnode_cb(vnode_t& vnode, uint64_t urlid, void *_this, storable_t<hnode_t>& hnode, storable_t<unode_t>& unode);

      static void unpack_active_hnode_cb(hnode_t& hnode, bool active, void *_this);
      /// @}

      static bool eval_hnode_cb(const hnode_t *hnode, void *arg);

      static bool eval_unode_cb(const unode_t *unode, void *arg);

      template <typename node_t, bool (database_t::*put_node)(const node_t& node, storage_info_t& strg_info)>
      static void swap_out_node_cb(storable_t<node_t> *node, void *arg);

   public:
      state_t(const config_t& config, end_visit_cb_t end_visit_db, end_download_cb_t end_download_cb, void *and_cb_arg);

      ~state_t(void);

      bool initialize(void);

      void cleanup(void);

      void save_state(void);

      void restore_state(void);

      static void upgrade_database(storable_t<sysnode_t>& sysnode, system_database_t& sysdb);

      void clear_month(void);
      
      void update_hourly_stats(void);

      void set_tstamp(const tstamp_t& tstamp);

      const sysnode_t& get_sysnode(void) const {return sysnode;}

      void database_info(void) const;

      void swap_out(int64_t tstamp, size_t maxmem);

      ///
      /// @name   Serialization callbacks
      ///
      /// @{

      static void unpack_dlnode_cached_host_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *_this, const storable_t<hnode_t>& hnode);

      static void unpack_dlnode_and_host_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *_this, storable_t<hnode_t>& hnode);
      /// @}

      /// Returns application version with or without the build number.
      static string_t get_app_version(bool build = true);

      /// Returns human-readable application version.
      static string_t get_app_hr_version(void);

      /// Returns a string representation of the specified application version.
      static string_t get_version(u_int version);

      /// Returns a human-readable string representation of the specified application version.
      static string_t get_hr_version(u_int version);
};

#endif  // _PRESERVE_H
