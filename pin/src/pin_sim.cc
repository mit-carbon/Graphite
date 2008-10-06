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
#include "config.h"
#include "chip.h"
#include "cache.h"
#include "ocache.h"
#include "perfmdl.h"
#include "knobs.h"


Chip *g_chip;
Config *g_config;

INT32 usage()
{
   cerr << "This tool implements a multicore simulator." << endl;
   cerr << KNOB_BASE::StringKnobSummary() << endl;

   return -1;
}


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
               bool check_scoreboard)
{
   //cout << "parent = " << stats->parent_routine << endl;

   int rank;
   chipRank(&rank);
   if(rank > -1)
   {
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
      } 
      else 
      {
         assert(dcache_ld_addr == (ADDRINT) NULL);
         assert(dcache_ld_addr2 == (ADDRINT) NULL);
         assert(dcache_ld_size == 0);
      }

      if ( do_dcache_write_modeling )
      {
         bool d_hit = dcacheRunStoreModel(dcache_st_addr, dcache_st_size);
         if ( do_perf_modeling )
         { 
            perfModelLogDCacheStoreAccess(stats, d_hit); 
         }
      } 
      else 
      {
         assert(dcache_st_addr == (ADDRINT) NULL);
         assert(dcache_st_size == 0);
      }

      if ( do_perf_modeling )
      {
         perfModelRun(stats);
      }
   }
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
   stats = (do_perf_modeling || do_icache_modeling) ? 
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


   // Build a list of write registers if relevant
   UINT32 num_writes = 0;
   REG *writes = NULL;
   if ( g_knob_enable_performance_modeling )
   {
      num_writes = INS_MaxNumWRegs(ins);         
      writes = new REG[num_writes];
      for (UINT32 i = 0; i < num_writes; i++) {
         writes[i] = INS_RegW(ins, i);
      }
   }


   //for building the arguments to the function which dispatches calls to the various modelers
   IARGLIST args = IARGLIST_Alloc();

   // Properly add the associated addresses for the argument call
   if(do_dcache_read_modeling)
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
         IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard, IARG_END); 

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

   for (BBL bbl = RTN_BblHead(rtn); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      rtn_ins_count += BBL_NumIns(bbl);

      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

         // remove from list of outstanding regs if is a use
         for (UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++)
         {
            REG r = INS_RegR(ins, i);
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
      return AFUNPTR(chipInit);
   }
   else if(name == "CAPI_rank"){
      return AFUNPTR(chipRank);
   }
   else if(name == "CAPI_message_send_w"){
      return AFUNPTR(chipSendW);
   }
   else if(name == "CAPI_message_receive_w"){
      return AFUNPTR(chipRecvW);
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

   // cout << "routine " << RTN_Name(rtn) << endl;

   if ( (msg_ptr = mapMsgAPICall(rtn, rtn_name)) != NULL ) {
      RTN_Replace(rtn, msg_ptr);
   } 
   else 
   {
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
   g_chip->fini(code, v);
}

/* ===================================================================== */

VOID init_globals()
{
   g_config = new Config;
   //g_config->loadFromFile(FIXME);

   // NOTE: transport and queues must be inited before the chip
   Transport::ptInitQueue(g_knob_num_cores);

   g_chip = new Chip(g_knob_num_cores);
}


int main(int argc, char *argv[])
{
   PIN_InitSymbols();

   if( PIN_Init(argc,argv) )
      return usage();

   init_globals();
    
   RTN_AddInstrumentFunction(routine, 0);

   PIN_AddFiniFunction(fini, 0);
 
   // Never returns
   PIN_StartProgram();
    
   return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
