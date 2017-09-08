/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   linklist.h
*/

#ifndef _LINKLIST_H
#define _LINKLIST_H

#include "util_string.h"
#include "tstring.h"
#include "types.h"

#include <list>

// base string list node
struct base_list_node_t {
   string_t          string;
   bmh_delta_table   delta_table;

   private:
      void init_delta_table(void)
      {
         if(!string.isempty() && string[0] != '*' && string[string.length()-1] != '*')
            delta_table.reset(string);
      }

   public:
      base_list_node_t(void) {}
      base_list_node_t(const char *str) {string.assign(str); init_delta_table();}
      base_list_node_t(const char *str, size_t slen) {string.assign(str, slen); init_delta_table();}
      base_list_node_t(const string_t& str) {string = str; init_delta_table();}

      virtual ~base_list_node_t(void) {}

      const string_t& key(void) const {return string;}

      void set_key(const char *str, size_t slen) {string.assign(str, slen); init_delta_table();}
};

//
// base_list
//
template <typename node_t>
class base_list {
   public: 
      typedef typename std::list<node_t>::iterator iterator;
      typedef typename std::list<node_t>::const_iterator const_iterator;

   protected:
      std::list<node_t> list;

   public:
      base_list(void) {}
      
      ~base_list(void) {}

      typename std::list<node_t>::iterator begin(void) {return list.begin();}
      typename std::list<node_t>::const_iterator begin(void) const {return list.begin();}

      typename std::list<node_t>::iterator end(void) {return list.end();}
      typename std::list<node_t>::const_iterator end(void) const {return list.end();}

      size_t size(void) const {return list.size();}

      bool isempty(void) const {return list.empty();}

      const string_t *isinlist(const string_t& str, bool nocase = false) const;

      const string_t *isinlistex(const char *str, size_t slen, bool substr, bool nocase = false) const;

      const node_t *find_node_ex(const char *str, size_t slen, bool substr, bool nocase = false) const;

      const node_t *find_node(const string_t& str, typename std::list<node_t>::const_iterator& iter, bool next = false, bool nocase = false) const;
};

// list struct for IGNORE and HIDE items
struct nnode_t : public base_list_node_t {
   private:
      nnode_t(void) : base_list_node_t() {}

   public:
      nnode_t(const char *str) : base_list_node_t(str) {}
      nnode_t(const string_t& str) : base_list_node_t(str) {}
};

//
// ignore/include/hide/etc list
//
class nlist : public base_list<nnode_t> {
   public:
      nlist(void) : base_list<nnode_t>() {}

      ~nlist(void) {}

      bool add_nlist(const char *str);

      bool iswildcard(void) const;
};

// list struct for GROUP items
struct gnode_t : public base_list_node_t {
   string_t name;                            // name
   string_t qualifier;                       // search engine string qualifier
   bool     noname;

   private:
      gnode_t(void) {noname = true;}

   public:
      gnode_t(const char *name, size_t nlen, const char *value, size_t vlen, const char *qualifier, size_t qlen);
      gnode_t(const char *str, size_t slen) : base_list_node_t(str, slen) {noname = true;}
      gnode_t(const string_t& str) : base_list_node_t(str) {noname = true;}
};

//
// group list_t
//
class glist : public base_list<gnode_t> {
   private:
      bool enable_phrase_values;

   public:
      glist(void) : base_list<gnode_t>() {enable_phrase_values = false;}

      ~glist(void) {}

      bool add_glist(const char *str, bool srch = false);      // add group list item

      const string_t *isinglist(const string_t& str) const;    // scan glist for str

      const string_t *isinglist(const char *str, size_t slen, bool substr) const;

      /// calls the callback function for each node with no name or a name (not value) matching str
      void for_each(const char *str, void (*cb)(const char *, void*), void *ptr = NULL, bool nocase = false);

      void set_enable_phrase_values(bool enable) {enable_phrase_values = enable;}
};

#endif  // _LINKLIST_H 
