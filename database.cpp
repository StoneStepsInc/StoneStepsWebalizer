/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

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
#include "util.h"
#include "serialize.h"
#include "exception.h"

//
//
//
#define DBFLAGS               0
#define DBENVFLAGS            0

#define FILEMASK              0664              // database file access mask (rw-rw-r--)
#define DBBUFSIZE             65535             // shared buffer for key and data

#define BUFKEYSIZE            (DBBUFSIZE >> 1)
#define BUFDATASIZE           (DBBUFSIZE >> 1)

#define BUFKEYOFFSET          0
#define BUFDATAOFFSET         ((BUFKEYOFFSET) + (BUFKEYSIZE))

//
// B-Tree comparison function template
//
template <s_compare_cb_t compare>
static int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   return compare(dbt1->get_data(), dbt2->get_data());
}

//
// B-Tree comparison function template (reverse order)
//
template <s_compare_cb_t compare>
static int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   return -compare(dbt1->get_data(), dbt2->get_data());
}

//
// B-Tree field extraction function template
//
template <s_field_cb_t extract>
static int sc_extract_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result)
{
   u_int dsize;
   const void *dptr = extract(data->get_data(), data->get_size(), dsize);
   result->set_data(const_cast<void*>(dptr));
   result->set_size(dsize);
   return 0;
}

//
// B-Tree group field extraction function template
//
template <typename node_t, s_field_cb_t extract>
static int sc_extract_group_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result)
{
   if(!node_t::s_is_group(data->get_data(), data->get_size()))
      return DB_DONOTINDEX;

   return sc_extract_cb<extract>(secondary, key, data, result);
}

// -----------------------------------------------------------------------
//
// cursor iterators
//
// -----------------------------------------------------------------------

database_t::cursor_iterator_base::cursor_iterator_base(const Db *db) 
{
   cursor = NULL;
   error = 0;

   if(db) {
      if((error = const_cast<Db*>(db)->cursor(NULL, &cursor, 0)) != 0)
         cursor = NULL;
   }
}

database_t::cursor_iterator_base::~cursor_iterator_base(void) 
{
   close();
}

void database_t::cursor_iterator_base::close(void) 
{
   if(cursor)
      error = cursor->close();
   cursor = NULL;
}

bool database_t::cursor_iterator::set(Dbt& key, Dbt& data, Dbt *pkey)
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

bool database_t::cursor_iterator::next(Dbt& key, Dbt& data, Dbt *pkey, bool dupkey)
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

bool database_t::cursor_reverse_iterator::prev(Dbt& key, Dbt& data, Dbt *pkey)
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
// database_t::table_t
//
// -----------------------------------------------------------------------

database_t::table_t::table_t(DbEnv *_dbenv) : table(_dbenv, DBFLAGS)
{
   dbenv = _dbenv;
   values = NULL;
   sequence = NULL;
   threaded = false;
   readonly = false;
}

database_t::table_t::~table_t(void)
{
   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].scdb)
         delete_db(indexes[index].scdb);
   }

   if(sequence)
      delete_db_sequence(sequence);
}

void database_t::table_t::init_db_handles(void)
{
   Db *scdb;

   // construct secondary database handles
   for(u_int index = 0; index < indexes.size(); index++) {
      if((scdb = indexes[index].scdb) == NULL) 
         continue;

      new (scdb) Db(dbenv, DBFLAGS);
   }

   // construct the primary database
   new (&table) Db(dbenv, DBFLAGS);
}

void database_t::table_t::destroy_db_handles(void)
{
   Db *scdb;

   // destroy secondary database handles
   for(u_int index = 0; index < indexes.size(); index++) {
      if((scdb = indexes[index].scdb) == NULL) 
         continue;

      scdb->Db::~Db();
   }

   // destroy the primary database
   table.Db::~Db();
}

const database_t::table_t::db_desc_t *database_t::table_t::get_sc_desc(const char *dbname) const
{
   if(!dbname || !*dbname)
      return NULL;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return NULL;
}

database_t::table_t::db_desc_t *database_t::table_t::get_sc_desc(const char *dbname)
{
   if(!dbname || !*dbname)
      return NULL;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return NULL;
}

int database_t::table_t::open(const char *dbpath, const char *dbname, bt_compare_cb_t btcb)
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
   if((error = table.set_bt_compare(btcb)) != 0)
      return error;

   // set little endian byte order
   if((error = table.set_lorder(1234)) != 0)
      return error;

   if((error = table.open(NULL, dbpath, dbname, DB_BTREE, flags, FILEMASK)) != 0)
      return error;

   // associate all secondary databases with a non-NULL callback
   for(index = 0; index < indexes.size(); index++) {
      // if the extraction callback is not NULL, associate immediately
      if(indexes[index].scdb && indexes[index].sccb) {
         if((error = table.associate(NULL, indexes[index].scdb, indexes[index].sccb, 0)) != 0)
            return error;
      }
   }

   return 0;
}

int database_t::table_t::close(void)
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

   return table.close(0);
}

int database_t::table_t::truncate(u_int32_t *count)
{
   u_int32_t temp;
   return table.truncate(NULL, count ? count : &temp, 0);
}

int database_t::table_t::compact(u_int& bytes)
{
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 4
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
   if((error = table.get_pagesize(&pagesize)) != 0)
      return error;

   memset(&c_data, 0, sizeof(c_data));

   if((error = table.compact(NULL, NULL, NULL, &c_data, DB_FREE_SPACE, NULL)) != 0)
      return error;
      
   bytes += c_data.compact_pages_truncated * pagesize;
   
   return 0;
#else
   return EINVAL;
#endif
}

int database_t::table_t::sync(void)
{
   int error;

   for(u_int index = 0; index < indexes.size(); index++) {
      if((error = indexes[index].scdb->sync(0)) != 0)
         return error;
   }

   return table.sync(0);
}

int database_t::table_t::associate(const char *dbpath, const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t sccb)
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

int database_t::table_t::associate(const char *dbname, sc_extract_cb_t sccb, bool rebuild)
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
   if((error = table.associate(NULL, desc->scdb, sccb, flags)) != 0)
      return error;

   // mark as associated
   desc->sccb = sccb;

   return 0;
}

Db *database_t::table_t::secondary_db(const char *dbname)
{
   db_desc_t *desc;

   if((desc = get_sc_desc(dbname)) == NULL)
      return NULL;

   return desc->scdb;
}

const Db *database_t::table_t::secondary_db(const char *dbname) const
{
   const db_desc_t *desc;

   if((desc = get_sc_desc(dbname)) == NULL)
      return NULL;

   return desc->scdb;
}

int database_t::table_t::open_sequence(Db& seqdb, const char *colname, int32_t cachesize)
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
   key.set_size(strlen(colname));

   return sequence->open(NULL, &key, flags);
}

db_seq_t database_t::table_t::get_seq_id(int32_t delta)
{
   db_seq_t seqid;

   if(!sequence || sequence->get(NULL, delta, &seqid, 0))
      return -1;

   return seqid;
}

db_seq_t database_t::table_t::query_seq_id(void)
{
   DB_SEQUENCE_STAT *db_stat;
   db_seq_t cur_seq_id;
   
   if(!sequence || sequence->stat(&db_stat, 0))
      return -1;
      
   cur_seq_id = db_stat->st_current;
   
   free(db_stat);

   return cur_seq_id;
}

u_long database_t::table_t::count(const char *dbname) const
{
   DB_BTREE_STAT *stats;
   const Db *dbptr = &table;
   u_long nkeys;

   if(dbname && *dbname) {
      if((dbptr = secondary_db(dbname)) == NULL)
         return 0;
   }

   if(const_cast<Db*>(dbptr)->stat(NULL, &stats, 0) != 0)
      return 0;

   nkeys = (u_long) stats->bt_nkeys;

   free(stats);

   return nkeys;
}

// -----------------------------------------------------------------------
//
// database_t::iterator
//
// -----------------------------------------------------------------------

template <typename node_t>
database_t::iterator_base<node_t>::iterator_base(cursor_iterator_base& iter) : iter(iter)
{
   // set the default size for the keys and data
   set_buffer_size(BUFKEYSIZE, BUFDATASIZE);
}

template <typename node_t>
database_t::iterator_base<node_t>::~iterator_base(void)
{
   if(key.get_flags() & DB_DBT_USERMEM)
      delete [] (u_char*) key.get_data();

   if(pkey.get_flags() & DB_DBT_USERMEM)
      delete [] (u_char*) pkey.get_data();

   if(data.get_flags() & DB_DBT_USERMEM)
      delete [] (u_char*) data.get_data();
}

template <typename node_t>
void database_t::iterator_base<node_t>::set_buffer_size(u_int maxkey, u_int maxdata)
{
   if(maxkey) {
      key.set_data(new u_char[maxkey]);
      key.set_ulen(maxkey);
      key.set_flags(DB_DBT_USERMEM);

      pkey.set_data(new u_char[maxkey]);
      pkey.set_ulen(maxkey);
      pkey.set_flags(DB_DBT_USERMEM);
   }

   if(maxdata) {
      data.set_data(new u_char[maxdata]);
      data.set_ulen(maxdata);
      data.set_flags(DB_DBT_USERMEM);
   }
}

// -----------------------------------------------------------------------
//
// database_t::iterator
//
// -----------------------------------------------------------------------

template <typename node_t>
database_t::iterator<node_t>::iterator(const table_t& table, const char *dbname) : 
      iterator_base<node_t>(iter),
      iter(dbname ? table.secondary_db(dbname) : &table.primary_db())
{
   primdb = (dbname == NULL);
}

template <typename node_t>
bool database_t::iterator<node_t>::next(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   if(!iter.next(key, data, primdb ? NULL : &pkey))
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
// database_t::reverse_iterator
//
// -----------------------------------------------------------------------

template <typename node_t>
database_t::reverse_iterator<node_t>::reverse_iterator(const table_t& table, const char *dbname) : 
      iterator_base<node_t>(iter), 
      iter(dbname ? table.secondary_db(dbname) : &table.primary_db())
{
   primdb = (dbname == NULL);
}

template <typename node_t>
bool database_t::reverse_iterator<node_t>::prev(node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   if(!iter.prev(key, data, primdb ? NULL : &pkey))
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
// node_handler_t
//
// -----------------------------------------------------------------------

template <typename node_t>
bool database_t::node_handler_t<node_t>::put_node(table_t& table, u_char *buffer, const node_t& node)
{
   Dbt key, data;
   u_int keysize, datasize;

   if((keysize = node.s_pack_key(&buffer[BUFKEYOFFSET], BUFKEYSIZE)) == 0)
      return false;

   if((datasize = node.s_pack_data(&buffer[BUFDATAOFFSET], BUFDATASIZE)) == 0)
      return false;

   key.set_data(&buffer[BUFKEYOFFSET]);
   key.set_size(keysize);

   data.set_data(&buffer[BUFDATAOFFSET]);
   data.set_size(datasize);

   if(table.primary_db().put(NULL, &key, &data, 0)) 
      return false;

   // node flags are mutable
   const_cast<node_t&>(node).dirty = false;
   const_cast<node_t&>(node).storage = true;

   return true;
}

template <typename node_t>
bool database_t::node_handler_t<node_t>::get_node_by_id(const table_t& table, u_char *buffer, node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   Dbt key, pkey, data;
   u_int keysize;

   if((keysize = node.s_pack_key(&buffer[BUFKEYOFFSET], BUFKEYSIZE)) == 0)
      return false;

   key.set_data(&buffer[BUFKEYOFFSET]);
   key.set_size(keysize);
   key.set_ulen(keysize);
   key.set_flags(DB_DBT_USERMEM);

   data.set_data(&buffer[BUFDATAOFFSET]);
   data.set_ulen(BUFDATASIZE);
   data.set_flags(DB_DBT_USERMEM);

   if(const_cast<Db&>(table.primary_db()).get(NULL, &key, &data, 0))
      return false;

   if(node.s_unpack_data(data.get_data(), data.get_size(), upcb, arg) != data.get_size())
      return false;
   
   node.dirty = false;
   node.storage = true;

   return true;
}

template <typename node_t>
bool database_t::node_value_handler_t<node_t>::get_node_by_value(const table_t& table, u_char *buffer, node_t& node, typename node_t::s_unpack_cb_t upcb, void *arg)
{
   const Db *scdb;
   Dbt key, pkey, data;
   u_int keysize;
   u_long hashkey;

   if((scdb = table.values_db()) == NULL)
      return false;

   // make a value key
   hashkey = node.s_hash_value();
   keysize = sizeof(u_long);

   key.set_data(&hashkey);
   key.set_size(keysize);

   // share the key buffer between the key and the value
   pkey.set_data(&buffer[BUFKEYOFFSET]);
   pkey.set_ulen(BUFKEYSIZE);
   pkey.set_flags(DB_DBT_USERMEM);

   data.set_data(&buffer[BUFDATAOFFSET]);
   data.set_ulen(BUFDATASIZE);
   data.set_flags(DB_DBT_USERMEM);

   // open a cursor
   {cursor_iterator cursor(scdb);

   // find the first value and get the primary key and value data
   if(!cursor.set(key, data, &pkey))
      return false;

   do {
      // if the node value matched, break out
      if(!node.s_compare_value(data.get_data(), data.get_size()))
         break;

      // if there are no duplicates, return false
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

template <typename node_t>
bool database_t::node_handler_t<node_t>::delete_node(table_t& table, u_char *buffer, const keynode_t<u_long>& node)
{
   Dbt key;
   u_int keysize;

   if((keysize = node.s_pack_key(&buffer[BUFKEYOFFSET], BUFKEYSIZE)) == 0)
      return false;

   key.set_data(&buffer[BUFKEYOFFSET]);
   key.set_size(keysize);

   if(table.primary_db().del(NULL, &key, 0))
      return false;

   return true;
}

// -----------------------------------------------------------------------
//
// DbEnvEx
//
// -----------------------------------------------------------------------

database_t::DbEnvEx::DbEnvEx(u_int32_t flags) : DbEnv(flags)
{
   // configure the environment to use the correct memory manager
   if(set_alloc(database_t::malloc, database_t::realloc, database_t::free))
      throw exception_t(0, "Cannot set memory management functions for the database environment");
}

database_t::DbEnvEx::~DbEnvEx(void)
{
}

// -----------------------------------------------------------------------
//
// database_t
//
// -----------------------------------------------------------------------

database_t::database_t(const config_t& config) : 
      config(config),
      dbenv(DBENVFLAGS),
      sequences(&dbenv, DBFLAGS),
      urls(&dbenv),
      hosts(&dbenv),
      visits(&dbenv),
      downloads(&dbenv),
      active_downloads(&dbenv),
      agents(&dbenv),
      referrers(&dbenv),
      search(&dbenv),
      users(&dbenv),
      errors(&dbenv),
      dhosts(&dbenv),
      scodes(&dbenv),
      daily(&dbenv),
      hourly(&dbenv),
      totals(&dbenv),
      countries(&dbenv),
      system(&dbenv)
{
   u_int index = 0;

   trickle_thread = 0;
   trickle_event = NULL;
   trickle_thread_stop = false;
   trickle_thread_stopped = true;

   readonly = false;
   trickle = false;

   buffer = new u_char[DBBUFSIZE];

   // initialize the array to hold table pointers
   tables[index++] = &urls;
   tables[index++] = &hosts;
   tables[index++] = &visits;
   tables[index++] = &downloads;
   tables[index++] = &active_downloads;
   tables[index++] = &agents;
   tables[index++] = &referrers;
   tables[index++] = &search;
   tables[index++] = &users;
   tables[index++] = &errors;
   tables[index++] = &dhosts;
   tables[index++] = &scodes;
   tables[index++] = &daily;
   tables[index++] = &hourly;
   tables[index++] = &totals;
   tables[index++] = &countries;
   tables[index++] = &system;
   tables[index] = NULL;

   if(index >= sizeof(tables)/sizeof(tables[0]))
      throw exception_t(0, "Incorrect number of entries in the database_t::tables array");
}

database_t::~database_t(void)
{
   delete [] buffer;
}

void database_t::reset_db_handles(void)
{
   //
   // Once DbEnv::close and Db::close are called, their object instances 
   // become unusable. The code in all reset_db_handles methods calls 
   // destructors and constructors explicitly, reusing existing memory.
   //

   // destroy table databases
   for(table_t **tab = tables; *tab != NULL; tab++)
      (*tab)->destroy_db_handles();

   // destroy the sequences database
   sequences.Db::~Db();
   
   // reconstruct the environment 
   dbenv.~DbEnvEx();
   new (&dbenv) DbEnvEx(DBFLAGS);

   // construct the sequence database
   new (&sequences) Db(&dbenv, DBFLAGS);

   // construct table databases
   for(table_t **tab = tables; *tab != NULL; tab++)
      (*tab)->init_db_handles();
}

void *database_t::malloc(size_t size)
{
   return ::malloc(size);
}

void *database_t::realloc(void *block, size_t size)
{
   return ::realloc(block, size);
}

void database_t::free(void *block)
{
   ::free(block);
}

DbSequence *database_t::new_db_sequence(Db& db, u_int32_t flags)
{
   void *dbseq;
   
   if((dbseq = malloc(sizeof(DbSequence))) == NULL)
      return NULL;
   
   return new (dbseq) DbSequence(&db, flags);
}

void database_t::delete_db_sequence(DbSequence *dbseq)
{
   if(dbseq) {
      dbseq->~DbSequence();
      free(dbseq);
   }
}

Db *database_t::new_db(DbEnv *dbenv, u_int32_t flags)
{
   void *db;
   
   if((db = malloc(sizeof(Db))) == NULL)
      return NULL;
      
   return new (db) Db(dbenv, flags);
}

void database_t::delete_db(Db *db)
{
   if(db) {
      db->~Db();
      free(db);
   }
}

void database_t::db_error_cb(const DbEnv *dbenv, const char *errpfx, const char *errmsg)
{
   throw DbException(errpfx, errmsg, -1);
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
   u_int32_t dbflags = readonly ? DB_RDONLY : DB_CREATE;
   u_int32_t envflags = DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE;

   // set the error handler callback
   dbenv.set_errcall(db_error_cb);

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
      for(table_t **tab = tables; *tab != NULL; tab++)
         (*tab)->set_threaded(true);
   }

   // initialize table databases as read-only, if requested
   if(readonly) {
      for(table_t **tab = tables; *tab != NULL; tab++)
         (*tab)->set_readonly(true);
   }

   // get the fully-qualified database file path
   dbpath = config.get_db_path();

   // set the temporary directory
   if(dbenv.set_tmp_dir(config.db_path))
      return false;

   // if configured cache size is non-zero,
   if(config.db_cache_size) {
      // set the maximum database cache size
      if(dbenv.set_cachesize(0, config.db_cache_size, 0))
         return false;

      // and the maximum memory-mapped file size (read-only mode)
      if(readonly) {
         if(dbenv.set_mp_mmapsize(config.db_cache_size))
            return false;
      }
   }

   // disable OS buffering, if requested
   if(config.db_direct) {
      if(dbenv.set_flags(DB_DIRECT_DB, 1))
         return false;
   }

   // enable write-through I/O, if requested
   if(config.db_dsync) {
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 4
      if(dbenv.set_flags(DB_DSYNC_DB, 1))
         return false;
#else
      fprintf(stderr, "DB_DSYNC_DB is not supported in versions of Berkeley DB prior to v4.3\n");
#endif
   }

   // open the DB environment
   if(dbenv.open(dbpath, envflags, FILEMASK))
      return false;

   //
   // create the sequences database (unique node IDs)
   //
   if(sequences.open(NULL, dbpath, "sequences", DB_HASH, dbflags, FILEMASK))
         return false;

   //
   // confgure all databases
   //

   //
   // system
   //
   if(system.open(dbpath, "system", bt_compare_cb<sysnode_t::s_compare_key>))
      return false;

   //
   // urls
   //
   if(!readonly) {
      if(urls.open_sequence(sequences, "urls.seq", config.db_seq_cache_size))
         return false;
   }

   if(urls.associate(dbpath, "urls.values", bt_compare_cb<unode_t::s_compare_value_hash>, bt_compare_cb<unode_t::s_compare_value_hash>, sc_extract_cb<unode_t::s_field_value_hash>))
      return false;

   urls.set_values_db("urls.values");

   if(urls.associate(dbpath, "urls.hits", bt_compare_cb<unode_t::s_compare_hits>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_hits>) : NULL))
      return false;

   if(urls.associate(dbpath, "urls.xfer", bt_compare_cb<unode_t::s_compare_xfer>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_xfer>) : NULL))
      return false;

   if(urls.associate(dbpath, "urls.entry", bt_compare_cb<unode_t::s_compare_entry>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_entry>) : NULL))
      return false;

   if(urls.associate(dbpath, "urls.exit", bt_compare_cb<unode_t::s_compare_exit>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<unode_t::s_field_exit>) : NULL))
      return false;

   if(urls.associate(dbpath, "urls.groups.hits", bt_compare_cb<unode_t::s_compare_hits>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<unode_t, unode_t::s_field_hits>)) : NULL))
      return false;

   if(urls.associate(dbpath, "urls.groups.xfer", bt_compare_cb<unode_t::s_compare_xfer>, bt_compare_cb<unode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<unode_t, unode_t::s_field_xfer>)) : NULL))
      return false;

   if(urls.open(dbpath, "urls", bt_compare_cb<unode_t::s_compare_key>))
      return false;

   //
   // hosts
   //
   if(!readonly) {
      if(hosts.open_sequence(sequences, "hosts.seq", config.db_seq_cache_size))
         return false;
   }

   if(hosts.associate(dbpath, "hosts.values", bt_compare_cb<hnode_t::s_compare_value_hash>, bt_compare_cb<hnode_t::s_compare_value_hash>, sc_extract_cb<hnode_t::s_field_value_hash>))
      return false;

   hosts.set_values_db("hosts.values");

   if(hosts.associate(dbpath, "hosts.hits", bt_compare_cb<hnode_t::s_compare_hits>, bt_compare_cb<hnode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<hnode_t::s_field_hits>) : NULL))
      return false;

   if(hosts.associate(dbpath, "hosts.xfer", bt_compare_cb<hnode_t::s_compare_xfer>, bt_compare_cb<hnode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<hnode_t::s_field_xfer>) : NULL))
      return false;

   if(hosts.associate(dbpath, "hosts.groups.hits", bt_compare_cb<hnode_t::s_compare_hits>, bt_compare_cb<hnode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>)) : NULL))
      return false;

   if(hosts.associate(dbpath, "hosts.groups.xfer", bt_compare_cb<hnode_t::s_compare_xfer>, bt_compare_cb<hnode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>)) : NULL))
      return false;

   if(hosts.open(dbpath, "hosts", bt_compare_cb<hnode_t::s_compare_key>))
      return false;

   //
   // visits
   //
   if(visits.open(dbpath, "visits.active", bt_compare_cb<vnode_t::s_compare_key>))
      return false;

   //
   // downloads
   //
   if(!readonly) {
      if(downloads.open_sequence(sequences, "downloads.seq", config.db_seq_cache_size))
         return false;
   }
   
   if(downloads.associate(dbpath, "downloads.values", bt_compare_cb<dlnode_t::s_compare_value_hash>, bt_compare_cb<dlnode_t::s_compare_value_hash>, sc_extract_cb<dlnode_t::s_field_value_hash>))
      return false;

   downloads.set_values_db("downloads.values");

   if(downloads.associate(dbpath, "downloads.xfer", bt_compare_cb<dlnode_t::s_compare_xfer>, bt_compare_cb<dlnode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<dlnode_t::s_field_xfer>) : NULL))
      return false;

   if(downloads.open(dbpath, "downloads", bt_compare_cb<dlnode_t::s_compare_key>))
      return false;

   //
   // active downloads
   //
   if(active_downloads.open(dbpath, "downloads.active", bt_compare_cb<danode_t::s_compare_key>))
      return false;

   //
   // agents
   //
   if(!readonly) {
      if(agents.open_sequence(sequences, "agents.seq", config.db_seq_cache_size))
         return false;
   }

   if(agents.associate(dbpath, "agents.values", bt_compare_cb<anode_t::s_compare_value_hash>, bt_compare_cb<anode_t::s_compare_value_hash>, sc_extract_cb<anode_t::s_field_value_hash>))
      return false;

   agents.set_values_db("agents.values");

   if(agents.associate(dbpath, "agents.hits", bt_compare_cb<anode_t::s_compare_hits>, bt_compare_cb<anode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<anode_t::s_field_hits>) : NULL))
      return false;

   if(agents.associate(dbpath, "agents.visits", bt_compare_cb<anode_t::s_compare_visits>, bt_compare_cb<anode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<anode_t::s_field_visits>) : NULL))
      return false;

   if(agents.associate(dbpath, "agents.groups.visits", bt_compare_cb<anode_t::s_compare_hits>, bt_compare_cb<anode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<anode_t, anode_t::s_field_visits>)) : NULL))
      return false;

   if(agents.open(dbpath, "agents", bt_compare_cb<anode_t::s_compare_key>))
      return false;

   //
   // referrers
   //
   if(!readonly) {
      if(referrers.open_sequence(sequences, "referrers.seq", config.db_seq_cache_size))
         return false;
   }

   if(referrers.associate(dbpath, "referrers.values", bt_compare_cb<rnode_t::s_compare_value_hash>, bt_compare_cb<rnode_t::s_compare_value_hash>, sc_extract_cb<rnode_t::s_field_value_hash>))
      return false;

   referrers.set_values_db("referrers.values");

   if(referrers.associate(dbpath, "referrers.hits", bt_compare_cb<rnode_t::s_compare_hits>, bt_compare_cb<rnode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<rnode_t::s_field_hits>) : NULL))
      return false;

   if(referrers.associate(dbpath, "referrers.groups.hits", bt_compare_cb<rnode_t::s_compare_hits>, bt_compare_cb<rnode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>)) : NULL))
      return false;

   if(referrers.open(dbpath, "referrers", bt_compare_cb<rnode_t::s_compare_key>))
      return false;

   //
   // search strings
   //
   if(!readonly) {
      if(search.open_sequence(sequences, "search.seq", config.db_seq_cache_size))
         return false;
   }

   if(search.associate(dbpath, "search.values", bt_compare_cb<snode_t::s_compare_value_hash>, bt_compare_cb<snode_t::s_compare_value_hash>, sc_extract_cb<snode_t::s_field_value_hash>))
      return false;

   search.set_values_db("search.values");

   if(search.associate(dbpath, "search.hits", bt_compare_cb<snode_t::s_compare_hits>, bt_compare_cb<snode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<snode_t::s_field_hits>) : NULL))
      return false;

   if(search.open(dbpath, "search", bt_compare_cb<snode_t::s_compare_key>))
      return false;

   //
   // users
   //
   if(!readonly) {
      if(users.open_sequence(sequences, "users.seq", config.db_seq_cache_size))
         return false;
   }

   if(users.associate(dbpath, "users.values", bt_compare_cb<inode_t::s_compare_value_hash>, bt_compare_cb<inode_t::s_compare_value_hash>, sc_extract_cb<inode_t::s_field_value_hash>))
      return false;

   users.set_values_db("users.values");

   if(users.associate(dbpath, "users.hits", bt_compare_cb<inode_t::s_compare_hits>, bt_compare_cb<inode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<inode_t::s_field_hits>) : NULL))
      return false;

   if(users.associate(dbpath, "users.groups.hits", bt_compare_cb<inode_t::s_compare_hits>, bt_compare_cb<inode_t::s_compare_key>, readonly ? __gcc_bug11407__((sc_extract_group_cb<inode_t, inode_t::s_field_hits>)) : NULL))
      return false;

   if(users.open(dbpath, "users", bt_compare_cb<inode_t::s_compare_key>))
      return false;

   //
   // errors
   //
   if(!readonly) {
      if(errors.open_sequence(sequences, "errors.seq", config.db_seq_cache_size))
         return false;
   }

   if(errors.associate(dbpath, "errors.values", bt_compare_cb<rcnode_t::s_compare_value_hash>, bt_compare_cb<rcnode_t::s_compare_value_hash>, sc_extract_cb<rcnode_t::s_field_value_hash>))
      return false;

   errors.set_values_db("errors.values");

   if(errors.associate(dbpath, "errors.hits", bt_compare_cb<rcnode_t::s_compare_hits>, bt_compare_cb<rcnode_t::s_compare_key>, readonly ? __gcc_bug11407__(sc_extract_cb<rcnode_t::s_field_hits>) : NULL))
      return false;

   if(errors.open(dbpath, "errors", bt_compare_cb<rcnode_t::s_compare_key>))
      return false;

   //
   // daily hosts
   //
   if(!readonly) {
      if(dhosts.open_sequence(sequences, "dhosts.seq", config.db_seq_cache_size))
         return false;
   }

   if(dhosts.associate(dbpath, "dhosts.values", bt_compare_cb<tnode_t::s_compare_value_hash>, bt_compare_cb<tnode_t::s_compare_value_hash>, sc_extract_cb<tnode_t::s_field_value_hash>))
      return false;

   dhosts.set_values_db("dhosts.values");

   if(dhosts.open(dbpath, "dhosts", bt_compare_cb<tnode_t::s_compare_key>))
      return false;

   //
   // status codes
   //
   if(scodes.open(dbpath, "statuscodes", bt_compare_cb<scnode_t::s_compare_key>))
      return false;

   //
   // daily totals
   //
   if(daily.open(dbpath, "totals.daily", bt_compare_cb<daily_t::s_compare_key>))
      return false;

   //
   // hourly totals
   //
   if(hourly.open(dbpath, "totals.hourly", bt_compare_cb<hourly_t::s_compare_key>))
      return false;

   //
   // totals
   //
   if(totals.open(dbpath, "totals", bt_compare_cb<totals_t::s_compare_key>))
      return false;

   //
   // country codes
   //
   if(countries.open(dbpath, "countries", bt_compare_cb<ccnode_t::s_compare_key>))
      return false;

   return true;
}

bool database_t::close(void)
{
   u_int errcnt = 0, waitcnt = 300;

   // tell trickle thread to stop
   trickle_thread_stop = true;

   // wait for it for a while
   while(!trickle_thread_stopped && waitcnt--)
      msleep(50);

   // close all table databases
   for(table_t **tab = tables; *tab != NULL; tab++) {
      if((*tab)->close())
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

bool database_t::truncate(void)
{
   u_int errcnt = 0;

   for(table_t **tab = tables; *tab != NULL; tab++) {
      if((*tab)->truncate())
         errcnt++;
   }

   return !errcnt ? true : false;
}

int database_t::compact(u_int& bytes)
{
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 4
   int error;
   u_int tbytes;

   bytes = 0;

   for(table_t **tab = tables; *tab != NULL; tab++) {
      if((error = (*tab)->compact(tbytes)) != 0)
         return error;
      bytes += tbytes;
   }

   return 0;
#else
   return EINVAL;
#endif
}

bool database_t::flush(void)
{
   u_int errcnt = 0;

   if(dbenv.memp_sync(NULL))
      errcnt++;

   for(table_t **tab = tables; *tab != NULL; tab++) {
      if((*tab)->sync())
         errcnt++;
   }

   return !errcnt ? true : false;
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

#ifdef _WIN32
unsigned int __stdcall database_t::trickle_thread_proc(void *arg)
#else
void *database_t::trickle_thread_proc(void *arg)
#endif
{
   ((database_t*) arg)->trickle_thread_proc();
   return 0;
}

void database_t::trickle_thread_proc(void)
{
   int nwrote, error;
   u_long eventrc, count = 0;

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
      if((error = dbenv.memp_trickle(config.db_trickle_rate, &nwrote)) != 0) {
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

//
// unode
//
bool database_t::put_unode(const unode_t& unode)
{
   // unless unode_t::s_unpack_cb_t is provided, VC6 reports "fatal error C1506: unrecoverable block scoping error"
   return node_handler_t<unode_t>::put_node(urls, buffer, unode);
}

bool database_t::get_unode_by_id(unode_t& unode, unode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<unode_t>::get_node_by_id(urls, buffer, unode, upcb, arg);
}

bool database_t::get_unode_by_value(unode_t& unode, unode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<unode_t>::get_node_by_value(urls, buffer, unode, upcb, arg);
}

//
// hnode
//
bool database_t::put_hnode(const hnode_t& hnode)
{
   return node_handler_t<hnode_t>::put_node(hosts, buffer, hnode);
}

bool database_t::get_hnode_by_id(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<hnode_t>::get_node_by_id(hosts, buffer, hnode, upcb, arg);
}

bool database_t::get_hnode_by_value(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<hnode_t>::get_node_by_value(hosts, buffer, hnode, upcb, arg);
}

//
// vnode
//
bool database_t::put_vnode(const vnode_t& vnode)
{
   return node_handler_t<vnode_t>::put_node(visits, buffer, vnode);
}

bool database_t::get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<vnode_t>::get_node_by_id(visits, buffer, vnode, upcb, arg);
}

bool database_t::get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, const void *arg) const
{
   return node_handler_t<vnode_t>::get_node_by_id(visits, buffer, vnode, upcb, const_cast<void*>(arg));
}

bool database_t::delete_visit(const keynode_t<u_long>& vnode)
{
   return node_handler_t<vnode_t>::delete_node(visits, buffer, vnode);
}

// -----------------------------------------------------------------------
//
// downloads
//
// -----------------------------------------------------------------------
bool database_t::put_dlnode(const dlnode_t& dlnode)
{
   return node_handler_t<dlnode_t>::put_node(downloads, buffer, dlnode);
}

bool database_t::get_dlnode_by_id(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<dlnode_t>::get_node_by_id(downloads, buffer, dlnode, upcb, arg);
}

bool database_t::get_dlnode_by_value(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<dlnode_t>::get_node_by_value(downloads, buffer, dlnode, upcb, arg);
}

bool database_t::delete_download(const keynode_t<u_long>& danode)
{
   return node_handler_t<danode_t>::delete_node(active_downloads, buffer, danode);
}

// -----------------------------------------------------------------------
//
// active downloads
//
// -----------------------------------------------------------------------
bool database_t::put_danode(const danode_t& danode)
{
   return node_handler_t<danode_t>::put_node(active_downloads, buffer, danode);
}

bool database_t::get_danode_by_id(danode_t& danode, danode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<danode_t>::get_node_by_id(active_downloads, buffer, danode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// user agents
//
// -----------------------------------------------------------------------
bool database_t::put_anode(const anode_t& anode)
{
   return node_handler_t<anode_t>::put_node(agents, buffer, anode);
}

bool database_t::get_anode_by_id(anode_t& anode, anode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<anode_t>::get_node_by_id(agents, buffer, anode, upcb, arg);
}

bool database_t::get_anode_by_value(anode_t& anode, anode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<anode_t>::get_node_by_value(agents, buffer, anode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// referrers
//
// -----------------------------------------------------------------------
bool database_t::put_rnode(const rnode_t& rnode)
{
   return node_handler_t<rnode_t>::put_node(referrers, buffer, rnode);
}

bool database_t::get_rnode_by_id(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<rnode_t>::get_node_by_id(referrers, buffer, rnode, upcb, arg);
}

bool database_t::get_rnode_by_value(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<rnode_t>::get_node_by_value(referrers, buffer, rnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// search strings
//
// -----------------------------------------------------------------------
bool database_t::put_snode(const snode_t& snode)
{
   return node_handler_t<snode_t>::put_node(search, buffer, snode);
}

bool database_t::get_snode_by_id(snode_t& snode, snode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<snode_t>::get_node_by_id(search, buffer, snode, upcb, arg);
}

bool database_t::get_snode_by_value(snode_t& snode, snode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<snode_t>::get_node_by_value(search, buffer, snode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// users
//
// -----------------------------------------------------------------------
bool database_t::put_inode(const inode_t& inode)
{
   return node_handler_t<inode_t>::put_node(users, buffer, inode);
}

bool database_t::get_inode_by_id(inode_t& inode, inode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<inode_t>::get_node_by_id(users, buffer, inode, upcb, arg);
}

bool database_t::get_inode_by_value(inode_t& inode, inode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<inode_t>::get_node_by_value(users, buffer, inode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// errors
//
// -----------------------------------------------------------------------
bool database_t::put_rcnode(const rcnode_t& rcnode)
{
   return node_handler_t<rcnode_t>::put_node(errors, buffer, rcnode);
}

bool database_t::get_rcnode_by_id(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<rcnode_t>::get_node_by_id(errors, buffer, rcnode, upcb, arg);
}

bool database_t::get_rcnode_by_value(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<rcnode_t>::get_node_by_value(errors, buffer, rcnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// daily hosts
//
// -----------------------------------------------------------------------
bool database_t::put_tnode(const tnode_t& tnode)
{
   return node_handler_t<tnode_t>::put_node(dhosts, buffer, tnode);
}

bool database_t::get_tnode_by_id(tnode_t& tnode, tnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<tnode_t>::get_node_by_id(dhosts, buffer, tnode, upcb, arg);
}

bool database_t::get_tnode_by_value(tnode_t& tnode, tnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_value_handler_t<tnode_t>::get_node_by_value(dhosts, buffer, tnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// status codes
//
// -----------------------------------------------------------------------
bool database_t::put_scnode(const scnode_t& scnode)
{
   return node_handler_t<scnode_t>::put_node(scodes, buffer, scnode);
}

bool database_t::get_scnode_by_id(scnode_t& scnode, scnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<scnode_t>::get_node_by_id(scodes, buffer, scnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// daily totals
//
// -----------------------------------------------------------------------
bool database_t::put_tdnode(const daily_t& tdnode)
{
   return node_handler_t<daily_t>::put_node(daily, buffer, tdnode);
}

bool database_t::get_tdnode_by_id(daily_t& tdnode, daily_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<daily_t>::get_node_by_id(daily, buffer, tdnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// hourly totals
//
// -----------------------------------------------------------------------
bool database_t::put_thnode(const hourly_t& thnode)
{
   return node_handler_t<hourly_t>::put_node(hourly, buffer, thnode);
}

bool database_t::get_thnode_by_id(hourly_t& thnode, hourly_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<hourly_t>::get_node_by_id(hourly, buffer, thnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// totals
//
// -----------------------------------------------------------------------
bool database_t::put_tgnode(const totals_t& tgnode)
{
   return node_handler_t<totals_t>::put_node(totals, buffer, tgnode);
}

bool database_t::get_tgnode_by_id(totals_t& tgnode, totals_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<totals_t>::get_node_by_id(totals, buffer, tgnode, upcb, arg);
}

// -----------------------------------------------------------------------
//
// country codes
//
// -----------------------------------------------------------------------
bool database_t::put_ccnode(const ccnode_t& ccnode)
{
   return node_handler_t<ccnode_t>::put_node(countries, buffer, ccnode);
}

bool database_t::get_ccnode_by_id(ccnode_t& ccnode, ccnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<ccnode_t>::get_node_by_id(countries, buffer, ccnode, upcb, arg);
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

bool database_t::put_sysnode(const sysnode_t& sysnode)
{
   return node_handler_t<sysnode_t>::put_node(system, buffer, sysnode);
}

bool database_t::get_sysnode_by_id(sysnode_t& sysnode, sysnode_t::s_unpack_cb_t upcb, void *arg) const
{
   return node_handler_t<sysnode_t>::get_node_by_id(system, buffer, sysnode, upcb, arg);
}

bool database_t::fix_v3_8_0_4(void)
{
   db_seq_t tnode_seq_id, anode_seq_id, hnode_seq_id, rnode_seq_id, snode_seq_id, inode_seq_id, max_seq_id;
   int32_t delta;
   
   // get the IDs that were used incorrectly for tables outlined below
   if((tnode_seq_id = dhosts.query_seq_id()) == -1)
      return false;
      
   if((anode_seq_id = agents.query_seq_id()) == -1)
      return false;
      
   if((hnode_seq_id = hosts.query_seq_id()) == -1)
      return false;
   
   // grab the maximum ID value out of all three
   max_seq_id = MAX(tnode_seq_id, MAX(anode_seq_id, hnode_seq_id));
   
   //
   // referrer IDs were taken from tnodes in webalizer.cpp
   // referrer IDs were taken from agents in preserve.cpp (old text state file)
   //
   if((rnode_seq_id = referrers.query_seq_id()) == -1)
      return false;
   
   if((delta = (int32_t) (MAX(rnode_seq_id, max_seq_id) - rnode_seq_id)) > 0) {
      if(referrers.get_seq_id(delta) == -1)
         return false;
   }
   
   //
   // search string IDs were taken from agents
   //
   if((snode_seq_id = search.query_seq_id()) == -1)
      return false;
   
   if((delta = (int32_t) (MAX(snode_seq_id, max_seq_id) - snode_seq_id)) > 0) {   
      if(search.get_seq_id(delta) == -1)
         return false;
   }
   
   //
   // user IDs were taken from agents
   //
   if((inode_seq_id = users.query_seq_id()) == -1)
      return false;
   
   if((delta = (int32_t) (MAX(inode_seq_id, max_seq_id) - inode_seq_id)) > 0) {   
      if(users.get_seq_id(delta) == -1)
         return false;
   }
   
   //
   // agent IDs were taken from hosts
   //
   if((anode_seq_id = agents.query_seq_id()) == -1)
      return false;
   
   if((delta = (int32_t) (MAX(anode_seq_id, max_seq_id) - anode_seq_id)) > 0) {   
      if(agents.get_seq_id(delta) == -1)
         return false;
   }
   
   return true;
}

// -----------------------------------------------------------------------
//
// instantiate templates
//
// -----------------------------------------------------------------------
#if defined(_WIN32) && (_MSC_VER == 1200)
#pragma warning(disable:4660) // template-class specialization 'type' is already instantiated
#endif

// keynode_t
template int bt_compare_cb<keynode_t<u_long>::s_compare_key>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

// URLs
template int bt_compare_cb<unode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<unode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<unode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<unode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<unode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<unode_t, unode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<unode_t>;
template class database_t::iterator<unode_t>;
template class database_t::reverse_iterator<unode_t>;
template class database_t::node_handler_t<unode_t>;
template class database_t::node_value_handler_t<unode_t>;

// hosts
template int bt_compare_cb<hnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<hnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<hnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<hnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<hnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<hnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<hnode_t, hnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<hnode_t, hnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<hnode_t>;
template class database_t::iterator<hnode_t>;
template class database_t::reverse_iterator<hnode_t>;
template class database_t::node_handler_t<hnode_t>;
template class database_t::node_value_handler_t<hnode_t>;

// visits
template class database_t::node_handler_t<vnode_t>;
template class database_t::iterator_base<vnode_t>;
template class database_t::iterator<vnode_t>;

// downloads
template int bt_compare_cb<dlnode_t::s_compare_xfer>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<dlnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<dlnode_t::s_field_xfer>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::node_handler_t<dlnode_t>;
template class database_t::node_value_handler_t<dlnode_t>;
template class database_t::iterator_base<dlnode_t>;
template class database_t::iterator<dlnode_t>;
template class database_t::reverse_iterator<dlnode_t>;

// active downloads
template class database_t::node_handler_t<danode_t>;
template class database_t::iterator_base<danode_t>;
template class database_t::iterator<danode_t>;

// user agents
template int bt_compare_cb<anode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<anode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<anode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<anode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<anode_t, anode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<anode_t>;
template class database_t::iterator<anode_t>;
template class database_t::reverse_iterator<anode_t>;
template class database_t::node_handler_t<anode_t>;
template class database_t::node_value_handler_t<anode_t>;

// referrers
template int bt_compare_cb<rnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<rnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<rnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<rnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<rnode_t, rnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<rnode_t>;
template class database_t::iterator<rnode_t>;
template class database_t::reverse_iterator<rnode_t>;
template class database_t::node_handler_t<rnode_t>;
template class database_t::node_value_handler_t<rnode_t>;

// search strings
template int bt_compare_cb<snode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<snode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<snode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<snode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<snode_t>;
template class database_t::iterator<snode_t>;
template class database_t::reverse_iterator<snode_t>;
template class database_t::node_handler_t<snode_t>;
template class database_t::node_value_handler_t<snode_t>;

// users
template int bt_compare_cb<inode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
template int bt_compare_cb<inode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<inode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<inode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_group_cb<inode_t, inode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::iterator_base<inode_t>;
template class database_t::iterator<inode_t>;
template class database_t::reverse_iterator<inode_t>;
template class database_t::node_handler_t<inode_t>;
template class database_t::node_value_handler_t<inode_t>;

// errors
template int bt_compare_cb<rcnode_t::s_compare_hits>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template int sc_extract_cb<rcnode_t::s_field_value_hash>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
template int sc_extract_cb<rcnode_t::s_field_hits>(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

template class database_t::node_handler_t<rcnode_t>;
template class database_t::node_value_handler_t<rcnode_t>;
template class database_t::iterator_base<rcnode_t>;
template class database_t::iterator<rcnode_t>;
template class database_t::reverse_iterator<rcnode_t>;

// daily hosts
template int bt_compare_cb<tnode_t::s_compare_value_hash>(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template class database_t::node_handler_t<tnode_t>;
template class database_t::node_value_handler_t<tnode_t>;
template class database_t::iterator_base<tnode_t>;
template class database_t::iterator<tnode_t>;

// status codes
template class database_t::node_handler_t<scnode_t>;
template class database_t::iterator_base<scnode_t>;
template class database_t::iterator<scnode_t>;

// daily totals
template class database_t::node_handler_t<daily_t>;
template class database_t::iterator_base<daily_t>;
template class database_t::iterator<daily_t>;

// hourly totals
template class database_t::node_handler_t<hourly_t>;
template class database_t::iterator_base<hourly_t>;
template class database_t::iterator<hourly_t>;

// totals
template class database_t::node_handler_t<totals_t>;

// country codes
template class database_t::node_handler_t<ccnode_t>;
template class database_t::iterator_base<ccnode_t>;
template class database_t::iterator<ccnode_t>;

// system
template class database_t::node_handler_t<sysnode_t>;
