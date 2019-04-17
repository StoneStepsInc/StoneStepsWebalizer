/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_berkeleydb.cpp
*/
#include "pchtest.h"

#include "../berkeleydb.h"
#include "../anode.h"

#include <memory>

namespace sswtest {

class test_config_t : public berkeleydb_t::config_t {
   private:
      const string_t empty_path;

   public:
      test_config_t(void) {}

      const test_config_t& clone(void) const {return *new test_config_t();}

      void release(void) const {delete this;}

      const string_t& get_db_path(void) const {return empty_path;}

      const string_t& get_tmp_path(void) const {return empty_path;}

      uint32_t get_db_cache_size(void) const {return 0;}

      uint32_t get_db_seq_cache_size(void) const {return 0;}

      bool get_db_direct(void) const {return false;}
};

TEST(BerkeleyDBTest, OpenDatabase)
{
   test_config_t test_config;
   berkeleydb_t bdb(std::move(test_config));

   berkeleydb_t::table_t table1(bdb.make_table());
   berkeleydb_t::table_t table2(bdb.make_table());

   berkeleydb_t::status_t status;

   ASSERT_NO_THROW((status = bdb.open({&table1, &table2}))) << "A database with two tables should open without throwing any exception";
   ASSERT_EQ(0, status.err_num()) << "A database with two tables should open without an error";

   ASSERT_NO_THROW((status = bdb.close())) << "A database should close without throwing an exception";
   ASSERT_EQ(0, status.err_num()) << "A database should close without an error";
}

TEST(BerkeleyDBTest, StoreAgentNodes)
{
   test_config_t test_config;
   berkeleydb_t bdb(std::move(test_config));

   berkeleydb_t::table_t agents(bdb.make_table());

   berkeleydb_t::status_t status;

   ASSERT_NO_THROW((status = bdb.open({&agents}))) << "A database should open without throwing any exception";
   ASSERT_EQ(0, status.err_num()) << "A database should open without an error";

   // open a simple table - no values table of a sequence database
   ASSERT_NO_THROW((status = agents.open("agents", &bt_compare_cb<anode_t::s_compare_key>))) << "A database table should open without throwing an exception";
   ASSERT_EQ(0, status.err_num()) << "A database table should open without an error";

   for(uint64_t i = 1; i <= 100; i++) {
      // generate a few sequential agent records
      std::string agent = "Agent " + std::to_string(i);
      string_t agent_key(string_t::hold(agent.c_str(), agent.length()));
      storable_t<anode_t> anode(agent_key, false);

      // insert a new record with the loop counter node ID and a few data bits to verify later
      anode.nodeid = i;
      anode.count = i * 10;
      ASSERT_TRUE(agents.put_node<anode_t>(anode, anode.storage_info)) << "Agent node should be stored without an error";
   }

   // look up a few agent nodes with IDs we just inserted
   storable_t<anode_t> anode2;

   anode2.nodeid = 12;
   ASSERT_TRUE(agents.get_node_by_id(anode2, nullptr, nullptr)) << "A look-up should find an agent 12";
   ASSERT_EQ(120, anode2.count) << "Agent 12 should have 120 hits";
   ASSERT_STREQ("Agent 12", anode2.string) << "Agent 12 name should match";

   anode2.reset();

   anode2.nodeid = 75;
   ASSERT_TRUE(agents.get_node_by_id(anode2, nullptr, nullptr)) << "A look-up should find an agent 75";
   ASSERT_EQ(750, anode2.count) << "Agent 75 should have 750 hits";
   ASSERT_STREQ("Agent 75", anode2.string) << "Agent 75 name should match";

   // close the database
   ASSERT_NO_THROW((status = bdb.close())) << "A database should close without throwing an exception";
   ASSERT_EQ(0, status.err_num()) << "A database should close without an error";
}

}

#include "../berkeleydb_tmpl.cpp"
