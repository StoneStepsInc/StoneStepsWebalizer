/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   berkeleydb_tmpl.cpp
*/

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

#define DBFLAGS               ((u_int32_t) 0)
#define DBENVFLAGS            ((u_int32_t) 0)

#define FILEMASK              0664              // database file access mask (rw-rw-r--)

//
// B-Tree comparison function template for BDB v6 and up (top) and for prior versions 
// (bottom). Only one will be used in any build, but it's cleaner without having to 
// use conditional blocks.
//
template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   int64_t diff = compare(dbt1->get_data(), dbt1->get_size(), dbt2->get_data(), dbt2->get_size());
   return diff < 0ll ? -1 : diff > 0ll ? 1 : 0;
}

template <s_compare_cb_t compare>
int bt_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2)
{
   int64_t diff = compare(dbt1->get_data(), dbt1->get_size(), dbt2->get_data(), dbt2->get_size());
   return diff < 0ll ? -1 : diff > 0ll ? 1 : 0;
}

//
// B-Tree comparison function template (reverse order)
//
template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
   int64_t diff = compare(dbt1->get_data(), dbt1->get_size(), dbt2->get_data(), dbt2->get_size());
   return diff < 0ll ? 1 : diff > 0ll ? -1 : 0;
}

template <s_compare_cb_t compare>
int bt_reverse_compare_cb(Db *db, const Dbt *dbt1, const Dbt *dbt2)
{
   int64_t diff = compare(dbt1->get_data(), dbt1->get_size(), dbt2->get_data(), dbt2->get_size());
   return diff < 0ll ? 1 : diff > 0ll ? -1 : 0;
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
bool berkeleydb_t::iterator<node_t>::next(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, param_t ... param)
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

   if(node.template s_unpack_data<param_t...>(data.get_data(), data.get_size(), upcb, std::forward<param_t>(param) ...) != data.get_size())
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
bool berkeleydb_t::reverse_iterator<node_t>::prev(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, param_t ... param)
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

   if(node.template s_unpack_data<param_t...>(data.get_data(), data.get_size(), upcb, std::forward<param_t>(param) ...) != data.get_size())
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
bool berkeleydb_t::table_t::get_node_by_id(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
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

   if(node.template s_unpack_data<param_t...>(data.get_data(), data.get_size(), upcb, std::forward<param_t>(param) ...) != data.get_size())
      return false;
   
   node.storage_info.set_from_storage();

   return true;
}

template <typename node_t, typename ... param_t>
bool berkeleydb_t::table_t::get_node_by_value(storable_t<node_t>& node, typename node_t::template s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
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
   {cursor_dup_iterator cursor(values);

   // find the first value hash and get the primary key and value data
   if(!cursor.set(key, data, &pkey))
      return false;

   do {
      // if the node value matched, break out
      if(!node.s_compare_value(data.get_data(), data.get_size()))
         break;

   } while(cursor.next(key, data, &pkey));

   // the destructor will close the cursor
   if(cursor.get_error())
      return false;
   }

   // unpack the primary key
   if(node.s_unpack_key(pkey.get_data(), pkey.get_size()) != pkey.get_size())
      return false;

   // unpack data
   if(node.template s_unpack_data<param_t...>(data.get_data(), data.get_size(), upcb, std::forward<param_t>(param) ...) != data.get_size())
      return false;

   node.storage_info.set_from_storage();

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
