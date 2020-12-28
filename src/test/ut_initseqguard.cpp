/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_initseqguard.cpp
*/
#include "pch.h"

#include "../init_seq_guard.h"

namespace sswtest {

///
/// @brief  Test component that is assumed to be initialized on construction,
///         as if its initialization method was called.
///
/// @tparam compname    Character representing the name of the component.
/// @tparam throwex     Indicates whether `cleanup` should throw an exception or not.
///
/// If the component name is `X`, the constructor adds `+X` to `seqsteps` and `cleanup`
/// adds `-X`.
///
template <char compname, bool throwex = false>
struct TestComp {
   std::string&   seqsteps;   ///< Compound string of sequence initialization names.

   TestComp(std::string& seqsteps) :
         seqsteps(seqsteps)
   {
      seqsteps += '+';
      seqsteps += compname;
   }

   void cleanup(void)
   {
      if(throwex)
         throw 0;

      seqsteps += '-';
      seqsteps += compname;
   }
};

///
/// @brief  Tests clean-up of an initialization sequence interrupted by error exit.
///
TEST(InitSeqGuardTests, InterruptInitSequenceByExit)
{
   std::string seqsteps;

   {
      init_seq_guard_t init_seq_guard;

      TestComp<'A'> a(seqsteps);
      init_seq_guard.add_cleanup(a, &TestComp<'A'>::cleanup);

      TestComp<'B'> b(seqsteps);
      init_seq_guard.add_cleanup(b, &TestComp<'B'>::cleanup);

      TestComp<'C'> c(seqsteps);
      init_seq_guard.add_cleanup(c, &TestComp<'C'>::cleanup);

      // exit without disengaging, as if by error
   }

   EXPECT_EQ("+A+B+C-C-B-A", seqsteps) << "An initialization sequence interrupted by error exit is expected to be fully cleaned up";
}

///
/// @brief  Tests clean-up of an initialization sequence interrupted by exception.
///
TEST(InitSeqGuardTests, InterruptInitSequenceByException)
{
   std::string seqsteps;

   try {
      init_seq_guard_t init_seq_guard;

      TestComp<'A'> a(seqsteps);
      init_seq_guard.add_cleanup(a, &TestComp<'A'>::cleanup);

      TestComp<'B'> b(seqsteps);
      init_seq_guard.add_cleanup(b, &TestComp<'B'>::cleanup);

      TestComp<'C'> c(seqsteps);
      init_seq_guard.add_cleanup(c, &TestComp<'C'>::cleanup);

      throw 0;
   }
   catch (...) {
   }

   EXPECT_EQ("+A+B+C-C-B-A", seqsteps) << "An initialization sequence interrupted by exception is expected to be fully cleaned up";
}

///
/// @brief  Tests clean-up of an initialization sequence interrupted by error exit
///         and with one of the clean-up methods throwing an exception.
///
TEST(InitSeqGuardTests, InterruptInitSequenceWithABadCleanup)
{
   std::string seqsteps;

   {
      init_seq_guard_t init_seq_guard;

      TestComp<'A'> a(seqsteps);
      init_seq_guard.add_cleanup(a, &TestComp<'A'>::cleanup);

      // this clean-up call throws an exception
      TestComp<'B', true> b(seqsteps);
      init_seq_guard.add_cleanup(b, &TestComp<'B', true>::cleanup);

      TestComp<'C'> c(seqsteps);
      init_seq_guard.add_cleanup(c, &TestComp<'C'>::cleanup);

      // exit without disengaging, as if by error
   }

   EXPECT_EQ("+A+B+C-C-A", seqsteps) << "An initialization sequence interrupted by error exit is expected to finish all clean up steps";
}

///
/// @brief  Tests clean-up of a completed initialization sequence.
///
TEST(InitSeqGuardTests, SuccessfulInitSequence)
{
   std::string seqsteps;

   {
      init_seq_guard_t init_seq_guard;

      TestComp<'A'> a(seqsteps);
      init_seq_guard.add_cleanup(a, &TestComp<'A'>::cleanup);

      TestComp<'B'> b(seqsteps);
      init_seq_guard.add_cleanup(b, &TestComp<'B'>::cleanup);

      TestComp<'C'> c(seqsteps);
      init_seq_guard.add_cleanup(c, &TestComp<'C'>::cleanup);

      init_seq_guard.disengage();
   }

   EXPECT_EQ("+A+B+C", seqsteps) << "No clean-up should be performed if the initialization sequence was successful";
}

}
