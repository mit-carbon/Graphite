#ifndef MAIN_CORE_H
#define MAIN_CORE_H

#include "core.h"

class MainCore : public Core
{
public:
   MainCore(Tile* tile) : Core(tile, MAIN_CORE_TYPE)  {}
   ~MainCore() {}
};

#endif
