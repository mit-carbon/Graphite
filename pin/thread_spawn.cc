#include "pin_config.h"
#include "thread_spawn.h"

VOID SimPthreadAttrInitOtherAttr(pthread_attr_t *attr)
{
   core_id_t core_id;
   
   ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();
   core_id = req->core_id;

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreID(core_id, &stack_attr);

   pthread_attr_setstack(attr, stack_attr.lower_limit, stack_attr.size);
}
