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
// danode_t (active download job)
//
// -----------------------------------------------------------------------
// 1. An active download node shares the node ID with the download job
// node.
//
struct danode_t : public keynode_t<u_long>, public datanode_t<danode_t> {
      u_long      hits;              // request count (if zero, no active job)
      time_t      tstamp;            // last request timestamp
      u_long      proctime;		    // download job time (msec)
      u_long      xfer;              // xfer size in bytes

      public:
         typedef void (*s_unpack_cb_t)(danode_t& vnode, void *arg);

      public:
         danode_t(u_long _nodeid = 0);
         danode_t(const danode_t& danode);

         void reset(u_long _nodeid = 0);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer);
};


#endif // __DANODE_H

