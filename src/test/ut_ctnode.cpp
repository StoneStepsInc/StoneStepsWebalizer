/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_ctnode.cpp
*/
#include "pch.h"

#include "../ctnode.h"
#include "../tstring.h"

namespace sswtest {

///
/// @brief  Tests whether any combinations of the populated state for GeoName ID,
///         city name and country code produce invalid states.
///
/// Only the populated state is being testsed, such as whether GeoName ID is zero
/// or not or if the country code is empty or not, not their values.
///
TEST(CityNode, UsableCityInfo)
{
   //
   // Usable cases
   //
   EXPECT_TRUE(ctnode_t::is_usable_city(0, string_t(), string_t())) << "Usable: zero GeoName ID, empty city name, empty country code";

   EXPECT_TRUE(ctnode_t::is_usable_city(123, string_t::hold("city"), string_t::hold("ccode"))) << "Usable: non-zero GeoName ID, non-empty city name, non-empty country code";

   EXPECT_TRUE(ctnode_t::is_usable_city(0, string_t(), string_t::hold("ccode"))) << "Usable: zero GeoName ID, empty city name, non-empty country code";

   //
   // Unusable cases
   //
   EXPECT_FALSE(ctnode_t::is_usable_city(0, string_t::hold("city"), string_t::hold("ccode"))) << "Not usable: zero GeoName ID, non-empty city name, non-empty country code";

   EXPECT_FALSE(ctnode_t::is_usable_city(0, string_t::hold("city"), string_t())) << "Not usable: zero GeoName ID, non-empty city name, empty country code";

   EXPECT_FALSE(ctnode_t::is_usable_city(123, string_t::hold("city"), string_t())) << "Not usable: non-zero GeoName ID, non-empty city name, empty country code";

   EXPECT_FALSE(ctnode_t::is_usable_city(123, string_t(), string_t::hold("ccode"))) << "Not usable: non-zero GeoName ID, empty city name, non-empty country code";

   EXPECT_FALSE(ctnode_t::is_usable_city(123, string_t(), string_t())) << "Not usable: non-zero GeoName ID, empty city name, empty country code";
}

///
/// @brief  Tests compact node identifiers for city nodes.
///
TEST(CityNode, CompactNodeId)
{
   // check first all technically possible country codes
   for(char c1 = 'a'; c1 <= 'z'; c1++) {
      for(char c2 = 'a'; c2 <= 'z'; c2++) {
         char ccode[3] = {c1, c2, '\x0'};
         ctnode_t ctnode(0x12345678, string_t(), string_t::hold(ccode));
         ASSERT_EQ((((uint64_t) (c1 - 'a' + 1) << (32 + 5)) + ((uint64_t) (c2 - 'a' + 1) << 32) + 0x12345678u), ctnode.compact_nodeid());
      }
   }

   // test a special country code for unknown countries
   ctnode_t ctnode(0x12345678, string_t(), string_t::hold("*"));
   ASSERT_EQ(0x12345678, ctnode.compact_nodeid());
}

}

