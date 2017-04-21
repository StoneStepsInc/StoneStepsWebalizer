/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information   

   linklist_tmpl.cpp
*/

#include "linklist.h"

//
// Generic list implementation
//

template <typename node_t, void (*dispose)(node_t*)>
void list_t<node_t, dispose>::clear(void)
{
   node_t *cptr, *nptr;

   cptr = head;
   while(cptr != NULL) {
      nptr=cptr->next;
      if(dispose)
         dispose(cptr);
      cptr=nptr;
   }
   head = tail = NULL;
   count = 0;
}

template <typename node_t, void (*dispose)(node_t*)>
void list_t<node_t, dispose>::add_head(node_t *newptr)
{
   if(!newptr)
      return;

   if(head == NULL) {
      newptr->next = NULL;
      head = tail = newptr;
   }
   else {
      newptr->next = head;
      head = newptr;
   }
   count++;
}

template <typename node_t, void (*dispose)(node_t*)>
void list_t<node_t, dispose>::add_tail(node_t *newptr)
{
   if(!newptr)
      return;
      
   if(head == NULL) {
      newptr->next = NULL;
      head = tail = newptr;
   }
   else {
      tail->next = newptr;
      tail = newptr;
   }
   count++;
}

template <typename node_t, void (*dispose)(node_t*)>
node_t *list_t<node_t, dispose>::remove_head(void)
{
   node_t *nptr = NULL;
   
   if(head) {
      nptr = head;
      head = head->next;
      
      if(!head)
         tail = NULL;
         
      count--;
   }
   
   return nptr;
}

