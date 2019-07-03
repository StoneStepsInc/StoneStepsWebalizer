/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_ctnode.cpp
*/
#include "pchtest.h"

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

}

