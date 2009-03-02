// Jonathan Eastep, Harshad Kasture, Jason Miller, Chris Celio, Charles Gruenwald,
// Nathan Beckmann, David Wentzlaff, James Psota
// 10.12.08
//
// Carbon Computer Simulator
//
// This simulator models future multi-core computers with thousands of cores.
// It runs on today's x86 multicores and will scale as more and more cores
// and better inter-core communications mechanisms become available.
// The simulator provides a platform for research in processor architecture,
// compilers, network interconnect topologies, and some OS.
//
// The simulator runs on top of Intel's Pin dynamic binary instrumention engine.
// Application code in the absence of instrumentation runs more or less
// natively and is thus high performance. When instrumentation is used, models
// can be hot-swapped or dynamically enabled and disabled as desired so that
// performance tracks the level of simulation detail needed.

#include <iostream>
#include <assert.h>
#include <set>
#include <sys/syscall.h>
#include <unistd.h>

#include "pin.H"
#include "knobs.h"
#include "log.h"
#include "run_models.h"
#include "analysis.h"
#include "routine_replace.h"

// FIXME: This list could probably be trimmed down a lot.
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "syscall_model.h"
#include "user_space_wrappers.h"
#include "thread_manager.h"

#define LOG_DEFAULT_RANK    -1
#define LOG_DEFAULT_MODULE  PINSIM

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}

void routineCallback(RTN rtn, void *v)
{
   string rtn_name = RTN_Name(rtn);
   bool did_func_replace = replaceUserAPIFunction(rtn, rtn_name);

   if (!did_func_replace)
      replaceInstruction(rtn, rtn_name);
}

// syscall model wrappers
void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
      core->getSyscallMdl()->runEnter(ctx, syscall_standard);
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
      core->getSyscallMdl()->runExit(ctx, syscall_standard);
}

void SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallEnterRunModel(ctxt, std);
}

void SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallExitRunModel(ctxt, std);
}

void ApplicationStart()
{
}

void ApplicationExit(int, void*)
{
   LOG_PRINT("Application exit.");
   Simulator::release();
}

// void ThreadStart(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, void *v)
// {
//    LOG_PRINT("Thread Start: %d", syscall(__NR_gettid));
// }

// void ThreadFini(THREADID threadIndex, const CONTEXT *ctxt, INT32 code, void *v)
// {
//    LOG_PRINT("Thread Fini: %d", syscall(__NR_gettid));
// }

void SimSpawnThreadSpawner(CONTEXT *ctx, AFUNPTR fp_main)
{
   // Get the function for the thread spawner
   PIN_LockClient();
   AFUNPTR thread_spawner;
   IMG img = IMG_FindByAddress((ADDRINT)fp_main);
   RTN rtn = RTN_FindByName(img, "CarbonSpawnThreadSpawner");
   thread_spawner = RTN_Funptr(rtn);
   PIN_UnlockClient();

   // Get the address of the thread spawner
   int res;
   PIN_CallApplicationFunction(ctx,
         PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         thread_spawner,
         PIN_PARG(int), &res,
         PIN_PARG_END());

}

int SimMain(CONTEXT *ctx, AFUNPTR fp_main, int argc, char *argv[])
{
   ApplicationStart();

   SimSpawnThreadSpawner(ctx, fp_main);

   if (Config::getSingleton()->getCurrentProcessNum() == 0)
   {
      LOG_PRINT("Calling main()...");

      Sim()->getCoreManager()->initializeThread(0);

      // call main()
      int res;
      PIN_CallApplicationFunction(ctx,
                                  PIN_ThreadId(),
                                  CALLINGSTD_DEFAULT,
                                  fp_main,
                                  PIN_PARG(int), &res,
                                  PIN_PARG(int), argc,
                                  PIN_PARG(char**), argv,
                                  PIN_PARG_END());
   }
   else
   {
      LOG_PRINT("Waiting for main process to finish...");
      while (!Sim()->finished())
         usleep(100);
      LOG_PRINT("Finished!");
   }

   return 0;
}

int main(int argc, char *argv[])
{
   // Global initialization
   PIN_InitSymbols();

   if (PIN_Init(argc,argv))
      return usage();

   Simulator::allocate();
   Sim()->start();

   // Instrumentation
   LOG_PRINT("Start of instrumentation.");
   RTN_AddInstrumentFunction(routineCallback, 0);
   PIN_AddSyscallEntryFunction(SyscallEntry, 0);
   PIN_AddSyscallExitFunction(SyscallExit, 0);

//   PIN_AddThreadStartFunction(ThreadStart, 0);
//   PIN_AddThreadFiniFunction(ThreadFini, 0);
   PIN_AddFiniFunction(ApplicationExit, 0);

   // Just in case ... might not be strictly necessary
   Transport::getSingleton()->barrier();

   // Never returns
   LOG_PRINT("Running program...");
   PIN_StartProgram();

   return 0;
}
