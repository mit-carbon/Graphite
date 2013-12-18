#pragma once

#include "directory_entry.h"
#include "bit_vector.h"
#include "random.h"

class DirectoryEntryFullMap : public DirectoryEntry
{
public:
   DirectoryEntryFullMap(SInt32 max_hw_sharers);
   ~DirectoryEntryFullMap();
   
   bool hasSharer(tile_id_t sharer_id);
   bool addSharer(tile_id_t sharer_id);
   void removeSharer(tile_id_t sharer_id, bool reply_expected);

   tile_id_t getOwner();
   void setOwner(tile_id_t owner_id);

   bool getSharersList(vector<tile_id_t>& sharers_list);
   tile_id_t getOneSharer();
   SInt32 getNumSharers();

   UInt32 getLatency();

private:
   BitVector* _sharers;
   Random<int> _rand_num;
};
