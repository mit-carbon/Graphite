#pragma once

#include "fixed_types.h"

class SmallDataVector
{
public:
   SmallDataVector(UInt32 vec_size, UInt32 element_size);
   ~SmallDataVector();

   UInt32 get(UInt32 idx) const;
   void set(UInt32 idx, UInt32 element);

private:
   UInt8* _data;
   UInt32 _vec_size;
   UInt32 _element_size;
};
