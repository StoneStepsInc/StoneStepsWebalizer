/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   pool_allocator.h
*/
#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <memory>
#include <vector>
#include <stack>
#include <map>
#include <limits.h>

// -----------------------------------------------------------------------
// memory_pool_t
//
// A memory pool is a non-typed memory allocator that keeps a few stacks of 
// memory blocks in an associative container indexed by the block size. Up 
// to POOLSIZE buckets containing stacks of memory blocks of the same size 
// will be created. 
// 
// When the pool size limit is reached, the least used bucket will be removed 
// from the pool and a bucket for the new block size will be created.
//
// Each bucket in the pool maintains a stack of memory blocks of the same size,
// up to BUCKETSIZE entries. Once the BUCKETSIZE limit is reached, all memory 
// blocks returned to the pool will be freed.
//
template <size_t BUCKETSIZE, size_t POOLSIZE = SIZE_MAX>
class memory_pool_t {
   typedef std::stack<void*, std::vector<void*>> block_stack_t;

   private:
      //
      // A bucket maintains a stack of memory blocks up to BUCKETSIZE and a 
      // point in a life time counter that is updated every time a block of 
      // memory is allocated or deallocated to track the age of each bucket,
      // which shows how long ago the bucket was last used within the pool.
      //
      class bucket_t {
         uint64_t       lifept;  // point in a pool life time
         block_stack_t  blocks;  // stack of memory blocks

         public:
            bucket_t(uint64_t life, void *block) : lifept(life) 
            {
               blocks.push(block);
            }

            bucket_t(uint64_t life) : lifept(life) 
            {
            }

            bucket_t(bucket_t&& other) : lifept(other.lifept), blocks(std::move(other.blocks))
            {
               other.lifept = 0;
            }

            ~bucket_t(void)
            {
               while(!blocks.empty()) {
                  std::free(blocks.top());
                  blocks.pop();
               }
            }

            uint64_t age(uint64_t life) const
            {
               return life - lifept;
            }

            void put_block(uint64_t life, void *block)
            {
               lifept = life;

               if(blocks.size() == BUCKETSIZE)
                  std::free(block);
               else
                  blocks.push(block);
            }

            void *get_block(uint64_t life, size_t size)
            {
               void *block;
               
               lifept = life;

               if(blocks.empty())
                  block = std::malloc(size);
               else {
                  block = blocks.top();
                  blocks.pop();
               }

               return block;
            }
      };

   private:
      uint64_t                   life;       // pool life counter
      std::map<size_t, bucket_t> mempool;    // a pool of memory buckets

   private:
      void dump_oldest_bucket(void)
      {
         // current and maximum age iterators
         typename std::map<size_t, bucket_t>::iterator i = mempool.begin(), mi = i;

         // make sure there's at least one bucket in the pool
         if(i == mempool.end())
            return;

         // find the bucket with the greatest age (skip the first one)
         while(++i != mempool.end()) {
            if(i->second.age(life) > mi->second.age(life))
               mi = i;
         }

         // remove the bucket we just found from the pool
         mempool.erase(mi);
      }

   public:
      memory_pool_t(void) : life(0)
      {
      }

      memory_pool_t(const memory_pool_t&) = delete;

      void *allocate(size_t size)
      {
         typename std::map<size_t, bucket_t>::iterator bi = mempool.find(size);

         if(bi == mempool.end()) {
            // if we've reached the pool size limit, dump the least used bucket
            if(mempool.size() == POOLSIZE)
               dump_oldest_bucket();

            // create a new empty bucket, so we can start its life counter on allocation
            bi = mempool.emplace(size, bucket_t(0)).first;
         }

         // get a pooled or a new memory block from the bucket
         return bi->second.get_block(++life, size);
      }

      void deallocate(void *block, size_t size)
      {
         typename std::map<size_t, bucket_t>::iterator bi = mempool.find(size);
         
         if(bi != mempool.end())
            bi->second.put_block(++life, block);
         else {
            // if we've reached the pool size limit, dump the least used bucket
            if(mempool.size() == POOLSIZE)
               dump_oldest_bucket();

            // put the block into the bucket
            mempool.emplace(size, bucket_t(++life, block));
         }
      }
};

// -----------------------------------------------------------------------
// pool_allocator_t
//
// A pool allocator may be used with any STL container to minimize dynamic 
// memory allocations.
//
// The effectiveness of the pool allocator depends on the STL container type,
// container usage pattern and the pool limits in each template instantiation. 
// 
// For  example, std::list only allocates list nodes and configuring bucket 
// size to some sensible limit is all that is required. However, std::vector 
// may allocate unlimited number of memory blocks in multiples of sizeof(T) 
// and may be not very effective if pool limits are too low or may consume 
// too  much memory if limits are too high. Reserving some minimum capacity 
// will work well in many cases, especially for containers that go though 
// natural repetitive usage patterns (e.g. iteratively parsing query strings
// in URLs). 
//
// Some STL containers (e.g. VC++ list) require their allocator to be default
// constructable, which makes it impossible to construct a pool allocator with
// the same instance of a memory pool passed into a constructor, which would be 
// a more straightforward approach. As a work-around, a pool allocator maintains 
// a shared pointer to a memory pool created by the first default-constructed 
// allocator.
//
template <typename T, size_t BUCKETSIZE, size_t POOLSIZE = SIZE_MAX>
class pool_allocator_t {
   // make sure we can access rebound instances of pool_allocator_t
   template <typename U, size_t, size_t> friend class pool_allocator_t;

   private:
      std::shared_ptr<memory_pool_t<BUCKETSIZE, POOLSIZE>> mempool;

   public:
      typedef typename std::allocator<T>::value_type value_type;
      typedef typename std::allocator<T>::size_type size_type;

      typedef typename std::allocator<T>::pointer pointer;
      typedef typename std::allocator<T>::const_pointer const_pointer;
      typedef typename std::allocator<T>::reference reference;
      typedef typename std::allocator<T>::const_reference const_reference;

      template <typename U> struct rebind {typedef pool_allocator_t<U, BUCKETSIZE, POOLSIZE> other;};

   public:
      pool_allocator_t(void) : mempool(new memory_pool_t<BUCKETSIZE, POOLSIZE>())
      {
      }

      pool_allocator_t(const pool_allocator_t& other) : mempool(other.mempool)
      {
      }

      template <typename U>
      pool_allocator_t(const pool_allocator_t<U, BUCKETSIZE, POOLSIZE>& other) : mempool(other.mempool)
      {
      }

      ~pool_allocator_t(void)
      {
      }

      void operator = (const pool_allocator_t& other)
      {
         mempool = other.mempool;
      }

      bool operator == (const pool_allocator_t& other)
      {
         // two allocators are equal if memory allocated by one can be deallocated by the other
         return mempool == other.mempool;
      }

      bool operator != (const pool_allocator_t& other)
      {
         return mempool != other.mempool;
      }

      pointer allocate(size_type count)
      {
         return (pointer) mempool->allocate(count * sizeof(T));
      }

      void deallocate(pointer area, size_type count)
      {
         mempool->deallocate(area, count * sizeof(T));
      }

      template <typename U, typename ...Args>
      void construct(U *block, Args&&... args)
      {
         new (block) U(std::forward<Args>(args)...);
      }

      void destroy(pointer object)
      {
         object->~T();
      }
};

#endif // POOL_ALLOCATOR_H
