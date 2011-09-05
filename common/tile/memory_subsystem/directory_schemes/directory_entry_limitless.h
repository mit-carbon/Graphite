#pragma once

#include "directory_entry_limited.h"
#include "bit_vector.h"

class DirectoryEntryLimitless : public DirectoryEntryLimited
{
public:
   DirectoryEntryLimitless(SInt32 max_hw_sharers, SInt32 max_num_sharers);
   ~DirectoryEntryLimitless();
   
   bool hasSharer(tile_id_t sharer_id);
   bool addSharer(tile_id_t sharer_id);
   void removeSharer(tile_id_t sharer_id, bool reply_expected);

   bool getSharersList(vector<tile_id_t>& sharers_list);
   SInt32 getNumSharers();

   UInt32 getLatency();

private:
   // Software Sharers
   BitVector* _software_sharers;

   // Max Num Sharers - For Software Trap
   SInt32 _max_num_sharers;

   // Software Trap Variables
   bool _software_trap_enabled;
   static UInt32 _software_trap_penalty;
   static bool _software_trap_penalty_initialized;
};
