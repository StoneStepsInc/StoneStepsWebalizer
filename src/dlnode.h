/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    dlnode.h
*/
#ifndef DLNODE_H
#define DLNODE_H

#include "hashtab.h"
#include "types.h"
#include "tstring.h"
#include "basenode.h"
#include "danode.h"
#include "storable.h"

struct hnode_t;

///
/// @struct dlnode_t
///
/// @brief  Download job node
///
/// A download node owns the active download job node and will delete it in its
/// destructor.
///
/// A download node does not own a host node and will merely increment a download
/// reference count in the host node when it is attached to the download node.
///
struct dlnode_t : public base_node<dlnode_t> {
      ///
      /// @struct param_block
      ///
      /// @brief  A compound key structure for a download node.
      ///
      struct param_block {
         const char  *name;            ///< Download name
         const char  *ipaddr;          ///< IP address
      };

      // combined download job data
      uint64_t    count;               ///< Download job count
      uint64_t    sumhits;             ///< Total hits
      uint64_t    sumxfer;             ///< Total transfer size
      double      avgxfer;             ///< Average transfer size
      double      avgtime;             ///< Average job processing time (minutes)
      double      sumtime;             ///< Total job processing time (minutes)

      storable_t<danode_t> *download;  ///< Active download job
      hnode_t     *hnode;              ///< Host node

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(dlnode_t& dlnode, uint64_t hostid, bool active, void *arg, param_t ... param);

      public:
         dlnode_t(void);
         dlnode_t(dlnode_t&& tmp);
         dlnode_t(const string_t& name, hnode_t *hnode);

         ~dlnode_t(void);

         void set_host(hnode_t *hnode);

         void reset(uint64_t nodeid = 0);

         bool match_key_ex(const dlnode_t::param_block *pb) const;

         virtual bool match_key(const string_t& key) const override
         {
            throw std::logic_error("This node only supports searches with a compound key");
         }

         static uint64_t hash(const string_t& ipaddr, const string_t& dlname) 
         {
            return hash_ex(hash_ex(0, ipaddr), dlname);
         }

         virtual uint64_t get_hash(void) const override;

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param);

         uint64_t s_hash_value(void) const;

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_dlname(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_hostid(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_xfer(const void *buf1, const void *buf2);
};

//
// Download Jobs
//
class dl_hash_table : public hash_table<storable_t<dlnode_t>> {
   public:
      dl_hash_table(void) : hash_table<storable_t<dlnode_t>>() {}
};

#endif // DLNODE_H

