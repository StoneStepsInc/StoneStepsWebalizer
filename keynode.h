/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   basenode.h
*/
#ifndef __KEYNODE_H
#define __KEYNODE_H

#include "types.h"

// -----------------------------------------------------------------------
//
// keynode_t
//
// -----------------------------------------------------------------------
// 1. keynode_t provides functionality to serialize and deserialize 
// database keys. 
//
// 2. For numeric type_t, nodeid is set to zero to indicate a generic 
// node not containing any data.
//
// 3. keynode_t does not have a virtual destructor and should not be
// used to delete derived objects. The first derived object with a 
// virtual destructor should be used instead.
//
template <typename type_t>
struct keynode_t {
      type_t   nodeid;              // unique node identifier

      public:
         keynode_t(type_t nodeid);
         keynode_t(const keynode_t& keynode);

         void reset(type_t nodeid);

         //
         // serialization
         //
         u_int s_key_size(void) const;
         u_int s_pack_key(void *buffer, u_int bufsize) const;
         u_int s_unpack_key(const void *buffer, u_int bufsize);

         static u_int s_key_size(const void *buffer);

         static int s_compare_key(const void *buf1, const void *buf2);
};

#endif // __KEYNODE_H

