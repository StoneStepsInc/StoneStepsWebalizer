/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_hashtab.cpp
*/
#include "pchtest.h"

#include "../anode.h"
#include "../dlnode.h"
#include "../ccnode.h"
#include "../hnode.h"

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

   htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 1"), false), 1);

   EXPECT_NO_THROW(htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 2"), false), 1)) << "Same time stamp should not throw an exception when inserting";
   EXPECT_NO_THROW(htab.put_node(new storable_t<anode_t>(string_t::hold("Agent 3"), false), 10)) << "Same time stamp should not throw an exception when inserting";

   {
      // keep the node pointer, so we don't leak while throwing
      storable_t<anode_t> *anode = new storable_t<anode_t>(string_t::hold("Agent 4"), false);

      // do not release the pointer because it never is inserted
      EXPECT_THROW(htab.put_node(anode, 5), std::logic_error) << "Cannot insert a node with an earlier time stamp";
   }

   ASSERT_THROW(htab.find_node(string_t::hold("Agent 2"), OBJ_REG, 5), std::logic_error) << "Cannot insert a node with an earlier time stamp";

   EXPECT_NO_THROW(htab.find_node(string_t::hold("Agent 2"), OBJ_REG, 10)) << "A look-up for an existing key using the same time stamp should not throw an exception";
   EXPECT_NO_THROW(htab.find_node(string_t::hold("Agent 2"), OBJ_REG, 20)) << "A look-up for an existing key using a greater time stamp should not throw an exception";
}

///
/// @brief  Tests inserting storable objects with a single string key that are 
///         derived from `base_node`.
///
TEST(HashTableTest, NodeInsertBaseNodeStorableWithKey)
{
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   for(int i = 0; i < 100; i++) {
      int tstamp = i - i % 2;
      std::string agent = "Agent " + std::to_string(tstamp);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      // use a non-const look-up with a time stamp 
      anode_t *anode = htab.find_node(agent_key, OBJ_REG, tstamp);

      // alternate finding and not finding the node in the hash table
      if(i % 2)
         EXPECT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Every odd agent number should be found in the hash table";
      else {
         ASSERT_EQ(nullptr, anode) << "Every even agent number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((anode = htab.put_node(new storable_t<anode_t>(agent_key, false), tstamp)));

         ASSERT_TRUE(anode != nullptr) << "A newly inserted node cannot be NULL";

         EXPECT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Newly inserted node must have the correct key";
      }

      // use a const version of a node look-up to test the insert without changing the hash table
      const anode_t *canode = htab.find_node(agent_key, OBJ_REG);

      ASSERT_TRUE(canode != nullptr) << "A newly inserted node cannot be NULL when looked up via the const interface";

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

   for(int i = 0; i < 100; i++) {
      int tstamp = i - i % 2;
      std::string ccode = "cc" + std::to_string(tstamp);
      string_t ccode_key(string_t::hold(ccode.c_str(), ccode.length()));

      // use a non-const look-up with a time stamp 
      ccnode_t *ccnode = htab.find_node(ccode_key, OBJ_REG, tstamp);

      // alternate finding and not finding the node in the hash table
      if(i % 2)
         EXPECT_STREQ(ccode_key.c_str(), ccnode->ccode.c_str()) << "Every odd number should be found in the hash table";
      else {
         ASSERT_EQ(nullptr, ccnode) << "Every even number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((ccnode = htab.put_node(new storable_t<ccnode_t>(ccode_key, ("Country " + std::to_string(tstamp)).c_str()), tstamp)));

         ASSERT_TRUE(ccnode != nullptr) << "A newly inserted node cannot be NULL";

         EXPECT_STREQ(ccode_key.c_str(), ccnode->ccode.c_str()) << "Newly inserted node must have the correct key";
      }

      // use a const version of a node look-up to test the insert without changing the hash table
      const ccnode_t *cccnode = htab.find_node(ccode_key, OBJ_REG);

      ASSERT_TRUE(cccnode != nullptr) << "A newly inserted node cannot be NULL when looked up via the const interface";

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

      hnode_t *host = hosts.insert(hosts.end(), std::make_unique<hnode_t>(string_t::hold(ipaddr.c_str(), ipaddr.length())))->get();

      // compound download job key
      dlnode_t::param_block dlnode_key = {download.c_str(), ipaddr.c_str()};

      uint64_t hashval = dlnode_t::hash(string_t::hold(ipaddr.c_str()), string_t::hold(download.c_str()));

      // use a non-const look-up with a time stamp 
      storable_t<dlnode_t> *dlnode = htab.find_node(hashval, &dlnode_key, OBJ_REG, tstamp);

      // alternate finding and not finding the node in the hash table
      if(i % 2) {
         EXPECT_STREQ(string_t::hold(dlnode_key.name), dlnode->string.c_str()) << "Every odd number should be found in the hash table (name)";
         EXPECT_STREQ(string_t::hold(dlnode_key.ipaddr), dlnode->hnode->string.c_str()) << "Every odd number should be found in the hash table (IP address)";
      }
      else {
         ASSERT_EQ(nullptr, dlnode) << "Every even number should not be found in the hash table";

         // insert a new node and test equal and greater-than time stamps
         ASSERT_NO_THROW((dlnode = htab.put_node(new storable_t<dlnode_t>(string_t::hold(dlnode_key.name), host), tstamp)));

         ASSERT_TRUE(dlnode != nullptr) << "A newly inserted node cannot be NULL";

         EXPECT_STREQ(string_t::hold(dlnode_key.name), dlnode->string.c_str()) << "Newly inserted node must have the correct key";
         EXPECT_STREQ(string_t::hold(dlnode_key.ipaddr), dlnode->hnode->string.c_str()) << "Newly inserted node must have the correct key";
      }

      // there is no const interface for compound key nodes
      dlnode = htab.find_node(hashval, &dlnode_key, OBJ_REG, tstamp);

      ASSERT_TRUE(dlnode != nullptr) << "A newly inserted node cannot be NULL";

      EXPECT_STREQ(string_t::hold(dlnode_key.name), dlnode->string.c_str()) << "Newly inserted node must have the correct key";
      EXPECT_STREQ(string_t::hold(dlnode_key.ipaddr), dlnode->hnode->string.c_str()) << "Newly inserted node must have the correct key";
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

      ASSERT_NO_THROW(htab.put_node(new storable_t<anode_t>(agent_key, false), tstamp));
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

      ASSERT_NO_THROW(htab.put_node(new storable_t<anode_t>(agent_key, false), tstamp));

      // every 10 iterations insert a group entry with a zero time stamp, which should be ignored
      if(i % 10 == 0) {
         std::string agent_group = "Agent Group " + std::to_string(i);

         storable_t<anode_t> *anode_group = new storable_t<anode_t>(string_t::hold(agent_group.c_str(), agent_group.length()), false);
         anode_group->flag = OBJ_GRP;

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

      ASSERT_EQ(nullptr, htab.find_node(string_t::hold(agent.c_str(), agent.length()), OBJ_REG)) << "Swapped out keys should not be found";
   }

   // and all remaining regular and group keys are still there
   for(int i = 140; i < 200; i++) {
      std::string agent = "Agent " + std::to_string(i);

      ASSERT_TRUE(htab.find_node(string_t::hold(agent.c_str(), agent.length()), OBJ_REG) != nullptr) << "We sould be able to find every remaining regular object key";

      if(i % 10 == 0) {
         std::string agent_group = "Agent Group " + std::to_string(i);

         ASSERT_TRUE(htab.find_node(string_t::hold(agent_group.c_str(), agent_group.length()), OBJ_GRP) != nullptr) << "We sould be able to find every group object key";
      }
   }

   // make sure swapping out nodes didn't leave any dangling pointers
   ASSERT_NO_THROW(htab.clear());
}

///
/// @brief  Tests the hash table loading an array of object pointers.
///
TEST(HashTableTest, LoadAgentNodeArray)
{
   std::set<std::string> agent_keys;
   hash_table<storable_t<anode_t>> htab(10);

   // no look-ups are done because keys are guaranteed to be unique
   for(int i = 100; i < 200; i++) {
      int tstamp = i - i % 2;
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      ASSERT_NO_THROW(htab.put_node(new storable_t<anode_t>(agent_key, false), tstamp));

      agent_keys.insert(agent);

      // every 10 iterations insert a group entry with a zero time stamp, which should be ignored
      if(i % 10 == 0) {
         std::string agent_group = "Agent Group " + std::to_string(i);

         storable_t<anode_t> *anode_group = new storable_t<anode_t>(string_t::hold(agent_group.c_str(), agent_group.length()), false);
         anode_group->flag = OBJ_GRP;

         // pass a zero time stamp because group nodes do not participate in time stamp ordering
         ASSERT_NO_THROW(htab.put_node(anode_group, 0));

         agent_keys.insert(agent_group);
      }
   }

   // make sure we have the right number of elements before loading them into an array
   ASSERT_EQ(110, htab.size()) << "Hash table should contain 100 regular nodes and 10 group nodes";

   // define an array that is one element larger than we need to check overruns
   const anode_t *anode[111] = {};

   ASSERT_EQ(110, htab.load_array(anode)) << "All 100 regular nodes and 10 groups must appear in the array of node pointers";

   ASSERT_EQ(nullptr, anode[110]) << "The last element in the array must remain NULL";

   // keys in the array are in unspecified order
   for(size_t i = 0; i < 110; i++) {
      ASSERT_TRUE(agent_keys.find(anode[i]->string.c_str()) != agent_keys.end()) << "A key in the arraymust exist in the agent key set";

      // erase the key to ensure hash table keys are unique
      agent_keys.erase(anode[i]->string.c_str());
   }

   EXPECT_EQ(0, agent_keys.size()) << "Number of keys loaded in the array must match the number of inserted nodes";
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

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, false), 0)));
   }

   // nodes that are looked up are moved to the beginning of the list - make sure we can still find them
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      anode_t *anode;

      ASSERT_NO_THROW((anode = htab.find_node(agent_key, OBJ_REG, 0)));

      ASSERT_TRUE(anode != nullptr) << "A node must exist in the hash table";

      ASSERT_STREQ(agent_key.c_str(), anode->string.c_str()) << "Every node must be found in the hash table";
   }

   // look up the same nodes again to make sure any node movements within lists work properly
   for(int i = 0; i < 100; i++) {
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

      anode_t *anode;

      ASSERT_NO_THROW((anode = htab.find_node(agent_key, OBJ_REG, 0)));

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

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, false), 0)));

      if(i % 10 == 0) {
         std::string agent = "Agent Group " + std::to_string(i);
         string_t agent_key(string_t::hold(agent.c_str(), agent.length()));

         storable_t<anode_t> *agent_grp = new storable_t<anode_t>(agent_key, false);
         agent_grp->flag = OBJ_GRP;

         agent_node_names.insert(agent_key);

         ASSERT_NO_THROW((htab.put_node(agent_grp, 0)));
      }
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      if(iter.item()->flag == OBJ_REG)
         ASSERT_TRUE(strncmp("Agent ", iter.item()->string, 6) == 0);
      else
         ASSERT_TRUE(strncmp("Agent Group ", iter.item()->string, 12) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL from item() after the entire sequence has been traversed";

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

      ASSERT_NO_THROW((htab.put_node(new storable_t<anode_t>(agent_key, false), 0)));
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      ASSERT_EQ(OBJ_REG, iter.item()->flag) << "This hash map must only contain regular objects";

      ASSERT_TRUE(strncmp("Agent ", iter.item()->string, 6) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL from item() after the entire sequence has been traversed";

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

      storable_t<anode_t> *agent_grp = new storable_t<anode_t>(agent_key, false);
      agent_grp->flag = OBJ_GRP;

      agent_node_names.insert(agent_key);

      ASSERT_NO_THROW((htab.put_node(agent_grp, 0)));
   }

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL before item() is called for the first time";

   while(iter.next()) {
      std::set<string_t>::iterator agent_name = agent_node_names.find(iter.item()->string);

      ASSERT_TRUE(agent_name != agent_node_names.end()) << "Agent node name must be found in the name set";

      ASSERT_EQ(OBJ_GRP, iter.item()->flag) << "This hash map must only contain group objects";

      ASSERT_TRUE(strncmp("Agent Group ", iter.item()->string, 12) == 0);

      agent_node_names.erase(agent_name);
   }

   ASSERT_EQ(nullptr, iter.item()) << "The iterator should return NULL from item() after the entire sequence has been traversed";

   EXPECT_EQ(0, agent_node_names.size()) << "All agent node names must be iterated over";
}

///
/// @brief  Tests a hash table iterator going over an empty hash table.
///
TEST(HashTableTest, IteratorEmpty)
{
   hash_table<storable_t<anode_t>> htab(10);  // use 10 buckets to test bucket lists

   hash_table<storable_t<anode_t>>::iterator iter = htab.begin();

   ASSERT_EQ(nullptr, iter.next()) << "Next node of an empty hash map iterator should return NULL";
   ASSERT_EQ(nullptr, iter.item()) << "Current node of an empty hash map iterator should be NULL";
}

}
