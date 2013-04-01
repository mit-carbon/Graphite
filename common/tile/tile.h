#ifndef TILE_H
#define TILE_H

#include <iostream>
using std::ostream;

// some forward declarations for cross includes
class Network;
class Core;
class MemoryManager;
class TileEnergyMonitor;

#include "fixed_types.h"
#include "network.h"
#include "dvfs_manager.h"

void TileFreqScalingCallback(void* obj, NetPacket packet);

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

   static core_id_t getMainCoreId(tile_id_t id)    { return (core_id_t) {id, MAIN_CORE_TYPE}; }
   static bool isMainCore(core_id_t core_id)       { return (core_id.core_type == MAIN_CORE_TYPE); }

   float getFrequency() const          { return _frequency; }
   void setFrequency(float frequency)  { _frequency = frequency; }

   void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);

   void enableModels();
   void disableModels();

private:
   tile_id_t _id;
   Network* _network;
   Core* _core;
   MemoryManager* _memory_manager;
   DVFSManager* _dvfs_manager;
   TileEnergyMonitor* _tile_energy_monitor;

   volatile float _frequency;
};

#endif
