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
   bool did_func_replace = replaceUserAPIFunction(rtn, rtn_name);

   if (!did_func_replace)
      replaceInstruction(rtn, rtn_name);
}

// syscall model wrappers

void SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
   {
      UInt8 syscall_number = (UInt8) PIN_GetSyscallNumber(ctxt, std);
      SyscallMdl::syscall_args_t args;
      args.arg0 = PIN_GetSyscallArgument(ctxt, std, 0);
      args.arg1 = PIN_GetSyscallArgument(ctxt, std, 1);
      args.arg2 = PIN_GetSyscallArgument(ctxt, std, 2);
      args.arg3 = PIN_GetSyscallArgument(ctxt, std, 3);
      args.arg4 = PIN_GetSyscallArgument(ctxt, std, 4);
      args.arg5 = PIN_GetSyscallArgument(ctxt, std, 5);
      UInt8 new_syscall = core->getSyscallMdl()->runEnter(syscall_number, args);
      PIN_SetSyscallNumber(ctxt, std, new_syscall);
   }
}

void SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
   {
      carbon_reg_t old_return = 
#ifdef TARGET_IA32E
      PIN_GetContextReg(ctxt, REG_RAX);
#else
      PIN_GetContextReg(ctxt, REG_EAX);
#endif

      carbon_reg_t syscall_return = core->getSyscallMdl()->runExit(old_return);

#ifdef TARGET_IA32E
      PIN_SetContextReg(ctxt, REG_RAX, syscall_return);
#else
      PIN_SetContextReg(ctxt, REG_EAX, syscall_return);
#endif
   }
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

int CarbonMain(CONTEXT *ctx, AFUNPTR fp_main, int argc, char *argv[])
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

   LOG_PRINT("Leaving CarbonMain...");

   return 0;
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

   // Instrumentation
   LOG_PRINT("Start of instrumentation.");
   RTN_AddInstrumentFunction(routineCallback, 0);

   if(cfg->getBool("general/enable_syscall_modeling"))
   {
       PIN_AddSyscallEntryFunction(SyscallEntry, 0);
       PIN_AddSyscallExitFunction(SyscallExit, 0);
   }

   PIN_AddFiniFunction(ApplicationExit, 0);

   // Just in case ... might not be strictly necessary
   Transport::getSingleton()->barrier();

   // Never returns
   LOG_PRINT("Running program...");
   PIN_StartProgram();

   return 0;
}
