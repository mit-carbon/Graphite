#include <string.h>
#include <utility>
using std::make_pair;
#include "dvfs_manager.h"
#include "tile.h"
#include "packetize.h"

DVFSManager::DVFSLevels DVFSManager::_dvfs_levels;

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

// Called to initialize DVFS Levels
void
DVFSManager::initializeDVFSLevels()
{
   _dvfs_levels.push_back(make_pair(0.9,1.0));
}

// Called from common/user/dvfs
int
DVFSManager::getDVFS(tile_id_t tile_id, module_t module_type, double* frequency, double* voltage)
{
   // send request
   UnstructuredBuffer send_buffer;
   send_buffer << module_type;
   core_id_t remote_core_id = {tile_id, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(remote_core_id, DVFS_GET_REQUEST, send_buffer.getBuffer(), send_buffer.size());

   // receive reply
   core_id_t this_core_id = {_tile->getId(), MAIN_CORE_TYPE};
   NetPacket packet = _tile->getNetwork()->netRecv(remote_core_id, this_core_id, DVFS_GET_REPLY);
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   int rc;
   recv_buffer >> rc;
   recv_buffer >> *frequency;
   recv_buffer >> *voltage;

   return rc;
}

int
DVFSManager::setDVFS(tile_id_t tile_id, int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag)
{
   // TODO:
   // 1. figure out voltage. Currently just passing zero.
   // 2. make sure that flag combination is valid.
   // 3. send return code.
   
   // send request
   UnstructuredBuffer send_buffer;
   send_buffer << module_mask << frequency << frequency_flag << voltage_flag;
   core_id_t remote_core_id = {tile_id, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(remote_core_id, DVFS_SET_REQUEST, send_buffer.getBuffer(), send_buffer.size());

   // receive reply
   core_id_t this_core_id = {_tile->getId(), MAIN_CORE_TYPE};
   NetPacket packet = _tile->getNetwork()->netRecv(remote_core_id, this_core_id, DVFS_SET_REPLY);
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   int rc;
   recv_buffer >> rc;

   return rc;
}


int
DVFSManager::doGetDVFS(module_t module_type, core_id_t requester)
{
   double frequency, voltage;
   switch (module_type)
   {
      case CORE:
      {
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

   int rc = 0;
   UnstructuredBuffer send_buffer;
   send_buffer << rc << frequency << voltage;
   _tile->getNetwork()->netSend(requester, DVFS_GET_REPLY, send_buffer.getBuffer(), send_buffer.size());

   return rc;
}

int
DVFSManager::doSetDVFS(int module_mask, double frequency, dvfs_option_t frequency_flag, dvfs_option_t voltage_flag, core_id_t requester)
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

   UnstructuredBuffer send_buffer;
   int rc = 0;
   send_buffer << rc;

   _tile->getNetwork()->netSend(requester, DVFS_SET_REPLY, send_buffer.getBuffer(), send_buffer.size());

   return rc;
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
   dvfs_manager->doGetDVFS(module_type, packet.sender);
}

void
setDVFSCallback(void* obj, NetPacket packet)
{
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   int module_mask;
   double frequency;
   dvfs_option_t frequency_flag;
   dvfs_option_t voltage_flag;
   
   recv_buffer >> module_mask;
   recv_buffer >> frequency;
   recv_buffer >> frequency_flag;
   recv_buffer >> voltage_flag;

   DVFSManager* dvfs_manager = (DVFSManager* ) obj;
   dvfs_manager->doSetDVFS(module_mask, frequency, frequency_flag, voltage_flag, packet.sender);
}

// Called from the McPAT interfaces
double
DVFSManager::getNominalVoltage()
{
   return _dvfs_levels.front().first;
}

double
DVFSManager::getMaxFrequencyFactorAtVoltage(double voltage)
{
   for (DVFSLevels::iterator it = _dvfs_levels.begin(); it != _dvfs_levels.end(); it++)
   {
      if (voltage == (*it).first)
         return (*it).second;
   }
   LOG_PRINT_ERROR("Could not find voltage(%g) in DVFS Levels list", voltage);
   return 0.0;
}
