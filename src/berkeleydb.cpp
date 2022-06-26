/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   berkeleydb.cpp
*/
#include "pch.h"

#include "berkeleydb.h"

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
#include <algorithm>

// -----------------------------------------------------------------------
//
// berkeleydb_t::cursor_iterator_base
//
// -----------------------------------------------------------------------

berkeleydb_t::cursor_iterator_base::cursor_iterator_base(Db *db) 
{
   cursor = nullptr;
   error = 0;

   if(db) {
      if((error = db->cursor(nullptr, &cursor, 0)) != 0)
         cursor = nullptr;
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
   cursor = nullptr;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::cursor_dup_iterator
//
// -----------------------------------------------------------------------

bool berkeleydb_t::cursor_dup_iterator::set(Dbt& key, Dbt& data, Dbt *pkey)
{
   if(!cursor || is_error())
      return false;

   if(pkey)
      error = cursor->pget(&key, pkey, &data, DB_SET);
   else
      error = cursor->get(&key, &data, DB_SET);
      
   return !error;
}

bool berkeleydb_t::cursor_dup_iterator::next(Dbt& key, Dbt& data, Dbt *pkey)
{
   if(!cursor || is_error())
      return false;

   if(pkey)
      error = cursor->pget(&key, pkey, &data, DB_NEXT_DUP);
   else
      error = cursor->get(&key, &data, DB_NEXT_DUP);

   return !error;
}

bool berkeleydb_t::cursor_iterator::next(Dbt& key, Dbt& data, Dbt *pkey)
{
   if(!cursor || is_error())
      return false;

   if(pkey)
      error = cursor->pget(&key, pkey, &data, DB_NEXT);
   else
      error = cursor->get(&key, &data, DB_NEXT);

   return !error;
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

berkeleydb_t::table_t::table_t(const config_t& config, DbEnv& dbenv, Db& seqdb, buffer_allocator_t& buffer_allocator) :
      config(config),
      dbenv(&dbenv),
      table(new_db(&dbenv, DBFLAGS)),
      values(nullptr),
      seqdb(&seqdb),
      sequence(nullptr),
      buffer_allocator(&buffer_allocator),
      threaded(false)
{
}

berkeleydb_t::table_t::table_t(table_t&& other) noexcept :
      config(other.config),
      dbenv(other.dbenv),
      table(other.table),
      values(other.values),
      seqdb(other.seqdb),
      sequence(other.sequence),
      indexes(std::move(other.indexes)),
      buffer_allocator(other.buffer_allocator),
      threaded(other.threaded)
{
   other.dbenv = nullptr;
   other.table = nullptr;
   other.values = nullptr;
   other.seqdb = nullptr;
   other.sequence = nullptr;
   other.buffer_allocator = nullptr;
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

berkeleydb_t::table_t& berkeleydb_t::table_t::operator = (table_t&& other) noexcept
{
   dbenv = other.dbenv;
   table = other.table;
   values = other.values;
   seqdb = other.seqdb;
   sequence = other.sequence;
   buffer_allocator = other.buffer_allocator;
   indexes = std::move(other.indexes);

   threaded = other.threaded;

   other.dbenv = nullptr;
   other.table = nullptr;
   other.values = nullptr;
   other.seqdb = nullptr;
   other.sequence = nullptr;
   other.buffer_allocator = nullptr;

   return *this;
}

void berkeleydb_t::table_t::init_db_handles(void)
{
   Db *scdb;

   // construct secondary database handles
   for(u_int index = 0; index < indexes.size(); index++) {
      if((scdb = indexes[index].scdb) != nullptr) 
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
      if((scdb = indexes[index].scdb) != nullptr) 
         scdb->Db::~Db();
   }

   // destroy the primary database
   table->Db::~Db();
}

const berkeleydb_t::table_t::db_desc_t *berkeleydb_t::table_t::get_sc_desc(const char *dbname) const
{
   if(!dbname || !*dbname)
      return nullptr;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return nullptr;
}

berkeleydb_t::table_t::db_desc_t *berkeleydb_t::table_t::get_sc_desc(const char *dbname)
{
   if(!dbname || !*dbname)
      return nullptr;

   for(u_int index = 0; index < indexes.size(); index++) {
      if(indexes[index].dbname == dbname)
         return &indexes[index];
   }

   return nullptr;
}

int berkeleydb_t::table_t::open(const char *dbname, bt_compare_cb_t btcb)
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

      if(config.is_db_path_empty()) {
         // test configuration - skip checking if there's a memory pool file object
         if((error = indexes[index].scdb->get_mpf()->set_flags(DB_MPOOL_NOFILE, true)) != 0)
            return error;
      }

      if((error = indexes[index].scdb->open(nullptr, config.get_db_name_ptr(), indexes[index].dbname, DB_BTREE, flags, FILEMASK)) != 0)
         return error;
   }

   // then configure and open the primary database
   if((error = table->set_bt_compare(btcb)) != 0)
      return error;

   // set little endian byte order
   if((error = table->set_lorder(1234)) != 0)
      return error;

   if(config.is_db_path_empty()) {
      // test configuration - skip checking if there's a memory pool file object
      if((error = table->get_mpf()->set_flags(DB_MPOOL_NOFILE, true)) != 0)
         return error;
   }

   if((error = table->open(nullptr, config.get_db_name_ptr(), dbname, DB_BTREE, flags, FILEMASK)) != 0)
      return error;

   // associate all secondary databases with a non-nullptr data extraction callback
   for(index = 0; index < indexes.size(); index++) {
      // if the extraction callback is not nullptr, associate immediately
      if(indexes[index].scdb && indexes[index].scxcb) {
         if((error = table->associate(nullptr, indexes[index].scdb, indexes[index].scxcb, 0)) != 0)
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

   values = nullptr;

   if(sequence) {
      if((error = sequence->close(0)) != 0)
         return error;
      delete_db_sequence(sequence);
      sequence = nullptr;
   }

   return table->close(0);
}

int berkeleydb_t::table_t::truncate(u_int32_t *count)
{
   u_int32_t temp;
   return table->truncate(nullptr, count ? count : &temp, 0);
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
      if((error = indexes[index].scdb->compact(nullptr, nullptr, nullptr, &c_data, DB_FREE_SPACE, nullptr)) != 0)
         return error;
      
      // update the byte count   
      bytes += c_data.compact_pages_truncated * pagesize;
   }
   
   // do the same for the primary table database
   if((error = table->get_pagesize(&pagesize)) != 0)
      return error;

   memset(&c_data, 0, sizeof(c_data));

   if((error = table->compact(nullptr, nullptr, nullptr, &c_data, DB_FREE_SPACE, nullptr)) != 0)
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
int berkeleydb_t::table_t::associate(const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t scxcb)
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

   indexes.push_back(db_desc_t(scdb, dbname, scxcb));

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
/// first with a nullptr extraction callback to register a secondary database and then
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
      if((error = desc->scdb->truncate(nullptr, &temp, 0)) != 0)
         return error;
   }

   // associate the secondary, rebuilding if requested
   if((error = table->associate(nullptr, desc->scdb, scxcb, flags)) != 0)
      return error;

   // mark as associated
   desc->scxcb = scxcb;

   return 0;
}

Db *berkeleydb_t::table_t::secondary_db(const char *dbname) const
{
   const db_desc_t *desc;

   if((desc = get_sc_desc(dbname)) == nullptr)
      return nullptr;

   return desc->scdb;
}

int berkeleydb_t::table_t::open_sequence(const char *colname, int32_t cachesize, db_seq_t ini_seq_id)
{
   u_int32_t flags = DB_CREATE;
   int error;
   Dbt key;

   if(threaded)
      flags |= DB_THREAD;

   if(!sequence)
      sequence = new_db_sequence(seqdb, 0);

   if((error = sequence->initial_value(ini_seq_id)) != 0)
      return error;

   // set sequence cache size for this handle, if requested
   if(cachesize) {
      if((error = sequence->set_cachesize(cachesize)) != 0)
         return error;
   }

   key.set_data(const_cast<char*>(colname));
   key.set_size((u_int32_t) strlen(colname));

   return sequence->open(nullptr, &key, flags);
}

db_seq_t berkeleydb_t::table_t::get_seq_id(int32_t delta)
{
   db_seq_t seqid;

   if(!sequence || sequence->get(nullptr, delta, &seqid, 0))
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
      if((dbptr = secondary_db(dbname)) == nullptr)
         return 0;
   }

   if(dbptr->stat(nullptr, &stats, 0) != 0)
      return 0;

   nkeys = (uint64_t) stats->bt_nkeys;

   free(stats);

   return nkeys;
}

// -----------------------------------------------------------------------
//
// berkeleydb_t::table_t
//
// -----------------------------------------------------------------------

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

   if(table->del(nullptr, &key, 0))
      return false;

   return true;
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
   //
   // If the database wasn't closed properly, destroying a running trickle thread
   // will throw an exception, which will terminate the process. Make sure the
   // thread is stoppped, but don't attempt to close the database because we don't
   // know what state it is in.
   //
   if(!config.is_db_path_empty() && trickle)
      stop_trickle_thread();

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
   new (&dbenv) DbEnv(DBENVFLAGS);

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
   
   if((dbseq = malloc(sizeof(DbSequence))) == nullptr)
      return nullptr;
   
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
   
   if((db = malloc(sizeof(Db))) == nullptr)
      return nullptr;
      
   return new (db) Db(dbenv, flags);
}

void berkeleydb_t::delete_db(Db *db)
{
   if(db) {
      db->~Db();
      free(db);
   }
}

berkeleydb_t::status_t berkeleydb_t::open(std::initializer_list<table_t*> tblist)
{
   return open(tblist.begin(), tblist.size());
}

///
/// All tables passed via the parameter are registered with this class for subsequent
/// table operations on all these tables, such as being able to close, truncate, etc
/// all of them within this class.
///
/// The tables that are being registered must be initialized via `make_table` and must
/// not be opened at the time of this call. After this call succeeds, the database must
/// be closed even if derived method failed to open registered tables or failed in some
/// other way. This ensures that all registered tables are disassociated from the opened
/// environment.
///
berkeleydb_t::status_t berkeleydb_t::open(table_t * const tblist[], size_t tblcnt)
{
   u_int32_t dbflags = DB_CREATE;
   u_int32_t envflags = DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE;
   int major, minor, patch;
   status_t status;

   // it's a mistale to try to open the database twice without closing it after the first call
   if(!tables.empty())
      throw std::logic_error("No tables should be registered before the database is opened");

   db_version(&major, &minor, &patch);

   if(major < 4 || major == 4 && minor < 4)
      return string_t::_format("Berkeley DB must be v4.4 or newer (found v%d.%d.%d).\n", major, minor, patch);

   // do some additional initialization for threaded environment
   if(!config.is_db_path_empty() && trickle) {
      // initialize the environment and databases as thread-safe and with a single writer
      dbflags |= DB_THREAD;
      envflags |= DB_THREAD | DB_INIT_CDB;

      // initialize table databases as free-threaded
      for(size_t i = 0; i < tables.size(); i++)
         tables[i]->set_threaded(true);
   }

   // set the temporary directory if requested
   if(!config.get_tmp_path().isempty()) {
      if(!(status = dbenv.set_tmp_dir(config.get_tmp_path())).success())
         return status;
   }

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

   // open the DB environment
   if(!(status = dbenv.open(config.get_db_dir_ptr(), envflags, FILEMASK)).success())
      return status;

   if(config.is_db_path_empty()) {
      // test configuration - skip checking if there's a memory pool file object
      if(!(status = sequences.get_mpf()->set_flags(DB_MPOOL_NOFILE, true)).success())
         return status;
   }
   //
   // create the sequences database (unique node IDs)
   //
   if(!(status = sequences.open(nullptr, config.get_db_name_ptr(), "sequences", DB_HASH, dbflags, FILEMASK)).success())
      return status;

   // hold onto all the tables for subsequent operations
   tables.assign(tblist, tblist + tblcnt);

   // now that everything is initialized, we can start the trickle thread
   if(!config.is_db_path_empty() && trickle)
      trickle_thread = std::thread(&berkeleydb_t::trickle_thread_proc, this);

   return status;
}

berkeleydb_t::status_t berkeleydb_t::close(void)
{
   u_int errcnt = 0;
   int error = 0;
   status_t status;

   if(!config.is_db_path_empty() && trickle)
      stop_trickle_thread();

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

   tables.clear();

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

   if(!(status = dbenv.memp_sync(nullptr)).success())
      return status;

   for(size_t i = 0; i < tables.size(); i++) {
      if(!(status = tables[i]->sync()).success())
         return status;
   }

   return status;
}

void berkeleydb_t::stop_trickle_thread(void)
{
   // make sure the trickle thread is or was running
   if(trickle_thread.joinable()) {
      // signal the thread to stop
      trickle_mtx.lock();
      trickle_thread_stop = true;
      trickle_cv.notify_one();
      trickle_mtx.unlock();

      // and wait for it
      trickle_thread.join();
   }
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

#include "berkeleydb_tmpl.cpp"
