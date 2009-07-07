#include "iocoom_performance_model.h"

#include "log.h"
#include "dynamic_instruction_info.h"
#include "config.hpp"
#include "simulator.h"
#include "branch_predictor.h"

IOCOOMPerformanceModel::IOCOOMPerformanceModel()
   : m_instruction_count(0)
   , m_cycle_count(0)
   , m_register_scoreboard(512)
   , m_store_buffer(0)
   , m_load_unit(0)
   , m_l1_icache(0)
   , m_l1_dcache(0)
{
   config::Config *cfg = Sim()->getCfg();

   try
   {
      m_store_buffer = new StoreBuffer(cfg->getInt("perf_model/core/num_store_buffer_entries",1));
      m_load_unit = new ExecutionUnit(cfg->getInt("perf_model/core/num_outstanding_loads",3));

      if (cfg->getBool("perf_model/l1_icache/enable", false))
      {
         m_l1_icache = new ModeledCache(cfg->getInt("perf_model/l1_icache/line_size"),
                                        cfg->getInt("perf_model/l1_icache/num_sets"),
                                        cfg->getInt("perf_model/l1_icache/associativity"),
                                        cfg->getInt("perf_model/l1_icache/victim_cache_size"),
                                        cfg->getString("perf_model/l1_icache/replacement_policy") == "lru" ? ModeledCache::LRU : ModeledCache::RANDOM);
         m_l1_icache_miss_penalty = cfg->getInt("perf_model/l1_icache/miss_penalty");
      }

      if (cfg->getBool("perf_model/l1_dcache/enable", false))
      {
         m_l1_dcache = new ModeledCache(cfg->getInt("perf_model/l1_dcache/line_size"),
                                        cfg->getInt("perf_model/l1_dcache/num_sets"),
                                        cfg->getInt("perf_model/l1_dcache/associativity"),
                                        cfg->getInt("perf_model/l1_dcache/victim_cache_size"),
                                        cfg->getString("perf_model/l1_dcache/replacement_policy") == "lru" ? ModeledCache::LRU : ModeledCache::RANDOM);
         m_l1_dcache_access_time = cfg->getInt("perf_model/l1_dcache/access_time");
      }
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Config info not available.");
   }

   for (unsigned int i = 0; i < m_register_scoreboard.size(); i++)
   {
      m_register_scoreboard[i] = 0;
   }
}

IOCOOMPerformanceModel::~IOCOOMPerformanceModel()
{
   delete m_l1_dcache;
   delete m_l1_icache;
   delete m_load_unit;
   delete m_store_buffer;
}

void IOCOOMPerformanceModel::outputSummary(std::ostream &os)
{
   os << "  Instructions: " << m_instruction_count << std::endl
      << "  Cycles: " << m_cycle_count << std::endl;

   if (getBranchPredictor())
      getBranchPredictor()->outputSummary(os);
}

UInt64 IOCOOMPerformanceModel::getCycleCount()
{
   return m_cycle_count;
}

void IOCOOMPerformanceModel::setCycleCount(UInt64 time)
{
   m_cycle_count = time;
}

void IOCOOMPerformanceModel::handleInstruction(Instruction *instruction)
{
   // icache modeling
   modelIcache(instruction->getAddress());

   /* 
      model instruction in the following steps:
      - find when read operations are available
      - find latency of instruction
      - update write operands
   */
   const OperandList &ops = instruction->getOperands();

   // buffer write operands to be updated after instruction executes
   DynamicInstructionInfoQueue write_info;

   // find when read operands are available
   UInt64 operands_ready = m_cycle_count;
   UInt64 write_operands_ready = m_cycle_count;

   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_type == Operand::MEMORY)
      {
         DynamicInstructionInfo &info = getDynamicInstructionInfo();

         if (o.m_direction == Operand::READ)
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_READ,
                             "Expected memory read info, got: %d.", info.type);

            UInt64 load_ready = executeLoad(info) - m_cycle_count;

            if (load_ready > operands_ready)
               operands_ready = load_ready;
         }
         else
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                             "Expected memory write info, got: %d.", info.type);

            write_info.push(info);
         }

         popDynamicInstructionInfo();
      }
      else if (o.m_type == Operand::REG)
      {
         LOG_ASSERT_ERROR(o.m_value < m_register_scoreboard.size(),
                          "Register value out of range: %llu", o.m_value);

         if (o.m_direction == Operand::READ)
         {
            if (m_register_scoreboard[o.m_value] > operands_ready)
               operands_ready = m_register_scoreboard[o.m_value];
         }
         else
         {
            if (m_register_scoreboard[o.m_value] > write_operands_ready)
               write_operands_ready = m_register_scoreboard[o.m_value];
         }
      }
      else
      {
         // immediate -- do nothing
      }
   }

   // update cycle count with instruction cost
   UInt64 cost = instruction->getCost();

   m_instruction_count++;
   m_cycle_count = operands_ready + cost;

   // update write memory operands. this is done before doing register
   // operands to make sure the scoreboard is updated correctly
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_direction != Operand::WRITE)
         continue;

      if (o.m_type != Operand::MEMORY)
         continue;

      const DynamicInstructionInfo &info = write_info.front();
      UInt64 store_time = executeStore(info);
      write_info.pop();

      if (store_time > m_cycle_count)
         m_cycle_count = store_time;

      if (store_time > write_operands_ready)
         write_operands_ready = store_time;
   }

   // because we are modeling an in-order machine, if we encounter a
   // register that is being updated out-of-order by a memory
   // reference, we must stall until we can write the register
   if (write_operands_ready > m_cycle_count)
      m_cycle_count = write_operands_ready;

   // update write register operands.
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_direction != Operand::WRITE)
         continue;

      if (o.m_type != Operand::REG)
         continue;

      LOG_ASSERT_ERROR(m_register_scoreboard[o.m_value] <= m_cycle_count,
                       "Expected cycle count to exceed destination register times, %llu < %llu.", m_register_scoreboard[o.m_value], m_cycle_count);

      m_register_scoreboard[o.m_value] = m_cycle_count;
   }

   LOG_ASSERT_ERROR(write_info.empty(), "Some write info left over?");
}

UInt64 IOCOOMPerformanceModel::executeLoad(const DynamicInstructionInfo &info)
{
   bool l2_hit = info.memory_info.num_misses == 0;

   // similarly, a miss in the l2 with a completed entry in the store
   // buffer is treated as an invalidation
   StoreBuffer::Status status = m_store_buffer->isAddressAvailable(m_cycle_count, info.memory_info.addr);

   if ((status == StoreBuffer::VALID) || (l2_hit && status == StoreBuffer::COMPLETED))
      return m_cycle_count;

   // a miss in the l2 forces a miss in the l1 and store buffer since
   // we assume an inclusive l2 cache (a miss in the l2 generally
   // means the block has been invalidated)
   bool l1_hit = m_l1_dcache && m_l1_dcache->access(info.memory_info.addr);
   l1_hit = l1_hit && l2_hit;

   UInt64 latency  = l1_hit ? m_l1_dcache_access_time : info.memory_info.latency;

   return m_load_unit->execute(m_cycle_count, latency);
}

UInt64 IOCOOMPerformanceModel::executeStore(const DynamicInstructionInfo &info)
{
   bool l2_hit = info.memory_info.num_misses == 0;
   bool l1_hit = m_l1_dcache && m_l1_dcache->access(info.memory_info.addr);
   l1_hit = l1_hit && l2_hit;

   UInt64 latency = l1_hit ? m_l1_dcache_access_time : info.memory_info.latency;

   return m_store_buffer->executeStore(m_cycle_count, latency, info.memory_info.addr);
}

void IOCOOMPerformanceModel::modelIcache(IntPtr addr)
{
   if (!m_l1_icache || addr == 0)
      return;

   bool hit = m_l1_icache->access(addr);
   if (!hit)
      m_cycle_count += m_l1_icache_miss_penalty;
}

// Helper classes 

IOCOOMPerformanceModel::ExecutionUnit::ExecutionUnit(unsigned int num_units)
   : m_scoreboard(num_units)
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
   }
}

IOCOOMPerformanceModel::ExecutionUnit::~ExecutionUnit()
{
}

UInt64 IOCOOMPerformanceModel::ExecutionUnit::execute(UInt64 time, UInt64 occupancy)
{
   UInt64 unit = 0;

   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_scoreboard[i] <= time)
      {
         // a unit is available
         m_scoreboard[i] = time + occupancy;
         return time;
      }
      else
      {
         if (m_scoreboard[i] < m_scoreboard[unit])
            unit = i;
      }
   }

   // update unit, return time available
   m_scoreboard[unit] += occupancy;
   return m_scoreboard[unit] - occupancy;
}

IOCOOMPerformanceModel::StoreBuffer::StoreBuffer(unsigned int num_entries)
   : m_scoreboard(num_entries)
   , m_addresses(num_entries)
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
      m_addresses[i] = 0;
   }
}

IOCOOMPerformanceModel::StoreBuffer::~StoreBuffer()
{
}

UInt64 IOCOOMPerformanceModel::StoreBuffer::executeStore(UInt64 time, UInt64 occupancy, IntPtr addr)
{
   // Note: basically identical to ExecutionUnit, except we need to
   // track addresses as well

   // is address already in buffer?
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_addresses[i] == addr)
      {
         m_scoreboard[i] = time + occupancy;
         return time;
      }
   }

   // if not, find earliest available entry
   unsigned int unit = 0;

   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_scoreboard[i] <= time)
      {
         // a unit is available
         m_scoreboard[i] = time + occupancy;
         m_addresses[i] = addr;
         return time;
      }
      else
      {
         if (m_scoreboard[i] < m_scoreboard[unit])
            unit = i;
      }
   }

   // update unit, return time available
   m_scoreboard[unit] += occupancy;
   m_addresses[unit] = addr;
   return m_scoreboard[unit] - occupancy;
}

IOCOOMPerformanceModel::StoreBuffer::Status IOCOOMPerformanceModel::StoreBuffer::isAddressAvailable(UInt64 time, IntPtr addr)
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_addresses[i] == addr)
      {
         if (m_scoreboard[i] >= time)
            return VALID;
         else
            return COMPLETED;
      }
   }
   
   return NOT_FOUND;
}
