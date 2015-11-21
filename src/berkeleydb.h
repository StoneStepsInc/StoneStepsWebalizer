/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    berkeleydb.h
*/
#ifndef __BERKELEYDB_H
#define __BERKELEYDB_H

#include "keynode.h"
#include "tstring.h"
#include "vector.h"
#include "tstamp.h"
#include "serialize.h"
#include "buffer.h"

#include "event.h"
#include "thread.h"

#include <db_cxx.h>
#include <vector>

// -----------------------------------------------------------------
//
// berkeleydb_t
//
// -----------------------------------------------------------------
class berkeleydb_t {
   protected:
      //
      // Define BDB callback types (bt_compare_fcn_type, etc are deprecated)
      //
      typedef int (*sc_extract_cb_t)(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

#if DB_VERSION_MAJOR >= 6
      typedef int (*bt_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
      typedef int (*dup_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
#else
      typedef int (*bt_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2);
      typedef int (*dup_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2);
#endif

      //
      // When a pointer to a template function is used in a conditional
      // operator, GCC gets confused and reports that there's not enough
      // contextual information to determine the type. There's a GCC bug 
      // (11407) that prevents GCC from being able to obtain the address of 
      // a template function. May be that's what's causing this error when
      // compiling expressions like this one:
      //
      //    func(cond ? tmpl_func<fptr> : NULL)
      //
      // If &tmpl_func<fptr> is used, bug 11407 triggers another error. The
      // work-around below converts a pointer to a template function to a
      // pointer to a function, which seems to resolve the problem (see 13.4,
      // ISO/IEC 14882, Second Edition 2003-10-15 for details on overload
      // resolution).
      //
      #if defined(__GNUC__) && (__GNUC__ < 4 || __GNUC__ == 4 && __GNUC_MINOR__ < 5)
      inline sc_extract_cb_t __gcc_bug11407__(sc_extract_cb_t cb) {return cb;}
      #else
      #define __gcc_bug11407__(cb) cb
      #endif

   public:
      template <typename node_t> class iterator; 
      template <typename node_t> class reverse_iterator;

      // -----------------------------------------------------------------
      // config_t
      //
      // 1. config_t provides all configuration data needed to construct a berkeleydb_t
      // instance. Configuration cannot be changed after an instance of berkeleydb_t has
      // been constructed.
      //
      // 2. berkeleydb_t will use the config_t::clone and config_t::release methods to 
      // maintain a copy of the configuration object throughout the lifespan of each
      // berkeleydb_t instance. This allows berkeleydb_t instances constructed using 
      // temporary config_t objects.
      //
      // 3. The reference to a config_t instance returned by config_t::clone must point
      // to a valid object until config_t::release is called. Otherwise config_t makes
      // no assumption of how the underlying object memory is maintained.
      //
      class config_t {
         public:
            virtual const config_t& clone(void) const = 0;

            virtual void release(void) const = 0;

            virtual const string_t& get_db_path(void) const = 0;

            virtual const string_t& get_tmp_path(void) const = 0;

            virtual const string_t& get_db_fname(void) const = 0;

            virtual const string_t& get_db_fname_ext(void) const = 0;

            virtual uint32_t get_db_cache_size(void) const = 0;

            virtual uint32_t get_db_seq_cache_size(void) const = 0;

            virtual uint32_t get_db_trickle_rate(void) const = 0;

            virtual bool get_db_direct(void) const = 0;

            virtual bool get_db_dsync(void) const = 0;
      };

   private:
      // -----------------------------------------------------------------
      //
      // cursor iterators
      //
      // -----------------------------------------------------------------
      class cursor_iterator_base {
         protected:
            Dbc         *cursor;

            int          error;

         private:
            // do not allow assignments
            cursor_iterator_base& operator = (const cursor_iterator_base&) {return *this;}

         public:
            cursor_iterator_base(Db *db);
            
            ~cursor_iterator_base(void);

            bool is_error(void) const {return (error && error != DB_NOTFOUND) ? true : false;}

            int get_error(void) const {return error;}

            void close(void);
      };

      class cursor_iterator : public cursor_iterator_base {
         private:
            db_recno_t   count;     // remaining duplicate count (zero if unique)

         public:
            cursor_iterator(Db *db) : cursor_iterator_base(db) {count = 0;}

            db_recno_t dup_count(void) const {return count;}

            bool set(Dbt& key, Dbt& data, Dbt *pkey);

            bool next(Dbt& key, Dbt& data, Dbt *pkey, bool dupkey = false);
      };

      class cursor_reverse_iterator : public cursor_iterator_base {
         public:
            cursor_reverse_iterator(Db *db) : cursor_iterator_base(db) {}

            bool prev(Dbt& key, Dbt& data, Dbt *pkey);
      };

      // -----------------------------------------------------------------
      // buffer_allocator_t
      //
      // Objects derived from buffer_allocator_t allow table_t instances to
      // reuse buffers in a more optimal way defined by the allocator owner,
      // which minimizes memory allocations without having to use a single
      // large buffer.
      //
      class buffer_allocator_t {
         public:
            virtual buffer_t get_buffer(void) = 0;

            virtual void release_buffer(buffer_t&& buffer) = 0;
      };

      // -----------------------------------------------------------------
      // buffer_holder_t is a convenience class to help manage temporary
      // buffers. The intended use is as follows:
      //
      //  buffer_t&& buffer = buffer_holder_t(*buffer_allocator).buffer;
      //
      // However, VC++ 2013 fails to recognize that the rvalue reference 
      // is bound to a subobject of a temporary and creates a new buffer_t
      // temporary. VC 2015 fixes this problem, but until the project is 
      // upgraded, a local buffer_holder_t instance must be created to 
      // maintain a valid buffer_t instance.
      //
      // Note that operator buffer_t&& cannot be used to access the member 
      // variable buffer because the temporary buffer holder in the example 
      // above, which buffer is a subobject of, is bound to the return value
      // reference, which in turn initializes the local variable buffer, but 
      // the return value is destroyed after this line, causing the buffer 
      // holder to be destroyed as well.
      // -----------------------------------------------------------------
      class buffer_holder_t {
         private:
            buffer_allocator_t&  allocator;

         public:
            buffer_t             buffer;

         public:
            buffer_holder_t(buffer_allocator_t& allocator) : allocator(allocator), buffer(allocator.get_buffer()) {}

            buffer_holder_t(const buffer_holder_t&) = delete;

            buffer_holder_t(buffer_holder_t&& other) = delete;

            ~buffer_holder_t(void) {allocator.release_buffer(std::move(buffer));}

            buffer_holder_t& operator = (const buffer_holder_t&) = delete;

            buffer_holder_t& operator = (buffer_holder_t&&) = delete;

            // see notes in the class definition
            operator buffer_t&& (void) = delete;
      };

   protected:
      // -----------------------------------------------------------------
      //
      // a table (primary database) with indexes (secondary databases)
      //
      // -----------------------------------------------------------------
      //
      // 1. Each table contains the primary database, indexed by a numeric 
      // sequence ID (key), and a set of secondary databases, containing one 
      // field from the primary database (serving as an index). One of the 
      // index databases contains hashes of actual item values, such as IP 
      // addresses, and is cached in "values" to avoid an extra look-up of 
      // the database by name. Hashes are used instead of actual values to
      // minimize database size - storing actual values would create copies
      // of URLs, referrers, etc.
      //
      // New and updated records are put into the primary database. Records
      // may be looked up by the node ID (primary database) or by the value
      // string (values database). Secondary databases, even the values 
      // database, will contain duplicates, each pointing back to the primary 
      // database using the primary key. Secondary tables are updated 
      // automatically by Berkeley DB. 
      //
      //   hosts (primary database)       values (secondary database)
      //   +----------------------+       +---------------------+
      //   | key  : 123           |       | hash: hash(value)   |
      //   | value: 127.0.0.1     |       | key : 123           |
      //   | hash : hash(value)   |       +---------------------+
      //   | hits : 1234          |       
      //   | pages: 567           |       hits (secondary database) 
      //   +----------------------+       +---------------------+
      //                                  | hits: 1234          |
      //   +----------------------+       | key : 123, 456, ... |
      //   | key  : 456           |       +---------------------+
      //   | value: 127.0.0.2     |
      //   | ...                  |
      //
      // 2. Berkeley databases cannot be copied, so table_t instances only use
      // move semantics, which requires all non-copyable data members maintained 
      // as pointers. These pointers can only be NULL for objects that are near
      // their lifetime. Calling any methods for these objects will result in
      // undefined behavior.
      //
      // 3. Buffer allocator is also maintained as a pointer to allow creating
      // buffers in const table_t methods. The assumption is that buffer allocators 
      // are safe for the current threading model.
      //
      class table_t {
         private:
            struct db_desc_t {
               Db                *scdb;      // secondary database
               string_t          dbname;     // database name
               string_t          dbpath;     // database file path
               sc_extract_cb_t   sccb;       // secondary database data extract callback

               public:
               db_desc_t(void) {scdb = NULL; sccb = NULL;}
               db_desc_t(Db *scdb, const char *dbname, const char *dbpath, sc_extract_cb_t sccb) : scdb(scdb), dbname(dbname), dbpath(dbpath), sccb(sccb) {}
            };

         private:
            DbEnv                *dbenv;     // shared DB environment

            Db                   *table;     // primary database

            Db                   *values;    // pointer to the values database (stored in indexes)

            Db                   *seqdb;     // shared sequence database

            DbSequence           *sequence;  // source of primary keys

            vector_t<db_desc_t>  indexes;    // secondary databases

            bool                 threaded;

            bool                 readonly;

            buffer_allocator_t   *buffer_allocator;

         private:
            const db_desc_t *get_sc_desc(const char *dbname) const;
            db_desc_t *get_sc_desc(const char *dbname);

         public:
            table_t(DbEnv& env, Db& seqdb, buffer_allocator_t& buffer_allocator);

            table_t(table_t&& other);

            table_t(const table_t& other) = delete;

            ~table_t(void);

            table_t& operator = (const table_t& other) = delete;

            table_t& operator = (table_t&& other);

            void set_threaded(bool value) {threaded = value;}

            bool get_threaded(void) const {return threaded;}

            void set_readonly(bool value) {readonly = value;}

            bool get_readonly(void) const {return readonly;}

            void init_db_handles(void);

            void destroy_db_handles(void);

            int open(const char *dbpath, const char *dbname, bt_compare_cb_t btcb);

            int close(void);

            int truncate(u_int32_t *count = NULL);

            int compact(u_int& bytes);

            int sync(void);

            void set_values_db(const char *dbname) {values = secondary_db(dbname);}

            int associate(const char *dbpath, const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t sccb = NULL);

            int associate(const char *dbname, sc_extract_cb_t sccb, bool rebuild);

            Db *primary_db(void) const {return table;}

            Db *secondary_db(const char *dbname) const;

            Db *values_db(void) const {return values;}

            int open_sequence(const char *colname, int32_t cachesize);

            db_seq_t get_seq_id(int32_t delta = 1);
            
            db_seq_t query_seq_id(void);

            uint64_t count(const char *dbname = NULL) const;

            template <typename node_t>
            bool put_node(const node_t& unode);

            template <typename node_t>
            bool get_node_by_id(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

            bool delete_node(const keynode_t<uint64_t>& node);

            template <typename node_t>
            bool get_node_by_value(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

            template <typename node_t>
            iterator<node_t> begin(const char *dbname) const; 

            template <typename node_t>
            reverse_iterator<node_t> rbegin(const char *dbname) const;
      };

   public:
      // -----------------------------------------------------------------
      //
      // public iterator to traverse a primary or secondary database
      //
      // -----------------------------------------------------------------
      template <typename node_t>
      class iterator_base {
         protected:
            cursor_iterator_base&   cursor;        // cursor reference

            buffer_allocator_t      *buffer_allocator;

         protected:
            Dbt& set_dbt_buffer(Dbt& dbt, void *buffer, size_t size) const;

         public:
            iterator_base(buffer_allocator_t& buffer_allocator, cursor_iterator_base& cursor);

            ~iterator_base(void);

            void close(void) {cursor.close();}

            bool is_error(void) const {return cursor.is_error();}

            int get_error(void) const {return cursor.get_error();}

      };

      template <typename node_t>
      class iterator : public iterator_base<node_t> {
         private:
            using iterator_base<node_t>::set_dbt_buffer;
            using iterator_base<node_t>::buffer_allocator;

            cursor_iterator         cursor;

            bool                    primdb;        // is primary database?

         private:
            // do not allow assignments
            iterator& operator = (const iterator&) = delete;

         public:
            iterator(buffer_allocator_t& buffer_allocator, const table_t& table, const char *dbname);

            bool next(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);
      };

      template <typename node_t>
      class reverse_iterator : public iterator_base<node_t> {
         private:
            using iterator_base<node_t>::set_dbt_buffer;
            using iterator_base<node_t>::buffer_allocator;

            cursor_reverse_iterator cursor;

            bool                    primdb;        // is primary database?

         private:
            // do not allow assignments
            reverse_iterator& operator = (const reverse_iterator&) = delete;

         public:
            reverse_iterator(buffer_allocator_t& buffer_allocator, const table_t& table, const char *dbname);

            bool prev(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);
      };

      //
      // A queue of buffers shared between all table_t objects
      //
      class buffer_queue_t : public buffer_allocator_t {
         private:
            std::vector<buffer_t>   buffers;

         public:
            buffer_t get_buffer(void);

            void release_buffer(buffer_t&& buffer);
      };

   private:
      void reset_db_handles(void);

      void trickle_thread_proc(void);

      #ifdef _WIN32
      static unsigned int __stdcall trickle_thread_proc(void *arg);
      #else
      static void *trickle_thread_proc(void *arg);
      #endif

      //
      // If BDB is linked against another version of CRT, deleting 
      // BDB objects may end up using the wrong memory manager. For
      // example, this code will allocate memory in the executable,
      // but will delete memory from the destructor, which will use 
      // the delete operator in the DLL context:
      //
      // DbSequence *seq = new DbSequence(db, 0);
      // delete seq;
      //
      // These functions use placement new operator and initialize
      // BDB objects using executable-allocated memory.
      //
      static Db *new_db(DbEnv *dbenv, u_int32_t flags);
      static void delete_db(Db *db);

      static DbSequence *new_db_sequence(Db *seqdb, u_int32_t flags);
      static void delete_db_sequence(DbSequence *dbseq);
      
      //
      // Memory management functions for the environment
      //
      static void *malloc(size_t size);
      static void *realloc(void *block, size_t size);
      static void free(void *block);

   private:
      const config_t&   config;

      DbEnv             dbenv;
      Db                sequences;

      buffer_queue_t    buffer_queue;

      vector_t<table_t*> tables;

      thread_t          trickle_thread;
      event_t           trickle_event;
      string_t          trickle_error;
      bool              trickle_thread_stop;
      bool              trickle_thread_stopped;

      bool              readonly;
      bool              trickle;

   protected:
      void add_table(table_t& table) {tables.push(&table);}

      table_t make_table(void) {return table_t(dbenv, sequences, buffer_queue);}

   public:
      berkeleydb_t(config_t&& config);

      ~berkeleydb_t(void);

      void set_trickle(bool value) {trickle = value;}

      void set_readonly(bool value) {readonly = value;}

      bool get_readonly(void) const {return readonly;}

      bool open(void);

      bool close(void);

      bool truncate(void);

      bool flush(void);

      int compact(u_int& bytes);
};

//
// B-Tree comparison function template for BDB v6 and up (top) and for prior versions 
// (bottom).
//
template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2);

//
// B-Tree comparison function template (reverse order)
//
template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2);

//
// B-Tree field extraction function template
//
template <s_field_cb_t extract>
int sc_extract_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

//
// B-Tree group field extraction function template
//
template <typename node_t, s_field_cb_t extract>
int sc_extract_group_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

#endif // __BERKELEYDB_H

