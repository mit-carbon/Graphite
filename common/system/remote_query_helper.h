#pragma once

#include "network.h"

class Tile;

// Remote Query Type
enum RemoteQueryType
{
   CORE_TIME
};

// Callback
void RemoteQueryHelperNetworkCallback(void* obj, NetPacket packet);

class RemoteQueryHelper
{
public:
   RemoteQueryHelper(Tile* tile);
   ~RemoteQueryHelper();

   // Local
   Time getCoreTime(tile_id_t tile_id);
   // Remote
   void __getCoreTime(tile_id_t sender);

   // Handle query
   void handleQuery(const NetPacket& packet);

private:
   Tile* _tile;
};
