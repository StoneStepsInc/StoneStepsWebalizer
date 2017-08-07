/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   linklist.cpp
*/
#include "pch.h"

#include <cstring>
#include <cctype>

#include "lang.h"
#include "linklist.h"

/* internal function prototypes */

static gnode_t *new_gnode(const char *, size_t, const char *, size_t);  /* new group list node */

//
// base_list
//

template <typename node_t>
const string_t *base_list<node_t>::isinlist(const string_t& str, bool nocase) const
{
   const node_t *lptr;
   return ((lptr = find_node_ex(str.c_str(), str.length(), true, nocase)) != NULL) ? &lptr->string : NULL;
}

template <typename node_t>
const string_t *base_list<node_t>::isinlistex(const char *str, size_t slen, bool substr, bool nocase) const
{
   const node_t *lptr;
   return ((lptr = find_node_ex(str, slen, substr, nocase)) != NULL) ? &lptr->string : NULL;
}

template <typename node_t>
const node_t *base_list<node_t>::find_node_ex(const char *str, size_t slen, bool substr, bool nocase) const
{
   const node_t *lptr;

   if(str == NULL || *str == 0 || slen == 0 || list_t<node_t>::isempty())
      return NULL;

   for(lptr = list_t<node_t>::head; lptr != NULL; lptr=lptr->next) {
      if(!lptr->string)
         continue; 

      if(isinstrex(str, lptr->string, slen, lptr->string.length(), substr, &lptr->delta_table, nocase)) 
         return lptr;
   }

   return NULL;
}

template <typename node_t>
node_t *base_list<node_t>::find_node(const string_t& str, typename list_t<node_t>::iterator& iter, bool next, bool nocase)
{
   node_t *lptr;

   if(str.isempty())
      return NULL;

   while((lptr = iter.next()) != NULL) {
      if(isinstrex(str, lptr->key(), str.length(), lptr->key().length(), true, &lptr->delta_table, nocase)) 
         return lptr;

      if(next)
         break;
   }

   return NULL;
}

template <typename node_t>
const node_t *base_list<node_t>::find_node(const string_t& str, typename list_t<node_t>::const_iterator& iter, bool next, bool nocase)
{
   const node_t *lptr;

   if(str.isempty())
      return NULL;

   while((lptr = iter.next()) != NULL) {
      if(isinstrex(str, lptr->key(), str.length(), lptr->key().length(), true, &lptr->delta_table, nocase)) 
         return lptr;

      if(next)
         break;
   }

   return NULL;
}

/*********************************************/
/* ADD_NLIST - add item to FIFO linked list  */
/*********************************************/

bool nlist::add_nlist(const char *str)
{
   nnode_t *nptr;

   if(!str || !*str)
      return false;

   if((nptr = new nnode_t(str)) == NULL)
      return false;

   // insert a single asterisk at the head (wildcard)
   if(str[0] == '*' && str[1] == 0) 
      add_head(nptr);
   else 
      add_tail(nptr);

   return true;
}

/*********************************************/
/* ISINLIST - Test if string is in list      */
/*********************************************/

bool nlist::iswildcard(void) const
{
   return (head && head->string[0] == '*' && head->string[1] == 0) ? true : false;
}

/*********************************************/
/* NEW_GLIST - create new linked list node   */ 
/*********************************************/

static gnode_t *new_gnode(const char *name, size_t nlen, const char *value, size_t vlen, const char *qualifier, size_t qlen)
{
   gnode_t *newptr;

   if(value == NULL)
      return NULL;

   if(vlen == 0)
      vlen = strlen(value);

   if(name == NULL || *name == 0) {
      nlen = vlen;
      name = value;
   }
   else {
      if(nlen == 0)
         nlen = strlen(name);
   }

   if(qualifier && qlen == 0)
      qlen = strlen(qualifier);

   if((newptr = new gnode_t(value, vlen)) != NULL) {
       newptr->name.assign(name, nlen);

       if(qlen)
         newptr->qualifier.assign(qualifier, qlen);

       newptr->noname = (name == value) ? true : false;
       newptr->next=NULL;
   }

   return newptr;
}

/*********************************************/
/* ADD_GLIST - add item to FIFO linked list  */
/*********************************************/

bool glist::add_glist(const char *str, bool srcheng)
{
   gnode_t *newptr;
   const char *cp1, *cp2, *cp3 = NULL;
   size_t nlen, vlen, qlen;

   if(str == NULL)
      return 0;

   //
   // keyword     value    [<tab>  name[=search quaifier]]
   //             ^                ^     ^
   //             cp1              cp2   cp3
   //

   // find the end of the value
   cp2 = cp1 = str;
   if(enable_phrase_values)
      while(*cp2 && *cp2 != '\t') cp2++;
   else
      while(*cp2 && !string_t::isspace(*cp2)) cp2++;

   // trim trailing spaces
   vlen = cp2 - cp1;
   while(string_t::isspace(cp1[vlen-1])) vlen--;

   // find the first character of the name
   while(*cp2 && (string_t::isspace(*cp2))) cp2++;

   // if a search engine entry, locate the entry qualifier
   if(srcheng && (cp3 = strchr(cp2, '=')) != NULL) {
      nlen = ++cp3 - cp2;
      qlen = (*cp3) ? strlen(cp3) : 0;
   }
   else {
      nlen = (*cp2) ? strlen(cp2) : 0;
      qlen = 0;
   }

   if((newptr = new_gnode(cp2, nlen, cp1, vlen, cp3, qlen)) != NULL) 
      add_tail(newptr);

   return newptr == NULL;
}

/*********************************************/
/* ISINGLIST - Test if string is in list     */
/*********************************************/

const string_t *glist::isinglist(const string_t& str) const
{
   glist::const_iterator iter = begin();
   return isinglist(str, iter, false);
}

const string_t *glist::isinglist(const string_t& str, glist::const_iterator& iter, bool next) const
{
   const gnode_t *lptr;
   return ((lptr = find_node(str, iter, next)) != NULL) ? &lptr->name : NULL;
}

const string_t *glist::isinglist(const char *str, size_t slen, bool substr) const
{
   const gnode_t *lptr;
   return ((lptr = find_node_ex(str, slen, substr)) != NULL) ? &lptr->name : NULL;
}

void glist::for_each(const char *str, void (*cb)(const char *, void*), void *ptr, bool nocase)
{
   size_t slen;
   iterator iter = begin();
   gnode_t *nptr;

   slen = (str && *str) ? strlen(str) : 0;

   while((nptr = iter.next()) != NULL) {
      if(nptr->noname || (slen && isinstrex(str, nptr->name, slen, nptr->name.length(), false, &nptr->delta_table, nocase)))
         cb(nptr->string, ptr);
   }
}

#include "linklist_tmpl.cpp"

//
// Instantiate linked list templates (see comments at the end of hashtab.cpp)
//
template struct list_node_t<nnode_t>;
template struct list_node_t<gnode_t>;
template struct base_list_node_t<nnode_t>;
template struct base_list_node_t<gnode_t>;

template class list_t<nnode_t>;
template class list_t<gnode_t>;

template class base_list<nnode_t>;
template class base_list<gnode_t>;

