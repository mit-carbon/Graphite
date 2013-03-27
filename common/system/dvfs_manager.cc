#include "dvfs_manager.h"

DVFSManager::DVFSManager(UInt32 technology_node)
{
}

DVFSManager::~DVFSManager()
{
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
DVFSManager::getDVFSCallback(tile_id_t requester, module_t module_type)
{
}

void
DVFSManager::setDVFSCallback(tile_id_t requester, int module_mask, double frequency, double voltage)
{
}

