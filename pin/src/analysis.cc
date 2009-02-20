#include "analysis.h"
#include "config.h"

extern LEVEL_BASE::KNOB<bool> g_knob_enable_performance_modeling;
extern LEVEL_BASE::KNOB<bool> g_knob_enable_dcache_modeling;
extern LEVEL_BASE::KNOB<bool> g_knob_dcache_ignore_loads;
extern LEVEL_BASE::KNOB<bool> g_knob_dcache_ignore_stores;
extern LEVEL_BASE::KNOB<bool> g_knob_enable_icache_modeling;

UInt32 getInsMicroOpsCount(const INS& ins)
{
   // FIXME: assumes that stack is not supported by special hardware; load and
   // store microops are assumed to be required.

   bool does_read  = INS_IsMemoryRead(ins);
   bool does_read2 = INS_HasMemoryRead2(ins);
   bool does_write = INS_IsMemoryWrite(ins);

   UInt32 count = 0;

   // potentially load first operand from mem
   count += does_read ? 1 : 0;

   // potentially load second operand from mem
   count += does_read2 ? 1 : 0;

   // perform the op on the operands
   count += 1;

   // potentially store the result to mem
   count += does_write ? 1 : 0;

   return count;
}

PerfModelIntervalStat* analyzeInterval(const string& parent_routine,
      const INS& start_ins, const INS& end_ins)
{
   vector< pair<IntPtr, UInt32> > inst_trace;
   UInt32 microop_count = 0;
   UInt32 cycles_subtotal = 0;

   // do some analysis to get the number of cycles (before mem, branch stalls)
   // fixme: for now we approximate with approx # x86 microops;
   // need to account for pipeline depth / instruction latencies

   for (INS ins = start_ins; ins!=end_ins; ins = INS_Next(ins))
   {
      // debug info
      // cout << hex << "0x" << INS_Address(BBL_InsTail(bbl)) << dec << ": "
      //      << INS_Mnemonic(BBL_InsTail(bbl)) << endl;

      inst_trace.push_back(pair<IntPtr, UInt32>(INS_Address(ins), INS_Size(ins)));
      UInt32 micro_ops = getInsMicroOpsCount(ins);
      microop_count += micro_ops;
      // FIXME
      cycles_subtotal += micro_ops;
   }

   // allocate struct for instructs in the basic block to write stats into.
   // NOTE: if a basic block gets split, this data may become redundant
   PerfModelIntervalStat *stats = new PerfModelIntervalStat(parent_routine,
         inst_trace,
         microop_count,
         cycles_subtotal);
   return stats;
}



PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine,
      const INS& start_ins, const INS& end_ins)
{
   // using zero is a dirty hack
   // assumes its safe to use core zero to generate perfmodels for all cores
   assert(Config::getSingleton()->getNumLocalCores() > 0);

   //FIXME: These stats should be deleted at the end of execution
   PerfModelIntervalStat* *array = new PerfModelIntervalStat*[Config::getSingleton()->getNumLocalCores()];

   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
      array[i] = analyzeInterval(parent_routine, start_ins, end_ins);

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
   bool do_dcache_read_modeling  = g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_loads &&
                                   INS_IsMemoryRead(ins);
   bool do_dcache_write_modeling = g_knob_enable_dcache_modeling && !g_knob_dcache_ignore_stores &&
                                   INS_IsMemoryWrite(ins);

   //TODO: if we run on multiple machines we need shared memory
   //TODO: if we run on multiple machines we need syscall_modeling

   // If we are doing any other type of modeling then we need to do icache modeling
   bool do_icache_modeling       = g_knob_enable_icache_modeling &&
                                   (do_dcache_read_modeling || do_dcache_write_modeling || 
                                    is_bbl_ins_tail || check_scoreboard);

   bool do_perf_modeling         = g_knob_enable_performance_modeling &&
                                   (do_dcache_read_modeling || do_dcache_write_modeling || 
                                    do_icache_modeling || is_bbl_ins_tail || check_scoreboard);

   // Exit early if we aren't modeling anything
   if (!do_icache_modeling && !do_dcache_read_modeling  &&
         !do_dcache_write_modeling && !do_perf_modeling)
   {
      return false;
   }

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
                         IARG_BOOL, do_icache_modeling,
                         IARG_BOOL, do_dcache_read_modeling, IARG_BOOL, is_dual_read,
                         IARG_BOOL, do_dcache_write_modeling,
                         IARG_BOOL, do_perf_modeling, IARG_BOOL, check_scoreboard,
                         IARG_END);

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

}

void replaceInstruction(RTN rtn, string rtn_name)
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

