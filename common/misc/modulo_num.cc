#include <cassert>

#include "modulo_num.h"

ModuloNum::ModuloNum(UInt32 max_value, UInt32 value) :
   _value(value),
   _max_value(max_value)
{}


ModuloNum::~ModuloNum()
{}

ModuloNum
ModuloNum::operator+(ModuloNum& num)
{
   assert(_max_value == num._max_value);

   ModuloNum new_num(_max_value);
   new_num._value = (_value + num._value) % _max_value;
   return new_num;
}

ModuloNum
ModuloNum::operator-(ModuloNum& num)
{
   assert(_max_value == num._max_value);

   ModuloNum new_num(_max_value);
   if (_value >= num._value)
      new_num._value = _value - num._value;
   else
      new_num._value = _value + _max_value - num._value;
   return new_num;
}

ModuloNum
ModuloNum::operator+(UInt32 value)
{
   ModuloNum num(_max_value);
   num._value = value % _max_value;
   return (*this + num);
}

ModuloNum
ModuloNum::operator-(UInt32 value)
{
   ModuloNum num(_max_value);
   num._value = value % _max_value;
   return (*this - num);
}

bool
ModuloNum::operator==(ModuloNum& num)
{
   return ((_value == num._value) && (_max_value == num._max_value));
}

bool
ModuloNum::operator!=(ModuloNum& num)
{
   return (!(*this == num));
}
