#pragma once

#include <string>
using std::string;

namespace HybridProtocol_PPMOSI_SS
{

class Mode
{
public:
   enum Type
   {
      INVALID = 0,
      PRIVATE,
      REMOTE_LINE,
      REMOTE_SHARER
   };

   static string getName(Type type);
};

#define SPELL_MODE(x)      (Mode::getName(x).c_str())

}
