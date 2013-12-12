// Harshad Kasture, Jason Miller, Chris Celio, Charles Gruenwald,
// Nathan Beckmann, George Kurian, David Wentzlaff, James Psota
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
#include "tile_manager.h"
#include "config.h"
#include "tile.h"
#include "syscall_model.h"
#include "thread_manager.h"
#include "thread_scheduler.h"
#include "config_file.hpp"
#include "handle_args.h"
#include "thread_start.h"
#include "pin_config.h"
#include "log.h"
#include "vm_manager.h"
#include "instruction_modeling.h"
#include "progress_trace.h"
#include "clock_skew_management.h"
#include "handle_threads.h"
#include "runtime_energy_monitoring.h"
#include "redirect_memory.h"
#include "handle_syscalls.h"
#include "hash_map.h"
#include <typeinfo>

// lite directories
#include "lite/routine_replace.h"
#include "lite/memory_modeling.h"
#include "lite/handle_syscalls.h"

// ---------------------------------------------------------------
// FIXME: 
// There should be a better place to keep these globals
// -- a PinSimulator class or smthg
bool done_app_initialization = false;
config::ConfigFile *cfg;

// clone stuff
extern int *parent_tidptr;
extern int *child_tidptr;

extern PIN_LOCK clone_memory_update_lock;

map <ADDRINT, string> rtn_map;
PIN_LOCK rtn_map_lock;

HashMap core_map;
// ---------------------------------------------------------------

void printRtn (ADDRINT rtn_addr, bool enter)
{
   PIN_GetLock (&rtn_map_lock, 1);
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
      
   PIN_ReleaseLock (&rtn_map_lock);
}

VOID printInsInfo(CONTEXT* ctxt)
{
   __attribute__((unused)) ADDRINT reg_inst_ptr = PIN_GetContextReg(ctxt, REG_INST_PTR);
   __attribute__((unused)) ADDRINT reg_stack_ptr = PIN_GetContextReg(ctxt, REG_STACK_PTR);

   LOG_PRINT("eip(%#lx), esp(%#lx)", reg_inst_ptr, reg_stack_ptr);
}

void routineCallback(RTN rtn, void *v)
{
   string rtn_name = RTN_Name(rtn);
   
   replaceUserAPIFunction(rtn, rtn_name);

   // ---------------------------------------------------------------

   std::string module = Log::getSingleton()->getModule(__FILE__);
   if (Log::getSingleton()->isEnabled(module.c_str()) &&
       Sim()->getCfg()->getBool("log/stack_trace",false))
   {
      RTN_Open (rtn);
   
      ADDRINT rtn_addr = RTN_Address (rtn);
   
      PIN_GetLock (&rtn_map_lock, 1);
   
      rtn_map.insert (make_pair (rtn_addr, rtn_name));

      PIN_ReleaseLock (&rtn_map_lock);
   
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
}

VOID instructionCallback(INS ins, void *v)
{
   // Debugging Function
   if (Log::getSingleton()->isLoggingEnabled())
   {
      INS_InsertCall(ins, IPOINT_BEFORE,
            AFUNPTR(printInsInfo),
            IARG_CALL_ORDER, CALL_ORDER_FIRST,
            IARG_CONTEXT,
            IARG_END);
   }

   if (Config::getSingleton()->getEnableCoreModeling())
   {
      // Core Performance Modeling
      addInstructionModeling(ins);

      // Progress Trace
      addProgressTrace(ins);
      
      // Clock Skew Management
      addPeriodicSync(ins);
      
      // Scheduling
      addYield(ins);
      
      // Runtime Energy Monitoring
      addRuntimeEnergyMonitoring(ins);
   }

   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {
      // Special handling for futex syscall because of internal Pin lock
      if (INS_IsSyscall(ins))
      {
         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(handleFutexSyscall),
               IARG_CONTEXT,
               IARG_END);
      }
      else
      {
         // Emulate(/Rewrite)  Stack and Memory Operations
         if (rewriteStackOp (ins));
         else rewriteMemOp (ins);
      }
   }
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
   {
      // Special handling for futex syscall because of internal Pin lock
      if (INS_IsSyscall(ins))
      {
         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(lite::handleFutexSyscall),
               IARG_CONTEXT,
               IARG_END);
      }
      else
      {
         // Instrument Memory Operations
         lite::addMemoryModeling(ins);
      }
   }
}

// syscall model wrappers
void initializeSyscallModeling()
{
   PIN_InitLock(&clone_memory_update_lock);
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
   threadStartProgressTrace();

   // Conditions under which we must initialize a core
   // 1) (!done_app_initialization) && (curr_process_num == 0)
   // 2) (done_app_initialization) && (!thread_spawner)

   if (! done_app_initialization)
   {
      // The app is not initialized, start the main thread on the main core on tile 0.
      UInt32 curr_process_num = Sim()->getConfig()->getCurrentProcessNum();

      if (Sim()->getConfig()->getSimulationMode() == Config::LITE)
      {
         LOG_ASSERT_ERROR(curr_process_num == 0, "Lite mode can only be run with 1 process");
         Sim()->getTileManager()->initializeThread(Tile::getMainCoreId(0));
      }
      else // Sim()->getConfig()->getSimulationMode() == Config::FULL
      { 
         ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);
         allocateStackSpace();
         
         if (curr_process_num == 0)
         {
            Sim()->getTileManager()->initializeThread(Tile::getMainCoreId(0));

            ADDRINT reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);
            // 1) Copying over Static Data
            // Get the image first
            PIN_LockClient();
            IMG img = IMG_FindByAddress(reg_eip);
            PIN_UnlockClient();
            copyStaticData(img);

            // 2) Copying over initial stack data
            copyInitialStackData(reg_esp, Tile::getMainCoreId(0));
         }
         else
         {
            tile_id_t tile_id = Sim()->getConfig()->getCurrentThreadSpawnerTileNum();
            Sim()->getTileManager()->initializeThread(Tile::getMainCoreId(tile_id));
            
            Core *core = Sim()->getTileManager()->getCurrentCore();

            // main thread clock is not affected by start-up time of other processes
            core->getTile()->getNetwork()->netRecv(Tile::getMainCoreId(0), core->getId(), SYSTEM_INITIALIZATION_NOTIFY);

            copyInitialStackData(reg_esp, Tile::getMainCoreId(tile_id));
         }
         // Set the current ESP accordingly
         PIN_SetContextReg(ctxt, REG_STACK_PTR, reg_esp);
      }
      
      // All the real initialization is done in 
      // replacement_start at the moment
      done_app_initialization = true;
   }
   else
   {
      // This is NOT the main thread
      // 'application' thread or 'thread spawner'

      if (Sim()->getConfig()->getSimulationMode() == Config::LITE)
      {
         ThreadSpawnRequest req;
         Sim()->getThreadManager()->getThreadToSpawn(&req);
         Sim()->getThreadManager()->dequeueThreadSpawnReq(&req);

         LOG_ASSERT_ERROR(req.destination.tile_id < SInt32(Config::getSingleton()->getApplicationTiles()),
               "req.tile_id(%i), num application cores(%u)", req.destination.tile_id, Config::getSingleton()->getApplicationTiles());
         Sim()->getThreadManager()->onThreadStart(&req);
      }
      else // Sim()->getConfig()->getSimulationMode() == Config::FULL
      {
         ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);
         tile_id_t tile_id = PinConfig::getSingleton()->getTileIDFromStackPtr(reg_esp);
         LOG_PRINT("Got tile %d from stack ptr 0x%x", tile_id, reg_esp);

         core_id_t core_id = PinConfig::getSingleton()->getCoreIDFromStackPtr(reg_esp);
         LOG_PRINT("Got core {%d, %d} from stack ptr 0x%x", core_id.tile_id, core_id.core_type, reg_esp);

         LOG_ASSERT_ERROR(tile_id != -1, "All application threads and thread spawner are cores now");

         if (tile_id == Sim()->getConfig()->getCurrentThreadSpawnerTileNum())
         {
            // 'Thread Spawner' thread
            Sim()->getTileManager()->initializeThread(core_id);
         }
         else
         {
            // 'Application' thread
            ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();

            LOG_ASSERT_ERROR (req != NULL, "ThreadSpawnRequest is NULL !!");

            // This is an application thread
            LOG_ASSERT_ERROR(tile_id == req->destination.tile_id,
                             "Got 2 different tile_ids: req->destination = {%i, %i}, tile_id = %i",
                             req->destination.tile_id, req->destination.core_type, tile_id);

            Sim()->getThreadManager()->onThreadStart(req);
         }

         // Restore the clone syscall arguments
         PIN_SetContextReg (ctxt, REG_GDX, (ADDRINT) parent_tidptr);
         PIN_SetContextReg (ctxt, LEVEL_BASE::REG_R10, (ADDRINT) child_tidptr);

         __attribute__((unused)) Tile *tile = Sim()->getTileManager()->getCurrentTile();
         LOG_ASSERT_ERROR(tile, "tile(NULL)");

         // Copy over thread stack data
         // copySpawnedThreadStackData(reg_esp);

         // Wait to make sure that the spawner has written stuff back to memory
         // FIXME: What is this for(?) This seems arbitrary
         PIN_GetLock (&clone_memory_update_lock, 2);
         PIN_ReleaseLock (&clone_memory_update_lock);
      }
   }

   // Initialize Tile map
   core_map.insert(threadIndex, Sim()->getTileManager()->getCurrentCore());
}

VOID threadFiniCallback(THREADID threadIndex, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
   // De-initialize Tile map
   core_map.erase(threadIndex);
   
   Sim()->getThreadManager()->onThreadExit();
}

int main(int argc, char *argv[])
{
   // ---------------------------------------------------------------
   // FIXME: 
   PIN_InitLock (&rtn_map_lock);
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

   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
      PinConfig::allocate();

   // Instrumentation
   LOG_PRINT("Start of instrumentation.");
   
   // Added thread start/fini callback
   PIN_AddThreadStartFunction(threadStartCallback, 0);
   PIN_AddThreadFiniFunction(threadFiniCallback, 0);

   // Syscall modeling   
   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {
      initializeSyscallModeling();
      PIN_AddSyscallEntryFunction(syscallEnterRunModel, 0);
      PIN_AddSyscallExitFunction(syscallExitRunModel, 0);
      PIN_AddContextChangeFunction(contextChange, NULL);
   }
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
   {
      PIN_AddSyscallEntryFunction(lite::syscallEnterRunModel, 0);
      PIN_AddSyscallExitFunction(lite::syscallExitRunModel, 0);
   }

   // Add RTN instrumentation
   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
      RTN_AddInstrumentFunction(routineCallback, 0);
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
      RTN_AddInstrumentFunction(lite::routineCallback, 0);

   // Add INS instrumentation
   INS_AddInstrumentFunction(instructionCallback, 0);

   initProgressTrace();

   // Add Application Fini function
   PIN_AddFiniFunction(ApplicationExit, 0);

   // Never returns
   LOG_PRINT("Running program...");
   PIN_StartProgram();

   return 0;
}
