#include "core.h"
#include "debug.h"

#include "network_mesh_analytical.h"
//#define CORE_DEBUG

using namespace std;

int Core::coreInit(Chip *chip, int tid, int num_mod)
{
   the_chip = chip;
   core_tid = tid;
   core_num_mod = num_mod;

   //Switch which line is commented to choose the different 
   //network models
   //FIXME: Make this runtime configurable
   //NetworkModel net_model = NETWORK_BUS;
   NetworkModel net_model = NETWORK_ANALYTICAL_MESH;

   switch(net_model)
   {
       case NETWORK_BUS:
           network = new Network(chip, tid, num_mod, this);
           break;
       case NETWORK_ANALYTICAL_MESH:
           network = new NetworkMeshAnalytical(chip, tid, num_mod, this);
           break;
       case NUM_NETWORK_TYPES:
       default:
           cout << "ERROR: Unknown Network Model!";
           break;
   }
  

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

      cerr << "Core[" << tid << "]: instantiated memory manager model" << endl;
      memory_manager = new MemoryManager(this, ocache);

   } else {

      memory_manager = (MemoryManager *) NULL;
      cerr << "No Memory Manager being used. " << endl;
   
   }

   syscall_model = new SyscallMdl(network);
   InitLock(&dcache_lock);

   return 0;
}

int Core::coreCommID() { return network->netCommID(); }

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

#ifdef DEBUG
   cout << "Got packet: "
	<< "Send=" << packet.sender
	<< ", Recv=" << packet.receiver
	<< ", Type=" << packet.type
	<< ", Len=" << packet.length << endl;
#endif

   if((unsigned)size != packet.length){
      cerr << "ERROR (comm_id: " << coreCommID() << "):" << endl
           << "Received packet length (" << packet.length
	   << ") is not as expected (" << size << ")" << endl;
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
   delete network;

   if ( g_knob_enable_performance_modeling )
      perf_model->fini(code, v, out);

   if ( g_knob_enable_dcache_modeling || g_knob_enable_icache_modeling )
      ocache->fini(code,v,out);
}


//performance model wrappers

//these have been moved into the .h file
//m VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats)
//m { perf_model->run(interval_stats); }

//VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
//						 UINT32 num_reads)
//{ perf_model->run(interval_stats, reads, num_reads); }

//VOID Core::perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
//						 REG *writes, UINT32 num_writes)
//{ perf_model->run(interval_stats, dcache_load_hit, writes, num_writes); }

//PerfModelIntervalStat* Core::perfModelAnalyzeInterval(const string& parent_routine, 
//													   const INS& start_ins, 
//													   const INS& end_ins)
//{ return perf_model->analyzeInterval(parent_routine, start_ins, end_ins); }

//VOID Core::perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
//{ perf_model->logICacheLoadAccess(stats, hit); }

//VOID Core::perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
//{ perf_model->logDCacheStoreAccess(stats, hit); }

//VOID Core::perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
//{ perf_model->logBranchPrediction(stats, correct); }


// organic cache wrappers

bool Core::icacheRunLoadModel(ADDRINT i_addr, UINT32 size)
{ return ocache->runICacheLoadModel(i_addr, size).first; }

/*
 * dcacheRunModel (mem_operation_t operation, ADDRINT d_addr, char* data_buffer, UINT32 data_size)
 *
 * Arguments:
 *   d_addr :: address of location we want to access (read or write)
 *   shmem_req_t :: READ or WRITE
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *
 * Return Value:
 *   hit :: Say whether there has been at least one cache hit or not
 */
bool Core::dcacheRunModel(mem_operation_t operation, ADDRINT d_addr, char* data_buffer, UINT32 data_size)
{
	shmem_req_t shmem_operation;
	
	if (operation == LOAD)
		shmem_operaiton = READ;
	else
		shmem_operation = WRITE;

	if (g_knob_simarch_has_shared_mem)  {

		bool all_hits = true;

		if (size <= 0) {
			return (false);
			// FIXME: Why false - we are not encountering any cache misses anyway
		}

#ifdef SMEM_DEBUG
       debugPrint(getRank(), "CORE", "dcache initiating shared memory request (READ)");
#endif
		
		ADDRINT begin_addr = d_addr;
		ADDRINT end_addr = d_addr + data_size;
	   	ADDRINT begin_addr_aligned = begin_addr - (begin_addr % ocache->dCacheLineSize());
		ADDRINT end_addr_aligned = end_addr - (end_addr % ocache->dCacheLineSize());
		char *curr_data_buffer_head = data_buffer;

		for (ADDRINT curr_addr_aligned = begin_addr_aligned ; curr_addr_aligned <= end_addr_aligned /* Note <= */; curr_addr_aligned += ocache->dCacheLineSize())
		{
			// Access the cache one line at a time
			ADDRINT curr_offset;
			UINT32 curr_size;

			// Determine the offset
			// FIXME: Check if all this is correct
			if (curr_addr_aligned == begin_addr_aligned) {
				curr_offset = begin_addr % ocache->dCacheLineSize();
			}
			else {
				curr_offset = 0;
			}

			// Determine the size
			if (curr_addr_aligned = end_addr_aligned) {
				curr_size = (end_addr % ocache->dCacheLineSize()) - (curr_offset);
			}
			else {
				curr_size = ocache->dCacheLineSize() - (curr_offset);
			}

			if (!memory_manager->initiateSharedMemReq (shmem_operation, curr_addr_aligned, curr_offset, curr_data_buffer_head, curr_size)) {
				// If it is a LOAD operation, 'initiateSharedMemReq' causes curr_data_buffer_head to be automatically filled in
				// If it is a STORE operation, 'initiateSharedMemReq' reads the data from curr_data_buffer_head
				all_hits = false;
			}

			// Increment the buffer head
			curr_data_buffer_head += curr_size;
		}


		//NOTE: this method of multi-line accesses cache-aligns the addresses.  
		//TODO have this deal with data
			//access one cache line at a time
			//TODO set the size parameter correctly, based on the size of the data buffer
			//TODO does this spill over to another line? should shared_mem test look at other DRAM entries?
			//the "0" below is the addr_offset. important for partial-line operations when the data_size is less than cache_line_size
		
//		debugPrint(getRank(), "Core", "FINISHED ---- TRapping into SHARED_MEMORY");
		return all_hits;		    
   
	} 
	else {
	   	//run this if we aren't using shared_memory
		// FIXME: I am not sure this is right
		if(operation == LOAD)
			return ocache->runDCacheLoadModel(d_addr, size).first;
	   	else
			return ocache->runDCacheStoreModel(d_addr, size).first;
   	}
}

void Core::debugSetCacheState(ADDRINT address, CacheState::cstate_t cstate)
{
	//using Load Model, so that way we garuntee the tag isn't null
	pair<bool, CacheTag*> cache_result;
	
	switch(cstate) {
		case CacheState::INVALID:
			ocache->dCacheInvalidateLine(address);
			break;
		case CacheState::SHARED:
			cache_result = ocache->runDCacheLoadModel(address,1);
			cache_result.second->setCState(cstate);
			break;
		case CacheState::EXCLUSIVE:
			cache_result = ocache->runDCacheLoadModel(address,1);
			cache_result.second->setCState(cstate);
			break;
		default:
			cout << "ERROR in switch for Core::debugSetCacheState" << endl;
	}
}

bool Core::debugAssertCacheState(ADDRINT address, CacheState::cstate_t expected_cstate)
{
//	pair<bool,CacheTag*> cache_result = ocache->runDCachePeekModel(address, 1);
	pair<bool,CacheTag*> cache_result = ocache->runDCachePeekModel(address);
   
	bool is_assert_true;
   CacheState::cstate_t actual_cstate;

	if( cache_result.second != NULL ) {
		actual_cstate = cache_result.second->getCState();
		is_assert_true = ( actual_cstate  == expected_cstate );
	} else {
		actual_cstate = CacheState::INVALID; 
		is_assert_true = ( actual_cstate  == expected_cstate );
	}
	
	cerr << "   Asserting Cache[" << getRank() << "] : Expected: " << CacheState::cStateToString(expected_cstate) << " ,  Actual: " <<  CacheState::cStateToString(actual_cstate);
	
	if(is_assert_true) {
      cerr << "                    TEST PASSED " << endl;
	} else {
		cerr << "                    TEST FAILED ****** " << endl;
	}

	return is_assert_true;
	
}

void Core::debugSetDramState(ADDRINT address, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list)
{
	memory_manager->debugSetDramState(address, dstate, sharers_list);		   
}

bool Core::debugAssertDramState(ADDRINT address, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list)
{
	return memory_manager->debugAssertDramState(address, dstate, sharers_list);

}


/*
void Core::setDramBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_boundaries)
{
	cerr << "CORE: setting dram boundaries" << endl;
	assert( g_knob_simarch_has_shared_mem );
	memory_manager->setDramBoundaries(addr_boundaries);
	cerr << "CORE: finished setting dram boundaries" << endl;
}
*/
