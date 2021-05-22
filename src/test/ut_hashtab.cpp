/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_hashtab.cpp
*/
#include "pch.h"

#include "../anode.h"
#include "../dlnode.h"
#include "../ccnode.h"
#include "../hnode.h"
#include "../unode.h"

#include <string>
#include <list>
#include <stdexcept>

namespace sswtest {

///
/// @brief  Tests the time stamp order for inserts and look-ups.
///
TEST(HashTableTest, NodeTimeStampOrder)
{
   hash_table<storable_t<anode_t>> htab(10);

   htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 1"), OBJ_REG, false), 1);

   EXPECT_NO_THROW(htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 2"), OBJ_REG, false), 1)) << "Same time stamp should not throw an exception when inserting";
   EXPECT_NO_THROW(htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 3"), OBJ_REG, false), 10)) << "Same time stamp should not throw an exception when inserting";

   {
      // keep the node pointer, so we don't leak while throwing
      storable_t<anode_t> *anode = new storable_t<anode_t>(string_t::hold("Agent 4"), OBJ_REG, false);

      // do not release the pointer because it never is inserted
      EXPECT_THROW(htab.put_node(anode, 5), std::logic_error) << "Cannot insert a node with an earlier time stamp";
   }

   ASSERT_THROW(htab.find_node(OBJ_REG, (int64_t) 5, string_t::hold("Agent 2")), std::logic_error) << "Cannot insert a node with an earlier time stamp";

   EXPECT_NO_THROW(htab.find_node(OBJ_REG, (int64_t) 10, string_t::hold("Agent 2"))) << "A look-up for an existing key using the same time stamp should not throw an exception";
   EXPECT_NO_THROW(htab.find_node(OBJ_REG, (int64_t) 20, string_t::hold("Agent 2"))) << "A look-up for an existing key using a greater time stamp should not throw an exception";
}

///
/// @brief  Tests inserting storable objects with a single string key that are 
///         derived from `base_node`.
///
TEST(HashTableTest, NodeInsertBaseNodeStorableWithKey)
{
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   for(int64_t i = 0; i < 100; i++) {
      int64_t tstamp = i - i % 2;
      std::string agent = "Agent " + std::to_string(tstamp);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      // use a non-const look-up with a time stamp 
      anode_t *anode = htab.find_node(OBJ_REG, tstamp, agent_key);

      // alternate finding and not finding the node in the hash table
      if(i % 2)
         EXPECT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Every odd agent number should be found in the hash table";
      else {
         ASSERT_EQ(nullptr, anode) << "Every even agent number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((anode = htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), tstamp)));

         ASSERT_TRUE(anode != nullptr) << "A newly inserted node cannot be nullptr";

         EXPECT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Newly inserted node must have the correct key";
      }

      // use a const version of a node look-up to test the insert without changing the hash table
      const anode_t *canode = htab.find_node(OBJ_REG, agent_key);

      ASSERT_TRUE(canode != nullptr) << "A newly inserted node cannot be nullptr when looked up via the const interface";

      EXPECT_STREQ(agent_key.c_str(), canode->string.c_str()) << "Newly inserted node must have the correct key";
   }

   EXPECT_EQ(50, htab.size()) << "50 hash table nodes should be inserted";
}

///
/// @brief  Tests inserting storable objects with a single string key that are
///         not derived from `base_node`.
///
TEST(HashTableTest, NodeInsertStorableWithKey)
{
   hash_table<storable_t<ccnode_t>> htab(10);  // use 10 buckets to test bucket lists

   for(int64_t i = 0; i < 100; i++) {
      int64_t tstamp = i - i % 2;
      std::string ccode = "cc" + std::to_string(tstamp);
      string_t ccode_key(string_t::hold(ccode.c_str(), ccode.length()));

      // use a non-const look-up with a time stamp 
      ccnode_t *ccnode = htab.find_node(OBJ_REG, tstamp, ccode_key);

      // alternate finding and not finding the node in the hash table
      if(i % 2)
         EXPECT_STREQ(ccode_key.c_str(), ccnode->ccode.c_str()) << "Every odd number should be found in the hash table";
      else {
         ASSERT_EQ(nullptr, ccnode) << "Every even number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((ccnode = htab.put_node(new storable_t<ccnode_t>(ccode_key, ("Country " + std::to_string(tstamp)).c_str()), tstamp)));

         ASSERT_TRUE(ccnode != nullptr) << "A newly inserted node cannot be nullptr";

         EXPECT_STREQ(ccode_key.c_str(), ccnode->ccode.c_str()) << "Newly inserted node must have the correct key";
      }

      // use a const version of a node look-up to test the insert without changing the hash table
      const ccnode_t *cccnode = htab.find_node(OBJ_REG, ccode_key);

      ASSERT_TRUE(cccnode != nullptr) << "A newly inserted node cannot be nullptr when looked up via the const interface";

      EXPECT_STREQ(ccode_key.c_str(), cccnode->ccode.c_str()) << "Newly inserted node must have the correct key";
   }

   EXPECT_EQ(50, htab.size()) << "50 hash table nodes should be inserted";
}

///
/// @brief  Tests inserting storable objects with a compound key that are derived 
///         from `base_node`.
///
TEST(HashTableTest, NodeInsertBaseNodeStorableWithParamBlock)
{
   hash_table<storable_t<dlnode_t>> htab(10);   // use 10 buckets to test bucket lists

   std::list<std::unique_ptr<hnode_t>> hosts;   // download nodes reference hosts, but don't own them

   for(int i = 0; i < 100; i++) {
      int tstamp = i - i % 2;
      std::string download = "Download " + std::to_string(tstamp);
      std::string ipaddr = "127.0.0." + std::to_string(tstamp);

      hnode_t *host = hosts.insert(hosts.end(), std::make_unique<hnode_t>(string_t::hold(ipaddr.c_str(), ipaddr.length()), OBJ_REG))->get();

      uint64_t hashval = dlnode_t::hash_key(string_t::hold(ipaddr.c_str()), string_t::hold(download.c_str()));

      // use a non-const look-up with a time stamp 
      storable_t<dlnode_t> *dlnode = htab.find_node(hashval, OBJ_REG, tstamp, string_t::hold(ipaddr.c_str()), string_t::hold(download.c_str()));

      // alternate finding and not finding the node in the hash table
      if(i % 2) {
         EXPECT_STREQ(download.c_str(), dlnode->name.c_str()) << "Every odd number should be found in the hash table (name)";
         EXPECT_STREQ(ipaddr.c_str(), dlnode->hnode->string.c_str()) << "Every odd number should be found in the hash table (IP address)";
      }
      else {
         ASSERT_EQ(nullptr, dlnode) << "Every even number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((dlnode = htab.put_node(new storable_t<dlnode_t>(string_t::hold(download.c_str()), *host), tstamp)));

         ASSERT_TRUE(dlnode != nullptr) << "A newly inserted node cannot be nullptr";

         EXPECT_STREQ(string_t::hold(download.c_str()), dlnode->name.c_str()) << "Newly inserted node must have the correct key";
         EXPECT_STREQ(string_t::hold(ipaddr.c_str()), dlnode->hnode->string.c_str()) << "Newly inserted node must have the correct key";
      }

      // there is no const interface for compound key nodes
      dlnode = htab.find_node(hashval, OBJ_REG, tstamp, string_t::hold(ipaddr.c_str()), string_t::hold(download.c_str()));

      ASSERT_TRUE(dlnode != nullptr) << "A newly inserted node cannot be nullptr";

      EXPECT_STREQ(string_t::hold(download.c_str()), dlnode->name.c_str()) << "Newly inserted node must have the correct key";
      EXPECT_STREQ(string_t::hold(ipaddr.c_str()), dlnode->hnode->string.c_str()) << "Newly inserted node must have the correct key";
   }

   EXPECT_EQ(50, htab.size()) << "50 hash table nodes should be inserted";

   // make sure download job nodes are destroyed before host nodes they reference
   ASSERT_NO_THROW(htab.clear());
}

///
/// @brief  Tests hash table time stamp range.
///
TEST(HashTableTest, TimeStampRange)
{
   hash_table<storable_t<anode_t>> htab(10);

   EXPECT_EQ(0, htab.tm_range()) << "An empty hash table should report a zero for time range";

   for(int i = 100; i < 200; i++) {
      int tstamp = i - i % 2;
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      ASSERT_NO_THROW(htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), tstamp));
   }

   EXPECT_EQ(98, htab.tm_range()) << "The difference between the newest and oldest time stamps should be 98";
}

///
/// @brief  Tests swapping out oldest nodes from memory.
///
TEST(HashTableTest, SwapOut)
{
   size_t swapcnt = 0;     // number of swapped out nodes

   auto swap_cb = [] (storable_t<anode_t> *node, void *arg)
   {
      // increment the swap node count
      (*(size_t*) arg)++;
   };

   hash_table<storable_t<anode_t>> htab(10);

   htab.set_swap_out_cb(swap_cb, &swapcnt);

   // no look-ups are done because keys are guaranteed to be unique
   for(int i = 100; i < 200; i++) {
      int tstamp = i - i % 2;
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      ASSERT_NO_THROW(htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), tstamp));

      // every 10 iterations insert a group entry with a zero time stamp, which should be ignored
      if(i % 10 == 0) {
         std::string agent_group = "Agent Group " + std::to_string(i);

         storable_t<anode_t> *anode_group = new storable_t<anode_t>(string_t::hold(agent_group.c_str(), agent_group.length()), OBJ_GRP, false);

         // pass a zero time stamp because group nodes do not participate in time stamp ordering
         ASSERT_NO_THROW(htab.put_node(anode_group, 0));
      }
   }

   // 100 to 198, two of each time stamp
   EXPECT_EQ(98, htab.tm_range()) << "Time range should be 100-198 => 98";

   // swap swap nodes with time stamps 100-138 => 40 (two for each value)
   ASSERT_NO_THROW(htab.swap_out(138));

   // nodes with time stamps from 0 to 38, even values, no groups
   EXPECT_EQ(40, swapcnt) << "40 regular objects should be swapped out";

   // 60 regular objects and 10 groups
   EXPECT_EQ(60 + 10, htab.size()) << "60 regular nodes and 10 group nodes should remain in the hash table";

   // remaining nodes
   EXPECT_EQ(58, htab.tm_range()) << "The remaining  time range should be 140-198 => 58";

   // make sure all swapped out keys are gone
   for(int i = 100; i < 140; i++) {
      std::string agent = "Agent " + std::to_string(i);

      ASSERT_EQ(nullptr, htab.find_node(OBJ_REG, string_t::hold(agent.c_str(), agent.length()))) << "Swapped out keys should not be found";
   }

   // and all remaining regular and group keys are still there
   for(int i = 140; i < 200; i++) {
      std::string agent = "Agent " + std::to_string(i);

      ASSERT_TRUE(htab.find_node(OBJ_REG, string_t::hold(agent.c_str(), agent.length())) != nullptr) << "We sould be able to find every remaining regular object key";

      if(i % 10 == 0) {
         std::string agent_group = "Agent Group " + std::to_string(i);

         ASSERT_TRUE(htab.find_node(OBJ_GRP, string_t::hold(agent_group.c_str(), agent_group.length())) != nullptr) << "We sould be able to find every group object key";
      }
   }

   // make sure swapping out nodes didn't leave any dangling pointers
   ASSERT_NO_THROW(htab.clear());
}

///
/// @brief  Tests swapping out oldest nodes from the hash table by memory size.
///
TEST(HashTableTest, SwapOutByMemSize)
{
   size_t swapcnt = 0;     // number of swapped out nodes
   size_t nodesize = 0;    // estimated number of serialized bytes in the hash table

   auto swap_cb = [] (storable_t<anode_t> *node, void *arg)
   {
      // increment the swap node count
      (*(size_t*) arg)++;
   };

   hash_table<storable_t<anode_t>> htab(10);

   htab.set_swap_out_cb(swap_cb, &swapcnt);

   // no look-ups are done because keys are guaranteed to be unique and must be the same length
   for(int i = 100; i < 200; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      storable_t<anode_t> *anode = new storable_t<anode_t>(agent_key, OBJ_REG, false);

      nodesize += (anode->s_data_size() + sizeof(storable_t<anode_t>));

      ASSERT_NO_THROW(htab.put_node(anode, 0));
   }

   // swap out 40 nodes (50 + 10)
   ASSERT_NO_THROW(htab.swap_out(0, nodesize / 2 + nodesize / 10));

   EXPECT_EQ(40, swapcnt) << "40 nodes should be swapped out";

   EXPECT_EQ(60, htab.size()) << "60 nodes should remain in the hash table";
}

///
/// @brief  Tests swapping out oldest nodes from the hash table by memory size
///         that is less than actual table size.
///
TEST(HashTableTest, SwapOutByMemSizeLessThan)
{
   size_t swapcnt = 0;     // number of swapped out nodes
   size_t nodesize = 0;    // estimated number of serialized bytes in the hash table

   auto swap_cb = [] (storable_t<anode_t> *node, void *arg)
   {
      // increment the swap node count
      (*(size_t*) arg)++;
   };

   hash_table<storable_t<anode_t>> htab(10);

   htab.set_swap_out_cb(swap_cb, &swapcnt);

   // no look-ups are done because keys are guaranteed to be unique and must be the same length
   for(int i = 100; i < 200; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      storable_t<anode_t> *anode = new storable_t<anode_t>(agent_key, OBJ_REG, false);

      // simulate nodes reduced in size after they have been inserted into the hash table
      nodesize += anode->s_data_size() + sizeof(storable_t<anode_t>);
      nodesize -= 10;

      ASSERT_NO_THROW(htab.put_node(anode, 0));
   }

   // swap out to keep one node, but because nodes in the hast table are bigger, all will be swapped out
   ASSERT_NO_THROW(htab.swap_out(0, (nodesize + 100)/100));

   EXPECT_EQ(100, swapcnt) << "All nodes should be swapped out";

   EXPECT_EQ(0, htab.size()) << "Zero nodes should remain in the hash table";
}

///
/// @brief  Tests multiple hash table look-ups.
///
TEST(HashTableTest, MultipleLookUps)
{
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   // new nodes are inserted at the head of each bucket list
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), 0)));
   }

   // nodes that are looked up are moved to the beginning of the list - make sure we can still find them
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      anode_t *anode;

      ASSERT_NO_THROW((anode = htab.find_node(OBJ_REG, (int64_t) 0, agent_key)));

      ASSERT_TRUE(anode != nullptr) << "A node must exist in the hash table";

      ASSERT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Every node must be found in the hash table";
   }

   // look up the same nodes again to make sure any node movements within lists work properly
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      anode_t *anode;

      ASSERT_NO_THROW((anode = htab.find_node(OBJ_REG, (int64_t) 0, agent_key)));

      ASSERT_TRUE(anode != nullptr) << "A node must exist in the hash table";

      ASSERT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Every node must be found in the hash table";
   }
}

///
/// @brief  Tests a hash table iterator going over regular and group nodes.
///
TEST(HashTableTest, IteratorAllTypes)
{
   std::set<string_t> agent_node_names;
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   // new nodes are inserted at the head of each bucket list
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      agent_node_names.insert(agent_key);

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), 0)));

      if(i % 10 == 0) {
         std::string agent = "Agent Group " + std::to_string(i);
         string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

         storable_t<anode_t> *agent_grp = new storable_t<anode_t>(agent_key, OBJ_GRP, false);

         agent_node_names.insert(agent_key);

         ASSERT_NO_THROW((htab.put_node(agent_grp, 0)));
      }
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      if(iter.item()->flag == OBJ_REG)
         ASSERT_TRUE(strncmp("Agent ", iter.item()->string, 6) == 0);
      else
         ASSERT_TRUE(strncmp("Agent Group ", iter.item()->string, 12) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr from item() after the entire sequence has been traversed";

   EXPECT_EQ(0, agent_node_names.size()) << "All agent node names must be iterated over";
}

///
/// @brief  Tests a hash table iterator going over regular nodes.
///
TEST(HashTableTest, IteratorRegularObjects)
{
   std::set<string_t> agent_node_names;
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   // new nodes are inserted at the head of each bucket list
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      agent_node_names.insert(agent_key);

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, OBJ_REG, false), 0)));
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      ASSERT_EQ(OBJ_REG, iter.item()->flag) << "This hash map must only contain regular objects";

      ASSERT_TRUE(strncmp("Agent ", iter.item()->string, 6) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr from item() after the entire sequence has been traversed";

   EXPECT_EQ(0, agent_node_names.size()) << "All agent node names must be iterated over";
}

///
/// @brief  Tests a hash table iterator going over group nodes.
///
TEST(HashTableTest, IteratorGroupObjects)
{
   std::set<string_t> agent_node_names;
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   // new nodes are inserted at the head of each bucket list
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent Group " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      storable_t<anode_t> *agent_grp = new storable_t<anode_t>(agent_key, OBJ_GRP, false);

      agent_node_names.insert(agent_key);

      ASSERT_NO_THROW((htab.put_node(agent_grp, 0)));
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      ASSERT_EQ(OBJ_GRP, iter.item()->flag) << "This hash map must only contain group objects";

      ASSERT_TRUE(strncmp("Agent Group ", iter.item()->string, 12) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return nullptr from item() after the entire sequence has been traversed";

   EXPECT_EQ(0, agent_node_names.size()) << "All agent node names must be iterated over";
}

///
/// @brief  Tests a hash table iterator going over an empty hash table.
///
TEST(HashTableTest, IteratorEmpty)
{
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.next()) << "Next node of an empty hash map iterator should return nullptr";
   ASSERT_EQ(nullptr, iter.item()) << "Current node of an empty hash map iterator should be nullptr";
}

///
/// @brief  Tests that `unode_t` key methods produce expected results.
///
TEST(HashTableTest, URLMultiKey)
{
   string_t urlpath("/some/path/");
   string_t srchargs("n1=v1&n2=v2");

   string_t url_psa(urlpath + '?' + srchargs);     // URL path + search args
   string_t url_p(urlpath);                        // URL path without search args

   unode_t unode_psa(urlpath, OBJ_REG, srchargs);
   unode_t unode_p(urlpath, OBJ_REG, string_t());

   ASSERT_TRUE(unode_psa.match_key(urlpath, srchargs)) << "Separate URL key components must match for non-empty search args";
   ASSERT_TRUE(unode_psa.match_key(url_psa)) << "Single URL key must match for non-empty search args";

   ASSERT_TRUE(unode_p.match_key(urlpath, string_t())) << "Separate URL key components must match for empty search args";
   ASSERT_TRUE(unode_p.match_key(url_p)) << "Single URL key must match for empty search args";

   // both hash_key methods must yield the same hash as get_hash for non-empty search args
   ASSERT_EQ(unode_psa.get_hash(), unode_t::hash_key(urlpath, srchargs));
   ASSERT_EQ(unode_psa.get_hash(), unode_t::hash_key(url_psa));

   // both hash_key methods must yield the same hash as get_hash for empty search args
   ASSERT_EQ(unode_p.get_hash(), unode_t::hash_key(urlpath, string_t()));
   ASSERT_EQ(unode_p.get_hash(), unode_t::hash_key(url_p));

   // same as above, but compare hash_key methods directly
   ASSERT_EQ(unode_t::hash_key(url_psa), unode_t::hash_key(urlpath, srchargs));
   ASSERT_EQ(unode_t::hash_key(url_p), unode_t::hash_key(urlpath, string_t()));
}

}

#include "../hashtab_tmpl.cpp"
