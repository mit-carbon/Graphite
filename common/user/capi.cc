#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
#include "capi.h"
#include "config_file.hpp"

core_id_t CarbonGetCoreId()
{
   return Sim()->getCoreManager()->getCurrentCoreID();
}

CAPI_return_t CAPI_rank(int *core_id)
{
   *core_id = CarbonGetCoreId();
   return 0;
}

CAPI_return_t CAPI_Initialize(int rank)
{
   Sim()->getCoreManager()->initializeCommId(rank);
   return 0;
}

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d", sender, receiver);

   core_id_t sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   core_id_t receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   if(sending_core == INVALID_CORE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_core == INVALID_CORE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_core, receiving_core, buffer, size) : CAPI_SenderNotInitialized;
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d", sender, receiver);

   core_id_t sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   core_id_t receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   if(sending_core == INVALID_CORE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_core == INVALID_CORE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_core, receiving_core, buffer, size) : CAPI_ReceiverNotInitialized;
}
