/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   queue_tmpl.cpp
*/

#include "queue.h"

template <typename type_t> const u_int queue_t<type_t>::max_ecount = 100;

template <typename type_t>
queue_t<type_t>::queue_t(void)
{
   head = tail = empty = nullptr;
   count = ecount = 0;
}

template <typename type_t>
queue_t<type_t>::~queue_t(void)
{
   q_node_t *node;

   while(empty) {
      node = empty;
      empty = empty->next;
      delete node;
   }
}

template <typename type_t>
typename queue_t<type_t>::q_node_t *queue_t<type_t>::new_node(type_t *data)
{
   q_node_t *node;

   if(!empty)
      return new q_node_t(data);

   node = empty;
   empty = empty->next;
   ecount--;

   return node->reset(data);
}

template <typename type_t>
void queue_t<type_t>::delete_node(typename queue_t<type_t>::q_node_t *node)
{
   if(!node)
      return;

   // if there are too many empty nodes, just delete the node
   if(ecount >= max_ecount) {
      delete(node);
      return;
   }   

   // otherwise, save the node to reduce allocations
   node->data = nullptr;
   node->next = empty;
   empty = node;
   ecount++;
}

template <typename type_t>
void queue_t<type_t>::add(type_t *data)
{
   if(!data)
      return;

   if(!tail)
      head = tail = new_node(data);
   else {
      tail->next = new_node(data);
      tail = tail->next;
   }
   count++;
}

template <typename type_t>
type_t *queue_t<type_t>::remove(void)
{
   q_node_t *node;
   type_t *data;

   if(!head)
      return nullptr;

   node = head;

   if(!--count)
      head = tail = nullptr;
   else
      head = head->next;

   data = node->data;
   delete_node(node);

   return data;
}

