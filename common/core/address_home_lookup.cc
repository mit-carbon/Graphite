#include "address_home_lookup.h"

AddressHomeLookup::AddressHomeLookup(SInt32 core_id)
{

   // Each Block Address is as follows:
   // /////////////////////////////////////////////////////////// //
   //   block_num               |   block_offset                  //
   // /////////////////////////////////////////////////////////// //

   m_core_id = core_id;

   m_total_cores = Config::getSingleton()->getTotalCores();
   m_ahl_param = Config::getSingleton()->getAHLParam();
   m_cache_line_size = Config::getSingleton()->getCacheLineSize();

   assert ( (unsigned) (1 << m_ahl_param) >= m_cache_line_size);

}

UInt32 AddressHomeLookup::find_home_for_addr(IntPtr address) const
{
   UInt32 node = (address >> m_ahl_param) % m_total_cores;
   assert(0 <= node && node < m_total_cores);
   return (node);
}

AddressHomeLookup::~AddressHomeLookup()
{
   // There is no memory to deallocate, so destructor has no function
}

#if 0
int main(int argc, char *argv[])
{

   AddressHomeLookup addrLookupTable(13, 5, 0);

   assert(addrLookupTable.find_home_for_addr((152 << 5) | 31) == 9);
   assert(addrLookupTable.find_home_for_addr((0 << 5) | 0) == 0);
   assert(addrLookupTable.find_home_for_addr((169 << 5) | 31) == 0);
   assert(addrLookupTable.find_home_for_addr((168 << 5) | 31) == 12);
   assert(addrLookupTable.find_home_for_addr(12345) == 8);

   cout << "All Tests Passed\n";
}
#endif

