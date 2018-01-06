/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   database.cpp
*/
#include "pch.h"

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <errno.h>
#include <new>

#include "database.h"
#include "util_path.h"
#include "serialize.h"
#include "exception.h"

#include <type_traits>
#include <utility>

//
// B-Tree group field extraction function template
//
template <typename node_t, s_field_cb_t extract>
int sc_extract_group_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result)
{
   if(!node_t::s_is_group(data->get_data(), data->get_size()))
      return DB_DONOTINDEX;

   return sc_extract_cb<extract>(secondary, key, data, result);
}

// -----------------------------------------------------------------------
//
// database_t
//
// -----------------------------------------------------------------------

database_t::database_t(const ::config_t& config) : berkeleydb_t(db_config_t(config)),
      config(config),
      urls(make_table()),
      hosts(make_table()),
      visits(make_table()),
      downloads(make_table()),
      active_downloads(make_table()),
      agents(make_table()),
      referrers(make_table()),
      search(make_table()),
      users(make_table()),
      errors(make_table()),
      scodes(make_table()),
      daily(make_table()),
      hourly(make_table()),
      totals(make_table()),
      countries(make_table()),
      system(make_table())
{
   // initialize the array to hold table pointers
   add_table(urls);
   add_table(hosts);
   add_table(visits);
   add_table(downloads);
   add_table(active_downloads);
   add_table(agents);
   add_table(referrers);
   add_table(search);
   add_table(users);
   add_table(errors);
   add_table(scodes);
   add_table(daily);
   add_table(hourly);
   add_table(totals);
   add_table(countries);
   add_table(system);
}

database_t::~database_t(void)
{
}

bool database_t::attach_indexes(bool rebuild)
{
   //
   // urls
   //
   if(urls.associate("urls.hits", sc_extract_cb<unode_t::s_field_hits>, rebuild))
      return false;

   if(urls.associate("urls.xfer", sc_extract_cb<unode_t::s_field_xfer>, rebuild))
      return false;

   if(urls.associate("urls.entry", sc_extract_cb<unode_t::s_field_entry>, rebuild))
      return false;

   if(urls.associate("urls.exit", sc_extract_cb<unode_t::s_field_exit>, rebuild))
      return false;

   if(urls.associate("urls.groups.hits", sc_extract_group_cb<unode_t, unode_t::s_field_hits>, rebuild))
      return false;

   if(urls.associate("urls.groups.xfer", sc_extract_group_cb<unode_t, unode_t::s_field_xfer>, rebuild))
      return false;

   //
   // hosts
   //
   if(hosts.associate("hosts.hits", sc_extract_cb<hnode_t::s_field_hits>, rebuild))
      return false;

   if(hosts.associate("hosts.xfer", sc_extract_cb<hnode_t::s_field_xfer>, rebuild))
      return false;

   if(hosts.associate("hosts.groups.hits", sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>, rebuild))
      return false;

   if(hosts.associate("hosts.groups.xfer", sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>, rebuild))
      return false;

   // 
   // downloads
   // 
   if(downloads.associate("downloads.xfer", sc_extract_cb<dlnode_t::s_field_xfer>, rebuild))
      return false;

   //
   // agents
   //
   if(agents.associate("agents.hits", sc_extract_cb<anode_t::s_field_hits>, rebuild))
      return false;

   if(agents.associate("agents.visits", sc_extract_cb<anode_t::s_field_visits>, rebuild))
      return false;

   if(agents.associate("agents.groups.visits", sc_extract_group_cb<anode_t, anode_t::s_field_visits>, rebuild))
      return false;

   //
   // referrers
   //
   if(referrers.associate("referrers.hits", sc_extract_cb<rnode_t::s_field_hits>, rebuild))
      return false;

   if(referrers.associate("referrers.groups.hits", sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>, rebuild))
      return false;

   //
   // search strings
   //
   if(search.associate("search.hits", sc_extract_cb<snode_t::s_field_hits>, rebuild))
      return false;

   //
   // users
   //
   if(users.associate("users.hits", sc_extract_cb<inode_t::s_field_hits>, rebuild))
      return false;

   if(users.associate("users.groups.hits", sc_extract_group_cb<inode_t, inode_t::s_field_hits>, rebuild))
      return false;

   //
   // errors
   //
   if(errors.associate("errors.hits", sc_extract_cb<rcnode_t::s_field_hits>, rebuild))
      return false;

   return true;
}

bool database_t::open(void)
{
   if(!berkeleydb_t::open())
      return false;

   //
   // confgure all databases
   //

   //
   // system
   //
   if(system.open(config.get_db_path(), "system", bt_compare_cb<sysnode_t::s_compare_key>))
      return false;

   //
   // urls
   //
   if(!get_readonly()) {
      if(urls.open_sequence("urls.seq", config.db_seq_cache_size))
         return false;
   }

   if(urls.associate(config.get_db_path(), "urls.values", bt_compare_cb<unode_t::s_compare_value_hash>, bt_compare_cb<unode_t::s_compare_value_hash>, sc_extract_cb<unode_t::s_field_value_hash>))
      return false;

   urls.set_values_db("urls.values");

   if(urls.associate(config.get_db_path(), "urls.hits", bt_compare_cb<unode_t::s_compare_hits>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_hits>) : NULL))
      return false;

   if(urls.associate(config.get_db_path(), "urls.xfer", bt_compare_cb<unode_t::s_compare_xfer>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_xfer>) : NULL))
      return false;

   if(urls.associate(config.get_db_path(), "urls.entry", bt_compare_cb<unode_t::s_compare_entry>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_entry>) : NULL))
      return false;

   if(urls.associate(config.get_db_path(), "urls.exit", bt_compare_cb<unode_t::s_compare_exit>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_exit>) : NULL))
      return false;

   if(urls.associate(config.get_db_path(), "urls.groups.hits", bt_compare_cb<unode_t::s_compare_hits>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<unode_t, unode_t::s_field_hits>)) : NULL))
      return false;

   if(urls.associate(config.get_db_path(), "urls.groups.xfer", bt_compare_cb<unode_t::s_compare_xfer>, bt_compare_cb<unode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<unode_t, unode_t::s_field_xfer>)) : NULL))
      return false;

   if(urls.open(config.get_db_path(), "urls", bt_compare_cb<unode_t::s_compare_key>))
      return false;

   //
   // hosts
   //
   if(!get_readonly()) {
      if(hosts.open_sequence("hosts.seq", config.db_seq_cache_size))
         return false;
   }

   if(hosts.associate(config.get_db_path(), "hosts.values", bt_compare_cb<hnode_t::s_compare_value_hash>, bt_compare_cb<hnode_t::s_compare_value_hash>, sc_extract_cb<hnode_t::s_field_value_hash>))
      return false;

   hosts.set_values_db("hosts.values");

   if(hosts.associate(config.get_db_path(), "hosts.hits", bt_compare_cb<hnode_t::s_compare_hits>, bt_compare_cb<hnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<hnode_t::s_field_hits>) : NULL))
      return false;

   if(hosts.associate(config.get_db_path(), "hosts.xfer", bt_compare_cb<hnode_t::s_compare_xfer>, bt_compare_cb<hnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<hnode_t::s_field_xfer>) : NULL))
      return false;

   if(hosts.associate(config.get_db_path(), "hosts.groups.hits", bt_compare_cb<hnode_t::s_compare_hits>, bt_compare_cb<hnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>)) : NULL))
      return false;

   if(hosts.associate(config.get_db_path(), "hosts.groups.xfer", bt_compare_cb<hnode_t::s_compare_xfer>, bt_compare_cb<hnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>)) : NULL))
      return false;

   if(hosts.open(config.get_db_path(), "hosts", bt_compare_cb<hnode_t::s_compare_key>))
      return false;

   //
   // visits
   //
   if(visits.open(config.get_db_path(), "visits.active", bt_compare_cb<vnode_t::s_compare_key>))
      return false;

   //
   // downloads
   //
   if(!get_readonly()) {
      if(downloads.open_sequence("downloads.seq", config.db_seq_cache_size))
         return false;
   }
   
   if(downloads.associate(config.get_db_path(), "downloads.values", bt_compare_cb<dlnode_t::s_compare_value_hash>, bt_compare_cb<dlnode_t::s_compare_value_hash>, sc_extract_cb<dlnode_t::s_field_value_hash>))
      return false;

   downloads.set_values_db("downloads.values");

   if(downloads.associate(config.get_db_path(), "downloads.xfer", bt_compare_cb<dlnode_t::s_compare_xfer>, bt_compare_cb<dlnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<dlnode_t::s_field_xfer>) : NULL))
      return false;

   if(downloads.open(config.get_db_path(), "downloads", bt_compare_cb<dlnode_t::s_compare_key>))
      return false;

   //
   // active downloads
   //
   if(active_downloads.open(config.get_db_path(), "downloads.active", bt_compare_cb<danode_t::s_compare_key>))
      return false;

   //
   // agents
   //
   if(!get_readonly()) {
      if(agents.open_sequence("agents.seq", config.db_seq_cache_size))
         return false;
   }

   if(agents.associate(config.get_db_path(), "agents.values", bt_compare_cb<anode_t::s_compare_value_hash>, bt_compare_cb<anode_t::s_compare_value_hash>, sc_extract_cb<anode_t::s_field_value_hash>))
      return false;

   agents.set_values_db("agents.values");

   if(agents.associate(config.get_db_path(), "agents.hits", bt_compare_cb<anode_t::s_compare_hits>, bt_compare_cb<anode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<anode_t::s_field_hits>) : NULL))
      return false;

   if(agents.associate(config.get_db_path(), "agents.visits", bt_compare_cb<anode_t::s_compare_visits>, bt_compare_cb<anode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<anode_t::s_field_visits>) : NULL))
      return false;

   if(agents.associate(config.get_db_path(), "agents.groups.visits", bt_compare_cb<anode_t::s_compare_hits>, bt_compare_cb<anode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<anode_t, anode_t::s_field_visits>)) : NULL))
      return false;

   if(agents.open(config.get_db_path(), "agents", bt_compare_cb<anode_t::s_compare_key>))
      return false;

   //
   // referrers
   //
   if(!get_readonly()) {
      if(referrers.open_sequence("referrers.seq", config.db_seq_cache_size))
         return false;
   }

   if(referrers.associate(config.get_db_path(), "referrers.values", bt_compare_cb<rnode_t::s_compare_value_hash>, bt_compare_cb<rnode_t::s_compare_value_hash>, sc_extract_cb<rnode_t::s_field_value_hash>))
      return false;

   referrers.set_values_db("referrers.values");

   if(referrers.associate(config.get_db_path(), "referrers.hits", bt_compare_cb<rnode_t::s_compare_hits>, bt_compare_cb<rnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<rnode_t::s_field_hits>) : NULL))
      return false;

   if(referrers.associate(config.get_db_path(), "referrers.groups.hits", bt_compare_cb<rnode_t::s_compare_hits>, bt_compare_cb<rnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>)) : NULL))
      return false;

   if(referrers.open(config.get_db_path(), "referrers", bt_compare_cb<rnode_t::s_compare_key>))
      return false;

   //
   // search strings
   //
   if(!get_readonly()) {
      if(search.open_sequence("search.seq", config.db_seq_cache_size))
         return false;
   }

   if(search.associate(config.get_db_path(), "search.values", bt_compare_cb<snode_t::s_compare_value_hash>, bt_compare_cb<snode_t::s_compare_value_hash>, sc_extract_cb<snode_t::s_field_value_hash>))
      return false;

   search.set_values_db("search.values");

   if(search.associate(config.get_db_path(), "search.hits", bt_compare_cb<snode_t::s_compare_hits>, bt_compare_cb<snode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<snode_t::s_field_hits>) : NULL))
      return false;

   if(search.open(config.get_db_path(), "search", bt_compare_cb<snode_t::s_compare_key>))
      return false;

   //
   // users
   //
   if(!get_readonly()) {
      if(users.open_sequence("users.seq", config.db_seq_cache_size))
         return false;
   }

   if(users.associate(config.get_db_path(), "users.values", bt_compare_cb<inode_t::s_compare_value_hash>, bt_compare_cb<inode_t::s_compare_value_hash>, sc_extract_cb<inode_t::s_field_value_hash>))
      return false;

   users.set_values_db("users.values");

   if(users.associate(config.get_db_path(), "users.hits", bt_compare_cb<inode_t::s_compare_hits>, bt_compare_cb<inode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<inode_t::s_field_hits>) : NULL))
      return false;

   if(users.associate(config.get_db_path(), "users.groups.hits", bt_compare_cb<inode_t::s_compare_hits>, bt_compare_cb<inode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__((sc_extract_group_cb<inode_t, inode_t::s_field_hits>)) : NULL))
      return false;

   if(users.open(config.get_db_path(), "users", bt_compare_cb<inode_t::s_compare_key>))
      return false;

   //
   // errors
   //
   if(!get_readonly()) {
      if(errors.open_sequence("errors.seq", config.db_seq_cache_size))
         return false;
   }

   if(errors.associate(config.get_db_path(), "errors.values", bt_compare_cb<rcnode_t::s_compare_value_hash>, bt_compare_cb<rcnode_t::s_compare_value_hash>, sc_extract_cb<rcnode_t::s_field_value_hash>))
      return false;

   errors.set_values_db("errors.values");

   if(errors.associate(config.get_db_path(), "errors.hits", bt_compare_cb<rcnode_t::s_compare_hits>, bt_compare_cb<rcnode_t::s_compare_key>, get_readonly() ? __gcc_bug11407__(sc_extract_cb<rcnode_t::s_field_hits>) : NULL))
      return false;

   if(errors.open(config.get_db_path(), "errors", bt_compare_cb<rcnode_t::s_compare_key>))
      return false;

   //
   // status codes
   //
   if(scodes.open(config.get_db_path(), "statuscodes", bt_compare_cb<scnode_t::s_compare_key>))
      return false;

   //
   // daily totals
   //
   if(daily.open(config.get_db_path(), "totals.daily", bt_compare_cb<daily_t::s_compare_key>))
      return false;


   //
   // hourly totals
   //
   if(hourly.open(config.get_db_path(), "totals.hourly", bt_compare_cb<hourly_t::s_compare_key>))
      return false;

   //
   // totals
   //
   if(totals.open(config.get_db_path(), "totals", bt_compare_cb<totals_t::s_compare_key>))
      return false;

   //
   // country codes
   //
   if(countries.open(config.get_db_path(), "countries", bt_compare_cb<ccnode_t::s_compare_key>))
      return false;

   return true;
}

bool database_t::rollover(const tstamp_t& tstamp)
{
   u_int seqnum = 1;
   string_t path, curpath, newpath;

   // rollover is only called for the default database
   if(!config.is_default_db())
      return false;

   // close the database, so we can work with the database file
   if(!close())
      return false;

   // make the initial part of the path
   path = make_path(config.db_path, config.db_fname);

   // create a file name with a year/month sequence (e.g. webalizer_200706.db)
   curpath.format("%s.%s", path.c_str(), config.db_fname_ext.c_str());
   newpath.format("%s_%04d%02d.%s", path.c_str(), tstamp.year, tstamp.month, config.db_fname_ext.c_str());

   // if the file exists, increment the sequence number until a unique name is found
   while(!access(newpath, F_OK))
      newpath.format("%s_%04d%02d_%d.%s", path.c_str(), tstamp.year, tstamp.month, seqnum++, config.db_fname_ext.c_str());

   // rename the file
   if(rename(curpath, newpath))
      return false;

   // and reopen the database
   return open();
}

//
// unode
//
bool database_t::put_unode(const unode_t& unode, storage_info_t& strg_info)
{
   // unless unode_t::s_unpack_cb_t is provided, VC6 reports "fatal error C1506: unrecoverable block scoping error"
   return urls.put_node<unode_t>(unode, strg_info);
}

bool database_t::get_unode_by_id(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t upcb, void *arg) const
{
   return urls.get_node_by_id<unode_t>(unode, upcb, arg);
}

bool database_t::get_unode_by_value(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t upcb, void *arg) const
{
   return urls.get_node_by_value<unode_t>(unode, upcb, arg);
}

//
// hnode
//
bool database_t::put_hnode(const hnode_t& hnode, storage_info_t& strg_info)
{
   return hosts.put_node<hnode_t>(hnode, strg_info);
}

bool database_t::get_hnode_by_id(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return hosts.get_node_by_id<hnode_t>(hnode, upcb, arg);
}

bool database_t::get_hnode_by_value(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return hosts.get_node_by_value<hnode_t>(hnode, upcb, arg);
}

//
// vnode
//
bool database_t::put_vnode(const vnode_t& vnode, storage_info_t& strg_info)
{
   return visits.put_node<vnode_t>(vnode, strg_info);
}

bool database_t::get_vnode_by_id(storable_t<vnode_t>& vnode, vnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return visits.get_node_by_id<vnode_t>(vnode, upcb, arg);
}

bool database_t::delete_visit(const keynode_t<uint64_t>& vnode)
{
   return visits.delete_node(vnode);
}

// -----------------------------------------------------------------------
//
// downloads
//
// -----------------------------------------------------------------------
bool database_t::put_dlnode(const dlnode_t& dlnode, storage_info_t& strg_info)
{
   return downloads.put_node<dlnode_t>(dlnode, strg_info);
}

bool database_t::get_dlnode_by_id(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return downloads.get_node_by_id<dlnode_t>(dlnode, upcb, arg);
}

bool database_t::get_dlnode_by_value(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return downloads.get_node_by_value<dlnode_t>(dlnode, upcb, arg);
}

bool database_t::delete_download(const keynode_t<uint64_t>& danode)
{
   return active_downloads.delete_node(danode);
}

// -----------------------------------------------------------------------
//
// active downloads
//
// -----------------------------------------------------------------------
bool database_t::put_danode(const danode_t& danode, storage_info_t& strg_info)
{
   return active_downloads.put_node<danode_t>(danode, strg_info);
}

bool database_t::get_danode_by_id(storable_t<danode_t>& danode, danode_t::s_unpack_cb_t upcb, void *arg) const
{
   return active_downloads.get_node_by_id<danode_t>(danode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// user agents
//
// -----------------------------------------------------------------------
bool database_t::put_anode(const anode_t& anode, storage_info_t& strg_info)
{
   return agents.put_node<anode_t>(anode, strg_info);
}

bool database_t::get_anode_by_id(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t upcb, void *arg) const
{
   return agents.get_node_by_id<anode_t>(anode, upcb, arg);
}

bool database_t::get_anode_by_value(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t upcb, void *arg) const
{
   return agents.get_node_by_value<anode_t>(anode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// referrers
//
// -----------------------------------------------------------------------
bool database_t::put_rnode(const rnode_t& rnode, storage_info_t& strg_info)
{
   return referrers.put_node<rnode_t>(rnode, strg_info);
}

bool database_t::get_rnode_by_id(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return referrers.get_node_by_id<rnode_t>(rnode, upcb, arg);
}

bool database_t::get_rnode_by_value(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return referrers.get_node_by_value<rnode_t>(rnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// search strings
//
// -----------------------------------------------------------------------
bool database_t::put_snode(const snode_t& snode, storage_info_t& strg_info)
{
   return search.put_node<snode_t>(snode, strg_info);
}

bool database_t::get_snode_by_id(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t upcb, void *arg) const
{
   return search.get_node_by_id<snode_t>(snode, upcb, arg);
}

bool database_t::get_snode_by_value(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t upcb, void *arg) const
{
   return search.get_node_by_value<snode_t>(snode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// users
//
// -----------------------------------------------------------------------
bool database_t::put_inode(const inode_t& inode, storage_info_t& strg_info)
{
   return users.put_node<inode_t>(inode, strg_info);
}

bool database_t::get_inode_by_id(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t upcb, void *arg) const
{
   return users.get_node_by_id<inode_t>(inode, upcb, arg);
}

bool database_t::get_inode_by_value(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t upcb, void *arg) const
{
   return users.get_node_by_value<inode_t>(inode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// errors
//
// -----------------------------------------------------------------------
bool database_t::put_rcnode(const rcnode_t& rcnode, storage_info_t& strg_info)
{
   return errors.put_node<rcnode_t>(rcnode, strg_info);
}

bool database_t::get_rcnode_by_id(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return errors.get_node_by_id<rcnode_t>(rcnode, upcb, arg);
}

bool database_t::get_rcnode_by_value(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return errors.get_node_by_value<rcnode_t>(rcnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// status codes
//
// -----------------------------------------------------------------------
bool database_t::put_scnode(const scnode_t& scnode, storage_info_t& strg_info)
{
   return scodes.put_node<scnode_t>(scnode, strg_info);
}

bool database_t::get_scnode_by_id(storable_t<scnode_t>& scnode, scnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return scodes.get_node_by_id<scnode_t>(scnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// daily totals
//
// -----------------------------------------------------------------------
bool database_t::put_tdnode(const daily_t& tdnode, storage_info_t& strg_info)
{
   return daily.put_node<daily_t>(tdnode, strg_info);
}

bool database_t::get_tdnode_by_id(storable_t<daily_t>& tdnode, daily_t::s_unpack_cb_t upcb, void *arg) const
{
   return daily.get_node_by_id<daily_t>(tdnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// hourly totals
//
// -----------------------------------------------------------------------
bool database_t::put_thnode(const hourly_t& thnode, storage_info_t& strg_info)
{
   return hourly.put_node<hourly_t>(thnode, strg_info);
}

bool database_t::get_thnode_by_id(storable_t<hourly_t>& thnode, hourly_t::s_unpack_cb_t upcb, void *arg) const
{
   return hourly.get_node_by_id<hourly_t>(thnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// totals
//
// -----------------------------------------------------------------------
bool database_t::put_tgnode(const totals_t& tgnode, storage_info_t& strg_info)
{
   return totals.put_node<totals_t>(tgnode, strg_info);
}

bool database_t::get_tgnode_by_id(storable_t<totals_t>& tgnode, totals_t::s_unpack_cb_t upcb, void *arg) const
{
   return totals.get_node_by_id<totals_t>(tgnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// country codes
//
// -----------------------------------------------------------------------
bool database_t::put_ccnode(const ccnode_t& ccnode, storage_info_t& strg_info)
{
   return countries.put_node<ccnode_t>(ccnode, strg_info);
}

bool database_t::get_ccnode_by_id(storable_t<ccnode_t>& ccnode, ccnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return countries.get_node_by_id<ccnode_t>(ccnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// system
//
// -----------------------------------------------------------------------
bool database_t::is_sysnode(void) const
{
   return system.count() ? true : false;
}

bool database_t::put_sysnode(const sysnode_t& sysnode, storage_info_t& strg_info)
{
   return system.put_node<sysnode_t>(sysnode, strg_info);
}

bool database_t::get_sysnode_by_id(storable_t<sysnode_t>& sysnode, sysnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return system.get_node_by_id<sysnode_t>(sysnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// instantiate templates
//
// -----------------------------------------------------------------------
#include "berkeleydb_tmpl.cpp"

// keynode_t
template int bt_compare_cb<keynode_t<uint64_t>::s_compare_key>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<keynode_t<uint64_t>::s_compare_key>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<keynode_t<u_int>::s_compare_key>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<keynode_t<u_int>::s_compare_key>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

// URLs
template int bt_compare_cb<unode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<unode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<unode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<unode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<unode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<unode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<unode_t::s_compare_entry>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<unode_t::s_compare_exit>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<unode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<unode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<unode_t::s_field_entry>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<unode_t::s_field_exit>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<unode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<unode_t>;
template class berkeleydb_t::iterator<unode_t>;
template class berkeleydb_t::reverse_iterator<unode_t>;

template bool berkeleydb_t::table_t::put_node(const unode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id(storable_t<unode_t>& node, unode_t::s_unpack_cb_t upcb, void *arg) const;
template bool berkeleydb_t::table_t::get_node_by_value<unode_t>(storable_t<unode_t>& node, unode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<unode_t> berkeleydb_t::table_t::begin<unode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<unode_t> berkeleydb_t::table_t::rbegin<unode_t>(const char *dbname) const;

// hosts
template int bt_compare_cb<hnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<hnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<hnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<hnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<hnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<hnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<hnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<hnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<hnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<hnode_t>;
template class berkeleydb_t::iterator<hnode_t>;
template class berkeleydb_t::reverse_iterator<hnode_t>;

template bool berkeleydb_t::table_t::put_node<hnode_t>(const hnode_t& hnode, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<hnode_t>(storable_t<hnode_t>& node, hnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<hnode_t>(storable_t<hnode_t>& node, hnode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<hnode_t> berkeleydb_t::table_t::begin<hnode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<hnode_t> berkeleydb_t::table_t::rbegin<hnode_t>(const char *dbname) const;

// visits
template bool berkeleydb_t::table_t::put_node<vnode_t>(const vnode_t& hnode, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<vnode_t>(storable_t<vnode_t>& node, vnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template berkeleydb_t::iterator<vnode_t> berkeleydb_t::table_t::begin<vnode_t>(const char *dbname) const; 

template class berkeleydb_t::iterator_base<vnode_t>;
template class berkeleydb_t::iterator<vnode_t>;

// downloads
template int bt_compare_cb<dlnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<dlnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int bt_compare_cb<dlnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<dlnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<dlnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<dlnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template bool berkeleydb_t::table_t::put_node<dlnode_t>(const dlnode_t& hnode, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<dlnode_t>(storable_t<dlnode_t>& node, dlnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<dlnode_t>(storable_t<dlnode_t>& node, dlnode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<dlnode_t> berkeleydb_t::table_t::begin<dlnode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<dlnode_t> berkeleydb_t::table_t::rbegin<dlnode_t>(const char *dbname) const;

template class berkeleydb_t::iterator_base<dlnode_t>;
template class berkeleydb_t::iterator<dlnode_t>;
template class berkeleydb_t::reverse_iterator<dlnode_t>;

// active downloads
template bool berkeleydb_t::table_t::put_node<danode_t>(const danode_t& hnode, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<danode_t>(storable_t<danode_t>& node, danode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template berkeleydb_t::iterator<danode_t> berkeleydb_t::table_t::begin<danode_t>(const char *dbname) const; 

template class berkeleydb_t::iterator_base<danode_t>;
template class berkeleydb_t::iterator<danode_t>;

// user agents
template int bt_compare_cb<anode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<anode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<anode_t::s_compare_visits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *iocp);
template int bt_compare_cb<anode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<anode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<anode_t::s_compare_visits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<anode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<anode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<anode_t::s_field_visits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<anode_t, anode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<anode_t, anode_t::s_field_visits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<anode_t>;
template class berkeleydb_t::iterator<anode_t>;
template class berkeleydb_t::reverse_iterator<anode_t>;

template bool berkeleydb_t::table_t::put_node<anode_t>(const anode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<anode_t>(storable_t<anode_t>& node, anode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<anode_t>(storable_t<anode_t>& node, anode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<anode_t> berkeleydb_t::table_t::begin<anode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<anode_t> berkeleydb_t::table_t::rbegin<anode_t>(const char *dbname) const;

// referrers
template int bt_compare_cb<rnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<rnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<rnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<rnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<rnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<rnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<rnode_t>;
template class berkeleydb_t::iterator<rnode_t>;
template class berkeleydb_t::reverse_iterator<rnode_t>;

template bool berkeleydb_t::table_t::put_node<rnode_t>(const rnode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<rnode_t>(storable_t<rnode_t>& node, rnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<rnode_t>(storable_t<rnode_t>& node, rnode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<rnode_t> berkeleydb_t::table_t::begin<rnode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<rnode_t> berkeleydb_t::table_t::rbegin<rnode_t>(const char *dbname) const;

// search strings
template int bt_compare_cb<snode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<snode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<snode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<snode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<snode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<snode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<snode_t>;
template class berkeleydb_t::iterator<snode_t>;
template class berkeleydb_t::reverse_iterator<snode_t>;

template bool berkeleydb_t::table_t::put_node<snode_t>(const snode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<snode_t>(storable_t<snode_t>& node, snode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<snode_t>(storable_t<snode_t>& unode, snode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<snode_t> berkeleydb_t::table_t::begin<snode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<snode_t> berkeleydb_t::table_t::rbegin<snode_t>(const char *dbname) const;

// users
template int bt_compare_cb<inode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<inode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<inode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);
template int bt_compare_cb<inode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<inode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<inode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<inode_t, inode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class berkeleydb_t::iterator_base<inode_t>;
template class berkeleydb_t::iterator<inode_t>;
template class berkeleydb_t::reverse_iterator<inode_t>;

template bool berkeleydb_t::table_t::put_node<inode_t>(const inode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<inode_t>(storable_t<inode_t>& node, inode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<inode_t>(storable_t<inode_t>& node, inode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<inode_t> berkeleydb_t::table_t::begin<inode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<inode_t> berkeleydb_t::table_t::rbegin<inode_t>(const char *dbname) const;

// errors
template int bt_compare_cb<rcnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<rcnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int bt_compare_cb<rcnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<rcnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2);

template int sc_extract_cb<rcnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<rcnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template bool berkeleydb_t::table_t::put_node<rcnode_t>(const rcnode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<rcnode_t>(storable_t<rcnode_t>& node, rcnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
template bool berkeleydb_t::table_t::get_node_by_value<rcnode_t>(storable_t<rcnode_t>& node, rcnode_t::s_unpack_cb_t upcb, void *arg) const;

template berkeleydb_t::iterator<rcnode_t> berkeleydb_t::table_t::begin<rcnode_t>(const char *dbname) const; 
template berkeleydb_t::reverse_iterator<rcnode_t> berkeleydb_t::table_t::rbegin<rcnode_t>(const char *dbname) const;

template class berkeleydb_t::iterator_base<rcnode_t>;
template class berkeleydb_t::iterator<rcnode_t>;
template class berkeleydb_t::reverse_iterator<rcnode_t>;

// status codes
template bool berkeleydb_t::table_t::put_node<scnode_t>(const scnode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<scnode_t>(storable_t<scnode_t>& node, scnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template class berkeleydb_t::iterator_base<scnode_t>;
template class berkeleydb_t::iterator<scnode_t>;

// daily totals
template bool berkeleydb_t::table_t::put_node<daily_t>(const daily_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<daily_t>(storable_t<daily_t>& node, daily_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template class berkeleydb_t::iterator_base<daily_t>;
template class berkeleydb_t::iterator<daily_t>;

// hourly totals
template bool berkeleydb_t::table_t::put_node<hourly_t>(const hourly_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<hourly_t>(storable_t<hourly_t>& node, hourly_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template class berkeleydb_t::iterator_base<hourly_t>;
template class berkeleydb_t::iterator<hourly_t>;

// totals
template bool berkeleydb_t::table_t::put_node<totals_t>(const totals_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<totals_t>(storable_t<totals_t>& node, totals_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

// country codes
template bool berkeleydb_t::table_t::put_node<ccnode_t>(const ccnode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<ccnode_t>(storable_t<ccnode_t>& node, ccnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

template berkeleydb_t::iterator<ccnode_t> berkeleydb_t::table_t::begin<ccnode_t>(const char *dbname) const; 

template class berkeleydb_t::iterator_base<ccnode_t>;
template class berkeleydb_t::iterator<ccnode_t>;

// system
template bool berkeleydb_t::table_t::put_node<sysnode_t>(const sysnode_t& node, storage_info_t& strg_info);
template bool berkeleydb_t::table_t::get_node_by_id<sysnode_t>(storable_t<sysnode_t>& node, sysnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
