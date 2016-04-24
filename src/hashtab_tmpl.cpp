/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   hashtab_tmpl.cpp
*/
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util.h"
#include "hashtab.h"

//
//
//
#define NO_SWAP_COUNT   ((u_int) 8)

//
//
//

template <typename node_t, typename key_t>
hash_table<node_t, key_t>::hash_table(size_t maxhash, eval_cb_t evalcb, swap_cb_t swapcb, void *cbarg) : maxhash(maxhash), evalcb(evalcb), swapcb(swapcb), cbarg(cbarg)
{
   count = 0;
   emptycnt = maxhash;
   swap = false;
   cleared = false;
   htab = new bucket_t[maxhash];
}

template <typename node_t, typename key_t>
hash_table<node_t, key_t>::~hash_table(void)
{
   delete [] htab;
}

template <typename node_t, typename key_t>
void hash_table<node_t, key_t>::set_swap_out_cb(eval_cb_t eval, swap_cb_t swap, void *arg)
{
   evalcb = eval;
   swapcb = swap;
   cbarg = arg;
}

template <typename node_t, typename key_t>
typename hash_table<node_t, key_t>::swap_code hash_table<node_t, key_t>::swap_out_node(bucket_t& bucket, node_t **pptr)
{
   node_t *nptr; 

   if(!pptr || !swapcb)
      return swap_failed;

   nptr = *pptr;

   // check if the node can be swapped out
   if(evalcb && !evalcb(nptr, cbarg))     
      return swap_inuse;

   // unlink the node
   *pptr = nptr->next;
   nptr->next = NULL;

   // pass a non-const node to swapcb
   if(!swapcb(nptr, cbarg)) {
      // if failed, put the node back
      nptr->next = *pptr;
      *pptr = nptr;
      return swap_failed;
   }

   // adjust counters
   count--;
   if(--bucket.count == 0)
      emptycnt++;

   // indicate that some data has been swapped out
   if(!swap)
      swap = true;

   delete nptr;

   return swap_ok;
}

template <typename node_t, typename key_t>
bool hash_table<node_t, key_t>::swap_out_bucket(bucket_t& bucket)
{
   u_int index;
   swap_code scode;
   node_t **pptr;

   for(index = 0, pptr = &bucket.head; *pptr; index++) {
      // swap the node out
      if((scode = swap_out_node(bucket, pptr)) == swap_failed)
         return false;

      // if the node is in use, assign pptr to node's next
      if(scode == swap_inuse)
         pptr = &(*pptr)->next;
   }

   return true;
}

template <typename node_t, typename key_t>
node_t *hash_table<node_t, key_t>::put_node(uint64_t hashval, node_t *nptr)
{
   uint64_t hashidx;
   node_t **hptr;

   if(nptr == NULL)
      return NULL;

   hashidx = hashval % maxhash;

   if(!htab[hashidx].count)
      emptycnt--;

   hptr = &htab[hashidx].head;

   if(*hptr) 
      nptr->next = *hptr;
   *hptr = nptr;

   htab[hashidx].count++;
   count++;

   return nptr;
}

template <typename node_t, typename key_t>
void hash_table<node_t, key_t>::pop_node(uint64_t hashval, node_t *nptr)
{
   uint64_t hashidx;
   node_t **pptr, *cptr;

   if(nptr == NULL)
      return;

   hashidx = hashval % maxhash;
   pptr = &htab[hashidx].head;
   cptr = *pptr;

   while(cptr != NULL) {
      if(cptr == nptr) {
         *pptr = cptr->next;
         if(--htab[hashidx].count == 0)
            emptycnt++;
         count--;
         return;
      }
      pptr = &cptr->next;
      cptr = cptr->next;
   }
}

template <typename node_t, typename key_t>
const node_t *hash_table<node_t, key_t>::find_node(uint64_t hashval, const key_t& key) const
{
   node_t **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->key() == key) {
         if(index > NO_SWAP_COUNT)
            move_node(nptr, prev, pptr);
         return nptr;
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t, typename key_t>
node_t *hash_table<node_t, key_t>::find_node(uint64_t hashval, const key_t& key, nodetype_t type)
{
   node_t **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(nptr->get_type() == type) {
         if(nptr->key() == key) {
            if(index > NO_SWAP_COUNT)
               move_node(nptr, prev, pptr);
            return nptr;
         }
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t, typename key_t>
node_t *hash_table<node_t, key_t>::find_node(uint64_t hashval, const void *params)
{
   node_t **pptr, *nptr, *prev = NULL;

   pptr = &htab[hashval % maxhash].head;
   nptr = *pptr;

   for(u_int index = 0; nptr != NULL; index++) {
      if(compare(nptr, params)) {
         if(index > NO_SWAP_COUNT)
            move_node(nptr, prev, pptr);
         return nptr;
      }
      prev = nptr;
      nptr = nptr->next;
   }

   return NULL;
}

template <typename node_t, typename key_t>
void hash_table<node_t, key_t>::clear(void)
{
   /* free memory used by hash table */
   node_t *nptr, *tptr;
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

   cleared = true;
   count = 0;
   emptycnt = maxhash;
}

template <typename node_t, typename key_t>
uint64_t hash_table<node_t, key_t>::load_array(const node_t *array[]) const
{
   node_t *nptr;
   u_int hashidx, arridx;

   /* load the array */
   for(hashidx = 0, arridx = 0; hashidx < maxhash; hashidx++) {
      nptr = htab[hashidx].head;
      while(nptr) {
         if(load_array_check(nptr))
            array[arridx++] = nptr;
         nptr = nptr->next;
      }
   }

   return arridx;
}

template <typename node_t, typename key_t>
uint64_t hash_table<node_t, key_t>::load_array(const node_t *array[], nodetype_t type, uint64_t& typecnt) const
{
   const node_t *nptr, **pptr;
   size_t hashidx, arridx1, arridx2, skipcnt;

   //
   // load the array, grouping nodes by type
   //
   for(hashidx = 0, arridx1 = 0, arridx2 = count; hashidx < maxhash; hashidx++) {
      nptr = htab[hashidx].head;
      while(nptr) {
         if(load_array_check(nptr)) {
            if(nptr->get_type() == type)
               array[arridx1++] = nptr;
            else
               array[--arridx2] = nptr;
         }
         nptr = nptr->next;
      }
   }

   typecnt = arridx1;

   if((skipcnt = arridx2 - arridx1) == 0)
      return count;

   //
   // fill in the empty space with nodes from the end of the array
   //
   for(pptr = &array[count-1]; arridx1 != arridx2; arridx1++, pptr--)
      array[arridx1] = *pptr;

   return count - skipcnt;
}

template <typename node_t, typename key_t>
bool hash_table<node_t, key_t>::swap_out(void)
{
   if(swapcb == NULL)
      return true;

   // swap out each bucket
   for(u_int index = 0; index < maxhash; index++) {
      if(!htab[index].count)
         continue;

      if(!swap_out_bucket(htab[index]))
         return false;
   }

   return true;
}

