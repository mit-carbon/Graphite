#pragma once

#include <string>
#include "fixed_types.h"

class CacheParams
{
public:
   CacheParams(std::string type, UInt32 size, UInt32 blocksize,
         UInt32 associativity, UInt32 delay, double frequency):
      _type(type),
      _size(size),
      _blocksize(blocksize),
      _associativity(associativity),
      _delay(delay),
      _frequency(frequency)
   {}
   ~CacheParams() {}

   bool operator==(const CacheParams& cache_params)
   {  
      return ( (_type == cache_params._type) && 
               (_size == cache_params._size) && 
               (_blocksize == cache_params._blocksize) && 
               (_associativity == cache_params._associativity) &&
               (_delay == cache_params._delay) &&
               (_frequency == cache_params._frequency) );
   }

   std::string _type;
   UInt32 _size;
   UInt32 _blocksize;
   UInt32 _associativity;
   UInt32 _delay;
   double _frequency;
};

class CachePower
{
public:
   double _subthreshold_leakage_power;
   double _gate_leakage_power;
   double _tag_array_read_energy;
   double _tag_array_write_energy;
   double _data_array_read_energy;
   double _data_array_write_energy;
};

class CacheArea
{
public:
   double _area;
};
