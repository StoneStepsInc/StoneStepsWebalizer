/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   database_tmpl.cpp
*/

#include "database.h"

//
// hnode
//

template <typename ... param_t>
bool database_t::get_hnode_by_id(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
{
   return hosts.get_node_by_id(hnode, upcb, std::forward<param_t>(param) ...);
}

template <typename ... param_t>
bool database_t::get_hnode_by_value(storable_t<hnode_t>& hnode, hnode_t::s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
{
   return hosts.get_node_by_value(hnode, upcb, std::forward<param_t>(param) ...);
}

//
// downloads
//

template <typename ... param_t>
bool database_t::get_dlnode_by_id(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
{
   return downloads.get_node_by_id<dlnode_t, param_t...>(dlnode, upcb, std::forward<param_t>(param) ...);
}

template <typename ... param_t>
bool database_t::get_dlnode_by_value(storable_t<dlnode_t>& dlnode, dlnode_t::s_unpack_cb_t<param_t ...> upcb, param_t ... param) const
{
   return downloads.get_node_by_value<dlnode_t, param_t...>(dlnode, upcb, std::forward<param_t>(param) ...);
}

#include "berkeleydb_tmpl.cpp"
