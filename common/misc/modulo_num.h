#ifndef __MODULO_NUM_H__
#define __MODULO_NUM_H__

#include "fixed_types.h"

class ModuloNum
{
   public:
      ModuloNum(UInt32 max_value = -1, UInt32 value = 0);
      ~ModuloNum();

      ModuloNum operator+(ModuloNum& num);
      ModuloNum operator-(ModuloNum& num);
      ModuloNum operator+(UInt32 value);
      ModuloNum operator-(UInt32 value);
      bool operator==(ModuloNum& num);
      bool operator!=(ModuloNum& num);
      
      UInt32 _value;
      UInt32 _max_value;

};

#endif /* __MODULO_NUM_H__ */

