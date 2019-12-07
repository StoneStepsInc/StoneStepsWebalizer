/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   char_buffer.h
*/

#ifndef CHAR_BUFFER_H
#define CHAR_BUFFER_H

#include <cstddef>

///
/// @class  char_buffer_base
///
/// @brief  A generic character buffer object
///
/// @tparam chart_t  Character type (char or wchar_t)
///
/// 1. `char_buffer_base` is a transient object that maintains a block of memory, large 
/// enough to accommodate bufsize narrow or wide characters, depending on the template 
/// argument.
///
/// 2. `char_buffer_base` makes no assumption about the content of the buffer and does not
/// ever interpret the content as a null-terminated string.
///
/// 3. Character buffers cannot be copied because there is no indication how much data is
/// in the buffer and copying the entire buffer would be inefficient. Character buffers
/// are intended to facilitate detaching/attaching memory from/to strings and should use
/// move semantics.
///
/// 4. A const character buffer is a distinct type and const and non-const character 
/// buffers cannot be constructed from one another. 
///
/// 5. [removed]
///
/// 6. A holder buffer does not own the buffer memory and can be used with character 
/// arrays that are not allocated dynamically. If a holder buffer is resized, it will
/// allocate a new memory block and will become a non-holder buffer.
///
/// 7. A buffer is always resized, even if the new requested memory size is smaller
/// than the current buffer size.
///
template <typename char_t>
class char_buffer_base {
   private:
      char_t   *buffer;          ///< character buffer
      size_t   bufsize;          ///< buffer size, in characters
      bool     holder;           ///< if true, does not own buffer memory

   public:
      char_buffer_base(void);

      // copying character buffers would be inefficient (see #3 above)
      char_buffer_base(const char_buffer_base& other) = delete;

      char_buffer_base(char_buffer_base&& other) noexcept;

      char_buffer_base(char_t *buffer, size_t bufsize, bool holder = false);

      explicit char_buffer_base(size_t bufsize);

      ~char_buffer_base(void);

      // copying character buffers would be inefficient (see #3 above)
      char_buffer_base& operator = (const char_buffer_base& other) = delete;

      char_buffer_base& operator = (char_buffer_base&& other) noexcept;

      void reset(void);

      char_buffer_base& attach(char_t *buf, size_t bsize, bool hold);

      char_t *detach(size_t *bsize = NULL, bool *hold = NULL);

      char_buffer_base& resize(size_t size, size_t datasize);

      size_t capacity(void) const {return bufsize;}

      operator char_t*(void) {return buffer;}

      operator const char_t*(void) const {return buffer;}

      bool operator == (const char_t *ptr) const {return buffer == ptr;}

      bool operator == (const char_buffer_base& other) const {return buffer == other.buffer && holder == other.holder && bufsize == other.bufsize;}

      char_t *operator + (size_t offset) {return offset < bufsize ? buffer + offset : NULL;}

      const char_t *operator + (size_t offset) const {return offset < bufsize ? buffer + offset : NULL;}

      char_t *get_buffer(void) {return buffer;}

      const char_t *get_buffer(void) const {return buffer;}

      size_t memsize(void) const {return bufsize * sizeof(char_t);}

      bool isholder(void) const {return holder;}

      bool isempty(void) const {return bufsize == 0;}

      bool isnull(void) const {return buffer == NULL;}

      //
      // Buffer memory management functions
      //
      static char_t *alloc(char_t *buffer, size_t bufsize, size_t datasize);

      static char_t *alloc(size_t bufsize);

      static void free(char_t *buffer);

      static size_t memsize(size_t char_count);
};

//
// A generic byte buffer type
//
typedef char_buffer_base<unsigned char> buffer_t;

///
/// @class  fixed_char_buffer_t
///
/// @brief  A fixed-size character buffer
///
/// @tparam char_t   Character type (char or wchar_t)
/// @tparam BUFSIZE  Fixed buffer size, in char_t characters
///
/// A fixed-size character buffer that can be used as a temporary storage in a local 
/// scope. For example, this code creates a temporary string without having to allocate
/// memory:
/// ```
///    string_t::fixed_char_buffer_t<hnode_t::ccode_size+1> ccode_buffer;
///    string_t ccode(ccode_buffer, 0, false);
///    ccnode_t *ccptr = &state.cc_htab.get_ccnode(hnode.get_ccode(ccode));
/// ```
/// A fixed-size character buffer does not prevent resizing and, just like any other
/// holder buffers, will allocate a new memory block when resized. Do not use this
/// class if you plan to resize the buffer.
///
template <typename char_t, size_t BUFSIZE>
class fixed_char_buffer_t : public char_buffer_base<char_t> {
   private:
      char_t   fixed_buffer[BUFSIZE];

   public:
      fixed_char_buffer_t(void) : char_buffer_base<char_t>(fixed_buffer, BUFSIZE, true)
      {
      }
};

///
/// @class  char_buffer_allocator_tmpl
///
/// @brief  A base class for a generic character buffer allocator
///
/// @tparam char_t            Character type (char or wchar_t)
/// @tparam alloc_params_t    A template parameter pack passed to the allocator implementation
///
/// Objects derived from `char_buffer_allocator_tmpl` may be used to maintain reusable
/// buffers in an optimal way defined by the allocator implementation in order to 
/// minimize buffer memory allocations, without having to use a single large buffer.
///
template <typename char_t, typename ... alloc_params_t>
class char_buffer_allocator_tmpl {
   public:
      virtual ~char_buffer_allocator_tmpl(void) {}

      virtual char_buffer_base<char_t> get_buffer(alloc_params_t ... args) = 0;

      virtual void release_buffer(char_buffer_base<char_t>&& buffer) = 0;
};

///
/// @class  char_buffer_holder_tmpl
///
/// @brief  A buffer holder maintains a buffer within some scope
///
/// @tparam char_t            Character type (char or wchar_t)
/// @tparam alloc_params_t    A template parameter pack passed to the allocator implementation
///
/// `char_buffer_holder_tmpl` is a convenience class to help manage temporary buffers 
/// within a local scope. For example:
/// ```
///  char_buffer_base<char_t>&& buffer = buffer_holder_t(*buffer_allocator).buffer;
/// ```
/// Note that the life of a temporary object bound to a reference to a return value
/// of a method is not extended beyond the end of the expression, which means that 
/// the buffer data member must be accessed directly for this to work. This is also
/// the reason convenience member functions, such as a buffer reference operator, 
/// should not be defined for the buffer holder class.
///
/// VC++ 2013 fails to recognize that the reference is bound to a subobject of a 
/// temporary and destroys the buffer holder and the buffer at the end of the line. 
/// VC 2015 fixes this problem.
///
template <typename char_t, typename ... alloc_params_t>
class char_buffer_holder_tmpl {
   private:
      char_buffer_allocator_tmpl<char_t, alloc_params_t ...>&  allocator;

   public:
      char_buffer_base<char_t>             buffer;

   public:
      char_buffer_holder_tmpl(char_buffer_allocator_tmpl<char_t, alloc_params_t ...>& allocator, alloc_params_t ... alloc_args) :
               allocator(allocator), buffer(allocator.get_buffer(alloc_args ...)) 
      {
      }

      char_buffer_holder_tmpl(const char_buffer_holder_tmpl&) = delete;

      char_buffer_holder_tmpl(char_buffer_holder_tmpl&& other) = delete;

      ~char_buffer_holder_tmpl(void) {allocator.release_buffer(std::move(buffer));}

      char_buffer_holder_tmpl& operator = (const char_buffer_holder_tmpl&) = delete;

      char_buffer_holder_tmpl& operator = (char_buffer_holder_tmpl&&) = delete;

      // deleted for visibility (see notes above the class definition)
      operator char_buffer_base<char_t>&& (void) = delete;
};

#endif // CHAR_BUFFER_H
