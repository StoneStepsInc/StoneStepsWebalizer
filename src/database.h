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
#include "thread.h"

#include <db_cxx.h>

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
      // to a valid object until config_t::release is called. Otherwise config_t make
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
            cursor_iterator_base(const Db *db);
            
            ~cursor_iterator_base(void);

            bool is_error(void) const {return (error && error != DB_NOTFOUND) ? true : false;}

            int get_error(void) const {return error;}

            void close(void);
      };

      class cursor_iterator : public cursor_iterator_base {
         private:
            db_recno_t   count;     // remaining duplicate count (zero if unique)

         public:
            cursor_iterator(const Db *db) : cursor_iterator_base(db) {count = 0;}

            db_recno_t dup_count(void) const {return count;}

            bool set(Dbt& key, Dbt& data, Dbt *pkey);

            bool next(Dbt& key, Dbt& data, Dbt *pkey, bool dupkey = false);
      };

      class cursor_reverse_iterator : public cursor_iterator_base {
         public:
            cursor_reverse_iterator(const Db *db) : cursor_iterator_base(db) {}

            bool prev(Dbt& key, Dbt& data, Dbt *pkey);
      };

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

         private:
            const db_desc_t *get_sc_desc(const char *dbname) const;
            db_desc_t *get_sc_desc(const char *dbname);

            Db& primary_db(void) {return *table;}

            Db *secondary_db(const char *dbname);

         public:
            table_t(DbEnv& env, Db& seqdb);

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

            const Db& primary_db(void) const {return *table;}

            const Db *secondary_db(const char *dbname) const;

            Db *values_db(void) {return values;}
            const Db *values_db(void) const {return values;}

            int open_sequence(const char *colname, int32_t cachesize);

            db_seq_t get_seq_id(int32_t delta = 1);
            
            db_seq_t query_seq_id(void);

            uint64_t count(const char *dbname = NULL) const;

            template <typename node_t>
            bool put_node(u_char *buffer, const node_t& unode);

            template <typename node_t>
            bool get_node_by_id(u_char *buffer, node_t& unode, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

            bool delete_node(u_char *buffer, const keynode_t<uint64_t>& node);

            template <typename node_t>
            bool get_node_by_value(u_char *buffer, node_t& unode, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;
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
            cursor_iterator_base&   iter;          // cursor reference

            Dbt                     key;           // iterator key

            Dbt                     data;          // data

            Dbt                     pkey;          // optional primary key

         public:
            iterator_base(cursor_iterator_base& iter);

            ~iterator_base(void);

            void set_buffer_size(u_int maxkey, u_int maxdata);

            void close(void) {iter.close();}

            bool is_error(void) const {return iter.is_error();}

            int get_error(void) const {return iter.get_error();}

      };

      template <typename node_t>
      class iterator : public iterator_base<node_t> {
         private:
            //
            // key, data and pkey are members of a dependent class and are  
            // not visible if unqualified names are used      [temp.dep.3]
            //
            using iterator_base<node_t>::key;
            using iterator_base<node_t>::data;
            using iterator_base<node_t>::pkey;

            cursor_iterator         iter;

            bool                    primdb;        // is primary database?

         private:
            // do not allow assignments
            iterator& operator = (const iterator&) {return *this;}

         public:
            iterator(const table_t& table, const char *dbname);

            bool next(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);
      };

      template <typename node_t>
      class reverse_iterator : public iterator_base<node_t> {
         private:
            //
            // key, data and pkey are members of a dependent class and are  
            // not visible if unqualified names are used      [temp.dep.3]
            //
            using iterator_base<node_t>::key;
            using iterator_base<node_t>::data;
            using iterator_base<node_t>::pkey;

            cursor_reverse_iterator iter;

            bool                    primdb;        // is primary database?

         private:
            // do not allow assignments
            reverse_iterator& operator = (const reverse_iterator&) {return *this;}

         public:
            reverse_iterator(const table_t& table, const char *dbname);

            bool prev(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);
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

      table_t make_table(void) {return table_t(dbenv, sequences);}

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

      u_char            *buffer;

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

      bool get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, const void *arg) const;

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
