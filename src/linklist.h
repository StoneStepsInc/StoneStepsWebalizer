/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   linklist.h
*/

#ifndef LINKLIST_H
#define LINKLIST_H

#include "util_string.h"
#include "tstring.h"
#include "types.h"

#include <list>

///
/// @brief  A list node with a string pattern for matching beginning or ending of a
///         string or a substring of a string
///
/// The string pattern may take of the three forms:
///
///   1. abc*        - matches abc at the beginning of a string
///   2. *abc        - matches abc at the end of a string
///   3. abc         - matches abc anywhere in a string as a substring or exactly abc as a word
///
/// For a substring pattern a delta table is computed on initialization for fatser
/// searching.
///
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
      base_list_node_t(const char *str) {string.assign(str); init_delta_table();}
      base_list_node_t(const char *str, size_t slen) {string.assign(str, slen); init_delta_table();}
      base_list_node_t(const string_t& str) {string = str; init_delta_table();}

      virtual ~base_list_node_t(void) {}

      const string_t& key(void) const {return string;}

      void set_key(const char *str, size_t slen) {string.assign(str, slen); init_delta_table();}
};

///
/// @brief  An ordered linked list of nodes containing string patterns
///
/// The parameter `substr` changes how non-asterisk patterns are matched. If it is `true`,
/// the pattern `xyz` will match input `abcxyz123`. If `substr` is `false`, `abc` will only
/// match input `abc`.
///
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

      /// Removes all elements from the list.
      void clear(void) {list.clear();}

      /// scan list values for str as substring and return the matching one, if found, or nullptr otherwise
      const string_t *isinlist(const string_t& str, bool nocase = false) const;

      /// scan list values for str and return the matching one, if found, or nullptr otherwise
      const string_t *isinlistex(const char *str, size_t slen, bool substr, bool nocase = false) const;

      /// scan the list for str and return the node with a matching pattern or nullptr otherwise
      const node_t *find_node_ex(const char *str, size_t slen, bool substr, bool nocase = false) const;

      /// scan the list for str as substring starting from the specified position and return the node with a matching pattern or nullptr otherwise
      const node_t *find_node(const string_t& str, typename std::list<node_t>::const_iterator& iter, bool next = false, bool nocase = false) const;
};

///
/// @brief  A list node for IGNORE and HIDE list items
///
typedef base_list_node_t nnode_t;

///
/// @brief  An ordered linked list for ignore, include and hide list items
///
/// Order of items in `nlist` is important. For example, if asingle asterisk is at the
/// front of the list, all other items will be ignored in searches because the asterisk
/// always matches any input. 
///
class nlist : public base_list<nnode_t> {
   public:
      nlist(void) : base_list<nnode_t>() {}

      ~nlist(void) {}

      /// adds a string pattern to the list
      bool add_nlist(const char *str);

      /// returns true if the first entry in the list is a single asterisk character, false otherwise
      bool iswildcard(void) const;
};

///
/// @brief  An ordered list node for GROUP items
///
/// A group node adds a name to the string pattern in the base node. Multiple string patterns
/// may have the same name, such as `*.jpg` and `*.png` may map to the same name `Images`.
///
/// A name may optionally be followed by a qualifier separated from the name by the equal sign
/// character, such as `as_q=All Words`, in which case `as_q=` will be stored in `name` and `All
/// Words` in `qualifier`.
///
struct gnode_t : public base_list_node_t {
   string_t name;                            // name
   string_t qualifier;                       // name qualifier
   bool     noname;

   public:
      gnode_t(const char *name, size_t nlen, const char *value, size_t vlen, const char *qualifier, size_t qlen);
      gnode_t(const char *str, size_t slen) : base_list_node_t(str, slen) {noname = true;}
      gnode_t(const string_t& str) : base_list_node_t(str) {noname = true;}
};

///
/// @brief  An ordered linked list for grouping list items
///
/// In addition or the list order guidelines for `nlist`, items of a `glist` instance that
/// contain different variations of the same pattern must be ordered manually. For example,
/// multiple search engine search qualifirs must follow one another like this:
/// ```
///    www.google. <tab>   q=
///    www.google. <tab>   as_q=All Words
///    www.bing.   <tab>   q=
/// ```
/// This order greatly improves performance of search in large lists and cannot be enforced 
/// automatically because of a few reasons, such as that some generic sort would break the 
/// order of other items in the list and that patterns may have different characters and still 
/// match the same input (e.g. `*.google.com` and `google.com`).
///
class glist : public base_list<gnode_t> {
   private:
      bool enable_phrase_values;
      bool has_names;

   public:
      glist(void) : base_list<gnode_t>(), has_names(false), enable_phrase_values(false) {}

      ~glist(void) {}

      /// parse a group configuration entry formatted as `pattern [<tab> name[=search quaifier]]` and add a group list item
      bool add_glist(const char *str, bool srch = false);

      /// scan glist values case-sensitively for str as substring and return group name if found or nullptr otherwise
      const string_t *isinglist(const string_t& str) const;

      /// scan glist values case-sensitively for str as substring and return group name if found or nullptr otherwise
      const string_t *isinglist(const char *str, size_t slen, bool substr) const;

      /// calls the callback function for each node with no name or a name (not pattern) matching str
      void for_each(const char *str, void (*cb)(const char *, void*), void *ptr = nullptr, bool nocase = false, bool delmatch = false);

      /// enable or disable the phrase values mode, which allows values with spaces
      void set_enable_phrase_values(bool enable) {enable_phrase_values = enable;}

      /// returns true if at least one entry has a name associated with the value, false otherwise
      bool get_has_names(void) const {return has_names;}
};

#endif  // LINKLIST_H 
