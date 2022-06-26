/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    vnode.h
*/
#ifndef VNODE_H
#define VNODE_H

#include "keynode.h"
#include "datanode.h"
#include "linklist.h"
#include "tstamp.h"
#include "types.h"
#include "storable.h"

struct unode_t;

///
/// @brief  Visit node
///
/// 1. A visit node shares the node ID with the host node that owns it. 
///
/// 2. `set_lasturl` is the only method that can change the reference count 
/// of the last URL. Other methods (e.g. copy constructor, `reset`, destructor)
/// simply manipulate object data without affecting the reference count. 
///
/// For example, if v1 refers to a URL with `vstref` equal 3 and an assignment 
/// v2 = v1 is made, the reference count will remain 3 when v1 is destroyed.
///
/// 3. Some hits in a visit may be ignored (e.g. 302 status code). If such 
/// a hit was marked as a start of a new visit, it will not be counted as 
/// an entry URL. `entry_url` is used to track whether an entry URL for this 
/// host has been counted or not.
///
/// 4. The `robot` flag in the visit node is purely informational and should 
/// not affect the value of the `robot` flag in the host node. For example, 
/// two different robots may operate from the same IP address and if one of 
/// these robots is not listed in the robot configuration and the `robot` flag 
/// in the visit node overrides the `robot` flag in the host node, host and 
/// country visit counters will be miscounted when the active visit node is
/// gone because visitors and hosts are tightly coupled in the current design.
/// A better approach is to split hosts onto hosts and visitors, so multiple
/// visitors can share the same IP address, but are tracked independent of
/// their IP address alone.
///
struct vnode_t : public keynode_t<uint64_t>, public datanode_t<vnode_t> {
      storable_t<vnode_t> *next;    ///< Next visit node queued for host 
                                    ///< name grouping.

      bool     entry_url: 1;        ///< Entry URL set?
      bool     robot    : 1;        ///< Robot?
      bool     converted: 1;        ///< Requested target URL?

      tstamp_t start;               ///< First hit time stamp
      tstamp_t end;                 ///< Last hit time stamp

      uint64_t hits;                ///< Visit request count
      uint64_t files;               ///< Visit file request count
      uint64_t pages;               ///< Visit page request count

      uint64_t hostref;             ///< Host references

      storable_t<unode_t> *lasturl; ///< Last requested URL

      uint64_t  xfer;               ///< Visit transfer amount

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(vnode_t& vnode, uint64_t urlid, param_t ... param);

      public:
         vnode_t(uint64_t nodeid = 0);
         vnode_t(vnode_t&& vnode) noexcept;

         ~vnode_t(void);

         void reset(uint64_t nodeid = 0);

         void set_lasturl(storable_t<unode_t> *unode);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);
};

#endif // VNODE_H
