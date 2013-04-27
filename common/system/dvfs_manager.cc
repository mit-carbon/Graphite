#include <string.h>
#include <utility>
#include <iostream>
using std::make_pair;
#include "dvfs_manager.h"
#include "memory_manager.h"
#include "simulator.h"
#include "tile.h"
#include "core.h"
#include "network_model.h"
#include "core_model.h"
#include "packetize.h"
#include "utils.h"

DVFSManager::DVFSLevels DVFSManager::_dvfs_levels;
volatile double DVFSManager::_max_frequency;

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
   // Invalid tile id error
   if (tile_id < 0 || (unsigned int) tile_id >= Config::getSingleton()->getApplicationTiles()){
      *frequency = 0;
      *voltage = 0;
      return -1;
   }

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
DVFSManager::setDVFS(tile_id_t tile_id, int module_mask, double frequency, voltage_option_t voltage_flag)
{
   // Invalid tile error
   if (tile_id < 0 || (unsigned int) tile_id >= Config::getSingleton()->getApplicationTiles())
      return -1;

   // Get current time
   Time curr_time = _tile->getCore()->getModel()->getCurrTime();

   // send request
   UnstructuredBuffer send_buffer;
   send_buffer << module_mask << frequency << voltage_flag << curr_time;
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
   int rc = 0;
   switch (module_type)
   {
      case CORE:
         _tile->getCore()->getDVFS(frequency, voltage);
         break;

      case L1_ICACHE:
         rc = _tile->getMemoryManager()->getDVFS(L1_ICACHE, frequency, voltage);
         break;

      case L1_DCACHE:
         rc = _tile->getMemoryManager()->getDVFS(L1_DCACHE, frequency, voltage);
         break;

      case L2_CACHE:
         rc = _tile->getMemoryManager()->getDVFS(L2_CACHE, frequency, voltage);
         break;

      case L2_DIRECTORY:
         rc = _tile->getMemoryManager()->getDVFS(L2_DIRECTORY, frequency, voltage);
         break;

      case NETWORK_USER:
      {
         NetworkModel* user_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(USER);
         rc = user_network_model->getDVFS(frequency, voltage);
         break;
      }

      case NETWORK_MEMORY:
      {
         NetworkModel* mem_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(SHARED_MEM);
         rc = mem_network_model->getDVFS(frequency, voltage);
         break;
      }
      
      default:
         rc = -2;
         frequency = 0;
         voltage = 0;
         break;
   }

   UnstructuredBuffer send_buffer;
   send_buffer << rc << frequency << voltage;
   _tile->getNetwork()->netSend(requester, DVFS_GET_REPLY, send_buffer.getBuffer(), send_buffer.size());

   return rc;
}

int
DVFSManager::doSetDVFS(int module_mask, double frequency, voltage_option_t voltage_flag, const Time& curr_time, core_id_t requester)
{
   int rc = 0, rc_tmp = 0;

   // Invalid module mask
   if (module_mask <= 0 || module_mask>=MAX_MODULE_TYPES){ 
      rc = -2;
   }

   // Invalid voltage option
   else if (voltage_flag != HOLD && voltage_flag != AUTO){ 
      rc = -3;
   }

   // Invalid frequency
   else if (frequency <= 0 || frequency > _max_frequency){
      rc = -4;
   }

   // Parse mask and set frequency and voltage
   else{
      if (module_mask & CORE){
         rc_tmp = _tile->getCore()->setDVFS(frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
      if (module_mask & L1_ICACHE){
         rc_tmp = _tile->getMemoryManager()->setDVFS(L1_ICACHE, frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
      if (module_mask & L1_DCACHE){
         rc_tmp = _tile->getMemoryManager()->setDVFS(L1_DCACHE, frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
      if (module_mask & L2_CACHE){
         rc_tmp = _tile->getMemoryManager()->setDVFS(L2_CACHE, frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
      if (module_mask & L2_DIRECTORY){
         rc_tmp = _tile->getMemoryManager()->setDVFS(L2_DIRECTORY, frequency, voltage_flag, curr_time);
         if (rc_tmp != 0 && module_mask != TILE) rc = rc_tmp;
      }
      if (module_mask & NETWORK_USER){
         NetworkModel* user_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(USER);
         rc_tmp = user_network_model->setDVFS(frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
      if (module_mask & NETWORK_MEMORY){
         NetworkModel* mem_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(SHARED_MEM);
         rc_tmp = mem_network_model->setDVFS(frequency, voltage_flag, curr_time);
         if (rc_tmp != 0) rc = rc_tmp;
      }
   }

   UnstructuredBuffer send_buffer;
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
   voltage_option_t voltage_flag;
   Time curr_time;
   
   recv_buffer >> module_mask;
   recv_buffer >> frequency;
   recv_buffer >> voltage_flag;
   recv_buffer >> curr_time;

   DVFSManager* dvfs_manager = (DVFSManager* ) obj;
   dvfs_manager->doSetDVFS(module_mask, frequency, voltage_flag, curr_time, packet.sender);
}

// Called to initialize DVFS Levels
void
DVFSManager::initializeDVFSLevels()
{
   UInt32 technology_node = Sim()->getCfg()->getInt("general/technology_node");
   string input_filename = Sim()->getGraphiteHome() + "/technology/dvfs_levels_" + convertToString<UInt32>(technology_node) + "nm.cfg";
   ifstream input_file(input_filename.c_str());
   bool first_line = true;
   while (1)
   {
      char line_c[1024];
      input_file.getline(line_c, 1024);
      string line(line_c);
      if (input_file.gcount() == 0)
         break;
      line = trimSpaces(line);
      if (line == "")
         continue;
      if (line[0] == '#')  // Comment
         continue;
      if (first_line)
      {
         _max_frequency = convertFromString<double>(line);
         first_line = false;
      }
      else
      {
         vector<string> tokens;
         splitIntoTokens(line, tokens, " ");
         double voltage = convertFromString<double>(tokens[0]);
         double frequency_factor = convertFromString<double>(tokens[1]);
         _dvfs_levels.push_back(make_pair(voltage, frequency_factor * _max_frequency));
      }
   }
}

int
DVFSManager::getVoltage(volatile double &voltage, voltage_option_t voltage_flag, double frequency) 
{
   int rc = 0;

   switch (voltage_flag)
   {
      case HOLD:
         // above max frequency for current voltage
         if (voltage < getMinVoltage(frequency)){
            rc = -5;
         }
         break;

      case AUTO:
         voltage = getMinVoltage(frequency);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized voltage flag %i.", voltage_flag);
         break;
   }
   
   return rc;
}

const DVFSManager::DVFSLevels&
DVFSManager::getDVFSLevels()
{
   return _dvfs_levels;
}

double
DVFSManager::getMaxFrequency(double voltage) 
{
   for (DVFSLevels::const_iterator it = _dvfs_levels.begin(); it != _dvfs_levels.end(); it++)
   {
      if (voltage == (*it).first)
         return (*it).second;
   }
   LOG_PRINT_ERROR("Could not find voltage(%g V) in DVFS Levels list", voltage);
   return 0.0;
}

double
DVFSManager::getMinVoltage(double frequency) 
{
   for (DVFSLevels::const_reverse_iterator it = _dvfs_levels.rbegin(); it != _dvfs_levels.rend(); it++)
   {
      if (frequency <= (*it).second)
         return (*it).first;
   }
   LOG_PRINT_ERROR("Could not determine voltage for frequency(%g GHz)", frequency);
   return 0.0;
}

