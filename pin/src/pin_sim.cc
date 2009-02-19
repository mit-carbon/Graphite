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

#include "pin.H"
#include "utils.h"
#include "bit_vector.h"
#include "config.h"
#include "core_manager.h"
#include "cache.h"
#include "ocache.h"
#include "perfmdl.h"
#include "knobs.h"
#include "mcp.h"
#include "mcp_runner.h"
#include "sim_thread_runner.h"
#include "sim_thread.h"
#include "log.h"
#include "dram_directory_entry.h"
#include "core.h"
#include "syscall_model.h"
#include "run_models.h"
#include "analysis.h"
#include "routine_replace.h"

#include "shmem_debug_helper.h"

#define LOG_DEFAULT_RANK    core_id
#define LOG_DEFAULT_MODULE  PINSIM

CoreManager *g_core_manager = NULL;
Config *g_config = NULL;
Transport *g_transport = NULL;
MCP *g_MCP = NULL;
MCPRunner * g_mcp_runner = NULL;
SimThreadRunner * g_sim_thread_runners = NULL;
Log *g_log = NULL;
ShmemDebugHelper *g_shmem_debug_helper = NULL;

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
   Core *core = g_core_manager->getCurrentCore();

   if (core)
      core->getSyscallMdl()->runEnter(ctx, syscall_standard);
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   Core *core = g_core_manager->getCurrentCore();

   if (core)
      core->getSyscallMdl()->runExit(ctx, syscall_standard);
}

void fini(int code, void * v)
{
   LOG_PRINT_EXPLICIT(-1, PINSIM, "fini start");

   // Make sure all other processes are finished before we start tearing down stuffs
   if(g_config->getProcessCount() > 1)
      Transport::getSingleton()->barrier();

   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
      g_MCP->finish();

   SimThreadQuit();

   g_core_manager->outputSummary();

   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
      delete g_mcp_runner;

   delete [] g_sim_thread_runners;
   delete g_core_manager;

   delete g_transport;

   LOG_PRINT_EXPLICIT(-1, PINSIM, "fini end");

   delete g_log;
   delete g_config;
}

void init_globals()
{
   if (g_knob_simarch_has_shared_mem)
   {

      if (!g_knob_enable_dcache_modeling)
      {

         cerr << endl << "**********************************************************************" << endl;
         cerr << endl << "  User must set dcache modeling on (-mdc) to use shared memory model. " << endl;
         cerr << endl << "**********************************************************************" << endl;

         cerr << endl << "Exiting Program...." << endl;
         exit(-1);
      }
   }

   g_config = new Config;
   //g_config->loadFromFile(FIXME);

   g_transport = Transport::create();

   g_shmem_debug_helper = new ShmemDebugHelper();

   // I think this one probably wants a total core count
   g_core_manager = new CoreManager();

   // Note the MCP has a dependency on the transport layer and the core_manager.
   // Only create an MCP on the correct process.
   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
   {
      LOG_PRINT_EXPLICIT(-1, PINSIM, "Creating new MCP object in process %i", g_config->getCurrentProcessNum());
      Core * mcp_core = g_core_manager->getCoreFromID(g_config->getMCPCoreNum());
      if (!mcp_core)
      {
         LOG_PRINT_EXPLICIT(-1, PINSIM, "Could not find the MCP's core!");
         LOG_NOTIFY_ERROR();
      }
      Network & mcp_network = *(mcp_core->getNetwork());
      g_MCP = new MCP(mcp_network);
   }
}

void SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallEnterRunModel(ctxt, std);
}

void SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, void *v)
{
   syscallExitRunModel(ctxt, std);
}

void AppStart(void *v)
{
   // FIXME: This function is never called. Use ThreadStart?
   assert(false);
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Application Start.");
}

void ThreadStart(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, void *v)
{
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Thread Start: %d", syscall(__NR_gettid));
}

void ThreadFini(THREADID threadIndex, const CONTEXT *ctxt, INT32 code, void *v)
{
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Thread Fini: %d", syscall(__NR_gettid));
}

int main(int argc, char *argv[])
{
   // Global initialization
   PIN_InitSymbols();

   if (PIN_Init(argc,argv))
      return usage();

   init_globals();

   // Start up helper threads
   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
      g_mcp_runner = StartMCPThread();

   g_sim_thread_runners = SimThreadStart();

   // Instrumentation
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Start of instrumentation.");
   RTN_AddInstrumentFunction(routineCallback, 0);
   PIN_AddSyscallEntryFunction(SyscallEntry, 0);
   PIN_AddSyscallExitFunction(SyscallExit, 0);
   PIN_AddFiniFunction(fini, 0);

   PIN_AddThreadStartFunction(ThreadStart, 0);
   PIN_AddThreadFiniFunction(ThreadFini, 0);
   PIN_AddApplicationStartFunction(AppStart, 0);

   // Just in case ... might not be strictly necessary
   Transport::getSingleton()->barrier();

   // Never returns
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Running program...");
   PIN_StartProgram();

   return 0;
}

