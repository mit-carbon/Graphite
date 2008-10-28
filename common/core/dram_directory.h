#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "debug.h"
#include "pin.H"
#include "dram_directory_entry.h"
#include "network.h"
#include <map>

//TODO i don't think this is used
extern LEVEL_BASE::KNOB<UINT32> g_knob_dram_access_cost;

class DramDirectory
{
 private:
   Network* the_network; //assumption: each dram_directory is tied to a given network (node), and is addressed at dram_id
	UINT32 num_lines;
   unsigned int bytes_per_cache_line;
   UINT32 number_of_cores;
   //key dram entries on cache_line (assumes cache_line is 1:1 to dram memory lines)
   std::map<UINT32, DramDirectoryEntry*> dram_directory_entries;
   std::map<UINT32, char*> dram_directory_data;
   UINT32 dram_id;
   
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
	//make many of these private functions
	void copyDataToDram(ADDRINT address, char* data_buffer);
	//sending another memory line to another core. rename.
	void sendDataLine(DramDirectoryEntry* dram_dir_entry, UINT32 requestor, CacheState::cstate_t new_cstate);
	void invalidateSharers(DramDirectoryEntry* dram_dir_entry);

	NetPacket demoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate);

	void runDramAccessModel();
	UINT64 getDramAccessCost();
	
	//receive and process request for memory_block
	void processSharedMemReq(NetPacket req_packet);
	
	void processWriteBack(NetPacket req_packet);
	
   void setNumberOfLines(UINT32 number_of_lines) { num_lines = number_of_lines; }

   
	/***************************************/
	
	void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);
	bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);

   /***************************************/

	void setDramMemoryLine(ADDRINT addr, char* data_buffer, UINT32 data_size);
	
	//for debug purposes
   void print();
};

#endif
