/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   linklist.h
*/

#ifndef _LINKLIST_H
#define _LINKLIST_H

#include "util.h"
#include "tstring.h"
#include "types.h"

//
// link list node base
//
template <typename node_t>
struct list_node_t {
   node_t *next; 

   public:
      list_node_t(void) : next(NULL) {}
      list_node_t(node_t *next) : next(next) {}
      
      static void delete_node(node_t *node) {delete node;}      
};

//
// generic linked list object
//
template <typename node_t, void (*dispose)(node_t*) = &list_node_t<node_t>::delete_node>
class list_t {
   protected:
      node_t *head;
      node_t *tail;

      u_int  count;

   public:
      class iterator {
         friend class list_t<node_t>;

         private:
            node_t   *head;
            node_t   *nptr;

         protected:
            iterator(node_t *hptr) {head = hptr; nptr = NULL;}

         public:
            iterator(void) {head = NULL; nptr = NULL;}

            iterator(const iterator& iter) {head = iter.head; nptr = iter.nptr;}
            
            iterator& operator = (const iterator& iter) {head = iter.head; nptr = iter.nptr; return *this;}

            node_t *operator * (void) {return item();}

            node_t *operator ++ (void) {return next();}

            node_t *item(void) {return nptr;}

            node_t *next(void)
            {
               if(head) {
                  nptr = head; head = NULL;
                  return nptr;
               }

               if(nptr)
                  nptr = nptr->next;

               return nptr;
            }
      };

      class const_iterator : public iterator {
         friend class list_t<node_t>;

         private:
            const_iterator(node_t *hptr) : iterator(hptr) {}

         public:
            const_iterator(void) : iterator() {}

            const_iterator(const const_iterator& iter) : iterator(iter) {}

            const_iterator(const iterator& iter) : iterator(iter) {}

            const_iterator& operator = (const const_iterator& iter) {iterator::operator = (iter); return *this;}

            const node_t *operator * (void) {return iterator::item();}

            const node_t *operator ++ (void) {return iterator::next();}

            const node_t *item(void) {return iterator::item();}

            const node_t *next(void) {return iterator::next();}
      };
      
   public:
      list_t(void) {head = tail = NULL; count = 0;}

      ~list_t(void) {clear();}

      iterator begin(void) {return iterator(head);}

      const_iterator begin(void) const {return const_iterator(head);}

      void clear(void);

      void add_head(node_t *newptr);

      void add_tail(node_t *newptr);

      node_t *remove_head(void);

      const node_t *get_head(void) const {return head;}

      node_t *get_head(void) {return head;}

      const node_t *get_tail(void) const {return tail;}

      node_t *get_tail(void) {return tail;}

      bool isempty(void) const {return (!head) ? true : false;}

      u_int size(void) const {return count;}
};

// base string list node
template <typename node_t>
struct base_list_node_t : public list_node_t<node_t> {
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
};

//
// base_list
//
template <typename node_t>
class base_list : public list_t<node_t> {
   public:
      base_list(void) : list_t<node_t>() {}
      
      ~base_list(void) {}

      const string_t *isinlist(const string_t& str) const;

      const string_t *isinlistex(const char *str, size_t slen, bool substr) const;

      const node_t *find_node_ex(const char *str, size_t slen, bool substr) const;

      static node_t *find_node(const string_t& str, typename list_t<node_t>::iterator& iter, bool next = false);

      static const node_t *find_node(const string_t& str, typename list_t<node_t>::const_iterator& iter, bool next = false);
};

// list struct for IGNORE and HIDE items
struct nnode_t : public base_list_node_t<nnode_t> {
   private:
      nnode_t(void) : base_list_node_t<nnode_t>() {}

   public:
      nnode_t(const char *str) : base_list_node_t<nnode_t>(str) {}
      nnode_t(const string_t& str) : base_list_node_t<nnode_t>(str) {}
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
struct gnode_t : public base_list_node_t<gnode_t> {
   string_t name;                            // name
   string_t qualifier;                       // search engine string qualifier
   bool     noname;

   private:
      gnode_t(void) {noname = true;}

   public:
      gnode_t(const char *str, size_t slen) : base_list_node_t<gnode_t>(str, slen) {noname = true;}
      gnode_t(const string_t& str) : base_list_node_t<gnode_t>(str) {noname = true;}
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

      const string_t *isinglist(const string_t& str, const_iterator& iter, bool next = false) const;

      const string_t *isinglist(const char *str, size_t slen, bool substr) const;

      void for_each(const char *str, void (*cb)(const char *, void*), void *ptr = NULL);

      void set_enable_phrase_values(bool enable) {enable_phrase_values = enable;}
};

#endif  // _LINKLIST_H 
