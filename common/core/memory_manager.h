#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <sstream>
#include <iostream>
#include <queue>
#include "assert.h"
#include <math.h>
#include "debug.h"

// some forward declarations for cross includes
class Core;
class DramDirectory; //i hate compiling c++ code

// TODO: this is a hack that is due to the fact that network.h 
// is already included by the time this is handled, so NetPacket is 
// never getting defined. Fine some more elegant way to solve this.
typedef struct NetPacket NetPacket;
typedef struct NetMatch NetMatch;

#include "core.h"
#include "ocache.h"
#include "dram_directory.h"
#include "dram_directory_entry.h"
#include "network.h"
#include "address_home_lookup.h"
#include "cache_state.h"

extern LEVEL_BASE::KNOB<BOOL> g_knob_simarch_has_shared_mem;
extern LEVEL_BASE::KNOB<UINT32> g_knob_ahl_param;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dram_access_cost;
extern LEVEL_BASE::KNOB<UINT32> g_knob_line_size;

// TODO: move this into MemoryManager class?
enum shmem_req_t {
  READ, 
  WRITE,
  INVALIDATE,
  NUM_STATES
};

class MemoryManager
{
  public: 
	//i hate forward declarations
	struct RequestPayload;
	struct AckPayload;
	struct UpdatePayload;
 
  private:
	Core *the_core;
   OCache *ocache;
   DramDirectory *dram_dir;
   AddressHomeLookup *addr_home_lookup;

	//This is here to serialize the requests
	// do not process a new request until finished with current request
	// do not exit MMU until no more incoming requests
   UINT64 volatile debug_counter; //a primitive clock for debugging
   bool volatile processing_request_flag;
   int volatile incoming_requests_count;

   /* ============================================= */
   /* Added by George */ //moved to dram directory
//   UINT64 dramAccessCost;
	/* ============================================= */

	//evictions from the cache are written into this buffer
	char* eviction_buffer;
	//dram must fill this buffer and give it to the cache
	char* fill_buffer; 
	
	//FIFO queue
	queue<NetPacket> request_queue;
	void addRequestToQueue( NetPacket packet );
	NetPacket getNextRequest();

	void debugPrintReqPayload(MemoryManager::RequestPayload payload);
 
 public:

	/*
	 * memory coherency message payloads can vary depending
	 * on the type of message that needs to be seen
	 */

	enum sm_payload_t {
		REQUEST,
		UPDATE,
		ACK,
		PAYLOAD_NUM_STATES
	};

	/* one could make the argument to have one payload
	 * with fields that may not be used....
	 */
	struct RequestPayload {
		shmem_req_t request_type;
		ADDRINT request_address;
		UINT32 request_num_bytes; 
	};	

	struct UpdatePayload {
		CacheState::cstate_t update_new_cstate;
		ADDRINT update_address;
		char* data_buffer;
		UINT32 data_size; //in bytes
		bool is_writeback; //is this payload serving as a writeback message to dram?
	};

	struct AckPayload {
		CacheState::cstate_t ack_new_cstate;
		ADDRINT ack_address;
		char* data_buffer;
		UINT32 data_size;
		bool is_writeback; //when we invalidate/demote owners, we may need to do a writeback
		//if sent a downgrade message (E->S), but cache
		//no longer has the line, send a bit to tell dram directory
		//to remove it from the sharers' list
		BOOL remove_from_sharers;
	};

	MemoryManager(Core *the_core_arg, OCache *ocache_arg);
	virtual ~MemoryManager();

	/**** Added for Data Sharing ****/	
	
	void setCacheLineCState(CacheTag* c_line_info, CacheState::cstate_t new_cstate);

	void readFillBuffer( UINT32 offset, char* data_buffer, UINT32 data_size);
	void writeFillBuffer(UINT32 offset, char* data_buffer, UINT32 data_size);
	
	//cache interfacing functions.
	//TODO these functions should probably be moved into the ocache interface!
	void setCacheLineInfo(ADDRINT ca_address, CacheState::cstate_t new_cstate);
	pair<bool, CacheTag*> getCacheLineInfo(ADDRINT address);
	void readCacheLineData(ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size);
	void writeCacheLineData(ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size);
	void invalidateCacheLine(ADDRINT address);
	
	//request from DRAM permission to use an address
	//writes requested data into the "fill_buffer", and writes what the new_cstate should be on the receiving end
	void requestPermission(shmem_req_t shmem_req_type, ADDRINT address, CacheState::cstate_t* new_cstate);

	static void createUpdatePayloadBuffer (UpdatePayload* send_payload, char *data_buffer, char *payload_buffer, int* payload_size);
	static void extractUpdatePayloadBuffer (NetPacket* packet, UpdatePayload* payload, char* data_buffer);
	static void extractAckPayloadBuffer (NetPacket* packet, AckPayload* payload, char* data_buffer);

	static NetPacket makePacket(PacketType packet_type, char* payload_buffer, UINT32 payload_size, int sender_rank, int receiver_rank );
	static NetMatch makeNetMatch(PacketType packet_type, int sender_rank);
	/********************************/	
	
	bool initiateSharedMemReq(shmem_req_t shmem_req_type, ADDRINT ca_address, UINT32 addr_offset, char* data_buffer, UINT32 buffer_size);

	//TODO rename this function (and others that interface with Network)
	void addMemRequest(NetPacket req_packet);
//	void processSharedMemReq(NetPacket req_packet); << Moved to DRAM DIRECTORY
	void processUnexpectedSharedMemUpdate(NetPacket update_packet);

	static string sMemReqTypeToString(shmem_req_t type);
   void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);
	bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);

	/* ============================================== */
	/* Added by George */
//	void runDramAccessModel(void);
//	UINT64 getDramAccessCost(void);
	/* ============================================== */

	//write-back cache-line to DRAM
	void writeBackToDRAM();

	/* ============================================== */
	
	/*
	void setDramBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_boundaries);
	*/
};
#endif
