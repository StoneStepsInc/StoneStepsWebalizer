/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_linklist.cpp
*/
#include "pchtest.h"

#include "../linklist.h"

namespace sswtest {
class LinkedListTest : public testing::Test {
   protected:
      void FillNList(nlist& list)
      {
         list.add_nlist("one");
         list.add_nlist("two");
         list.add_nlist("start*");
         list.add_nlist("*end");
      }
};

///
/// @brief  nlist Substring Search
///
TEST_F(LinkedListTest, NListSubstringSearch)
{
   nlist list;

   FillNList(list);

   // match a substring case-sensitively
   EXPECT_STREQ("one", list.isinlist(string_t("onetwothree"), false)->c_str()) << "'onetwothree' should match 'one' as a substring, case-sensitively)";
   EXPECT_STREQ("two", list.isinlist(string_t("two"), false)->c_str()) << "'two' should match 'two' as a substring, case-sensitively)";

   EXPECT_EQ(nullptr, list.isinlist(string_t("ONETWOTHREE"), false)) << "'ONETWOTHREE' should not match 'one' as a substring, case-sensitively";
   EXPECT_EQ(nullptr, list.isinlist(string_t("TWO"), false)) << "'TWO' should not match 'two' as a subsetring, case-sensitively";

   // match a substring case-insensitively
   EXPECT_STREQ("one", list.isinlist(string_t("ONETWOTHREE"), true)->c_str()) << "'ONETWOTHREE' should match 'one' as a substring. case-insensitively";
   EXPECT_STREQ("two", list.isinlist(string_t("fourTWOthree"), true)->c_str()) << "'fourTWOthree' should match 'two' as a substring, case-insensitively";

   EXPECT_EQ(nullptr, list.isinlist(string_t("four"), false)) << "'four' should not match any of the list entries as a substring, case-insensitively";
}

///
/// @brief  nlist Word Search
///
TEST_F(LinkedListTest, NListWordSearch)
{
   nlist list;

   FillNList(list);

   // match a word case-sensitively
   EXPECT_EQ(nullptr, list.isinlistex("onetwothree", 11, false, false)) << "'onetwothree' should not match 'one' as a word, case-sensitively";
   EXPECT_STREQ("two", list.isinlist(string_t("two"), false)->c_str()) << "'two' should match 'two' as a word, case-sensitively";

   EXPECT_EQ(nullptr, list.isinlistex("ONE", 3, false, false)) << "'ONE' should not match 'one' as a word, case-sensitively";
   EXPECT_EQ(nullptr, list.isinlistex("TWO", 3, false, false)) << "'TWO' should not match 'two' as a word, case-sensitively";

   // match a word case-insensitively
   EXPECT_EQ(nullptr, list.isinlistex("ONETWOTHREE", 11, false, true)) << "'ONETWOTHREE' should not match 'one' as a word, case-insensitively";
   EXPECT_STREQ("two", list.isinlistex("TWO", 3, false, true)->c_str()) << "'TWO' should match 'two' as a word, case-insensitively";

   EXPECT_EQ(nullptr, list.isinlistex("four", 4, false, false)) << "'four' should not match any of the list entries as a word, case-insensitively";
}

///
/// @brief  nlist Prefix Search
///

TEST_F(LinkedListTest, NListPrefixSearch)
{
   const string_t *result = nullptr;
   nlist list;

   FillNList(list);

   // case-sensitive prefix search
   EXPECT_NE(nullptr, result = list.isinlist(string_t("start your engines"), false));
   EXPECT_STREQ("start*", result->c_str()) << "A string should match one of the prefix list entries case-sensitively";

   EXPECT_NE(nullptr, result = list.isinlist(string_t("start"), false));
   EXPECT_STREQ("start*", result->c_str()) << "A string that is equal the prefix should match one of the prefix list entries case-sensitively";

   // case-insensitive prefix search
   EXPECT_NE(nullptr, result = list.isinlist(string_t("START YOUR ENGINES"), true));
   EXPECT_STREQ("start*", result->c_str()) << "A string should match one of the prefix list entries case-insensitively";

   EXPECT_NE(nullptr, result = list.isinlist(string_t("START"), true));
   EXPECT_STREQ("start*", result->c_str()) << "A string that is equal the prefix should match one of the prefix list entries case-insensitively";

   // non-existing strings should't match any prefix entries
   EXPECT_EQ(nullptr, list.isinlist(string_t("begin your search"), false)) << "A non-existing string shouldn't match one of the list entries case-sensitively";
   EXPECT_EQ(nullptr, list.isinlist(string_t("we start our engines"), false)) << "A non-existing string shouldn't match one of the list entries case-sensitively";
}

///
/// @brief  nlist Suffix Search
///

TEST_F(LinkedListTest, NListSuffixSearch)
{
   const string_t *result = nullptr;
   nlist list;

   FillNList(list);

   // case-sensitive suffix search
   EXPECT_NE(nullptr, result = list.isinlist(string_t("the end"), false));
   EXPECT_STREQ("*end", result->c_str()) << "A string should match one of the suffix list entries case-sensitively";

   EXPECT_NE(nullptr, result = list.isinlist(string_t("end"), false));
   EXPECT_STREQ("*end", result->c_str()) << "A string that is equal the suffix should match one of the suffix list entries case-sensitively";

   // case-insensitive suffix search
   EXPECT_NE(nullptr, result = list.isinlist(string_t("THE END"), true));
   EXPECT_STREQ("*end", result->c_str()) << "A string should match one of the suffix list entries case-insensitively";

   EXPECT_NE(nullptr, result = list.isinlist(string_t("END"), true));
   EXPECT_STREQ("*end", result->c_str()) << "A string that is equal the suffix should match one of the suffix list entries case-insensitively";

   // non-existing strings should't match any suffix entries
   EXPECT_EQ(nullptr, list.isinlist(string_t("begin your search"), false)) << "A non-existing string shouldn't match one of the list entries case-sensitively";
   EXPECT_EQ(nullptr, list.isinlist(string_t("we end our journey"), false)) << "A non-existing string shouldn't match one of the list entries case-sensitively";
}

///
/// @brief  glist Config Include
///

TEST_F(LinkedListTest, GListConfigInclude)
{
   int included = 0;
   glist list;

   list.set_enable_phrase_values(true);

   // simulate config include files
   list.add_glist("/path-1/to/config/file-a\thost-a");
   list.add_glist("/path-2/to/config/file-b\thost-b");
   list.add_glist("/path-3/to/config/file-c\thost-c");
   list.add_glist("/path-4/to/config/file-b\thost-b");

   // the lambda should be called twice for file-b
   list.for_each("host-b", [] (const char *val, void *included) {if(strstr(val, "/file-b")) (*(int*)included)++;}, &included);

   // make sure we had two calls and two matche
   EXPECT_EQ(2, included) << "Two config files for host-b are expected";
}

///
/// @brief  glist Duplicate Values
///

TEST_F(LinkedListTest, GListDuplicateValues)
{
   const char *names[] = {"name-3", "name-4", "name-5", nullptr};
   int nameidx = 0;
   const gnode_t *node = nullptr;
   glist list;

   list.set_enable_phrase_values(true);

   // insert tree duplicate group values with different names
   list.add_glist("value-a\tname-1");
   list.add_glist("value-b\tname-2");
   list.add_glist("value-c\tname-3");
   list.add_glist("value-c\tname-4");
   list.add_glist("value-c\tname-5");
   list.add_glist("value-d\tname-6");
   list.add_glist("value-e\tname-7");
   list.add_glist("value-f\tname-8");

   glist::const_iterator iter = list.begin();
         
   // iterate over duplicate entries and make sure we got the right ones
   while((node = list.find_node(string_t("value-c"), iter, (node != nullptr), false)) != nullptr) {
      EXPECT_NE(nullptr, names[nameidx]) << "Should not reach the NULL entry of the expected names array";
      EXPECT_STREQ(names[nameidx], node->name.c_str()) << "A name for a duplicate entry should match the extected name";

      nameidx++;
   }

   // only three entries should be found
   EXPECT_EQ(3, nameidx) << "The number of iterations should match the number of entries in the names array";
}

///
/// @brief  glist Duplicate Search Engine Values
///

TEST_F(LinkedListTest, GListDuplicateSearchValues)
{
   const char *names[] = {"name-3=", "name-4=", "name-5=", nullptr};
   const char *qualifiers[] = {"three", "four", "five", nullptr};
   int nameidx = 0;
   const gnode_t *node = nullptr;
   glist list;

   list.set_enable_phrase_values(true);

   // insert tree duplicate group values with search qualifiers
   list.add_glist("value-a\tname-1=one", true);
   list.add_glist("value-b\tname-2=two", true);
   list.add_glist("value-c\tname-3=three", true);
   list.add_glist("value-c\tname-4=four", true);
   list.add_glist("value-c\tname-5=five", true);
   list.add_glist("value-d\tname-6=six", true);
   list.add_glist("value-e\tname-7=seven", true);
   list.add_glist("value-f\tname-8=eight", true);

   glist::const_iterator iter = list.begin();
         
   // iterate over duplicate entries and make sure we got the right ones
   while((node = list.find_node(string_t("value-c"), iter, (node != nullptr), false)) != nullptr) {
      EXPECT_NE(nullptr, names[nameidx]) << "Should not reach the NULL entry of the expected names array";
      EXPECT_STREQ(names[nameidx], node->name.c_str()) << "A name for a duplicate entry should match the extected name";
      EXPECT_STREQ(qualifiers[nameidx], node->qualifier.c_str()) << "A search qualifier for a duplicate entry should match the extected qualifier";

      nameidx++;
   }

   // only three entries should be found
   EXPECT_EQ(3, nameidx) << "The number of iterations should match the number of entries in the names array";
}
}
