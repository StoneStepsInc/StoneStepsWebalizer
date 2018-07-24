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

//
//
//
#define NO_SWAP_COUNT   ((u_int) 8)

//
//
//

template <typename node_t>
hash_table<node_t>::hash_table(size_t maxhash, swap_cb_t swapcb, void *cbarg) : maxhash(maxhash), swapcb(swapcb), cbarg(cbarg)
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
void hash_table<node_t>::set_swap_out_cb(swap_cb_t swap, void *arg)
{
   swapcb = swap;
   cbarg = arg;
}

template <typename node_t>
typename hash_table<node_t>::swap_code hash_table<node_t>::swap_out_node(bucket_t& bucket, htab_node_t<node_t> **pptr)
{
   htab_node_t<node_t> *nptr; 

   if(!pptr || !swapcb)
      return swap_failed;

   nptr = *pptr;

   // unlink the node
   *pptr = nptr->next;
   nptr->next = NULL;

   // pass a non-const node to swapcb
   if(!swapcb(nptr->node, cbarg)) {
      // if failed, put the node back
      nptr->next = *pptr;
      *pptr = nptr;
      return swap_failed;
   }

   // adjust counters
   count--;
   if(--bucket.count == 0)
      emptycnt++;

   delete nptr;

   return swap_ok;
}

template <typename node_t>
node_t *hash_table<node_t>::put_node(node_t *nptr)
{
   if(!nptr)
      return NULL;

   return put_node(nptr->get_hash(), nptr);
}

template <typename node_t>
node_t *hash_table<node_t>::put_node(uint64_t hashval, node_t *node)
{
   uint64_t hashidx;
   htab_node_t<node_t> **hptr, *nptr;

   if(node == NULL)
      return NULL;

   nptr = new htab_node_t<node_t>(node);

   hashidx = hashval % maxhash;

   if(!htab[hashidx].count)
      emptycnt--;

   hptr = &htab[hashidx].head;

   if(*hptr) 
      nptr->next = *hptr;
   *hptr = nptr;

   htab[hashidx].count++;
   count++;

   return nptr->node;
}

template <typename node_t>
const node_t *hash_table<node_t>::find_node(uint64_t hashval, const string_t& key) const
{
   htab_node_t<node_t> **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->node->match_key(key))
         return nptr->node;

      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t>
node_t *hash_table<node_t>::find_node(uint64_t hashval, const string_t& key)
{
   htab_node_t<node_t> **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->node->match_key(key)) {
         if(index > NO_SWAP_COUNT)
            move_node(nptr, prev, pptr);
         return nptr->node;
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t>
node_t *hash_table<node_t>::find_node(uint64_t hashval, const string_t& key, nodetype_t type)
{
   htab_node_t<node_t> **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->node->get_type() == type) {
         if(nptr->node->match_key(key)) {
            if(index > NO_SWAP_COUNT)
               move_node(nptr, prev, pptr);
            return nptr->node;
         }
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t>
node_t *hash_table<node_t>::find_node(uint64_t hashval, const typename node_t::param_block *params)
{
   htab_node_t<node_t> **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->node->match_key_ex(params)) {
         if(index > NO_SWAP_COUNT)
            move_node(nptr, prev, pptr);
         return nptr->node;
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t>
void hash_table<node_t>::clear(void)
{
   /* free memory used by hash table */
   htab_node_t<node_t> *nptr, *tptr;
   uint64_t index;

   for(index = 0; index < maxhash; index++) {
      if((nptr = htab[index].head) == NULL)
         continue;

      while(nptr != NULL) {
         tptr = nptr;
         nptr = nptr->next;
         delete tptr;
      }

      htab[index].head = NULL;
      htab[index].count = 0;
   }

   count = 0;
   emptycnt = maxhash;
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
