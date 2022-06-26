/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_poolalloc.cpp
*/
#include "pch.h"

#include "../pool_allocator.h"
#include <list>
#include <vector>
#include <cstdint>

namespace sswtest {

///
/// @brief  Test structure for pool allocator tests.
///
struct X {
   int i;
   unsigned long ul;
};

///
/// @brief  Test the pool allocator with a linked list.
///
TEST(PoolAllocatorTests, LinkListTest)
{
   pool_allocator_t<X, 8> a;
   std::list<X, pool_allocator_t<X, 8>> l(a);

   // insert one list node
   l.push_back({123, 456ul});

   // hold onto the address of X within the node
   uintptr_t xp = (uintptr_t) &l.front();

   // return the node to the pool
   l.pop_back();

   // push a new node in an empty list
   l.push_back({456, 789ul});

   // check that the address of X within the node is the same (relies on the list allocating in the same order)
   EXPECT_EQ(xp, (uintptr_t) &l.front()) << "Existing memory block should be allocated from the pool";
}

///
/// @brief  Test the pool allocator with a vector.
///
TEST(PoolAllocatorTests, VectorTest)
{
   pool_allocator_t<X, 8> a;
   uintptr_t xp1 = 0, xp2 = 0;

   // create a vector and make it allocate two memory blocks of different sizes
   {  std::vector<X, pool_allocator_t<X, 8>> v(a);

      v.push_back({123, 456ul});

      // hold onto the address of X within the first element
      xp1 = (uintptr_t) &v.front();

      // resize to a value large enough that it would be guaranteed to allocate a new block
      v.resize(1000);

      // hold onto the second memory block
      xp2 = (uintptr_t) &v.front();
   }

   // check that all prebviously allocated memory blocks are reused
   {  std::vector<X, pool_allocator_t<X, 8>> v(a);
   
      v.push_back({456, 789ul});

      // check that the address of X within the first element is the same
      EXPECT_EQ(xp1, (uintptr_t) &v.front()) << "Existing maller memory block should be allocated from the pool";

      v.resize(1000);

      // and once more for the larger memory block
      EXPECT_EQ(xp2, (uintptr_t) &v.front()) << "Existing larger memory block should be allocated from the pool";

      // make sure we got two different memory blocks
      EXPECT_NE(xp1, xp2) << "Larger and smaller memory blocks should be different allocations";
   }
}

}
