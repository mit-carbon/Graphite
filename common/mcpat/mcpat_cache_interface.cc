/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#include "mcpat_cache_interface.h"

//---------------------------------------------------------------------------
// McPAT Cache Interface Constructor
//---------------------------------------------------------------------------
McPATCacheInterface::McPATCacheInterface(Cache* cache, UInt32 technology_node)
{
   // Make a ParseXML Object and Initialize it
   _xml = new McPAT::ParseXML();

   // Initialize ParseXML Params and Stats
   _xml->initialize();

   // Fill the ParseXML's Core Params from McPATCacheInterface
   fillCacheParamsIntoXML(cache, technology_node);

   // Make a Processor Object from the ParseXML
   _cache_wrapper = new McPAT::CacheWrapper(_xml);
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCacheInterface::~McPATCacheInterface()
{
   delete _cache_wrapper;
   delete _xml;
}

//---------------------------------------------------------------------------
// Compute Energy from McPAT
//---------------------------------------------------------------------------
void McPATCacheInterface::computeEnergy(Cache* cache)
{
   // Fill the ParseXML's Core Event Stats from McPATCacheInterface
   fillCacheStatsIntoXML(cache);

   // Compute Energy from Processor
   _cache_wrapper->computeEnergy();

   // Store Energy into Data Structure
   // Core
   _mcpat_cache_out.area               = _cache_wrapper->cache->area.get_area() * 1e-6;
	bool long_channel                   = _xml->sys.longer_channel_device;
   double subthreshold_leakage_power   = long_channel ? _cache_wrapper->cache->power.readOp.longer_channel_leakage : _cache_wrapper->cache->power.readOp.leakage;
   double gate_leakage_power           = _cache_wrapper->cache->power.readOp.gate_leakage;
   _mcpat_cache_out.leakage_power      = subthreshold_leakage_power + gate_leakage_power;
   _mcpat_cache_out.dynamic_energy     = _cache_wrapper->cache->rt_power.readOp.dynamic;
}

//---------------------------------------------------------------------------
// Display Energy from McPAT
//---------------------------------------------------------------------------
void McPATCacheInterface::outputSummary(ostream& os)
{
   string indent4(4, ' ');
   os << indent4 << "Area (in mm^2): "         << _mcpat_cache_out.area << endl;
   os << indent4 << "Leakage Power (in W): "   << _mcpat_cache_out.leakage_power << endl;
   os << indent4 << "Dynamic Energy (in J): "  << _mcpat_cache_out.dynamic_energy << endl;
}

void McPATCacheInterface::fillCacheParamsIntoXML(Cache* cache, UInt32 technology_node)
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
   _xml->sys.target_core_clockrate = cache->_frequency * 1000;
   _xml->sys.temperature = 380;
   _xml->sys.number_cache_levels = 2;
   _xml->sys.interconnect_projection_type = 0;
   _xml->sys.device_type = 0;    // HP (High Performance)
   _xml->sys.longer_channel_device = 1;
   _xml->sys.machine_bits = 64; 
   _xml->sys.virtual_address_width = 64;
   _xml->sys.physical_address_width = 52;
   _xml->sys.virtual_memory_page_size = 4096;
   _xml->sys.total_cycles = 100000;

   _xml->sys.L2[0].clockrate = cache->_frequency * 1000;       // Frequency (in MHz)
   _xml->sys.L2[0].ports[0] = 1;                               // Number of read ports
   _xml->sys.L2[0].ports[1] = 1;                               // Number of write ports
   _xml->sys.L2[0].ports[2] = 1;                               // Number of read/write ports
   _xml->sys.L2[0].device_type = 0;                            // Device type (High performance)

   _xml->sys.L2[0].L2_config[0] = cache->_cache_size;          // Cache size (in bytes)
   _xml->sys.L2[0].L2_config[1] = cache->_line_size;           // Cache line size (in bytes)
   _xml->sys.L2[0].L2_config[2] = cache->_associativity;       // Cache associativity
   _xml->sys.L2[0].L2_config[3] = 1;                           // Number of banks = 1
   _xml->sys.L2[0].L2_config[4] = 1;                           // Throughput = 1 access per cycle
   _xml->sys.L2[0].L2_config[5] = cache->_access_delay;        // Cache access latency
   _xml->sys.L2[0].L2_config[6] = cache->_line_size;           // Output width
   _xml->sys.L2[0].L2_config[7] = 1;                           // Cache policy (initialized from Niagara1.xml)

   _xml->sys.L2[0].buffer_sizes[0] = 4;                        // Miss buffer size
   _xml->sys.L2[0].buffer_sizes[1] = 4;                        // Fill buffer size
   _xml->sys.L2[0].buffer_sizes[2] = 4;                        // Prefetch buffer size
   _xml->sys.L2[0].buffer_sizes[3] = 4;                        // Writeback buffer size
   
   _xml->sys.L2[0].conflicts = 0;                              // Initialized from Niagara1.xml
   _xml->sys.L2[0].duty_cycle = 0.5;                           // Initialized from Niagara1.xml
}

void McPATCacheInterface::fillCacheStatsIntoXML(Cache* cache)
{
   _xml->sys.L2[0].read_accesses = cache->_total_read_accesses;
   _xml->sys.L2[0].write_accesses = cache->_total_write_accesses;
   _xml->sys.L2[0].read_misses = cache->_total_read_misses;
   _xml->sys.L2[0].write_misses = cache->_total_write_misses;
}
