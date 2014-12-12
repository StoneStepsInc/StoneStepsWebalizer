/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    dlnode.h
*/
#ifndef __DLNODE_H
#define __DLNODE_H

#include "hashtab.h"
#include "types.h"
#include "tstring.h"
#include "basenode.h"
#include "danode.h"

struct hnode_t;

// -----------------------------------------------------------------------
//
// download node
//
// -----------------------------------------------------------------------
struct dlnode_t : public base_node<dlnode_t> {
      // combined download job data
      uint64_t      count;             // download job count
      uint64_t      sumhits;           // total hits
      uint64_t    sumxfer;           // total transfer size
      double      avgxfer;           // average transfer size
      double      avgtime;				 // average job processing time (minutes)
      double      sumtime;		       // total job processing time (minutes)

      danode_t    *download;         // active download job
      hnode_t     *hnode;            // host node

      bool        ownhost : 1;       // true, if dlnode_t owns hnode

      public:
         typedef void (*s_unpack_cb_t)(dlnode_t& vnode, uint64_t hostid, bool active, void *arg);

      public:
         dlnode_t(void);
         dlnode_t(const dlnode_t& tmp);
         dlnode_t(const char *name, hnode_t *hnode);

         ~dlnode_t(void);

         void set_host(hnode_t *hnode);

         void reset(uint64_t nodeid = 0);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         uint64_t s_hash_value(void) const;

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_dlname(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_hostid(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_mp_compare_value(const void *buf1, const void *buf2, u_int partid, bool& lastpart);
         static int64_t s_compare_xfer(const void *buf1, const void *buf2);
};

//
// Download Jobs
//
class dl_hash_table : public hash_table<dlnode_t> {
   public:
      struct param_block {
         const char  *name;
         const char  *ipaddr;
      };

   private:
      virtual bool compare(const dlnode_t *nptr, const void *param) const;

      virtual bool load_array_check(const dlnode_t *nptr) const {return (nptr && !nptr->download) ? true : false;}

   public:
      dl_hash_table(void) : hash_table<dlnode_t>() {}
};

#endif // __DLNODE_H

