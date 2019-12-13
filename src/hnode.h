/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hnode.h
*/
#ifndef HNODE_H
#define HNODE_H

#include "linklist.h"
#include "hashtab.h"
#include "basenode.h"
#include "unode.h"
#include "vnode.h"
#include "tstamp.h"
#include "types.h"
#include "storable.h"

///
/// @brief  Host node
///
/// 1. Host node cannot delete active visit until visit data is factored 
/// into the host data, which requires the current log time stamp and cannot 
/// be done at the `hnode_t` level.
///
/// 2. The `resolved` flag indicates that the host node has gone through the
/// DNS resolver in the current run. This flag is not saved in the state
/// database and instead host nodes that come from the state database are 
/// assumed to be resolved and the `resolved` flag is set when nodes are read
/// from the database.
///
/// 4. `grp_visit` is a linked list of ended visits that have not been grouped
/// because the host name has not been resolved. Visit nodes in this list 
/// are owned by the host node and do not maintain reference counts for the
/// referring host node.
///
/// 5. The `robot` flag indicates that this IP address was the source address
/// of a request with a user agent matching the robot criteria. Although it 
/// is possible that a human visitor will share the IP address with a robot,
/// it is not very likely to justify having to maintain separate counts 
/// (hits, xfer, etc) for robot and non-robot users using a particular IP 
/// address.
///
/// 6. It is possible for `hnode_t::robot` and `vnode_t::robot` to have different 
/// values. For example, different robots may operate from the same IP address
/// and some of these robots may not be listed in the configuration. Robot flag
/// in the visit node should always be ignored to make sure all counters are in
/// sync regardless whether there is a visit active or not. See `vnode_t` for
/// details.
///
struct hnode_t : public base_node<hnode_t> {
      static const size_t ccode_size = 2;   ///< In characters, not counting the zero terminator

      uint64_t count;                ///< Request count
      uint64_t files;                ///< Files requested
      uint64_t pages;                ///< Pages requested

      uint64_t visits;               ///< Visits started
      uint64_t visits_conv;          ///< Visits converted

      uint64_t visit_max;            ///< Maximum visit length (in seconds)

      uint64_t max_v_hits;           ///< Maximum number of hits
      uint64_t max_v_files;          ///< Maximum number of files
      uint64_t max_v_pages;          ///< Maximum number of pages per visit

      uint64_t dlref;                ///< Download node reference count
      
      tstamp_t tstamp;               ///< Last request timestamp

      storable_t<vnode_t> *visit;      ///< Current visit (nullptr if none)
      storable_t<vnode_t> *grp_visit;  ///< Visits queued for name grouping

      string_t name;                 ///< Host name

      uint64_t max_v_xfer;           ///< Maximum transfer amount per visit
      uint64_t xfer;                 ///< Transfer amount in bytes
      double   visit_avg;            ///< Average visit length (in seconds)

      bool     spammer  : 1;         ///< Caught spamming?
      bool     robot    : 1;         ///< Robot?
      bool     resolved : 1;         ///< Has been resolved? (not saved in the state database)

      char     ccode[ccode_size+1];  ///< Country code

      string_t city;                 ///< City name reported by GeoIP

      double   latitude;             ///< Latitude reported by GeoIP
      double   longitude;            ///< Longitude reported by GeoIP

      uint32_t geoname_id;           ///< Geoname identifier (see `ctnode_t`)

      uint32_t as_num;              ///< Autonomous system number.
      string_t as_org;              ///< Autonomous system organization.

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(hnode_t& hnode, bool active, param_t ... param);

      public:
         hnode_t(void);
         hnode_t(hnode_t&& tmp) noexcept;
         hnode_t(const string_t& ipaddr);

         ~hnode_t(void);

         void set_visit(storable_t<vnode_t> *vnode);

         void reset(uint64_t nodeid = 0);

         bool entry_url_set(void) const {return visit && visit->entry_url;}

         void set_entry_url(void) {if(visit) visit->entry_url = true;}
         
         void set_last_url(storable_t<unode_t> *unode) {if(visit) visit->set_lasturl(unode);}

         void set_ccode(const char ccode[2]);

         string_t get_ccode(void) const {return string_t::hold(ccode);}

         void reset_ccode(void);

         const string_t& hostname(void) const {return name.isempty() ? string : name;}

         void add_grp_visit(storable_t<vnode_t> *vnode);

         vnode_t *get_grp_visit(void);
         
         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

///
/// @brief  A hash table containing all IP addresses for the current month.
///
class h_hash_table : public hash_table<storable_t<hnode_t>> {
   public:
      h_hash_table(void) : hash_table<storable_t<hnode_t>>(LMAXHASH) {}
};

#endif // HNODE_H
