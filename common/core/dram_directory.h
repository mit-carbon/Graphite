#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "pin.H"
#include "dram_directory_entry.h"
#include <map>

class DramDirectory
{
 private:
   UINT32 num_lines;
   unsigned int bytes_per_cache_line;
   UINT32 number_of_cores;
   //key dram entries on cache_line (assumes cache_line is 1:1 to dram memory lines)
   std::map<UINT32, DramDirectoryEntry*> dram_directory_entries;
   std::map<UINT32, char*> dram_directory_data;
   UINT32 dram_id;
   
	//TODO debugAssertValidStates();
	//scan the directory occasionally for invalid state configurations.
	//can that be done here? or only in the MMU since we may need cache access.

public:
   DramDirectory(UINT32 num_lines, UINT32 bytes_per_cache_line, UINT32 dram_id_arg, UINT32 num_of_cores);
   virtual ~DramDirectory();
   

	//receive and process request for memory_block
	void processSharedMemReq(Netpacket req_packet);
	
	void processWriteBack(Netpacket req_packet);
	
	
	DramDirectoryEntry* getEntry(ADDRINT address);
   void setNumberOfLines(UINT32 number_of_lines) { num_lines = number_of_lines; }

   void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);
	bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list);

   //for debug purposes
   void print();
};

#endif
