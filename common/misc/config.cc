#include "config.h"

#include "network_model.h"
#include "network_types.h"
#include "packet_type.h"
#include "simulator.h"
#include "utils.h"

#include <sstream>
#include "log.h"

#define DEBUG

UInt32 Config::m_knob_total_tiles;
UInt32 Config::m_knob_num_process;
bool Config::m_knob_simarch_has_shared_mem;
std::string Config::m_knob_output_file;
bool Config::m_knob_enable_core_modeling;
bool Config::m_knob_enable_power_modeling;
bool Config::m_knob_enable_area_modeling;
UInt32 Config::m_knob_max_threads_per_core;

using namespace std;

Config *Config::m_singleton;

Config *Config::getSingleton()
{
   assert(m_singleton != NULL);
   return m_singleton;
}

Config::Config()
      : m_current_process_num((UInt32)-1)
{
   // NOTE: We can NOT use logging in the config constructor! The log
   // has not been instantiated at this point!
   try
   {
      m_knob_total_tiles = Sim()->getCfg()->getInt("general/total_cores");
      m_knob_num_process = Sim()->getCfg()->getInt("general/num_processes");
      m_knob_simarch_has_shared_mem = Sim()->getCfg()->getBool("general/enable_shared_mem");
      m_knob_output_file = Sim()->getCfg()->getString("general/output_file");
      m_knob_enable_core_modeling = Sim()->getCfg()->getBool("general/enable_core_modeling");
      m_knob_enable_power_modeling = Sim()->getCfg()->getBool("general/enable_power_modeling");
      m_knob_enable_area_modeling = Sim()->getCfg()->getBool("general/enable_area_modeling");
      // WARNING: Do not change this parameter. Hard-coded until multi-threading bug is fixed
      m_knob_max_threads_per_core = 1; // Sim()->getCfg()->getInt("general/max_threads_per_core");

      // Simulation Mode
      m_simulation_mode = parseSimulationMode(Sim()->getCfg()->getString("general/mode"));
   }
   catch(...)
   {
      fprintf(stderr, "ERROR: Config obtained a bad value from config.\n");
      exit(EXIT_FAILURE);
   }

   m_num_processes = m_knob_num_process;
   m_total_tiles = m_knob_total_tiles;
   m_application_tiles = m_total_tiles;
   m_max_threads_per_core = m_knob_max_threads_per_core;

   m_num_cores_per_tile = 1;

   if ((m_simulation_mode == LITE) && (m_num_processes > 1))
   {
      fprintf(stderr, "ERROR: Use only 1 process in lite mode\n");
      exit(EXIT_FAILURE);
   }

   m_singleton = this;

   assert(m_num_processes > 0);
   assert(m_total_tiles > 0);

   // Add one for the MCP
   m_total_tiles += 1;

   // Add the thread-spawners (one for each process)
   if (m_simulation_mode == FULL)
      m_total_tiles += m_num_processes;

   // Parse network parameters - Need to be done here to initialize the network models 
   parseNetworkParameters();

   // Check if any of the on-chip networks have restrictions on the tile_count
   // If those restrictions are not met, quit
   if (!isTileCountPermissible(m_application_tiles))
      exit(-1);

   // Parse tile parameters
   parseTileParameters();

   // Compute Tile ID length in bits
   m_tile_id_length = computeTileIDLength(m_application_tiles);

   GenerateTileMap();
}

Config::~Config()
{
   // Clean up the dynamic memory we allocated
   delete [] m_proc_to_tile_list_map;
   delete [] m_proc_to_application_tile_list_map;
}

UInt32 Config::getTotalTiles()
{
   return m_total_tiles;
}

UInt32 Config::getApplicationTiles()
{
   return m_application_tiles;
}

bool Config::isApplicationTile(tile_id_t tile_id)
{
   return ((tile_id >= 0) && (tile_id < (tile_id_t) getApplicationTiles()));
}

tile_id_t Config::getThreadSpawnerTileNum(UInt32 proc_num)
{
   if (m_simulation_mode == FULL)
      return (getTotalTiles() - (1 + getProcessCount() - proc_num));
   else
      return INVALID_TILE_ID;
}

core_id_t Config::getThreadSpawnerCoreId(UInt32 proc_num)
{
   return (core_id_t) {getThreadSpawnerTileNum(proc_num), MAIN_CORE_TYPE};
}

tile_id_t Config::getCurrentThreadSpawnerTileNum()
{
   if (m_simulation_mode == FULL)
      return (getTotalTiles() - (1 + getProcessCount() - getCurrentProcessNum()));
   else
      return INVALID_TILE_ID;
}

core_id_t Config::getCurrentThreadSpawnerCoreId()
{
   return (core_id_t) {getCurrentThreadSpawnerTileNum(), MAIN_CORE_TYPE};
}

UInt32 Config::computeTileIDLength(UInt32 application_tile_count)
{
   return ceilLog2(application_tile_count);
}

void Config::GenerateTileMap()
{
   vector<TileList> process_to_tile_mapping = computeProcessToTileMapping();
   
   m_proc_to_tile_list_map = new TileList[m_num_processes];
   m_proc_to_application_tile_list_map = new TileList[m_num_processes];

   m_tile_to_proc_map.resize(m_total_tiles);

   // Populate the data structures for non-thread-spawner and non-MCP tiles
   assert(process_to_tile_mapping.size() == m_num_processes);
   for (UInt32 i = 0; i < m_num_processes; i++)
   {
      TileList::iterator tile_it;
      for (tile_it = process_to_tile_mapping[i].begin(); tile_it != process_to_tile_mapping[i].end(); tile_it++)
      {
         assert((*tile_it) < (SInt32) m_application_tiles);
         m_tile_to_proc_map[*tile_it] = i;
         m_proc_to_tile_list_map[i].push_back(*tile_it);
         m_proc_to_application_tile_list_map[i].push_back(*tile_it);
      }
   }
 
   if (m_simulation_mode == FULL)
   {
      // Assign the thread-spawners to tiles
      // Thread-spawners occupy tile-id's (m_application_tiles) to (m_total_tiles - 2)
      UInt32 current_proc = 0;
      for (UInt32 i = m_application_tiles; i < (m_total_tiles - 1); i++)
      {
         assert((current_proc >= 0) && (current_proc < m_num_processes));
         m_tile_to_proc_map[i] = current_proc;
         m_proc_to_tile_list_map[current_proc].push_back(i);
         current_proc++;
      }
   }
   
   // Add one for the MCP
   m_proc_to_tile_list_map[0].push_back(m_total_tiles - 1);
   m_tile_to_proc_map[m_total_tiles - 1] = 0;

   // printProcessToTileMapping();
}

vector<Config::TileList>
Config::computeProcessToTileMapping()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      UInt32 network_model = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(i));
      pair<bool,vector<TileList> > process_to_tile_mapping_struct = NetworkModel::computeProcessToTileMapping(network_model);
      if (process_to_tile_mapping_struct.first)
      {
         switch(network_model)
         {
         case NETWORK_EMESH_HOP_BY_HOP:
         case NETWORK_ATAC:
            return process_to_tile_mapping_struct.second;

         default:
            fprintf(stderr, "Unrecognized Network Type(%u)\n", network_model);
            exit(EXIT_FAILURE);
         }
      }
   }
   
   vector<TileList> process_to_tile_mapping(m_num_processes);
   UInt32 current_proc = 0;
   for (UInt32 i = 0; i < m_application_tiles; i++)
   {
      process_to_tile_mapping[current_proc].push_back(i);
      current_proc = (current_proc + 1) % m_num_processes;
   }
   return process_to_tile_mapping;
}

void Config::printProcessToTileMapping()
{
   UInt32 curr_process_num = atoi(getenv("CARBON_PROCESS_INDEX"));
   if (curr_process_num == 0)
   {
      for (UInt32 i = 0; i < m_num_processes; i++)
      {
         fprintf(stderr, "\nProcess(%u): %u\n", i, (UInt32) m_proc_to_tile_list_map[i].size());
         for (TileList::iterator tile_it = m_proc_to_tile_list_map[i].begin(); \
               tile_it != m_proc_to_tile_list_map[i].end(); tile_it++)
         {
            fprintf(stderr, "%i, ", *tile_it);
         }
         fprintf(stderr, "\n\n");
      }
   }
   exit(-1);
}

void Config::logTileMap()
{
   // Log the map we just created
   LOG_PRINT("Process num: %d\n", m_num_processes);
   for (UInt32 i=0; i < m_num_processes; i++)
   {
      LOG_ASSERT_ERROR(!m_proc_to_tile_list_map[i].empty(),
                       "Process %u assigned zero tiles.", i);

      stringstream ss;
      ss << "Process " << i << ": (" << m_proc_to_tile_list_map[i].size() << ") ";
      for (TLCI m = m_proc_to_tile_list_map[i].begin(); m != m_proc_to_tile_list_map[i].end(); m++)
         ss << "[" << *m << "]";
      LOG_PRINT(ss.str().c_str());
   }
}

SInt32 Config::getIndexFromTileID(UInt32 proc_num, tile_id_t tile_id)
{ 
   TileList tile_list = getTileListForProcess(proc_num);
   for (UInt32 i = 0; i < tile_list.size(); i++)
   {
      if (tile_list[i] == tile_id)
         return (SInt32) i;
   }
   return -1;
}

tile_id_t Config::getTileIDFromIndex(UInt32 proc_num, SInt32 index)
{
   TileList tile_list = getTileListForProcess(proc_num);
   if (index < ((SInt32) tile_list.size()))
   {
      return tile_list[index];
   }
   else
   {
      return -1;
   }
}

core_id_t Config::getMainCoreIDFromIndex(UInt32 proc_num, SInt32 index)
{
   return (core_id_t) {getTileIDFromIndex(proc_num, index), MAIN_CORE_TYPE};
}

// Parse XML config file and use it to fill in config state.  Only modifies
// fields specified in the config file.  Therefore, this method can be used
// to override only the specific options given in the file.
void Config::loadFromFile(char* filename)
{
   return;
}

// Fill in config state from command-line arguments.  Only modifies fields
// specified on the command line.  Therefore, this method can be used to
// override only the specific options given.
void Config::loadFromCmdLine()
{
   return;
}

bool Config::isSimulatingSharedMemory() const
{
   return (bool)m_knob_simarch_has_shared_mem;
}

bool Config::getEnableCoreModeling() const
{
   return (bool)m_knob_enable_core_modeling;
}

bool Config::getEnablePowerModeling() const
{
   return (bool)m_knob_enable_power_modeling;
}

bool Config::getEnableAreaModeling() const
{
   return (bool)m_knob_enable_area_modeling;
}

std::string Config::getOutputFileName() const
{
   return formatOutputFileName(m_knob_output_file);
}

std::string Config::formatOutputFileName(string filename) const
{
   return Sim()->getCfg()->getString("general/output_dir",".") + "/" + filename;
}

void Config::updateCommToTileMap(UInt32 comm_id, tile_id_t tile_id)
{
   m_comm_to_tile_map[comm_id] = tile_id;
}

UInt32 Config::getTileFromCommId(UInt32 comm_id)
{
   CommToTileMap::iterator it = m_comm_to_tile_map.find(comm_id);
   return it == m_comm_to_tile_map.end() ? INVALID_TILE_ID : it->second;
}

Config::SimulationMode Config::parseSimulationMode(string mode)
{
   if (mode == "full")
      return FULL;
   else if (mode == "lite")
      return LITE;
   else
   {
      fprintf(stderr, "Unrecognized Simulation Mode(%s)\n", mode.c_str());
      exit(EXIT_FAILURE);
   }
}

void Config::parseTileParameters()
{
   // Default values are as follows:
   // 1) Number of tiles -> Number of application tiles
   // 2) Core Type -> simple
   // 3) L1-I Cache Type -> T1
   // 4) L1-D Cache Type -> T1
   // 5) L2 Cache Type -> T1

   const UInt32 DEFAULT_NUM_TILES = getApplicationTiles();
   const string DEFAULT_CORE_TYPE = "simple";
   const string DEFAULT_CACHE_TYPE = "T1";

   string tile_parameter_tuple_str;
   vector<string> tile_parameter_tuple_vec;
   try
   {
      tile_parameter_tuple_str = Sim()->getCfg()->getString("tile/model_list");
   }
   catch(...)
   {
      fprintf(stderr, "ERROR: Could not read [tile/model_list] from the cfg file\n");
      exit(EXIT_FAILURE);
   }

   UInt32 num_initialized_tiles = 0;

   parseList(tile_parameter_tuple_str, tile_parameter_tuple_vec, "<>");
   
   for (vector<string>::iterator tuple_it = tile_parameter_tuple_vec.begin();
         tuple_it != tile_parameter_tuple_vec.end(); tuple_it++)
   {
      // Initializing using default values
      UInt32 num_tiles = DEFAULT_NUM_TILES;
      string core_type = DEFAULT_CORE_TYPE;
      string l1_icache_type = DEFAULT_CACHE_TYPE;
      string l1_dcache_type = DEFAULT_CACHE_TYPE;
      string l2_cache_type = DEFAULT_CACHE_TYPE;

      vector<string> tile_parameter_tuple;
      parseList(*tuple_it, tile_parameter_tuple, ",");
     
      SInt32 param_num = 0; 
      for (vector<string>::iterator param_it = tile_parameter_tuple.begin();
            param_it != tile_parameter_tuple.end(); param_it ++)
      {
         if (*param_it != "default")
         {
            switch (param_num)
            {
            case 0:
               num_tiles = convertFromString<UInt32>(*param_it);
               break;

            case 1:
               core_type = trimSpaces(*param_it);
               break;

            case 2:
               l1_icache_type = trimSpaces(*param_it);
               break;

            case 3:
               l1_dcache_type = trimSpaces(*param_it);
               break;

            case 4:
               l2_cache_type = trimSpaces(*param_it);
               break;

            default:
               fprintf(stderr, "Tuple encountered with (%i) parameters\n", param_num);
               exit(EXIT_FAILURE);
               break;
            }
         }
         param_num ++;
      }

      // Append these values to an internal list
      for (UInt32 i = num_initialized_tiles; i < num_initialized_tiles + num_tiles; i++)
      {
         m_tile_parameters_vec.push_back(TileParameters(core_type, l1_icache_type, l1_dcache_type, l2_cache_type));
      }
      num_initialized_tiles += num_tiles;

      if (num_initialized_tiles > getApplicationTiles())
      {
         fprintf(stderr, "num initialized tiles(%u), num application tiles(%u)\n",
            num_initialized_tiles, getApplicationTiles());
         exit(EXIT_FAILURE);
      }
   }
   
   if (num_initialized_tiles != getApplicationTiles())
   {
      fprintf(stderr, "num initialized tiles(%u), num application tiles(%u)\n",
         num_initialized_tiles, getApplicationTiles());
      exit(EXIT_FAILURE);
   }

   // MCP and Thread Spawner cores
   for (UInt32 i = getApplicationTiles(); i < getTotalTiles(); i++)
   {
      m_tile_parameters_vec.push_back(TileParameters(DEFAULT_CORE_TYPE,
               DEFAULT_CACHE_TYPE, DEFAULT_CACHE_TYPE, DEFAULT_CACHE_TYPE));
   }
}

void Config::parseNetworkParameters()
{
   const string DEFAULT_NETWORK_TYPE = "magic";

   string network_parameters_list[NUM_STATIC_NETWORKS];
   try
   {
      config::Config *cfg = Sim()->getCfg();
      network_parameters_list[STATIC_NETWORK_USER] = cfg->getString("network/user");
      network_parameters_list[STATIC_NETWORK_MEMORY] = cfg->getString("network/memory");
      network_parameters_list[STATIC_NETWORK_SYSTEM] = DEFAULT_NETWORK_TYPE;
      network_parameters_list[STATIC_NETWORK_DVFS] = DEFAULT_NETWORK_TYPE;
   }
   catch (...)
   {
      fprintf(stderr, "ERROR: Unable to read network parameters from the cfg file\n");
      exit(EXIT_FAILURE);
   }

   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_parameters_vec.push_back(NetworkParameters(network_parameters_list[i]));
   }
}

string Config::getCoreType(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR(tile_id < ((SInt32) getTotalTiles()),
         "tile_id(%i), total tiles(%u)", tile_id, getTotalTiles());
   LOG_ASSERT_ERROR(m_tile_parameters_vec.size() == getTotalTiles(),
         "m_tile_parameters_vec.size(%u), total tiles(%u)",
         m_tile_parameters_vec.size(), getTotalTiles());

   return m_tile_parameters_vec[tile_id].getCoreType();
}

string Config::getL1ICacheType(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR(tile_id < ((SInt32) getTotalTiles()),
         "tile_id(%i), total tiles(%u)", tile_id, getTotalTiles());
   LOG_ASSERT_ERROR(m_tile_parameters_vec.size() == getTotalTiles(),
         "m_tile_parameters_vec.size(%u), total tiles(%u)",
         m_tile_parameters_vec.size(), getTotalTiles());

   return m_tile_parameters_vec[tile_id].getL1ICacheType();
}

string Config::getL1DCacheType(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR(tile_id < ((SInt32) getTotalTiles()),
         "tile_id(%i), total tiles(%u)", tile_id, getTotalTiles());
   LOG_ASSERT_ERROR(m_tile_parameters_vec.size() == getTotalTiles(),
         "m_tile_parameters_vec.size(%u), total tiles(%u)",
         m_tile_parameters_vec.size(), getTotalTiles());

   return m_tile_parameters_vec[tile_id].getL1DCacheType();
}

string Config::getL2CacheType(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR(tile_id < ((SInt32) getTotalTiles()),
         "tile_id(%i), total tiles(%u)", tile_id, getTotalTiles());
   LOG_ASSERT_ERROR(m_tile_parameters_vec.size() == getTotalTiles(),
         "m_tile_parameters_vec.size(%u), total tiles(%u)",
         m_tile_parameters_vec.size(), getTotalTiles());

   return m_tile_parameters_vec[tile_id].getL2CacheType();
}

string Config::getNetworkType(SInt32 network_id)
{
   LOG_ASSERT_ERROR(m_network_parameters_vec.size() == NUM_STATIC_NETWORKS,
         "m_network_parameters_vec.size(%u), NUM_STATIC_NETWORKS(%u)",
         m_network_parameters_vec.size(), NUM_STATIC_NETWORKS);

   return m_network_parameters_vec[network_id].getType();
}

bool Config::isTileCountPermissible(UInt32 tile_count)
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      UInt32 network_model = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(i));
      bool permissible = NetworkModel::isTileCountPermissible(network_model, (SInt32) tile_count);
      if (!permissible)
      {
         fprintf(stderr, "ERROR: Problem using the network models specified in the cfg file\n");
         return false;
      }
   }
   return true;
}
