#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H


#include "debug.h"
#include "pin.H"
#include "dram_directory_entry.h"
#include "network.h"
#include "memory_manager.h"
#include <map>
#include <queue>

//TODO i don't think this is used
extern LEVEL_BASE::KNOB<UINT32> g_knob_dram_access_cost;

//LIMITED_DIRECTORY Flag
//Dir(i)NB ; i = number of pointers
//if MAX_SHARERS >= total number of cores, then the directory
//collaspes into the full-mapped case.
//TODO use a knob to set this instead
//(-dms) : directory_max_sharers
//TODO provide easy mechanism to initiate a broadcast invalidation
//	static const UINT32 MAX_SHARERS = 2;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dir_max_sharers;
enum shmem_req_t;


class SingleDramRequest {

	public:

		ADDRINT address;
		shmem_req_t shmem_req_type;
		UINT32 requestor;

		SingleDramRequest() : address(0), shmem_req_type(READ), requestor(0) {}

		SingleDramRequest(ADDRINT address, shmem_req_t shmem_req_type, UINT32 requestor) {
			this->address = address;
			this->shmem_req_type = shmem_req_type;
			this->requestor = requestor;
		}
};

class DramRequest {

	private:

		SingleDramRequest single_dram_req;	
		DramDirectoryEntry::dstate_t old_dstate;
		UINT32 num_acks_to_recv;

		std::queue<SingleDramRequest> waiting_requests;

	public:

		void setRequestAttributes(
				  							SingleDramRequest& single_dram_req, 
											DramDirectoryEntry::dstate_t old_dstate,
											UINT32 num_acks_to_recv
										 )
		{
			this->single_dram_req.address = single_dram_req.address;
			this->single_dram_req.shmem_req_type = single_dram_req.shmem_req_type;
			this->single_dram_req.requestor = single_dram_req.requestor;
			this->old_dstate = old_dstate;
			this->num_acks_to_recv = num_acks_to_recv;
		}

		void decNumAcksToRecv () {
			num_acks_to_recv --;
		}

		shmem_req_t getShmemReqType () {
			return (single_dram_req.shmem_req_type);
		}

		UINT32 getRequestor () {
			return (single_dram_req.requestor);
		}
		
		DramDirectoryEntry::dstate_t getOldDState() {
			return (old_dstate);
		}
		
		UINT32 getNumAcksToRecv () {
			return (num_acks_to_recv);
		}

		SingleDramRequest getNextRequest() {
			assert (waiting_requests.size() != 0);
			SingleDramRequest single_dram_req = waiting_requests.front();
			waiting_requests.pop();
			return (single_dram_req);
		}

		INT32 numWaitingRequests() {
			return (waiting_requests.size());
		}

		void addRequestToQueue(SingleDramRequest& single_dram_req) {
			waiting_requests.push (single_dram_req);
		}


};


/* Actual Directory Starts Here */

class DramDirectory
{
 private:
   //assumption: each dram_directory is tied to a given network (node), and is addressed at dram_id
	Network* the_network;
	UINT32 num_lines;
   unsigned int bytes_per_cache_line;
   UINT32 number_of_cores;
   //key dram entries on cache_line (assumes cache_line is 1:1 to dram memory lines)
   std::map<UINT32, DramDirectoryEntry*> dram_directory_entries;
   UINT32 dram_id;

	//state for re-entering the Directory
	std::map<ADDRINT, DramRequest*> dram_request_list;

	/* Added by George */
   UINT64 dramAccessCost;
   
	//TODO debugAssertValidStates();
	//scan the directory occasionally for invalid state configurations.
	//can that be done here? or only in the MMU since we may need cache access.

public:
	//is this a needed function?
	DramDirectoryEntry* getEntry(ADDRINT address);

   DramDirectory(UINT32 num_lines, UINT32 bytes_per_cache_line, UINT32 dram_id_arg, UINT32 num_of_cores, Network* network_arg);
   virtual ~DramDirectory();
   
   /***************************************/
	
	//receive and process request for memory_block
	void startSharedMemRequest(NetPacket& req_packet);
	void finishSharedMemRequest(ADDRINT address);
	void startNextSharedMemRequest(ADDRINT address);

	void processSharedMemRequest (UINT32 requestor, shmem_req_t shmem_req_type, ADDRINT address);
	
	void processAck(NetPacket& ack_packet);
	
	void startDemoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate);
	void startInvalidateAllSharers(DramDirectoryEntry* dram_dir_entry);
	void startInvalidateSingleSharer(DramDirectoryEntry* dram_dir_entry, UINT32 sharer_id);
	
	void processDemoteOwnerAck(UINT32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v, char *data_buffer, DramDirectoryEntry::dstate_t new_dstate);
	
	void processInvalidateSharerAck(UINT32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v, char *data_buffer);
	
	//make many of these private functions
	void copyDataToDram(ADDRINT address, char* data_buffer);
	//sending another memory line to another core. rename.
	void sendDataLine(DramDirectoryEntry* dram_dir_entry, UINT32 requestor, CacheState::cstate_t new_cstate);
	
	void processWriteBack(NetPacket& req_packet);
	
   void setNumberOfLines(UINT32 number_of_lines) { num_lines = number_of_lines; }

	void runDramAccessModel();
	UINT64 getDramAccessCost();
	
	/***************************************/
	
	void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data);
	bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data);

   /***************************************/

	void setDramMemoryLine(ADDRINT addr, char* data_buffer, UINT32 data_size);
	
	//for debug purposes
   void print();
};


#endif
