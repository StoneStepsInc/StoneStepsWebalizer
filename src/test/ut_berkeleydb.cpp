/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_berkeleydb.cpp
*/
#include "pch.h"

#include "../berkeleydb.h"
#include "../anode.h"
#include "../hnode.h"

#include <memory>

namespace sswtest {

///
/// @brief  A test database configuration object for an in-memory database.
///
class test_config_t : public berkeleydb_t::config_t {
   private:
      const string_t empty_path;

   public:
      test_config_t(void) {}

      const test_config_t& clone(void) const override {return *new test_config_t();}

      void release(void) const override {delete this;}

      const string_t& get_db_path(void) const override {return empty_path;}

      const string_t& get_db_dir(void) const override {return empty_path;}

      const string_t& get_db_name(void) const override {return empty_path;}

      const string_t& get_tmp_path(void) const override {return empty_path;}

      uint32_t get_db_cache_size(void) const override {return 0;}

      uint32_t get_db_seq_cache_size(void) const override {return 0;}

      bool get_db_direct(void) const override {return false;}
};

///
/// @brief  A test fixture that creates an in-memory Berkeley DB database and
///         provides methods to populate its tables.
///
class BerkeleyDBTest : public ::testing::Test {
   protected:
      berkeleydb_t            bdb;

      berkeleydb_t::table_t   agents;
      berkeleydb_t::table_t   hosts;

   protected:
      BerkeleyDBTest(void) :
         bdb(test_config_t()),
         agents(bdb.make_table()),
         hosts(bdb.make_table())
      {
      }

      ~BerkeleyDBTest(void)
      {
      }

      /// Opens the database and registers all tables defined in this class with the database.
      void SetUp(void) override
      {
         berkeleydb_t::status_t status;

         ASSERT_NO_THROW((status = bdb.open({&agents, &hosts}))) << "A database should open without throwing any exception";
         ASSERT_EQ(0, status.err_num()) << "A database should open without an error";
      }

      /// Closes all registered tables and the database
      void TearDown(void) override
      {
         berkeleydb_t::status_t status;

         // close the database (will close all tables)
         ASSERT_NO_THROW((status = bdb.close())) << "A database should close without throwing an exception";
         ASSERT_EQ(0, status.err_num()) << "A database should close without an error";
      }

      /// Returns node hit count value as a node ID multiplied by ten.
      static uint64_t HitCountValueX10(uint64_t nodeid, uint64_t minid, uint64_t maxid) 
      {
         return nodeid * 10;
      }

      /// Returns node hit count value as a difference between the max node ID and `nodeid` multiplied by `10` and divided by `4`.
      static uint64_t HitCountValueDescX10Div4(uint64_t nodeid, uint64_t minid, uint64_t maxid) 
      {
         return ((maxid + minid - nodeid) * 10) / 4;
      }

      ///
      /// @brief  Opens `table` and populates it with sequential data.
      ///
      /// @tparam node_t   Node type to construct to populate the table.
      /// @tparam param_t  Parameyers for `node_t` constructor.
      ///
      /// @note   The table must be registered with the database in `SetUp`.
      ///
      /// All nodes will have their hit counts set to the value returned from `hit_count_value`.
      ///
      template <typename node_t, typename ... param_t>
      void PopulateTable(const char *table_name, berkeleydb_t::table_t& table, const char *key_prefix, uint64_t seq_id_first, uint64_t seq_id_last, uint64_t (*hit_count_value)(uint64_t, uint64_t, uint64_t), param_t ... arg)
      {
         berkeleydb_t::status_t status;

         // configure the sequence database
         ASSERT_NO_THROW((status = table.open_sequence(string_t(table_name) + ".seq", 10, (db_seq_t) seq_id_first))) << "Table sequence should open without throwing any exceptions";
         ASSERT_EQ(0, status.err_num()) << "Table sequence should open without an error";

         // configure the values database
         ASSERT_NO_THROW((status = table.associate(string_t(table_name) + ".values", 
                  &bt_compare_cb<node_t::s_compare_value_hash>, 
                  &bt_compare_cb<node_t::s_compare_value_hash>, 
                  &sc_extract_cb<node_t::s_field_value_hash>)).success()) << "Values table should open withou throwing any exceptions";
         ASSERT_EQ(0, status.err_num()) << "Values table should open without an error";

         table.set_values_db(string_t(table_name) + ".values");

         // open the primary data table
         ASSERT_NO_THROW((status = table.open(table_name, &bt_compare_cb<node_t::s_compare_key>))) << "A database table should open without throwing an exception";
         ASSERT_EQ(0, status.err_num()) << "A database table should open without an error";

         // fill the table with data
         for(uint64_t i = seq_id_first; i <= seq_id_last; i++) {
            // generate a few sequential agent records
            std::string key = key_prefix + std::to_string(i);
            string_t node_key(string_t::hold(key.c_str(), key.length()));
            storable_t<node_t> node(node_key, std::forward<param_t>(arg)...);

            // get the next sequence ID and advance the counter
            db_seq_t seq_id = table.get_seq_id();

            ASSERT_EQ(i, seq_id) << "Table sequence IDs should be sequential";

            // insert a new record with the loop counter node ID and a few data bits to verify later
            node.nodeid = i;
            node.count = hit_count_value(i, seq_id_first, seq_id_last);
            ASSERT_TRUE(table.put_node<node_t>(node, node.storage_info)) << "Data node should be stored without an error";
         }
      }
};

///
/// @brief  Stores a few agent nodes in a database and looks some of them up by
///         node ID and value.
///
TEST_F(BerkeleyDBTest, LookUpAgentNodes)
{
   berkeleydb_t::status_t status;

   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, HitCountValueX10, OBJ_REG, false);

   // look up agent nodes with IDs we just inserted
   storable_t<anode_t> anode;

   for(uint64_t i = 1; i <= 100; i++) {
      anode.nodeid = i;
      ASSERT_TRUE(agents.get_node_by_id(anode)) << "A look-up should find any agent from 1 to 100";
      ASSERT_EQ(i * 10, anode.count) << "Agent hit count should be a multiple of 10 of its node ID";
      ASSERT_STREQ(("Agent " + std::to_string(i)).c_str(), anode.string) << "Agent name should match its node ID";

      anode.reset();
   }
}

///
/// @brief  Stores a few host nodes in a database and looks some of them up by
///         node ID and value.
///
/// @note   Host names in this test do not resemble not IP addresses and just
///         are treated as strings.
///
TEST_F(BerkeleyDBTest, LookUpHostNodes)
{
   berkeleydb_t::status_t status;

   PopulateTable<hnode_t>("hosts", hosts, "Host ", 200, 400, HitCountValueX10, OBJ_REG);

   // look up a few agent nodes with IDs we just inserted
   storable_t<hnode_t> hnode;

   for(uint64_t i = 200; i <= 400; i++) {
      hnode.nodeid = i;
      ASSERT_TRUE(hosts.get_node_by_id(hnode)) << "A look-up should find any host from 200 to 400";
      ASSERT_EQ(i * 10, hnode.count) << "Host hit count should be a multiple of 10 of its node ID";
      ASSERT_STREQ(("Host " + std::to_string(i)).c_str(), hnode.string) << "Host name should match its node ID";

      hnode.reset();
   }
}

///
/// @brief  Populates a table and traverses its records by a secondary index
///         in ascending order.
///
TEST_F(BerkeleyDBTest, ForwardIndexTraversal)
{
   berkeleydb_t::status_t status;

   // create an index with an extract callback, so it will be associated immediately when the table is opened
   ASSERT_NO_THROW((status = agents.associate("agents.hits", 
               &bt_compare_cb<anode_t::s_compare_hits>, 
               &bt_compare_cb<anode_t::s_compare_key>, 
               &sc_extract_cb<anode_t::s_field_hits>))) << "A table index should associate without throwing any exceptions";
   ASSERT_EQ(0, status.err_num()) << "A table index should associate without an error";

   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, HitCountValueDescX10Div4, OBJ_REG, false);

   storable_t<anode_t> anode;

   // traverse all nodes in ascending order of hits (node IDs are in reverse order)
   berkeleydb_t::iterator<anode_t> iter = agents.begin<anode_t>("agents.hits");

   for(uint64_t i = 1; iter.next(anode); i++) {
      ASSERT_EQ((i * 10) / 4, anode.count) << "Agent hit count should be node ID * 10 / 4";
      ASSERT_STREQ(("Agent " + std::to_string(101 - i)).c_str(), anode.string) << "Agent name should match the node ID";

      anode.reset();
   }

   ASSERT_FALSE(iter.next(anode)) << "There should be no more than 100 secondary index entries";
}

///
/// @brief  Populates a table and traverses its records by a secondary index
///         in descending order.
///
TEST_F(BerkeleyDBTest, ReverseIndexTraversal)
{
   berkeleydb_t::status_t status;

   // create an index with an extract callback, so it will be associated immediately when the table is opened
   ASSERT_NO_THROW((status = agents.associate("agents.hits", 
               &bt_compare_cb<anode_t::s_compare_hits>, 
               &bt_compare_cb<anode_t::s_compare_key>, 
               &sc_extract_cb<anode_t::s_field_hits>))) << "A table index should associate without throwing any exceptions";
   ASSERT_EQ(0, status.err_num()) << "A table index should associate without an error";

   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, HitCountValueDescX10Div4, OBJ_REG, false);

   storable_t<anode_t> anode;

   // traverse all nodes in descending order of hits (node IDs are in reverse order)
   berkeleydb_t::reverse_iterator<anode_t> iter = agents.rbegin<anode_t>("agents.hits");

   for(uint64_t i = 100; iter.prev(anode); i--) {
      ASSERT_EQ((i * 10) / 4, anode.count) << "Agent hit count should be node ID * 10 / 4";
      ASSERT_STREQ(("Agent " + std::to_string(101 - i)).c_str(), anode.string) << "Agent name should match the node ID";

      anode.reset();
   }

   ASSERT_FALSE(iter.prev(anode)) << "There should be no more than 100 secondary index entries";
}

///
/// @brief  Populates a table without an index and then builds a new index
///         and traverses the table in ascending order.
///
TEST_F(BerkeleyDBTest, BuildNewIndex)
{
   berkeleydb_t::status_t status;

   // create an index but do not associate it with the table yet (node IDs for duplicate hit counts will be in reverse)
   ASSERT_NO_THROW((status = agents.associate("agents.hits", 
               &bt_compare_cb<anode_t::s_compare_hits>, 
               &bt_reverse_compare_cb<anode_t::s_compare_key>, 
               nullptr))) << "A table index should associate without throwing any exceptions";
   ASSERT_EQ(0, status.err_num()) << "A table index should associate without an error";

   // populate the table with data without an index and assign 25 distinct hit counts in the order reverse to node IDs
   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, HitCountValueDescX10Div4, OBJ_REG, false);

   // build a new index against the table we just populated
   ASSERT_NO_THROW((status = agents.associate("agents.hits", 
               &sc_extract_cb<anode_t::s_field_hits>, true))) << "A new index should be creates against an existing database without throwing any exceptions";
   ASSERT_EQ(0, status.err_num()) << "A new index should be creates against an existing database without an error";

   storable_t<anode_t> anode;

   // traverse all nodes in ascending order of hits
   berkeleydb_t::iterator<anode_t> iter = agents.begin<anode_t>("agents.hits");

   // hit counts will be increasing (2, 5, 7, ...) and node values will be sequential and in reverse order (`Agent 100`, Agent 99`, ...)
   for(uint64_t i = 1; iter.next(anode); i++) {
      ASSERT_EQ((i * 10) / 4, anode.count) << "Agent hit count should be a multiple of 10 of its node ID";
      ASSERT_STREQ(("Agent " + std::to_string(101 - i)).c_str(), anode.string) << "Agent name should match the node ID in reverse order";

      anode.reset();
   }

   ASSERT_FALSE(iter.next(anode)) << "There should be no more than 25 secondary index entries";
}

TEST_F(BerkeleyDBTest, AgentNodeReadCallback)
{
   berkeleydb_t::status_t status;

   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, HitCountValueX10, OBJ_REG, false);

   // use the void pointer to pass in a node ID, so we don't have to instantiate read templates just for this test
   auto read_agent_cb = [] (anode_t& anode, void *arg) -> void
   {
      uint64_t& nodeid = *(uint64_t*) arg;

      ASSERT_EQ(nodeid * 10, anode.count) << "Agent hit count should be a multiple of 10 of its node ID";
      ASSERT_STREQ(("Agent " + std::to_string(nodeid)).c_str(), anode.string) << "Agent name should match its node ID";
   };

   // look up agent nodes with IDs we just inserted
   storable_t<anode_t> anode;

   for(uint64_t i = 1; i <= 100; i++) {
      anode.nodeid = i;
      ASSERT_TRUE((agents.get_node_by_id<anode_t, void*>(anode, (anode_t::s_unpack_cb_t<void*>) read_agent_cb, &i))) << "A look-up should find any agent from 1 to 100";

      anode.reset();
   }
}

}

#include "../database_tmpl.cpp"
