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
   m_num_local_cores = Sim()->getConfig()->getNumLocalTiles();
   
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
   
   if (Config::getSingleton()->getEnablePepCores())
   {
      m_stack_lower_limit = global_stack_base + 2 * num_tiles * m_stack_size_per_core;
      m_stack_upper_limit = m_stack_lower_limit + 2 * m_num_local_cores * m_stack_size_per_core;
   }
   else
   {
      m_stack_lower_limit = global_stack_base + num_tiles * m_stack_size_per_core;
      m_stack_upper_limit = m_stack_lower_limit + m_num_local_cores * m_stack_size_per_core;
   }
}

// Get Tile ID from stack pointer
tile_id_t PinConfig::getTileIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
   {
      return -1;
   }     

   SInt32 tile_index;

   if (Config::getSingleton()->getEnablePepCores())
      tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (2*m_stack_size_per_core));
   else
      tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (m_stack_size_per_core));

   return (Config::getSingleton()->getTileIDFromIndex(m_current_process_num, tile_index));
}

core_id_t PinConfig::getCoreIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
   {
      return INVALID_CORE_ID;
   }     
  

   if (Config::getSingleton()->getEnablePepCores())
   {
      // Stacks for each tile per core are stored adjacent to each other:  |main core 1|pep core 1|main core 2| ...
      SInt32 tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / (2 * m_stack_size_per_core));
      UInt32 core_type = (UInt32) (((stack_ptr - m_stack_lower_limit) - (tile_index * (2 * m_stack_size_per_core))) / m_stack_size_per_core);
      return ((core_id_t) {Config::getSingleton()->getTileIDFromIndex(m_current_process_num, tile_index), core_type});
   }
   else
   {
      SInt32 tile_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / m_stack_size_per_core);
      return ((core_id_t) {Config::getSingleton()->getTileIDFromIndex(m_current_process_num, tile_index), MAIN_CORE_TYPE});
   }
}

SInt32 PinConfig::getStackAttributesFromCoreID (core_id_t core_id, StackAttributes& stack_attr)
{
   // Get the stack attributes
   SInt32 tile_index = Config::getSingleton()->getIndexFromTileID(m_current_process_num, core_id.first);
   LOG_ASSERT_ERROR (tile_index != -1, "Tile %i does not belong to Process %i", 
         core_id.first, Config::getSingleton()->getCurrentProcessNum());

   if (Config::getSingleton()->getEnablePepCores())
   {
      stack_attr.lower_limit = m_stack_lower_limit + (tile_index * 2 * m_stack_size_per_core);

      if (core_id.second == PEP_CORE_TYPE)
      {
         stack_attr.lower_limit += m_stack_size_per_core;
      }

      stack_attr.size = m_stack_size_per_core;
   }
   else
   {
      stack_attr.lower_limit = m_stack_lower_limit + (tile_index * m_stack_size_per_core);
      stack_attr.size = m_stack_size_per_core;
   }
   return 0;
}

