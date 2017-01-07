/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   preserve.h
*/

#ifndef _PRESERVE_H
#define _PRESERVE_H

#include "types.h"
#include "scnode.h"
#include "daily.h"
#include "hourly.h"
#include "tstring.h"
#include "history.h"
#include "database.h"
#include "hashtab_nodes.h"

#include <vector>

class config_t;
class lang_t;
class webalizer_t;

// -----------------------------------------------------------------------
// state_t
//
// 1. Represents the entire processing state for the current month. The state
// consists of all time totals (total hits, files, etc), daily and hourly totals 
// in the current month and lookup hash tables contaning totals for individual 
// items, such as URLs or hosts. The state is stored in the database between 
// runs.
//
// 2. While the history is a data member, it's not a part of the monthly state
// and it's restored and saved outside of the state restore/save methods. History 
// contains a collection of totals for each month. A history record may be updated 
// using current monthly state after the state is restored and when the state is 
// being saved. The former case works even when there is no history file available, 
// which means that a new history file will be created. 
//
class state_t {
   friend class webalizer_t;

   public:
      totals_t totals;

      daily_t t_daily[31];                       // daily totals

      hourly_t t_hourly[24];                     // hourly totals

      sc_hash_table response;                    // HTTP status codes

      // hash tables
      h_hash_table hm_htab;                      // hosts (monthly)
      u_hash_table um_htab;                      // URLS
      r_hash_table rm_htab;                      // referrers
      a_hash_table am_htab;                      // user agents
      s_hash_table sr_htab;                      // search string table
      i_hash_table im_htab;                      // ident table (username)
      rc_hash_table rc_htab;                     // HTTP status codes
      dl_hash_table dl_htab;                     // active download jobs
      sp_hash_table sp_htab;                     // spammers

      cc_hash_table cc_htab;                     // countries

      std::vector<uint64_t> v_ended;             // ended active visit node IDs
      std::vector<uint64_t> dl_ended;            // ended active download node IDs

      history_t   history;
                                                 // see note #2 above
      database_t  database;

   private:
      const config_t&   config;

      char              *buffer;

      bool              stfile;                  // old state file?

      sysnode_t         sysnode;

   private:
      //
      // 
      //
      hnode_t *put_hnode(const hnode_t& hnode);
      rnode_t *put_rnode(const rnode_t& rnode);
      unode_t *put_unode(const unode_t& unode);
      anode_t *put_anode(const anode_t& anode);
      snode_t *put_snode(const snode_t& snode);
      inode_t *put_inode(const inode_t& inode);
      rcnode_t *put_rcnode(const rcnode_t& rcnode);
      dlnode_t *put_dlnode(const dlnode_t& dlnode);
      spnode_t *put_spnode(const string_t& host);
      
      template <typename type_t>
      void update_avg_max(double& avg, type_t& max, type_t value, uint64_t newcnt) const;

      unode_t *find_url(const string_t& url);

      void del_htabs(void);

      void init_counters(void);

      bool is_state_file(void);

      bool del_state_file(void);

      static void unpack_dlnode_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *_this);

      static void unpack_vnode_cb(vnode_t& vnode, uint64_t urlid, void *_this);

      static void unpack_hnode_cb(hnode_t& hnode, bool active, void *_this);

      static bool eval_hnode_cb(const hnode_t *hnode, void *arg);
      static bool swap_hnode_cb(hnode_t *hnode, void *arg);

      static bool eval_unode_cb(const unode_t *unode, void *arg);
      static bool swap_unode_cb(unode_t *unode, void *arg);

   public:
      state_t(const config_t& config);

      ~state_t(void);

      bool initialize(void);

      void cleanup(void);

      int save_state(void);

      int restore_state(void);

      int upgrade_database(void);

      void clear_month(void);
      
      void update_hourly_stats(void);

      void set_tstamp(const tstamp_t& tstamp);

      void swap_out(void);

      const sysnode_t& get_sysnode(void) const {return sysnode;}

      void database_info(void) const;

      static void unpack_dlnode_const_cb(dlnode_t& dlnode, uint64_t hostid, bool active, void *_this);

      static void unpack_hnode_const_cb(hnode_t& hnode, bool active, void *arg);

      static string_t get_app_version(void);

      static string_t get_version(u_int version);
};

#endif  /* _PRESERVE_H */
