#include <cmath>
#include <cassert>
#include "small_data_vector.h"
#include "utils.h"
#include "log.h"

SmallDataVector::SmallDataVector(UInt32 vec_size, UInt32 element_size)
   : _vec_size(vec_size)
   , _element_size(element_size)
{
   if (_element_size > (8 * sizeof(UInt8)))
   {
      LOG_PRINT_ERROR("SmallDataVector: Max element size allowed = %u bits, Received element size = %u bits",
                      (8 * sizeof(UInt8)), _element_size);
   }
   _data = new UInt8[_vec_size];
   for (UInt32 i = 0; i < _vec_size; i++)
      _data[i] = (UInt8) 0;
}

SmallDataVector::~SmallDataVector()
{
   delete [] _data;
}

UInt32
SmallDataVector::get(UInt32 idx) const
{
   return (UInt32) _data[idx];
}

void
SmallDataVector::set(UInt32 idx, UInt32 element)
{
   _data[idx] = (UInt8) element;
}
