/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   init_seq_guard.h
*/

#include <tuple>
#include <utility>

#ifndef INIT_SEQ_GUARD_H
#define INIT_SEQ_GUARD_H

#include <stack>
#include <memory>

///
/// @brief  Interrupted component initialization sequence clean-up interface.
///
struct component_cleanup_t {
   /// Expected to clean-up the associated component, unless `disengage` was called before. 
   virtual ~component_cleanup_t(void) {}

   /// Expected to disable automatic clean-up in the destructor.
   virtual void disengage(void) = 0;
};

///
/// @brief  Interrupted component initialization sequence clean-up implementation.
//
template <typename component_t>
class component_cleanup_tmpl_t : public component_cleanup_t {
   public:
      typedef void (component_t::*cleanup_func_t)(void);

   private:
      component_t       *component;       ///< A component initialized in a sequence.

      cleanup_func_t    cleanup_func;     ///< A pointer to the component's clean-up method.

   public:
      /// Constructs a component clean-up object from a component and the clean-up method pointers.
      component_cleanup_tmpl_t(component_t& component, cleanup_func_t cleanup_func) :
            component(&component), cleanup_func(cleanup_func)
      {
      }

      /// Moves component clean-up elements from another component clean-up object.
      component_cleanup_tmpl_t(component_cleanup_tmpl_t&& other) :
         component(other.component), cleanup_func(other.cleanup_func)
      {
         other.component = nullptr;
         other.cleanup_func = nullptr;
      }

      /// Invokes the component clean-up method, unless `disengage` was called earlier.
      ~component_cleanup_tmpl_t(void)
      {
         try {
            if(component && cleanup_func)
               (component->*cleanup_func)();
         }
         catch (...) {
            //
            // This sequence of clean-up calls is a side effect of a failed initialization,
            // so keep it quiet and don't report any failures, so we don't drown them in
            // what may be a fall-out of the failed initialization.
            //
         }
      }

      ///< Disables automatic component clean-up call on destruction.
      void disengage(void) override
      {
         component = nullptr;
         cleanup_func = nullptr;
      }
};

///
/// @brief  Interrupted component initialization sequence clean-up guard.
///
/// If multiple components are initialized in a sequence and one of them throws an
/// exception, components that have already been initialized should be cleaned up
/// in a proper sequence before component destructors are called, which may lead
/// to clean-up calls made out of order or to an exception thrown from one of the
/// destructors during stack unwinding. This class protects against this outcome
/// by calling clean-up methods of each registered component in the reverse order
/// of their registration.
///
class init_seq_guard_t {
   private:
      typedef std::stack<std::unique_ptr<component_cleanup_t>> comp_stack_t;

   private:
      comp_stack_t   components;       ///< Components initialized in a sequence.

   public:
      /// Cleans up components in the reverse order of their registration, unless `disengage` was called.
      ~init_seq_guard_t(void)
      {
         while(!components.empty())
            components.pop();
      }

      /// Registers a component for a possible clean-up after it has been initialized.
      template <typename component_t>
      void add_cleanup(component_t& component, typename component_cleanup_tmpl_t<component_t>::cleanup_func_t cleanup)
      {
         components.emplace(new component_cleanup_tmpl_t<component_t>(component, cleanup));
      }

      ///< Disables automatic component clean-up call on destruction.
      void disengage(void)
      {
         while(!components.empty()) {
            components.top()->disengage();
            components.pop();
         }
      }
};

#endif // INIT_SEQ_GUARD_H
