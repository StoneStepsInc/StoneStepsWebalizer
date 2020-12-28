/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   queue.h
*/
#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"
#include <cstddef>

///
/// @tparam type_t   The type of individual items in the queue
///
/// @brief  A generic FIFO queue that is optimized to keep a few empty nodes to minimize
///         memory allocations
///
template <typename type_t>
class queue_t {
   private:
      static const u_int max_ecount;

   private:
      struct q_node_t {
         q_node_t    *next;
         type_t      *data;

         public:
            q_node_t(type_t *data) : data(data), next(nullptr) {}

            q_node_t *reset(type_t *_data) {next = nullptr; data = _data; return this;}
      };

   private:
      q_node_t    *head;            // first node
      q_node_t    *tail;            // last node
      u_int       count;            // node count

      q_node_t    *empty;           // empty node list
      u_int       ecount;           // empty node count

   private:
      q_node_t *new_node(type_t *data);
      void delete_node(q_node_t *node);

   public:
      queue_t(void);

      ~queue_t(void);

      inline u_int size(void) const {return count;}

      inline const type_t *top(void) const {return head ? head->data : nullptr;}

      inline type_t *top(void) {return head ? head->data : nullptr;}

      void add(type_t *data);

      type_t *remove(void);
};

#endif // QUEUE_H
