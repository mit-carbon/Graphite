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
#include "log.h"
#include "run_models.h"
#include "analysis.h"
#include "routine_replace.h"

// FIXME: This list could probably be trimmed down a lot.
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "syscall_model.h"
#include "thread_manager.h"
#include "config_file.hpp"
#include "handle_args.h"

#include "redirect_memory.h"
#include "handle_syscalls.h"
#include <typeinfo>

bool done_copying_static_data = false;
config::ConfigFile *cfg;

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}

void routineCallback(RTN rtn, void *v)
{
   string rtn_name = RTN_Name(rtn);
   
   replaceUserAPIFunction(rtn, rtn_name);
   
   // TODO:
   // Commenting out performance modeling code since it causes multiple accesses to memory
   // when we are simulating shared memory. Fix perf model code to not cause any memory accesses
   //  
   // bool did_func_replace = replaceUserAPIFunction(rtn, rtn_name);
   // if (!did_func_replace)
   //    replaceInstruction(rtn, rtn_name);
}

void instructionCallback (INS ins, void *v)
{
   // Emulate stack operations
   bool stack_op = rewriteStackOp (ins);

   // Else, redirect memory to the simulated memory system
   if (!stack_op)
   {
      rewriteMemOp (ins);
   }

   return;
}

// syscall model wrappers

void SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallEnterRunModel (ctxt, std);
}

void SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallExitRunModel (ctxt, std);
}

VOID threadStart (THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
   Sim()->getThreadManager()->threadStart ();
}

VOID threadFini (THREADID tid, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
   Sim()->getThreadManager()->threadFini ();
}

void ApplicationStart()
{
}

void ApplicationExit(int, void*)
{
   LOG_PRINT("Application exit.");
   Simulator::release();
   delete cfg;
}

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

VOID threadStartCallback(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, VOID *v)
{
   UInt32 curr_process_num = Config::getSingleton()->getCurrentProcessNum();

   ADDRINT reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);
   ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);

   // Conditions under which we must initialize a core
   // 1) (!done_app_initialization) && (curr_process_num == 0)
   // 2) (done_app_initialization) && (!thread_spawner)

   if (! done_app_initialization)
   {
      // This is the main thread
      int res;
      PIN_CallApplicationFunction(ctxt,
            PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            AFUNPTR(CarbonSpawnThreadSpawner),
            PIN_PARG(int), &res,
            PIN_PARG_END());
      
      LOG_ASSERT_ERROR(res == 0, "Failed to spawn Thread Spawner");

      allocateStackSpace();

      if (curr_process_num == 0)
      {
         Sim()->getCoreManager()->initializeThread(0);
         // 1) Copying over Static Data
         // Get the image first
         IMG img = IMG_FindByAddress(reg_eip);
         copyStaticData(img);

         // 2) Copying over initial stack data
         copyInitialStackData(reg_esp);
      }

      done_app_initialization = true;

      if (curr_process_num == 0)
      {
         // Call '_start'
         return;
      }
      else
      {
         LOG_PRINT("Waiting for main process to finish...");
         while (!Sim()->finished())
            usleep(100);
         LOG_PRINT("Finished!");
         exit(0);
      }
 
   }
   else
   {
      // This is NOT the main thread
      // Maybe 'application' thread or 'thread spawner'
      Sim()->getThreadManager()->threadStart();
   }
}

int main(int argc, char *argv[])
{

   // Global initialization
   PIN_InitSymbols();
   PIN_Init(argc,argv);

   string_vec args;

   // Set the default config path if it isn't 
   // overwritten on the command line.
   std::string config_path = "carbon_sim.cfg";

   parse_args(args, config_path, argc, argv);

   cfg = new config::ConfigFile();
   cfg->load(config_path);

   handle_args(args, *cfg);

   Simulator::setConfig(cfg);

   Simulator::allocate();
   Sim()->start();

   // Copying over the static data now
   IMG_AddInstrumentationFunction(imageCallback, 0);

   // Instrumentation
   LOG_PRINT("Start of instrumentation.");
   RTN_AddInstrumentFunction(routineCallback, 0);

   PIN_AddThreadStartFunction (threadStartCallback, 0);
   PIN_AddThreadFiniFunction (threadFiniCallback, 0);
   
   if(cfg->getBool("general/enable_syscall_modeling"))
   {
      PIN_AddSyscallEntryFunction(SyscallEntry, 0);
      PIN_AddSyscallExitFunction(SyscallExit, 0);
      PIN_AddContextChangeFunction (contextChange, NULL);
   }

   if (cfg->getBool("general/enable_shared_mem"))
   {
      INS_AddInstrumentFunction (instructionCallback, 0);
   }

   PIN_AddFiniFunction(ApplicationExit, 0);

   // Just in case ... might not be strictly necessary
   Transport::getSingleton()->barrier();

   // Never returns
   LOG_PRINT("Running program...");
   PIN_StartProgram();

   return 0;
}
