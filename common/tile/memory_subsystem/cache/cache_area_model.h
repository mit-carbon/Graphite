#pragma once

#include <string>
#include "fixed_types.h"

class CacheAreaModel
{
public:
   CacheAreaModel(std::string type, UInt32 size, UInt32 blocksize,
         UInt32 associativity, UInt32 delay, float frequency);
   ~CacheAreaModel() {}

   void outputSummary(std::ostream& out);
   static void dummyOutputSummary(std::ostream& out);

private:
   double _area;
};
