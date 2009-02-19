// RunModels
//
// This file is responsible for the pin callback hooks that are used in order 
// interface with the model of the core and the cache during the instrumented
// program's execution.


#ifndef RUNMODELS_H
#define RUNMODELS_H

#include "pin.H"
#include "perfmdl.h"
#include "fixed_types.h"

void runModels(IntPtr dcache_ld_addr, IntPtr dcache_ld_addr2, UINT32 dcache_ld_size,
               IntPtr dcache_st_addr, UINT32 dcache_st_size,
               PerfModelIntervalStat* *stats,
               REG *reads, UINT32 num_reads, REG *writes, UINT32 num_writes,
               bool do_icache_modeling, bool do_dcache_read_modeling, bool is_dual_read,
               bool do_dcache_write_modeling, bool do_perf_modeling, bool check_scoreboard);

#endif
