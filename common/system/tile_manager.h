#ifndef TILE_MANAGER_H
#define TILE_MANAGER_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "fixed_types.h"
#include "tls.h"
#include "lock.h"

class Tile;
class Lock;
class Core;

class TileManager
{
public:
   TileManager();
   ~TileManager();

   void initializeCommId(SInt32 comm_id);
   void initializeThread();
   void initializeThread(core_id_t core_id);
   void terminateThread();
   tile_id_t registerSimThread();

   core_id_t getCurrentCoreID(); // id of currently active core (or INVALID_CORE_ID)
   tile_id_t getCurrentTileID(); // id of currently active core (or INVALID_CORE_ID)

   Tile *getCurrentTile();
   UInt32 getCurrentTileIndex();
   Tile *getTileFromID(tile_id_t id);
   Tile *getTileFromIndex(UInt32 index);

   Core* getCore(Tile *tile);
   Core *getCurrentCore();
   UInt32 getCurrentCoreIndex();
   Core *getCoreFromID(core_id_t id);
   static core_id_t getMainCoreId(tile_id_t tile_id);
   //Core *getCoreFromIndex(UInt32 index);

   void outputSummary(std::ostream &os);

   UInt32 getTileIndexFromID(tile_id_t tile_id);

   bool amiAppThread();
   bool amiSimThread();

private:

   void doInitializeThread(UInt32 tile_index);

   UInt32 *tid_map;
   TLS *m_tile_tls;
   TLS *m_tile_index_tls;
   TLS *m_thread_type_tls;

   enum ThreadType {
       INVALID,
       APP_THREAD,
       SIM_THREAD
   };

   std::vector<bool> m_initialized_cores;
   Lock m_initialized_cores_lock;

   UInt32 m_num_registered_sim_threads;
   Lock m_num_registered_sim_threads_lock;

   std::vector<Tile*> m_tiles;
};

#endif
