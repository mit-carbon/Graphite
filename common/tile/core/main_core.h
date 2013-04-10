#ifndef MAIN_CORE_H
#define MAIN_CORE_H

#include "core.h"

class MainCore : public Core
{
public:
   MainCore(Tile* tile, double frequency, double voltage) : Core(tile, MAIN_CORE_TYPE, frequency, voltage)  {}
   ~MainCore() {}
};

#endif
