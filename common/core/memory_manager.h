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

#include "packet_type.h"
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

enum PacketType;

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

		RequestPayload()
			: request_type(READ), 
				request_address(0), 
				request_num_bytes(0)
		{}
	};	

	struct UpdatePayload {
		CacheState::cstate_t update_new_cstate;
		ADDRINT update_address;
		UINT32 data_size; //in bytes
		bool is_writeback; //is this payload serving as a writeback message to dram?
		
		UpdatePayload()
			: update_new_cstate(CacheState::INVALID), 
				update_address(0),
				data_size(0),
				is_writeback(false)
		{}
	};

	struct AckPayload {
		CacheState::cstate_t ack_new_cstate;
		ADDRINT ack_address;
		UINT32 data_size; //this is used to tell us how much data to extract 
		bool is_writeback; //when we invalidate/demote owners, we may need to do a writeback
		BOOL is_eviction; // need to know if "is_writeback" is true, if also is an eviction 
		//if sent a downgrade message (E->S), but cache
		//no longer has the line, send a bit to tell dram directory
		//to remove it from the sharers' list
		BOOL remove_from_sharers; //DEPRECATED
		
		AckPayload()
			:	ack_new_cstate(CacheState::INVALID),
				ack_address(0),
				data_size(0),
				is_writeback(false),
				is_eviction(false),
				remove_from_sharers(false)
		{}
	};

	MemoryManager(Core *the_core_arg, OCache *ocache_arg);
	virtual ~MemoryManager();

	DramDirectory* getDramDirectory() { return dram_dir;}
	
	/* ============================================== */
	
	//cache interfacing functions.
	void setCacheLineInfo(ADDRINT ca_address, CacheState::cstate_t new_cstate);
	pair<bool, CacheTag*> getCacheLineInfo(ADDRINT address);
	void accessCacheLineData(CacheBase::AccessType access_type, ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size);
	void invalidateCacheLine(ADDRINT address);
	
	static void createUpdatePayloadBuffer (UpdatePayload* send_payload, char *data_buffer, char *payload_buffer, UINT32 payload_size);
	static void createAckPayloadBuffer (AckPayload* send_payload, char *data_buffer, char *payload_buffer, UINT32 payload_size);
	static void extractUpdatePayloadBuffer (NetPacket* packet, UpdatePayload* payload, char* data_buffer);
	static void extractAckPayloadBuffer (NetPacket* packet, AckPayload* payload, char* data_buffer);

	static NetPacket makePacket(PacketType packet_type, char* payload_buffer, UINT32 payload_size, int sender_rank, int receiver_rank );
	static NetMatch makeNetMatch(PacketType packet_type, int sender_rank);
	
	/* ============================================== */
	
	//core traps all memory accesses here.
	bool initiateSharedMemReq(shmem_req_t shmem_req_type, ADDRINT ca_address, UINT32 addr_offset, char* data_buffer, UINT32 buffer_size);
	
	//request from DRAM permission to use an address
	//writes requested data into the "fill_buffer", and writes what the new_cstate should be on the receiving end
	void requestPermission(shmem_req_t shmem_req_type, ADDRINT address, CacheState::cstate_t* new_cstate);

	//network interrupt calls this.
	//Any requests are serialized onto a stack,
	//and then forwarded to the DRAM for processing.
	void addMemRequest(NetPacket req_packet);
	
	//network interrupt calls this. 
	//All UPDATE_UNEXPECTED packets are sent here.
	void processUnexpectedSharedMemUpdate(NetPacket update_packet);
	
	//network interrupt calls this.  
	//the MMU then forwards write-back packet to the DRAM.
	void forwardWriteBackToDram(NetPacket wb_packet);
	
	/* ============================================== */

	//debugging stuff
	static string sMemReqTypeToString(shmem_req_t type);
	void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);
	bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);

	
};
#endif
