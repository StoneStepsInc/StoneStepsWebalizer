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
   if(str == NULL || *str == 0 || slen == 0 || list.empty())
      return NULL;

   for(typename std::list<node_t>::const_iterator lptr = list.begin(); lptr != list.end(); lptr++) {
      if(lptr->string.isempty())
         continue; 

      if(isinstrex(str, lptr->string, slen, lptr->string.length(), substr, &lptr->delta_table, nocase)) 
         return &(*lptr);
   }

   return NULL;
}

template <typename node_t>
const node_t *base_list<node_t>::find_node(const string_t& str, typename std::list<node_t>::const_iterator& lptr, bool next, bool nocase) const
{
   if(str.isempty())
      return NULL;

   while(lptr != list.end()) {
      if(isinstrex(str, lptr->key(), str.length(), lptr->key().length(), true, &lptr->delta_table, nocase))
         return &*lptr++;

      lptr++;

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
   if(!str || !*str)
      return false;

   // insert a single asterisk at the head (wildcard)
   if(str[0] == '*' && str[1] == 0) 
      list.emplace_front(str);
   else 
      list.emplace_back(str);

   return true;
}

bool nlist::iswildcard(void) const
{
   return (!list.empty() && list.front().string[0] == '*' && list.front().string[1] == 0);
}

gnode_t::gnode_t(const char *name, size_t nlen, const char *value, size_t vlen, const char *qualifier, size_t qlen)
{
   if(value == NULL)
      throw std::invalid_argument("Group key value cannot be NULL");

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

   set_key(value, vlen);

   this->name.assign(name, nlen);

   if(qlen)
      this->qualifier.assign(qualifier, qlen);

   this->noname = name == value;
}

bool glist::add_glist(const char *str, bool srcheng)
{
   const char *cp1, *cp2, *cp3 = NULL;
   size_t nlen, vlen, qlen;

   if(str == NULL)
      return false;

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

   if(!has_names && nlen)
      has_names = true;

   list.emplace_back(cp2, nlen, cp1, vlen, cp3, qlen);

   return true;
}

/*********************************************/
/* ISINGLIST - Test if string is in list     */
/*********************************************/

const string_t *glist::isinglist(const string_t& str) const
{
   const gnode_t *lptr;
   glist::const_iterator iter = list.begin();
   return ((lptr = find_node(str, iter, false)) != NULL) ? &lptr->name : NULL;
}

const string_t *glist::isinglist(const char *str, size_t slen, bool substr) const
{
   const gnode_t *lptr;
   return ((lptr = find_node_ex(str, slen, substr)) != NULL) ? &lptr->name : NULL;
}

void glist::for_each(const char *str, void (*cb)(const char *, void*), void *ptr, bool nocase)
{
   size_t slen;

   slen = (str && *str) ? strlen(str) : 0;

   for(std::list<gnode_t>::iterator nptr = list.begin(); nptr != list.end(); nptr++) {
      if(nptr->noname || (slen && isinstrex(str, nptr->name, slen, nptr->name.length(), false, NULL, nocase)))
         cb(nptr->string, ptr);
   }
}

//
// Instantiate linked list templates (see comments at the end of hashtab.cpp)
//
template class base_list<nnode_t>;
template class base_list<gnode_t>;

