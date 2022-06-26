/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    berkeleydb.h
*/
#ifndef BERKELEYDB_H
#define BERKELEYDB_H

#include "keynode.h"
#include "tstring.h"
#include "tstamp.h"
#include "serialize.h"
#include "char_buffer.h"
#include "char_buffer_stack.h"
#include "storable.h"

#include "thread.h"

#include <db_cxx.h>

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

/// Unit test classes that need access to private members.
namespace sswtest {
   class BerkeleyDBTest;
}

///
/// @brief  A base class of a Berkeley DB interface wrapper
///
/// The `berkeleydb_t` class wraps various Berkeley DB interfaces, such as primary and 
/// secondary databases storing blobs, and exposes them in a somewhat more traditional 
/// form of database tables storing record-like node objects that are indexed with 
/// customizable keys.
/// 
class berkeleydb_t {
   friend class sswtest::BerkeleyDBTest;

   private:
      static const size_t        DBBUFSIZE = 32768;

      static const u_int32_t     DBFLAGS = 0;            ///< Berkeley DB database flags.
      static const u_int32_t     DBENVFLAGS = 0;         ///< Berkeley DB environment flags.

      static const int           FILEMASK = 0664;        ///< Database file access mask (rw-rw-r--).

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
      //    func(cond ? tmpl_func<fptr> : nullptr)
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

      //
      // Define buffer allocator and holder types for internal use
      //
      typedef char_buffer_allocator_tmpl<unsigned char> buffer_allocator_t;
      typedef char_buffer_stack_tmpl<unsigned char> buffer_stack_t;
      typedef char_buffer_holder_tmpl<unsigned char> buffer_holder_t;

   public:
      template <typename node_t> class iterator; 
      template <typename node_t> class reverse_iterator;

      ///
      /// @brief  Combines a Berkeley DB error and an application error message in one
      ///         class that is returned from `berkeleydb_t` methods.
      ///
      /// Application messages are not localized at this point. If there will be a desire
      /// to localize error messages, the constructor that takes an application error 
      /// message will be changed to take an `enum` value corresponding to the application
      /// error code.
      ///
      class status_t {
         private:
            int         error;           ///< Berkeley DB error code.
            string_t    message;         ///< Berkeley DB or application error message.

         public:
            ///
            /// @brief  A template constructor that triggers a compiler error, if used
            ///         instead of the one that takes `int`.
            /// 
            template <typename T>
            status_t(T value)
            {
               static_assert(sizeof(T) != 0, "Cannot initialize berkeleydb_t::status_t with this value");
            }

            /// Constructs a status object from a Berkeley DB error code.
            status_t(int error) : error(error)
            {
               if(error)
                  message = db_strerror(error);
            }

            /// Constructs a status object that indicates a success.
            status_t(void) : 
               error(0)
            {
            }

            /// Constructs a status object from an application error message.
            status_t(const char *message) : 
               error(0),
               message(message)
            {
            }

            /// Constructs a status object from an application error message.
            status_t(string_t&& message) : 
               error(0),
               message(std::move(message))
            {
            }

            /// Constructs the status object by moving data from another instance.
            status_t(status_t&& other) noexcept :
               error(other.error),
               message(std::move(other.message))
            {
            }

            /// Moves the status object data from another instance.
            status_t& operator = (status_t&& other) noexcept
            {
               error = other.error;
               message = std::move(other.message);

               return *this;
            }

            /// Indicates whether the status is a success or not.
            bool success(void) const
            {
               // check the error just in case db_strerror returns an empty string
               return !error && message.isempty(); 
            }

            /// Returns an error message if there is one or an empty string otherwise.
            const string_t& err_msg(void) const 
            {
               return message;
            }

            /// Returns the error code. A zero indicates success.
            int err_num(void) const
            {
               return error;
            }
      };

      ///
      /// @brief  Defines an interface for a configuration necessary to construct a
      ///         `berkeleydb_t` instance.
      ///
      /// 1. `config_t` provides all configuration data needed to construct a `berkeleydb_t`
      /// instance. Configuration cannot be changed after an instance of `berkeleydb_t` has
      /// been constructed.
      ///
      /// 2. `berkeleydb_t` will use the `config_t::clone` and `config_t::release` methods
      /// to maintain a copy of the configuration object throughout the lifespan of each
      /// `berkeleydb_t` instance. This allows `berkeleydb_t` instances constructed using 
      /// temporary `config_t` objects.
      ///
      /// 3. The reference to a `config_t` instance returned by `config_t::clone` must point
      /// to a valid object until `config_t::release` is called. Otherwise `config_t` makes
      /// no assumption of how the underlying object memory is maintained.
      ///
      class config_t {
         public:
            /// Creates a copy of this object.
            virtual const config_t& clone(void) const = 0;

            /// Releases a copy of this object returned from `clone`.
            virtual void release(void) const = 0;

            /// Returns a database file path, including the file name.
            virtual const string_t& get_db_path(void) const = 0;

            /// Returns a database directory path, not including the file name.
            virtual const string_t& get_db_dir(void) const = 0;

            /// Returns just the database file name.
            virtual const string_t& get_db_name(void) const = 0;

            /// Returns a character pointer to the full database path. May be `nullptr` if it is empty.
            const char *get_db_path_ptr(void) const {return !get_db_path().isempty() ? get_db_path().c_str() : nullptr;}

            /// Returns a character pointer to the database directory path. May be `nullptr` if it is empty.
            const char *get_db_dir_ptr(void) const {return !get_db_dir().isempty() ? get_db_dir().c_str() : nullptr;}

            /// Returns a character pointer to the database file name. May be `nullptr` if it is empty.
            const char *get_db_name_ptr(void) const {return !get_db_name().isempty() ? get_db_name().c_str() : nullptr;}

            /// Returns `true` if the database path is empty, `false` otherwise.
            bool is_db_path_empty(void) const {return get_db_path().isempty();}

            /// Returns a path to a directory for temporary Berkeley DB files.
            virtual const string_t& get_tmp_path(void) const = 0;

            /// Returns Berkeley DB cache size, in bytes. If zero, the default is used.
            virtual uint32_t get_db_cache_size(void) const = 0;

            /// Number of sequence increments reserved for each sequence instance at a time.
            virtual uint32_t get_db_seq_cache_size(void) const = 0;

            /// Indicates whether OS I/O buffering should be disabled (`true`) or not (`false`).
            virtual bool get_db_direct(void) const = 0;
      };

   private:
      ///
      /// @brief  A base class that wraps Berkeley DB cursors to traverse primary and
      ///         secondary databases
      ///
      class cursor_iterator_base {
         protected:
            Dbc         *cursor;

            int          error;

         private:
            // do not allow assignments
            cursor_iterator_base& operator = (const cursor_iterator_base&) = delete;

         public:
            cursor_iterator_base(Db *db);
            
            ~cursor_iterator_base(void);

            bool is_error(void) const {return (error && error != DB_NOTFOUND) ? true : false;}

            int get_error(void) const {return error;}

            void close(void);
      };

      ///
      /// @brief  A forward Berkeley DB iterator to traverse duplicate keys.
      ///
      /// The content of the `key` parameter must not change between `set` and any of the
      /// subsequent `next` calls.
      ///
      class cursor_dup_iterator : public cursor_iterator_base {
         public:
            cursor_dup_iterator(Db *db) : cursor_iterator_base(db) {}

            bool set(Dbt& key, Dbt& data, Dbt *pkey);

            bool next(Dbt& key, Dbt& data, Dbt *pkey);
      };

      ///
      /// @brief  A forward Berkeley DB cursor iterator
      ///
      class cursor_iterator : public cursor_iterator_base {
         public:
            cursor_iterator(Db *db) : cursor_iterator_base(db) {}

            bool next(Dbt& key, Dbt& data, Dbt *pkey);
      };

      ///
      /// @brief  A reverse Berkeley DB cursor iterator
      ///
      class cursor_reverse_iterator : public cursor_iterator_base {
         public:
            cursor_reverse_iterator(Db *db) : cursor_iterator_base(db) {}

            bool prev(Dbt& key, Dbt& data, Dbt *pkey);
      };

   protected:
      ///
      /// @brief  A table object stores data along with accompanying indexes
      ///
      /// 1. Each table contains a primary database with a numeric sequence key, 
      /// and a set of secondary databases, each containing values from one field 
      /// of the primary database, such as transfer amounts or number of visits, 
      /// and linked to the primary database via the sequence key of the primary 
      /// database. 
      ///
      /// One of the secondary databases stores hashes of text values stored in 
      /// the primary database, such as IP addresses or URLs, and allows us to 
      /// look-up these text values without storing copies of the text in the 
      /// secondary database. This secondary database is maintained in the `values` 
      /// data member to avoid extra look-ups of the database descriptor by name.
      ///
      /// New and updated records are put into the primary database. Records
      /// may be looked up by the node ID (primary database) or by the value
      /// string (values database). Secondary databases, even the values 
      /// database, will contain duplicates, each pointing back to the primary 
      /// database using the primary key. Secondary databases are associated
      /// with the primary database when `table_t` is constructed and are updated 
      /// automatically by Berkeley DB.
      /// ```
      ///    hosts (primary database)       values (secondary database)
      ///    +----------------------+       +---------------------+
      ///    | key  : 123           |       | hash: H(127.0.0.1)  |
      ///    | value: 127.0.0.1     |       | key : 123           |
      ///    | hash : hash(value)   |       | ...                 |
      ///    | hits : 1234          |       
      ///    | pages: 567           |       hits (secondary database) 
      ///    +----------------------+       +---------------------+
      ///    | key  : 456           |       | hits: 1234          |
      ///    | value: 127.0.0.2     |       | key : 123           |
      ///    | ...                  |       | hits: 1234          |
      ///                                   | key : 456           |
      ///                                   | ...                 |
      /// ```
      /// Secondary databases (indexes) may be optionally associated with the
      /// primary database via two `associate` calls. The first one registers
      /// comparison callbacks within the table and the second one completes
      /// the association by registering all callbacks with Berkeley DB.
      ///
      /// It is not necessary to associate a secondary database for a table
      /// that is not intended to be queried by its associated field.
      ///
      /// 2. Berkeley databases cannot be copied, so `table_t` instances only use
      /// move semantics, which requires all non-copyable data members maintained 
      /// as pointers. These pointers can only be nullptr for objects that are near
      /// their lifetime. Calling any methods for these objects will result in
      /// undefined behavior.
      ///
      /// 3. Buffer allocator is also maintained as a pointer to allow creating
      /// buffers in `const` `table_t` methods. The assumption is that buffer 
      /// allocators are safe for the current threading model.
      ///
      class table_t {
         private:
            ///
            /// @brief  A secondary database descriptor
            ///
            struct db_desc_t {
               Db                *scdb;      ///< Secondary database instance.
               string_t          dbname;     ///< Secondary database name.
               sc_extract_cb_t   scxcb;      ///< Extracts a secondary database key from the primary record.

               public:
               db_desc_t(void) : scdb(nullptr), scxcb(nullptr) {}
               db_desc_t(Db *scdb, const char *dbname, sc_extract_cb_t scxcb) : scdb(scdb), dbname(dbname), scxcb(scxcb) {}
            };

         private:
            const config_t&      config;

            DbEnv                *dbenv;     // shared DB environment

            Db                   *table;     // primary database

            Db                   *values;    // pointer to the values database (stored in indexes)

            Db                   *seqdb;     // shared sequence database

            DbSequence           *sequence;  // source of primary keys

            std::vector<db_desc_t> indexes;  // secondary databases

            bool                 threaded;

            buffer_allocator_t   *buffer_allocator;

         private:
            const db_desc_t *get_sc_desc(const char *dbname) const;
            db_desc_t *get_sc_desc(const char *dbname);

         public:
            table_t(const config_t& config, DbEnv& env, Db& seqdb, buffer_allocator_t& buffer_allocator);

            table_t(table_t&& other) noexcept;

            table_t(const table_t& other) = delete;

            ~table_t(void);

            table_t& operator = (const table_t& other) = delete;

            table_t& operator = (table_t&& other) noexcept;

            /// initializes Berkeley DB for multi-thread calls
            void set_threaded(bool value) {threaded = value;}

            /// returns whether Berkeley DB was initialized for multi-thread calls
            bool get_threaded(void) const {return threaded;}

            void init_db_handles(void);

            void destroy_db_handles(void);

            /// opes a table and all of its associated indexes located in the specified database file
            int open(const char *dbname, bt_compare_cb_t btcb);

            /// closes the table and all of its associated indexes
            int close(void);

            /// erases all data in the table
            int truncate(u_int32_t *count = nullptr);

            /// rearranges data pages on disk to minimize unused space
            int compact(u_int& bytes);

            /// flushes dirty data pages to disk
            int sync(void);

            /// designates the specified secondary database name as the value database
            void set_values_db(const char *dbname) {values = secondary_db(dbname);}

            /// Prepares a named secondary database association with the primary database.
            int associate(const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t scxcb = nullptr);

            /// Associates a prepared secondary database with the primary database.
            int associate(const char *dbname, sc_extract_cb_t scxcb, bool rebuild);

            Db *primary_db(void) const {return table;}

            Db *secondary_db(const char *dbname) const;

            Db *values_db(void) const {return values;}

            /// opens a sequence within a sequence database
            int open_sequence(const char *colname, int32_t cachesize, db_seq_t ini_seq_id = 1);

            /// gets the next ID from the sequence for this table
            db_seq_t get_seq_id(int32_t delta = 1);
            
            /// Queries the current sequence ID without incrementing it.
            db_seq_t query_seq_id(void);

            /// Returns the number of unique keys in the primary or a named secondary database.
            uint64_t count(const char *dbname = nullptr) const;

            /// inserts a new node or updates the existing node in the primary database
            template <typename node_t>
            bool put_node(const node_t& unode, storage_info_t& storage_info);

            /// retrieves a node by its unique ID
            template <typename node_t, typename ... param_t>
            bool get_node_by_id(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb = nullptr, param_t ... param) const;

            /// deletes a node by its key
            bool delete_node(const keynode_t<uint64_t>& node);

            /// retrieves a node by its value
            template <typename node_t, typename ... param_t>
            bool get_node_by_value(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb = nullptr, param_t ... param) const;

            /// returns a forward table iterator
            template <typename node_t>
            iterator<node_t> begin(const char *dbname) const; 

            /// returns a reverse table iterator
            template <typename node_t>
            reverse_iterator<node_t> rbegin(const char *dbname) const;
      };

   public:
      ///
      /// @brief  A public cursor iterator base class to traverse primary and secondary 
      ///         databases
      ///
      /// @tparam node_t   The node class to retrieve in each iteration
      ///
      /// This public database iterator hides the Berkeley DB cursor implementation in 
      /// cursor_iterator_base.
      ///
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

      ///
      /// @brief  A forward cursor iterator base class to traverse primary and secondary 
      ///         databases
      ///
      /// @tparam node_t   The node class to retrieve in each iteration
      ///
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

            template <typename ... param_t>
            bool next(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb = nullptr, param_t ... param);
      };

      ///
      /// @brief  A reverse cursor iterator base class to traverse primary and secondary 
      ///         databases
      ///
      /// @tparam node_t   The node class to retrieve in each iteration
      ///
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

            template <typename ... param_t>
            bool prev(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t...> upcb = nullptr, param_t ... param);
      };

   private:
      void reset_db_handles(void);

      void trickle_thread_proc(void);

      void stop_trickle_thread(void);

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

   protected:
      const config_t&   config;

   private:
      DbEnv             dbenv;
      Db                sequences;

      buffer_stack_t    buffer_stack;

      std::vector<table_t*> tables;

      std::mutex        trickle_mtx;
      std::condition_variable trickle_cv;
      std::thread       trickle_thread;
      string_t          trickle_error;
      bool              trickle_thread_stop;

      bool              trickle;

   protected:
      table_t make_table(void) {return table_t(config, dbenv, sequences, buffer_stack);}

   public:
      berkeleydb_t(config_t&& config);

      ~berkeleydb_t(void);

      /// if trickle is enabled, dirty pages will be trickled to disk by a background thread
      void set_trickle(bool value) {trickle = value;}

      /// A convenience method that calls `berkeleydb_t::open` with a table array.
      status_t open(std::initializer_list<table_t*> tblist);

      /// Sets up the Berkeley DB environment and opens the sequence database, but not table databases.
      status_t open(table_t * const tblist[], size_t tblcnt);

      /// closes all table databases and the Berkeley DB environment
      status_t close(void);

      /// erases data from all tables in the database
      status_t truncate(void);

      /// writes modified data pages to disk
      status_t flush(void);

      /// rearranges data pages on disk to minimize unused space
      status_t compact(u_int& bytes);
};

///
/// @brief  Compares two Dbt values by calling `compare`
///
/// The top B-Tree comparison function template is for BDB v6 and up and the bottom
/// one is for prior versions.
///
template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2);

///
/// @brief  Compares two Dbt values by calling `compare` and reversing the result
///
/// The top B-Tree comparison function template is for BDB v6 and up and the bottom
/// one is for prior versions.
///
template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2);

///
/// @brief  Translates field value returned by the `extract` function into a Dbt 
///         value used by Berkeley DB as a secondary datavase value
///
template <s_field_cb_t extract>
int sc_extract_cb(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);

#endif // BERKELEYDB_H

