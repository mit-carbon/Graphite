#ifndef THREAD_START_H
#define THREAD_START_H

#include <elf.h>
#include "pin.H"

int spawnThreadSpawner(CONTEXT *ctxt);
VOID copyStaticData(IMG& img);
VOID copyInitialStackData(ADDRINT reg_esp);
VOID copySpawnedThreadStackData(ADDRINT reg_esp);
VOID allocateStackSpace();

#endif
