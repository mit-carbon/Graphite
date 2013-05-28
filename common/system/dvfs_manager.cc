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
map<module_t, pair<int, double> > DVFSManager::_dvfs_domain_map;

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
   if (tile_id < 0 || (unsigned int) tile_id >= Config::getSingleton()->getApplicationTiles())
   {
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

void
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

      case DIRECTORY:
         rc = _tile->getMemoryManager()->getDVFS(DIRECTORY, frequency, voltage);
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
}

void
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
      if (module_mask & DIRECTORY){
         rc_tmp = _tile->getMemoryManager()->setDVFS(DIRECTORY, frequency, voltage_flag, curr_time);
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
   UInt32 technology_node = 0;
   try
   {
      technology_node = Sim()->getCfg()->getInt("general/technology_node");
      _max_frequency = Sim()->getCfg()->getFloat("general/max_frequency");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not either read [general/technology_node] or [general/max_frequency] from the cfg file");
   }

   string input_filename = Sim()->getGraphiteHome() + "/technology/dvfs_levels_" + convertToString<UInt32>(technology_node) + "nm.cfg";
   ifstream input_file(input_filename.c_str());
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
      
      vector<string> tokens;
      splitIntoTokens(line, tokens, " ");
      double voltage = convertFromString<double>(tokens[0]);
      double frequency_factor = convertFromString<double>(tokens[1]);
      _dvfs_levels.push_back(make_pair(voltage, frequency_factor * _max_frequency));
   }
}


// Called to initialize DVFS domain map
void
DVFSManager::initializeDVFSDomainMap()
{
   string dvfs_domains_list_str;
   try
   {
      dvfs_domains_list_str = Sim()->getCfg()->getString("dvfs/dvfs_domains");
   }
   catch(...)
   {
      fprintf(stderr, "ERROR: Could not read [dvfs/dvfs_domains] from the cfg file\n");
      exit(EXIT_FAILURE);
   }

   vector<string> dvfs_domains_list_vec;
   parseList(dvfs_domains_list_str, dvfs_domains_list_vec, "<>");


   // parse each domain
   for (vector<string>::iterator list_it = dvfs_domains_list_vec.begin();
         list_it != dvfs_domains_list_vec.end(); list_it++)
   {
      vector<string> dvfs_parameter_tuple;
      parseList(*list_it, dvfs_parameter_tuple, ",");

      SInt32 param_num = 0; 
      SInt32 domain_number = 0; 
      double frequency = 0; 
      for (vector<string>::iterator param_it = dvfs_parameter_tuple.begin();
            param_it != dvfs_parameter_tuple.end(); param_it ++)
      {
         switch (param_num)
         {
            case 0:
               domain_number = convertFromString<UInt32>(*param_it);
               break;
   
            case 1:
               frequency = convertFromString<double>(*param_it);
               if (frequency <= 0)
               {
                  fprintf(stderr, "Error: Invalid frequency(%g GHz), must be greater than zero\n", frequency);
                  exit(EXIT_FAILURE);
               }

               break;
   
            default:
               string component = trimSpaces(*param_it);
   
               if (component == "CORE"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(CORE) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[CORE] = pair<int, double>(domain_number, frequency);
               }
               else if (component == "L1_ICACHE"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_ICACHE) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[L1_ICACHE] = pair<int, double>(domain_number, frequency);
               }
               else if (component == "L1_DCACHE"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_DCACHE) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  assert(_dvfs_domain_map.find(L1_DCACHE) == _dvfs_domain_map.end());
                  _dvfs_domain_map[L1_DCACHE] = pair<int, double>(domain_number, frequency);
               } 
               else if (component == "L2_CACHE"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(L2_CACHE) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[L2_CACHE] = pair<int, double>(domain_number, frequency);
               }
               else if (component == "DIRECTORY"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(DIRECTORY) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[DIRECTORY] = pair<int, double>(domain_number, frequency);
               }
               else if (component == "NETWORK_USER"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_USER) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[NETWORK_USER] = pair<int, double>(domain_number, frequency);
               }
               else if (component == "NETWORK_MEMORY"){
                  LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_MEMORY) == _dvfs_domain_map.end(),"DVFS component (%s) must be listed only once", component.c_str());
                  _dvfs_domain_map[NETWORK_MEMORY] = pair<int, double>(domain_number, frequency);
               }
               else{
                  fprintf(stderr, "Unrecognized DVFS component (%s)\n", component.c_str());
                  exit(EXIT_FAILURE);
               }
         }
         param_num ++;
      }
   }

   // check if all components are listed 
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(CORE) != _dvfs_domain_map.end(),"DVFS component CORE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_ICACHE) != _dvfs_domain_map.end(),"DVFS component L1_ICACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_DCACHE) != _dvfs_domain_map.end(),"DVFS component L1_DCACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L2_CACHE) != _dvfs_domain_map.end(),"DVFS component L2_CACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(DIRECTORY) != _dvfs_domain_map.end(),"DVFS component DIRECTORY must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_USER) != _dvfs_domain_map.end(),"DVFS component NETWORK_USER must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_MEMORY) != _dvfs_domain_map.end(),"DVFS component NETWORK_MEMORY must be listed in a DVFS domain");

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

int
DVFSManager::getInitialFrequencyAndVoltage(module_t component, volatile double &frequency, volatile double &voltage)
{
   frequency = _dvfs_domain_map[component].second;
   int rc = DVFSManager::getVoltage(voltage, AUTO, frequency);
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

bool
DVFSManager::hasSameDVFSDomain(module_t component1, module_t component2)
{
   return _dvfs_domain_map[component1].first == _dvfs_domain_map[component2].first;
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

