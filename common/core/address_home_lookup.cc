#include "address_home_lookup.h"
#include "config.h"
#include "log.h"

AddressHomeLookup::AddressHomeLookup(UInt32 ahl_param,
      core_id_t core_id, 
      UInt32 cache_block_size):
   m_ahl_param(ahl_param),
   m_core_id(core_id),
   m_cache_block_size(cache_block_size)
{

   // Each Block Address is as follows:
   // /////////////////////////////////////////////////////////// //
   //   block_num               |   block_offset                  //
   // /////////////////////////////////////////////////////////// //

   m_total_cores = Config::getSingleton()->getTotalCores();

   LOG_ASSERT_ERROR((1 << m_ahl_param) >= (SInt32) m_cache_block_size,
         "2^AHL param(%u) must be >= Cache Block Size(%u)",
         m_ahl_param, m_cache_block_size);
}

AddressHomeLookup::~AddressHomeLookup()
{
   // There is no memory to deallocate, so destructor has no function
}

UInt32 AddressHomeLookup::getHome(IntPtr address) const
{
   core_id_t node = (address >> m_ahl_param) % m_total_cores;
   assert(0 <= node && node < (core_id_t) m_total_cores);
   return (node);
}
