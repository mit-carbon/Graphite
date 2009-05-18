#ifndef __THREAD_START_H__
#define __THREAD_START_H__

#include <elf.h>
#include "pin.H"

int spawnThreadSpawner(CONTEXT *ctxt);
void terminateThreadSpawner();
VOID copyStaticData(IMG& img);
VOID copyInitialStackData(ADDRINT reg_esp);
VOID copySpawnedThreadStackData(ADDRINT reg_esp);
VOID allocateStackSpace();

// Initialize stack attributes
VOID SimPthreadAttrInitOtherAttr(pthread_attr_t *attr);

#endif /* __THREAD_START_H__ */
