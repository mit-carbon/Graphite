#pragma once

#include <string>
using std::string;

#include "fixed_types.h"
#include "mem_component.h"
#include "core.h"

class MemOp
{
public:
   MemOp()
      : _mem_component(MemComponent::INVALID)
      , _type(Core::READ)
      , _lock_signal(Core::NONE)
      , _address(INVALID_ADDRESS)
      , _data_buf(NULL)
      , _offset(0)
      , _data_length(0)
      , _modeled(false)
   {}
   MemOp(MemComponent::Type mem_component, Core::mem_op_t type, Core::lock_signal_t lock_signal,
         IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length, bool modeled)
      : _mem_component(mem_component)
      , _type(type)
      , _lock_signal(lock_signal)
      , _address(address)
      , _data_buf(data_buf)
      , _offset(offset)
      , _data_length(data_length)
      , _modeled(modeled)
   {}
   ~MemOp()
   {}

   static string getName(Core::mem_op_t mem_op_type);

   MemComponent::Type getMemComponent()   { return _mem_component;   }
   Core::mem_op_t getType()               { return _type;            }
   Core::lock_signal_t getLockSignal()    { return _lock_signal;     }
   IntPtr getAddress()                    { return _address;         }
   Byte* getDataBuf()                     { return _data_buf;        }
   UInt32 getOffset()                     { return _offset;          }
   UInt32 getDataLength()                 { return _data_length;     }
   bool isModeled()                       { return _modeled;         }

private:
   MemComponent::Type _mem_component;
   Core::mem_op_t _type;
   Core::lock_signal_t _lock_signal;
   IntPtr _address;
   Byte* _data_buf;
   UInt32 _offset;
   UInt32 _data_length;
   bool _modeled;
};

#define SPELL_MEMOP(x)     (MemOp::getName(x).c_str())
