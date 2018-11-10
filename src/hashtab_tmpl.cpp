/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   hashtab_tmpl.cpp
*/
#include <cstdlib>
#include <cstring>
#include <climits>

#include "hashtab.h"

template <typename node_t>
hash_table<node_t>::hash_table(size_t maxhash, swap_cb_t swapcb, void *cbarg, eval_cb_t evalcb) : 
      maxhash(maxhash), swapcb(swapcb), cbarg(cbarg), evalcb(evalcb), memsize(0)
{
   count = 0;
   emptycnt = maxhash;
   htab = new bucket_t[maxhash];
}

template <typename node_t>
hash_table<node_t>::~hash_table(void)
{
   clear();
   delete [] htab;
}

template <typename node_t>
void hash_table<node_t>::set_swap_out_cb(swap_cb_t swap, void *arg, eval_cb_t eval)
{
   evalcb = eval;
   swapcb = swap;
   cbarg = arg;
}

///
/// @param[in] tstamp   The last inclusive time stamp in the swap-out range.
/// @param[in] maxsize  The expected maximum memory size after the method returns
///
/// The caller can subtract the visit timeout value from the current log time stamp
/// to derive `tstamp`, which will swap out all nodes that haven't been touched in 
/// visit timeout time.
///
template <typename node_t>
void hash_table<node_t>::swap_out(int64_t tstamp, size_t maxsize)
{
   // cannot swap out without a callback 
   if(!swapcb)
      throw std::logic_error("Cannot swap out nodes without a swap callback");

   typename node_list_t<node_t>::iterator lsnode = tmlist.begin();

   //
   // Swap out oldest nodes with time stamps less than or equal to tstamp until the
   // remaining hash table memory size is less than maxsize. Note, though, that memory
   // size is an estimated quantity and we can reach zero while there are still nodes
   // in the hash table, so once the hash table memory size is zero, ignore it and
   // finish evaluating time stamps.
   //
   while(lsnode != tmlist.end() && (*lsnode)->tstamp <= tstamp && (!memsize || memsize > maxsize)) {
      // only regular nodes can be in the time stamp list
      if((*lsnode)->node->get_type() != OBJ_REG)
         throw std::logic_error("Only regular object nodes may be swapped out");

      htab_node_t<node_t> *nptr = *lsnode;
      bucket_t& bucket = htab[nptr->hashval % maxhash];

      if(nptr->lsnode == tmlist.end())
         throw std::logic_error("Bad time stamp list node reference");

      // check if we can swap out this node
      if(evalcb && !evalcb(nptr->node, cbarg))
         lsnode++;
      else {
         // remove the node from the bucket
         unlink_node(bucket, nptr);

         // and from the time-ordered list and advance the node iterator
         lsnode = tmlist.erase(nptr->lsnode);

         // serialized node size may have changed since it was added (e.g. city was added later)
         size_t nsize = nptr->node->s_data_size() + sizeof(node_t);
         if(memsize > nsize)
            memsize -= nsize;
         else
            memsize = 0;

         // adjust counters
         count--;
         if(--bucket.count == 0)
            emptycnt++;

         // wrap the node in a unique pointer in case swapcb throws an exception
         std::unique_ptr<htab_node_t<node_t>> uptr(nptr);

         // finally, save the node in some external storage
         swapcb(nptr->node, cbarg);
      }
   }
}

///
/// This method must only be used when incoming object nodes are guaranteed to have
/// unique keys and the caller didn't need to call `find_node` prior to this call.
/// Otherwise, `find_node`/`put_node` that take a hash value should be used, so the
/// latter is not computed multiple times.
///
template <typename node_t>
node_t *hash_table<node_t>::put_node(node_t *nptr, int64_t tstamp)
{
   if(!nptr)
      return NULL;

   return put_node(nptr->get_hash(), nptr, tstamp);
}

///
/// This method takes ownership of `node` and will delete it immediately if an
/// exception is thrown.
///
/// The new time stamp must be greater than any of the time stamps in the hash 
/// table.
///
/// @warning   This method does not verify whether the same key already exists 
/// in the bucket because it would require additional list traversal and key 
/// comparisons. The caller must call `find_node` prior to calling this method 
/// to ensure that the key is not in the hash table.
///
template <typename node_t>
node_t *hash_table<node_t>::put_node(uint64_t hashval, node_t *node, int64_t tstamp)
{
   uint64_t hashidx;
   htab_node_t<node_t> **hptr, *nptr;
   std::unique_ptr<node_t> objptr(node);

   if(!node)
      throw std::logic_error("Cannot insert a NULL node pointer");

   if(node->get_type() != OBJ_REG) {
      // ignore the time stamp because group nodes don't participate in time stamp ordering
      nptr = new htab_node_t<node_t>(objptr.release(), hashval, grplist.insert(grplist.end(), nullptr), 0);
      grplist.back() = nptr;
   }
   else {
      // enforce time stamp order for new regular nodes
      if(!tmlist.empty() && tmlist.back()->tstamp > tstamp)
         throw std::logic_error("Nodes must be linserted in the ascending time stamp order");

      //
      // Insert a new list node with a NULL value at the end of the list and allocate 
      // a new hash table node with the object node and the list iterator pointing to 
      // the list node we just inserted.
      //
      nptr = new htab_node_t<node_t>(objptr.release(), hashval, tmlist.insert(tmlist.end(), nullptr), tstamp);

      // set the list node to point back to the hash table node
      tmlist.back() = nptr;
   }

   // insert the new hash table node into its bucket
   hashidx = hashval % maxhash;

   if(!htab[hashidx].count)
      emptycnt--;

   hptr = &htab[hashidx].head;

   if(*hptr) {
      nptr->next = *hptr;
      (*hptr)->prev = nptr;
   }
   *hptr = nptr;

   // update sizes and counts
   memsize += (nptr->node->s_data_size() + sizeof(node_t));

   htab[hashidx].count++;
   count++;

   return nptr->node;
}

template <typename node_t>
const node_t *hash_table<node_t>::find_node(const string_t& key, nodetype_t type) const
{
   uint64_t hashval;
   htab_node_t<node_t> *nptr;

   hashval = hash_ex(0, key);

   for(nptr = htab[hashval % maxhash].head; nptr; nptr = nptr->next) {
      if(nptr->node->get_type() == type) {
         if(nptr->node->match_key(key))
            return nptr->node;
      }
   }

   return nullptr;
}

///
/// @warning   `unlink_node` may be used to move nodes within the bucket and
///            it does not adjust bucket and hash table counts. The caller
///            must do so if the node is being completely removed.
///
template <typename node_t>
void hash_table<node_t>::unlink_node(bucket_t& bucket, htab_node_t<node_t> *nptr) const
{
   if(nptr->prev)
      nptr->prev->next = nptr->next;
   else
      bucket.head = nptr->next;

   if(nptr->next)
      nptr->next->prev = nptr->prev;

   nptr->next = nptr->prev = nullptr;
}

template <typename node_t>
void hash_table<node_t>::move_to_front(bucket_t& bucket, htab_node_t<node_t> *nptr) const
{
   unlink_node(bucket, nptr);

   if(bucket.head)
      bucket.head->prev = nptr;

   nptr->next = bucket.head;

   bucket.head = nptr;
}

template <typename node_t>
template <typename ... param_t>
node_t *hash_table<node_t>::find_node_ex(uint64_t hashval, nodetype_t type, int64_t tstamp, bool (node_t::*match_key)(param_t ... args) const, param_t ... arg)
{
   bucket_t& bucket = htab[hashval % maxhash];
   
   // enforce time stamp order for pre-insert look-ups 
   if(type == OBJ_REG && !tmlist.empty() && tmlist.back()->tstamp > tstamp)
      throw std::logic_error("Nodes must be looked up in the ascending time stamp order when inserting");

   u_int nodeidx = 0;

   for(htab_node_t<node_t> *nptr = bucket.head; nptr != nullptr; nptr = nptr->next, nodeidx++) {
      if(nptr->node->get_type() == type) {
         if((nptr->node->*match_key)(arg ...)) {
            // if the node is further than 4 nodes from the head, move it to the front
            if(nodeidx > 4)
               move_to_front(bucket, nptr);

            // if it's a regular object, move the node to the end of the time stamp list
            if(type == OBJ_REG) {
               tmlist.erase(nptr->lsnode);
               nptr->tstamp = tstamp;
               nptr->lsnode = tmlist.insert(tmlist.end(), nptr);
            }

            return nptr->node;
         }
      }
   }

   return nullptr;
}

template <typename node_t>
node_t *hash_table<node_t>::find_node(uint64_t hashval, const string_t& key, nodetype_t type, int64_t tstamp)
{
   //
   // GCC 6.4.1 for some reason deduces node_t in node_t::match_key not as the actual
   // node_t type, such as storable_t<hnode_t>, but rather as one of the base classes, 
   // such as base_node<hnode_t>, when the member function pointer expression appears
   // directly in the find_node_ex call below. If the member function pointer is defined
   // as a variable, it compiles just fine.
   //
   bool (node_t::*match_key)(const string_t&) const = &node_t::match_key;

   return find_node_ex<const string_t&>(hashval, type, tstamp, match_key, key);
}

template <typename node_t>
node_t *hash_table<node_t>::find_node(uint64_t hashval, const typename node_t::param_block *params, nodetype_t type, int64_t tstamp)
{
   // see find_node above for details
   bool (node_t::*match_key_ex)(const typename node_t::param_block*) const = &node_t::match_key_ex;

   return find_node_ex<const typename node_t::param_block*>(hashval, type, tstamp, match_key_ex, params);
}

template <typename node_t>
void hash_table<node_t>::clear(void)
{
   // clear group nodes and ignore all counts
   while(!grplist.empty()) {
      unlink_node(htab[grplist.front()->hashval % maxhash], grplist.front());
      delete grplist.front();
      grplist.erase(grplist.begin());
   }

   // clear regular nodes and ignore all counts
   while(!tmlist.empty()) {
      unlink_node(htab[tmlist.front()->hashval % maxhash], tmlist.front());
      delete tmlist.front();
      tmlist.erase(tmlist.begin());
   }

   // now adjust all counts
   count = 0;
   emptycnt = maxhash;
   memsize = 0;
}

template <typename node_t>
uint64_t hash_table<node_t>::load_array(const typename inner_node<node_t>::type *array[]) const
{
   htab_node_t<node_t> *nptr;
   u_int hashidx, arridx;

   /* load the array */
   for(hashidx = 0, arridx = 0; hashidx < maxhash; hashidx++) {
      nptr = htab[hashidx].head;
      while(nptr) {
         if(load_array_check(nptr->node))
            array[arridx++] = nptr->node;
         nptr = nptr->next;
      }
   }

   return arridx;
}

template <typename node_t>
typename hash_table<node_t>::tm_range_t hash_table<node_t>::tm_range(void) const
{
   if(tmlist.empty())
      return {0, 0};
      
   return {tmlist.front()->tstamp, tmlist.back()->tstamp};
}
