#include "pin_config.h"
#include "thread_spawn.h"

VOID CarbonPthreadAttrInit(pthread_attr_t *attr)
{
   pthread_attr_init(attr);
   pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE);

   core_id_t core_id;
   
   ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();
   core_id = req->core_id;

   PinConfig::StackAttributes stack_attr;
   PinConfig->getSingleton()->getStackAttributesFromCoreID(core_id, &stack_attr);

   pthread_attr_setstack(&attr, stack_attr.base, stack_attr.size);
}
