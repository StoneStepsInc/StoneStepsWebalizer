/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
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

      // hash tables
      h_hash_table hm_htab;                      // hosts (monthly)
      u_hash_table um_htab;                      // URLS
      r_hash_table rm_htab;                      // referrers
      a_hash_table am_htab;                      // user agents
      s_hash_table sr_htab;                      // search string table
      i_hash_table im_htab;                      // ident table (username)
      rc_hash_table rc_htab;                     // HTTP status codes
      dl_hash_table dl_htab;                     // active download jobs

      std::unordered_set<string_t, hash_string> sp_htab; ///< Spammer hosts

      cc_hash_table cc_htab;                     // countries
      ct_hash_table ct_htab;                     ///< City hash table

      as_hash_table as_htab;                     ///< Autonomous system table.

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

      void del_htabs(void);

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

      static string_t get_app_version(void);

      static string_t get_version(u_int version);
};

#endif  // _PRESERVE_H
