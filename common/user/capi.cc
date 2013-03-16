#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "tile.h"
#include "core.h"
#include "carbon_user.h"
#include "log.h"

CAPI_return_t CAPI_rank(int *tile_id)
{
   *tile_id = CarbonGetTileId();
   return 0;
}

CAPI_return_t CAPI_Initialize(int rank)
{
   Sim()->getTileManager()->initializeCommId(rank);
   return 0;
}

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver, 
      char *buffer, 
      int size)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = CAPI_ENDPOINT_ALL;
   if (receiver != CAPI_ENDPOINT_ALL)
      receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_tile, receiving_tile, buffer, size, (carbon_network_t) CARBON_NET_USER) : CAPI_SenderNotInitialized;
}

CAPI_return_t CAPI_message_send_w_ex(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver, 
      char *buffer, 
      int size,
      carbon_network_t net_type)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = CAPI_ENDPOINT_ALL;
   if (receiver != CAPI_ENDPOINT_ALL)
      receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_tile, receiving_tile, buffer, size, net_type) : CAPI_SenderNotInitialized;
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver,
      char *buffer, 
      int size)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = CAPI_ENDPOINT_ANY;
   if (sender != CAPI_ENDPOINT_ANY)
      sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_tile, receiving_tile, buffer, size, (carbon_network_t) CARBON_NET_USER) : CAPI_ReceiverNotInitialized;
}

CAPI_return_t CAPI_message_receive_w_ex(CAPI_endpoint_t sender, 
      CAPI_endpoint_t receiver,
      char *buffer, 
      int size,
      carbon_network_t net_type)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d, size: %d", sender, receiver, size);

   tile_id_t sending_tile = CAPI_ENDPOINT_ANY;
   if (sender != CAPI_ENDPOINT_ANY)
      sending_tile = Config::getSingleton()->getTileFromCommId(sender);
   
   tile_id_t receiving_tile = Config::getSingleton()->getTileFromCommId(receiver);

   if(sending_tile == INVALID_TILE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_tile == INVALID_TILE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_tile, receiving_tile, buffer, size, net_type) : CAPI_ReceiverNotInitialized;
}
