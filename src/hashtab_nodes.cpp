/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

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

//
// initialize specialized node versions
//
template<> const u_short datanode_t<hnode_t> ::__version = 9;
template<> const u_short datanode_t<unode_t> ::__version = 3;
template<> const u_short datanode_t<rnode_t> ::__version = 2;
template<> const u_short datanode_t<anode_t> ::__version = 3;
template<> const u_short datanode_t<snode_t> ::__version = 2;
template<> const u_short datanode_t<inode_t> ::__version = 3;
template<> const u_short datanode_t<vnode_t> ::__version = 4;
template<> const u_short datanode_t<rcnode_t>::__version = 1;
template<> const u_short datanode_t<dlnode_t>::__version = 1;
template<> const u_short datanode_t<ccnode_t>::__version = 3;
template<> const u_short datanode_t<ctnode_t>::__version = 1;
template<> const u_short datanode_t<asnode_t>::__version = 1;
template<> const u_short datanode_t<totals_t>::__version = 7;
template<> const u_short datanode_t<danode_t>::__version = 2;
template<> const u_short datanode_t<scnode_t>::__version = 2;
template<> const u_short datanode_t<daily_t> ::__version = 2;
template<> const u_short datanode_t<hourly_t>::__version = 1;
template<> const u_short datanode_t<sysnode_t>::__version = 6;

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
template class datanode_t<ccnode_t>;
template class datanode_t<ctnode_t>;
template class datanode_t<asnode_t>;
template class datanode_t<totals_t>;
template class datanode_t<vnode_t>;
template class datanode_t<danode_t>;
template class datanode_t<scnode_t>;
template class datanode_t<daily_t>;
template class datanode_t<hourly_t>;
template class datanode_t<sysnode_t>;

//
// hash table nodes
//
template struct htab_node_t<storable_t<hnode_t>>;
template struct htab_node_t<storable_t<unode_t>>;
template struct htab_node_t<storable_t<rnode_t>>;
template struct htab_node_t<storable_t<anode_t>>;
template struct htab_node_t<storable_t<snode_t>>;
template struct htab_node_t<storable_t<inode_t>>;
template struct htab_node_t<storable_t<rcnode_t>>;
template struct htab_node_t<storable_t<dlnode_t>>;
template struct htab_node_t<storable_t<ccnode_t>>;
template struct htab_node_t<storable_t<ctnode_t>>;
template struct htab_node_t<storable_t<asnode_t>>;

//
// hash tables
//
template class hash_table<storable_t<hnode_t>>;
template class hash_table<storable_t<unode_t>>;
template class hash_table<storable_t<rnode_t>>;
template class hash_table<storable_t<anode_t>>;
template class hash_table<storable_t<snode_t>>;
template class hash_table<storable_t<inode_t>>;
template class hash_table<storable_t<rcnode_t>>;
template class hash_table<storable_t<dlnode_t>>;
template class hash_table<storable_t<ccnode_t>>;
template class hash_table<storable_t<ctnode_t>>;
template class hash_table<storable_t<asnode_t>>;
