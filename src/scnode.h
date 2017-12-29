/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   scnode.h
*/
#ifndef SCNODE_H
#define SCNODE_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"
#include "storable.h"

#include <vector>

///
/// @struct scnode_t
///
/// @brief  HTTP status code node
///
/// Prior to v2 this node was missing the version value in serialized data and the 
/// only way to identify that version is by the size of serialized data. Starting 
/// with v2 serialized node data will be one byte larger than v1 because of `v2pad`,
/// which allows us to get back to the versioned serialized node layout.
///
struct scnode_t : public keynode_t<u_int>, public datanode_t<scnode_t> {
   uint64_t       count;            ///< Number of requests with this status code
   u_char         v2pad;            ///< v2 serialized node padding (see class definition)

   public:
      typedef void (*s_unpack_cb_t)(scnode_t& scnode, void *arg);

   public:
      scnode_t(u_int code) : keynode_t<u_int>(code), v2pad(0) {count = 0;}

      scnode_t(const scnode_t& node) : keynode_t<u_int>(node), v2pad(node.v2pad) {count = node.count;}
      
      scnode_t& operator = (const scnode_t& node) {keynode_t<u_int>::operator = (node); count = node.count; v2pad = node.v2pad; return *this;}

      u_int get_scode(void) const {return nodeid;}

      //
      // serialization
      //
      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

      static size_t s_data_size(const void *buffer);
};

///
/// @class   sc_table
///
/// @brief  Maintains a table of HTTP status code nodes.
///
/// The first element is reserved for the unknown code, which is 
/// returned when the requested code is not found or if the index is 
/// out of range.
///
/// HTTP status codes are expected to be inserted in the ascending 
/// order. Codes inserted out of order will be discarded and the first, 
/// unknown code, will be returned when such code is requested.
///
class sc_table_t {
   std::vector<storable_t<scnode_t>> stcodes;      ///< HTTP status code nodes

   size_t               clsindex[6];   ///< HTTP status class group offsets (0th is not used)

   public:
      sc_table_t(void);

      ~sc_table_t(void);

      void add_status_code(u_int code);

      storable_t<scnode_t>& get_status_code(u_int code);

      size_t size(void) const {return stcodes.size();}

      storable_t<scnode_t>& operator [] (size_t index);

      const storable_t<scnode_t>& operator [] (size_t index) const;
};

#endif // SCNODE_H
