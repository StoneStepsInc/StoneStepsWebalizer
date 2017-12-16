/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    storage_info.h
*/
#ifndef STORAGE_INFO_H
#define STORAGE_INFO_H

#include <utility>
#include <stdexcept>

///
/// @struct storage_info_t
///
/// @brief  Represents the storage state of a data object and indicates whether the
///         object came from storage and whether it has been modified since it was
///         read from storage.
///
struct storage_info_t {
   bool dirty   : 1;          ///< Was the data object modified? New object is considered modified.
   bool deleted : 1;          ///< Was the data object deleted since it was read from storage?
   bool storage : 1;          ///< Was the data object read from storage?

   /// Constructs a new instance of a storage descriptor as modified and not from storage.
   storage_info_t(void) : dirty(true), deleted(false), storage(false) 
   {
   }

   /// Sets storage flags to indicate that the data object came from storage and hasn't been modified.
   void set_from_storage(void)
   {
      dirty = false;
      deleted = false;
      storage = true;
   }

   /// Sets storage flags to indicate that the data object was created and didn't come from storage.
   void set_created(void)
   {
      dirty = true;
      deleted = false;
      storage = false;
   }

   /// Sets the deleted flag to indicate that the data object is pending removal from storage.
   void set_deleted(void)
   {
      if(!storage)
         throw std::logic_error("Only nodes that came from storage can be marked as deleted");

      deleted = true;
   }

   /// Sets the modified flag to indicate that the data object changed since it was read from storage.
   void set_modified(void)
   {
      if(!storage)
         throw std::logic_error("Only nodes that came from storage can be marked as modified");

      dirty = true;
   }
};

//
// VC++ 2015 and 2017 seem to have incorrect interpretation of copy constructors 
// in the C++ standard, which says that all forms of copy constructors are allowed. 
// This is an example from the section 12.8.1.3 of the standard:
//
//    struct X {
//       X(const X&);
//       X(X&);            // OK
//       X(X&&);
//       X(const X&&);     // OK, but possibly not sensible
//    };
//
// Disable the warning about multiple copy constructors for just storable_t.
//
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4521)  // multiple copy constructors specified
#endif

///
/// @struct storable_t
///
/// @brief  Maintains storage flags on behalf of the underlying data object.
///
/// @tparam object_t An underlying data object class name.
///
/// This object derives from an underlying data object, so it is possible to separate the 
/// underlying object data from storage information, which in turn allows us to pass a 
/// reference to a `const` underlying data object into storage functions and a reference 
/// to a non-`const` storage information object. For example:
/// ```
///     // read X from storage and update storage flags within storable_t
///     void read_from_storage(storable_t<X>& x);
///
///     // write X to storage and update flags in storage_info_t
///     void write_to_storage(const X& x, storage_info_t& xsi);
///
///     storable_t<X> obj_x;
///
///     read_from_storage(x);
///     // ... use and modify x ...
///     write_to_storage(x, x.storage_info);
/// ```
/// Keeping storage information in `storage_info`, as opposed to keeping it in `storable_t`, 
/// makes a clear cut between the underlying data object and `storable_t`. If storage flags
/// were in `storable_t`, the example above wouldn't work.
///
/// We need to define bodies of all copy and move constructors because VC++ 2015 and 2017 
/// fail to compile multiple defaulted copy and move constructors.
///
/// The copy constructor that takes a reference to a non-`const` storable object is needed
/// because otherwise the template constructor is called and copy semantics are not followed.
///
template <typename object_t>
struct storable_t : public object_t {
   storage_info_t storage_info;        ///< Storage flags and other related information

   /// Constructs a default instance of a storable and underlying objects
   storable_t(void)
   {
   }

   /// Constructs a copy of a storable object from a reference to a `const` underlying object.
   storable_t(const storable_t<object_t>& other) : storage_info(other.storage_info), object_t(other) 
   {
   }

   /// Constructs a copy of a storable object from a reference to an underlying object.
   storable_t(storable_t<object_t>& other) : storage_info(other.storage_info), object_t(other) 
   {
   }

   /// Constructs an instance of a storable object by moving the underlying object from the supplied reference.
   storable_t(storable_t<object_t>&& other) : storage_info(other.storage_info), object_t(std::move(other)) 
   {
   }

   /// Constructs an underlying object from supplied arbitrary parameters.
   template <typename ... param_t>
   storable_t(param_t&& ... param) : object_t(std::forward<param_t>(param) ...)
   {
   }

   /// Resets the storage descriptor and the underlying object using supplied arbitrary parameters.
   template <typename ... param_t>
   void reset(param_t&& ... param)
   {
      object_t::reset(std::forward<param_t>(param) ...);
      storage_info.set_created();
   }
};

// Revert the multiple copy constructor warning to its original setting
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // STORAGE_INFO_H
