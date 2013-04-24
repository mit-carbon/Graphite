/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#include "mcpat_core_interface.h"
#include "simulator.h"
#include "dvfs_manager.h"

//---------------------------------------------------------------------------
// McPAT Core Interface Constructor
//---------------------------------------------------------------------------
McPATCoreInterface::McPATCoreInterface(double frequency, double voltage, UInt32 load_buffer_size, UInt32 store_buffer_size)
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

   // Initialize Architectural Parameters
   initializeArchitecturalParameters(load_buffer_size, store_buffer_size);
   // Initialize Event Counters
   initializeEventCounters();

   // Initialize Output Data Structure
   initializeOutputDataStructure();

   _enable_area_and_power_modeling = Config::getSingleton()->getEnableAreaModeling() || Config::getSingleton()->getEnablePowerModeling();
   if (_enable_area_and_power_modeling)
   {
      // Make a ParseXML Object and Initialize it
      _xml = new McPAT::ParseXML();

      // Initialize ParseXML Params and Stats
      _xml->initialize();
      _xml->setNiagara1();

      // Fill the ParseXML's Core Params from McPATCoreInterface
      fillCoreParamsIntoXML(technology_node, temperature);

      // Calculate max frequency at given voltage
      double max_frequency_at_voltage = DVFSManager::getMaxFrequency(voltage);
      // Create cache wrapper
      _core_wrapper = createCoreWrapper(voltage, max_frequency_at_voltage);
      // Save for future use
      _core_wrapper_map[voltage] = _core_wrapper;

      // Initialize Static Power
      computeEnergy();
   }
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCoreInterface::~McPATCoreInterface()
{
   if (_enable_area_and_power_modeling)
   {
      for (CoreWrapperMap::iterator it = _core_wrapper_map.begin(); it != _core_wrapper_map.end(); it++)
         delete (*it).second;
      delete _xml;
   }
}

//---------------------------------------------------------------------------
// Create core wrapper
//---------------------------------------------------------------------------
McPAT::CoreWrapper* McPATCoreInterface::createCoreWrapper(double voltage, double max_frequency_at_voltage)
{
   // Set frequency and voltage in XML object
   _xml->sys.vdd = voltage;
   // Frequency (in MHz)
   _xml->sys.target_core_clockrate = max_frequency_at_voltage * 1000;
   _xml->sys.core[0].clock_rate = max_frequency_at_voltage * 1000;

   // Create McPAT core object
   return new McPAT::CoreWrapper(_xml);
}

//---------------------------------------------------------------------------
// setDVFS (change voltage and frequency)
//---------------------------------------------------------------------------
void McPATCoreInterface::setDVFS(double voltage, double frequency)
{
   if (!_enable_area_and_power_modeling)
      return;

   // Check if a McPATInterface object has already been created
   _core_wrapper = _core_wrapper_map[voltage];
   if (_core_wrapper == NULL)
   {

      // Calculate max frequency at given voltage
      double max_frequency_at_voltage = DVFSManager::getMaxFrequency(voltage);

      _core_wrapper = createCoreWrapper(voltage, max_frequency_at_voltage);

      // Save for future use
      _core_wrapper_map[voltage] = _core_wrapper;
   }
}
//---------------------------------------------------------------------------
// Initialize Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeArchitecturalParameters(UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // System Parameters
   // Architectural Parameters
   // |---- General Parameters
   _instruction_length = 32;
   _opcode_width = 9;
   _machine_type = 1;
   _num_hardware_threads = 1;
   _fetch_width = 1;
   _num_instruction_fetch_ports = 1;
   _decode_width = 1;
   _issue_width = 1;
   _fp_issue_width = 1;
   _commit_width = 1;
   _prediction_width = 1;  // Enable Branch Predictor Table (BPT) and Branch Target Buffer (BTB) within McPAT
   _integer_pipeline_depth = 6;
   _fp_pipeline_depth = 6;
   _ALU_per_core = 1;
   _MUL_per_core = 1;
   _FPU_per_core = 1;
   _instruction_buffer_size = 16;
   _decoded_stream_buffer_size = 16;
   // |---- Register File
   _arch_regs_IRF_size = 24;
   _arch_regs_FRF_size = 24;
   _phy_regs_IRF_size = 24;
   _phy_regs_FRF_size = 24;
   // |---- Load-Store Unit
   _LSU_order = "inorder";
   _load_buffer_size = load_buffer_size;
   _store_buffer_size = store_buffer_size;
   _num_memory_ports = 1;
   _RAS_size = 16;
   // |---- OoO Core
   _instruction_window_scheme = 0;
   _instruction_window_size = 0;
   _fp_instruction_window_size = 0;
   _ROB_size = 0;
   _rename_scheme = 0;
   // |---- Register Windows (specific to Sun processors)
   _register_windows_size = 0;
}

//---------------------------------------------------------------------------
// Initialize Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeEventCounters()
{
   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   _total_instructions = 0;
   _generic_instructions = 0;
   _int_instructions = 0;
   _fp_instructions = 0;
   _branch_instructions = 0;
   _branch_mispredictions = 0;
   _load_instructions = 0;
   _store_instructions = 0;
   _committed_instructions = 0;
   _committed_int_instructions = 0;
   _committed_fp_instructions = 0;
   // |---- Cycle Counters
   _total_cycles = 0;
   _idle_cycles = 0;
   _busy_cycles = 0;
   // |---- Reg File Access Counters
   _int_regfile_reads = 0;
   _int_regfile_writes = 0;
   _fp_regfile_reads = 0;
   _fp_regfile_writes = 0;
   // |---- Execution Unit Access Counters
   _ialu_accesses = 0;
   _mul_accesses = 0;
   _fpu_accesses = 0;
   _cdb_alu_accesses = 0;
   _cdb_mul_accesses = 0;
   _cdb_fpu_accesses = 0;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   _inst_window_reads = 0;
   _inst_window_writes = 0;
   _inst_window_wakeup_accesses = 0;
   _fp_inst_window_reads = 0;
   _fp_inst_window_writes = 0;
   _fp_inst_window_wakeup_accesses = 0;
   _ROB_reads = 0;
   _ROB_writes = 0;
   _rename_accesses = 0;
   _fp_rename_accesses = 0;
   // |---- Function Calls and Context Switches
   _function_calls = 0;
   _context_switches = 0;
}

//---------------------------------------------------------------------------
// Initialize Output Data Structure
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeOutputDataStructure()
{
   // Zero the Energy in Data Structure
   // Core
   _mcpat_core_out.core.area                           = 0;
   _mcpat_core_out.core.leakage                        = 0;
   _mcpat_core_out.core.longer_channel_leakage         = 0;
   _mcpat_core_out.core.gate_leakage                   = 0;
   _mcpat_core_out.core.dynamic                        = 0;
   //    Instruction Fetch Unit
   _mcpat_core_out.ifu.ifu.area                        = 0;
   _mcpat_core_out.ifu.ifu.leakage                     = 0;
   _mcpat_core_out.ifu.ifu.longer_channel_leakage      = 0;
   _mcpat_core_out.ifu.ifu.gate_leakage                = 0;
   _mcpat_core_out.ifu.ifu.dynamic                     = 0;
   //       Instruction Cache
   _mcpat_core_out.ifu.icache.area                     = 0;
   _mcpat_core_out.ifu.icache.leakage                  = 0;
   _mcpat_core_out.ifu.icache.longer_channel_leakage   = 0;
   _mcpat_core_out.ifu.icache.gate_leakage             = 0;
   _mcpat_core_out.ifu.icache.dynamic                  = 0;
   //       Instruction Buffer
   _mcpat_core_out.ifu.IB.area                         = 0;
   _mcpat_core_out.ifu.IB.leakage                      = 0;
   _mcpat_core_out.ifu.IB.longer_channel_leakage       = 0;
   _mcpat_core_out.ifu.IB.gate_leakage                 = 0;
   _mcpat_core_out.ifu.IB.dynamic                      = 0;
   //       Instruction Decoder
   _mcpat_core_out.ifu.ID.area                         = 0;
   _mcpat_core_out.ifu.ID.leakage                      = 0;
   _mcpat_core_out.ifu.ID.longer_channel_leakage       = 0;
   _mcpat_core_out.ifu.ID.gate_leakage                 = 0;
   _mcpat_core_out.ifu.ID.dynamic                      = 0;
   //       Branch Predictor Table
   _mcpat_core_out.ifu.BPT.area                         = 0;
   _mcpat_core_out.ifu.BPT.leakage                      = 0;
   _mcpat_core_out.ifu.BPT.longer_channel_leakage       = 0;
   _mcpat_core_out.ifu.BPT.gate_leakage                 = 0;
   _mcpat_core_out.ifu.BPT.dynamic                      = 0;
   //       Branch Target Buffer
   _mcpat_core_out.ifu.BTB.area                         = 0;
   _mcpat_core_out.ifu.BTB.leakage                      = 0;
   _mcpat_core_out.ifu.BTB.longer_channel_leakage       = 0;
   _mcpat_core_out.ifu.BTB.gate_leakage                 = 0;
   _mcpat_core_out.ifu.BTB.dynamic                      = 0;
   
   //    Load Store Unit
   _mcpat_core_out.lsu.lsu.area                        = 0;
   _mcpat_core_out.lsu.lsu.leakage                     = 0;
   _mcpat_core_out.lsu.lsu.longer_channel_leakage      = 0;
   _mcpat_core_out.lsu.lsu.gate_leakage                = 0;
   _mcpat_core_out.lsu.lsu.dynamic                     = 0;
   //       Data Cache
   _mcpat_core_out.lsu.dcache.area                     = 0;
   _mcpat_core_out.lsu.dcache.leakage                  = 0;
   _mcpat_core_out.lsu.dcache.longer_channel_leakage   = 0;
   _mcpat_core_out.lsu.dcache.gate_leakage             = 0;
   _mcpat_core_out.lsu.dcache.dynamic                  = 0;
   //       Load/Store Queue
   _mcpat_core_out.lsu.LSQ.area                        = 0;
   _mcpat_core_out.lsu.LSQ.leakage                     = 0;
   _mcpat_core_out.lsu.LSQ.longer_channel_leakage      = 0;
   _mcpat_core_out.lsu.LSQ.gate_leakage                = 0;
   _mcpat_core_out.lsu.LSQ.dynamic                     = 0;
   //    Memory Management Unit
   _mcpat_core_out.mmu.mmu.area                        = 0;
   _mcpat_core_out.mmu.mmu.leakage                     = 0;
   _mcpat_core_out.mmu.mmu.longer_channel_leakage      = 0;
   _mcpat_core_out.mmu.mmu.gate_leakage                = 0;
   _mcpat_core_out.mmu.mmu.dynamic                     = 0;
   //       Itlb
   _mcpat_core_out.mmu.itlb.area                       = 0;
   _mcpat_core_out.mmu.itlb.leakage                    = 0;
   _mcpat_core_out.mmu.itlb.longer_channel_leakage     = 0;
   _mcpat_core_out.mmu.itlb.gate_leakage               = 0;
   _mcpat_core_out.mmu.itlb.dynamic                    = 0;
   //       Dtlb
   _mcpat_core_out.mmu.dtlb.area                       = 0;
   _mcpat_core_out.mmu.dtlb.leakage                    = 0;
   _mcpat_core_out.mmu.dtlb.longer_channel_leakage     = 0;
   _mcpat_core_out.mmu.dtlb.gate_leakage               = 0;
   _mcpat_core_out.mmu.dtlb.dynamic                    = 0;
   //    Execution Unit
   _mcpat_core_out.exu.exu.area                        = 0;
   _mcpat_core_out.exu.exu.leakage                     = 0;
   _mcpat_core_out.exu.exu.longer_channel_leakage      = 0;
   _mcpat_core_out.exu.exu.gate_leakage                = 0;
   _mcpat_core_out.exu.exu.dynamic                     = 0;
   //       Register Files
   _mcpat_core_out.exu.rfu.rfu.area                    = 0;
   _mcpat_core_out.exu.rfu.rfu.leakage                 = 0;
   _mcpat_core_out.exu.rfu.rfu.longer_channel_leakage  = 0;
   _mcpat_core_out.exu.rfu.rfu.gate_leakage            = 0;
   _mcpat_core_out.exu.rfu.rfu.dynamic                 = 0;
   //          Integer RF
   _mcpat_core_out.exu.rfu.IRF.area                    = 0;
   _mcpat_core_out.exu.rfu.IRF.leakage                 = 0;
   _mcpat_core_out.exu.rfu.IRF.longer_channel_leakage  = 0;
   _mcpat_core_out.exu.rfu.IRF.gate_leakage            = 0;
   _mcpat_core_out.exu.rfu.IRF.dynamic                 = 0;
   //          Floating Point RF
   _mcpat_core_out.exu.rfu.FRF.area                    = 0;
   _mcpat_core_out.exu.rfu.FRF.leakage                 = 0;
   _mcpat_core_out.exu.rfu.FRF.longer_channel_leakage  = 0;
   _mcpat_core_out.exu.rfu.FRF.gate_leakage            = 0;
   _mcpat_core_out.exu.rfu.FRF.dynamic                 = 0;
   //          Register Windows
   _mcpat_core_out.exu.rfu.RFWIN.area                     = 0;
   _mcpat_core_out.exu.rfu.RFWIN.leakage                  = 0;
   _mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage   = 0;
   _mcpat_core_out.exu.rfu.RFWIN.gate_leakage             = 0;
   _mcpat_core_out.exu.rfu.RFWIN.dynamic                  = 0;
   //       Instruction Scheduler
   _mcpat_core_out.exu.scheu.scheu.area                   = 0;
   _mcpat_core_out.exu.scheu.scheu.leakage                = 0;
   _mcpat_core_out.exu.scheu.scheu.longer_channel_leakage = 0;
   _mcpat_core_out.exu.scheu.scheu.gate_leakage           = 0;
   _mcpat_core_out.exu.scheu.scheu.dynamic                = 0;
   //          Instruction Window
   _mcpat_core_out.exu.scheu.int_inst_window.area                     = 0;
   _mcpat_core_out.exu.scheu.int_inst_window.leakage                  = 0;
   _mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage   = 0;
   _mcpat_core_out.exu.scheu.int_inst_window.gate_leakage             = 0;
   _mcpat_core_out.exu.scheu.int_inst_window.dynamic                  = 0;
   //       Integer ALUs
   _mcpat_core_out.exu.exeu.area                       = 0;
   _mcpat_core_out.exu.exeu.leakage                    = 0;
   _mcpat_core_out.exu.exeu.longer_channel_leakage     = 0;
   _mcpat_core_out.exu.exeu.gate_leakage               = 0;
   _mcpat_core_out.exu.exeu.dynamic                    = 0;
   //       Floating Point Units (FPUs)
   _mcpat_core_out.exu.fp_u.area                       = 0;
   _mcpat_core_out.exu.fp_u.leakage                    = 0;
   _mcpat_core_out.exu.fp_u.longer_channel_leakage     = 0;
   _mcpat_core_out.exu.fp_u.gate_leakage               = 0;
   _mcpat_core_out.exu.fp_u.dynamic                    = 0;
   //       Complex ALUs (Mul/Div)
   _mcpat_core_out.exu.mul.area                        = 0;
   _mcpat_core_out.exu.mul.leakage                     = 0;
   _mcpat_core_out.exu.mul.longer_channel_leakage      = 0;
   _mcpat_core_out.exu.mul.gate_leakage                = 0;
   _mcpat_core_out.exu.mul.dynamic                     = 0;
   //       Results Broadcast Bus
   _mcpat_core_out.exu.bypass.area                     = 0;
   _mcpat_core_out.exu.bypass.leakage                  = 0;
   _mcpat_core_out.exu.bypass.longer_channel_leakage   = 0;
   _mcpat_core_out.exu.bypass.gate_leakage             = 0;
   _mcpat_core_out.exu.bypass.dynamic                  = 0;
}

//---------------------------------------------------------------------------
// Update Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::updateEventCounters(Instruction* instruction, UInt64 cycle_count, UInt64 total_branch_misprediction_count)
{
   // Get Instruction Type
   McPATInstructionType instruction_type = getMcPATInstructionType(instruction->getType());
   if (instruction->getType() != INST_STALL)
      updateInstructionCounters(instruction_type, total_branch_misprediction_count);

   // Execution Unit Accesses
   // A single instruction can access multiple execution units
   // FIXME: Find out whether we need the whole instruction for this purpose
   ExecutionUnitList access_list = getExecutionUnitAccessList(instruction->getType());
   for (UInt32 i = 0; i < access_list.size(); i++)
      updateExecutionUnitAccessCounters(access_list[i]);

   // Count access to multiple execution units as additional micro-ops
   for (UInt32 i = 1; i < access_list.size(); i++)
      updateInstructionCounters(instruction_type, total_branch_misprediction_count);

   // Update Cycle Counters
   updateCycleCounters(cycle_count);

   const OperandList& ops = instruction->getOperands();
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      // Loads/Stores
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::READ))
         updateInstructionCounters(LOAD_INST, total_branch_misprediction_count);
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::WRITE))
         updateInstructionCounters(STORE_INST, total_branch_misprediction_count);

      // Reg File Accesses
      if (o.m_type == Operand::REG)
         updateRegFileAccessCounters(o.m_direction, o.m_value);
   }
}

//---------------------------------------------------------------------------
// Update Instruction Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::updateInstructionCounters(McPATInstructionType instruction_type, UInt64 total_branch_misprediction_count)
{
   _total_instructions ++;
   _committed_instructions ++;
   
   switch (instruction_type)
   {
   case GENERIC_INST:
      _generic_instructions ++;
      break;

   case INTEGER_INST:
      _int_instructions ++;
      _committed_int_instructions ++;
      break;

   case FLOATING_POINT_INST:
      _fp_instructions ++;
      _committed_fp_instructions ++;
      break;

   case LOAD_INST:
      _load_instructions ++;
      break;

   case STORE_INST:
      _store_instructions ++;
      break;

   case BRANCH_INST:
      _branch_instructions ++;
      _branch_mispredictions = total_branch_misprediction_count;
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Instruction Type(%u)", instruction_type);
      break;
   }
}

//---------------------------------------------------------------------------
// Update Reg File Access Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::updateRegFileAccessCounters(Operand::Direction operand_direction, UInt32 reg_id)
{
   if (operand_direction == Operand::READ)
   {
      if (isIntegerReg(reg_id))
         _int_regfile_reads ++;
      else if (isFloatingPointReg(reg_id))
         _fp_regfile_reads ++;
      else if (isXMMReg(reg_id))
         _fp_regfile_reads += 2;
   }
   else if (operand_direction == Operand::WRITE)
   {
      if (isIntegerReg(reg_id))
         _int_regfile_writes ++;
      else if (isFloatingPointReg(reg_id))
         _fp_regfile_writes ++;
      else if (isXMMReg(reg_id))
         _fp_regfile_writes += 2;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Operand Direction(%u)", operand_direction);
   }
}

//---------------------------------------------------------------------------
// Update Execution Unit Access Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::updateExecutionUnitAccessCounters(ExecutionUnitType unit_type)
{
   switch (unit_type)
   {
   case ALU:
      _ialu_accesses ++;
      _cdb_alu_accesses ++;
      break;

   case MUL:
      _mul_accesses ++;
      _cdb_mul_accesses ++;
      break;

   case FPU:
      _fpu_accesses ++;
      _cdb_fpu_accesses ++;
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Execution Unit(%u)", unit_type);
      break;
   }
}

//---------------------------------------------------------------------------
// Update Cycle Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::updateCycleCounters(UInt64 cycle_count)
{
   _total_cycles = cycle_count;
   _busy_cycles  = cycle_count;
   // TODO: Update for idle cycles later
}

//---------------------------------------------------------------------------
// Compute Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::computeEnergy()
{
   // Fill the ParseXML's Core Event Stats from McPATCoreInterface
   fillCoreStatsIntoXML();

   // Compute Energy from Processor
   _core_wrapper->computeEnergy();

   // Store Energy into Data Structure
   // Core
   _mcpat_core_out.core.area                   = _core_wrapper->core->area.get_area()*1e-6;                     
   _mcpat_core_out.core.leakage                = _core_wrapper->core->power.readOp.leakage;                  
   _mcpat_core_out.core.longer_channel_leakage = _core_wrapper->core->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.core.gate_leakage           = _core_wrapper->core->power.readOp.gate_leakage;             
   _mcpat_core_out.core.dynamic                = _core_wrapper->core->rt_power.readOp.dynamic;
   //    Instruction Fetch Unit
   _mcpat_core_out.ifu.ifu.area                   = _core_wrapper->core->ifu->area.get_area()*1e-6;                     
   _mcpat_core_out.ifu.ifu.leakage                = _core_wrapper->core->ifu->power.readOp.leakage;                  
   _mcpat_core_out.ifu.ifu.longer_channel_leakage = _core_wrapper->core->ifu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.ifu.ifu.gate_leakage           = _core_wrapper->core->ifu->power.readOp.gate_leakage;             
   _mcpat_core_out.ifu.ifu.dynamic                = _core_wrapper->core->ifu->rt_power.readOp.dynamic;
   //       Instruction Cache
   _mcpat_core_out.ifu.icache.area                   = _core_wrapper->core->ifu->icache.area.get_area()*1e-6;                     
   _mcpat_core_out.ifu.icache.leakage                = _core_wrapper->core->ifu->icache.power.readOp.leakage;                  
   _mcpat_core_out.ifu.icache.longer_channel_leakage = _core_wrapper->core->ifu->icache.power.readOp.longer_channel_leakage;   
   _mcpat_core_out.ifu.icache.gate_leakage           = _core_wrapper->core->ifu->icache.power.readOp.gate_leakage;             
   _mcpat_core_out.ifu.icache.dynamic                = _core_wrapper->core->ifu->icache.rt_power.readOp.dynamic;
   //       Instruction Buffer
   _mcpat_core_out.ifu.IB.area                       = _core_wrapper->core->ifu->IB->area.get_area()*1e-6;                     
   _mcpat_core_out.ifu.IB.leakage                    = _core_wrapper->core->ifu->IB->power.readOp.leakage;                  
   _mcpat_core_out.ifu.IB.longer_channel_leakage     = _core_wrapper->core->ifu->IB->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.ifu.IB.gate_leakage               = _core_wrapper->core->ifu->IB->power.readOp.gate_leakage;             
   _mcpat_core_out.ifu.IB.dynamic                    = _core_wrapper->core->ifu->IB->rt_power.readOp.dynamic;
   //       Instruction Decoder
   _mcpat_core_out.ifu.ID.area                       = (_core_wrapper->core->ifu->ID_inst->area.get_area() +
                                                        _core_wrapper->core->ifu->ID_operand->area.get_area() +
                                                        _core_wrapper->core->ifu->ID_misc->area.get_area())*
                                                        _core_wrapper->core->ifu->coredynp.decodeW * 1e-6;                     
   _mcpat_core_out.ifu.ID.leakage                    = (_core_wrapper->core->ifu->ID_inst->power.readOp.leakage +
                                                        _core_wrapper->core->ifu->ID_operand->power.readOp.leakage +
                                                        _core_wrapper->core->ifu->ID_misc->power.readOp.leakage);                
   _mcpat_core_out.ifu.ID.longer_channel_leakage     = (_core_wrapper->core->ifu->ID_inst->power.readOp.longer_channel_leakage +
                                                        _core_wrapper->core->ifu->ID_operand->power.readOp.longer_channel_leakage +
                                                        _core_wrapper->core->ifu->ID_misc->power.readOp.longer_channel_leakage);   
   _mcpat_core_out.ifu.ID.gate_leakage               = (_core_wrapper->core->ifu->ID_inst->power.readOp.gate_leakage +
                                                        _core_wrapper->core->ifu->ID_operand->power.readOp.gate_leakage +
                                                        _core_wrapper->core->ifu->ID_misc->power.readOp.gate_leakage);          
   _mcpat_core_out.ifu.ID.dynamic                    = (_core_wrapper->core->ifu->ID_inst->rt_power.readOp.dynamic +
                                                        _core_wrapper->core->ifu->ID_operand->rt_power.readOp.dynamic +
                                                        _core_wrapper->core->ifu->ID_misc->rt_power.readOp.dynamic);
   //       Branch Predictor
   assert(_core_wrapper->core->ifu->coredynp.predictionW > 0);
   _mcpat_core_out.ifu.BPT.area                      = _core_wrapper->core->ifu->BPT->area.get_area() * 1e-6;
   _mcpat_core_out.ifu.BPT.leakage                   = _core_wrapper->core->ifu->BPT->power.readOp.leakage;
   _mcpat_core_out.ifu.BPT.longer_channel_leakage    = _core_wrapper->core->ifu->BPT->power.readOp.longer_channel_leakage;
   _mcpat_core_out.ifu.BPT.gate_leakage              = _core_wrapper->core->ifu->BPT->power.readOp.gate_leakage;
   _mcpat_core_out.ifu.BPT.dynamic                   = _core_wrapper->core->ifu->BPT->rt_power.readOp.dynamic;
   //       Branch Target Buffer
   _mcpat_core_out.ifu.BTB.area                      = _core_wrapper->core->ifu->BTB->area.get_area() * 1e-6;
   _mcpat_core_out.ifu.BTB.leakage                   = _core_wrapper->core->ifu->BTB->power.readOp.leakage;
   _mcpat_core_out.ifu.BTB.longer_channel_leakage    = _core_wrapper->core->ifu->BTB->power.readOp.longer_channel_leakage;
   _mcpat_core_out.ifu.BTB.gate_leakage              = _core_wrapper->core->ifu->BTB->power.readOp.gate_leakage;
   _mcpat_core_out.ifu.BTB.dynamic                   = _core_wrapper->core->ifu->BTB->rt_power.readOp.dynamic;
   
   //    Load Store Unit
   _mcpat_core_out.lsu.lsu.area                   = _core_wrapper->core->lsu->area.get_area()*1e-6;                     
   _mcpat_core_out.lsu.lsu.leakage                = _core_wrapper->core->lsu->power.readOp.leakage;                  
   _mcpat_core_out.lsu.lsu.longer_channel_leakage = _core_wrapper->core->lsu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.lsu.lsu.gate_leakage           = _core_wrapper->core->lsu->power.readOp.gate_leakage;             
   _mcpat_core_out.lsu.lsu.dynamic                = _core_wrapper->core->lsu->rt_power.readOp.dynamic;
   //       Data Cache
   _mcpat_core_out.lsu.dcache.area                   = _core_wrapper->core->lsu->dcache.area.get_area()*1e-6;                     
   _mcpat_core_out.lsu.dcache.leakage                = _core_wrapper->core->lsu->dcache.power.readOp.leakage;                  
   _mcpat_core_out.lsu.dcache.longer_channel_leakage = _core_wrapper->core->lsu->dcache.power.readOp.longer_channel_leakage;   
   _mcpat_core_out.lsu.dcache.gate_leakage           = _core_wrapper->core->lsu->dcache.power.readOp.gate_leakage;             
   _mcpat_core_out.lsu.dcache.dynamic                = _core_wrapper->core->lsu->dcache.rt_power.readOp.dynamic;
   //       Load/Store Queue
   _mcpat_core_out.lsu.LSQ.area                      = _core_wrapper->core->lsu->LSQ->area.get_area()*1e-6;                     
   _mcpat_core_out.lsu.LSQ.leakage                   = _core_wrapper->core->lsu->LSQ->power.readOp.leakage;                  
   _mcpat_core_out.lsu.LSQ.longer_channel_leakage    = _core_wrapper->core->lsu->LSQ->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.lsu.LSQ.gate_leakage              = _core_wrapper->core->lsu->LSQ->power.readOp.gate_leakage;             
   _mcpat_core_out.lsu.LSQ.dynamic                   = _core_wrapper->core->lsu->LSQ->rt_power.readOp.dynamic;
   //    Memory Management Unit
   _mcpat_core_out.mmu.mmu.area                   = _core_wrapper->core->mmu->area.get_area()*1e-6;                     
   _mcpat_core_out.mmu.mmu.leakage                = _core_wrapper->core->mmu->power.readOp.leakage;                  
   _mcpat_core_out.mmu.mmu.longer_channel_leakage = _core_wrapper->core->mmu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.mmu.mmu.gate_leakage           = _core_wrapper->core->mmu->power.readOp.gate_leakage;             
   _mcpat_core_out.mmu.mmu.dynamic                = _core_wrapper->core->mmu->rt_power.readOp.dynamic;
   //       Itlb
   _mcpat_core_out.mmu.itlb.area                     = _core_wrapper->core->mmu->itlb->area.get_area()*1e-6;                     
   _mcpat_core_out.mmu.itlb.leakage                  = _core_wrapper->core->mmu->itlb->power.readOp.leakage;                  
   _mcpat_core_out.mmu.itlb.longer_channel_leakage   = _core_wrapper->core->mmu->itlb->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.mmu.itlb.gate_leakage             = _core_wrapper->core->mmu->itlb->power.readOp.gate_leakage;             
   _mcpat_core_out.mmu.itlb.dynamic                  = _core_wrapper->core->mmu->itlb->rt_power.readOp.dynamic;
   //       Dtlb
   _mcpat_core_out.mmu.dtlb.area                     = _core_wrapper->core->mmu->dtlb->area.get_area()*1e-6;                     
   _mcpat_core_out.mmu.dtlb.leakage                  = _core_wrapper->core->mmu->dtlb->power.readOp.leakage;                  
   _mcpat_core_out.mmu.dtlb.longer_channel_leakage   = _core_wrapper->core->mmu->dtlb->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.mmu.dtlb.gate_leakage             = _core_wrapper->core->mmu->dtlb->power.readOp.gate_leakage;             
   _mcpat_core_out.mmu.dtlb.dynamic                  = _core_wrapper->core->mmu->dtlb->rt_power.readOp.dynamic;
   //    Execution Unit
   _mcpat_core_out.exu.exu.area                   = _core_wrapper->core->exu->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.exu.leakage                = _core_wrapper->core->exu->power.readOp.leakage;                  
   _mcpat_core_out.exu.exu.longer_channel_leakage = _core_wrapper->core->exu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.exu.gate_leakage           = _core_wrapper->core->exu->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.exu.dynamic                = _core_wrapper->core->exu->rt_power.readOp.dynamic;
   //       Register Files
   _mcpat_core_out.exu.rfu.rfu.area                        = _core_wrapper->core->exu->rfu->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.rfu.rfu.leakage                     = _core_wrapper->core->exu->rfu->power.readOp.leakage;                  
   _mcpat_core_out.exu.rfu.rfu.longer_channel_leakage      = _core_wrapper->core->exu->rfu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.rfu.rfu.gate_leakage                = _core_wrapper->core->exu->rfu->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.rfu.rfu.dynamic                     = _core_wrapper->core->exu->rfu->rt_power.readOp.dynamic;
   //          Integer RF
   _mcpat_core_out.exu.rfu.IRF.area                           = _core_wrapper->core->exu->rfu->IRF->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.rfu.IRF.leakage                        = _core_wrapper->core->exu->rfu->IRF->power.readOp.leakage;                  
   _mcpat_core_out.exu.rfu.IRF.longer_channel_leakage         = _core_wrapper->core->exu->rfu->IRF->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.rfu.IRF.gate_leakage                   = _core_wrapper->core->exu->rfu->IRF->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.rfu.IRF.dynamic                        = _core_wrapper->core->exu->rfu->IRF->rt_power.readOp.dynamic;
   //          Floating Point RF
   _mcpat_core_out.exu.rfu.FRF.area                           = _core_wrapper->core->exu->rfu->FRF->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.rfu.FRF.leakage                        = _core_wrapper->core->exu->rfu->FRF->power.readOp.leakage;                  
   _mcpat_core_out.exu.rfu.FRF.longer_channel_leakage         = _core_wrapper->core->exu->rfu->FRF->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.rfu.FRF.gate_leakage                   = _core_wrapper->core->exu->rfu->FRF->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.rfu.FRF.dynamic                        = _core_wrapper->core->exu->rfu->FRF->rt_power.readOp.dynamic;
   //          Register Windows
   if (_core_wrapper->core->exu->rfu->RFWIN)
   {
      _mcpat_core_out.exu.rfu.RFWIN.area                         = _core_wrapper->core->exu->rfu->RFWIN->area.get_area()*1e-6;                     
      _mcpat_core_out.exu.rfu.RFWIN.leakage                      = _core_wrapper->core->exu->rfu->RFWIN->power.readOp.leakage;                  
      _mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage       = _core_wrapper->core->exu->rfu->RFWIN->power.readOp.longer_channel_leakage;   
      _mcpat_core_out.exu.rfu.RFWIN.gate_leakage                 = _core_wrapper->core->exu->rfu->RFWIN->power.readOp.gate_leakage;             
      _mcpat_core_out.exu.rfu.RFWIN.dynamic                      = _core_wrapper->core->exu->rfu->RFWIN->rt_power.readOp.dynamic;
   }
   //       Instruction Scheduler
   _mcpat_core_out.exu.scheu.scheu.area                    = _core_wrapper->core->exu->scheu->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.scheu.scheu.leakage                 = _core_wrapper->core->exu->scheu->power.readOp.leakage;                  
   _mcpat_core_out.exu.scheu.scheu.longer_channel_leakage  = _core_wrapper->core->exu->scheu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.scheu.scheu.gate_leakage            = _core_wrapper->core->exu->scheu->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.scheu.scheu.dynamic                 = _core_wrapper->core->exu->scheu->rt_power.readOp.dynamic;
   //          Instruction Window
   if (_core_wrapper->core->exu->scheu->int_inst_window)
   {
      _mcpat_core_out.exu.scheu.int_inst_window.area                    = _core_wrapper->core->exu->scheu->int_inst_window->area.get_area()*1e-6;                     
      _mcpat_core_out.exu.scheu.int_inst_window.leakage                 = _core_wrapper->core->exu->scheu->int_inst_window->power.readOp.leakage;                  
      _mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage  = _core_wrapper->core->exu->scheu->int_inst_window->power.readOp.longer_channel_leakage;   
      _mcpat_core_out.exu.scheu.int_inst_window.gate_leakage            = _core_wrapper->core->exu->scheu->int_inst_window->power.readOp.gate_leakage;             
      _mcpat_core_out.exu.scheu.int_inst_window.dynamic                 = _core_wrapper->core->exu->scheu->int_inst_window->rt_power.readOp.dynamic;
   }
   //       Integer ALUs
   _mcpat_core_out.exu.exeu.area                     = _core_wrapper->core->exu->exeu->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.exeu.leakage                  = _core_wrapper->core->exu->exeu->power.readOp.leakage;                  
   _mcpat_core_out.exu.exeu.longer_channel_leakage   = _core_wrapper->core->exu->exeu->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.exeu.gate_leakage             = _core_wrapper->core->exu->exeu->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.exeu.dynamic                  = _core_wrapper->core->exu->exeu->rt_power.readOp.dynamic;
   //       Floating Point Units (FPUs)
   _mcpat_core_out.exu.fp_u.area                     = _core_wrapper->core->exu->fp_u->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.fp_u.leakage                  = _core_wrapper->core->exu->fp_u->power.readOp.leakage;                  
   _mcpat_core_out.exu.fp_u.longer_channel_leakage   = _core_wrapper->core->exu->fp_u->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.fp_u.gate_leakage             = _core_wrapper->core->exu->fp_u->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.fp_u.dynamic                  = _core_wrapper->core->exu->fp_u->rt_power.readOp.dynamic;
   //       Complex ALUs (Mul/Div)
   _mcpat_core_out.exu.mul.area                      = _core_wrapper->core->exu->mul->area.get_area()*1e-6;                     
   _mcpat_core_out.exu.mul.leakage                   = _core_wrapper->core->exu->mul->power.readOp.leakage;                  
   _mcpat_core_out.exu.mul.longer_channel_leakage    = _core_wrapper->core->exu->mul->power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.mul.gate_leakage              = _core_wrapper->core->exu->mul->power.readOp.gate_leakage;             
   _mcpat_core_out.exu.mul.dynamic                   = _core_wrapper->core->exu->mul->rt_power.readOp.dynamic;
   //       Results Broadcast Bus
   _mcpat_core_out.exu.bypass.area                   = _core_wrapper->core->exu->bypass.area.get_area()*1e-6;                     
   _mcpat_core_out.exu.bypass.leakage                = _core_wrapper->core->exu->bypass.power.readOp.leakage;                  
   _mcpat_core_out.exu.bypass.longer_channel_leakage = _core_wrapper->core->exu->bypass.power.readOp.longer_channel_leakage;   
   _mcpat_core_out.exu.bypass.gate_leakage           = _core_wrapper->core->exu->bypass.power.readOp.gate_leakage;             
   _mcpat_core_out.exu.bypass.dynamic                = _core_wrapper->core->exu->bypass.rt_power.readOp.dynamic;

   // Subtract Instruction Cache Area, Energy, and Power
   // Core
   _mcpat_core_out.core.area                   = _mcpat_core_out.core.area                   - _mcpat_core_out.ifu.icache.area;
   _mcpat_core_out.core.leakage                = _mcpat_core_out.core.leakage                - _mcpat_core_out.ifu.icache.leakage;
   _mcpat_core_out.core.longer_channel_leakage = _mcpat_core_out.core.longer_channel_leakage - _mcpat_core_out.ifu.icache.longer_channel_leakage;
   _mcpat_core_out.core.gate_leakage           = _mcpat_core_out.core.gate_leakage           - _mcpat_core_out.ifu.icache.gate_leakage;
   _mcpat_core_out.core.dynamic                = _mcpat_core_out.core.dynamic                - _mcpat_core_out.ifu.icache.dynamic;
   //    Instruction Fetch Unit
   _mcpat_core_out.ifu.ifu.area                   = _mcpat_core_out.ifu.ifu.area                   - _mcpat_core_out.ifu.icache.area;
   _mcpat_core_out.ifu.ifu.leakage                = _mcpat_core_out.ifu.ifu.leakage                - _mcpat_core_out.ifu.icache.leakage;
   _mcpat_core_out.ifu.ifu.longer_channel_leakage = _mcpat_core_out.ifu.ifu.longer_channel_leakage - _mcpat_core_out.ifu.icache.longer_channel_leakage;
   _mcpat_core_out.ifu.ifu.gate_leakage           = _mcpat_core_out.ifu.ifu.gate_leakage           - _mcpat_core_out.ifu.icache.gate_leakage;
   _mcpat_core_out.ifu.ifu.dynamic                = _mcpat_core_out.ifu.ifu.dynamic                - _mcpat_core_out.ifu.icache.dynamic;

   // Subtract Data Cache Area, Energy, and Power
   // Core
   _mcpat_core_out.core.area                   = _mcpat_core_out.core.area                   - _mcpat_core_out.lsu.dcache.area;
   _mcpat_core_out.core.leakage                = _mcpat_core_out.core.leakage                - _mcpat_core_out.lsu.dcache.leakage;
   _mcpat_core_out.core.longer_channel_leakage = _mcpat_core_out.core.longer_channel_leakage - _mcpat_core_out.lsu.dcache.longer_channel_leakage;
   _mcpat_core_out.core.gate_leakage           = _mcpat_core_out.core.gate_leakage           - _mcpat_core_out.lsu.dcache.gate_leakage;
   _mcpat_core_out.core.dynamic                = _mcpat_core_out.core.dynamic                - _mcpat_core_out.lsu.dcache.dynamic;
   //    Load Store Unit
   _mcpat_core_out.lsu.lsu.area                   = _mcpat_core_out.lsu.lsu.area                   - _mcpat_core_out.lsu.dcache.area;
   _mcpat_core_out.lsu.lsu.leakage                = _mcpat_core_out.lsu.lsu.leakage                - _mcpat_core_out.lsu.dcache.leakage;
   _mcpat_core_out.lsu.lsu.longer_channel_leakage = _mcpat_core_out.lsu.lsu.longer_channel_leakage - _mcpat_core_out.lsu.dcache.longer_channel_leakage;
   _mcpat_core_out.lsu.lsu.gate_leakage           = _mcpat_core_out.lsu.lsu.gate_leakage           - _mcpat_core_out.lsu.dcache.gate_leakage;
   _mcpat_core_out.lsu.lsu.dynamic                = _mcpat_core_out.lsu.lsu.dynamic                - _mcpat_core_out.lsu.dcache.dynamic;

   // Subtract out Memory Management Unit power
   _mcpat_core_out.core.area                    = _mcpat_core_out.core.area - _mcpat_core_out.mmu.mmu.area;
   _mcpat_core_out.core.leakage                 = _mcpat_core_out.core.leakage - _mcpat_core_out.mmu.mmu.leakage;
   _mcpat_core_out.core.longer_channel_leakage  = _mcpat_core_out.core.longer_channel_leakage - _mcpat_core_out.mmu.mmu.longer_channel_leakage;
   _mcpat_core_out.core.gate_leakage            = _mcpat_core_out.core.gate_leakage - _mcpat_core_out.mmu.mmu.gate_leakage;
   _mcpat_core_out.core.dynamic                 = _mcpat_core_out.core.dynamic - _mcpat_core_out.mmu.mmu.dynamic;
}

//---------------------------------------------------------------------------
// Collect Energy from McPat
//---------------------------------------------------------------------------
double McPATCoreInterface::getDynamicEnergy()
{
   double dynamic_energy = _mcpat_core_out.core.dynamic;

   return dynamic_energy;
}

double McPATCoreInterface::getStaticPower()
{
   // Get Longer Channel Leakage Boolean
   bool long_channel = _xml->sys.longer_channel_device;

   double static_power = _mcpat_core_out.core.gate_leakage + 
                        (long_channel? _mcpat_core_out.core.longer_channel_leakage:_mcpat_core_out.core.leakage);

   return static_power;
}

//---------------------------------------------------------------------------
// Display Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::displayEnergy(std::ostream &os)
{
   // Get Longer Channel Leakage Boolean
   bool long_channel = _xml->sys.longer_channel_device;
   
   // Test Output from Data Structure
   string indent2(2, ' ');
   string indent4(4, ' ');
   string indent6(6, ' ');
   string indent8(8, ' ');
   string indent10(10, ' ');
   string indent12(12, ' ');
   string indent14(14, ' ');
   
   // Core
   os << indent2 << "Core:" << std::endl;
   os << indent4 << "Area (in mm^2): "              << _mcpat_core_out.core.area << std::endl;               
   os << indent4 << "Static Power (in W): "      << (_mcpat_core_out.core.gate_leakage + (long_channel? _mcpat_core_out.core.longer_channel_leakage:_mcpat_core_out.core.leakage)) << std::endl; 
   os << indent4 << "Dynamic Energy (in J): "    << _mcpat_core_out.core.dynamic << std::endl;
   //    Instruction Fetch Unit
   os << indent4 << "Instruction Fetch Unit:" << std::endl;
   os << indent6 << "Area (in mm^2): "              << _mcpat_core_out.ifu.ifu.area << std::endl;               
   os << indent6 << "Static Power (in W): "      << (_mcpat_core_out.ifu.ifu.gate_leakage + (long_channel? _mcpat_core_out.ifu.ifu.longer_channel_leakage:_mcpat_core_out.ifu.ifu.leakage)) << std::endl;           
   os << indent6 << "Dynamic Energy (in J): "    << _mcpat_core_out.ifu.ifu.dynamic << std::endl;
   
   //       Instruction Buffer
   os << indent8 << "Instruction Buffer:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.ifu.IB.area << std::endl;               
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.ifu.IB.gate_leakage + (long_channel? _mcpat_core_out.ifu.IB.longer_channel_leakage:_mcpat_core_out.ifu.IB.leakage)) << std::endl; 
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.ifu.IB.dynamic << std::endl;
   //       Instruction Decoder
   os << indent8 << "Instruction Decoder:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.ifu.ID.area << std::endl;               
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.ifu.ID.gate_leakage + (long_channel? _mcpat_core_out.ifu.ID.longer_channel_leakage:_mcpat_core_out.ifu.ID.leakage)) << std::endl; 
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.ifu.ID.dynamic << std::endl;
   //       Branch Predictor Table
   os << indent8 << "Branch Predictor Table:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.ifu.BPT.area << std::endl;               
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.ifu.BPT.gate_leakage + (long_channel? _mcpat_core_out.ifu.BPT.longer_channel_leakage:_mcpat_core_out.ifu.BPT.leakage)) << std::endl; 
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.ifu.BPT.dynamic << std::endl;
   //       Branch Target Buffer
   os << indent8 << "Branch Target Buffer:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.ifu.BTB.area << std::endl;               
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.ifu.BTB.gate_leakage + (long_channel? _mcpat_core_out.ifu.BTB.longer_channel_leakage:_mcpat_core_out.ifu.BTB.leakage)) << std::endl; 
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.ifu.BTB.dynamic << std::endl;

   //    Load Store Unit
   os << indent4 << "Load Store Unit:" << std::endl;
   os << indent6 << "Area (in mm^2): "              << _mcpat_core_out.lsu.lsu.area << std::endl;               
   os << indent6 << "Static Power (in W): "      << (_mcpat_core_out.lsu.lsu.gate_leakage + (long_channel? _mcpat_core_out.lsu.lsu.longer_channel_leakage:_mcpat_core_out.lsu.lsu.leakage)) << std::endl;             
   os << indent6 << "Dynamic Energy (in J): "    << _mcpat_core_out.lsu.lsu.dynamic << std::endl;
   //       Load/Store Queue
   os << indent8 << "Load/Store Queue:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.lsu.LSQ.area << std::endl;               
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.lsu.LSQ.gate_leakage + (long_channel? _mcpat_core_out.lsu.LSQ.longer_channel_leakage:_mcpat_core_out.lsu.LSQ.leakage)) << std::endl;            
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.lsu.LSQ.dynamic << std::endl;

   /*   
   //    Memory Management Unit
   os << indent4 << "Memory Management Unit:" << std::endl;
   os << indent6 << "Area (in mm^2): "              << _mcpat_core_out.mmu.mmu.area << std::endl;               
   os << indent6 << "Static Power (in W): "      << (_mcpat_core_out.mmu.mmu.gate_leakage + (long_channel? _mcpat_core_out.mmu.mmu.longer_channel_leakage:_mcpat_core_out.mmu.mmu.leakage)) << std::endl;            
   os << indent6 << "Dynamic Energy (in J): "    << _mcpat_core_out.mmu.mmu.dynamic << std::endl;
   //       Itlb
   os << indent8 << "Itlb:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.mmu.itlb.area << std::endl;                          
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.mmu.itlb.gate_leakage + (long_channel? _mcpat_core_out.mmu.itlb.longer_channel_leakage:_mcpat_core_out.mmu.itlb.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.mmu.itlb.dynamic << std::endl;
   //       Dtlb
   os << indent8 << "Dtlb:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.mmu.dtlb.area << std::endl;                            
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.mmu.dtlb.gate_leakage + (long_channel? _mcpat_core_out.mmu.dtlb.longer_channel_leakage:_mcpat_core_out.mmu.dtlb.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.mmu.dtlb.dynamic << std::endl;
   */
   
   //    Execution Unit
   os << indent4 << "Execution Unit:" << std::endl;
   os << indent6 << "Area (in mm^2): "              << _mcpat_core_out.exu.exu.area << std::endl;                         
   os << indent6 << "Static Power (in W): "      << (_mcpat_core_out.exu.exu.gate_leakage + (long_channel? _mcpat_core_out.exu.exu.longer_channel_leakage:_mcpat_core_out.exu.exu.leakage)) << std::endl;
   os << indent6 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.exu.dynamic << std::endl;
   //       Register Files
   os << indent8 << "Register Files:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.exu.rfu.rfu.area << std::endl;                          
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.exu.rfu.rfu.gate_leakage + (long_channel? _mcpat_core_out.exu.rfu.rfu.longer_channel_leakage:_mcpat_core_out.exu.rfu.rfu.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.rfu.rfu.dynamic << std::endl;
   //          Integer RF
   os << indent12 << "Integer RF:" << std::endl;
   os << indent14 << "Area (in mm^2): "              << _mcpat_core_out.exu.rfu.IRF.area << std::endl;                            
   os << indent14 << "Static Power (in W): "      << (_mcpat_core_out.exu.rfu.IRF.gate_leakage + (long_channel? _mcpat_core_out.exu.rfu.IRF.longer_channel_leakage:_mcpat_core_out.exu.rfu.IRF.leakage)) << std::endl;
   os << indent14 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.rfu.IRF.dynamic << std::endl;
   //          Floating Point RF
   os << indent12 << "Floating Point RF:" << std::endl;
   os << indent14 << "Area (in mm^2): "              << _mcpat_core_out.exu.rfu.FRF.area << std::endl;                          
   os << indent14 << "Static Power (in W): "      << (_mcpat_core_out.exu.rfu.FRF.gate_leakage + (long_channel? _mcpat_core_out.exu.rfu.FRF.longer_channel_leakage:_mcpat_core_out.exu.rfu.FRF.leakage)) << std::endl;
   os << indent14 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.rfu.FRF.dynamic << std::endl;
   
   //       Integer ALUs
   os << indent8 << "Integer ALUs:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.exu.exeu.area << std::endl;                       
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.exu.exeu.gate_leakage + (long_channel? _mcpat_core_out.exu.exeu.longer_channel_leakage:_mcpat_core_out.exu.exeu.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.exeu.dynamic << std::endl;
   //       Floating Point Units (FPUs)
   os << indent8 << "Floating Point Units (FPUs):" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.exu.fp_u.area << std::endl;                          
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.exu.fp_u.gate_leakage + (long_channel? _mcpat_core_out.exu.fp_u.longer_channel_leakage:_mcpat_core_out.exu.fp_u.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.fp_u.dynamic << std::endl;
   //       Complex ALUs (Mul/Div)
   os << indent8 << "Complex ALUs (Mul/Div):" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.exu.mul.area << std::endl;                          
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.exu.mul.gate_leakage + (long_channel? _mcpat_core_out.exu.mul.longer_channel_leakage:_mcpat_core_out.exu.mul.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.mul.dynamic << std::endl;
   //       Results Broadcast Bus
   os << indent8 << "Results Broadcast Bus:" << std::endl;
   os << indent10 << "Area (in mm^2): "              << _mcpat_core_out.exu.bypass.area << std::endl;                       
   os << indent10 << "Static Power (in W): "      << (_mcpat_core_out.exu.bypass.gate_leakage + (long_channel? _mcpat_core_out.exu.bypass.longer_channel_leakage:_mcpat_core_out.exu.bypass.leakage)) << std::endl;
   os << indent10 << "Dynamic Energy (in J): "    << _mcpat_core_out.exu.bypass.dynamic << std::endl;
}

//---------------------------------------------------------------------------
// Display Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayParam(std::ostream &os)
{
   os << "  Core Parameters:" << std::endl;
   // Architectural Parameters
   // |---- General Parameters
   os << "    Instruction Length : " << _instruction_length << std::endl;
   os << "    Opcode Width : " << _opcode_width << std::endl;
   os << "    Machine Type : " << _machine_type << std::endl;
   os << "    Num Hardware Threads : " << _num_hardware_threads << std::endl;
   os << "    Fetch Width : " << _fetch_width << std::endl;
   os << "    Num Instruction Fetch Ports : " << _num_instruction_fetch_ports << std::endl;
   os << "    Decode Width : " << _decode_width << std::endl;
   os << "    Issue Width : " << _issue_width << std::endl;
   os << "    Commit Width : " << _commit_width << std::endl;
   os << "    FP Issue Width : " << _fp_issue_width << std::endl;
   os << "    Prediction Width : " << _prediction_width << std::endl;
   os << "    Integer Pipeline Depth : " << _integer_pipeline_depth << std::endl;
   os << "    FP Pipeline Depth : " << _fp_pipeline_depth << std::endl;
   os << "    ALU Per Core : " << _ALU_per_core << std::endl;
   os << "    MUL Per Core : " << _MUL_per_core << std::endl;
   os << "    FPU Per Core : " << _FPU_per_core << std::endl;
   os << "    Instruction Buffer Size : " << _instruction_buffer_size << std::endl;
   os << "    Decoded Stream Buffer Size : " << _decoded_stream_buffer_size << std::endl;
   // |---- Register File
   os << "    Arch Regs Int RF Size : " << _arch_regs_IRF_size << std::endl;
   os << "    Arch Regs FP RF Size : " << _arch_regs_FRF_size << std::endl;
   os << "    Phy Regs Int RF Size : " << _phy_regs_IRF_size << std::endl;
   os << "    Phy Regs FP RF Size : " << _phy_regs_FRF_size << std::endl;
   // |---- Load-Store Unit
   os << "    LSU Order : " << _LSU_order << std::endl;
   os << "    Store Buffer Size : " << _store_buffer_size << std::endl;
   os << "    Load Buffer Size : " << _load_buffer_size << std::endl;
   os << "    Num Memory Ports : " << _num_memory_ports << std::endl;
   os << "    RAS Size : " << _RAS_size << std::endl;
   // |---- OoO Core
   /*os << "    Instruction Window Scheme : " << _instruction_window_scheme << std::endl;
   os << "    Instruction Window Size : " << _instruction_window_size << std::endl;
   os << "    FP Instruction Window Size : " << _fp_instruction_window_size << std::endl;
   os << "    ROB Size : " << _ROB_size << std::endl;
   os << "    Rename Scheme : " << _rename_scheme << std::endl;
   // |---- Register Windows (specific to Sun processors)
   os << "    Register Windows Size : " << _register_windows_size << std::endl;*/
}

//---------------------------------------------------------------------------
// Display Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayStats(std::ostream &os)
{
   // Event Counters
   os << "  Core Statistics:" << std::endl;
   
   // |---- Micro-Ops Counters
   os << "    Total Micro-Ops : " << (UInt64) _total_instructions << std::endl;
   os << "    Generic Micro-Ops : " << (UInt64) _generic_instructions << std::endl;
   os << "    Int Micro-Ops : " << (UInt64) _int_instructions << std::endl;
   os << "    FP Micro-Ops : " << (UInt64) _fp_instructions << std::endl;
   os << "    Branch Micro-Ops : " << (UInt64) _branch_instructions << std::endl;
   os << "    Branch Mispredictions : " << (UInt64) _branch_mispredictions << std::endl;
   os << "    Load Micro-Ops : " << (UInt64) _load_instructions << std::endl;
   os << "    Store Micro-Ops : " << (UInt64) _store_instructions << std::endl;
   os << "    Committed Micro-Ops : " << (UInt64) _committed_instructions << std::endl;
   os << "    Committed Int Micro-Ops : " << (UInt64) _committed_int_instructions << std::endl;
   os << "    Committed FP Micro-Ops : " << (UInt64) _committed_fp_instructions << std::endl;

   // |---- Cycle Counters
   os << "    Total Cycles : " << (UInt64) _total_cycles << std::endl;
   os << "    Idle Cycles : " << (UInt64) _idle_cycles << std::endl;
   os << "    Busy Cycles : " << (UInt64) _busy_cycles << std::endl;
   
   // |---- Reg File Access Counters
   os << "    Int Regfile Reads : " << (UInt64) _int_regfile_reads << std::endl;
   os << "    Int Regfile Writes : " << (UInt64) _int_regfile_writes << std::endl;
   os << "    FP Regfile Reads : " << (UInt64) _fp_regfile_reads << std::endl;
   os << "    FP Regfile Writes : " << (UInt64) _fp_regfile_writes << std::endl;

   // |---- Execution Unit Access Counters
   os << "    IALU Accesses : " << (UInt64) _ialu_accesses << std::endl;
   os << "    Mul Accesses : " << (UInt64) _mul_accesses << std::endl;
   os << "    FPU Accesses : " << (UInt64) _fpu_accesses << std::endl;
   os << "    CDB ALU Accesses : " << (UInt64) _cdb_alu_accesses << std::endl;
   os << "    CDB Mul Accesses : " << (UInt64) _cdb_mul_accesses << std::endl;
   os << "    CDB FPU Accesses : " << (UInt64) _cdb_fpu_accesses << std::endl;

   /*// |-- Unused Event Counters
   // |---- OoO Core Event Counters
   os << "    Inst Window Reads : " << _inst_window_reads << std::endl;
   os << "    Inst Window Writes : " << _inst_window_writes << std::endl;
   os << "    Inst Window Wakeup Accesses : " << _inst_window_wakeup_accesses << std::endl;
   os << "    FP Inst Window Reads : " << _fp_inst_window_reads << std::endl;
   os << "    FP Inst Window Writes : " << _fp_inst_window_writes << std::endl;
   os << "    FP Inst Window Wakeup Accesses : " << _fp_inst_window_wakeup_accesses << std::endl;
   os << "    ROB Reads : " << _ROB_reads << std::endl;
   os << "    ROB Writes : " << _ROB_writes << std::endl;
   os << "    Rename Accesses : " << _rename_accesses << std::endl;
   os << "    FP Rename Accesses : " << _fp_rename_accesses << std::endl;
   // |---- Function Calls and Context Switches
   os << "    Function Calls : " << _function_calls << std::endl;
   os << "    Context Switches : " << _context_switches << std::endl;*/
}

//---------------------------------------------------------------------------
// Fill ParseXML Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::fillCoreParamsIntoXML(UInt32 technology_node, UInt32 temperature)
{
   // System parameters
   _xml->sys.number_of_cores = 1;
   _xml->sys.number_of_L1Directories = 0;
   _xml->sys.number_of_L2Directories = 0;
   _xml->sys.number_of_L2s = 0;
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
   _xml->sys.temperature = temperature;  // In Kelvin (K)
   _xml->sys.number_cache_levels = 2;
   _xml->sys.interconnect_projection_type = 0;
   _xml->sys.device_type = 0;    // 0 - HP (High Performance), 1 - LSTP (Low Standby Power)
   _xml->sys.longer_channel_device = 1;
   _xml->sys.machine_bits = 64; 
   _xml->sys.virtual_address_width = 64;
   _xml->sys.physical_address_width = 52;
   _xml->sys.virtual_memory_page_size = 4096;
   
   _xml->sys.core[0].instruction_length = _instruction_length;
   _xml->sys.core[0].opcode_width = _opcode_width;
   _xml->sys.core[0].machine_type = _machine_type;
   _xml->sys.core[0].number_hardware_threads = _num_hardware_threads;
   _xml->sys.core[0].fetch_width = _fetch_width;
   _xml->sys.core[0].number_instruction_fetch_ports = _num_instruction_fetch_ports;
   _xml->sys.core[0].decode_width = _decode_width;
   _xml->sys.core[0].issue_width = _issue_width;
   _xml->sys.core[0].commit_width = _commit_width;
   _xml->sys.core[0].fp_issue_width = _fp_issue_width;
   _xml->sys.core[0].prediction_width = _prediction_width;
   _xml->sys.core[0].pipeline_depth[0] = _integer_pipeline_depth;
   _xml->sys.core[0].pipeline_depth[1] = _fp_pipeline_depth;
   _xml->sys.core[0].ALU_per_core = _ALU_per_core;
   _xml->sys.core[0].MUL_per_core = _MUL_per_core;
   _xml->sys.core[0].FPU_per_core = _FPU_per_core;
   _xml->sys.core[0].instruction_buffer_size = _instruction_buffer_size;
   _xml->sys.core[0].decoded_stream_buffer_size = _decoded_stream_buffer_size;
   // |---- Register File
   //cout << "|---- Register File" << endl;
   _xml->sys.core[0].archi_Regs_IRF_size = _arch_regs_IRF_size;
   _xml->sys.core[0].archi_Regs_FRF_size = _arch_regs_FRF_size;
   _xml->sys.core[0].phy_Regs_IRF_size = _phy_regs_IRF_size;
   _xml->sys.core[0].phy_Regs_FRF_size = _phy_regs_FRF_size;
   // |---- Load-Store Unit
   //cout << "|---- Load-Store Unit" << endl;
   strcpy(_xml->sys.core[0].LSU_order, _LSU_order.c_str());
   _xml->sys.core[0].store_buffer_size = _store_buffer_size;
   _xml->sys.core[0].load_buffer_size = _load_buffer_size;
   _xml->sys.core[0].memory_ports = _num_memory_ports;
   _xml->sys.core[0].RAS_size = _RAS_size;      
   // |---- OoO Core
   //cout << "|---- OoO Core" << endl;
   _xml->sys.core[0].instruction_window_scheme = _instruction_window_scheme;
   _xml->sys.core[0].instruction_window_size = _instruction_window_size;
   _xml->sys.core[0].fp_instruction_window_size = _fp_instruction_window_size;
   _xml->sys.core[0].ROB_size = _ROB_size;
   _xml->sys.core[0].rename_scheme = _rename_scheme;
   // |---- Register Windows (specific to Sun processors)
   //cout << "|---- Register Windows" << endl;
   _xml->sys.core[0].register_windows_size = _register_windows_size;
}

//---------------------------------------------------------------------------
// Fill ParseXML Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::fillCoreStatsIntoXML()
{
   // SYSTEM STATS
   _xml->sys.total_cycles = _total_cycles;
   // SYSTEM.CORE STATS
   // |-- Used Event Counters
   // |---- Instruction Counters
   _xml->sys.core[0].total_instructions = _total_instructions;
   _xml->sys.core[0].int_instructions = _int_instructions;
   _xml->sys.core[0].fp_instructions = _fp_instructions;
   _xml->sys.core[0].branch_instructions = _branch_instructions;
   _xml->sys.core[0].branch_mispredictions = _branch_mispredictions;
   _xml->sys.core[0].load_instructions = _load_instructions;
   _xml->sys.core[0].store_instructions = _store_instructions;
   _xml->sys.core[0].committed_instructions = _committed_instructions;
   _xml->sys.core[0].committed_int_instructions = _committed_int_instructions;
   _xml->sys.core[0].committed_fp_instructions = _committed_fp_instructions;
   // |---- Pipeline duty cycle
   if (_total_cycles > 0)
      _xml->sys.core[0].pipeline_duty_cycle = _total_instructions / _total_cycles;
   // |---- Cycle Counters
   //cout << "|---- Cycle Counters" << endl;
   _xml->sys.core[0].total_cycles = _total_cycles;
   _xml->sys.core[0].busy_cycles = _busy_cycles;
   _xml->sys.core[0].idle_cycles = _idle_cycles;
   // |---- Reg File Access Counters
   //cout << "|---- Reg File Access Counters" << endl;
   _xml->sys.core[0].int_regfile_reads = _int_regfile_reads;
   _xml->sys.core[0].int_regfile_writes = _int_regfile_writes;
   _xml->sys.core[0].float_regfile_reads = _fp_regfile_reads;
   _xml->sys.core[0].float_regfile_writes = _fp_regfile_writes;
   // |---- Execution Unit Access Counters
   //cout << "|---- Execution Unit Access Counters" << endl;
   _xml->sys.core[0].ialu_accesses = _ialu_accesses;
   _xml->sys.core[0].mul_accesses = _mul_accesses;
   _xml->sys.core[0].fpu_accesses = _fpu_accesses;
   _xml->sys.core[0].cdb_alu_accesses = _cdb_alu_accesses;
   _xml->sys.core[0].cdb_mul_accesses = _cdb_mul_accesses;
   _xml->sys.core[0].cdb_fpu_accesses = _cdb_fpu_accesses;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   //cout << "|---- OoO Core Event Counters" << endl;
   _xml->sys.core[0].inst_window_reads = _inst_window_reads;
   _xml->sys.core[0].inst_window_writes = _inst_window_writes;
   _xml->sys.core[0].inst_window_wakeup_accesses = _inst_window_wakeup_accesses;
   _xml->sys.core[0].fp_inst_window_reads = _fp_inst_window_reads;
   _xml->sys.core[0].fp_inst_window_writes = _fp_inst_window_writes;
   _xml->sys.core[0].fp_inst_window_wakeup_accesses = _fp_inst_window_wakeup_accesses;
   _xml->sys.core[0].ROB_reads = _ROB_reads;
   _xml->sys.core[0].ROB_writes = _ROB_writes;
   _xml->sys.core[0].rename_accesses = _rename_accesses;
   _xml->sys.core[0].fp_rename_accesses = _fp_rename_accesses;
   // |---- Function Calls and Context Switches
   //cout << "|---- Function Calls and Context Switches" << endl;
   _xml->sys.core[0].function_calls = _function_calls;
   _xml->sys.core[0].context_switches = _context_switches;
}

//---------------------------------------------------------------------------
// Dummy Implementations
//---------------------------------------------------------------------------
__attribute__((weak)) McPATCoreInterface::McPATInstructionType
getMcPATInstructionType(InstructionType type)
{
   return (McPATCoreInterface::INTEGER_INST);
}

__attribute__((weak)) bool
isIntegerReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) bool
isFloatingPointReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) bool
isXMMReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) McPATCoreInterface::ExecutionUnitList
getExecutionUnitAccessList(InstructionType type)
{
   McPATCoreInterface::ExecutionUnitList access_list;
   access_list.push_back(McPATCoreInterface::ALU);
   return access_list;
}
