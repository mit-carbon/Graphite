#include "dvfs_manager.h"
#include "tile.h"
#include "packetize.h"
#include <string.h>
#include <utility>

DVFSManager::DVFSManager(UInt32 technology_node, Tile* tile):
   _tile(tile)
{
   // register callbacks
   _tile->getNetwork()->registerCallback(DVFS_SET_REQUEST, setDVFSCallback, this);
   _tile->getNetwork()->registerCallback(DVFS_GET_REQUEST, getDVFSCallback, this);
}

DVFSManager::~DVFSManager()
{
   // unregister callback
   _tile->getNetwork()->unregisterCallback(DVFS_SET_REQUEST);
   _tile->getNetwork()->unregisterCallback(DVFS_GET_REQUEST);
}

// Called from common/user/dvfs
int
DVFSManager::getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage)
{
   // send request
   UnstructuredBuffer send_buffer;
   send_buffer << module_type;
   core_id_t core_id = {tile_id, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(core_id, DVFS_GET_REQUEST, send_buffer.getBuffer(), send_buffer.size());

   // receive reply
   core_id_t recv_id = {_tile->getId(), MAIN_CORE_TYPE};
   NetPacket packet = _tile->getNetwork()->netRecv(core_id, recv_id, DVFS_GET_REPLY);
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   recv_buffer >> *frequency;
   recv_buffer >> *voltage;

   return 0;
}

int
DVFSManager::setDVFS(tile_id_t tile_id, int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag)
{
   // TODO:
   // 1. figure out voltage. Currently just passing zero.
   // 2. make sure that flag combination is valid.
   // 3. send return code.
   
   double voltage = 1.0;
   
   UnstructuredBuffer send_buffer;
   send_buffer << module_mask << frequency << voltage;
   core_id_t core_id = {tile_id, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(core_id, DVFS_SET_REQUEST, send_buffer.getBuffer(), send_buffer.size());

   return 0;
}

int
DVFSManager::doSetDVFS(int module_mask, double frequency, double voltage)
{
   // parse mask and set frequency and voltage
   if (module_mask & CORE){
      //_tile->getCore()->setFrequency(frequency);
      //_tile->getCore()->setVoltage(voltage);
   }

   if (module_mask & L1_ICACHE){
//      _tile->getCore()->setFrequency(frequency);
//      _tile->getCore()->setVoltage(voltage);
   }
   if (module_mask & L1_DCACHE){
//      _tile->getCore()->setFrequency(frequency);
//      _tile->getCore()->setVoltage(voltage);
   }
   if (module_mask & L2_CACHE){
//      _tile->getCore()->setFrequency(frequency);
//      _tile->getCore()->setVoltage(voltage);
   }

   return 0;
}

int
DVFSManager::doGetDVFS(module_t module_type, tile_id_t requester)
{
   double frequency, voltage;
   switch (module_type)
   {
      case CORE:
      {
         frequency = 2.5;
         voltage = 1.5;
         //*frequency = _tile->getCore()->getFrequency();
         //*voltage = _tile->getCore()->getVoltage();
         break;
      }

      case L1_ICACHE:
         //*frequency = _tile->getCore()->getFrequency();
         //*voltage = _tile->getCore()->getVoltage();
         break;

      case L1_DCACHE:
         //*frequency = _tile->getCore()->getFrequency();
         //*voltage = _tile->getCore()->getVoltage();
         break;

      case L2_CACHE:
         //*frequency = _tile->getCore()->getFrequency();
         //*voltage = _tile->getCore()->getVoltage();
         break;


      case TILE:
         LOG_PRINT_ERROR("You must specify a submodule within the tile.", module_type);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized module type %i.", module_type);
         break;
   }

   UnstructuredBuffer send_buffer;
   send_buffer << frequency << voltage;

   core_id_t core_id = {requester, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(core_id, DVFS_GET_REPLY, send_buffer.getBuffer(), send_buffer.size());

   return 0;
}

// Called over the network (callbacks)
void
getDVFSCallback(void* obj, NetPacket packet)
{
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   module_t module_type;
   
   recv_buffer >> module_type;

   DVFSManager* dvfs_manager = (DVFSManager* ) obj;
   dvfs_manager->doGetDVFS(module_type, packet.sender.tile_id);
}

void
setDVFSCallback(void* obj, NetPacket packet)
{
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   int module_mask;
   double frequency;
   double voltage;
   
   recv_buffer >> module_mask;
   recv_buffer >> frequency;
   recv_buffer >> voltage;

   DVFSManager* dvfs_manager = (DVFSManager* ) obj;
   dvfs_manager->doSetDVFS(module_mask, frequency, voltage);
}

