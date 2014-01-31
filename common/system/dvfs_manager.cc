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
#include "tile_energy_monitor.h"

DVFSManager::DVFSLevels DVFSManager::_dvfs_levels;
double DVFSManager::_max_frequency;
DVFSManager::DomainType DVFSManager::_dvfs_domain_map;
UInt32 DVFSManager::_synchronization_delay_cycles;

DVFSManager::DVFSManager(UInt32 technology_node, Tile* tile):
   _tile(tile)
{
   // register callbacks
   _tile->getNetwork()->registerCallback(DVFS_SET_REQUEST, setDVFSCallback, this);
   _tile->getNetwork()->registerCallback(DVFS_GET_REQUEST, getDVFSCallback, this);
   _tile->getNetwork()->registerCallback(GET_TILE_ENERGY_REQUEST, getTileEnergyCallback, this);
}

DVFSManager::~DVFSManager()
{
   // unregister callback
   _tile->getNetwork()->unregisterCallback(DVFS_SET_REQUEST);
   _tile->getNetwork()->unregisterCallback(DVFS_GET_REQUEST);
   _tile->getNetwork()->unregisterCallback(GET_TILE_ENERGY_REQUEST);
}

// Called from common/user/dvfs
int
DVFSManager::getDVFS(tile_id_t tile_id, module_t module_mask, double* frequency, double* voltage)
{
   // Invalid tile id error
   if (tile_id < 0 || (unsigned int) tile_id >= Config::getSingleton()->getApplicationTiles())
   {
      return -1;
   }

   // Invalid domain error
   bool valid_domain = false;
   for (DomainType::iterator it = _dvfs_domain_map.begin(); it != _dvfs_domain_map.end(); it++){
      if (module_mask == it->second.first){
         valid_domain = true;
         break;
      }
   }
   if (!valid_domain) return -2;

   // send request
   UnstructuredBuffer send_buffer;
   send_buffer << module_mask;
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

   // Invalid domain error
   bool valid_domain = false;
   for (DomainType::iterator it = _dvfs_domain_map.begin(); it != _dvfs_domain_map.end(); it++){
      if (module_mask == it->second.first){
         valid_domain = true;
         break;
      }
   }
   if (!valid_domain) return -2;
   
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
DVFSManager::doGetDVFS(module_t module_mask, core_id_t requester)
{
   double frequency, voltage;
   int rc = 0;
   if (module_mask & CORE){
      rc = _tile->getCore()->getDVFS(frequency, voltage);
   }
   else if (module_mask & L1_ICACHE){
      rc = _tile->getMemoryManager()->getDVFS(L1_ICACHE, frequency, voltage);
   }
   else if (module_mask & L1_DCACHE){
      rc = _tile->getMemoryManager()->getDVFS(L1_DCACHE, frequency, voltage);
   }
   else if (module_mask & L2_CACHE){
      rc = _tile->getMemoryManager()->getDVFS(L2_CACHE, frequency, voltage);
   }
   else if (module_mask & DIRECTORY){
      rc = _tile->getMemoryManager()->getDVFS(DIRECTORY, frequency, voltage);
   }
   else if (module_mask & NETWORK_USER){
      NetworkModel* user_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(USER);
      rc = user_network_model->getDVFS(frequency, voltage);
   }
   else if (module_mask & NETWORK_MEMORY){
      NetworkModel* mem_network_model = _tile->getNetwork()->getNetworkModelFromPacketType(SHARED_MEM);
      rc = mem_network_model->getDVFS(frequency, voltage);
   }
   else{
      rc = -2;
   }

   UnstructuredBuffer send_buffer;
   send_buffer << rc << frequency << voltage;
   _tile->getNetwork()->netSend(requester, DVFS_GET_REPLY, send_buffer.getBuffer(), send_buffer.size());
}

void
DVFSManager::doSetDVFS(int module_mask, double frequency, voltage_option_t voltage_flag, const Time& curr_time, core_id_t requester)
{
   int rc = 0, rc_tmp = 0;

   // Invalid voltage option
   if (voltage_flag != HOLD && voltage_flag != AUTO){ 
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
      if ((MemoryManager::getCachingProtocolType() != PR_L1_SH_L2_MSI) && (module_mask & DIRECTORY)){
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


// Called to initialize DVFS
void
DVFSManager::initializeDVFS()
{
   DVFSManager::initializeDVFSLevels();
   DVFSManager::initializeDVFSDomainMap();
   _synchronization_delay_cycles =  Sim()->getCfg()->getInt("dvfs/synchronization_delay");
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
   _dvfs_domain_map[INVALID_MODULE] = pair<module_t, double>(INVALID_MODULE, _max_frequency);

   string dvfs_domains_list_str;
   try
   {
      dvfs_domains_list_str = Sim()->getCfg()->getString("dvfs/domains");
   }
   catch(...)
   {
      fprintf(stderr, "ERROR: Could not read [dvfs/domains] from the cfg file\n");
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
      double frequency = 0; 
      module_t domain_mask = INVALID_MODULE;
      vector<module_t> domain_modules;
      vector<string> domain_modules_str;
      for (vector<string>::iterator param_it = dvfs_parameter_tuple.begin();
            param_it != dvfs_parameter_tuple.end(); param_it ++)
      {
         if (param_num == 0){
            frequency = convertFromString<double>(*param_it);
            if (frequency <= 0)
            {
               fprintf(stderr, "Error: Invalid frequency(%g GHz), must be greater than zero\n", frequency);
               exit(EXIT_FAILURE);
            }
         }
         else{
            // create domain mask
            string module = trimSpaces(*param_it);
            domain_modules_str.push_back(module);
            if (module == "CORE"){
               domain_mask = (module_t) (domain_mask | CORE);
               domain_modules.push_back(CORE);
            }
            else if (module == "L1_ICACHE"){
               domain_mask = (module_t) (domain_mask | L1_ICACHE);
               domain_modules.push_back(L1_ICACHE);
            }
            else if (module == "L1_DCACHE"){
               domain_mask = (module_t) (domain_mask | L1_DCACHE);
               domain_modules.push_back(L1_DCACHE);
            }
            else if (module == "L2_CACHE"){
               domain_mask = (module_t) (domain_mask | L2_CACHE);
               domain_modules.push_back(L2_CACHE);
            }
            else if (module == "DIRECTORY"){
               domain_mask = (module_t) (domain_mask | DIRECTORY);
               domain_modules.push_back(DIRECTORY);
            }
            else if (module == "NETWORK_USER"){
               domain_mask = (module_t) (domain_mask | NETWORK_USER);
               domain_modules.push_back(NETWORK_USER);
            }
            else if (module == "NETWORK_MEMORY"){
               domain_mask = (module_t) (domain_mask | NETWORK_MEMORY);
               domain_modules.push_back(NETWORK_MEMORY);
            }
            else{
               fprintf(stderr, "Unrecognized DVFS module (%s)\n", module.c_str());
               exit(EXIT_FAILURE);
            }
         }

         param_num ++;
      }

      pair<module_t,double> domain_value(domain_mask, frequency);
      for (unsigned int i=0; i<domain_modules.size(); i++){
         LOG_ASSERT_ERROR(_dvfs_domain_map.find(domain_modules[i]) ==
            _dvfs_domain_map.end(), "DVFS module (%s) can only be listed once",
            domain_modules_str[i].c_str());
         _dvfs_domain_map[domain_modules[i]] = domain_value;
      }
   }

   // check if all modules are listed 
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(CORE) != _dvfs_domain_map.end(),"DVFS module CORE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_ICACHE) != _dvfs_domain_map.end(),"DVFS module L1_ICACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L1_DCACHE) != _dvfs_domain_map.end(),"DVFS module L1_DCACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(L2_CACHE) != _dvfs_domain_map.end(),"DVFS module L2_CACHE must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(DIRECTORY) != _dvfs_domain_map.end(),"DVFS module DIRECTORY must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_USER) != _dvfs_domain_map.end(),"DVFS module NETWORK_USER must be listed in a DVFS domain");
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(NETWORK_MEMORY) != _dvfs_domain_map.end(),"DVFS module NETWORK_MEMORY must be listed in a DVFS domain");

}

UInt32 DVFSManager::getSynchronizationDelay()
{
   return _synchronization_delay_cycles;
}

int
DVFSManager::getVoltage(double &voltage, voltage_option_t voltage_flag, double frequency) 
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
DVFSManager::getInitialFrequencyAndVoltage(module_t module, double &frequency, double &voltage)
{
   frequency = _dvfs_domain_map[module].second;
   int rc = DVFSManager::getVoltage(voltage, AUTO, frequency);
   return rc;
}

void
DVFSManager::getTileEnergy(tile_id_t tile_id, double *energy)
{

   // send request
   UnstructuredBuffer send_buffer;
   core_id_t remote_core_id = {tile_id, MAIN_CORE_TYPE};
   _tile->getNetwork()->netSend(remote_core_id, GET_TILE_ENERGY_REQUEST, send_buffer.getBuffer(), send_buffer.size());

   // receive reply
   core_id_t this_core_id = {_tile->getId(), MAIN_CORE_TYPE};
   NetPacket packet = _tile->getNetwork()->netRecv(remote_core_id, this_core_id, GET_TILE_ENERGY_REPLY);
   UnstructuredBuffer recv_buffer;
   recv_buffer << std::make_pair(packet.data, packet.length);

   recv_buffer >> *energy;
}

void DVFSManager::doGetTileEnergy(core_id_t requester)
{

   double energy = _tile->getTileEnergyMonitor()->getTileEnergy();

   UnstructuredBuffer send_buffer;
   send_buffer << energy;

   _tile->getNetwork()->netSend(requester, GET_TILE_ENERGY_REPLY, send_buffer.getBuffer(), send_buffer.size());
}

void
getTileEnergyCallback(void* obj, NetPacket packet)
{

   DVFSManager* dvfs_manager = (DVFSManager* ) obj;
   dvfs_manager->doGetTileEnergy(packet.sender);
}

const DVFSManager::DVFSLevels&
DVFSManager::getDVFSLevels()
{
   return _dvfs_levels;
}

module_t DVFSManager::getDVFSDomain(module_t module_type)
{
   LOG_ASSERT_ERROR(_dvfs_domain_map.find(module_type) != _dvfs_domain_map.end(),
      "Cannot get DVFS domain: Invalid module(%d).", module_type);
   return _dvfs_domain_map[module_type].first;
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
DVFSManager::hasSameDVFSDomain(module_t module1, module_t module2)
{
   return _dvfs_domain_map[module1].first == _dvfs_domain_map[module2].first;
}

module_t
DVFSManager::convertToModule(MemComponent::Type component){
   switch(component){
      case MemComponent::L1_ICACHE:
         return L1_ICACHE;
      case MemComponent::L1_DCACHE:
         return L1_DCACHE;
      case MemComponent::L2_CACHE:
         return L2_CACHE;
      case MemComponent::DRAM_DIRECTORY:
         return DIRECTORY;
      default:
         LOG_PRINT_ERROR("Unknown memory component.");
         break;
   }
   return INVALID_MODULE;
}

void
DVFSManager::printAsynchronousMap(ostream& os, module_t module, AsynchronousMap &asynchronous_map)
{
   if (asynchronous_map.empty())
      return;

   AsynchronousMap::iterator it;
   for (it = asynchronous_map.begin(); it != asynchronous_map.end(); it++){
      if (it->second.getTime() != 0)
         break;
   }
   if (it == asynchronous_map.end())
      return;

   string padding("  ");
   if (module == CORE)
      padding = "";
   os << padding + "  Asynchronous communication: " << endl;
   if (asynchronous_map.find(CORE) != asynchronous_map.end() && 
         !hasSameDVFSDomain(module, CORE))
      os << padding + "    Core (in nanoseconds): " << asynchronous_map[CORE].toNanosec() << endl;
   if (asynchronous_map.find(L1_ICACHE) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, L1_ICACHE))
      os << padding + "    L1-I Cache (in nanoseconds): " << asynchronous_map[L1_ICACHE].toNanosec() << endl;
   if (asynchronous_map.find(L1_DCACHE) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, L1_DCACHE))
      os << padding + "    L1-D Cache (in nanoseconds): " << asynchronous_map[L1_DCACHE].toNanosec() << endl;
   if (asynchronous_map.find(L2_CACHE) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, L2_CACHE))
      os << padding + "    L2 Cache (in nanoseconds): " << asynchronous_map[L2_CACHE].toNanosec() << endl;
   if (asynchronous_map.find(DIRECTORY) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, DIRECTORY))
      os << padding + "    Directory (in nanoseconds): " << asynchronous_map[DIRECTORY].toNanosec() << endl;
   if (asynchronous_map.find(NETWORK_USER) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, NETWORK_USER))
      os << padding + "    User Nework (in nanoseconds): " << asynchronous_map[NETWORK_USER].toNanosec() << endl;
   if (asynchronous_map.find(NETWORK_MEMORY) != asynchronous_map.end() &&
         !hasSameDVFSDomain(module, NETWORK_MEMORY))
      os << padding + "    Memory Nework (in nanoseconds): " << asynchronous_map[NETWORK_MEMORY].toNanosec() << endl;

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

