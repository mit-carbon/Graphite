// Jonathan Eastep, Jason Miller, Harshad Kasture, Jim Psota
// 04.08.08
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
//


#include <iostream>
#include <assert.h>
#include <set>

#include "pin.H"
#include "utils.h"
#include "bit_vector.h"
#include "config.h"
#include "chip.h"
#include "cache.h"
#include "ocache.h"
#include "perfmdl.h"
#include "knobs.h"

#define INSTRUMENT_ALLOWED_FUNCTIONS

//#define PRINTOUT_FLAGS

Chip *g_chip = NULL;
Config *g_config = NULL;
SyscallServer *g_syscall_server = NULL;

//TODO only here for debugging ins in runModel

struct InsInfo {
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

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}

PIN_LOCK dcache_read_lock;
PIN_LOCK dcache_write_lock;

/* ===================================================================== */
/* For instrumentation / modeling */
/* ===================================================================== */


VOID runModels(ADDRINT dcache_ld_addr, ADDRINT dcache_ld_addr2, UINT32 dcache_ld_size,
               ADDRINT dcache_st_addr, UINT32 dcache_st_size,
               PerfModelIntervalStat *stats,
               REG *reads, UINT32 num_reads, REG *writes, UINT32 num_writes, 
               bool do_network_modeling, bool do_icache_modeling, 
               bool do_dcache_read_modeling, bool is_dual_read, 
               bool do_dcache_write_modeling, bool do_bpred_modeling, bool do_perf_modeling, 
//               bool check_scoreboard)
               bool check_scoreboard,
					VOID* ins_info)
{
   //cout << "parent = " << stats->parent_routine << endl;
 
   int rank;
   chipRank(&rank);



#ifdef PRINTOUT_FLAGS
	if(rank == 0) {

		cout << " ----------------------------------" << endl;
		cout <<  " CORE#0 START  running runModels!" << endl;
		cout << "Is ins_info NULL? : " << (ins_info == NULL) << endl;
	}
	
	if(rank == 1) {
		cout << " ----------------------------------" << endl;
		cout << " CORE#1 START running runModels!" << endl;
		cout << "Is ins_info NULL? : " << (ins_info == NULL) << endl;
	}

	if(rank > -1) 
	{

		cout << "[" << rank << "] Is ins_info NULL? : " << (ins_info == NULL) << endl;
		if( true || ((InsInfo*) ins_info)->opcode == 608 || ((InsInfo*) ins_info)->next_opcode == 608) 
		{
			cout << "[" << rank << "] PINSIM -: OPCODE$     = " << LEVEL_CORE::OPCODE_StringShort(((InsInfo*) ins_info)->opcode) << " (" << ((InsInfo*) ins_info)->opcode << ") " << endl;
			cout << "[" << rank << "] PINSIM -: IS SYSCALL  = " << ((InsInfo*) ins_info)->is_sys_call << endl;
			cout << "[" << rank << "] PINSIM -: SYSCALL STD = " << ((InsInfo*) ins_info)->sys_call_std << endl;
			cout << "[" << rank << "] PINSIM -: IS SYSENTER = " << ((InsInfo*) ins_info)->is_sys_enter << endl;
			cout << "----------" << endl;
			
			if(((InsInfo*) ins_info)->next_is_valid) 
			{
				cout << "[" << rank << "] PINSIM -: NEXT_OPCODE$     = " << LEVEL_CORE::OPCODE_StringShort(((InsInfo*) ins_info)->next_opcode) << " (" << ((InsInfo*) ins_info)->next_opcode << ") " << endl;
				cout << "[" << rank << "] PINSIM -: NEXT_IS SYSCALL  = " << ((InsInfo*) ins_info)->next_is_sys_call << endl;
				cout << "[" << rank << "] PINSIM -: NEXT_SYSCALL STD = " << ((InsInfo*) ins_info)->next_sys_call_std << endl;
				cout << "[" << rank << "] PINSIM -: NEXT_IS SYSENTER = " << ((InsInfo*) ins_info)->next_is_sys_enter << endl;
			}
		}
		
		cout << "[" << rank << "] Is ins_info NULL? : " << (ins_info == NULL) << endl;
	}

#endif

	if(rank > -1)
   {
      assert( !do_network_modeling );
      assert( !do_bpred_modeling );
//=======
//>>>>>>> master:pin/src/pin_sim.cc

   // This must be consistent with the behavior of
   // insertInstructionModelingCall.

   // Trying to prevent using NULL stats. This happens when
   // instrumenting portions of the main thread.
   bool skip_modeling = (rank < 0) ||
     ((check_scoreboard || do_perf_modeling || do_icache_modeling) && stats == NULL);

   if (skip_modeling)
     return;

   assert( rank >= 0 && rank < g_chip->getNumModules() );

   assert( !do_network_modeling );
   assert( !do_bpred_modeling );

   // JME: think this was an error; want some other model on if icache modeling is on
   //   assert( !(!do_icache_modeling && (do_network_modeling || 
   //                                  do_dcache_read_modeling || do_dcache_write_modeling ||
   //                                  do_bpred_modeling || do_perf_modeling)) );

   // no longer needed since we guarantee icache model will run at basic block boundary
   //assert( !do_icache_modeling || (do_network_modeling || 
   //                                do_dcache_read_modeling || do_dcache_write_modeling ||
   //                                do_bpred_modeling || do_perf_modeling) );

   if ( do_icache_modeling )
     {
       for (UINT32 i = 0; i < (stats->inst_trace.size()); i++)
         {
	   // first = PC, second = size
	   bool i_hit = icacheRunLoadModel(stats->inst_trace[i].first,
					   stats->inst_trace[i].second);
	   if ( do_perf_modeling ) {
	     perfModelLogICacheLoadAccess(stats, i_hit);
	   }
         }
     }

   // this check must go before everything but the icache check
   assert( !check_scoreboard || do_perf_modeling );
   if ( check_scoreboard )
     {
       // it's not possible to delay the evaluation of the performance impact for these. 
       // get the cycle counter up to date then account for dependency stalls
       perfModelRun(stats, reads, num_reads); 
     }

   if ( do_dcache_read_modeling )
     {
       // it's not possible to delay the evaluation of the performance impact for these. 
       // get cycle count up to date so time stamp for when miss is ready is correct

//		cerr << "[" << rank << "] dCache READ Modeling: before getting locks " << endl;
//       GetLock(&dcache_read_lock, 1);
//       GetLock(&dcache_write_lock, 1);

//			cerr << "[" << rank << "] dCache READ Modeling: GOT LOCKS " << endl;
       bool d_hit = dcacheRunLoadModel(dcache_ld_addr, dcache_ld_size);
       if ( do_perf_modeling ) {
           perfModelRun(stats, d_hit, writes, num_writes);
       }

       if ( is_dual_read ) {
           bool d_hit2 = dcacheRunLoadModel(dcache_ld_addr2, dcache_ld_size);
           if ( do_perf_modeling ) {
               perfModelRun(stats, d_hit2, writes, num_writes);
           }
       }

//       ReleaseLock(&dcache_write_lock);
//       ReleaseLock(&dcache_read_lock);
//		 cerr << "[" << rank << "] dCache READ Modeling: RELEASED LOCKS " << endl;
     } 
   else 
     {
       assert(dcache_ld_addr == (ADDRINT) NULL);
       assert(dcache_ld_addr2 == (ADDRINT) NULL);
       assert(dcache_ld_size == 0);
     }

   if ( do_dcache_write_modeling )
     {
//		 cerr << "[" << rank << "] dCache WRITE Modeling: before locks" << endl;
//       GetLock(&dcache_read_lock, 1);
//      GetLock(&dcache_write_lock, 1);
//		 cerr << "[" << rank << "] dCache WRITE Modeling: GOT LOCKS " << endl;
       bool d_hit = dcacheRunStoreModel(dcache_st_addr, dcache_st_size);
       if ( do_perf_modeling )
         { 
	   perfModelLogDCacheStoreAccess(stats, d_hit); 
         }
//       ReleaseLock(&dcache_write_lock);
//       ReleaseLock(&dcache_read_lock);
//		 cerr << "[" << rank << "] dCache WRITE Modeling: RELEASED LOCKS " << endl;
     } 
   else 
     {
       assert(dcache_st_addr == (ADDRINT) NULL);
       assert(dcache_st_size == 0);
     }

   // this should probably go last
   if ( do_perf_modeling )
     {
       perfModelRun(stats);
     }
   }

#ifdef PRINTOUT_FLAGS
	if(rank == 0) {
		cout <<  " CORE#0 I'm FINISHED w/ runModels!" << endl;
		cout << " ----------------------------------" << endl;
	}
	
	if(rank == 1) {
		cout << " CORE#1 I'm FINISHED w/ runModels!" << endl;
		cout << " ----------------------------------" << endl;
	}
#endif
} //end of runModels

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
                                   ( do_network_modeling || do_dcache_read_modeling || 
                                     do_dcache_write_modeling || do_bpred_modeling || is_bbl_ins_tail || 
                                     check_scoreboard );

   bool do_perf_modeling         = g_knob_enable_performance_modeling && 
                                   ( do_network_modeling || do_dcache_read_modeling || 
                                     do_dcache_write_modeling || do_icache_modeling || 
                                     do_bpred_modeling || is_bbl_ins_tail || check_scoreboard );


   // Exit early if we aren't modeling anything
   if ( !do_network_modeling && !do_icache_modeling && !do_dcache_read_modeling  &&
        !do_dcache_write_modeling && !do_bpred_modeling && !do_perf_modeling)
   {
      return false;
   }

   assert( !do_network_modeling );
   assert( !do_bpred_modeling );

   //this flag may or may not get used
   bool is_dual_read = INS_HasMemoryRead2(ins);

   PerfModelIntervalStat *stats;
   INS end_ins = INS_Next(ins);
   // stats also needs to get allocated if icache modeling is turned on
   stats = (do_perf_modeling || do_icache_modeling || check_scoreboard) ? 
      perfModelAnalyzeInterval(rtn_name, start_ins, end_ins) : 
      NULL; 


   // Build a list of read registers if relevant
   UINT32 num_reads = 0;
   REG *reads = NULL;      
   if ( g_knob_enable_performance_modeling )
   {
      num_reads = INS_MaxNumRRegs(ins);
      reads = new REG[num_reads];
      for (UINT32 i = 0; i < num_reads; i++) {
         reads[i] = INS_RegR(ins, i);
      }
   } 

	/**** TAKE OUT LATER TODO *****/
	//only for debugging instruction in runModels
	//at run time
	InsInfo* ins_info = (InsInfo*) malloc(sizeof(InsInfo));
	ins_info->opcode = INS_Opcode(ins);
	ins_info->is_sys_call = INS_IsSyscall(ins);
	ins_info->is_sys_enter = INS_IsSysenter(ins);
	ins_info->sys_call_std = INS_SyscallStd(ins);
	
	INS next_ins = INS_Next(ins);
	if(!INS_Valid(next_ins)) {
		ins_info->next_is_valid = false;
	} else {
		ins_info->next_is_valid = true;
		ins_info->next_opcode = INS_Opcode(next_ins);
		ins_info->next_is_sys_call = INS_IsSyscall(next_ins);
		ins_info->next_is_sys_enter = INS_IsSysenter(next_ins);
		ins_info->next_sys_call_std = INS_SyscallStd(next_ins);
	}
/*
 
      switch(which_case) 
      {
      case 0:
         IARGLIST_AddArguments(args, 
	    // icache load modeling 
            // dcache load modeling  
                                        IARG_MEMORYREAD_EA, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
	    // dcache store modeling 
                                        IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
	    // perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
	    // model-enable flags    
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
   		                        IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID *) ins_info, IARG_END);
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END);
            break;
      case 1:
         IARGLIST_AddArguments(args, 
	    // icache load modeling
            // dcache load modeling  
                                        IARG_MEMORYREAD_EA, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
	    // dcache store modeling
                                        IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0,
	    // perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
	    // model-enable flags
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
	   	                        IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END);
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID*) ins_info, IARG_END);
         break;
      case 2:
         IARGLIST_AddArguments(args, 
	    // icache load modeling
            // dcache load modeling  
                                        IARG_MEMORYREAD_EA, IARG_ADDRINT, (ADDRINT) NULL, IARG_MEMORYREAD_SIZE,
	    // dcache store modeling
                                        IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
	    // perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
	    // model-enable flags
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
		                        IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END);
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID *) ins_info, IARG_END);
         break;
      case 3:
         IARGLIST_AddArguments(args, 
	    // icache load modeling
            // dcache load modeling
                                        IARG_MEMORYREAD_EA, IARG_ADDRINT, (ADDRINT) NULL, IARG_MEMORYREAD_SIZE,
            // dcache store modeling
                                        IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0,
            //perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
            // model-enable flags
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
   	                                IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END);
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID*) ins_info, IARG_END);
         break;
      case 4:
      case 6:
         IARGLIST_AddArguments(args, 
            // icache load modeling 
            // dcache load modeling
                                        IARG_ADDRINT, (ADDRINT) NULL, IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0,
            // dcache store modeling
                                        IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
            // perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
            // model-enable flags
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
   	                                IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END); 
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID *) ins_info, IARG_END); 
         break;
      case 5:
      case 7:
         IARGLIST_AddArguments(args, 
            // icache load modeling
            // dcache load modeling
                                        IARG_ADDRINT, (ADDRINT) NULL, IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0,
            // dcache store modeling
                                        IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0,
            // perf modeling
                                        IARG_PTR, (VOID *) stats,
                                        IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
                                        IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
            // model-enable flags
                                        IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
   	  	                        IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                                        IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
//                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END); 
                                        IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_PTR, (VOID*) ins_info, IARG_END); 
         break;   
      default:
         assert( false );
======= */
   // Build a list of write registers if relevant
   UINT32 num_writes = 0;
   REG *writes = NULL;
   if ( g_knob_enable_performance_modeling )
   {
      num_writes = INS_MaxNumWRegs(ins);         
      writes = new REG[num_writes];
      for (UINT32 i = 0; i < num_writes; i++) {
         writes[i] = INS_RegW(ins, i);
//>>>>>>> master:pin/src/pin_sim.cc
      }
   }

/*
<<<<<<< HEAD:pin/src/pin_sim.cc
		
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) runModels, IARG_IARGLIST, args, IARG_END); 
      IARGLIST_Free(args);

//		cout << "--[" << rank << "] END     insertModeling TRUE ending" << endl;

		return true;
   } 
   else 
======= */

   //for building the arguments to the function which dispatches calls to the various modelers
   IARGLIST args = IARGLIST_Alloc();

   // Properly add the associated addresses for the argument call
   if(do_dcache_read_modeling)
//>>>>>>> master:pin/src/pin_sim.cc
   {
      IARGLIST_AddArguments(args, IARG_MEMORYREAD_EA, IARG_END);

      // If it's a dual read then we need the read2 ea otherwise, null
      if(is_dual_read)
         IARGLIST_AddArguments(args, IARG_MEMORYREAD2_EA, IARG_END);
      else
         IARGLIST_AddArguments(args, IARG_ADDRINT, (ADDRINT) NULL, IARG_END);

      IARGLIST_AddArguments(args, IARG_MEMORYREAD_SIZE, IARG_END);
   }
   else
   {
      IARGLIST_AddArguments(args, IARG_ADDRINT, (ADDRINT) NULL, IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0, IARG_END);
   }

//<<<<<<< HEAD:pin/src/pin_sim.cc
//   return false;
//=======
   // Do this after those first three
   if(do_dcache_write_modeling)
      IARGLIST_AddArguments(args,IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
   else
      IARGLIST_AddArguments(args,IARG_ADDRINT, (ADDRINT) NULL, IARG_UINT32, 0, IARG_END);

   // Now pass on our values for the appropriate models
   IARGLIST_AddArguments(args, 
         // perf modeling
         IARG_PTR, (VOID *) stats,
         IARG_PTR, (VOID *) reads, IARG_UINT32, num_reads, 
         IARG_PTR, (VOID *) writes, IARG_UINT32, num_writes, 
         // model-enable flags
         IARG_BOOL, do_network_modeling, IARG_BOOL, do_icache_modeling, 
         IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
         IARG_BOOL, do_dcache_write_modeling, IARG_BOOL, do_bpred_modeling, 
         IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, 
         IARG_END); 

   IARGLIST_AddArguments(args, IARG_PTR, (VOID *) ins_info);
	
	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) runModels, IARG_IARGLIST, args, IARG_END); 
   IARGLIST_Free(args);

   return true;
//>>>>>>> master:pin/src/pin_sim.cc
}


void getPotentialLoadFirstUses(const RTN& rtn, set<INS>& ins_uses)
{
   //FIXME: does not currently account for load registers live across rtn boundaries

   UINT32 rtn_ins_count = 0;

   // find the write registers not consumed within the basic block for instructs which read mem
   BitVector bbl_dest_regs(LEVEL_BASE::REG_LAST);
   BitVector dest_regs(LEVEL_BASE::REG_LAST);

   for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      rtn_ins_count += BBL_NumIns(bbl);

      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

         // remove from list of outstanding regs if is a use
         for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++)
         {
            REG r = INS_RegR(ins, i);
	    assert(0 <= r && r < LEVEL_BASE::REG_LAST);
            if ( bbl_dest_regs.at(r) ) {
               bbl_dest_regs.clear(r);
               ins_uses.insert( ins );
            }
         }

         if ( !INS_IsMemoryRead(ins) ) {

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

      dest_regs.set( bbl_dest_regs );
   }

   // find the instructs that source these registers
   for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
         for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++)
         {      
            REG r = INS_RegR(ins, i);
	    assert(0 <= r && r < LEVEL_BASE::REG_LAST);
            if ( dest_regs.at(r) )
               ins_uses.insert(ins);
         }
      }
   }      

#if 0  
   cout << "Routine " << RTN_Name(rtn) << endl;
   cout << "  instrumented " << ins_uses.size() << " of " << rtn_ins_count << endl;
   for (set<INS>::iterator it = ins_uses.begin(); it != ins_uses.end(); it++) {
      cout << "  " << INS_Disassemble( *it ) << endl;
   }
#endif

}


/* ===================================================================== */

AFUNPTR mapMsgAPICall(RTN& rtn, string& name)
{
   
	if(name == "CAPI_Initialize"){
	   cout << "Replacing CAPI_initialize" << endl;
      return AFUNPTR(chipInit);
   }
   else if(name == "CAPI_rank"){
      return AFUNPTR(commRank);
   }
   else if(name == "CAPI_message_send_w"){
      return AFUNPTR(chipSendW);
   }
   else if(name == "CAPI_message_receive_w"){
      return AFUNPTR(chipRecvW);
   }
   //FIXME added by cpc as a hack to get around calling Network for finished cores
   else if(name == "_Z11CAPI_Finishi"){
      cout << "replacing CAPI_Finish" << endl;
	  return AFUNPTR(chipHackFinish);
   }
	else if(name == "CAPI_debugSetMemState") {
		cout << "replacing CAPI_debugSetMemState" << endl;
		return AFUNPTR(chipDebugSetMemState);
	}
	else if(name == "CAPI_debugAssertMemState") {
		cout << "replacing CAPI_debugAssertMemState" << endl;
		return AFUNPTR(chipDebugAssertMemState);
	}
	else if(name == "CAPI_setDramBoundaries") {
		cout << "replacing CAPI_setDramBoundaries" << endl;
		return AFUNPTR(chipSetDramBoundaries);
	}
   else if(name == "runSyscallServer"){
      return AFUNPTR(syscallServerRun);
   }
   
   return NULL;
}

VOID routine(RTN rtn, VOID *v)
{
   AFUNPTR msg_ptr = NULL;
   RTN_Open(rtn);
   INS rtn_head = RTN_InsHead(rtn);
   string rtn_name = RTN_Name(rtn);
   bool is_rtn_ins_head = true;
   set<INS> ins_uses;

//    cout << "routine " << RTN_Name(rtn) << endl;

   if ( (msg_ptr = mapMsgAPICall(rtn, rtn_name)) != NULL ) {
      RTN_Replace(rtn, msg_ptr);
   } 
#ifdef INSTRUMENT_ALLOWED_FUNCTIONS
	else if(
		rtn_name != "_Z13instrument_mev" 
		&& rtn_name != "_Z4pongPv"
		&& rtn_name != "_Z4pingPv"
		&& rtn_name != "_Z22awesome_test_suite_msii" 
		&& rtn_name != "_Z22awesome_test_suite_msiv" 
	//don't do anything
	) {}
#endif
   else 
   {
      
#ifdef INSTRUMENT_ALLOWED_FUNCTIONS
		cout << "Routine name is: " << rtn_name << endl;
#endif
	  
	  if ( g_knob_enable_performance_modeling && g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_loads ) 
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
            assert( !INS_Valid(next) || (INS_Address(next) > INS_Address(ins)) );
            assert( !is_rtn_ins_head || (ins==rtn_head) );

            set<INS>::iterator it = ins_uses.find(ins);
            bool is_potential_load_use = ( it != ins_uses.end() );
            if ( is_potential_load_use ) {  
               ins_uses.erase( it );
            }

            bool instrumented = 
               insertInstructionModelingCall(rtn_name, start_ins, ins, is_rtn_ins_head, (inst_offset == 0),
                                             !INS_Valid(next), is_potential_load_use );
            if ( instrumented ) {
               start_ins = INS_Next(ins);
	    }
            ++inst_offset;
            is_rtn_ins_head = false;
         }
         assert( inst_offset == last_offset );
      }  
   }

   RTN_Close(rtn);
}


/* ===================================================================== */

VOID fini(int code, VOID * v)
{
   Transport::ptFinish();
   g_chip->fini(code, v);
}

/* ===================================================================== */

VOID init_globals()
{
  
   if( g_knob_simarch_has_shared_mem ) {

      if( !g_knob_enable_dcache_modeling ) {
   
         cout << endl << "**********************************************************************" << endl;
         cout << endl << "  User must set dcache modeling on (-mdc) to use shared memory model. " << endl;
         cout << endl << "**********************************************************************" << endl;

         cout << endl << "Exiting Program...." << endl;
         exit(-1); 
      }
   }
   
   g_config = new Config;
   //g_config->loadFromFile(FIXME);

   // NOTE: transport and queues must be inited before the chip
   Transport::ptInitQueue(g_knob_num_cores);

   g_chip = new Chip(g_knob_num_cores);

   // Note the syscall server has a dependency on the transport layer and the chip
   g_syscall_server = new SyscallServer();

   InitLock(&dcache_read_lock);
   InitLock(&dcache_write_lock);
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
   PIN_InitSymbols();

   if( PIN_Init(argc,argv) )
      return usage();

   init_globals();
    
   RTN_AddInstrumentFunction(routine, 0);
   PIN_AddSyscallEntryFunction(SyscallEntry, 0);
   PIN_AddSyscallExitFunction(SyscallExit, 0);

   PIN_AddFiniFunction(fini, 0);
 
   // Never returns
   PIN_StartProgram();
    
   return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
