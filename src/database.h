/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    database.h
*/
#ifndef DATABASE_H
#define DATABASE_H

#include "config.h"
#include "hashtab.h"
#include "tstring.h"
#include "tstamp.h"
#include "types.h"
#include "hashtab_nodes.h"
#include "event.h"
#include "berkeleydb.h"
#include "storable.h"

///
/// @brief  Translates application configuration into database configuration.
///
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

      uint32_t get_db_cache_size(void) const {return config.db_cache_size;}

      uint32_t get_db_seq_cache_size(void) const {return config.db_seq_cache_size;}

      bool get_db_direct(void) const {return config.db_direct;}
};

///
/// @brief  Opens only the system table in the specified database.
///
/// An instance of `system_database_t` has only the system table open and may be used
/// to validate the database version, time settings and byte order, as well as to make
/// schema changes, such as to rename tables other than the system tables.
///
class system_database_t : public berkeleydb_t {
   protected:
      const ::config_t& config;           ///< A reference to the application configuration,

      table_t           system;           ///< The system table.

   public:
      system_database_t(const ::config_t& config);

      ~system_database_t(void);

      /// Opens the database with just the system table open.
      status_t open(void);

      /// Returns `true` if there is system node in the database.
      bool is_sysnode(void) const;

      /// Saves the system node in the database.
      bool put_sysnode(const sysnode_t& sysnode, storage_info_t& strg_info);

      /// Reads the system node from the database.
      bool get_sysnode_by_id(storable_t<sysnode_t>& sysnode, sysnode_t::s_unpack_cb_t<> upcb, void *arg) const;
};

///
/// @class  database_t
///
/// @brief  Application-specific database management class
///
class database_t : public berkeleydb_t {
   private:
      struct table_desc_t;
      struct index_desc_t;

   private:
      static const table_desc_t table_desc[];
      static const index_desc_t index_desc[];

      const ::config_t& config;                    ///< A reference to the application configuration,

      table_t           system;                    ///< The system table.
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
      table_t           cities;

   public:
      database_t(const ::config_t& config);

      ~database_t(void);

      status_t open(void);

      status_t attach_indexes(bool rebuild);

      status_t rollover(const tstamp_t& tstamp);

      // urls
      uint64_t get_unode_id(void) {return (uint64_t) urls.get_seq_id();}

      iterator<unode_t> begin_urls(const char *dbname) const {return urls.begin<unode_t>(dbname);}

      reverse_iterator<unode_t> rbegin_urls(const char *dbname) const {return urls.rbegin<unode_t>(dbname);}

      bool put_unode(const unode_t& unode, storage_info_t& strg_info);

      bool get_unode_by_id(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      bool get_unode_by_value(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      // hosts
      uint64_t get_hnode_id(void) {return (uint64_t) hosts.get_seq_id();}

      iterator<hnode_t> begin_hosts(const char *dbname) const {return hosts.begin<hnode_t>(dbname);}

      reverse_iterator<hnode_t> rbegin_hosts(const char *dbname) const {return hosts.rbegin<hnode_t>(dbname);}

      bool put_hnode(const hnode_t& hnode, storage_info_t& strg_info);

      template <typename ... param_t>
      bool get_hnode_by_id(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const;

      template <typename ... param_t>
      bool get_hnode_by_value(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const;

      // visits
      bool put_vnode(const vnode_t& vnode, storage_info_t& strg_info);

      iterator<vnode_t> begin_visits(void) const {return visits.begin<vnode_t>(NULL);}

      bool get_vnode_by_id(storable_t<vnode_t>& vnode, vnode_t::s_unpack_cb_t<storable_t<unode_t>&> upcb, void *arg, storable_t<unode_t>& unode) const;

      bool delete_visit(const keynode_t<uint64_t>& vnode);

      uint64_t get_vcount(const char *dbname = NULL) const {return visits.count(dbname);}

      //
      // downloads
      //
      uint64_t get_dlnode_id(void) {return (uint64_t) downloads.get_seq_id();}

      iterator<dlnode_t> begin_downloads(const char *dbname) const {return downloads.begin<dlnode_t>(dbname);}

      reverse_iterator<dlnode_t> rbegin_downloads(const char *dbname) const {return downloads.rbegin<dlnode_t>(dbname);}

      bool put_dlnode(const dlnode_t& dlnode, storage_info_t& strg_info);

      template <typename ... param_t>
      bool get_dlnode_by_id(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const;

      template <typename ... param_t>
      bool get_dlnode_by_value(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const;

      bool delete_download(const keynode_t<uint64_t>& danode);

      //
      // active downloads
      //
      bool put_danode(const danode_t& danode, storage_info_t& strg_info);

      iterator<danode_t> begin_active_downloads(void) const {return active_downloads.begin<danode_t>(NULL);}

      bool get_danode_by_id(storable_t<danode_t>& danode, danode_t::s_unpack_cb_t<> upcb, void *arg) const;

      uint64_t get_dacount(const char *dbname = NULL) const {return active_downloads.count(dbname);}

      //
      // agents
      //
      uint64_t get_anode_id(void) {return (uint64_t) agents.get_seq_id();}

      iterator<anode_t> begin_agents(const char *dbname) const {return agents.begin<anode_t>(dbname);}

      reverse_iterator<anode_t> rbegin_agents(const char *dbname) const {return agents.rbegin<anode_t>(dbname);}

      bool put_anode(const anode_t& anode, storage_info_t& strg_info);

      bool get_anode_by_id(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      bool get_anode_by_value(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      //
      // referrers
      //
      uint64_t get_rnode_id(void) {return (uint64_t) referrers.get_seq_id();}

      iterator<rnode_t> begin_referrers(const char *dbname) const {return referrers.begin<rnode_t>(dbname);}

      reverse_iterator<rnode_t> rbegin_referrers(const char *dbname) const {return referrers.rbegin<rnode_t>(dbname);}

      bool put_rnode(const rnode_t& rnode, storage_info_t& strg_info);

      bool get_rnode_by_id(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      bool get_rnode_by_value(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      //
      // search strings
      //
      uint64_t get_snode_id(void) {return (uint64_t) search.get_seq_id();}

      iterator<snode_t> begin_search(const char *dbname) const {return search.begin<snode_t>(dbname);}

      reverse_iterator<snode_t> rbegin_search(const char *dbname) const {return search.rbegin<snode_t>(dbname);}

      bool put_snode(const snode_t& snode, storage_info_t& strg_info);

      bool get_snode_by_id(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      bool get_snode_by_value(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      //
      // users
      //
      uint64_t get_inode_id(void) {return (uint64_t) users.get_seq_id();}

      iterator<inode_t> begin_users(const char *dbname) const {return users.begin<inode_t>(dbname);}

      reverse_iterator<inode_t> rbegin_users(const char *dbname) const {return users.rbegin<inode_t>(dbname);}

      bool put_inode(const inode_t& inode, storage_info_t& strg_info);

      bool get_inode_by_id(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      bool get_inode_by_value(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t<> upcb = NULL, void *arg = NULL) const;

      //
      // errors
      //
      uint64_t get_rcnode_id(void) {return (uint64_t) errors.get_seq_id();}

      iterator<rcnode_t> begin_errors(const char *dbname) const {return errors.begin<rcnode_t>(dbname);}

      reverse_iterator<rcnode_t> rbegin_errors(const char *dbname) const {return errors.rbegin<rcnode_t>(dbname);}

      bool put_rcnode(const rcnode_t& rcnode, storage_info_t& strg_info);

      bool get_rcnode_by_id(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t<> upcb, void *arg) const;

      bool get_rcnode_by_value(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t<> upcb, void *arg) const;

      uint64_t get_errcount(const char *dbname = NULL) const {return errors.count(dbname);}

      //
      // status codes
      //
      bool put_scnode(const scnode_t& scnode, storage_info_t& strg_info);

      bool get_scnode_by_id(storable_t<scnode_t>& scnode, scnode_t::s_unpack_cb_t<> upcb, void *arg) const;

      //
      // daily totals
      //
      bool put_tdnode(const daily_t& tdnode, storage_info_t& strg_info);

      bool get_tdnode_by_id(storable_t<daily_t>& tdnode, daily_t::s_unpack_cb_t<> upcb, void *arg) const;

      //
      // hourly totals
      //
      bool put_thnode(const hourly_t& thnode, storage_info_t& strg_info);

      bool get_thnode_by_id(storable_t<hourly_t>& thnode, hourly_t::s_unpack_cb_t<> upcb, void *arg) const;

      //
      // totals
      //
      bool put_tgnode(const totals_t& tgnode, storage_info_t& strg_info);

      bool get_tgnode_by_id(storable_t<totals_t>& tgnode, totals_t::s_unpack_cb_t<> upcb, void *arg) const;

      //
      // country codes
      //
      iterator<ccnode_t> begin_countries(const char *dbname) const {return countries.begin<ccnode_t>(dbname);}

      reverse_iterator<ccnode_t> rbegin_countries(const char *dbname) const {return countries.rbegin<ccnode_t>(dbname);}

      bool put_ccnode(const ccnode_t& ccnode, storage_info_t& strg_info);

      bool get_ccnode_by_id(storable_t<ccnode_t>& ccnode, ccnode_t::s_unpack_cb_t<> upcb, void *arg) const;

      //
      // cities
      //
      iterator<ctnode_t> begin_cities(const char *dbname) const {return cities.begin<ctnode_t>(dbname);}

      reverse_iterator<ctnode_t> rbegin_cities(const char *dbname) const {return cities.rbegin<ctnode_t>(dbname);}

      bool put_ctnode(const ctnode_t& ctnode, storage_info_t& strg_info);

      bool get_ctnode_by_id(storable_t<ctnode_t>& ctnode, ctnode_t::s_unpack_cb_t<> upcb, void *arg) const;

      ///
      /// @name   System
      /// @{

      /// Saves the system node in the database.
      bool put_sysnode(const sysnode_t& sysnode, storage_info_t& strg_info);

      /// Reads the system node from the database.
      bool get_sysnode_by_id(storable_t<sysnode_t>& sysnode, sysnode_t::s_unpack_cb_t<> upcb, void *arg) const;
      /// @}
};

#endif // DATABASE_H
