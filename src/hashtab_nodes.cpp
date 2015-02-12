/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hashtab-nodes.cpp
*/
#include "pch.h"

#include "hashtab_nodes.h"

//
// include templates that are being instantiated
//
#include "datanode_tmpl.cpp"
#include "basenode_tmpl.cpp"
#include "hashtab_tmpl.cpp"

#if defined(_WIN32) && (_MSC_VER == 1200)
#pragma warning(disable:4660) // template-class specialization 'type' is already instantiated
#endif

//
// initialize specialized node versions
//
template<> const u_short datanode_t<hnode_t> ::__version = 6;
template<> const u_short datanode_t<unode_t> ::__version = 3;
template<> const u_short datanode_t<rnode_t> ::__version = 2;
template<> const u_short datanode_t<anode_t> ::__version = 3;
template<> const u_short datanode_t<snode_t> ::__version = 2;
template<> const u_short datanode_t<inode_t> ::__version = 3;
template<> const u_short datanode_t<vnode_t> ::__version = 4;
template<> const u_short datanode_t<rcnode_t>::__version = 1;
template<> const u_short datanode_t<dlnode_t>::__version = 1;
template<> const u_short datanode_t<spnode_t>::__version = 1;
template<> const u_short datanode_t<ccnode_t>::__version = 2;
template<> const u_short datanode_t<totals_t>::__version = 6;
template<> const u_short datanode_t<danode_t>::__version = 2;
template<> const u_short datanode_t<scnode_t>::__version = 1;
template<> const u_short datanode_t<daily_t> ::__version = 2;
template<> const u_short datanode_t<hourly_t>::__version = 1;
template<> const u_short datanode_t<sysnode_t>::__version = 5;

//
// hash table base webalizer nodes
//
template struct base_node<hnode_t>;
template struct base_node<unode_t>;
template struct base_node<rnode_t>;
template struct base_node<anode_t>;
template struct base_node<snode_t>;
template struct base_node<inode_t>;
template struct base_node<rcnode_t>;
template struct base_node<dlnode_t>;
template struct base_node<spnode_t>;

//
//
//
template class datanode_t<hnode_t>;
template class datanode_t<unode_t>;
template class datanode_t<rnode_t>;
template class datanode_t<anode_t>;
template class datanode_t<snode_t>;
template class datanode_t<inode_t>;
template class datanode_t<rcnode_t>;
template class datanode_t<dlnode_t>;
template class datanode_t<spnode_t>;
template class datanode_t<ccnode_t>;
template class datanode_t<totals_t>;
template class datanode_t<vnode_t>;
template class datanode_t<danode_t>;
template class datanode_t<scnode_t>;
template class datanode_t<daily_t>;
template class datanode_t<hourly_t>;
template class datanode_t<sysnode_t>;

//
// VC6 generates erroneous warnings if node templates are instantiated explicitly
//
#if _MSC_VER != 1200
//
// hash table nodes
//
template struct htab_node_t<hnode_t>;
template struct htab_node_t<unode_t>;
template struct htab_node_t<rnode_t>;
template struct htab_node_t<anode_t>;
template struct htab_node_t<snode_t>;
template struct htab_node_t<inode_t>;
template struct htab_node_t<rcnode_t>;
template struct htab_node_t<dlnode_t>;
template struct htab_node_t<spnode_t>;
template struct htab_node_t<ccnode_t>;
#endif

//
// hash tables
//
template class hash_table<hnode_t>;
template class hash_table<unode_t>;
template class hash_table<rnode_t>;
template class hash_table<anode_t>;
template class hash_table<snode_t>;
template class hash_table<inode_t>;
template class hash_table<rcnode_t>;
template class hash_table<dlnode_t>;
template class hash_table<spnode_t>;
template class hash_table<ccnode_t>;

