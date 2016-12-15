/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   pool_allocator.h
*/
#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <memory>
#include <vector>
#include <limits.h>

// -----------------------------------------------------------------------
// pool_allocator_t
//
// 1. Pool allocator makes such STL containers as std::list more efficient 
// by minimizing memory allocations for container nodes.
//
// 2. The pool contains memory blocks of sizeof(T) size, not constructed
// objects. Memory allocations for more than one object are not pooled.
//
template <typename T, size_t POOLSIZE>
class pool_allocator_t {
   template <typename U, size_t> friend class pool_allocator_t;

private:
   std::allocator<T>    stdalloc;      // standard allocator

   std::vector<T*>      mempool;       // memory block pool

public:
   typedef typename std::allocator<T>::value_type value_type;
   typedef typename std::allocator<T>::size_type size_type;
   typedef typename std::allocator<T>::difference_type difference_type;

   typedef typename std::allocator<T>::pointer pointer;
   typedef typename std::allocator<T>::const_pointer const_pointer;
   typedef typename std::allocator<T>::reference reference;
   typedef typename std::allocator<T>::const_reference const_reference;

   template <typename U> struct rebind {typedef pool_allocator_t<U, POOLSIZE> other;};

   pool_allocator_t(void)
   {
      mempool.reserve(POOLSIZE);
   }

   pool_allocator_t(const pool_allocator_t& other) : stdalloc(other.stdalloc)
   {
      mempool.reserve(POOLSIZE);
   }

   pool_allocator_t(pool_allocator_t&& other) : stdalloc(std::move(other.stdalloc))
   {
      mempool.reserve(POOLSIZE);
      mempool = std::move(other.mempool);
   }

   template <typename U>
   pool_allocator_t(const pool_allocator_t<U, POOLSIZE>& other) : stdalloc(other.stdalloc)
   {
      mempool.reserve(POOLSIZE);
   }

   template <typename U>
   pool_allocator_t(pool_allocator_t<U, POOLSIZE>&& other) : stdalloc(std::forward<std::allocator<U>>(other.stdalloc))
   {
      mempool.reserve(POOLSIZE);
   }

   ~pool_allocator_t(void)
   {
      for(typename std::vector<T*>::iterator iter = mempool.begin(); iter != mempool.end(); iter++)
         deallocate(*iter, 1);
      mempool.clear();
   }

   void operator = (const pool_allocator_t& other)
   {
      stdalloc = other.stdalloc;
   }

   void operator = (pool_allocator_t&& other)
   {
      stdalloc = other.stdalloc;
      mempool = std::move(other.mempool);
   }

   bool operator == (const pool_allocator_t& other)
   {
      // two allocators are equal if memory allocated by one can be deallocated by the other
      return stdalloc == other.stdalloc;
   }

   bool operator != (const pool_allocator_t& other)
   {
      return stdalloc != other.stdalloc;
   }

   pointer address(reference ref) const
   {
      return stdalloc.address(ref);
   }

   const_pointer address(const_reference ref) const
   {
      return stdalloc.address(ref);
   }

   pointer allocate(size_type count)
   {
      if(count != 1 || mempool.empty())
         return stdalloc.allocate(count);

      pointer memblock = mempool.back();

      mempool.pop_back();

      return memblock;
   }

   void deallocate(pointer area, size_type count)
   {
      if(count != 1 || mempool.size() == POOLSIZE)
         stdalloc.deallocate(area, count);
      else
         mempool.push_back(area);
   }

   template <typename U, typename ...Args>
   void construct(U *memblock, Args&&... args)
   {
      stdalloc.construct(memblock, std::forward<Args>(args)...);
   }

   void destroy(pointer object)
   {
      stdalloc.destroy(object);
   }

    size_type max_size(void) const
    {
       return stdalloc.max_size();
    }
};

#endif // POOL_ALLOCATOR_H
