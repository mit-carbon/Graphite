#ifndef KNOBS_H
#define KNOBS_H

#include "fixed_types.h"
#include <string>
using std::string;

/* ===================================================================== */
/* General Simulator Knobs */
/* ===================================================================== */

KNOB<string> g_knob_output_file(KNOB_MODE_WRITEONCE, "pintool",
                                "o", "sim.out", "specify ocache file name");
KNOB<UInt32> g_knob_total_cores(KNOB_MODE_WRITEONCE, "pintool",
                                "tc", "0", "Specifies the total number of cores in all processes");
KNOB<UInt32> g_knob_num_process(KNOB_MODE_WRITEONCE, "pintool",
                                "np", "0", "Specifies the total number of processes in the simulation");

KNOB<BOOL>   g_knob_enable_dcache_modeling(KNOB_MODE_WRITEONCE, "pintool",
      "mdc", "0", "turn on modeling for the data cache");
KNOB<BOOL>   g_knob_enable_icache_modeling(KNOB_MODE_WRITEONCE, "pintool",
      "mic", "0", "turn on modeling for the instruction cache");
KNOB<BOOL>   g_knob_enable_performance_modeling(KNOB_MODE_WRITEONCE, "pintool",
      "mpf", "0", "turns on performance modeler");

KNOB<BOOL>   g_knob_simarch_has_shared_mem(KNOB_MODE_WRITEONCE, "pintool",
      "msm", "0", "toggles simulation of shared memory");
KNOB<UInt32>   g_knob_dir_max_sharers(KNOB_MODE_WRITEONCE, "pintool",
                                      "dms", "9001", "Specifies the maximum number of sharer pointers kept in directory.");

/* ===================================================================== */
/* Performance Modeler Knobs */
/* ===================================================================== */

KNOB<BOOL>   g_knob_enable_syscall_modeling(KNOB_MODE_WRITEONCE, "pintool",
      "msys", "1", "turns on syscall modeler");

/* ===================================================================== */
/* Organic Cache Knobs */
/* ===================================================================== */

KNOB<BOOL>   g_knob_dcache_track_loads(KNOB_MODE_WRITEONCE, "pintool",
                                       "dl", "0", "track individual dcache loads -- increases profiling time");
KNOB<BOOL>   g_knob_dcache_track_stores(KNOB_MODE_WRITEONCE, "pintool",
                                        "ds", "0", "track individual dcache stores -- increases profiling time");
KNOB<UInt32> g_knob_dcache_threshold_hit(KNOB_MODE_WRITEONCE , "pintool",
      "drh", "100", "only report dcache memops with hit count above threshold");
KNOB<UInt32> g_knob_dcache_threshold_miss(KNOB_MODE_WRITEONCE, "pintool",
      "drm","100", "only report dcache memops with miss count above threshold");
KNOB<BOOL>   g_knob_dcache_ignore_loads(KNOB_MODE_WRITEONCE, "pintool",
                                        "dnl", "0", "ignore all dcache loads");
KNOB<BOOL>   g_knob_dcache_ignore_stores(KNOB_MODE_WRITEONCE, "pintool",
      "dns", "0", "ignore all dcache stores");
KNOB<BOOL>   g_knob_dcache_ignore_size(KNOB_MODE_WRITEONCE, "pintool",
                                       "dz", "0", "ignore size of all dcache references (default size is 4 bytes)");

KNOB<BOOL>   g_knob_icache_track_insts(KNOB_MODE_WRITEONCE, "pintool",
                                       "ii", "0", "track individual instructions -- increases profiling time");
KNOB<UInt32> g_knob_icache_threshold_hit(KNOB_MODE_WRITEONCE , "pintool",
      "irh", "100", "only report icache ops with hit count above threshold");
KNOB<UInt32> g_knob_icache_threshold_miss(KNOB_MODE_WRITEONCE, "pintool",
      "irm","100", "only report icache ops with miss count above threshold");
KNOB<BOOL>   g_knob_icache_ignore_size(KNOB_MODE_WRITEONCE, "pintool",
                                       "iz", "0", "ignore size of instruction (default size is 4 bytes)");

KNOB<UInt32> g_knob_cache_size(KNOB_MODE_WRITEONCE, "pintool",
                               "c","64", "cache size in kilobytes");
KNOB<UInt32> g_knob_cache_line_size(KNOB_MODE_WRITEONCE, "pintool",
                              "b","32", "cache block size in bytes");
KNOB<UInt32> g_knob_associativity(KNOB_MODE_WRITEONCE, "pintool",
   "a","8", "cache associativity (1 for direct mapped)");
KNOB<UInt32> g_knob_mutation_interval(KNOB_MODE_WRITEONCE, "pintool",
                                      "m","0", "cache auto-reconfiguration period in number of accesses");

KNOB<UInt32> g_knob_dcache_size(KNOB_MODE_WRITEONCE, "pintool",
                                "dc","32", "data cache size in kilobytes");
KNOB<UInt32> g_knob_dcache_associativity(KNOB_MODE_WRITEONCE, "pintool",
   "da","4", "data cache associativity (1 for direct mapped)");
KNOB<UInt32> g_knob_dcache_max_search_depth(KNOB_MODE_WRITEONCE, "pintool",
      "dd","1", "data cache max hash search depth (1 for no multi-cycle hashing)");

KNOB<UInt32> g_knob_icache_size(KNOB_MODE_WRITEONCE, "pintool",
                                "ic","32", "instruction cache size in kilobytes");
KNOB<UInt32> g_knob_icache_associativity(KNOB_MODE_WRITEONCE, "pintool",
   "ia","4", "instruction cache associativity (1 for direct mapped)");
KNOB<UInt32> g_knob_icache_max_search_depth(KNOB_MODE_WRITEONCE, "pintool",
      "id","1", "instruction cache max hash search depth (1 for no multi-cycle hashing)");

/* =================================================================== */
/* Address Home Lookup */
/* =================================================================== */
KNOB<UInt32> g_knob_ahl_param(KNOB_MODE_WRITEONCE, "pintool",
	"ahl","8", "AHL parameter (See File addr_home_lookup.cc)");

#endif
