/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

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

#include "util.h"
#include "serialize.h"
#include "exception.h"

#include <type_traits>
#include <utility>

#include "berkeleydb.h"

//
//
//
#define DBFLAGS               0
#define DBENVFLAGS            0

#define FILEMASK              0664              // database file access mask (rw-rw-r--)

#define DBBUFSIZE             32768             // shared buffer for key and data

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
      threaded(false),
      readonly(false)
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
      threaded(other.threaded),
      readonly(other.readonly)
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
   readonly = other.readonly;

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
   u_int32_t flags = readonly ? DB_RDONLY : DB_CREATE;

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

   // associate all secondary databases with a non-NULL callback
   for(index = 0; index < indexes.size(); index++) {
      // if the extraction callback is not NULL, associate immediately
      if(indexes[index].scdb && indexes[index].sccb) {
         if((error = table->associate(NULL, indexes[index].scdb, indexes[index].sccb, 0)) != 0)
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
#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 4
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
#else
   return EINVAL;
#endif
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

int berkeleydb_t::table_t::associate(const char *dbpath, const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t sccb)
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

   indexes.push(db_desc_t(scdb, dbname, dbpath, sccb));

   return 0;

errexit:
   delete_db(scdb);
   return error;
}

int berkeleydb_t::table_t::associate(const char *dbname, sc_extract_cb_t sccb, bool rebuild)
{
   int error;
   u_int32_t temp;
   u_int32_t flags = rebuild ? DB_CREATE : 0;
   db_desc_t *desc = get_sc_desc(dbname);

   // skip those that have been associated
   if(!desc || desc->sccb)
      return 0;

   if(rebuild) {
      // make sure the secondary database is empty
      if((error = desc->scdb->truncate(NULL, &temp, 0)) != 0)
         return error;
   }

   // associate the secondary, rebuilding if requested
   if((error = table->associate(NULL, desc->scdb, sccb, flags)) != 0)
      return error;

   // mark as associated
   desc->sccb = sccb;

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
   dbt.set_ulen(size);
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
bool berkeleydb_t::iterator<node_t>::next(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   Dbt key, data, pkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   // for a secondary database we retrieve two keys
   buffer.resize(DBBUFSIZE * (primdb ? 2 : 3));

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

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg) != data.get_size())
      return false;

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
bool berkeleydb_t::reverse_iterator<node_t>::prev(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   Dbt key, data, pkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   // for a secondary database we retrieve two keys
   buffer.resize(DBBUFSIZE * (primdb ? 2 : 3));

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

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg) != data.get_size())
      return false;

   return true;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::table_t
//
// -----------------------------------------------------------------------

template <typename node_t>
bool berkeleydb_t::table_t::put_node(const node_t& node)
{
   Dbt key, data;
   size_t keysize = node.s_key_size(), datasize = node.s_data_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   buffer.resize(keysize+datasize);

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

   // node flags are mutable
   const_cast<node_t&>(node).dirty = false;
   const_cast<node_t&>(node).storage = true;

   return true;
}

template <typename node_t>
bool berkeleydb_t::table_t::get_node_by_id(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg) const
{
   Dbt key, pkey, data;
   size_t keysize = node.s_key_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   buffer.resize(keysize+DBBUFSIZE);

   if(node.s_pack_key(buffer, keysize) != keysize)
      return false;

   key.set_data(buffer);
   key.set_size((u_int32_t) keysize);
   key.set_ulen((u_int32_t) keysize);
   key.set_flags(DB_DBT_USERMEM);

   data.set_data(buffer+keysize);
   data.set_ulen(DBBUFSIZE);
   data.set_flags(DB_DBT_USERMEM);

   if(table->get(NULL, &key, &data, 0))
      return false;

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg) != data.get_size())
      return false;
   
   node.dirty = false;
   node.storage = true;

   return true;
}

template <typename node_t>
bool berkeleydb_t::table_t::get_node_by_value(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg) const
{
   Dbt key, pkey, data;
   u_int32_t keysize = node.s_key_size();
   uint64_t hashkey;
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   buffer.resize(keysize+DBBUFSIZE);

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
   data.set_ulen(DBBUFSIZE);
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
   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg) != data.get_size())
      return false;

   node.dirty = false;
   node.storage = true;

   return true;
}

bool berkeleydb_t::table_t::delete_node(const keynode_t<uint64_t>& node)
{
   Dbt key;
   size_t keysize = node.s_key_size();
   buffer_holder_t buffer_holder(*buffer_allocator);
   buffer_t& buffer = buffer_holder.buffer; 

   buffer.resize(keysize);

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

//
// buffer_queue_t
//
buffer_t berkeleydb_t::buffer_queue_t::get_buffer(void)
{
   buffer_t buffer;

   if(!buffers.empty()) {
      buffer = std::move(buffers.back());
      buffers.pop_back();
   }

   return buffer;
}

void berkeleydb_t::buffer_queue_t::release_buffer(buffer_t&& buffer)
{
   buffers.push_back(std::move(buffer));
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

   trickle_thread = 0;                 // $$$
   trickle_event = NULL;
   trickle_thread_stop = false;
   trickle_thread_stopped = true;

   readonly = false;
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

bool berkeleydb_t::open(void)
{
   u_int32_t dbflags = readonly ? DB_RDONLY : DB_CREATE;
   u_int32_t envflags = DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE;

   // do some additional initialization for threaded environment
   if(!readonly && trickle) {
      // initialize the environment and databases as thread-safe
      dbflags |= DB_THREAD;
      envflags |= DB_THREAD;

      // create an event to indicate that there's something to write
      if((trickle_event = event_create(true, false)) == NULL)
         return false;
   
      // create a trickle thread
      if((trickle_thread = thread_create(trickle_thread_proc, this)) == 0)
         return false;

      // initialize table databases as free-threaded
      for(size_t i = 0; i < tables.size(); i++)
         tables[i]->set_threaded(true);
   }

   // initialize table databases as read-only, if requested
   if(readonly) {
      for(size_t i = 0; i < tables.size(); i++)
         tables[i]->set_readonly(true);
   }

   // set the temporary directory
   if(dbenv.set_tmp_dir(config.get_tmp_path()))
      return false;

   // if configured cache size is non-zero,
   if(config.get_db_cache_size()) {
      // set the maximum database cache size
      if(dbenv.set_cachesize(0, config.get_db_cache_size(), 0))
         return false;

      // and the maximum memory-mapped file size (read-only mode)
      if(readonly) {
         if(dbenv.set_mp_mmapsize(config.get_db_cache_size()))
            return false;
      }
   }

   // disable OS buffering, if requested
   if(config.get_db_direct()) {
      if(dbenv.set_flags(DB_DIRECT_DB, 1))
         return false;
   }

   // enable write-through I/O, if requested
   if(config.get_db_dsync()) {
#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 4
      if(dbenv.set_flags(DB_DSYNC_DB, 1))
         return false;
#else
      fprintf(stderr, "DB_DSYNC_DB is not supported in versions of Berkeley DB prior to v4.3\n");
#endif
   }

   // open the DB environment
   if(dbenv.open(config.get_db_path(), envflags, FILEMASK))
      return false;

   //
   // create the sequences database (unique node IDs)
   //
   if(sequences.open(NULL, config.get_db_path(), "sequences", DB_HASH, dbflags, FILEMASK))
      return false;

   return true;
}

bool berkeleydb_t::close(void)
{
   u_int errcnt = 0, waitcnt = 300;

   // tell trickle thread to stop
   trickle_thread_stop = true;

   // wait for it for a while
   while(!trickle_thread_stopped && waitcnt--)
      msleep(50);

   // close all table databases
   for(size_t i = 0; i < tables.size(); i++) {
      if(tables[i]->close())
         errcnt++;
   }

   // close the sequences database
   if(sequences.close(0))
      errcnt++;

   // finally, close the environment
   if(dbenv.close(0))
      errcnt++;

   reset_db_handles();

   // destroy all trickling events
   if(trickle_event) {
      event_destroy(trickle_event);
      trickle_event = NULL;
   }

   // destroy the trickle thread
   if(trickle_thread) {
      thread_destroy(trickle_thread);
      trickle_thread = 0;
   }

   return !errcnt ? true : false;
}

bool berkeleydb_t::truncate(void)
{
   u_int errcnt = 0;

   for(size_t i = 0; i < tables.size(); i++) {
      if(tables[i]->truncate())
         errcnt++;
   }

   return !errcnt ? true : false;
}

int berkeleydb_t::compact(u_int& bytes)
{
#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 4
   int error;
   u_int tbytes;

   bytes = 0;

   for(size_t i = 0; i < tables.size(); i++) {
      if((error = tables[i]->compact(tbytes)) != 0)
         return error;
      bytes += tbytes;
   }

   return 0;
#else
   return EINVAL;
#endif
}

bool berkeleydb_t::flush(void)
{
   u_int errcnt = 0;

   if(dbenv.memp_sync(NULL))
      errcnt++;

   for(size_t i = 0; i < tables.size(); i++) {
      if(tables[i]->sync())
         errcnt++;
   }

   return !errcnt ? true : false;
}

#ifdef _WIN32
unsigned int __stdcall berkeleydb_t::trickle_thread_proc(void *arg)
#else
void *berkeleydb_t::trickle_thread_proc(void *arg)
#endif
{
   ((berkeleydb_t*) arg)->trickle_thread_proc();
   return 0;
}

void berkeleydb_t::trickle_thread_proc(void)
{
   int nwrote, error;
   uint64_t eventrc, count = 0;

   trickle_thread_stopped = false;

   while(!trickle_thread_stop) {
      if((eventrc = event_wait(trickle_event, 50)) == EVENT_TIMEOUT) {
         // trickle every second, even if event was not set
         if(++count < 20)
            continue;

         // set the event to avoid waiting
         if(!event_set(trickle_event)) {
            trickle_error = "Failed to set the database trickle event";
            break;
         }
         count = 0;
      }

      if(eventrc == EVENT_ERROR) {
         trickle_error = "Failed while waiting for the database trickle event";
         break;
      }

      // trickle some dirty pages to disk
      if((error = dbenv.memp_trickle(config.get_db_trickle_rate(), &nwrote)) != 0) {
         trickle_error.format("Failed to trickle database cache to disk (%d)", error);
         break;
      }

      // check if anything was actually written
      if(!nwrote) {
         // if there's nothing written, pause trickling
         if(!event_reset(trickle_event)) {
            trickle_error = "Failed to reset the database trickle event";
            break;
         }
      }
   }

   // reset the stop request and indicate that the thread is not running
   trickle_thread_stop = false;
   trickle_thread_stopped = true;
}

