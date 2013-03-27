#include "dvfs_manager.h"
#include "tile.h"

DVFSManager::DVFSManager(UInt32 technology_node, Tile* tile):
   _tile(tile)
{
   // register callbacks
   _tile->getNetwork()->registerCallback(DVFS_SET, setDVFSCallback, this);
   _tile->getNetwork()->registerCallback(DVFS_GET, getDVFSCallback, this);
}

DVFSManager::~DVFSManager()
{
   // unregister callback
   _tile->getNetwork()->unregisterCallback(DVFS_SET);
   _tile->getNetwork()->unregisterCallback(DVFS_GET);
}

// Called from common/user/dvfs
int
DVFSManager::getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage)
{
   return 0;
}

int
DVFSManager::setDVFS(tile_id_t tile_id, int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag)
{
   return 0;
}

int
DVFSManager::setDVFS(tile_id_t tile_id, int module_mask, double frequency, double voltage)
{
   return 0;
}

// Called over the network (callbacks)
void
getDVFSCallback(void* obj, NetPacket packet)
{
}

void
setDVFSCallback(void* obj, NetPacket packet)
{
}

