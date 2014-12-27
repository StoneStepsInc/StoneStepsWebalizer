/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    danode.h
*/
#ifndef __DANODE_H
#define __DANODE_H

#include "keynode.h"
#include "datanode.h"
#include "types.h"

// -----------------------------------------------------------------------
//
// Active download job
//
// -----------------------------------------------------------------------
// 1. An active download node shares the node ID with the download job
// node.
//
struct danode_t : public keynode_t<uint64_t>, public datanode_t<danode_t> {
      uint64_t    hits;              // request count (if zero, no active job)
      tstamp_t    tstamp;            // last request timestamp
      uint64_t    proctime;		    // download job time (msec)
      uint64_t    xfer;              // xfer size in bytes

      public:
         typedef void (*s_unpack_cb_t)(danode_t& vnode, void *arg);

      public:
         danode_t(uint64_t _nodeid = 0);
         danode_t(const danode_t& danode);

         void reset(uint64_t _nodeid = 0);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);
};


#endif // __DANODE_H

