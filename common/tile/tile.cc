#include "tile.h"
#include "network.h"
#include "network_model.h"
#include "network_types.h"
#include "memory_manager.h"
#include "main_core.h"
#include "simulator.h"
#include "log.h"
#include <string.h>

Tile::Tile(tile_id_t id)
   : _id(id)
   , _memory_manager(NULL)
{
   LOG_PRINT("Tile ctor for (%i)", _id);

   _frequency = Config::getSingleton()->getTileFrequency(_id);
   _network = new Network(this);
   _core = new MainCore(this);
   
   if (Config::getSingleton()->isSimulatingSharedMemory())
      _memory_manager = MemoryManager::createMMU(Sim()->getCfg()->getString("caching_protocol/type"), this);

   // Register callback for clock frequency change
   getNetwork()->registerCallback(FREQ_CONTROL, TileFreqScalingCallback, this);
}

Tile::~Tile()
{
   getNetwork()->unregisterCallback(FREQ_CONTROL);

   if (_memory_manager)
      delete _memory_manager;
   delete _core;
   delete _network;
}

void TileFreqScalingCallback(void* obj, NetPacket packet)
{
   Tile *tile = (Tile*) obj;
   assert(tile != NULL);

   core_id_t sender = packet.sender;
   volatile float new_frequency;
   memcpy((void*) &new_frequency, packet.data, sizeof(float));

   float old_frequency = tile->getFrequency();
   tile->updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
   tile->setFrequency(new_frequency);
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

void
Tile::updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency)
{
   _core->updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
   _memory_manager->updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
}

void
Tile::acquireLock()
{
   _core->acquireLock();
   _memory_manager->acquireLock();
}

void
Tile::releaseLock()
{
   _memory_manager->releaseLock();
   _core->releaseLock();
}
