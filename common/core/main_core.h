#ifndef MAIN_CORE_H
#define MAIN_CORE_H

#include <string.h>

// some forward declarations for cross includes
class CorePerfModel;

#include "core.h"

using namespace std;

class MainCore : protected Core
{
   public:

      MainCore(Tile* tile);
      ~MainCore();

      ClockSkewMinimizationClient* getClockSkewMinimizationClient() { return m_clock_skew_minimization_client; }
      
      private:
      
      ClockSkewMinimizationClient *m_clock_skew_minimization_client;
};

#endif
