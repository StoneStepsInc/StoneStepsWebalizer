/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   hashtab.h
*/
#ifndef HASHTAB_H
#define HASHTAB_H

#include "tstring.h"
#include "types.h"
#include "storable.h"

#include <stdexcept>

#define LMAXHASH     1048576ul
#define MAXHASH      16384ul
#define SMAXHASH     1024ul

//
// Hash tables may contain primary objects, such as user agents or URLs, and 
// object groups, such all versions of a particular browser or all URLs in a 
// selected directory.
//
enum nodetype_t {
   OBJ_REG = 0,                     /* Regular object               */
   OBJ_GRP = 2                      /* Grouped object               */
};

// -----------------------------------------------------------------------
//
// hash functions
//
// -----------------------------------------------------------------------

//
// The sdbm hash function below generates 64-bit hash values that are well 
// distributed across the entire 64-bit range.
//
inline uint64_t hash_byte(uint64_t hashval, u_char b) {return (uint64_t) b + (hashval << 6) + (hashval << 16) - hashval;}

uint64_t hash_bin(uint64_t hashval, const u_char *buf, size_t blen);
uint64_t hash_str(uint64_t hashval, const char *str, size_t slen);
template <typename type_t> uint64_t hash_num(uint64_t hashval, type_t num);

inline uint64_t hash_ex(uint64_t hashval, const char *str) {return (str && *str) ? hash_str(hashval, str, strlen(str)) : 0;}
inline uint64_t hash_ex(uint64_t hashval, const string_t& str) {return hash_str(hashval, str.c_str(), str.length());}
inline uint64_t hash_ex(uint64_t hashval, u_int data) {return hash_num(hashval, data);}

///
/// @struct struct htab_obj_t
///
/// @brief  A generic hash table object interface.
///
/// Hash table node and objects historically were combined in a single object in 
/// this project. Consequently, all objects within a hash table were called nodes 
/// (e.g. `hnode_t` for a host object or a `unode_t` for a URL object). Later on, 
/// the hash table node object was split onto the base object for hash table content 
/// objects (`htab_obj_t`) and the hash table node object that maintains content 
/// objects within a hash table (`htab_node_t`).
///
struct htab_obj_t {
      //
      // param_block provides a way to search the hash table using compound keys. 
      // The base class has no data members and is defined to allow hash table 
      // instantiation for classes that do not make use of compound keys.
      //
      struct param_block {};

      public:
         virtual ~htab_obj_t(void) {}

         virtual const string_t& key(void) const = 0;

         virtual nodetype_t get_type(void) const = 0;

         virtual uint64_t get_hash(void) const = 0;
};

///
/// @struct htab_node_t
///
/// @brief  A hash table node that contains a single object of `node_t` type and
///         is linked to other `htab_node_t` nodes with the same key hash value.
///
/// Hash table `node_t` ojects must be dynamically allocated and will be deleted
/// by calling the `delete` operator.
///
template <typename node_t> 
struct htab_node_t {
      node_t         *node;                ///< Hash table content object
      htab_node_t    *next;                ///< Next hash table node

      public:
         htab_node_t(void) : node(NULL), next(NULL) 
         {
         }

         htab_node_t(node_t *node) : node(node), next(NULL) 
         {
         }

         ~htab_node_t(void) 
         {
            delete node;
         }
};

///
/// @class  hash_table
///
/// @tparam node_t   Hash table node type
/// @tparam key_t    Node key type
///
/// @brief  A generic hash table class
///
template <typename node_t, typename key_t = string_t>
class hash_table {
   private:
      enum swap_code {swap_ok, swap_failed, swap_inuse};

      struct bucket_t {
         htab_node_t<node_t>   *head;
         u_int                      count;

         public:
            bucket_t(void) {head = NULL; count = 0;}
      };

   public:
      ///
      /// @struct inner_node
      ///
      /// @brief  A primary class template to define a type of hash table nodes 
      ///         that are not storable objects.
      ///
      template <typename T>
      struct inner_node {
         typedef T type;
      };

      ///
      /// @struct inner_node<storable_t<T>>
      ///
      /// @brief  A specialization of the `inner_node` template to define a type
      ///         of a data node at the base of a storable object.
      ///
      template <template <typename> class storable_t, typename T>
      struct inner_node<storable_t<T>> {
         typedef T type;
      };

      //
      // Evaluation callback is called first. Returns true if the node
      // can be swapped out, false otherwise. In the latter case the
      // swap-out callback will not be called.
      //
      typedef bool (*eval_cb_t)(const typename inner_node<node_t>::type *node, void *arg);

      //
      // If evaluation was successful, the swap-out callback is called.
      // If swap-out callback returns false, the swap_out method exits 
      // the loop and returns false.
      //
      typedef bool (*swap_cb_t)(node_t *node, void *arg);

   public:
      class iterator {
         friend class hash_table<node_t, key_t>;

         private:
            bucket_t *htab;
            uint64_t   index;
            uint64_t   maxhash;
            htab_node_t<node_t> *nptr;

         protected:
            iterator(bucket_t _htab[], uint64_t _maxhash) {htab = _htab; index = 0; maxhash = _maxhash; nptr = NULL;}

         public:
            iterator(void) {htab = NULL; nptr = NULL; index = 0; maxhash = 0;}

            node_t *item(void) {return nptr ? nptr->node : NULL;}

            node_t *next(void)
            {
               if(htab == NULL)
                  return NULL;

               if(nptr && nptr->next) {
                  nptr = nptr->next;
                  return nptr->node;
               }

               while(index < maxhash) {
                  if((nptr = htab[index++].head) != NULL)
                     return nptr->node;
               }

               return NULL;
            }
      };

      class const_iterator : public iterator {
         friend class hash_table<node_t, key_t>;

         private:
            const_iterator(bucket_t _htab[], uint64_t _maxhash) : iterator(_htab, _maxhash) {}

         public:
            const_iterator(void) : iterator() {}

            const node_t *item(void) {return iterator::item();}

            const node_t *next(void) {return iterator::next();}
      };

   private:
      size_t      count;      // number of hash table entries
      size_t      maxhash;    // number of buckets in the hash table
      size_t      emptycnt;   // number of empty buckets
      bucket_t    *htab;      // buckets

      bool        swap;       // true, if some data is swapped out
      bool        cleared;    // true when the table has been cleared

      eval_cb_t   evalcb;     // swap out evaluation callback
      swap_cb_t   swapcb;     // swap out callback
      void        *cbarg;     // swap out callback argument

   private:
      htab_node_t<node_t> *move_node(htab_node_t<node_t> *nptr, htab_node_t<node_t> *prev, htab_node_t<node_t> **next) const
      {
         prev->next = nptr->next;
         nptr->next = *next;
         *next = nptr;
         return nptr;
      }

      swap_code swap_out_node(bucket_t& bucket, htab_node_t<node_t> **pptr);

      bool swap_out_bucket(bucket_t& bucket);

      virtual bool compare(const typename inner_node<node_t>::type *nptr, const typename inner_node<node_t>::type::param_block *params) const 
      {
         throw std::logic_error("This node type does not support searches with compound keys");
      }

   protected:
      virtual bool load_array_check(const node_t *) const {return true;}

   public:
      hash_table(size_t maxhash = MAXHASH, eval_cb_t evalcb = NULL, swap_cb_t swapcb = NULL, void *cbarg = NULL);

      ~hash_table(void);

      //
      // informational
      //
      size_t size(void) const {return count;}

      size_t buckets(void) const {return maxhash;} 

      size_t empty_buckets(void) const {return emptycnt;}

      //
      // swap-out interface
      //
      bool is_cleared(void) const {return cleared;}

      void set_cleared(bool value) {cleared = value;}

      bool is_swapped_out(void) const {return swap;}

      void set_swapped_out(bool value) {swap = value;}

      void set_swap_out_cb(eval_cb_t evalcb, swap_cb_t swapcb, void *arg);

      bool swap_out(void);

      //
      // iterators
      //
      iterator begin(void) {return iterator(htab, maxhash);}

      const_iterator begin(void) const {return const_iterator(htab, maxhash);}

      //
      // hash table interface
      //
      void clear(void);

      bool find_key(const key_t& key) const {return find_node(key) != NULL;}

      const node_t *find_node(const key_t& key) const {return find_node(hash_ex(0, key), key);}

      node_t *find_node(const key_t& key) {return find_node(hash_ex(0, key), key);}

      const node_t *find_node(uint64_t hashval, const key_t& key) const;

      node_t *find_node(uint64_t hashval, const key_t& key);

      node_t *find_node(uint64_t hashval, const key_t& str, nodetype_t type);

      node_t *find_node(uint64_t hashval, const typename node_t::param_block *params);

      node_t *put_node(const key_t& key, node_t *nptr) {return put_node(hash_ex(0, key), nptr);}

      node_t *put_node(node_t *nptr);

      node_t *put_node(uint64_t hashval, node_t *node);

      //
      // miscellaneous
      //
      uint64_t load_array(const typename inner_node<node_t>::type *array[]) const;
};

#endif  // HASHTAB_H
