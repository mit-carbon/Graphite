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
class Core;

class TileManager
{
public:
   TileManager();
   ~TileManager();

   void initializeCommId(SInt32 comm_id);
   void initializeThread(core_id_t core_id, SInt32 thread_index = 0, thread_id_t thread_id = 0);
   void terminateThread();
   tile_id_t registerSimThread();

   core_id_t getCurrentCoreID(); // id of currently active core (or INVALID_CORE_ID)
   tile_id_t getCurrentTileID(); // id of currently active core (or INVALID_TILE_ID)

   Tile *getCurrentTile();
   UInt32 getCurrentTileIndex();
   Tile *getTileFromID(tile_id_t id);
   Tile *getTileFromIndex(UInt32 index);

   Core *getCurrentCore();
   Core *getCoreFromID(core_id_t id);

   thread_id_t getCurrentThreadIndex();
   thread_id_t getCurrentThreadID();

   void updateTLS(UInt32 tile_index, SInt32 thread_index, thread_id_t thread_id);

   void outputSummary(std::ostream &os);

   UInt32 getTileIndexFromID(tile_id_t tile_id);

   bool amiAppThread();
   bool amiSimThread();

private:

   void doInitializeThread(UInt32 tile_index, SInt32 thread_index, thread_id_t thread_id);

   TLS *m_tile_tls;
   TLS *m_tile_index_tls;
   TLS *m_thread_id_tls;
   TLS *m_thread_index_tls;
   TLS *m_thread_type_tls;

   enum ThreadType {
       INVALID,
       APP_THREAD,
       SIM_THREAD
   };

   bool** m_initialized_threads;
   Lock m_initialized_threads_lock;

   std::vector<UInt32> m_num_initialized_threads;
   Lock m_num_initialized_threads_lock;

   std::vector<bool> m_initialized_cores;
   Lock m_initialized_cores_lock;

   UInt32 m_num_registered_sim_threads;
   Lock m_num_registered_sim_threads_lock;

   std::vector<Tile*> m_tiles;
   UInt32 m_max_threads_per_core;
};

#endif
