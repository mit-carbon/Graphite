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
#include "routine_replace.h"

// FIXME: This list could probably be trimmed down a lot.
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "syscall_model.h"
#include "thread_manager.h"
#include "config_file.hpp"
#include "handle_args.h"
#include "thread_start.h"
#include "pin_config.h"
#include "log.h"
#include "vm_manager.h"
#include "instruction_modeling.h"
#include "progress_trace.h"

#include "redirect_memory.h"
#include "handle_syscalls.h"
#include "opcodes.h"
#include <typeinfo>

// ---------------------------------------------------------------
// FIXME: 
// There should be a better place to keep these globals
// -- a PinSimulator class or smthg
bool done_app_initialization = false;
config::ConfigFile *cfg;

ADDRINT initial_reg_esp;

// clone stuff
extern int *parent_tidptr;
extern struct user_desc *newtls;
extern int *child_tidptr;
extern PIN_LOCK clone_memory_update_lock;
// ---------------------------------------------------------------

// ---------------------------------------------------------------
map <ADDRINT, string> rtn_map;
PIN_LOCK rtn_map_lock;

void printRtn (ADDRINT rtn_addr, bool enter)
{
   GetLock (&rtn_map_lock, 1);
   map<ADDRINT, string>::iterator it = rtn_map.find (rtn_addr);

   string point = enter ? "Enter" : "Exit";
   if (it != rtn_map.end())
   {
      LOG_PRINT ("Stack trace : %s %s", point.c_str(), (it->second).c_str());
   }
   else
   {
      LOG_PRINT ("Stack trace : %s UNKNOWN", point.c_str());
   }
      
   ReleaseLock (&rtn_map_lock);
}

// ---------------------------------------------------------------

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}

void initializeSyscallModeling ()
{
   // Initialize clone stuff
   parent_tidptr = NULL;
   newtls = NULL;
   child_tidptr = NULL;
   InitLock (&clone_memory_update_lock);
}

void routineCallback(RTN rtn, void *v)
{
   string rtn_name = RTN_Name(rtn);
   
   replaceUserAPIFunction(rtn, rtn_name);

   // ---------------------------------------------------------------

   if (Log::getSingleton()->isEnabled(__FILE__) &&
       Sim()->getCfg()->getBool("log/stack_trace",false))
   {
      RTN_Open (rtn);
   
      ADDRINT rtn_addr = RTN_Address (rtn);
   
      GetLock (&rtn_map_lock, 1);
   
      rtn_map.insert (make_pair (rtn_addr, rtn_name));

      ReleaseLock (&rtn_map_lock);
   
      RTN_InsertCall (rtn, IPOINT_BEFORE,
                      AFUNPTR (printRtn),
                      IARG_ADDRINT, rtn_addr,
                      IARG_BOOL, true,
                      IARG_END);

      RTN_InsertCall (rtn, IPOINT_AFTER,
                      AFUNPTR (printRtn),
                      IARG_ADDRINT, rtn_addr,
                      IARG_BOOL, false,
                      IARG_END);

      RTN_Close (rtn);
   }

   // ---------------------------------------------------------------

   if (rtn_name == "CarbonSpawnThreadSpawner")
   {
      RTN_Open (rtn);

      RTN_InsertCall (rtn, IPOINT_BEFORE,
            AFUNPTR (setupCarbonSpawnThreadSpawnerStack),
            IARG_CONTEXT,
            IARG_END);

      RTN_Close (rtn);
   }

   else if (rtn_name == "CarbonThreadSpawner")
   {
      RTN_Open (rtn);

      RTN_InsertCall (rtn, IPOINT_BEFORE,
            AFUNPTR (setupCarbonThreadSpawnerStack),
            IARG_CONTEXT,
            IARG_END);

      RTN_Close(rtn);
   }
   
   // TODO:
   // Commenting out performance modeling code since it causes multiple accesses to memory
   // when we are simulating shared memory. Fix perf model code to not cause any memory accesses
   //  
   // bool did_func_replace = replaceUserAPIFunction(rtn, rtn_name);
   // if (!did_func_replace)
   //    replaceInstruction(rtn, rtn_name);
}

void showInstructionInfo(INS ins)
{
   if (Sim()->getCoreManager()->getCurrentCore()->getId() != 0)
      return;
   printf("\t");
   if(INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins))
      printf("* ");
   else
      printf("  ");
//   printf("%d - %s ", INS_Category(ins), CATEGORY_StringShort(INS_Category(ins)).c_str());
   printf("%x - %s ", INS_Opcode(ins), OPCODE_StringShort(INS_Opcode(ins)).c_str());
   printf(" %s ", INS_Disassemble(ins).c_str());
   printf("\n");
}

VOID instructionCallback (INS ins, void *v)
{
//   showInstructionInfo(ins);

   if (Log::getSingleton()->isLoggingEnabled())
   {
      INS_InsertCall(ins, IPOINT_BEFORE,
                     AFUNPTR(printInsInfo),
                     IARG_CALL_ORDER, CALL_ORDER_FIRST,
                     IARG_CONTEXT,
                     IARG_END);
   }

   if (Config::getSingleton()->getEnablePerformanceModeling())
      addInstructionModeling(ins);

   addProgressTrace(ins);

   if (INS_IsSyscall(ins))
   {
      INS_InsertCall(ins, IPOINT_BEFORE,
            AFUNPTR(handleFutexSyscall),
            IARG_CONTEXT,
            IARG_END);
   }

   // Emulate Stack Operations
   if (rewriteStringOp (ins));
   else if (rewriteStackOp (ins));
   else rewriteMemOp (ins);
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

void ApplicationStart()
{
}

void ApplicationExit(int, void*)
{
   LOG_PRINT("Application exit.");
   Simulator::release();
   shutdownProgressTrace();
   delete cfg;
}

VOID threadStartCallback(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, VOID *v)
{
   ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);

   threadStartProgressTrace();

   // Conditions under which we must initialize a core
   // 1) (!done_app_initialization) && (curr_process_num == 0)
   // 2) (done_app_initialization) && (!thread_spawner)

   if (! done_app_initialization)
   {
      disablePerformanceModelsInCurrentProcess();

#ifdef REDIRECT_MEMORY
      allocateStackSpace();
#endif
      
      UInt32 curr_process_num = Sim()->getConfig()->getCurrentProcessNum();
      ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);

      if (curr_process_num == 0)
      {
         Sim()->getCoreManager()->initializeThread(0);

#ifdef REDIRECT_MEMORY
         ADDRINT reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);
         // 1) Copying over Static Data
         // Get the image first
         PIN_LockClient();
         IMG img = IMG_FindByAddress(reg_eip);
         PIN_UnlockClient();

         LOG_PRINT("Process: 0, Start Copying Static Data");
         copyStaticData(img);
         LOG_PRINT("Process: 0, Finished Copying Static Data");

         // 2) Copying over initial stack data
         LOG_PRINT("Process: 0, Start Copying Initial Stack Data");
         copyInitialStackData(reg_esp, 0);
         LOG_PRINT("Process: 0, Finished Copying Initial Stack Data");
#endif
      }

      else
      {
         core_id_t core_id = Sim()->getConfig()->getCurrentThreadSpawnerCoreNum();
         Sim()->getCoreManager()->initializeThread(core_id);
         
         Core *core = Sim()->getCoreManager()->getCurrentCore();

         // main thread clock is not affected by start-up time of other processes
         core->getNetwork()->netRecv (0, SYSTEM_INITIALIZATION_NOTIFY);

         LOG_PRINT("Process: %i, Start Copying Initial Stack Data");
         copyInitialStackData(reg_esp, core_id);
         LOG_PRINT("Process: %i, Finished Copying Initial Stack Data");
      }
      
      // All the real initialization is done in 
      // replacement_start at the moment
      done_app_initialization = true;

      // Set the current ESP accordingly
      PIN_SetContextReg(ctxt, REG_STACK_PTR, reg_esp);
   }
   else
   {
      // This is NOT the main thread
      // 'application' thread or 'thread spawner'

      core_id_t core_id = PinConfig::getSingleton()->getCoreIDFromStackPtr(reg_esp);

      LOG_ASSERT_ERROR(core_id != -1, "All application threads and thread spawner are cores now");

      if (core_id == Sim()->getConfig()->getCurrentThreadSpawnerCoreNum())
      {
         // 'Thread Spawner' thread
         Sim()->getCoreManager()->initializeThread(core_id);
      }
      else
      {
         // 'Application' thread
         ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();

         LOG_ASSERT_ERROR (req != NULL, "ThreadSpawnRequest is NULL !!");

         // This is an application thread
         LOG_ASSERT_ERROR(core_id == req->core_id, "Got 2 different core_ids: req->core_id = %i, core_id = %i", req->core_id, core_id);

         Sim()->getThreadManager()->onThreadStart(req);
      }
     
      // Restore the clone syscall arguments
      PIN_SetContextReg (ctxt, REG_GDX, (ADDRINT) parent_tidptr);
      PIN_SetContextReg (ctxt, REG_GSI, (ADDRINT) newtls);
      PIN_SetContextReg (ctxt, REG_GDI, (ADDRINT) child_tidptr);

      Core *core = Sim()->getCoreManager()->getCurrentCore();
      assert (core);

      // Wait to make sure that the spawner has written stuff back to memory
      // FIXME: What is this for(?) This seems arbitrary
      GetLock (&clone_memory_update_lock, 2);
      ReleaseLock (&clone_memory_update_lock);
   }
}

VOID threadFiniCallback(THREADID threadIndex, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
   Sim()->getThreadManager()->onThreadExit();
}

int main(int argc, char *argv[])
{
   // ---------------------------------------------------------------
   // FIXME: 
   InitLock (&rtn_map_lock);
   // ---------------------------------------------------------------

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

   PinConfig::allocate();

   VMManager::allocate();

   // Instrumentation
   LOG_PRINT("Start of instrumentation.");
   RTN_AddInstrumentFunction(routineCallback, 0);

   PIN_AddThreadStartFunction (threadStartCallback, 0);
   PIN_AddThreadFiniFunction (threadFiniCallback, 0);
   
   if(cfg->getBool("general/enable_syscall_modeling"))
   {
      initializeSyscallModeling();

      PIN_AddSyscallEntryFunction(SyscallEntry, 0);
      PIN_AddSyscallExitFunction(SyscallExit, 0);
      PIN_AddContextChangeFunction (contextChange, NULL);
   }

   if (cfg->getBool("general/enable_shared_mem"))
   {
      INS_AddInstrumentFunction (instructionCallback, 0);
   }

   initProgressTrace();

   PIN_AddFiniFunction(ApplicationExit, 0);

   // Just in case ... might not be strictly necessary
   Transport::getSingleton()->barrier();

   // Never returns
   LOG_PRINT("Running program...");
   PIN_StartProgram();

   return 0;
}
