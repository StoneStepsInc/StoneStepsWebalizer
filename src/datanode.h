/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   datanode.h
*/
#ifndef DATANODE_H
#define DATANODE_H

#include "types.h"

///
/// @class  datanode_t
///
/// @brief  A base class for database nodes
/// 
/// `datanode_t` is a template class so its version can be explicitly specialized for 
/// each specific node type in hashtab_nodes.cpp.
///
/// The version is stored in the database, but not in a node instance in memory. 
/// Instead, the version is made available when unpacking data, so the node could 
/// read the buffer according to the stored version of serialized data.
///
/// New data is always written according to the latest version.
///
/// Node state flags are for in-memory maintenance only and are not
/// saved in the database. Flags can be updated for const nodes (e.g.  
/// when saving node data to the database).
///
/// The node is marked as dirty if its content is new or updated,
/// compared with some base state. For most nodes, such as host nodes,
/// such state is represented by the version of the node in the database
/// (a new node is a special case, when it's missing from the database). 
/// Visit and download nodes are always dirty, but may be destroyed 
/// without having to save them (i.e. when factoring in their data into
/// visits and downloads). 
/// 
/// If the storage flag is `true`, the node was read from the database.
///
template <typename node_t>
class datanode_t {
   private:
      static const u_short __version;

   public:
      bool dirty   : 1;           // is new or updated content?
      bool storage : 1;           // is stored externally?

   public:
      datanode_t(void);

      void reset(void);

      //
      // serialization
      //
      size_t s_data_size(void) const;

      size_t s_pack_data(void *buffer, size_t bufsize) const;

      size_t s_unpack_data(const void *buffer, size_t bufsize);

      static size_t s_data_size(const void *buffer);

      static u_short s_node_ver(const void *buffer);
};

#endif // DATANODE_H
