#include <string.h>
#include "tile.h"
#include "network.h"
#include "network_model.h"
#include "network_types.h"
#include "memory_manager.h"
#include "main_core.h"
#include "simulator.h"
#include "log.h"
#include "tile_energy_monitor.h"

Tile::Tile(tile_id_t id)
   : _id(id)
   , _memory_manager(NULL)
   , _tile_energy_monitor(NULL)
{
   LOG_PRINT("Tile ctor for (%i)", _id);

   double frequency = Config::getSingleton()->getTileFrequency(_id);
   double voltage = 0.0;
   int rc = DVFSManager::getVoltage(voltage, AUTO, frequency);
   LOG_ASSERT_ERROR(rc == 0, "Error setting initial voltage for frequency(%g)", frequency);

   _network = new Network(this);
   _core = new MainCore(this, frequency, voltage);
   
   if (Config::getSingleton()->isSimulatingSharedMemory())
      _memory_manager = MemoryManager::createMMU(Sim()->getCfg()->getString("caching_protocol/type"), this, frequency, voltage);

   if (Config::getSingleton()->getEnablePowerModeling())
      _tile_energy_monitor = new TileEnergyMonitor(this);
   
   // Create DVFS manager
   UInt32 technology_node = Sim()->getCfg()->getInt("general/technology_node");
   _dvfs_manager = new DVFSManager(technology_node, this);
   
}

Tile::~Tile()
{
   delete _dvfs_manager;
   if (_memory_manager)
      delete _memory_manager;
   delete _core;
   delete _network;
   if (_tile_energy_monitor)
      delete _tile_energy_monitor;
}

void Tile::outputSummary(ostream &os)
{
   LOG_PRINT("Core Summary");
   _core->outputSummary(os);

   LOG_PRINT("Memory Subsystem Summary");
   if (_memory_manager)
      _memory_manager->outputSummary(os);

   LOG_PRINT("Network Summary");
   _network->outputSummary(os);
   
   LOG_PRINT("Tile Energy Monitor Summary");
   if (_tile_energy_monitor)
      _tile_energy_monitor->outputSummary(os);
}

void Tile::enableModels()
{
   LOG_PRINT("enableModels(%i) start", _id);
   _network->enableModels();
   _core->enableModels();
   if (_memory_manager)
      _memory_manager->enableModels();
   LOG_PRINT("enableModels(%i) end", _id);
}

void Tile::disableModels()
{
   LOG_PRINT("disableModels(%i) start", _id);
   _network->disableModels();
   _core->disableModels();
   if (_memory_manager)
      _memory_manager->disableModels();
   LOG_PRINT("disableModels(%i) end", _id);
}

