#include "pin_config.h"
#include "thread_spawn.h"
#include "thread_support.h"
#include "simulator.h"
#include "thread_manager.h"

VOID SimPthreadAttrInitOtherAttr(pthread_attr_t *attr)
{
   core_id_t core_id;
   
   ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();
   core_id = req->core_id;

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreID(core_id, stack_attr);

   pthread_attr_setstack(attr, (void*) stack_attr.lower_limit, stack_attr.size);
}
