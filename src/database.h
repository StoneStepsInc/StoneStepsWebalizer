/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    database.h
*/
#ifndef __DATABASE_H
#define __DATABASE_H

#include "config.h"
#include "hashtab.h"
#include "tstring.h"
#include "vector.h"
#include "tstamp.h"
#include "types.h"
#include "hashtab_nodes.h"
#include "event.h"
#include "berkeleydb.h"

// -----------------------------------------------------------------
//
// database_t
//
// -----------------------------------------------------------------
class database_t : public berkeleydb_t {
   private:
      class db_config_t : public berkeleydb_t::config_t {
         private:
            const ::config_t& config;
            string_t          db_path;

         public:
            db_config_t(const ::config_t& config) : config(config), db_path(config.get_db_path()) {}

            const db_config_t& clone(void) const {return *new db_config_t(config);}

            void release(void) const {delete this;}

            const string_t& get_db_path(void) const {return db_path;}

            const string_t& get_tmp_path(void) const {return config.db_path;}

            const string_t& get_db_fname(void) const {return config.db_fname;}

            const string_t& get_db_fname_ext(void) const {return config.db_fname_ext;}

            uint32_t get_db_cache_size(void) const {return config.db_cache_size;}

            uint32_t get_db_seq_cache_size(void) const {return config.db_seq_cache_size;}

            uint32_t get_db_trickle_rate(void) const {return config.db_trickle_rate;}

            bool get_db_direct(void) const {return config.db_direct;}

            bool get_db_dsync(void) const {return config.db_dsync;}
      };

   private:
      const ::config_t& config;

      table_t           urls;
      table_t           hosts;
      table_t           visits;
      table_t           downloads;
      table_t           active_downloads;
      table_t           agents;
      table_t           referrers;
      table_t           search;
      table_t           users;
      table_t           errors;
      table_t           scodes;
      table_t           daily;
      table_t           hourly;
      table_t           totals;
      table_t           countries;
      table_t           system;

   public:
      database_t(const ::config_t& config);

      ~database_t(void);

      bool open(void);

      bool attach_indexes(bool rebuild);

      bool rollover(const tstamp_t& tstamp);

      // urls
      uint64_t get_unode_id(void) {return (uint64_t) urls.get_seq_id();}

      iterator<unode_t> begin_urls(const char *dbname) const {return iterator<unode_t>(urls, dbname);}

      reverse_iterator<unode_t> rbegin_urls(const char *dbname) const {return reverse_iterator<unode_t>(urls, dbname);}

      bool put_unode(const unode_t& unode);

      bool get_unode_by_id(unode_t& unode, unode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_unode_by_value(unode_t& unode, unode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      uint64_t get_ucount(const char *dbname = NULL) const {return urls.count(dbname);}

      // hosts
      uint64_t get_hnode_id(void) {return (uint64_t) hosts.get_seq_id();}

      iterator<hnode_t> begin_hosts(const char *dbname) const {return iterator<hnode_t>(hosts, dbname);}

      reverse_iterator<hnode_t> rbegin_hosts(const char *dbname) const {return reverse_iterator<hnode_t>(hosts, dbname);}

      bool put_hnode(const hnode_t& hnode);

      bool get_hnode_by_id(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_hnode_by_value(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const;

      uint64_t get_hcount(const char *dbname = NULL) const {return hosts.count(dbname);}

      // visits
      bool put_vnode(const vnode_t& vnode);

      iterator<vnode_t> begin_visits(void) const {return iterator<vnode_t>(visits, NULL);}

      bool get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool delete_visit(const keynode_t<uint64_t>& vnode);

      uint64_t get_vcount(const char *dbname = NULL) const {return visits.count(dbname);}

      //
      // downloads
      //
      uint64_t get_dlnode_id(void) {return (uint64_t) downloads.get_seq_id();}

      iterator<dlnode_t> begin_downloads(const char *dbname) const {return iterator<dlnode_t>(downloads, dbname);}

      reverse_iterator<dlnode_t> rbegin_downloads(const char *dbname) const {return reverse_iterator<dlnode_t>(downloads, dbname);}

      bool put_dlnode(const dlnode_t& dlnode);

      bool get_dlnode_by_id(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_dlnode_by_value(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const;

      uint64_t get_dlcount(const char *dbname = NULL) const {return downloads.count(dbname);}

      bool delete_download(const keynode_t<uint64_t>& danode);

      //
      // active downloads
      //
      bool put_danode(const danode_t& danode);

      iterator<danode_t> begin_active_downloads(void) const {return iterator<danode_t>(active_downloads, NULL);}

      bool get_danode_by_id(danode_t& danode, danode_t::s_unpack_cb_t upcb, void *arg) const;

      uint64_t get_dacount(const char *dbname = NULL) const {return active_downloads.count(dbname);}

      //
      // agents
      //
      uint64_t get_anode_id(void) {return (uint64_t) agents.get_seq_id();}

      iterator<anode_t> begin_agents(const char *dbname) const {return iterator<anode_t>(agents, dbname);}

      reverse_iterator<anode_t> rbegin_agents(const char *dbname) const {return reverse_iterator<anode_t>(agents, dbname);}

      bool put_anode(const anode_t& anode);

      bool get_anode_by_id(anode_t& anode, anode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_anode_by_value(anode_t& anode, anode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      uint64_t get_acount(const char *dbname = NULL) const {return agents.count(dbname);}

      //
      // referrers
      //
      uint64_t get_rnode_id(void) {return (uint64_t) referrers.get_seq_id();}

      iterator<rnode_t> begin_referrers(const char *dbname) const {return iterator<rnode_t>(referrers, dbname);}

      reverse_iterator<rnode_t> rbegin_referrers(const char *dbname) const {return reverse_iterator<rnode_t>(referrers, dbname);}

      bool put_rnode(const rnode_t& rnode);

      bool get_rnode_by_id(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_rnode_by_value(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      uint64_t get_rcount(const char *dbname = NULL) const {return referrers.count(dbname);}

      //
      // search strings
      //
      uint64_t get_snode_id(void) {return (uint64_t) search.get_seq_id();}

      iterator<snode_t> begin_search(const char *dbname) const {return iterator<snode_t>(search, dbname);}

      reverse_iterator<snode_t> rbegin_search(const char *dbname) const {return reverse_iterator<snode_t>(search, dbname);}

      bool put_snode(const snode_t& snode);

      bool get_snode_by_id(snode_t& snode, snode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_snode_by_value(snode_t& snode, snode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      uint64_t get_scount(const char *dbname = NULL) const {return search.count(dbname);}

      //
      // users
      //
      uint64_t get_inode_id(void) {return (uint64_t) users.get_seq_id();}

      iterator<inode_t> begin_users(const char *dbname) const {return iterator<inode_t>(users, dbname);}

      reverse_iterator<inode_t> rbegin_users(const char *dbname) const {return reverse_iterator<inode_t>(users, dbname);}

      bool put_inode(const inode_t& inode);

      bool get_inode_by_id(inode_t& inode, inode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_inode_by_value(inode_t& inode, inode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      uint64_t get_icount(const char *dbname = NULL) const {return users.count(dbname);}

      //
      // errors
      //
      uint64_t get_rcnode_id(void) {return (uint64_t) errors.get_seq_id();}

      iterator<rcnode_t> begin_errors(const char *dbname) const {return iterator<rcnode_t>(errors, dbname);}

      reverse_iterator<rcnode_t> rbegin_errors(const char *dbname) const {return reverse_iterator<rcnode_t>(errors, dbname);}

      bool put_rcnode(const rcnode_t& rcnode);

      bool get_rcnode_by_id(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_rcnode_by_value(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const;

      uint64_t get_errcount(const char *dbname = NULL) const {return errors.count(dbname);}

      //
      // status codes
      //
      bool put_scnode(const scnode_t& scnode);

      bool get_scnode_by_id(scnode_t& scnode, scnode_t::s_unpack_cb_t upcb, void *arg) const;

      //
      // daily totals
      //
      bool put_tdnode(const daily_t& tdnode);

      bool get_tdnode_by_id(daily_t& tdnode, daily_t::s_unpack_cb_t upcb, void *arg) const;

      //
      // hourly totals
      //
      bool put_thnode(const hourly_t& thnode);

      bool get_thnode_by_id(hourly_t& thnode, hourly_t::s_unpack_cb_t upcb, void *arg) const;

      //
      // totals
      //
      bool put_tgnode(const totals_t& tgnode);

      bool get_tgnode_by_id(totals_t& tgnode, totals_t::s_unpack_cb_t upcb, void *arg) const;

      //
      // country codes
      //
      iterator<ccnode_t> begin_countries(void) const {return iterator<ccnode_t>(countries, NULL);}

      bool put_ccnode(const ccnode_t& ccnode);

      bool get_ccnode_by_id(ccnode_t& ccnode, ccnode_t::s_unpack_cb_t upcb, void *arg) const;

      //
      // system
      //
      bool is_sysnode(void) const;

      bool put_sysnode(const sysnode_t& sysnode);

      bool get_sysnode_by_id(sysnode_t& sysnode, sysnode_t::s_unpack_cb_t upcb, void *arg) const;
};

#endif // __DATABASE_H
