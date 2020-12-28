/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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

system_database_t::system_database_t(const ::config_t& config) : berkeleydb_t(db_config_t(config)),
      system(make_table())
{
}

system_database_t::~system_database_t(void)
{
}

berkeleydb_t::status_t system_database_t::open(void)
{
   status_t status;

   // open the underlying database and register the system table pointer
   if(!(status = berkeleydb_t::open({&system})).success())
      return status;

   // open the system database and return
   return system.open("system", &bt_compare_cb<sysnode_t::s_compare_key>);
}

bool system_database_t::put_sysnode(const sysnode_t& sysnode, storage_info_t& strg_info)
{
   return system.put_node(sysnode, strg_info);
}

bool system_database_t::get_sysnode_by_id(storable_t<sysnode_t>& sysnode, sysnode_t::s_unpack_cb_t<> upcb) const
{
   return system.get_node_by_id(sysnode, upcb);
}

///
/// @name   Database Schema
///
/// Most tables (primary databases) contain no references to other tables and are
/// structured this way:
/// ```
/// * node ID        - A numeric key obtained from sequence database with the `.seq` 
///                    suffix. Node ID is stored as a Berkeley DB key and not within
///                    node data. Some tables derive their node ID from the data,
///                    such as those that store country and city data.
///
/// * node version   - A numeric version of the node data layout.
/// * node value     - UTF-8 character value (e.g. URL, User Agent, etc)
/// * value hash     - Hash of the node value to facilitate value look-ups in value
///                    databases with the `.values` suffix without value duplication
///                    between the primary and the secondary values database.
/// * node data      - Node data serialized according to the node version.
/// ```
/// Tables listed below contain references to other tables.
///
/// **visits.active**
///
/// This table contains active visits that have not yet ended and reference these
/// tables:
/// ```
/// hosts            - Visit ID is the same as the corresponding host ID
/// urls             - Contains the node ID of the last visited URL
/// ```
///
/// **downloads.active**
///
/// This table contains active downloads that have yet not been completed. 
/// ```
/// download node ID - Active download ID is the same as the download ID
/// ```
///
/// **downloads**
///
/// This table contains all completed downloads.
/// ```
/// hosts            - Contains the node ID of the host for this download
/// downloads.active - If a download is marked as active, download ID is the 
///                    same as the active download ID
/// ```
/// @{

///
/// An array of primary table descriptors that contains all data to set up
/// all primary tables without indexes.
///
const struct database_t::table_desc_t {
   berkeleydb_t::table_t   database_t::*table;        ///< Pointer to a data member table for a given database.
   const char              *primary_db;               ///< Name of the primary database.
   bt_compare_cb_t         key_compare_cb;            ///< Compares two primary keys.

   const char              *sequence_db;              ///< Name of the sequence database or `nullptr` if it's not used.

   const char              *value_db;                 ///< Name of the value database or `nullptr` if it's not used.
   bt_compare_cb_t         value_hash_compare_cb;     ///< Compares two value hashes in secondary database keys.
   sc_extract_cb_t         value_hash_extract_cb;     ///< Extracts value hash from serialized node data.
} database_t::table_desc[] = {
   {&database_t::system, "system", 
         &bt_compare_cb<sysnode_t::s_compare_key>},
   {&database_t::urls, "urls", 
         &bt_compare_cb<unode_t::s_compare_key>, 
         "urls.seq",
         "urls.values", &bt_compare_cb<unode_t::s_compare_value_hash>, &sc_extract_cb<unode_t::s_field_value_hash>},
   {&database_t::hosts, "hosts", 
         &bt_compare_cb<hnode_t::s_compare_key>, 
         "hosts.seq",
         "hosts.values", &bt_compare_cb<hnode_t::s_compare_value_hash>, &sc_extract_cb<hnode_t::s_field_value_hash>},
   {&database_t::visits, "visits.active", 
         &bt_compare_cb<vnode_t::s_compare_key>},
   {&database_t::downloads, "downloads", 
         &bt_compare_cb<dlnode_t::s_compare_key>,
         "downloads.seq", 
         "downloads.values", &bt_compare_cb<dlnode_t::s_compare_value_hash>, &sc_extract_cb<dlnode_t::s_field_value_hash>},
   {&database_t::active_downloads, "downloads.active", 
         &bt_compare_cb<danode_t::s_compare_key>},
   {&database_t::agents, "agents", 
         &bt_compare_cb<anode_t::s_compare_key>,
         "agents.seq", 
         "agents.values", &bt_compare_cb<anode_t::s_compare_value_hash>, &sc_extract_cb<anode_t::s_field_value_hash>},
   {&database_t::referrers, "referrers", 
         &bt_compare_cb<rnode_t::s_compare_key>,
         "referrers.seq", 
         "referrers.values", &bt_compare_cb<rnode_t::s_compare_value_hash>, &sc_extract_cb<rnode_t::s_field_value_hash>},
   {&database_t::search, "search", 
         &bt_compare_cb<snode_t::s_compare_key>,
         "search.seq", 
         "search.values", &bt_compare_cb<snode_t::s_compare_value_hash>, &sc_extract_cb<snode_t::s_field_value_hash>},
   {&database_t::users, "users", 
         &bt_compare_cb<inode_t::s_compare_key>,
         "users.seq",
         "users.values", &bt_compare_cb<inode_t::s_compare_value_hash>, &sc_extract_cb<inode_t::s_field_value_hash>},
   {&database_t::errors, "errors", 
         &bt_compare_cb<rcnode_t::s_compare_key>,
         "errors.seq",
         "errors.values", bt_compare_cb<rcnode_t::s_compare_value_hash>, &sc_extract_cb<rcnode_t::s_field_value_hash>},
   {&database_t::scodes, "statuscodes", 
         &bt_compare_cb<scnode_t::s_compare_key>},
   {&database_t::daily, "totals.daily", 
         &bt_compare_cb<daily_t::s_compare_key>},
   {&database_t::hourly, "totals.hourly", 
         &bt_compare_cb<hourly_t::s_compare_key>},
   {&database_t::totals, "totals", 
         &bt_compare_cb<totals_t::s_compare_key>},
   {&database_t::countries, "countries", 
         &bt_compare_cb<ccnode_t::s_compare_key>},
   {&database_t::cities, "cities", 
         &bt_compare_cb<ctnode_t::s_compare_key>},
   {&database_t::asn, "asn", 
         &bt_compare_cb<asnode_t::s_compare_key>}
};

///
/// An array of index descriptors that contains all data to set up all indexes
/// for each primary table.
///
/// Secondary databases have a key that is some data from the primary database, such as
/// the number of pages or transfer amount, and its data is the primary key that links
/// secondary database records to their primary database records. The primary key for
/// most tables with indexes (secondary databases) is a node ID, which means that the
/// duplicate compare callback will only have node IDs in the data items passed into the
/// callback. This makes it impossible to sort duplicate secondary database values by
/// anything, but their order of insertion in the pkrimary database, which is how the 
/// node ID works out. 
///
const struct database_t::index_desc_t {
   table_t           database_t::*table;              ///< Pointer to a data member table for a given database.
   const char        *index_db;                       ///< Name of the secondary database. 

   bt_compare_cb_t   index_compare_cb;                ///< Compares two serialized secondary database keys.
   bt_compare_cb_t   index_dup_compare_cb;            ///< Compares two serialized nodes to sort duplicate secondary database keys (see above).
   sc_extract_cb_t   index_extract_cb;                ///< Extracts a secondary database key from a serialized node.
} database_t::index_desc[] = {
   // urls
   {&database_t::urls, "urls.hits", 
         &bt_compare_cb<unode_t::s_compare_hits>, &bt_compare_cb<unode_t::s_compare_key>, &sc_extract_cb<unode_t::s_field_hits>},
   {&database_t::urls, "urls.xfer", 
         &bt_compare_cb<unode_t::s_compare_xfer>, &bt_compare_cb<unode_t::s_compare_key>, &sc_extract_cb<unode_t::s_field_xfer>},
   {&database_t::urls, "urls.entry", 
         &bt_compare_cb<unode_t::s_compare_entry>, &bt_compare_cb<unode_t::s_compare_key>, &sc_extract_cb<unode_t::s_field_entry>},
   {&database_t::urls, "urls.exit", 
         &bt_compare_cb<unode_t::s_compare_exit>, &bt_compare_cb<unode_t::s_compare_key>, &sc_extract_cb<unode_t::s_field_exit>},
   {&database_t::urls, "urls.groups.hits",
         &bt_compare_cb<unode_t::s_compare_hits>, &bt_compare_cb<unode_t::s_compare_key>, &sc_extract_group_cb<unode_t, unode_t::s_field_hits>},
   {&database_t::urls, "urls.groups.xfer", 
         &bt_compare_cb<unode_t::s_compare_xfer>, &bt_compare_cb<unode_t::s_compare_key>, sc_extract_group_cb<unode_t, unode_t::s_field_xfer>},
   // hosts
   {&database_t::hosts, "hosts.hits", 
         &bt_compare_cb<hnode_t::s_compare_hits>, &bt_compare_cb<hnode_t::s_compare_key>, &sc_extract_cb<hnode_t::s_field_hits>},
   {&database_t::hosts, "hosts.xfer", 
         &bt_compare_cb<hnode_t::s_compare_xfer>, &bt_compare_cb<hnode_t::s_compare_key>, &sc_extract_cb<hnode_t::s_field_xfer>},
   {&database_t::hosts, "hosts.groups.hits",
         &bt_compare_cb<hnode_t::s_compare_hits>, &bt_compare_cb<hnode_t::s_compare_key>, &sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>},
   {&database_t::hosts, "hosts.groups.xfer", 
         &bt_compare_cb<hnode_t::s_compare_xfer>, &bt_compare_cb<hnode_t::s_compare_key>, &sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>},
   // downloads
   {&database_t::downloads, "downloads.xfer", 
         &bt_compare_cb<dlnode_t::s_compare_xfer>, &bt_compare_cb<dlnode_t::s_compare_key>, &sc_extract_cb<dlnode_t::s_field_xfer>},
   // agents
   {&database_t::agents, "agents.hits", 
         &bt_compare_cb<anode_t::s_compare_hits>, &bt_compare_cb<anode_t::s_compare_key>, &sc_extract_cb<anode_t::s_field_hits>},
   {&database_t::agents, "agents.visits", 
         &bt_compare_cb<anode_t::s_compare_visits>, &bt_compare_cb<anode_t::s_compare_key>, &sc_extract_cb<anode_t::s_field_visits>},
   {&database_t::agents, "agents.groups.visits", 
         &bt_compare_cb<anode_t::s_compare_hits>, &bt_compare_cb<anode_t::s_compare_key>, &sc_extract_group_cb<anode_t, anode_t::s_field_visits>},
   // referrers
   {&database_t::referrers, "referrers.hits", 
         &bt_compare_cb<rnode_t::s_compare_hits>, &bt_compare_cb<rnode_t::s_compare_key>, &sc_extract_cb<rnode_t::s_field_hits>},
   {&database_t::referrers, "referrers.groups.hits", 
         &bt_compare_cb<rnode_t::s_compare_hits>, &bt_compare_cb<rnode_t::s_compare_key>, &sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>},
   // search
   {&database_t::search, "search.hits", 
         &bt_compare_cb<snode_t::s_compare_hits>, &bt_compare_cb<snode_t::s_compare_key>, &sc_extract_cb<snode_t::s_field_hits>},
   // users
   {&database_t::users, "users.hits", 
         &bt_compare_cb<inode_t::s_compare_hits>, &bt_compare_cb<inode_t::s_compare_key>, &sc_extract_cb<inode_t::s_field_hits>},
   {&database_t::users, "users.groups.hits", 
         &bt_compare_cb<inode_t::s_compare_hits>, &bt_compare_cb<inode_t::s_compare_key>, &sc_extract_group_cb<inode_t, inode_t::s_field_hits>},
   // errors
   {&database_t::errors, "errors.hits", 
         &bt_compare_cb<rcnode_t::s_compare_hits>, &bt_compare_cb<rcnode_t::s_compare_key>, &sc_extract_cb<rcnode_t::s_field_hits>},
   // countries
   {&database_t::countries, "countries.visits", 
         &bt_compare_cb<ccnode_t::s_compare_visits>, &bt_reverse_compare_cb<ccnode_t::s_compare_key>, &sc_extract_cb<ccnode_t::s_field_visits>},
   // cities
   {&database_t::cities, "cities.visits", 
         &bt_compare_cb<ctnode_t::s_compare_visits>, &bt_reverse_compare_cb<ctnode_t::s_compare_key>, &sc_extract_cb<ctnode_t::s_field_visits>},
   // asn
   {&database_t::asn, "asn.visits", 
         &bt_compare_cb<asnode_t::s_compare_visits>, &bt_reverse_compare_cb<asnode_t::s_compare_key>, &sc_extract_cb<asnode_t::s_field_visits>}
};

/// @}

// -----------------------------------------------------------------------
//
// database_t
//
// -----------------------------------------------------------------------

database_t::database_t(const ::config_t& config) : berkeleydb_t(db_config_t(config)),
      system(make_table()),
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
      cities(make_table()),
      asn(make_table())
{
}

database_t::~database_t(void)
{
}

berkeleydb_t::status_t database_t::attach_indexes(bool rebuild)
{
   status_t status;

   // attach all registered indexes
   for(size_t i = 0; i < sizeof(index_desc)/sizeof(index_desc[0]); i++) {
      if(!(status = (this->*index_desc[i].table).associate(index_desc[i].index_db, index_desc[i].index_extract_cb, rebuild)).success())
         return status;
   }

   return status;
}

berkeleydb_t::status_t database_t::open(void)
{
   status_t status;

   // initialize the table list for the underlying database layer
   std::initializer_list<table_t*> tblist = {&system,
               &urls, &hosts, &visits, &downloads, &active_downloads, &agents,
               &referrers, &search, &users, &errors, &scodes, &daily, &hourly,
               &totals, &countries, &cities, &asn};

   if(!(status = berkeleydb_t::open(tblist)).success())
      return status;

   //
   // confgure all databases
   //

   //
   // Secondary databases used for indexes must be initialized first because some 
   // of the callbacks, such as the duplicate compare callback, must be called 
   // before the database is opened. Otherwise BDB gets confused and crashes on a 
   // `nullptr` when indexes are attached later, after all logs are processed.
   //
   for(size_t i = 0; i < sizeof(index_desc)/sizeof(index_desc[0]); i++) {
      if(!(status = (this->*index_desc[i].table).associate(
                                                      index_desc[i].index_db, 
                                                      index_desc[i].index_compare_cb, 
                                                      index_desc[i].index_dup_compare_cb, 
                                                      nullptr)).success())
         return status;
   }

   //
   // Set up the primary databases and the associated value and sequence databases 
   // for those tables that have them configured.
   //
   for(size_t i = 0; i < sizeof(table_desc)/sizeof(table_desc[0]); i++) {
      // open the sequence database only if we intend to generate new record identifiers
      if(table_desc[i].sequence_db) {
         if(!(status = (this->*table_desc[i].table).open_sequence(table_desc[i].sequence_db, config.get_db_seq_cache_size())).success())
            return status;
      }

      // open a value database if this table has one
      if(table_desc[i].value_db) {
         //
         // The duplicate value hash comparison callback is the same as the value hash 
         // comparison function just to indicate that there may be hash collisions, so
         // the secondary database is set up to allow duplicates. The value database is
         // never traversed by value, so record ordering defined by these callbacks is 
         // is irrelevant and for value look-ups all duplicates are examined until a
         // matching value is found.
         //
         if(!(status = (this->*table_desc[i].table).associate(table_desc[i].value_db, table_desc[i].value_hash_compare_cb, table_desc[i].value_hash_compare_cb, table_desc[i].value_hash_extract_cb)).success())
            return status;

         // configure the secondary values database for this table
         (this->*table_desc[i].table).set_values_db(table_desc[i].value_db);
      }

      // open the primary database that contains all data
      if(!(status = (this->*table_desc[i].table).open(table_desc[i].primary_db, table_desc[i].key_compare_cb)).success())
         return status;
   }

   return status;
}

//
// unode
//
bool database_t::put_unode(const unode_t& unode, storage_info_t& strg_info)
{
   return urls.put_node<unode_t>(unode, strg_info);
}

bool database_t::get_unode_by_id(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t<> upcb) const
{
   return urls.get_node_by_id(unode, upcb);
}

bool database_t::get_unode_by_value(storable_t<unode_t>& unode, unode_t::s_unpack_cb_t<> upcb) const
{
   return urls.get_node_by_value(unode, upcb);
}

//
// hnode
//
bool database_t::put_hnode(const hnode_t& hnode, storage_info_t& strg_info)
{
   return hosts.put_node<hnode_t>(hnode, strg_info);
}

//
// vnode
//
bool database_t::put_vnode(const vnode_t& vnode, storage_info_t& strg_info)
{
   return visits.put_node<vnode_t>(vnode, strg_info);
}

bool database_t::get_vnode_by_id(storable_t<vnode_t>& vnode, vnode_t::s_unpack_cb_t<storable_t<unode_t>&> upcb, storable_t<unode_t>& unode) const
{
   return visits.get_node_by_id<vnode_t, storable_t<unode_t>&>(vnode, upcb, unode);
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

bool database_t::get_danode_by_id(storable_t<danode_t>& danode, danode_t::s_unpack_cb_t<> upcb) const
{
   return active_downloads.get_node_by_id(danode, upcb);
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

bool database_t::get_anode_by_id(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t<> upcb) const
{
   return agents.get_node_by_id(anode, upcb);
}

bool database_t::get_anode_by_value(storable_t<anode_t>& anode, anode_t::s_unpack_cb_t<> upcb) const
{
   return agents.get_node_by_value(anode, upcb);
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

bool database_t::get_rnode_by_id(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t<> upcb) const
{
   return referrers.get_node_by_id(rnode, upcb);
}

bool database_t::get_rnode_by_value(storable_t<rnode_t>& rnode, rnode_t::s_unpack_cb_t<> upcb) const
{
   return referrers.get_node_by_value(rnode, upcb);
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

bool database_t::get_snode_by_id(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t<> upcb) const
{
   return search.get_node_by_id(snode, upcb);
}

bool database_t::get_snode_by_value(storable_t<snode_t>& snode, snode_t::s_unpack_cb_t<> upcb) const
{
   return search.get_node_by_value(snode, upcb);
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

bool database_t::get_inode_by_id(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t<> upcb) const
{
   return users.get_node_by_id(inode, upcb);
}

bool database_t::get_inode_by_value(storable_t<inode_t>& inode, inode_t::s_unpack_cb_t<> upcb) const
{
   return users.get_node_by_value(inode, upcb);
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

bool database_t::get_rcnode_by_id(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t<> upcb) const
{
   return errors.get_node_by_id(rcnode, upcb);
}

bool database_t::get_rcnode_by_value(storable_t<rcnode_t>& rcnode, rcnode_t::s_unpack_cb_t<> upcb) const
{
   return errors.get_node_by_value(rcnode, upcb);
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

bool database_t::get_scnode_by_id(storable_t<scnode_t>& scnode, scnode_t::s_unpack_cb_t<> upcb) const
{
   return scodes.get_node_by_id(scnode, upcb);
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

bool database_t::get_tdnode_by_id(storable_t<daily_t>& tdnode, daily_t::s_unpack_cb_t<> upcb) const
{
   return daily.get_node_by_id(tdnode, upcb);
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

bool database_t::get_thnode_by_id(storable_t<hourly_t>& thnode, hourly_t::s_unpack_cb_t<> upcb) const
{
   return hourly.get_node_by_id(thnode, upcb);
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

bool database_t::get_tgnode_by_id(storable_t<totals_t>& tgnode, totals_t::s_unpack_cb_t<> upcb) const
{
   return totals.get_node_by_id(tgnode, upcb);
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

bool database_t::get_ccnode_by_id(storable_t<ccnode_t>& ccnode, ccnode_t::s_unpack_cb_t<> upcb) const
{
   return countries.get_node_by_id(ccnode, upcb);
}

// -----------------------------------------------------------------------
//
// cities
//
// -----------------------------------------------------------------------
bool database_t::put_ctnode(const ctnode_t& ctnode, storage_info_t& strg_info)
{
   return cities.put_node<ctnode_t>(ctnode, strg_info);
}

bool database_t::get_ctnode_by_id(storable_t<ctnode_t>& ctnode, ctnode_t::s_unpack_cb_t<> upcb) const
{
   return cities.get_node_by_id(ctnode, upcb);
}

// -----------------------------------------------------------------------
//
// asn
//
// -----------------------------------------------------------------------
bool database_t::put_asnode(const asnode_t& asnode, storage_info_t& strg_info)
{
   return asn.put_node<asnode_t>(asnode, strg_info);
}

bool database_t::get_asnode_by_id(storable_t<asnode_t>& asnode, asnode_t::s_unpack_cb_t<> upcb) const
{
   return asn.get_node_by_id(asnode, upcb);
}

// -----------------------------------------------------------------------
//
// system
//
// -----------------------------------------------------------------------

bool database_t::put_sysnode(const sysnode_t& sysnode, storage_info_t& strg_info)
{
   return system.put_node(sysnode, strg_info);
}

bool database_t::get_sysnode_by_id(storable_t<sysnode_t>& sysnode, sysnode_t::s_unpack_cb_t<> upcb) const
{
   return system.get_node_by_id(sysnode, upcb);
}

#include "database_tmpl.cpp"
