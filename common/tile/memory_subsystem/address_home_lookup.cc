#include "address_home_lookup.h"
#include "log.h"

AddressHomeLookup::AddressHomeLookup(UInt32 ahl_param, vector<tile_id_t>& tile_list, UInt32 cache_line_size):
   _ahl_param(ahl_param),
   _tile_list(tile_list),
   _cache_line_size(cache_line_size)
{
   LOG_ASSERT_ERROR((1 << _ahl_param) >= (SInt32) _cache_line_size,
                    "[1 << AHL param](%u) must be >= [Cache Block Size](%u)",
                    1 << _ahl_param, _cache_line_size);
   _total_modules = tile_list.size();
}

AddressHomeLookup::~AddressHomeLookup()
{}

tile_id_t
AddressHomeLookup::getHome(IntPtr address) const
{
   SInt32 module_num = (address >> _ahl_param) % _total_modules;
   LOG_ASSERT_ERROR(0 <= module_num && module_num < (SInt32) _total_modules, "module_num(%i), total_modules(%u)", module_num, _total_modules);
   
   LOG_PRINT("address(%#lx), module_num(%i)", address, module_num);
   return (_tile_list[module_num]);
}
