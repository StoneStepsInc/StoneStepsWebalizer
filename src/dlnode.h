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
#include "hashtab.h"
#include "keynode.h"
#include "datanode.h"
#include "danode.h"
#include "storable.h"

struct hnode_t;

///
/// @brief  A download job node.
///
/// A download job is a download name tracked for a specific visitor identified
/// by an IP address. A download job may be executed one or more times for its
/// visitor, which is reflected as a download count for this download job.
///
/// Another way to look at download jobs is to 
///
/// For example, if there are two files posted for a download, `a.zip` and `b.zip`
/// and they correspond to download names `A` and `B`, respectively, and if `a.zip`
/// was downloaded twice by a visitor `X` and `b.zip` was downloaded once by the
/// visitor `X` and twice by a visitor `Y`, then there will be three download jobs
/// - `{A, X}`, `{B, X}` and `{B, Y}`, and five downloads - `2 + 1 + 2`.
///
/// Only one download may be in progress (active) for each download job, which is
/// tracked by an instance of an active download node (`danode_t`), which is owned
/// by the download job and will be deleted with the download job node.
///
/// A download job node with a non-empty name must be associated with a host node
/// one way ot another. In the most common case both nodes exist in their hash tables
/// and the download job reference count in the host node reflects the number of
/// download job nodes pointing to this host, which prevents host nodes with non-zero
/// reference counts from being swapped out. However, it is also possible to read a
/// download node and the associated host node from the database into local vatiables,
/// in which case the download job node will not be set up automatically to point to
/// the host node because having a pointer to an address of what could be a local
/// variable is very error-prone, and instead the function using these local variables
/// should call `set_host` to link both nodes and should arrange that the download
/// node is destroyed first.
///
struct dlnode_t : public htab_obj_t<const string_t&, const string_t&>, public keynode_t<uint64_t>, public datanode_t<dlnode_t> {
      // combined download job data
      string_t    name;                ///< Download job name.
      uint64_t    count;               ///< Number of times this download was performed.
      uint64_t    sumhits;             ///< Total number of requests.
      uint64_t    sumxfer;             ///< Total transfer size.
      double      avgxfer;             ///< Average transfer size per download.
      double      avgtime;             ///< Average job processing time per download (minutes).
      double      sumtime;             ///< Total job processing time (minutes).

      storable_t<danode_t> *download;  ///< Active download node.
      hnode_t     *hnode;              ///< Host node.

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(dlnode_t& dlnode, uint64_t hostid, bool active, param_t ... param);

      public:
         dlnode_t(void);
         dlnode_t(dlnode_t&& tmp);
         dlnode_t(const string_t& name, hnode_t& hnode);

         ~dlnode_t(void);

         nodetype_t get_type(void) const override {return OBJ_REG;}

         void set_host(hnode_t *hnode);

         void reset(uint64_t nodeid = 0);

         bool match_key(const string_t& ipaddr, const string_t& dlname) const override;

         static uint64_t hash_key(const string_t& ipaddr, const string_t& dlname) 
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
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         uint64_t s_hash_value(void) const;
         size_t s_hash_value_size(void) const {return sizeof(uint64_t);}

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_value_hash(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

//
// Download Jobs
//
class dl_hash_table : public hash_table<storable_t<dlnode_t>> {
   public:
      dl_hash_table(void) : hash_table<storable_t<dlnode_t>>() {}
};

#endif // DLNODE_H

