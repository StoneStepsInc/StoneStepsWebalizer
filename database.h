/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

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
// database_t
//
// -----------------------------------------------------------------
class database_t {
   private:
      //
      // Define BDB callback types (bt_compare_fcn_type, etc are deprecated)
      //
      typedef int (*sc_extract_cb_t)(Db *secondary, const Dbt *key, const Dbt *data, Dbt *result);
      typedef int (*bt_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);
      typedef int (*dup_compare_cb_t)(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp);

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
      #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 5
      #define __gcc_bug11407__(cb) cb
      #else
      inline sc_extract_cb_t __gcc_bug11407__(sc_extract_cb_t cb) {return cb;}
      #endif

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
      // Each table contains the primary database, indexed by a numeric 
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

            Db                   table;      // primary database

            Db                   *values;    // pointer to the values database (stored in indexes)

            DbSequence           *sequence;  // source of primary keys

            string_t             errmsg;

            vector_t<db_desc_t>  indexes;    // secondary databases

            bool                 threaded;

            bool                 readonly;

         private:
            const db_desc_t *get_sc_desc(const char *dbname) const;
            db_desc_t *get_sc_desc(const char *dbname);

         public:
            table_t(DbEnv *env);

            ~table_t(void);

            void set_threaded(bool value) {threaded = value;}

            bool get_threaded(void) const {return threaded;}

            void set_readonly(bool value) {readonly = value;}

            bool get_readonly(void) const {return readonly;}

            void init_db_handles(void);
            void destroy_db_handles(void);

            const string_t& get_errmsg(void) const {return errmsg;}

            int open(const char *dbpath, const char *dbname, bt_compare_cb_t btcb);

            int close(void);

            int truncate(u_int32_t *count = NULL);

            int compact(u_int& bytes);

            int sync(void);

            void set_values_db(const char *dbname) {values = secondary_db(dbname);}

            int associate(const char *dbpath, const char *dbname, bt_compare_cb_t btcb, dup_compare_cb_t dpcb, sc_extract_cb_t sccb = NULL);

            int associate(const char *dbname, sc_extract_cb_t sccb, bool rebuild);

            Db& primary_db(void) {return table;}
            const Db& primary_db(void) const {return table;}

            Db *secondary_db(const char *dbname);
            const Db *secondary_db(const char *dbname) const;

            Db *values_db(void) {return values;}
            const Db *values_db(void) const {return values;}

            int open_sequence(Db& seqdb, const char *colname, int32_t cachesize);

            db_seq_t get_seq_id(int32_t delta = 1);
            
            db_seq_t query_seq_id(void);

            u_long count(const char *dbname = NULL) const;
      };

      // -----------------------------------------------------------------
      //
      // node_handler_t
      //
      // -----------------------------------------------------------------
      template <typename node_t>
      class node_handler_t {
         public:
            static bool put_node(table_t& table, u_char *buffer, const node_t& unode);

            static bool get_node_by_id(const table_t& table, u_char *buffer, node_t& unode, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);

            static bool delete_node(table_t& table, u_char *buffer, const keynode_t<u_long>& node);
      };

      // -----------------------------------------------------------------
      //
      // node_value_handler_t
      //
      // -----------------------------------------------------------------
      //
      // Similar to node_handler_t, but is instantiated only for nodes 
      // that can be looked up by value, such as hosts, URLs, etc.
      //
      template <typename node_t>
      class node_value_handler_t {
         public:
            static bool get_node_by_value(const table_t& table, u_char *buffer, node_t& unode, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL);
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
      
      // -----------------------------------------------------------------
      //
      // DbEnvEx initializes DbEnv before table databases are created
      //
      // -----------------------------------------------------------------
      class DbEnvEx : public DbEnv {
         public:
            DbEnvEx(u_int32_t flags);
            
            ~DbEnvEx(void);
      };

   private:
      void reset_db_handles(void);

      void trickle_thread_proc(void);

      #ifdef _WIN32
      static unsigned int __stdcall trickle_thread_proc(void *arg);
      #else
      static void *trickle_thread_proc(void *arg);
      #endif

      static void db_error_cb(const DbEnv *dbenv, const char *errpfx, const char *errmsg);

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

      static DbSequence *new_db_sequence(Db& db, u_int32_t flags);
      static void delete_db_sequence(DbSequence *dbseq);
      
      //
      // Memory management functions for the environment
      //
      static void *malloc(size_t size);
      static void *realloc(void *block, size_t size);
      static void free(void *block);

   private:
      const config_t&   config;

      DbEnvEx           dbenv;
      Db                sequences;

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
      table_t           dhosts;
      table_t           scodes;
      table_t           daily;
      table_t           hourly;
      table_t           totals;
      table_t           countries;
      table_t           system;

      table_t           *tables[18];

      thread_t          trickle_thread;
      event_t           trickle_event;
      string_t          trickle_error;
      bool              trickle_thread_stop;
      bool              trickle_thread_stopped;

      bool              readonly;
      bool              trickle;

      string_t          dbpath;

   public:
      database_t(const config_t& config);

      ~database_t(void);

      void set_trickle(bool value) {trickle = value;}

      const string_t& get_dbpath(void) const {return dbpath;}

      void set_readonly(bool value) {readonly = value;}

      bool get_readonly(void) const {return readonly;}

      bool open(void);

      bool close(void);

      bool truncate(void);

      bool flush(void);

      bool rollover(const tstamp_t& tstamp);

      int compact(u_int& bytes);

      bool attach_indexes(bool rebuild);

      // urls
      u_long get_unode_id(void) {return (u_long) urls.get_seq_id();}

      iterator<unode_t> begin_urls(const char *dbname) const {return iterator<unode_t>(urls, dbname);}

      reverse_iterator<unode_t> rbegin_urls(const char *dbname) const {return reverse_iterator<unode_t>(urls, dbname);}

      bool put_unode(const unode_t& unode);

      bool get_unode_by_id(unode_t& unode, unode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_unode_by_value(unode_t& unode, unode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      u_long get_ucount(const char *dbname = NULL) const {return urls.count(dbname);}

      // hosts
      u_long get_hnode_id(void) {return (u_long) hosts.get_seq_id();}

      iterator<hnode_t> begin_hosts(const char *dbname) const {return iterator<hnode_t>(hosts, dbname);}

      reverse_iterator<hnode_t> rbegin_hosts(const char *dbname) const {return reverse_iterator<hnode_t>(hosts, dbname);}

      bool put_hnode(const hnode_t& hnode);

      bool get_hnode_by_id(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_hnode_by_value(hnode_t& hnode, hnode_t::s_unpack_cb_t upcb, void *arg) const;

      u_long get_hcount(const char *dbname = NULL) const {return hosts.count(dbname);}

      // visits
      bool put_vnode(const vnode_t& vnode);

      iterator<vnode_t> begin_visits(void) const {return iterator<vnode_t>(visits, NULL);}

      bool get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_vnode_by_id(vnode_t& vnode, vnode_t::s_unpack_cb_t upcb, const void *arg) const;

      bool delete_visit(const keynode_t<u_long>& vnode);

      u_long get_vcount(const char *dbname = NULL) const {return visits.count(dbname);}

      //
      // downloads
      //
      u_long get_dlnode_id(void) {return (u_long) downloads.get_seq_id();}

      iterator<dlnode_t> begin_downloads(const char *dbname) const {return iterator<dlnode_t>(downloads, dbname);}

      reverse_iterator<dlnode_t> rbegin_downloads(const char *dbname) const {return reverse_iterator<dlnode_t>(downloads, dbname);}

      bool put_dlnode(const dlnode_t& dlnode);

      bool get_dlnode_by_id(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_dlnode_by_value(dlnode_t& dlnode, dlnode_t::s_unpack_cb_t upcb, void *arg) const;

      u_long get_dlcount(const char *dbname = NULL) const {return downloads.count(dbname);}

      bool delete_download(const keynode_t<u_long>& danode);

      //
      // active downloads
      //
      bool put_danode(const danode_t& danode);

      iterator<danode_t> begin_active_downloads(void) const {return iterator<danode_t>(active_downloads, NULL);}

      bool get_danode_by_id(danode_t& danode, danode_t::s_unpack_cb_t upcb, void *arg) const;

      u_long get_dacount(const char *dbname = NULL) const {return active_downloads.count(dbname);}

      //
      // agents
      //
      u_long get_anode_id(void) {return (u_long) agents.get_seq_id();}

      iterator<anode_t> begin_agents(const char *dbname) const {return iterator<anode_t>(agents, dbname);}

      reverse_iterator<anode_t> rbegin_agents(const char *dbname) const {return reverse_iterator<anode_t>(agents, dbname);}

      bool put_anode(const anode_t& anode);

      bool get_anode_by_id(anode_t& anode, anode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_anode_by_value(anode_t& anode, anode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      u_long get_acount(const char *dbname = NULL) const {return agents.count(dbname);}

      //
      // referrers
      //
      u_long get_rnode_id(void) {return (u_long) referrers.get_seq_id();}

      iterator<rnode_t> begin_referrers(const char *dbname) const {return iterator<rnode_t>(referrers, dbname);}

      reverse_iterator<rnode_t> rbegin_referrers(const char *dbname) const {return reverse_iterator<rnode_t>(referrers, dbname);}

      bool put_rnode(const rnode_t& rnode);

      bool get_rnode_by_id(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_rnode_by_value(rnode_t& rnode, rnode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      u_long get_rcount(const char *dbname = NULL) const {return referrers.count(dbname);}

      //
      // search strings
      //
      u_long get_snode_id(void) {return (u_long) search.get_seq_id();}

      iterator<snode_t> begin_search(const char *dbname) const {return iterator<snode_t>(search, dbname);}

      reverse_iterator<snode_t> rbegin_search(const char *dbname) const {return reverse_iterator<snode_t>(search, dbname);}

      bool put_snode(const snode_t& snode);

      bool get_snode_by_id(snode_t& snode, snode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_snode_by_value(snode_t& snode, snode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      u_long get_scount(const char *dbname = NULL) const {return search.count(dbname);}

      //
      // users
      //
      u_long get_inode_id(void) {return (u_long) users.get_seq_id();}

      iterator<inode_t> begin_users(const char *dbname) const {return iterator<inode_t>(users, dbname);}

      reverse_iterator<inode_t> rbegin_users(const char *dbname) const {return reverse_iterator<inode_t>(users, dbname);}

      bool put_inode(const inode_t& inode);

      bool get_inode_by_id(inode_t& inode, inode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      bool get_inode_by_value(inode_t& inode, inode_t::s_unpack_cb_t upcb = NULL, void *arg = NULL) const;

      u_long get_icount(const char *dbname = NULL) const {return users.count(dbname);}

      //
      // errors
      //
      u_long get_rcnode_id(void) {return (u_long) errors.get_seq_id();}

      iterator<rcnode_t> begin_errors(const char *dbname) const {return iterator<rcnode_t>(errors, dbname);}

      reverse_iterator<rcnode_t> rbegin_errors(const char *dbname) const {return reverse_iterator<rcnode_t>(errors, dbname);}

      bool put_rcnode(const rcnode_t& rcnode);

      bool get_rcnode_by_id(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_rcnode_by_value(rcnode_t& rcnode, rcnode_t::s_unpack_cb_t upcb, void *arg) const;

      u_long get_errcount(const char *dbname = NULL) const {return errors.count(dbname);}

      //
      // daily hosts
      //
      u_long get_tnode_id(void) {return (u_long) dhosts.get_seq_id();}

      bool clear_daily_hosts(void) {return !dhosts.truncate() ? true : false;}

      iterator<tnode_t> begin_dhosts(void) const {return iterator<tnode_t>(dhosts, NULL);}

      bool put_tnode(const tnode_t& tnode);

      bool get_tnode_by_id(tnode_t& tnode, tnode_t::s_unpack_cb_t upcb, void *arg) const;

      bool get_tnode_by_value(tnode_t& tnode, tnode_t::s_unpack_cb_t upcb, void *arg) const;

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
      
      //
      //
      //
      bool fix_v3_8_0_4(void);

};


#endif // __DATABASE_H
