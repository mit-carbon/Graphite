/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#include "mcpat_cache_interface.h"
#include "simulator.h"
#include "dvfs_manager.h"

//---------------------------------------------------------------------------
// McPAT Cache Interface Constructor
//---------------------------------------------------------------------------
McPATCacheInterface::McPATCacheInterface(Cache* cache)
   : _cache(cache)
   , _last_energy_compute_time(Time(0))
{
   UInt32 technology_node = 0;
   UInt32 temperature = 0;
   try
   {
      technology_node = Sim()->getCfg()->getInt("general/technology_node");
      temperature = Sim()->getCfg()->getInt("general/temperature");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [general/technology_node] or [general/temperature] from the cfg file");
   }

   // Make a ParseXML Object and Initialize it
   _xml = new McPAT::ParseXML();

   // Initialize ParseXML Params and Stats
   _xml->initialize();

   // Fill the ParseXML's Core Params from McPATCacheInterface
   fillCacheParamsIntoXML(technology_node, temperature);

   // Create the cache wrappers
   const DVFSManager::DVFSLevels& dvfs_levels = DVFSManager::getDVFSLevels();
   for (DVFSManager::DVFSLevels::const_iterator it = dvfs_levels.begin(); it != dvfs_levels.end(); it++)
   {
      double current_voltage = (*it).first;
      double current_frequency = (*it).second;
      // Create core wrapper (and) save for future use
      _cache_wrapper_map[current_voltage] = createCacheWrapper(current_voltage, current_frequency);
   }
   
   // Initialize current cache wrapper
   _cache_wrapper = _cache_wrapper_map[_cache->_voltage];

   // Initialize event counters
   initializeEventCounters();

   // Initialize output data structure
   initializeOutputDataStructure();
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCacheInterface::~McPATCacheInterface()
{
   for (CacheWrapperMap::iterator it = _cache_wrapper_map.begin(); it != _cache_wrapper_map.end(); it++)
      delete (*it).second;
   delete _xml;
}

//---------------------------------------------------------------------------
// Create cache wrapper
//---------------------------------------------------------------------------
McPAT::CacheWrapper* McPATCacheInterface::createCacheWrapper(double voltage, double max_frequency_at_voltage)
{
   // Set frequency and voltage in XML object
   _xml->sys.L2[0].vdd = voltage;
   // Frequency (in MHz)
   _xml->sys.target_core_clockrate = max_frequency_at_voltage * 1000;
   _xml->sys.L2[0].clockrate = max_frequency_at_voltage * 1000;

   // Create McPAT cache object
   return new McPAT::CacheWrapper(_xml);
}

//---------------------------------------------------------------------------
// setDVFS (change voltage and frequency)
//---------------------------------------------------------------------------
void McPATCacheInterface::setDVFS(double old_frequency, double new_voltage, double new_frequency, const Time& curr_time)
{
   // Compute leakage/dynamic energy for the previous interval of time
   computeEnergy(curr_time, old_frequency);
   
   // Check if a McPATInterface object has already been created
   _cache_wrapper = _cache_wrapper_map[new_voltage];
   LOG_ASSERT_ERROR(_cache_wrapper, "McPAT cache power model with Voltage(%g) has NOT been created", new_voltage);
}

//---------------------------------------------------------------------------
// Compute Energy from McPAT
//---------------------------------------------------------------------------
void McPATCacheInterface::computeEnergy(const Time& curr_time, double frequency)
{
   Time energy_compute_time = curr_time;
   if (energy_compute_time < _last_energy_compute_time)
      energy_compute_time = _last_energy_compute_time;

   Time time_interval = energy_compute_time - _last_energy_compute_time;
   UInt64 interval_cycles = time_interval.toCycles(frequency);

   // Fill the ParseXML's Core Event Stats from McPATCacheInterface
   fillCacheStatsIntoXML(interval_cycles);

   // Compute Energy from Processor
   _cache_wrapper->computeEnergy();

   // Update the output data structure
   updateOutputDataStructure(time_interval.toSec());

   // Set _last_energy_compute_time to energy_compute_time
   _last_energy_compute_time = energy_compute_time;
}

//---------------------------------------------------------------------------
// Initialize the Output Data Structure
// --------------------------------------------------------------------------
void McPATCacheInterface::initializeOutputDataStructure()
{
   _mcpat_cache_out.area           = 0;
   _mcpat_cache_out.leakage_energy = 0;
   _mcpat_cache_out.dynamic_energy = 0;
}

//---------------------------------------------------------------------------
// Update the Output Data Structure
// --------------------------------------------------------------------------
void McPATCacheInterface::updateOutputDataStructure(double time_interval)
{
   // Is long channel device?
   bool long_channel = _xml->sys.longer_channel_device;
   
   // Store Energy into Data Structure
   double leakage_power = _cache_wrapper->cache->power.readOp.gate_leakage +
                          (long_channel ? _cache_wrapper->cache->power.readOp.longer_channel_leakage
                           : _cache_wrapper->cache->power.readOp.leakage);
   _mcpat_cache_out.area            = _cache_wrapper->cache->area.get_area() * 1e-6;
   _mcpat_cache_out.leakage_energy += (leakage_power * time_interval);
   _mcpat_cache_out.dynamic_energy += _cache_wrapper->cache->rt_power.readOp.dynamic;
}

//---------------------------------------------------------------------------
// Collect Energy from McPAT
//---------------------------------------------------------------------------
double McPATCacheInterface::getDynamicEnergy()
{
   return _mcpat_cache_out.dynamic_energy;
}

double McPATCacheInterface::getLeakageEnergy()
{
   return _mcpat_cache_out.leakage_energy;
}

//---------------------------------------------------------------------------
// Output Summary
//---------------------------------------------------------------------------
void McPATCacheInterface::outputSummary(ostream& os, const Time& target_completion_time, double frequency)
{
   // Compute leakage/dynamic energy for last time interval
   computeEnergy(target_completion_time, frequency);
   displayEnergy(os, target_completion_time);
}

//---------------------------------------------------------------------------
// Display Energy from McPAT
//---------------------------------------------------------------------------
void McPATCacheInterface::displayEnergy(ostream& os, const Time& target_completion_time)
{
   // Convert the completion time into secs
   double target_completion_sec = target_completion_time.toSec();

   string indent4(4, ' ');
   string indent6(6, ' ');
   os << indent4 << "Area and Power Model Statistics: " << endl;
   os << indent6 << "Area (in mm^2): "                << _mcpat_cache_out.area << endl;
   os << indent6 << "Average Static Power (in W): "  << _mcpat_cache_out.leakage_energy / target_completion_sec << endl;
   os << indent6 << "Average Dynamic Power (in W): " << _mcpat_cache_out.dynamic_energy / target_completion_sec << endl;
   os << indent6 << "Total Static Energy (in J): "   << _mcpat_cache_out.leakage_energy << endl;
   os << indent6 << "Total Dynamic Energy (in J): "  << _mcpat_cache_out.dynamic_energy << endl;
}

//---------------------------------------------------------------------------
// Fill Cache Params into XML Structure
//---------------------------------------------------------------------------
void McPATCacheInterface::fillCacheParamsIntoXML(UInt32 technology_node, UInt32 temperature)
{
   // System parameters
   _xml->sys.number_of_cores = 0;
   _xml->sys.number_of_L1Directories = 0;
   _xml->sys.number_of_L2Directories = 0;
   _xml->sys.number_of_L2s = 1;
   _xml->sys.number_of_L3s = 0;
   _xml->sys.number_of_NoCs = 0;
   _xml->sys.homogeneous_cores = 1;
   _xml->sys.homogeneous_L2s = 1;
   _xml->sys.homogeneous_L1Directories = 1;
   _xml->sys.homogeneous_L2Directories = 1;
   _xml->sys.homogeneous_L3s = 1;
   _xml->sys.homogeneous_ccs = 1;
   _xml->sys.homogeneous_NoCs = 1;
   _xml->sys.core_tech_node = technology_node;
   _xml->sys.temperature = temperature;                        // In Kelvin (K)
   _xml->sys.number_cache_levels = 2;
   _xml->sys.interconnect_projection_type = 0;
   _xml->sys.device_type = 0;                                  // 0 - HP (High Performance), 1 - LSTP (Low Standby Power)
   _xml->sys.longer_channel_device = 1;
   _xml->sys.machine_bits = 64; 
   _xml->sys.virtual_address_width = 64;
   _xml->sys.physical_address_width = 52;
   _xml->sys.virtual_memory_page_size = 4096;

   _xml->sys.L2[0].ports[0] = 1;                               // Number of read ports
   _xml->sys.L2[0].ports[1] = 1;                               // Number of write ports
   _xml->sys.L2[0].ports[2] = 1;                               // Number of read/write ports
   _xml->sys.L2[0].device_type = 0;                            // 0 - HP (High Performance), 1 - LSTP (Low Standby Power)

   _xml->sys.L2[0].L2_config[0] = _cache->_cache_size;         // Cache size (in bytes)
   _xml->sys.L2[0].L2_config[1] = _cache->_line_size;          // Cache line size (in bytes)
   _xml->sys.L2[0].L2_config[2] = _cache->_associativity;      // Cache associativity
   _xml->sys.L2[0].L2_config[3] = _cache->_num_banks;          // Number of banks
   _xml->sys.L2[0].L2_config[4] = 1;                           // Throughput = 1 access per cycle
   _xml->sys.L2[0].L2_config[5] = _cache->_perf_model->getLatency(CachePerfModel::ACCESS_DATA_AND_TAGS).toCycles(_cache->_frequency);  // Cache access latency
   _xml->sys.L2[0].L2_config[6] = _cache->_line_size;          // Output width
   _xml->sys.L2[0].L2_config[7] = 1;                           // Cache policy (initialized from Niagara1.xml)

   _xml->sys.L2[0].buffer_sizes[0] = 4;                        // Miss buffer size
   _xml->sys.L2[0].buffer_sizes[1] = 4;                        // Fill buffer size
   _xml->sys.L2[0].buffer_sizes[2] = 4;                        // Prefetch buffer size
   _xml->sys.L2[0].buffer_sizes[3] = 4;                        // Writeback buffer size
   
   _xml->sys.L2[0].conflicts = 0;                              // Initialized from Niagara1.xml
   _xml->sys.L2[0].duty_cycle = 0.5;                           // Initialized from Niagara1.xml
}

//---------------------------------------------------------------------------
// Fill Cache Stats into XML Structure
//---------------------------------------------------------------------------
void McPATCacheInterface::fillCacheStatsIntoXML(UInt64 interval_cycles)
{
   _xml->sys.total_cycles              = interval_cycles;
   _xml->sys.L2[0].read_accesses       = _cache->_total_read_accesses  - _prev_read_accesses;
   _xml->sys.L2[0].write_accesses      = _cache->_total_write_accesses - _prev_write_accesses;
   _xml->sys.L2[0].read_misses         = _cache->_total_read_misses    - _prev_read_misses;
   _xml->sys.L2[0].write_misses        = _cache->_total_write_misses   - _prev_write_misses;
   _xml->sys.L2[0].tag_array_reads     = _cache->_event_counters[Cache::TAG_ARRAY_READ]   - _prev_event_counters[Cache::TAG_ARRAY_READ];
   _xml->sys.L2[0].tag_array_writes    = _cache->_event_counters[Cache::TAG_ARRAY_WRITE]  - _prev_event_counters[Cache::TAG_ARRAY_WRITE];
   _xml->sys.L2[0].data_array_reads    = _cache->_event_counters[Cache::DATA_ARRAY_READ]  - _prev_event_counters[Cache::DATA_ARRAY_READ];
   _xml->sys.L2[0].data_array_writes   = _cache->_event_counters[Cache::DATA_ARRAY_WRITE] - _prev_event_counters[Cache::DATA_ARRAY_WRITE];

   // Update the prev counters
   _prev_read_accesses  = _cache->_total_read_accesses;
   _prev_write_accesses = _cache->_total_write_accesses;
   _prev_read_misses    = _cache->_total_read_misses;
   _prev_write_misses   = _cache->_total_write_misses;
   for (UInt32 i = 0; i < Cache::NUM_OPERATION_TYPES; i++)
      _prev_event_counters[i] = _cache->_event_counters[i];
}

//---------------------------------------------------------------------------
// Initialize Event Counters
//---------------------------------------------------------------------------
void McPATCacheInterface::initializeEventCounters()
{
   _prev_read_accesses = 0;
   _prev_write_accesses = 0;
   _prev_read_misses = 0;
   _prev_write_misses = 0;
   for (UInt32 i = 0; i < Cache::NUM_OPERATION_TYPES; i++)
      _prev_event_counters[i] = 0;   
}
