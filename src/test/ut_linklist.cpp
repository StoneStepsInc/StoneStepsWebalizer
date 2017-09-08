/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_linklist.cpp
*/
#include "pchtest.h"

#include "../linklist.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

namespace sswtest {
TEST_CLASS(LinkedList) {
   private:
      void FillNList(nlist& list)
      {
         list.add_nlist("one");
         list.add_nlist("two");
         list.add_nlist("start*");
         list.add_nlist("*end");
      }

   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(NListSubstringSearch)
         TEST_DESCRIPTION(L"nlist Substring Search")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NListSubstringSearch)
      {
         nlist list;

         FillNList(list);

         // match a substring case-sensitively
         mstest::Assert::AreEqual("one", list.isinlist(string_t("onetwothree"), false)->c_str(), L"'onetwothree' should match 'one' as a substring, case-sensitively)");
         mstest::Assert::AreEqual("two", list.isinlist(string_t("two"), false)->c_str(), L"'two' should match 'two' as a substring, case-sensitively)");

         mstest::Assert::IsNull(list.isinlist(string_t("ONETWOTHREE"), false), L"'ONETWOTHREE' should not match 'one' as a substring, case-sensitively");
         mstest::Assert::IsNull(list.isinlist(string_t("TWO"), false), L"'TWO' should not match 'two' as a subsetring, case-sensitively");

         // match a substring case-insensitively
         mstest::Assert::AreEqual("one", list.isinlist(string_t("ONETWOTHREE"), true)->c_str(), L"'ONETWOTHREE' should match 'one' as a substring. case-insensitively");
         mstest::Assert::AreEqual("two", list.isinlist(string_t("fourTWOthree"), true)->c_str(), L"'fourTWOthree' should match 'two' as a substring, case-insensitively");

         mstest::Assert::IsNull(list.isinlist(string_t("four"), false), L"'four' should not match any of the list entries as a substring, case-insensitively");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NListWordSearch)
         TEST_DESCRIPTION(L"nlist Word Search")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NListWordSearch)
      {
         nlist list;

         FillNList(list);

         // match a word case-sensitively
         mstest::Assert::IsNull(list.isinlistex("onetwothree", 11, false, false), L"'onetwothree' should not match 'one' as a word, case-sensitively");
         mstest::Assert::AreEqual("two", list.isinlist(string_t("two"), false)->c_str(), L"'two' should match 'two' as a word, case-sensitively");

         mstest::Assert::IsNull(list.isinlistex("ONE", 3, false, false), L"'ONE' should not match 'one' as a word, case-sensitively");
         mstest::Assert::IsNull(list.isinlistex("TWO", 3, false, false), L"'TWO' should not match 'two' as a word, case-sensitively");

         // match a word case-insensitively
         mstest::Assert::IsNull(list.isinlistex("ONETWOTHREE", 11, false, true), L"'ONETWOTHREE' should not match 'one' as a word, case-insensitively");
         mstest::Assert::AreEqual("two", list.isinlistex("TWO", 3, false, true)->c_str(), L"'TWO' should match 'two' as a word, case-insensitively");

         mstest::Assert::IsNull(list.isinlistex("four", 4, false, false), L"'four' should not match any of the list entries as a word, case-insensitively");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NListPrefixSearch)
         TEST_DESCRIPTION(L"nlist Prefix Search")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NListPrefixSearch)
      {
         nlist list;

         FillNList(list);

         // case-sensitive prefix search
         mstest::Assert::AreEqual("start*", list.isinlist(string_t("start your engines"), false)->c_str(), L"A string should match one of the prefix list entries case-sensitively");

         // case-insensitive prefix search
         mstest::Assert::AreEqual("start*", list.isinlist(string_t("START YOUR ENGINES"), true)->c_str(), L"A string should match one of the prefix list entries case-insensitively");

         // non-existing strings should't match any prefix entries
         mstest::Assert::IsNull(list.isinlist(string_t("begin your search"), false), L"A non-existing string shouldn't match one of the list entries case-sensitively");
         mstest::Assert::IsNull(list.isinlist(string_t("we start our engines"), false), L"A non-existing string shouldn't match one of the list entries case-sensitively");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NListSuffixSearch)
         TEST_DESCRIPTION(L"nlist Suffix Search")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NListSuffixSearch)
      {
         nlist list;

         FillNList(list);

         // case-sensitive prefix search
         mstest::Assert::AreEqual("*end", list.isinlist(string_t("the end"), false)->c_str(), L"A string should match one of the suffix list entries case-sensitively");

         // case-insensitive prefix search
         mstest::Assert::AreEqual("*end", list.isinlist(string_t("THE END"), true)->c_str(), L"A string should match one of the suffix list entries case-insensitively");

         // non-existing strings should't match any prefix entries
         mstest::Assert::IsNull(list.isinlist(string_t("begin your search"), false), L"A non-existing string shouldn't match one of the list entries case-sensitively");
         mstest::Assert::IsNull(list.isinlist(string_t("we end our journey"), false), L"A non-existing string shouldn't match one of the list entries case-sensitively");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(GListConfigInclude)
         TEST_DESCRIPTION(L"glist Config Include")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(GListConfigInclude)
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
         mstest::Assert::AreEqual(2, included, L"Two config files for host-b are expected");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(GListDuplicateValues)
         TEST_DESCRIPTION(L"glist Duplicate Values")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(GListDuplicateValues)
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

         glist::iterator iter = list.begin();
         
         // iterate over duplicate entries and make sure we got the right ones
         while((node = list.find_node(string_t("value-c"), iter, (node != nullptr), false)) != nullptr) {
            mstest::Assert::IsNotNull(names[nameidx], L"Should not reach the NULL entry of the expected names array");
            mstest::Assert::AreEqual(names[nameidx], node->name.c_str(), L"A name for a duplicate entry should match the extected name");

            nameidx++;
         }

         // only three entries should be found
         mstest::Assert::AreEqual(3, nameidx, L"The number of iterations should match the number of entries in the names array");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(GListDuplicateSearchValues)
         TEST_DESCRIPTION(L"glist Duplicate Search Enngine Values")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Linked List")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(GListDuplicateSearchValues)
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

         glist::iterator iter = list.begin();
         
         // iterate over duplicate entries and make sure we got the right ones
         while((node = list.find_node(string_t("value-c"), iter, (node != nullptr), false)) != nullptr) {
            mstest::Assert::IsNotNull(names[nameidx], L"Should not reach the NULL entry of the expected names array");
            mstest::Assert::AreEqual(names[nameidx], node->name.c_str(), L"A name for a duplicate entry should match the extected name");
            mstest::Assert::AreEqual(qualifiers[nameidx], node->qualifier.c_str(), L"A search qualifier for a duplicate entry should match the extected qualifier");

            nameidx++;
         }

         // only three entries should be found
         mstest::Assert::AreEqual(3, nameidx, L"The number of iterations should match the number of entries in the names array");
      }
};
}
