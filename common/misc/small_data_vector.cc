#include <cmath>
#include <cassert>
#include "small_data_vector.h"
#include "utils.h"
#include "log.h"

SmallDataVector::SmallDataVector(UInt32 vec_size, UInt32 element_size)
   : _vec_size(vec_size)
   , _element_size(element_size)
{
   if (_element_size > ((8 * sizeof(UInt32)) - 1))
   {
      LOG_PRINT_ERROR("SmallDataVector: Max element size allowed = %u bits, Received element size = %u bits",
                      ((8*sizeof(UInt32)) - 1), _element_size);
   }
   _total_size = (UInt32) ceil( (1.0 * _vec_size * _element_size) / 32);
   _data = new UInt32[_total_size];
   for (UInt32 i = 0; i < _total_size; i++)
      _data[i] = (UInt32) 0;
}

SmallDataVector::~SmallDataVector()
{
   delete [] _data;
}

UInt32
SmallDataVector::get(UInt32 idx)
{
   UInt32 id = (idx * _element_size) / 32;
   UInt32 offset = (idx * _element_size) % 32;
   
   UInt32 element = 0;
   UInt32 bits_left = _element_size;
   UInt32 bits_copied = 0;

   while (bits_left > 0)
   {
      UInt32 bits_to_copy = getMin<UInt32>(bits_left, 32 - offset);
      assert(bits_to_copy <= 32);
      UInt32 element_subset = (_data[id] >> offset) & ((1 << bits_to_copy) - 1);
      element |= (element_subset << bits_copied);

      bits_left -= bits_to_copy;
      bits_copied += bits_to_copy;
      assert(bits_copied <= _element_size);
      assert(bits_left >= 0);
      id ++;
      offset = 0;
   }

   return element;
}

void
SmallDataVector::set(UInt32 idx, UInt32 element)
{
   if ( element >= ((UInt32) (1 << _element_size)) )
   {
      LOG_PRINT_WARNING("Element [%u] uses %i (>%u) bits. Truncated to [%u]",
                        element, floorLog2(element)+1, _element_size, element & ((1 << _element_size) - 1));
   }

   UInt32 id = (idx * _element_size) / 32;
   UInt32 offset = (idx * _element_size) % 32;

   UInt32 bits_left = _element_size;
   UInt32 bits_copied = 0;

   while (bits_left > 0)
   {
      UInt32 bits_to_copy = min<UInt32>(bits_left, 32 - offset);
      assert(bits_to_copy <= 32);
      UInt32 element_subset = (element >> bits_copied) & ((1 << bits_to_copy) - 1);
      _data[id] &= ~( ((1 << (bits_to_copy + offset)) - 1) - ((1 << offset) - 1) );
      _data[id] |= (element_subset << offset);

      bits_left -= bits_to_copy;
      bits_copied += bits_to_copy;
      assert(bits_copied <= _element_size);
      assert(bits_left >= 0);
      id ++;
      offset = 0;
   }
}
