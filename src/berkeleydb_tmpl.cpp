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

#include "serialize.h"
#include "exception.h"

#include <type_traits>
#include <utility>

#include "berkeleydb.h"

//
//
//
#define DBFLAGS               ((u_int32_t) 0)
#define DBENVFLAGS            ((u_int32_t) 0)

#define FILEMASK              0664              // database file access mask (rw-rw-r--)

const size_t DBBUFSIZE = 32768;                 // shared buffer for key and data

//
// B-Tree comparison function template for BDB v6 and up (top) and for prior versions 
// (bottom). Only one will be used in any build, but it's cleaner without having to 
// use conditional blocks.
//
template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   int64_t diff = compare(dbt1->get_data(), dbt2->get_data());
   return diff < 0ll ? -1 : diff > 0ll ? 1 : 0;
}

template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2)
{
   int64_t diff = compare(dbt1->get_data(), dbt2->get_data());
   return diff < 0ll ? -1 : diff > 0ll ? 1 : 0;
}

//
// B-Tree comparison function template (reverse order)
//
template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   return -compare(dbt1->get_data(), dbt2->get_data());
}

template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2)
{
   return -compare(dbt1->get_data(), dbt2->get_data());
}

//
// B-Tree field extraction function template
//
template <s_field_cb_t extract>
int sc_extract_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result)
{
   size_t dsize;
   const void *dptr = extract(data->get_data(), data->get_size(), dsize);
   result->set_data(const_cast<void*>(dptr));
   result->set_size((u_int32_t) dsize);
   return 0;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::cursor_iterator_base
//
// -----------------------------------------------------------------------

berkeleydb_t::cursor_iterator_base::cursor_iterator_base(Db *db) 
{
   cursor = NULL;
   error = 0;

   if(db) {
      if((error = db->cursor(NULL, &cursor, 0)) != 0)
         cursor = NULL;
   }
}

berkeleydb_t::cursor_iterator_base::~cursor_iterator_base(void) 
{
   close();
}

void berkeleydb_t::cursor_iterator_base::close(void) 
{
   if(cursor)
      error = cursor->close();
   cursor = NULL;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::cursor_iterator
//
// -----------------------------------------------------------------------

bool berkeleydb_t::cursor_iterator::set(Dbt& key, Dbt& data, Dbt *pkey)
{
   if(!cursor || is_error())
      return false;

   count = 0;

   if(pkey)
      error = cursor->pget(&key, pkey, &data, DB_SET);
   else
      error = cursor->get(&key, &data, DB_SET);
      
   if(error)
      return false;

   if((error = cursor->count(&count, 0)) != 0)
      return false;

   // account for one record returned in this call
   count--;

   return true;
}

bool berkeleydb_t::cursor_iterator::next(Dbt& key, Dbt& data, Dbt *pkey, bool dupkey)
{
   if(!cursor || is_error())
      return false;

   if(count) {
      // we still have duplicates - get the next one
      if(pkey)
         error = cursor->pget(&key, pkey, &data, DB_NEXT_DUP);
      else
         error = cursor->get(&key, &data, DB_NEXT_DUP);

      if(error)
         return false;
          
      count--;
   }
   else {
      // return if a duplicate key is requested
      if(dupkey) {
         error = DB_NOTFOUND;
         return false;
      }

      if(pkey)
         error = cursor->pget(&key, pkey, &data, DB_NEXT);
      else
         error = cursor->get(&key, &data, DB_NEXT);

      if(error)
         return false;

      // grab the total number of duplicates for this key
      if((error = cursor->count(&count, 0)) != 0) {
         count = 0;
         return false;
      }

      count--;
   }

   return true;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::cursor_reverse_iterator
//
// -----------------------------------------------------------------------

bool berkeleydb_t::cursor_reverse_iterator::prev(Dbt& key, Dbt& data, Dbt *pkey)
{
   if(!cursor || is_error())
      return false;

   if(pkey)
      error = cursor->pget(&key, pkey, &data, DB_PREV);
   else
      error = cursor->get(&key, &data, DB_PREV);

   return (!error) ? true : false;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::table_t
//
// -----------------------------------------------------------------------

berkeleydb_t::table_t::table_t(DbEnv& dbenv, Db& seqdb, buffer_allocator_t& buffer_allocator) :
      dbenv(&dbenv),
      table(new_db(&dbenv, DBFLAGS)),
      values(NULL),
      seqdb(&seqdb),
      sequence(NULL),
      buffer_allocator(&buffer_allocator),
      threaded(false)
{
}

berkeleydb_t::table_t::table_t(table_t&& other) :
      dbenv(other.dbenv),
      table(other.table),
      values(other.values),
      seqdb(other.seqdb),
      sequence(other.sequence),
      indexes(std::move(other.indexes)),
      buffer_allocator(other.buffer_allocator),
      threaded(other.threaded)
{
   other.dbenv = NULL;
   other.table = NULL;
   other.values = NULL;
   other.seqdb = NULL;
   other.sequence = NULL;
   other.buffer_allocator = NULL;
}

berkeleydb_t::table_t::~table_t(void)
{
   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].scdb)
         delete_db(indexes[index].scdb);
   }

   if(sequence)
      delete_db_sequence(sequence);

   if(table)
      delete_db(table);
}

berkeleydb_t::table_t& berkeleydb_t::table_t::operator = (table_t&& other)
{
   dbenv = other.dbenv;
   table = other.table;
   values = other.values;
   seqdb = other.seqdb;
   sequence = other.sequence;
   buffer_allocator = other.buffer_allocator;
   indexes = std::move(other.indexes);

   threaded = other.threaded;

   other.dbenv = NULL;
   other.table = NULL;
   other.values = NULL;
   other.seqdb = NULL;
   other.sequence = NULL;
   other.buffer_allocator = NULL;

   return *this;
}

void berkeleydb_t::table_t::init_db_handles(void)
{
   Db *scdb;

   // construct secondary database handles
   for(u_int index = 0; index < indexes.size(); index++) {
      if((scdb = indexes[index].scdb) == NULL) 
         continue;

      new (scdb) Db(dbenv, DBFLAGS);
   }

   // construct the primary database
   new (table) Db(dbenv, DBFLAGS);
}

void berkeleydb_t::table_t::destroy_db_handles(void)
{
   Db *scdb;

   // destroy secondary database handles
   for(u_int index = 0; index < indexes.size(); index++) {
      if((scdb = indexes[index].scdb) == NULL) 
         continue;

      scdb->Db::~Db();
   }

   // destroy the primary database
   table->Db::~Db();
}

const berkeleydb_t::table_t::db_desc_t *berkeleydb_t::table_t::get_sc_desc(const char *dbname) const
{
   if(!dbname || !*dbname)
      return NULL;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return NULL;
}

berkeleydb_t::table_t::db_desc_t *berkeleydb_t::table_t::get_sc_desc(const char *dbname)
{
   if(!dbname || !*dbname)
      return NULL;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return NULL;
}

int berkeleydb_t::table_t::open(const char *dbpath, const char *dbname, bt_compare_cb_t btcb)
{
   u_int index;
   int error;
   u_int32_t flags = DB_CREATE;

   if(threaded)
      flags |= DB_THREAD;

   // open all secondary databases first
   for(index = 0; index < indexes.size(); index++) {
      if(!indexes[index].scdb) 
         continue;

      if((error = indexes[index].scdb->open(NULL, indexes[index].dbpath, indexes[index].dbname, DB_BTREE, flags, FILEMASK)) != 0)
         return error;
   }

   // then configure and open the primary database
   if((error = table->set_bt_compare(btcb)) != 0)
      return error;

   // set little endian byte order
   if((error = table->set_lorder(1234)) != 0)
      return error;

   if((error = table->open(NULL, dbpath, dbname, DB_BTREE, flags, FILEMASK)) != 0)
      return error;

   // associate all secondary databases with a non-NULL data extraction callback
   for(index = 0; index < indexes.size(); index++) {
      // if the extraction callback is not NULL, associate immediately
      if(indexes[index].scdb && indexes[index].scxcb) {
         if((error = table->associate(NULL, indexes[index].scdb, indexes[index].scxcb, 0)) != 0)
            return error;
      }
   }

   return 0;
}

int berkeleydb_t::table_t::close(void)
{
   int error;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].scdb) {
         if((error = indexes[index].scdb->close(0)) != 0)
            return error;
         delete_db(indexes[index].scdb);
      }
   }

   indexes.clear();

   values = NULL;

   if(sequence) {
      if((error = sequence->close(0)) != 0)
         return error;
      delete_db_sequence(sequence);
      sequence = NULL;
   }

   return table->close(0);
}

int berkeleydb_t::table_t::truncate(u_int32_t *count)
{
   u_int32_t temp;
   return table->truncate(NULL, count ? count : &temp, 0);
}

int berkeleydb_t::table_t::compact(u_int& bytes)
{
   DB_COMPACT c_data;
   int error;
   u_int32_t pagesize;
   
   bytes = 0;

   // first, compact all index databases
   for(u_int index = 0; index < indexes.size(); index++) {
      // get the page size
      if((error = indexes[index].scdb->get_pagesize(&pagesize)) != 0)
         return error;
      
      // fill the stats structure with zeros
      memset(&c_data, 0, sizeof(c_data));

      // and compact the database
      if((error = indexes[index].scdb->compact(NULL, NULL, NULL, &c_data, DB_FREE_SPACE, NULL)) != 0)
         return error;
      
      // update the byte count   
      bytes += c_data.compact_pages_truncated * pagesize;
   }
   
   // do the same for the primary table database
   if((error = table->get_pagesize(&pagesize)) != 0)
      return error;

   memset(&c_data, 0, sizeof(c_data));

   if((error = table->compact(NULL, NULL, NULL, &c_data, DB_FREE_SPACE, NULL)) != 0)
      return error;
      
   bytes += c_data.compact_pages_truncated * pagesize;
   
   return 0;
}

int berkeleydb_t::table_t::sync(void)
{
   int error;

   for(u_int index = 0; index < indexes.size(); index++) {
      if((error = indexes[index].scdb->sync(0)) != 0)
         return error;
   }

   return table->sync(0);
}

///
/// Creates a secondary database and sets it up with the B-tree comparison callback and
/// an optional duplicate key callback and registers this database with the table under
/// `dbname`. The new secondary database is not associated with the primary database at
/// this point. 
///
/// If the secondary database key extraction callback is provided in this call, the 
/// subsequent `table_t::open` call will associate the secondary database created in
/// this method with the primary database. Otherwise, this secondary database will
/// need to be attached later via the second `associate` method.
///
int berkeleydb_t::table_t::associate(const char *dbpath, const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t scxcb)
{
   int error;
   Db *scdb = new_db(dbenv, DBFLAGS);

   // set little endian byte order
   if((error = scdb->set_lorder(1234)) != 0)
      goto errexit;

   if(dpcb) {
      // if a duplicate comparison callback provided, allow duplicates
      if((error = scdb->set_flags(DB_DUPSORT)) != 0)
         goto errexit;

      if((error = scdb->set_dup_compare(dpcb)) != 0)
         goto errexit;
   }

   // set the key comparison callback for the secondary database
   if((error = scdb->set_bt_compare(btcb)) != 0)
      goto errexit;

   indexes.push_back(db_desc_t(scdb, dbname, dbpath, scxcb));

   return 0;

errexit:
   delete_db(scdb);
   return error;
}

///
/// For performance reasons, it works faster if a primary database that has a lot
/// of inserts and updates is not associated with any secondary databases. Once all
/// updates are done, all secondary databases may be attached and each secondary key
/// will be extracted only once. This method is intended for this purpose.
///
/// In order to use this method, the alternative `associate` method must be called
/// first with a NULL extraction callback to register a secondary database and then
/// this method should be called after all updates to the primary database have been
/// completed.
///
/// If `rebuild` parameter is `true`, the secondary database is truncated before it
/// is associated with the primary database. If `rebuild` is `false` and the secondary
/// database contains any entries, it will not be properly updated.
///
/// @note   Truncating the secondary database in order to rebuild the index was used
///         historically by this method and it appears to work, but the documented
///         way to rebuild secondary databases is to remove it from the pkrimary
///         database and then associate again, which cannot be done on open databases
///         and would be much more expensive than truncation. May have to be changed
///         in the future.
///
int berkeleydb_t::table_t::associate(const char *dbname, sc_extract_cb_t scxcb, bool rebuild)
{
   int error;
   u_int32_t temp;
   u_int32_t flags = rebuild ? DB_CREATE : 0;
   db_desc_t *desc = get_sc_desc(dbname);

   // skip those that have been associated
   if(!desc || desc->scxcb)
      return 0;

   if(rebuild) {
      // make sure the secondary database is empty
      if((error = desc->scdb->truncate(NULL, &temp, 0)) != 0)
         return error;
   }

   // associate the secondary, rebuilding if requested
   if((error = table->associate(NULL, desc->scdb, scxcb, flags)) != 0)
      return error;

   // mark as associated
   desc->scxcb = scxcb;

   return 0;
}

Db *berkeleydb_t::table_t::secondary_db(const char *dbname) const
{
   const db_desc_t *desc;

   if((desc = get_sc_desc(dbname)) == NULL)
      return NULL;

   return desc->scdb;
}

int berkeleydb_t::table_t::open_sequence(const char *colname, int32_t cachesize)
{
   u_int32_t flags = DB_CREATE;
   int error;
   Dbt key;

   if(threaded)
      flags |= DB_THREAD;

   if(!sequence)
      sequence = new_db_sequence(seqdb, 0);

   if((error = sequence->initial_value(1)) != 0)
      return error;

   if((error = sequence->set_cachesize(cachesize)) != 0)
      return error;

   key.set_data(const_cast<char*>(colname));
   key.set_size((u_int32_t) strlen(colname));

   return sequence->open(NULL, &key, flags);
}

db_seq_t berkeleydb_t::table_t::get_seq_id(int32_t delta)
{
   db_seq_t seqid;

   if(!sequence || sequence->get(NULL, delta, &seqid, 0))
      return -1;

   return seqid;
}

db_seq_t berkeleydb_t::table_t::query_seq_id(void)
{
   DB_SEQUENCE_STAT *db_stat;
   db_seq_t cur_seq_id;
   
   if(!sequence || sequence->stat(&db_stat, 0))
      return -1;
      
   cur_seq_id = db_stat->st_current;
   
   free(db_stat);

   return cur_seq_id;
}

uint64_t berkeleydb_t::table_t::count(const char *dbname) const
{
   DB_BTREE_STAT *stats;
   Db *dbptr = table;
   uint64_t nkeys;

   if(dbname && *dbname) {
      if((dbptr = secondary_db(dbname)) == NULL)
         return 0;
   }

   if(dbptr->stat(NULL, &stats, 0) != 0)
      return 0;

   nkeys = (uint64_t) stats->bt_nkeys;

   free(stats);

   return nkeys;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::iterator_base
//
// -----------------------------------------------------------------------

template <typename node_t>
berkeleydb_t::iterator_base<node_t>::iterator_base(buffer_allocator_t& buffer_allocator, cursor_iterator_base& cursor) : 
      buffer_allocator(&buffer_allocator), 
      cursor(cursor)
{
}

template <typename node_t>
berkeleydb_t::iterator_base<node_t>::~iterator_base(void)
{
}

template <typename node_t>
Dbt& berkeleydb_t::iterator_base<node_t>::set_dbt_buffer(Dbt& dbt, void *buffer, size_t size) const
{
   dbt.set_data(buffer);
   dbt.set_ulen((u_int32_t) size);
   dbt.set_flags(DB_DBT_USERMEM);

   return dbt;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::iterator
//
// -----------------------------------------------------------------------

template <typename node_t>
berkeleydb_t::iterator<node_t>::iterator(buffer_allocator_t& buffer_allocator, const table_t& table, const char *dbname) : 
      iterator_base<node_t>(buffer_allocator, cursor),
      cursor(dbname ? table.secondary_db(dbname) : table.primary_db())
{
   primdb = (dbname == NULL);
}

template <typename node_t>
template <typename ... param_t>
bool berkeleydb_t::iterator<node_t>::next(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   Dbt key, data, pkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   // for a secondary database we retrieve two keys
   if(buffer.capacity() < DBBUFSIZE * (primdb ? 2 : 3))
      buffer.resize(DBBUFSIZE * (primdb ? 2 : 3), 0);

   set_dbt_buffer(key, buffer, DBBUFSIZE);
   set_dbt_buffer(data, buffer + DBBUFSIZE, DBBUFSIZE);

   if(!primdb)
      set_dbt_buffer(pkey, buffer + DBBUFSIZE*2, DBBUFSIZE);

   if(!cursor.next(key, data, primdb ? NULL : &pkey))
      return false;

   if(primdb) {
      if(node.s_unpack_key(key.get_data(), key.get_size()) != key.get_size())
         return false;
   }
   else {
      if(node.s_unpack_key(pkey.get_data(), pkey.get_size()) != pkey.get_size())
         return false;
   }

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg, std::forward<param_t>(param) ...) != data.get_size())
      return false;

   node.storage_info.set_from_storage();

   return true;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::reverse_iterator
//
// -----------------------------------------------------------------------

template <typename node_t>
berkeleydb_t::reverse_iterator<node_t>::reverse_iterator(buffer_allocator_t& buffer_allocator, const table_t& table, const char *dbname) : 
      iterator_base<node_t>(buffer_allocator, cursor), 
      cursor(dbname ? table.secondary_db(dbname) : table.primary_db())
{
   primdb = (dbname == NULL);
}

template <typename node_t>
template <typename ... param_t>
bool berkeleydb_t::reverse_iterator<node_t>::prev(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   Dbt key, data, pkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   // for a secondary database we retrieve two keys
   if(buffer.capacity() < DBBUFSIZE * (primdb ? 2 : 3))
      buffer.resize(DBBUFSIZE * (primdb ? 2 : 3), 0);

   set_dbt_buffer(key, buffer, DBBUFSIZE);
   set_dbt_buffer(data, buffer + DBBUFSIZE, DBBUFSIZE);

   if(!primdb)
      set_dbt_buffer(pkey, buffer + DBBUFSIZE*2, DBBUFSIZE);

   if(!cursor.prev(key, data, primdb ? NULL : &pkey))
      return false;

   if(primdb) {
      if(node.s_unpack_key(key.get_data(), key.get_size()) != key.get_size())
         return false;
   }
   else {
      if(node.s_unpack_key(pkey.get_data(), pkey.get_size()) != pkey.get_size())
         return false;
   }

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg, std::forward<param_t>(param) ...) != data.get_size())
      return false;

   node.storage_info.set_from_storage();

   return true;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::table_t
//
// -----------------------------------------------------------------------

template <typename node_t>
bool berkeleydb_t::table_t::put_node(const node_t& node, storage_info_t& storage_info)
{
   Dbt key, data;
   size_t keysize = node.s_key_size(), datasize = node.s_data_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   if(buffer.capacity() < keysize+datasize)
      buffer.resize(keysize+datasize, 0);

   if(node.s_pack_key(buffer, keysize) != keysize)
      return false;

   if(node.s_pack_data(buffer+keysize, datasize) != datasize)
      return false;

   key.set_data(buffer);
   key.set_size((u_int32_t) keysize);

   data.set_data(buffer+keysize);
   data.set_size((u_int32_t) datasize);

   if(table->put(NULL, &key, &data, 0)) 
      return false;

   // indicate that the node came from the database
   storage_info.set_from_storage();

   return true;
}

template <typename node_t, typename ... param_t>
bool berkeleydb_t::table_t::get_node_by_id(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const
{
   Dbt key, pkey, data;
   size_t keysize = node.s_key_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   if(buffer.capacity() < keysize+DBBUFSIZE)
      buffer.resize(keysize+DBBUFSIZE, 0);

   if(node.s_pack_key(buffer, keysize) != keysize)
      return false;

   key.set_data(buffer);
   key.set_size((u_int32_t) keysize);
   key.set_ulen((u_int32_t) keysize);
   key.set_flags(DB_DBT_USERMEM);

   data.set_data(buffer+keysize);
   data.set_ulen((u_int32_t) (DBBUFSIZE-keysize));
   data.set_flags(DB_DBT_USERMEM);

   if(table->get(NULL, &key, &data, 0))
      return false;

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg, std::forward<param_t>(param) ...) != data.get_size())
      return false;
   
   node.storage_info.set_from_storage();

   return true;
}

template <typename node_t, typename ... param_t>
bool berkeleydb_t::table_t::get_node_by_value(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param) const
{
   Dbt key, pkey, data;
   u_int32_t keysize = (u_int32_t) node.s_key_size();
   uint64_t hashkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   if(buffer.capacity() < keysize+DBBUFSIZE)
      buffer.resize(keysize+DBBUFSIZE, 0);

   if(values == NULL)
      return false;

   // make a value key
   hashkey = node.s_hash_value();
   keysize = (u_int32_t) node.s_hash_value_size();

   key.set_data(&hashkey);
   key.set_size(keysize);

   // share the key buffer between the key and the value
   pkey.set_data(buffer);
   pkey.set_ulen(keysize);
   pkey.set_flags(DB_DBT_USERMEM);

   data.set_data(buffer+keysize);
   data.set_ulen((u_int32_t) (DBBUFSIZE-keysize));
   data.set_flags(DB_DBT_USERMEM);

   // open a cursor
   {cursor_iterator cursor(values);

   // find the first value hash and get the primary key and value data
   if(!cursor.set(key, data, &pkey))
      return false;

   do {
      // if the node value matched, break out
      if(!node.s_compare_value(data.get_data(), data.get_size()))
         break;

      // if there are no more duplicates, value is not found
      if(!cursor.dup_count())
         return false;

   } while(cursor.next(key, data, &pkey, true));

   // the destructor will close the cursor
   if(cursor.get_error())
      return false;
   }

   // unpack the primary key
   if(node.s_unpack_key(pkey.get_data(), pkey.get_size()) != pkey.get_size())
      return false;

   // unpack data
   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg, std::forward<param_t>(param) ...) != data.get_size())
      return false;

   node.storage_info.set_from_storage();

   return true;
}

bool berkeleydb_t::table_t::delete_node(const keynode_t<uint64_t>& node)
{
   Dbt key;
   size_t keysize = node.s_key_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   if(buffer.capacity() < keysize)
      buffer.resize(keysize, 0);

   if(node.s_pack_key(buffer, keysize) != keysize)
      return false;

   key.set_data(buffer);
   key.set_size((u_int32_t) keysize);

   if(table->del(NULL, &key, 0))
      return false;

   return true;
}

template <typename node_t>
berkeleydb_t::iterator<node_t> berkeleydb_t::table_t::begin(const char *dbname) const 
{
   return iterator<node_t>(*buffer_allocator, *this, dbname);
}

template <typename node_t>
berkeleydb_t::reverse_iterator<node_t> berkeleydb_t::table_t::rbegin(const char *dbname) const 
{
   return reverse_iterator<node_t>(*buffer_allocator, *this, dbname);
}

// -----------------------------------------------------------------------
//
// berkeleydb_t
//
// -----------------------------------------------------------------------
berkeleydb_t::berkeleydb_t(config_t&& config) :
      config(config.clone()),
      dbenv(DBENVFLAGS),
      sequences(&dbenv, DBFLAGS)
{
   // configure the environment to use the correct memory manager
   if(dbenv.set_alloc(berkeleydb_t::malloc, berkeleydb_t::realloc, berkeleydb_t::free))
      throw exception_t(0, "Cannot set memory management functions for the database environment");

   // no threading at this point, we can just assign
   trickle_thread_stop = false;

   trickle = false;
}

berkeleydb_t::~berkeleydb_t()
{
   config.release();
}

void berkeleydb_t::reset_db_handles(void)
{
   //
   // Once DbEnv::close and Db::close are called, their object instances 
   // become unusable. The code in all reset_db_handles methods calls 
   // destructors and constructors explicitly, reusing existing memory.
   //

   // destroy table databases
   for(size_t i = 0; i < tables.size(); i++)
      tables[i]->destroy_db_handles();

   // destroy the sequences database
   sequences.Db::~Db();
   
   // reconstruct the environment 
   dbenv.~DbEnv();
   new (&dbenv) DbEnv(DBFLAGS);

   // construct the sequence database
   new (&sequences) Db(&dbenv, DBFLAGS);

   // construct table databases
   for(size_t i = 0; i < tables.size(); i++)
      tables[i]->init_db_handles();
}

void *berkeleydb_t::malloc(size_t size)
{
   return ::malloc(size);
}

void *berkeleydb_t::realloc(void *block, size_t size)
{
   return ::realloc(block, size);
}

void berkeleydb_t::free(void *block)
{
   ::free(block);
}

DbSequence *berkeleydb_t::new_db_sequence(Db *seqdb, u_int32_t flags)
{
   void *dbseq;
   
   if((dbseq = malloc(sizeof(DbSequence))) == NULL)
      return NULL;
   
   return new (dbseq) DbSequence(seqdb, flags);
}

void berkeleydb_t::delete_db_sequence(DbSequence *dbseq)
{
   if(dbseq) {
      dbseq->~DbSequence();
      free(dbseq);
   }
}

Db *berkeleydb_t::new_db(DbEnv *dbenv, u_int32_t flags)
{
   void *db;
   
   if((db = malloc(sizeof(Db))) == NULL)
      return NULL;
      
   return new (db) Db(dbenv, flags);
}

void berkeleydb_t::delete_db(Db *db)
{
   if(db) {
      db->~Db();
      free(db);
   }
}

berkeleydb_t::status_t berkeleydb_t::open(void)
{
   u_int32_t dbflags = DB_CREATE;
   u_int32_t envflags = DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE;
   int major, minor, patch;
   status_t status;

   db_version(&major, &minor, &patch);

   if(major < 4 || major == 4 && minor < 4)
      return string_t::_format("Berkeley DB must be v4.4 or newer (found v%d.%d.%d).\n", major, minor, patch);

   // do some additional initialization for threaded environment
   if(trickle) {
      // initialize the environment and databases as thread-safe and with a single writer
      dbflags |= DB_THREAD;
      envflags |= DB_THREAD | DB_INIT_CDB;

      // initialize table databases as free-threaded
      for(size_t i = 0; i < tables.size(); i++)
         tables[i]->set_threaded(true);
   }

   // set the temporary directory
   if(!(status = dbenv.set_tmp_dir(config.get_tmp_path())).success())
      return status;

   // if configured cache size is non-zero,
   if(config.get_db_cache_size()) {
      // set the maximum database cache size
      if(!(status = dbenv.set_cachesize(0, config.get_db_cache_size(), 0)).success())
         return status;
   }

   // disable OS buffering, if requested
   if(config.get_db_direct()) {
      if(!(status = dbenv.set_flags(DB_DIRECT_DB, 1)).success())
         return status;
   }

   // enable write-through I/O, if requested
   if(config.get_db_dsync()) {
      if(!(status = dbenv.set_flags(DB_DSYNC_DB, 1)).success())
         return status;
   }

   // open the DB environment
   if(!(status = dbenv.open(config.get_db_path(), envflags, FILEMASK)).success())
      return status;

   //
   // create the sequences database (unique node IDs)
   //
   if(!(status = sequences.open(NULL, config.get_db_path(), "sequences", DB_HASH, dbflags, FILEMASK)).success())
      return status;

   // now that everything is initialized, we can start the trickle thread
   if(trickle)
      trickle_thread = std::thread(&berkeleydb_t::trickle_thread_proc, this);

   return status;
}

berkeleydb_t::status_t berkeleydb_t::close(void)
{
   u_int errcnt = 0;
   int error = 0;
   status_t status;

   // tell trickle thread to stop
   trickle_mtx.lock();
   trickle_thread_stop = true;
   trickle_cv.notify_one();
   trickle_mtx.unlock();

   // if the trickle thread was started, wait for it to stop
   if(trickle_thread.joinable())
      trickle_thread.join();

   // close all table databases
   for(size_t i = 0; i < tables.size(); i++) {
      if((error = tables[i]->close()) != 0)
         errcnt++;

      // report the first error
      if(errcnt == 1)
         status = error;
   }

   // close the sequences database
   if((error = sequences.close(0)) != 0)
      errcnt++;

   if(errcnt == 1)
      status = error;

   // finally, close the environment
   if((error = dbenv.close(0)) != 0)
      errcnt++;

   if(errcnt == 1)
      status = error;

   reset_db_handles();

   return status;
}

berkeleydb_t::status_t berkeleydb_t::truncate(void)
{
   status_t status;

   for(size_t i = 0; i < tables.size(); i++) {
      // bail out as soon as we see the first erro
      if(!(status = tables[i]->truncate()).success())
         return status;
   }

   return status;
}

berkeleydb_t::status_t berkeleydb_t::compact(u_int& bytes)
{
   status_t status;
   u_int tbytes;

   bytes = 0;

   for(size_t i = 0; i < tables.size(); i++) {
      if(!(status = tables[i]->compact(tbytes)).success())
         return status;
      bytes += tbytes;
   }

   return status;
}

berkeleydb_t::status_t berkeleydb_t::flush(void)
{
   status_t status;

   if(!(status = dbenv.memp_sync(NULL)).success())
      return status;

   for(size_t i = 0; i < tables.size(); i++) {
      if(!(status = tables[i]->sync()).success())
         return status;
   }

   return status;
}

void berkeleydb_t::trickle_thread_proc(void)
{
   int pagecnt = 0, error;

   std::unique_lock<std::mutex> lock(trickle_mtx);

   while(!trickle_thread_stop) {
      //
      // Switch the wait time between one and five seconds, depending on whether any
      // pages were written to disk the last time.
      //
      if(trickle_cv.wait_for(lock, std::chrono::seconds(pagecnt ? 1 : 5)) == std::cv_status::timeout) {
         lock.unlock();

         //
         // Keep 10% of the pages in the memory pool clean, so when we look up previously
         // saved items, they can be read into memory without having to write existing
         // items to disk in the context of the main thread.
         //
         if((error = dbenv.memp_trickle(10, &pagecnt)) != 0) {
            trickle_error.format("Failed to trickle database cache to disk (%d)", error);
            break;
         }

         lock.lock();
      }
   }
}

