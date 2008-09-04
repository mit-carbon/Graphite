#include "core.h"
#include "debug.h"

using namespace std;

int Core::coreInit(Chip *chip, int tid, int num_mod)
{
   the_chip = chip;
   core_tid = tid;
   core_num_mod = num_mod;

   network = new Network;
   network->netInit(chip, tid, num_mod, this);
   
   if ( g_knob_enable_performance_modeling ) 
   {
      perf_model = new PerfModel("performance modeler");
      debugPrint(tid, "Core", "instantiated performance model");
   } else 
   {
      perf_model = (PerfModel *) NULL;    
   }   

   if ( g_knob_enable_dcache_modeling || g_knob_enable_icache_modeling ) 
   {
      ocache = new OCache("organic cache", 
                          g_knob_cache_size.Value() * k_KILO,
                          g_knob_line_size.Value(),
                          g_knob_associativity.Value(),
                          g_knob_mutation_interval.Value(),
                          g_knob_dcache_threshold_hit.Value(),
                          g_knob_dcache_threshold_miss.Value(),
                          g_knob_dcache_size.Value() * k_KILO,
                          g_knob_dcache_associativity.Value(),
                          g_knob_dcache_max_search_depth.Value(),
                          g_knob_icache_threshold_hit.Value(),
                          g_knob_icache_threshold_miss.Value(),
                          g_knob_icache_size.Value() * k_KILO,
                          g_knob_icache_associativity.Value(),
                          g_knob_icache_max_search_depth.Value());                        
//                          g_knob_simarch_is_shared_mem.Value());                        

      debugPrint(tid, "Core", "instantiated organic cache model");
      cout << ocache->statsLong() << endl;
  
   
   } else 
   {
      ocache = (OCache *) NULL;
   }   


   if ( g_knob_simarch_has_shared_mem ) {
     
      assert( g_knob_enable_dcache_modeling ); 

      cout << "Core[" << tid << "]: instantiated memory manager model" << endl;
      memory_manager = new MemoryManager(this, ocache);

   } else {

      memory_manager = (MemoryManager *) NULL;
   
   }


   return 0;
}

int Core::coreSendW(int sender, int receiver, char *buffer, int size)
{
   // Create a net packet
   NetPacket packet;
   packet.sender= sender;
   packet.receiver= receiver;
   packet.type = USER;
   packet.length = size;
   packet.data = buffer;

   /*
   packet.data = new char[size];
   for(int i = 0; i < size; i++)
      packet.data[i] = buffer[i];
   */
   
   network->netSend(packet);
   return 0;
}

int Core::coreRecvW(int sender, int receiver, char *buffer, int size)
{
   NetPacket packet;
   NetMatch match;

   match.sender = sender;
   match.sender_flag = true;
   match.type = USER;
   match.type_flag = true;

   packet = network->netRecv(match);

   if((unsigned)size != packet.length){
      cout << "ERROR:" << endl
           << "Received packet length is not as expected" << endl;
      exit(-1);
   }

   for(int i = 0; i < size; i++)
      buffer[i] = packet.data[i];

   // De-allocate dynamic memory
   delete [] packet.data;

   return 0;
}

Network* Core::getNetwork()
{
  return network;
}

VOID Core::fini(int code, VOID *v, ofstream& out)
{
   if ( g_knob_enable_performance_modeling )
      perf_model->fini(code, v, out);

   if ( g_knob_enable_dcache_modeling || g_knob_enable_icache_modeling )
      ocache->fini(code,v,out);
}


//performance model wrappers

//BUG the following ten functions *should* be inlined, but 
//we end up with undefined symbols at run-time. -cpc
VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats)
{ perf_model->run(interval_stats); }

VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
						 UINT32 num_reads)
{ perf_model->run(interval_stats, reads, num_reads); }

VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
						 REG *writes, UINT32 num_writes)
{ perf_model->run(interval_stats, dcache_load_hit, writes, num_writes); }

PerfModelIntervalStat* Core::perfModelAnalyzeInterval(const string& parent_routine, 
													   const INS& start_ins, 
													   const INS& end_ins)
{ return perf_model->analyzeInterval(parent_routine, start_ins, end_ins); }

VOID Core::perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
{ perf_model->logICacheLoadAccess(stats, hit); }

VOID Core::perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
{ perf_model->logDCacheStoreAccess(stats, hit); }

VOID Core::perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
{ perf_model->logBranchPrediction(stats, correct); }


// organic cache wrappers

bool Core::icacheRunLoadModel(ADDRINT i_addr, UINT32 size)
{ return ocache->runICacheLoadModel(i_addr, size).first; }

bool Core::dcacheRunLoadModel(ADDRINT d_addr, UINT32 size)
{ 
   
#ifdef CORE_DEBUG
	  debugPrintHex(getRank(), "Core", "dcache (READ)  : ADDR: ", d_addr);
#endif	
	
	if( g_knob_simarch_has_shared_mem ) { 
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", "dcache initiating shared memory request (READ)");
#endif
	   bool ret = memory_manager->initiateSharedMemReq(d_addr, size, READ); 
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", " COMPLETED - dcache initiating shared memory request (READ)");
#endif
	   return ret;
   } else {
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", "dcache initiating NON-shared memory request (READ)");
#endif
	   return ocache->runDCacheLoadModel(d_addr, size).first;
   }
}

bool Core::dcacheRunStoreModel(ADDRINT d_addr, UINT32 size)
{ 

#ifdef CORE_DEBUG
	  debugPrintHex(getRank(), "Core", "dcache (WRITE) : ADDR: ", d_addr);
#endif	

   if( g_knob_simarch_has_shared_mem ) { 
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", "dcache initiating shared memory request (WRITE)");
#endif
	   bool ret = memory_manager->initiateSharedMemReq(d_addr, size, WRITE); 
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", " COMPLETED - dcache initiating shared memory request (WRITE)");
#endif
	   return ret;
   } else {
#ifdef SMEM_DEBUG
       debugPrint(getRank(), "Core", "dcache initiating NON-shared memory request (WRITE)");
#endif
	   return ocache->runDCacheStoreModel(d_addr, size).first;
   }
}
