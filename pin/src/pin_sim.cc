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

#include "user_space_wrappers.h"
#include "shmem_debug_helper.h"

#define LOG_DEFAULT_RANK    core_id
#define LOG_DEFAULT_MODULE  PINSIM

CoreManager *g_core_manager = NULL;
Config *g_config = NULL;
MCP *g_MCP = NULL;
MCPRunner * g_mcp_runner = NULL;
SimThreadRunner * g_sim_thread_runners = NULL;
Log *g_log = NULL;
ShmemDebugHelper *g_shmem_debug_helper = NULL;

//TODO only here for debugging ins in runModel

struct InsInfo
{
   IntPtr ip_address;
   OPCODE opcode;
   bool is_sys_call;
   bool is_sys_enter;
   SYSCALL_STANDARD sys_call_std;

   bool next_is_valid;
   OPCODE next_opcode;
   bool next_is_sys_call;
   bool next_is_sys_enter;
   SYSCALL_STANDARD next_sys_call_std;
};

//FIXME
//PIN_LOCK g_lock1;
//PIN_LOCK g_lock2;

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}

/* ===================================================================== */
/* For instrumentation / modeling */
/* ===================================================================== */


void runModels(IntPtr dcache_ld_addr, IntPtr dcache_ld_addr2, UINT32 dcache_ld_size,
               IntPtr dcache_st_addr, UINT32 dcache_st_size,
               PerfModelIntervalStat* *stats,
               REG *reads, UINT32 num_reads, REG *writes, UINT32 num_writes,
               bool do_network_modeling, bool do_icache_modeling,
               bool do_dcache_read_modeling, bool is_dual_read,
               bool do_dcache_write_modeling, bool do_bpred_modeling, bool do_perf_modeling,
               //               bool check_scoreboard)
               bool check_scoreboard,
               void* ins_info_array)
{
   int core_id = g_core_manager->getCurrentCoreID();

   if (core_id > -1)
   {
      Core *current_core = g_core_manager->getCoreFromID(core_id);

      // InsInfo* ins_info = ((InsInfo**) ins_info_array)[core_id];
      // stringstream ss;
      // ss << "OPCODE$ = " << LEVEL_CORE::OPCODE_StringShort(ins_info->opcode) << " (" << ins_info->opcode << ") ";
      // LOG_PRINT(ss.str());

      // This must be consistent with the behavior of
      // insertInstructionModelingCall.

      // Trying to prevent using NULL stats. This happens when
      // instrumenting portions of the main thread.
      bool skip_modeling = (core_id < 0) ||
                           ((check_scoreboard || do_perf_modeling || do_icache_modeling) && stats == NULL);

      if (skip_modeling)
         return;

      assert((UInt32)core_id < g_config->getNumLocalCores());
      assert(!do_network_modeling);
      assert(!do_bpred_modeling);

      // flag passed to perfModelRun to prevent it from double-counting things on successive
      // calls to perfModelRun
      bool firstCallInIntvl = true;


      // JME: think this was an error; want some other model on if icache modeling is on
      //   assert( !(!do_icache_modeling && (do_network_modeling ||
      //                                  do_dcache_read_modeling || do_dcache_write_modeling ||
      //                                  do_bpred_modeling || do_perf_modeling)) );

      // no longer needed since we guarantee icache model will run at basic block boundary
      // assert( !do_icache_modeling || (do_network_modeling ||
      //                                do_dcache_read_modeling || do_dcache_write_modeling ||
      //                                do_bpred_modeling || do_perf_modeling) );

      if (do_icache_modeling)
      {
         for (UINT32 i = 0; i < (stats[core_id]->inst_trace.size()); i++)
         {
            // first = PC, second = size
            bool i_hit = current_core->icacheRunLoadModel(stats[core_id]->inst_trace[i].first,
                         stats[core_id]->inst_trace[i].second);
            if (do_perf_modeling)
            {
               current_core->getPerfModel()->logICacheLoadAccess(stats[core_id], i_hit);
            }
         }
      }

      // this check must go before everything but the icache check
      assert(!check_scoreboard || do_perf_modeling);

      if (check_scoreboard)
      {
         // it's not possible to delay the evaluation of the performance impact for these.
         // get the cycle counter up to date then account for dependency stalls
         current_core->getPerfModel()->run(stats[core_id], reads, num_reads, firstCallInIntvl);
         firstCallInIntvl = false;
      }

      if (do_dcache_read_modeling)
      {
         if (g_shmem_debug_helper->aliasEnabled())
         {
            //FIXME
            g_shmem_debug_helper->aliasReadModeling();
         }
         else
         {
            // FIXME: This should actually be a UINT32 which tells how many read misses occured

            char data_ld_buffer[dcache_ld_size];
            //TODO HARSHAD sharedmemory will fill ld_buffer
            bool d_hit = current_core->dcacheRunModel(CacheBase::k_ACCESS_TYPE_LOAD, dcache_ld_addr, data_ld_buffer, dcache_ld_size);
            // bool d_hit = dcacheRunLoadModel(dcache_ld_addr, dcache_ld_size);

            if (do_perf_modeling)
            {
               current_core->getPerfModel()->run(stats[core_id], d_hit, writes, num_writes, firstCallInIntvl);
               firstCallInIntvl = false;
            }

            if (is_dual_read)
            {
               char data_ld_buffer_2[dcache_ld_size];
               //TODO HARSHAD sharedmemory will fill ld_buffer
               bool d_hit2 = current_core->dcacheRunModel(CacheBase::k_ACCESS_TYPE_LOAD, dcache_ld_addr2, data_ld_buffer_2, dcache_ld_size);
               // bool d_hit2 = dcacheRunLoadModel(dcache_ld_addr2, dcache_ld_size);
               if (do_perf_modeling)
               {
                  current_core->getPerfModel()->run(stats[core_id], d_hit2, writes, num_writes, firstCallInIntvl);
                  firstCallInIntvl = false;
               }
            }

            // cerr << "[" << core_id << "] dCache READ Modeling: Over " << endl;
         }

      }
      else
      {
         assert(dcache_ld_addr == (IntPtr) NULL);
         assert(dcache_ld_addr2 == (IntPtr) NULL);
         assert(dcache_ld_size == 0);
      }

      if (do_dcache_write_modeling)
      {
         if (g_shmem_debug_helper->aliasEnabled())
         {
            g_shmem_debug_helper->aliasWriteModeling();

         }
         else
         {
            // FIXME: This should actually be a UINT32 which tells how many write misses occurred
            char data_st_buffer[dcache_ld_size];

            //TODO Harshad: st buffer needs to be written
            //TODO Harshad: shared memory expects all data_buffers to be pre-allocated
            bool d_hit = current_core->dcacheRunModel(CacheBase::k_ACCESS_TYPE_STORE, dcache_st_addr, data_st_buffer, dcache_st_size);
            if (do_perf_modeling)
            {
               current_core->getPerfModel()->logDCacheStoreAccess(stats[core_id], d_hit);
            }
            //cerr << "[" << core_id << "] dCache WRITE Modeling: RELEASED LOCKS " << endl;
         }
      }
      else
      {
         assert(dcache_st_addr == (IntPtr) NULL);
         assert(dcache_st_size == 0);
      }

      // this should probably go last
      if (do_perf_modeling)
      {
         current_core->getPerfModel()->run(stats[core_id], firstCallInIntvl);
         firstCallInIntvl = false;
      }
   }

} //end of runModels

PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine,
      const INS& start_ins, const INS& end_ins)
{
   // using zero is a dirty hack
   // assumes its safe to use core zero to generate perfmodels for all cores
   assert(g_config->getNumLocalCores() > 0);

   //FIXME: These stats should be deleted at the end of execution
   PerfModelIntervalStat* *array = new PerfModelIntervalStat*[g_config->getNumLocalCores()];

   for (UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
      array[i] = g_core_manager->getCoreFromID(0)->getPerfModel()->analyzeInterval(parent_routine, start_ins, end_ins);

   return array;
}


bool insertInstructionModelingCall(const string& rtn_name, const INS& start_ins,
                                   const INS& ins, bool is_rtn_ins_head, bool is_bbl_ins_head,
                                   bool is_bbl_ins_tail, bool is_potential_load_use)
{

   // added constraint that perf model must be on
   bool check_scoreboard         = g_knob_enable_performance_modeling &&
                                   g_knob_enable_dcache_modeling &&
                                   !g_knob_dcache_ignore_loads &&
                                   is_potential_load_use;

   //FIXME: check for API routine
   bool do_network_modeling      = g_knob_enable_network_modeling && is_rtn_ins_head;
   bool do_dcache_read_modeling  = g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_loads &&
                                   INS_IsMemoryRead(ins);
   bool do_dcache_write_modeling = g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_stores &&
                                   INS_IsMemoryWrite(ins);
   bool do_bpred_modeling        = g_knob_enable_bpred_modeling && INS_IsBranchOrCall(ins);

   //TODO: if we run on multiple machines we need shared memory
   //TODO: if we run on multiple machines we need syscall_modeling

   // If we are doing any other type of modeling then we need to do icache modeling
   bool do_icache_modeling       = g_knob_enable_icache_modeling &&
                                   (do_network_modeling || do_dcache_read_modeling ||
                                    do_dcache_write_modeling || do_bpred_modeling || is_bbl_ins_tail ||
                                    check_scoreboard);

   bool do_perf_modeling         = g_knob_enable_performance_modeling &&
                                   (do_network_modeling || do_dcache_read_modeling ||
                                    do_dcache_write_modeling || do_icache_modeling ||
                                    do_bpred_modeling || is_bbl_ins_tail || check_scoreboard);


   // Exit early if we aren't modeling anything
   if (!do_network_modeling && !do_icache_modeling && !do_dcache_read_modeling  &&
         !do_dcache_write_modeling && !do_bpred_modeling && !do_perf_modeling)
   {
      return false;
   }

   assert(!do_network_modeling);
   assert(!do_bpred_modeling);

   //this flag may or may not get used
   bool is_dual_read = INS_HasMemoryRead2(ins);

   PerfModelIntervalStat* *stats;
   INS end_ins = INS_Next(ins);
   // stats also needs to get allocated if icache modeling is turned on
   stats = (do_perf_modeling || do_icache_modeling || check_scoreboard) ?
           perfModelAnalyzeInterval(rtn_name, start_ins, end_ins) :
           NULL;


   // Build a list of read registers if relevant
   UINT32 num_reads = 0;
   REG *reads = NULL;
   if (g_knob_enable_performance_modeling)
   {
      num_reads = INS_MaxNumRRegs(ins);
      reads = new REG[num_reads];
      for (UINT32 i = 0; i < num_reads; i++)
      {
         reads[i] = INS_RegR(ins, i);
      }
   }

   InsInfo** ins_info_array = NULL;
#ifdef PRINOUT_FLAGS
   /**** TAKE OUT LATER TODO *****/
   //only for debugging instruction in runModels
   //at run time
   //provide each core it's own copy of the ins_info
   ins_info_array = (InsInfo**) malloc(sizeof(InsInfo*));
   assert(ins_info_array != NULL);
   int NUMBER_OF_CORES = 2;

   for (int i=0; i < NUMBER_OF_CORES; i++)
   {
      ins_info_array[i] = (InsInfo*) malloc(sizeof(InsInfo));
      InsInfo* ins_info = ins_info_array[i];

      ins_info->ip_address = INS_Address(ins);
      ins_info->opcode = INS_Opcode(ins);
      ins_info->is_sys_call = INS_IsSyscall(ins);
      ins_info->is_sys_enter = INS_IsSysenter(ins);
      ins_info->sys_call_std = INS_SyscallStd(ins);

      INS next_ins = INS_Next(ins);
      if (!INS_Valid(next_ins))
      {
         ins_info->next_is_valid = false;
      }
      else
      {
         ins_info->next_is_valid = true;
         ins_info->next_opcode = INS_Opcode(next_ins);
         ins_info->next_is_sys_call = INS_IsSyscall(next_ins);
         ins_info->next_is_sys_enter = INS_IsSysenter(next_ins);
         ins_info->next_sys_call_std = INS_SyscallStd(next_ins);
      }
   }
#endif

   // Build a list of write registers if relevant
   UINT32 num_writes = 0;
   REG *writes = NULL;
   if (g_knob_enable_performance_modeling)
   {
      num_writes = INS_MaxNumWRegs(ins);
      writes = new REG[num_writes];
      for (UINT32 i = 0; i < num_writes; i++)
      {
         writes[i] = INS_RegW(ins, i);
      }
   }

   //for building the arguments to the function which dispatches calls to the various modelers
   IARGLIST args = IARGLIST_Alloc();

   // Properly add the associated addresses for the argument call
   if (do_dcache_read_modeling)
   {
      IARGLIST_AddArguments(args, IARG_MEMORYREAD_EA, IARG_END);

      // If it's a dual read then we need the read2 ea otherwise, null
      if (is_dual_read)
         IARGLIST_AddArguments(args, IARG_MEMORYREAD2_EA, IARG_END);
      else
         IARGLIST_AddArguments(args, IARG_ADDRINT, (IntPtr) NULL, IARG_END);

      IARGLIST_AddArguments(args, IARG_MEMORYREAD_SIZE, IARG_END);
   }
   else
   {
      IARGLIST_AddArguments(args, IARG_ADDRINT, (IntPtr) NULL, IARG_ADDRINT, (IntPtr) NULL, IARG_UINT32, 0, IARG_END);
   }

   // Do this after those first three
   if (do_dcache_write_modeling)
      IARGLIST_AddArguments(args,IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
   else
      IARGLIST_AddArguments(args,IARG_ADDRINT, (IntPtr) NULL, IARG_UINT32, 0, IARG_END);

   // Now pass on our values for the appropriate models
   IARGLIST_AddArguments(args,
                         // perf modeling
                         IARG_PTR, (void *) stats,
                         IARG_PTR, (void *) reads, IARG_UINT32, num_reads,
                         IARG_PTR, (void *) writes, IARG_UINT32, num_writes,
                         // model-enable flags
                         IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling,
                         IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                         IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling,
                         IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard,
                         IARG_PTR, (void *) ins_info_array,
                         IARG_END);

   //   IARGLIST_AddArguments(args, IARG_PTR, (void *) ins_info);

   INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) runModels, IARG_IARGLIST, args, IARG_END);
   IARGLIST_Free(args);

   return true;
}


void getPotentialLoadFirstUses(const RTN& rtn, set<INS>& ins_uses)
{
   //FIXME: does not currently account for load registers live across rtn boundaries

   UINT32 rtn_ins_count = 0;

   // find the write registers not consumed within the basic block for instructs which read mem
   BitVector bbl_dest_regs(LEVEL_BASE::REG_LAST);
   BitVector dest_regs(LEVEL_BASE::REG_LAST);

   for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl))
   {
      rtn_ins_count += BBL_NumIns(bbl);

      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
      {

         // remove from list of outstanding regs if is a use
         for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++)
         {
            REG r = INS_RegR(ins, i);
            assert(0 <= r && r < LEVEL_BASE::REG_LAST);
            if (bbl_dest_regs.at(r))
            {
               bbl_dest_regs.clear(r);
               ins_uses.insert(ins);
            }
         }

         if (!INS_IsMemoryRead(ins))
         {

            // remove from list of outstanding regs if is overwrite
            for (UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++)
            {
               REG r = INS_RegW(ins, i);
               bbl_dest_regs.clear(r);
            }

         }
         else
         {

            // add to list if writing some function of memory read; remove other writes
            // FIXME: not all r will be dependent on memory; need to remove those
            for (UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++)
            {
               REG r = INS_RegW(ins, i);
               bbl_dest_regs.set(r);
            }

         }

      }

      dest_regs.set(bbl_dest_regs);
   }

   // find the instructs that source these registers
   for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl))
   {
      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
      {
         for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++)
         {
            REG r = INS_RegR(ins, i);
            assert(0 <= r && r < LEVEL_BASE::REG_LAST);
            if (dest_regs.at(r))
               ins_uses.insert(ins);
         }
      }
   }

#if 0
   cerr << "Routine " << RTN_Name(rtn) << endl;
   cerr << "  instrumented " << ins_uses.size() << " of " << rtn_ins_count << endl;
   for (set<INS>::iterator it = ins_uses.begin(); it != ins_uses.end(); it++)
   {
      cerr << "  " << INS_Disassemble(*it) << endl;
   }
#endif

}

// This function will create a separate context for the MCP to run (i.e. it spawns the MCP)
MCPRunner* StartMCPThread()
{
   MCPRunner *runner = new MCPRunner(g_MCP);
   OS_SERVICES::ITHREAD *my_thread_p;
   my_thread_p = OS_SERVICES::ITHREADS::GetSingleton()->Spawn(4096, runner);
   assert(my_thread_p);
   return runner;
}

/* ===================================================================== */

bool replaceUserAPIFunction(RTN& rtn, string& name)
{

   AFUNPTR msg_ptr = NULL;
   PROTO proto = NULL;

   //FIXME added by cpc as a hack to get around calling Network for finished cores
   if (name == "CAPI_Initialize")
   {
      msg_ptr = AFUNPTR(SimInitializeThread);
   }
   else if (name == "CAPI_Initialize_FreeRank")
   {
      msg_ptr = AFUNPTR(SimInitializeThreadFreeRank);
   }
   else if (name == "CAPI_rank")
   {
      msg_ptr = AFUNPTR(SimGetCoreID);
   }
   else if (name == "CAPI_message_send_w")
   {
      msg_ptr = AFUNPTR(SimSendW);
   }
   else if (name == "CAPI_message_receive_w")
   {
      msg_ptr = AFUNPTR(SimRecvW);
   }
   else if (name == "mutexInit")
   {
      msg_ptr = AFUNPTR(SimMutexInit);
   }
   else if (name == "mutexLock")
   {
      msg_ptr = AFUNPTR(SimMutexLock);
   }
   else if (name == "mutexUnlock")
   {
      msg_ptr = AFUNPTR(SimMutexUnlock);
   }
   else if (name == "condInit")
   {
      msg_ptr = AFUNPTR(SimCondInit);
   }
   else if (name == "condWait")
   {
      msg_ptr = AFUNPTR(SimCondWait);
   }
   else if (name == "condSignal")
   {
      msg_ptr = AFUNPTR(SimCondSignal);
   }
   else if (name == "condBroadcast")
   {
      msg_ptr = AFUNPTR(SimCondBroadcast);
   }
   else if (name == "barrierInit")
   {
      msg_ptr = AFUNPTR(SimBarrierInit);
   }
   else if (name == "barrierWait")
   {
      msg_ptr = AFUNPTR(SimBarrierWait);
   }
   //FIXME
#if 0
   else if (name == "CAPI_debugSetMemState")
   {
      msg_ptr = AFUNPTR(chipDebugSetMemState);
   }
   else if (name == "CAPI_debugAssertMemState")
   {
      msg_ptr = AFUNPTR(chipDebugAssertMemState);
   }
   else if (name == "CAPI_alias")
   {
      msg_ptr = AFUNPTR(chipAlias);
   }
#endif

   if (msg_ptr == AFUNPTR(SimGetCoreID)
         || (msg_ptr == AFUNPTR(SimInitializeThreadFreeRank))
         || (msg_ptr == AFUNPTR(SimMutexInit))
         || (msg_ptr == AFUNPTR(SimMutexLock))
         || (msg_ptr == AFUNPTR(SimMutexUnlock))
         || (msg_ptr == AFUNPTR(SimCondInit))
         || (msg_ptr == AFUNPTR(SimCondSignal))
         || (msg_ptr == AFUNPTR(SimCondBroadcast))
         || (msg_ptr == AFUNPTR(SimBarrierWait))
      )
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimSendW)) || (msg_ptr == AFUNPTR(SimRecvW)))
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(CAPI_endpoint_t),
                             PIN_PARG(CAPI_endpoint_t),
                             PIN_PARG(char*),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimCondWait)))
   {
      proto = PROTO_Allocate(PIN_PARG(void),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG(int*),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimBarrierInit)))
   {
      proto = PROTO_Allocate(PIN_PARG(void),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimInitializeThread)))
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   //FIXME
#if 0
   else if ((msg_ptr == AFUNPTR(chipDebugSetMemState)) || (msg_ptr == AFUNPTR(chipDebugAssertMemState))
            || (msg_ptr == AFUNPTR(chipAlias)))
   {
      RTN_Replace(rtn, msg_ptr);
   }
#endif
   return false;
}

void routine(RTN rtn, void *v)
{
   string rtn_name = RTN_Name(rtn);
   bool did_func_replace = replaceUserAPIFunction(rtn, rtn_name);

   if (!did_func_replace)
   {
      RTN_Open(rtn);
      INS rtn_head = RTN_InsHead(rtn);
      bool is_rtn_ins_head = true;
      set<INS> ins_uses;

      if (g_knob_enable_performance_modeling && g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_loads)
      {
         getPotentialLoadFirstUses(rtn, ins_uses);
      }

      // Add instrumentation to each basic block for modeling
      for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl))
      {
         UINT32 inst_offset = 0;
         UINT32 last_offset = BBL_NumIns(bbl);

         INS start_ins = BBL_InsHead(bbl);
         INS ins;

         for (ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
         {
            INS next = INS_Next(ins);
            assert(!INS_Valid(next) || (INS_Address(next) > INS_Address(ins)));
            assert(!is_rtn_ins_head || (ins==rtn_head));

            set<INS>::iterator it = ins_uses.find(ins);
            bool is_potential_load_use = (it != ins_uses.end());
            if (is_potential_load_use)
            {
               ins_uses.erase(it);
            }

            bool instrumented =
               insertInstructionModelingCall(rtn_name, start_ins, ins, is_rtn_ins_head, (inst_offset == 0),
                                             !INS_Valid(next), is_potential_load_use);
            if (instrumented)
            {
               start_ins = INS_Next(ins);
            }
            ++inst_offset;
            is_rtn_ins_head = false;
         }
         assert(inst_offset == last_offset);
      }

      RTN_Close(rtn);
   }
}

// syscall model wrappers
void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int core_id = g_core_manager->getCurrentCoreID();

   if (core_id >= 0)
      g_core_manager->getCoreFromID(core_id)->getSyscallMdl()->runEnter(ctx, syscall_standard);
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int core_id = g_core_manager->getCurrentCoreID();

   if (core_id >= 0)
      g_core_manager->getCoreFromID(core_id)->getSyscallMdl()->runExit(ctx, syscall_standard);
}

/* ===================================================================== */

void fini(int code, void * v)
{
   LOG_PRINT_EXPLICIT(-1, PINSIM, "fini start");

   // Make sure all other processes are finished before we start tearing down stuffs
   Transport::ptBarrier();

   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
      g_MCP->finish();

   SimThreadQuit();

   Transport::ptFinish();

   g_core_manager->outputSummary();

   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
      delete g_mcp_runner;

   delete [] g_sim_thread_runners;

   delete g_core_manager;

   LOG_PRINT_EXPLICIT(-1, PINSIM, "fini end");

   delete g_log;
}

/* ===================================================================== */

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

   // NOTE: transport and queues must be inited before the core_manager
   // I think this one wants a per-process core count it adds one
   // on it's own for the mcp
   Transport::ptGlobalInit();

   g_shmem_debug_helper = new ShmemDebugHelper();

   // I think this one probably wants a total core count
   g_core_manager = new CoreManager();

   // Note the MCP has a dependency on the transport layer and the core_manager.
   // Only create an MCP on the correct process.
   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
   {
      LOG_PRINT_EXPLICIT(-1, PINSIM, "Creating new MCP object in process %i", g_config->getCurrentProcessNum());
      Core * mcp_core = g_core_manager->getCoreFromID(g_config->getTotalCores()-1);
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
   //RTN_AddInstrumentFunction(routine, 0);
   //PIN_AddSyscallEntryFunction(SyscallEntry, 0);
   //PIN_AddSyscallExitFunction(SyscallExit, 0);
   PIN_AddFiniFunction(fini, 0);

   // Just in case ... might not be strictly necessary
   Transport::ptBarrier();

   // Never returns
   LOG_PRINT_EXPLICIT(-1, PINSIM, "Running program...");
   PIN_StartProgram();

   return 0;
}

