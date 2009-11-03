#include "address_home_lookup.h"
#include "log.h"

AddressHomeLookup::AddressHomeLookup(UInt32 ahl_param,
      UInt32 total_modules,
      UInt32 cache_block_size):
   m_ahl_param(ahl_param),
   m_total_modules(total_modules),
   m_cache_block_size(cache_block_size)
{

   // Each Block Address is as follows:
   // /////////////////////////////////////////////////////////// //
   //   block_num               |   block_offset                  //
   // /////////////////////////////////////////////////////////// //

   LOG_ASSERT_ERROR((1 << m_ahl_param) >= (SInt32) m_cache_block_size,
         "2^AHL param(%u) must be >= Cache Block Size(%u)",
         m_ahl_param, m_cache_block_size);
}

AddressHomeLookup::~AddressHomeLookup()
{
   // There is no memory to deallocate, so destructor has no function
}

SInt32 AddressHomeLookup::getHome(IntPtr address) const
{
   SInt32 module_num = (address >> m_ahl_param) % m_total_modules;
   LOG_ASSERT_ERROR(0 <= module_num && module_num < (SInt32) m_total_modules, "module_num(%i), total_modules(%u)", module_num, m_total_modules);
   
   LOG_PRINT("address(0x%x), module_num(%i)", address, module_num);
   return (module_num);
}
