/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_berkeleydb.cpp
*/
#include "pchtest.h"

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

      ///
      /// @brief  Opens `table` and populates it with sequential data.
      ///
      /// @note   The table must be registered with the database in `SetUp`.
      ///
      template <typename node_t, typename ... param_t>
      void PopulateTable(const char *table_name, berkeleydb_t::table_t& table, const char *key_prefix, uint64_t seq_id_first, uint64_t seq_id_last, param_t ... arg)
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
            node.count = i * 10;
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

   PopulateTable<anode_t>("agents", agents, "Agent ", 1, 100, false);

   // look up a few agent nodes with IDs we just inserted
   storable_t<anode_t> anode;

   anode.nodeid = 12;
   ASSERT_TRUE(agents.get_node_by_id(anode, nullptr, nullptr)) << "A look-up should find an agent 12";
   ASSERT_EQ(120, anode.count) << "Agent 12 should have 120 hits";
   ASSERT_STREQ("Agent 12", anode.string) << "Agent 12 name should match";

   anode.reset();

   anode.nodeid = 75;
   ASSERT_TRUE(agents.get_node_by_id(anode, nullptr, nullptr)) << "A look-up should find an agent 75";
   ASSERT_EQ(750, anode.count) << "Agent 75 should have 750 hits";
   ASSERT_STREQ("Agent 75", anode.string) << "Agent 75 name should match";

   anode.reset();

   anode.string = "Agent 45";
   ASSERT_TRUE(agents.get_node_by_value(anode, nullptr, nullptr)) << "A look-up should find an agent 45";
   ASSERT_EQ(450, anode.count) << "Agent 45 should have 450 hits";
   ASSERT_STREQ("Agent 45", anode.string) << "Agent 45 name should match";

   anode.reset();

   anode.string = "Agent 82";
   ASSERT_TRUE(agents.get_node_by_value(anode, nullptr, nullptr)) << "A look-up should find an agent 82";
   ASSERT_EQ(820, anode.count) << "Agent 82 should have 450 hits";
   ASSERT_STREQ("Agent 82", anode.string) << "Agent 82 name should match";
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

   PopulateTable<hnode_t>("hosts", hosts, "Host ", 200, 400);

   // look up a few agent nodes with IDs we just inserted
   storable_t<hnode_t> hnode;

   hnode.nodeid = 250;
   ASSERT_TRUE(hosts.get_node_by_id(hnode, nullptr, nullptr)) << "A look-up should find a host 250";
   ASSERT_EQ(2500, hnode.count) << "Host 250 should have 2500 hits";
   ASSERT_STREQ("Host 250", hnode.string) << "Host 250 name should match";

   hnode.reset();

   hnode.nodeid = 375;
   ASSERT_TRUE(hosts.get_node_by_id(hnode, nullptr, nullptr)) << "A look-up should find a host 375";
   ASSERT_EQ(3750, hnode.count) << "Host 375 should have 3750 hits";
   ASSERT_STREQ("Host 375", hnode.string) << "Host 375 name should match";

   hnode.reset();

   hnode.string = "Host 235";
   ASSERT_TRUE(hosts.get_node_by_value(hnode, nullptr, nullptr)) << "A look-up should find a host 235";
   ASSERT_EQ(2350, hnode.count) << "Host 235 should have 3750 hits";
   ASSERT_STREQ("Host 235", hnode.string) << "Host 235 name should match";

   hnode.reset();

   hnode.string = "Host 353";
   ASSERT_TRUE(hosts.get_node_by_value(hnode, nullptr, nullptr)) << "A look-up should find a host 353";
   ASSERT_EQ(3530, hnode.count) << "Host 353 should have 3530 hits";
   ASSERT_STREQ("Host 353", hnode.string) << "Host 353 name should match";
}

}

#include "../berkeleydb_tmpl.cpp"
