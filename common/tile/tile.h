#ifndef TILE_H
#define TILE_H

#include <iostream>
using std::ostream;

// some forward declarations for cross includes
class Network;
class Core;
class MemoryManager;
class TileEnergyMonitor;
class RemoteQueryHelper;
class DVFSManager;

#include "fixed_types.h"
#include "network.h"

class Tile
{
public:
   Tile(tile_id_t id);
   ~Tile();

   void outputSummary(ostream &os);

   int getId()                         { return _id; }
   Network* getNetwork()               { return _network; }
   Core* getCore()                     { return _core; }
   MemoryManager* getMemoryManager()   { return _memory_manager; }
   DVFSManager* getDVFSManager()       { return _dvfs_manager; }
   TileEnergyMonitor* getTileEnergyMonitor()       { return _tile_energy_monitor; }
   RemoteQueryHelper* getRemoteQueryHelper()       { return _remote_query_helper; }

   Time getCoreTime(tile_id_t tile_id) const;

   static core_id_t getMainCoreId(tile_id_t id)    { return (core_id_t) {id, MAIN_CORE_TYPE}; }
   static bool isMainCore(core_id_t core_id)       { return (core_id.core_type == MAIN_CORE_TYPE); }

   void enableModels();
   void disableModels();

private:
   tile_id_t _id;
   Network* _network;
   Core* _core;
   MemoryManager* _memory_manager;
   DVFSManager* _dvfs_manager;
   TileEnergyMonitor* _tile_energy_monitor;
   RemoteQueryHelper* _remote_query_helper;

   Time getTargetCompletionTime();
};

#endif
