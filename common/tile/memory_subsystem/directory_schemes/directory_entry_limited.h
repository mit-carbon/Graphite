#pragma once

#include "directory_entry.h"
#include "random.h"

class DirectoryEntryLimited : public DirectoryEntry
{
public:
   DirectoryEntryLimited(SInt32 max_hw_sharers);
   ~DirectoryEntryLimited();

   bool hasSharer(tile_id_t sharer_id);
   bool addSharer(tile_id_t sharer_id);
   void removeSharer(tile_id_t sharer_id);

   bool getSharersList(vector<tile_id_t>& sharers);
   tile_id_t getOneSharer();
   SInt32 getNumSharers();

protected:
   vector<SInt16> _sharers;
   SInt32 _num_tracked_sharers;
   static const SInt16 INVALID_SHARER = 0xffff;

private:
   Random<int> _rand_num;
};
