/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   hashtab.h
*/
#ifndef HASHTAB_H
#define HASHTAB_H

#include "tstring.h"
#include "types.h"
#include "storable.h"

#include <list>
#include <stdexcept>

#define LMAXHASH     1048576ul
#define MAXHASH      16384ul
#define SMAXHASH     1024ul

///
/// Hash tables may contain primary objects, such as user agents or URLs, and 
/// object groups, such all versions of a particular browser or all URLs in a 
/// selected directory.
///
enum nodetype_t {
   OBJ_REG = 0,                     /* Regular object               */
   OBJ_GRP = 2                      /* Grouped object               */
};

///
/// @name   Hash functions
///
/// @{

///
/// The sdbm hash function below generates 64-bit hash values that are well 
/// distributed across the entire 64-bit range.
///
inline uint64_t hash_byte(uint64_t hashval, u_char b) {return (uint64_t) b + (hashval << 6) + (hashval << 16) - hashval;}

uint64_t hash_bin(uint64_t hashval, const u_char *buf, size_t blen);
uint64_t hash_str(uint64_t hashval, const char *str, size_t slen);
template <typename type_t> uint64_t hash_num(uint64_t hashval, type_t num);

inline uint64_t hash_ex(uint64_t hashval, const string_t& str) {return hash_str(hashval, str.c_str(), str.length());}
inline uint64_t hash_ex(uint64_t hashval, u_int data) {return hash_num(hashval, data);}
/// @}

///
/// @struct hash_string
///
/// @brief  A hash function for `std::unordered_map` with a `string_t` key.
///
struct hash_string {
   size_t operator () (const string_t& str) const
   {
      //
      // The sdbm hash is well-distributed across the entire 64-bit range, so we 
      // can throw away the top half of the hash value on the 32-bit platform. 
      //
      return (size_t) hash_ex(0, str);
   }
};

template <typename node_t> struct htab_node_t;
template <typename node_t> using node_list_t = std::list<htab_node_t<node_t>*>;

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
/// Hash table nodes may be identified by a simple string key or by a compound key 
/// via node-specific `param_block`. Note that `match_key_ex` is not a virtual method
/// and is called in template methods using node types that define `param_block` 
/// appropriate for each node type.
///
struct htab_obj_t {
      ///
      /// @struct param_block
      ///
      /// @brief  A special compound key structure containing a simple string key.
      ///
      struct param_block {
         const char *key;           ///< A pointer to a node-specific key string
      };

      public:
         virtual ~htab_obj_t(void) {}

         ///
         /// @brief  Returns `true` if the key in `param_block` is equal to the node 
         ///         key, `false` otherwise.
         ///
         /// `match_key_ex` is not a virtual method and must be overriden for those 
         /// node types that make use of compound keys.
         ///
         bool match_key_ex(const param_block *pb) const 
         {
            return match_key(string_t::hold(pb->key));
         }

         /// Returns `true` if `key` is equal to the node key.
         virtual bool match_key(const string_t& key) const = 0;

         /// Returns a type of this node
         virtual nodetype_t get_type(void) const = 0;

         /// Returns a hash value for this node
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
      htab_node_t    *prev;                ///< Previous hash table node
      uint64_t       hashval;              ///< Hash value of the node object.
      int64_t        tstamp;               ///< Relative time stamp associated with this node.

      typename node_list_t<node_t>::iterator    lsnode;

      public:
         htab_node_t(node_t *node, uint64_t hashval, typename node_list_t<node_t>::iterator lsnode, int64_t tstamp) :
               node(node), next(nullptr), prev(nullptr), tstamp(tstamp), lsnode(lsnode), hashval(hashval)
         {
         }

         ~htab_node_t(void) 
         {
            delete node;
         }
};

///
/// @class  hash_table_base
///
/// @brief  A non-template hash table base class.
///
/// This non-template class is intended for definitions that can be passed between
/// different hash table instantiations without having to declare member function
/// templates to access these definitions in their own instantiations.
///
/// Virtual functions declared in this class are not intended to describe the full
/// hash table interface, but rather serve the purpose of being able to call these
/// functions against a base hash table class in a loop for different hash table
/// types.
///
class hash_table_base {
   public:
      ///
      /// @struct tm_range_t
      ///
      /// @brief  Represents a range of time stamps between the last and the first 
      ///         nodes in the time stamp list.
      ///
      /// A range with both time stamps being zero is a null range and a range with
      /// either of the time stamps being zero, but not both, is an open-ended range.
      /// Neither should be used as an actual time range.
      ///
      struct tm_range_t {
         int64_t  min_tstamp;          ///< Earliest time stamp in the hash table.
         int64_t  max_tstamp;          ///< Latest time stamp in the hash table.

         /// Returns the time stamp range value.
         operator int64_t (void) const
         {
            return max_tstamp - min_tstamp;
         }
      };

   public:
      /// Returns estimated memory size for this hash table.
      virtual size_t get_memsize(void) const = 0;

      /// Swaps out oldest nodes with time stamps less than or equal `tstamp` to some external storage.
      virtual void swap_out(int64_t tstamp, size_t maxsize = 0) = 0;
};

///
/// @class  hash_table
///
/// @tparam node_t   Hash table node type
///
/// @brief  A specialized hash table class that provides simple and compound key 
///         look-up functionality and also maintains a time-ordered list of all
///         regular object nodes in the hash table.
///
/// This hash table is optimized as a cache for some external storage and minimizes 
/// the number of hash value computations and key comparisons at the expense of having
/// a less flexible and robust interface in the sense that the caller is trusted to 
/// compute correct hash values for some hash table methods.
/// 
/// The caller is expected to perform these steps when working with instances of this
/// class:
///
///   * Obtain the hash value for the node object
///   * Call `find_node` with this hash value and the node key
///   * If `find_node` returns NULL, look up the key in the external storage
///   * If the key was not found in the external storage, call `put_node` with
///     the hash value to insert a new node in the hash table. Otherwise, insert
///     the node found in the external storage.
///   * Call `swap_out` with a time stamp far enough in the past to move oldest 
///     nodes to the external storage
///   * Once main processing is done, walk the hash table and save all remaining
///     nodes in the external storage.
///
/// Only regular object nodes participate in time stamp ordering and are inserted
/// into the time stamp list (`tmlist`). Consequently, only regular object nodes may
/// be swapped out to the external storage while log processing is in progress.
///
template <typename node_t>
class hash_table : public hash_table_base {
   private:
      ///
      /// @struct bucket_t
      ///
      /// @brief  A collection of hash table nodes whose keys yield the same hash value
      ///
      struct bucket_t {
         htab_node_t<node_t>  *head;      ///< Head of the node list in this bucket
         u_int                count;      ///< Number of nodes in this bucket

         public:
            bucket_t(void) : head(nullptr), count(0)
            {
            }
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

      ///
      /// @brief  Evaluates whether the node can be swapped out to some external storage.
      ///
      /// Should returns `true` if `node` can be swapped out, `false` otherwise.
      ///
      /// Note that only the inner node should be evaluated and not the storable flags.
      ///
      typedef bool (*eval_cb_t)(const typename inner_node<node_t>::type *node, void *arg);

      ///
      /// @brief  Saves the node in some external storage.
      ///
      typedef void (*swap_cb_t)(node_t *node, void *arg);

   public:
      ///
      /// @class  iterator_base
      ///
      /// @tparam list_iter_t    Either `std::list::iterator` or `std::list::const_iterator`.
      ///
      /// @brief  A hash table iterator template for `const` and non-`const` iterator types.
      ///
      /// This is a specialized iterator that walks over two hash table lists - the unordered
      /// group node list and the time-ordered regular node list.
      ///
      /// The iterator is initially positioned before the first node in the combined logical
      /// node list, which simplifies the traversal loop by having only one method to call to
      /// advance and check whethere there are any more nodes in the logical list.
      ///
      /// Neither of the underlying lists can change while there are any active iterators
      /// referencing any of those lists.
      ///
      template <typename list_iter_t>
      class iterator_base {
         friend class hash_table<node_t>;

         private:
            bool pre;                     ///< A pre-first node position indicator.

            list_iter_t    grpnode;       ///< The iterator pointing to the current group node.
            list_iter_t    grpend;        ///< The end iterator of the group node list.

            list_iter_t    tmnode;        ///< The iterator pointing to the current regular node.
            list_iter_t    tmend;         ///< The end iterator of the time-ordered list of regular nodes.

         protected:
            iterator_base(list_iter_t grpbegin, list_iter_t grpend, list_iter_t tmbegin, list_iter_t tmend) : 
               pre(true), grpnode(grpbegin), grpend(grpend), tmnode(tmbegin), tmend(tmend)
            {
            }

         public:
            /// Returns the current combined list item, if there is one, or `nullptr` otherwise.
            node_t *item(void) 
            {
               // return a NULL if we have not started iterating
               if(pre)
                  return nullptr;

               // if there are nodes in the group list, return a group node
               if(grpnode != grpend)
                  return (*grpnode)->node;

               // if there are nodes in the time-ordered list, return a regular node
               if(tmnode != tmend)
                  return (*tmnode)->node;

               // otherwise there are no nodes in either of the lists
               return nullptr;
            }

            /// Moves to the next item in the combined list and returns that item, if there is one or `nullptr` otherwise.
            node_t *next(void)
            {
               //
               // When the iterator is positioned before the first node, there's no
               // change in either of the list iterators - we just return the node
               // from the first valid iterator and clear the pre-first node flag.
               //
               if(pre) {
                  pre = false;

                  // if the group list has any nodes, return the first one
                  if(grpnode != grpend)
                     return (*grpnode)->node;

                  // if there are no group nodes, return the first regular node
                  if(tmnode != tmend)
                     return (*tmnode)->node;

                  // both lists are empty, return NULL
                  return nullptr;
               }

               //
               // Once we returned the first node, walk the group list until we run out 
               // of group nodes.
               //
               if(grpnode != grpend) {
                  grpnode++;

                  if(grpnode != grpend)
                     return (*grpnode)->node;

                  if(tmnode != tmend)
                     return (*tmnode)->node;

                  return nullptr;
               }

               //
               // Finally, walk the time-ordered regular node list until we run out of 
               // regular nodes too.
               //
               if(tmnode != tmend)
                  tmnode++;

               if(tmnode != tmend)
                  return (*tmnode)->node;

               return nullptr;
            }
      };

      ///
      /// @class  iterator
      ///
      /// @brief  A hash table iterator.
      ///
      class iterator : public iterator_base<typename node_list_t<node_t>::iterator> {
         friend class hash_table<node_t>;

         private:
            typedef typename node_list_t<node_t>::iterator list_iter_t;

         public:
            iterator(list_iter_t grpbegin, list_iter_t grpend, list_iter_t tmbegin, list_iter_t tmend) :
               iterator_base<list_iter_t>(grpbegin, grpend, tmbegin, tmend)
            {
            }
      };

      ///
      /// @class  const_iterator
      ///
      /// @brief  A hash table iterator to iterate over `const` nodes.
      ///
      /// This iterator class hides both methods of the base template iterator class
      /// and overrides them with versions that call the base and return pointers to
      /// `const` nodes of the same type.
      ///
      class const_iterator : private iterator_base<typename node_list_t<node_t>::const_iterator> {
         friend class hash_table<node_t>;

         private:
            typedef typename node_list_t<node_t>::const_iterator list_iter_t;

         private:
            const_iterator(list_iter_t grpbegin, list_iter_t grpend, list_iter_t tmbegin, list_iter_t tmend) :
               iterator_base<list_iter_t>(grpbegin, grpend, tmbegin, tmend)
            {
            }

         public:
            const node_t *item(void) {return iterator_base<list_iter_t>::item();}

            const node_t *next(void) {return iterator_base<list_iter_t>::next();}
      };

   private:
      size_t      count;      ///< Number of hash table entries
      size_t      maxhash;    ///< Number of buckets in the hash table
      size_t      emptycnt;   ///< Number of empty buckets
      size_t      memsize;    ///< Estimated serialized size in bytes of all nodes.
      bucket_t    *htab;      ///< Buckets

      node_list_t<node_t>  tmlist;  ///< Time-ordered list of regular nodes.
      node_list_t<node_t>  grplist; ///< Unordered list of group nodes.

      eval_cb_t   evalcb;     ///< Evaluation callback.
      swap_cb_t   swapcb;     ///< Swap out callback.
      void        *cbarg;     ///< Swap out and evaluation callbacks argument.

   private:
      /// Completely unlinks the specified node from the bucket list.
      void unlink_node(bucket_t& bucket, htab_node_t<node_t> *nptr) const;

      /// Moves the specified node to the beginning of the bucket list.
      void move_to_front(bucket_t& bucket, htab_node_t<node_t> *nptr) const;

      /// Implements `find_node` methods that look up nodes with a hash value and a key and move the node to the end of the time stamp list.
      template <typename ... param_t>
      node_t *find_node_ex(uint64_t hashval, nodetype_t type, int64_t tstamp, bool (node_t::*match_key)(param_t ... args) const, param_t ... param);

   public:
      /// Constructs a hash table with the specified number of buckets.
      hash_table(size_t maxhash = MAXHASH, swap_cb_t swapcb = nullptr, void *cbarg = nullptr, eval_cb_t evalcb = nullptr);

      /// Destroys the hash table and its contents.
      ~hash_table(void);

      ///
      /// @name   Informational
      ///
      /// @{

      /// Returns the number of nodes in the hash table.
      size_t size(void) const {return count;}

      /// Returns estimated memory size for this hash table.
      size_t get_memsize(void) const override {return memsize;}
      /// @}

      ///
      /// @brief  Swap-out interface
      ///
      /// @{

      /// Sets a swap-out callback function and its argument.
      void set_swap_out_cb(swap_cb_t swapcb, void *arg, eval_cb_t evalcb = nullptr);

      /// Swaps out oldest nodes with time stamps less than or equal `tstamp` to some external storage.
      void swap_out(int64_t tstamp, size_t maxsize = 0) override;
      /// @}

      ///
      /// @name   Iterators
      ///
      /// @{

      iterator begin(void) {return iterator(grplist.begin(), grplist.end(), tmlist.begin(), tmlist.end());}

      const_iterator begin(void) const {return const_iterator(grplist.begin(), grplist.end(), tmlist.begin(), tmlist.end());}
      /// @}

      ///
      /// @name   Hash table interface
      ///
      /// @{

      /// Deletes all hash table nodes.
      void clear(void);

      /// Looks for a node with a string key and does not move the node to the end of the time stamp list.
      const node_t *find_node(const string_t& key, nodetype_t type) const;

      /// Looks for a node with a string key and, if found, moves it to the end of the time stamp list.
      node_t *find_node(const string_t& key, nodetype_t type, int64_t tstamp) {return find_node(hash_ex(0, key), key, type, tstamp);}

      /// Looks for a node with a hash value and a string key and, if found, moves it to the end of the time stamp list.
      node_t *find_node(uint64_t hashval, const string_t& str, nodetype_t type, int64_t tstamp);

      /// Looks for a node with a hash value and a compound key and, if found, moves it to the end of the time stamp list.
      node_t *find_node(uint64_t hashval, const typename node_t::param_block *params, nodetype_t type, int64_t tstamp);

      /// Obtains a hash value from `nptr` and inserts it into the corresponding bucket.
      node_t *put_node(node_t *nptr, int64_t tstamp);

      /// Inserts `node` into a bucket identified by `hashval`.
      node_t *put_node(uint64_t hashval, node_t *node, int64_t tstamp);
      /// @}

      ///
      /// @name   Miscellaneous
      ///
      /// @{

      /// Returns the time stamp range between the newest and oldest time stamps in the hash table.
      tm_range_t tm_range(void) const;
      /// @}
};

#endif  // HASHTAB_H
