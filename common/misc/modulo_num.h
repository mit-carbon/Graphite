#ifndef __MODULO_NUM_H__
#define __MODULO_NUM_H__

#include "fixed_types.h"

class ModuloNum
{
   private:
      UInt32 m_value;
      UInt32 m_max_value;

   public:
      ModuloNum(UInt32 max_value = -1, UInt32 value = 0);
      ~ModuloNum();

      UInt32 getValue() { return m_value; }
      UInt32 getMaxValue() { return m_max_value; }
      void setValue(UInt32 value) { m_value = value; }
      void setMaxValue(UInt32 max_value) { m_max_value = max_value; }

      ModuloNum operator+(ModuloNum& num);
      ModuloNum operator-(ModuloNum& num);
      ModuloNum operator+(UInt32 value);
      ModuloNum operator-(UInt32 value);
      bool operator==(ModuloNum& num);
      bool operator!=(ModuloNum& num);
};

#endif /* __MODULO_NUM_H__ */

