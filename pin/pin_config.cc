#include "pin_config.h"
#include "simulator.h"
#include <boost/lexical_cast.hpp>
#include "core.h"
PinConfig *PinConfig::m_singleton = NULL;

void PinConfig::allocate()
{
   assert (m_singleton == NULL);
   m_singleton = new PinConfig();
}

void PinConfig::release()
{
   delete m_singleton;
   m_singleton = NULL;
}

PinConfig *PinConfig::getSingleton()
{
   return m_singleton;
}

PinConfig::PinConfig()
{
   m_current_process_num = Sim()->getConfig()->getCurrentProcessNum();
   m_total_tiles = Sim()->getConfig()->getTotalTiles();
   m_num_local_tiles = Sim()->getConfig()->getNumLocalTiles();
   m_max_threads_per_core = Sim()->getConfig()->getMaxThreadsPerCore();
   
   setStackBoundaries();
}

PinConfig::~PinConfig()
{
}

// INIT function to set the stack limits
void PinConfig::setStackBoundaries()
{
   IntPtr global_stack_base;

   try
   {
      global_stack_base = (IntPtr) boost::lexical_cast <unsigned long int> (Sim()->getCfg()->get("stack/stack_base"));
      m_stack_size_per_core = (IntPtr) boost::lexical_cast <unsigned long int> (Sim()->getCfg()->get("stack/stack_size_per_core"));
   }
   catch (const boost::bad_lexical_cast&)
   {
      cerr << "Error parsing stack parameters from the config file" << endl;
      exit (-1);
   }

   // To calculate our stack base, we need to get the total number of cores
   // allocated to processes that have ids' lower than me
   UInt32 num_tiles = 0;
   for (SInt32 i = 0; i < (SInt32) m_current_process_num; i++)
   {
      num_tiles += Sim()->getConfig()->getNumTilesInProcess(i);
   }

   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();

   m_stack_lower_limit = global_stack_base + num_cores_per_tile * num_tiles * m_stack_size_per_core;
   m_stack_upper_limit = m_stack_lower_limit + num_cores_per_tile * m_num_local_tiles * m_stack_size_per_core;
}

// Get Tile ID from stack pointer
tile_id_t PinConfig::getTileIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
      return INVALID_THREAD_ID;

   SInt32 tile_index;

   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();

   tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (num_cores_per_tile * m_stack_size_per_core));

   return (Config::getSingleton()->getTileIDFromIndex(m_current_process_num, tile_index));
}

core_id_t PinConfig::getCoreIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
      return INVALID_CORE_ID;
  

   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();
   SInt32 tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (num_cores_per_tile * m_stack_size_per_core));

   return (Config::getSingleton()->getMainCoreIDFromIndex(m_current_process_num, tile_index));
}


thread_id_t PinConfig::getThreadIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
      return INVALID_THREAD_ID;
  
   UInt32 stack_size_per_thread = m_stack_size_per_core/m_max_threads_per_core;
   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();

   SInt32 tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (num_cores_per_tile * m_stack_size_per_core));
   thread_id_t thread_id = (SInt32) ((stack_ptr - (tile_index * (num_cores_per_tile * m_stack_size_per_core))) / stack_size_per_thread);

   return thread_id;
}


// The stack looks like | main core 0 | main core 1 | 
SInt32 PinConfig::getStackAttributesFromCoreID (core_id_t core_id, StackAttributes& stack_attr)
{
   // Get the stack attributes
   SInt32 tile_index = Config::getSingleton()->getIndexFromTileID(m_current_process_num, core_id.tile_id);
   LOG_ASSERT_ERROR (tile_index != -1, "Tile %i does not belong to Process %i", 
         core_id.tile_id, Config::getSingleton()->getCurrentProcessNum());

   UInt32 stack_size_per_thread = m_stack_size_per_core/m_max_threads_per_core;
   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();

   stack_attr.lower_limit = m_stack_lower_limit + (tile_index * num_cores_per_tile * m_stack_size_per_core);

   stack_attr.size = stack_size_per_thread;

   return 0;
}

// The stack looks like | main core 0 | main core 1 | main core 2 | main core 3 | 
//                      |tid0|tid1|...|tid0|tid1|...|tid0|tid1|...|tid0|tid1|..|
SInt32 PinConfig::getStackAttributesFromCoreAndThreadID (core_id_t core_id, thread_id_t thread_idx, StackAttributes& stack_attr)
{
   // Get the stack attributes
   SInt32 tile_index = Config::getSingleton()->getIndexFromTileID(m_current_process_num, core_id.tile_id);
   LOG_ASSERT_ERROR (tile_index != -1, "Tile %i does not belong to Process %i", 
         core_id.tile_id, Config::getSingleton()->getCurrentProcessNum());

   SInt32 thread_index = thread_idx;

   UInt32 num_cores_per_tile = Config::getSingleton()->getNumCoresPerTile();
   UInt32 stack_size_per_thread = m_stack_size_per_core/m_max_threads_per_core;


   stack_attr.lower_limit = m_stack_lower_limit + (tile_index * num_cores_per_tile * m_stack_size_per_core);
   stack_attr.lower_limit += thread_index * stack_size_per_thread;
   stack_attr.size = stack_size_per_thread;

   return 0;
}
