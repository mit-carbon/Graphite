#ifndef PEP_CORE_H
#define PEP_CORE_H

#include <string.h>

// some forward declarations for cross includes
class CorePerfModel;

#include "core.h"

using namespace std;

class PepCore : protected Core
{
   public:

      PepCore(Tile* tile);
      ~PepCore();

      ClockSkewMinimizationClient* getClockSkewMinimizationClient() { return NULL; }

      private:
      
};

#endif
