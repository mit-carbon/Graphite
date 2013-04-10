#include "dvfs.h"
#include "fxsupport.h"
#include "log.h"
#include "core.h"
#include "tile_manager.h"
#include "tile.h"
#include "simulator.h"

// Get DVFS
int CarbonGetDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage)
{
   int rc;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   rc = tile->getDVFSManager()->getDVFS(tile_id, module_type, frequency, voltage);

   return rc;
}

int CarbonGetFrequency(tile_id_t tile_id, module_t module_type, double* frequency)
{
   int rc;
   double voltage;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   rc = tile->getDVFSManager()->getDVFS(tile_id, module_type, frequency, &voltage);
   return rc;
}

int CarbonGetVoltage(tile_id_t tile_id, module_t module_type, double* voltage)
{
   int rc;
   double frequency;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   rc = tile->getDVFSManager()->getDVFS(tile_id, module_type, &frequency, voltage);
   return rc;
}

// Set DVFS
int CarbonSetDVFS(tile_id_t tile_id, int module_mask, double* frequency, voltage_option_t voltage_flag)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;
   
   int rc;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   rc = tile->getDVFSManager()->setDVFS(tile_id, module_mask, *frequency, voltage_flag);

   return rc;
}
