/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   datanode.h
*/
#ifndef DATANODE_H
#define DATANODE_H

#include "types.h"
#include "storable.h"

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
/// read the buffer according to the stored version of serialized data. This way
/// we don't waste two bytes in each node instance on a misleading version that
/// becomes obsolete the moment it was read from the database.
///
/// New data is always written according to the latest version.
///
template <typename node_t>
class datanode_t {
   private:
      static const u_short __version;

   public:
      datanode_t(void);

      void reset(void);

      //
      // serialization
      //
      size_t s_data_size(void) const;

      size_t s_pack_data(void *buffer, size_t bufsize) const;

      size_t s_unpack_data(const void *buffer, size_t bufsize);

      static size_t s_data_size(const void *buffer, size_t bufsize);

      static u_short s_node_ver(const void *buffer);
};

#endif // DATANODE_H
