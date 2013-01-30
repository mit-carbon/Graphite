#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

#include "fixed_types.h"
#include "directory_block_info.h"
#include "directory_type.h"
#include "caching_protocol_type.h"

class DirectoryEntry
{
public:
   DirectoryEntry(SInt32 max_hw_sharers);
   virtual ~DirectoryEntry();

   static DirectoryType parseDirectoryType(string directory_type);
   static DirectoryEntry* create(CachingProtocolType caching_protocol_type, DirectoryType directory_type,
                                 SInt32 max_hw_sharers, SInt32 max_num_sharers);
   static UInt32 getSize(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers);

   DirectoryBlockInfo* getDirectoryBlockInfo();

   virtual bool hasSharer(tile_id_t sharer_id) = 0;
   virtual bool addSharer(tile_id_t sharer_id) = 0;
   virtual void removeSharer(tile_id_t sharer_id, bool reply_expected = false) = 0;

   tile_id_t getOwner();
   void setOwner(tile_id_t owner_id);

   IntPtr getAddress() { return _address; }
   void setAddress(IntPtr address) { _address = address; }

   virtual bool inBroadcastMode() { return false; }
   virtual bool getSharersList(vector<tile_id_t>& sharers_list) = 0;
   virtual tile_id_t getOneSharer() = 0;
   virtual SInt32 getNumSharers() = 0;

   virtual UInt32 getLatency() = 0;

protected:
   IntPtr _address;
   DirectoryBlockInfo* _directory_block_info;
   tile_id_t _owner_id;
   SInt32 _max_hw_sharers;

private:
   static DirectoryEntry* create(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers);
};
