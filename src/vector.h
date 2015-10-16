/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    vector.h
*/

#ifndef __VECTOR_H
#define __VECTOR_H

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <new>

// -----------------------------------------------------------------------
// 
// vector_t
//
// -----------------------------------------------------------------------
// 1. If copyref is true, references within vector elements can be relocated
// in memory. An object that contains a reference or a pointer that points to 
// another member of the same object or any of its subobjects cannot be moved 
// in memory and has to be recreated in the new location. For example, object
// D cannot be moved:
//
//    struct B {int i;};
//    struct D : B {const int& ir; D() : ir(i) {}};
//
// 2. The at() method returns a copy of the requested element or errval. It
// cannot return a reference without throwing an exception in case index is
// invalid (we cannot return a reference to errval either). This could be 
// partialy be worked around by defining an error data member in the class 
// whose reference could be returned, but the reference in this case would
// have to be to a const object.
//
template <typename type_t>
class vector_t {
   private:
      bool     copyref;             // true if node references can be copied
      size_t   count;               // element count
      size_t   maxcount;            // allocated elements
      size_t   delta;               // allocation delta (in elements)
      type_t   *elements;           // vector elements

   public:
      class iterator {
         friend class vector_t<type_t>;

         private:
            size_t   index;
            size_t   count;
            type_t   *eptr;

         protected:
            iterator(size_t ncount, type_t *nptr) {count = ncount; index = -1; eptr = nptr;}

         public:
            iterator(void) {count = 0; index = -1; eptr = NULL;}

            iterator(const iterator& iter) {count = iter.count; index = iter.index; eptr = iter.eptr;}

            iterator& operator = (const iterator& iter) {count = iter.count; index = iter.index; eptr = iter.eptr; return *this;}

            type_t& operator * (void) {return item();}

            type_t& item(void) {return *eptr;}

            bool more(void) const {return (int) index < (int) (count-1);}

            type_t& next(void) {return (++index == 0) ? *eptr : *++eptr;}

            type_t next(type_t errval) {return more() ? next() : errval;}
      };

      class const_iterator : public iterator {
         friend class vector_t<type_t>;

         private:
            const_iterator(size_t ncount, type_t *nptr) : iterator(ncount, nptr) {}

         public:
            const_iterator(void) : iterator() {}

            const_iterator(const const_iterator& iter) : iterator(iter) {}

            const_iterator(const iterator& iter) : iterator(iter) {}

            const_iterator& operator = (const const_iterator& iter) {iterator::operator = (iter); return *this;}

            const_iterator& operator = (const iterator& iter) {iterator::operator = (iter); return *this;}

            const type_t& operator * (void) {return iterator::item();}

            const type_t& item(void) {return iterator::item();}

            const type_t& next(void) {return iterator::next();}

            type_t next(type_t errval) {return iterator::next(errval);}
      };

      class reverse_iterator {
         friend class vector_t<type_t>;

         private:
            size_t   index;
            size_t   count;
            type_t   *eptr;

         protected:
            reverse_iterator(size_t ncount, type_t *nptr) {index = count = ncount; eptr = &nptr[ncount];}

         public:
            reverse_iterator(void) : count(0) {index = 0; eptr = NULL;}

            reverse_iterator(const reverse_iterator& iter) {count = iter.count; index = iter.index; eptr = iter.eptr;}

            reverse_iterator operator = (const reverse_iterator& iter) {count = iter.count; index = iter.index; eptr = iter.eptr; return *this;}

            type_t& operator * (void) {return item();}

            type_t& item(void) {return *eptr;}

            bool more(void) const {return index > 0;}

            type_t& next(void) {--index; return *--eptr;}

            type_t next(type_t errval) {return (more()) ? next() : errval;}
      };

      class const_reverse_iterator : public reverse_iterator {
         friend class vector_t<type_t>;

         private:
            const_reverse_iterator(size_t ncount, type_t *nptr) : reverse_iterator(ncount, nptr) {}

         public:
            const_reverse_iterator(void) : reverse_iterator() {}

            const_reverse_iterator(const const_reverse_iterator& iter) : reverse_iterator(iter) {}

            const_reverse_iterator(const reverse_iterator& iter) : reverse_iterator(iter) {}

            const_reverse_iterator& operator = (const const_reverse_iterator& iter) {reverse_iterator::operator = (iter); return *this;}

            const_reverse_iterator& operator = (const reverse_iterator& iter) {reverse_iterator::operator * (iter); return *this;}

            const type_t& operator * (void) {return reverse_iterator::item();}

            const type_t& item(void) {return reverse_iterator::item();}

            const type_t& next(void) {return reverse_iterator::next();}

            type_t next(type_t errval) {return reverse_iterator::next(errval);}
      };

   public:
      vector_t(size_t delta = 16, bool copyref = true) : delta(delta ? delta : 16), copyref(copyref) 
      {
         count = maxcount = 0; 
         elements = NULL;
      }

      vector_t(const vector_t& other) :
            delta(other.delta), copyref(other.copyref), 
            count(other.count), maxcount(other.maxcount)
      {
         elements = (type_t*) malloc(maxcount * sizeof(type_t)); 

         for(size_t i = 0; i < count; i++)
            ::new (elements+i) type_t(other.elements[i]);
      }

      vector_t(vector_t&& other) : 
            delta(other.delta), copyref(other.copyref), 
            count(other.count), maxcount(other.maxcount), 
            elements(other.elements)
      {
         other.count = 0;
         other.elements = NULL;
      }

      ~vector_t(void) 
      {
         if(elements) {
            for(size_t index = 0; index < count; index++)
               elements[index].~type_t();

            free(elements);
         }
      }

      vector_t& operator = (const vector_t& other)
      {
         clear();

         free(elements);

         delta = other.delta;
         copyref = other.copyref;
         count = other.count;
         maxcount = other.maxcount;

         elements = (type_t*) malloc(maxcount * sizeof(type_t)); 

         for(size_t i = 0; i < count; i++)
            ::new (elements+i) type_t(other.elements[i]);

         return *this;
      }

      vector_t& operator = (vector_t&& other)
      {
         clear();

         free(elements);

         delta = other.delta;
         copyref = other.copyref;
         count = other.count;
         maxcount = other.maxcount;
         elements = other.elements;

         other.count = 0;
         other.elements = NULL;

         return *this;
      }

      iterator begin(void) {return iterator(count, elements);}

      const_iterator begin(void) const {return const_iterator(count, elements);}

      reverse_iterator rbegin(void) {return reverse_iterator(count, elements);}

      const_reverse_iterator rbegin(void) const {return const_reverse_iterator(count, elements);}

      void push(const type_t& value) {insert(count, value);}

      void insert(size_t index, const type_t& value, size_t inscnt = 1)
      {
         size_t mvcnt, i;
         
         // check if we need to allocate more storage
         if(elements == NULL || count + inscnt > maxcount)
            reserve(count + inscnt + delta);

         if(index >= count)
            index = count;
         else {
            if((mvcnt = count-index) > 0) {
               if(copyref)
                  memmove(&elements[index+inscnt], &elements[index], mvcnt * sizeof(type_t));
               else {
                  // move elements to the right
                  for(i = count-1; i >= index; i--) {
                     // copy-initialize the new element 
                     ::new (elements+i+inscnt) type_t(elements[i]);
                     
                     // and destroy the old one
                     elements[i].~type_t();
                  }
               }
            }
         }

         // initialize new elements
         for(i = index; i < index+inscnt; i++)
            ::new (elements+i) type_t(value);
         
         count += inscnt;
      }

      void reserve(size_t nmax)
      {
         size_t index;

         if(nmax <= maxcount)
            return;

         //
         // If references inside elements can be copied, use realloc to
         // minimize initialization. Otherwise, use copy constructors to
         // copy existing elements into new membory block and default
         // constructors to initialize the remainder of the block.
         //
         if(copyref)
            elements = (type_t*) realloc(elements, nmax * sizeof(type_t));
         else {
            type_t *elems = elements;

            // allocate a new block
            elements = (type_t*) malloc(nmax * sizeof(type_t));

            // move existing elements to the new block
            for(index = 0; index < count; index++) {
               // copy-initialize the new element
               ::new (elements + index) type_t(elems[index]);
               
               // and destroy the old one
               elems[index].~type_t();
            }
            
            // free the old block   
            free(elems);
         }

         maxcount = nmax;
      }

      void remove(size_t index, size_t rmcnt = 1)
      {
         size_t mvcnt, i;

         if(rmcnt && index < count) {
            // check if rmcnt is more than we have
            if(index+rmcnt > count)
               rmcnt = count-index;

            // destroy removed elements
            for(i = index; i < index+rmcnt; i++)
               elements[i].~type_t();

            // shift the remaining elements to the left
            if((mvcnt = count-index-rmcnt) > 0) {
               if(copyref) 
                  memmove(&elements[index], &elements[index+rmcnt], mvcnt * sizeof(type_t));
               else {
                  for(i = index; i < count-rmcnt; i++) {
                     // copy the next element
                     ::new (elements+i) type_t(elements[i+rmcnt]);
                     
                     // and delete the one we just copied
                     elements[i+rmcnt].~type_t();
                  }
               }
            }

            count -= rmcnt;
         }
      }

      void clear(void) {if(count) remove(0, count);}

      size_t size(void) const {return count;}

      size_t capacity(void) const {return maxcount;}

      // see note #2 above
      type_t at(size_t index, type_t errval) const {return (index < count) ? elements[index] : errval;}

      const type_t& operator [] (size_t index) const {return elements[index];}

      type_t& operator [] (size_t index) {return elements[index];}
};

#endif // __VECTOR_H
