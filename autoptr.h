/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    autoptr.h
*/
#ifndef __AUTOPTR_H
#define __AUTOPTR_H

// -----------------------------------------------------------------------
//
// autoptr_t
//
// -----------------------------------------------------------------------
template <typename type_t>
class autoptr_t {
   private:
      type_t   *object;

   public:
      explicit autoptr_t(type_t *object = NULL) : object(object) 
      {
      }

      autoptr_t(autoptr_t& ptr) : object(ptr.object) 
      {
         ptr.object = NULL;
      }

      ~autoptr_t(void) 
      {
         delete object;
      }

      autoptr_t& operator = (autoptr_t& ptr)
      {
         set(ptr.object);
         ptr.object = NULL;

         return *this;
      }

      autoptr_t& operator = (type_t **pptr)
      {
         return set(pptr);
      }

      autoptr_t& set(type_t *obj) 
      {
         delete object; 
         object = obj;
         return *this;
      }

      autoptr_t& set(type_t **pptr) 
      {
         delete object;

         if(!pptr)
            object = NULL;
         else {
            object = *pptr;
            *pptr = NULL;
         }
         return *this;
      }

      type_t *release(void)
      {
         type_t *tmp = object;
         object = NULL;
         return tmp;
      }

      operator type_t * (void) 
      {
         return object;
      }

      operator const type_t * (void) const 
      {
         return object;
      }

      type_t *operator -> (void) 
      {
         return object;
      }

      const type_t *operator -> (void) const 
      {
         return object;
      }
};

#endif // __AUTOPTR_H
