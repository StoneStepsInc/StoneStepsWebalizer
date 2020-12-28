/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    danode.h
*/
#ifndef DANODE_H
#define DANODE_H

#include "keynode.h"
#include "datanode.h"
#include "tstamp.h"
#include "types.h"

///
/// @brief  Active download job node
///
/// 1. An active download node shares the node ID with the download job
/// node.
///
struct danode_t : public keynode_t<uint64_t>, public datanode_t<danode_t> {
      uint64_t    hits;              ///< Request count
      tstamp_t    tstamp;            ///< Last request timestamp
      uint64_t    proctime;          ///< Download job time (msec)
      uint64_t    xfer;              ///< xfer size in bytes

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(danode_t& vnode, param_t ... param);

      public:
         danode_t(uint64_t _nodeid = 0);

         void reset(uint64_t _nodeid = 0);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t...> upcb, param_t ... param);
};


#endif // DANODE_H

