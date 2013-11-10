#include "router_model.h"
#include "network_model.h"
#include "network.h"
#include "utils.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"
#include "log.h"
#include "time_types.h"

RouterModel::RouterModel(NetworkModel* model, double frequency, double voltage,
                         SInt32 num_input_ports, SInt32 num_output_ports,
                         SInt32 num_flits_per_port_buffer, UInt64 delay, SInt32 flit_width,
                         bool contention_model_enabled, string& contention_model_type)
   : _model(model)
   , _frequency(frequency)
   , _num_input_ports(num_input_ports)
   , _num_output_ports(num_output_ports)
   , _delay(delay)
   , _contention_model_enabled(contention_model_enabled)
   , _power_model(NULL)
{
   if (_contention_model_enabled)
   {
      _contention_model_list.resize(_num_output_ports);
      for (SInt32 i = 0; i < _num_output_ports; i++)
         _contention_model_list[i] = QueueModel::create(contention_model_type, /* UInt64 */ 1);
   }

   initializeEventCounters();
   initializeContentionCounters();
   
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _power_model = new RouterPowerModel(_frequency, voltage, _num_input_ports, _num_output_ports,
                                          num_flits_per_port_buffer, flit_width);
   }
}

RouterModel::~RouterModel()
{
   if (Config::getSingleton()->getEnablePowerModeling())
      delete _power_model;

   if (_contention_model_enabled)
   {
      for (SInt32 i = 0; i < _num_output_ports; i++)
         delete _contention_model_list[i];
   }
}

void
RouterModel::processPacket(const NetPacket& pkt, SInt32 output_port,
                           UInt64& zero_load_delay, UInt64& contention_delay)
{
   vector<SInt32> output_port_list;
   
   if (output_port == OUTPUT_PORT_ALL)
   {
      for (SInt32 i = 0; i < _num_output_ports; i++)
         output_port_list.push_back(i);
   }
   else // only 1 output port
   {
      output_port_list.push_back(output_port);
   }

   processPacket(pkt, output_port_list, zero_load_delay, contention_delay);
}

void
RouterModel::processPacket(const NetPacket& pkt, vector<SInt32>& output_port_list,
                                  UInt64& zero_load_delay, UInt64& contention_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   assert( (1 <= (SInt32) output_port_list.size()) && (_num_output_ports >= (SInt32) output_port_list.size()) );

   // Supposed to increment both zero_load_delay and contention_delay
   UInt32 packet_length = _model->getModeledLength(pkt); // packet_length is in bits
   SInt32 num_flits = _model->computeNumFlits(packet_length);

   // Add to zero_load_delay
   zero_load_delay += _delay;
  
   if (_contention_model_enabled)
   {
      UInt64 max_queue_delay = 0;
      for (vector<SInt32>::iterator it = output_port_list.begin(); it != output_port_list.end(); it++)
      {
         UInt64 queue_delay = _contention_model_list[*it]->computeQueueDelay(pkt.time.toCycles(_frequency), num_flits);
         max_queue_delay = max<UInt64>(max_queue_delay, queue_delay);
      }

      // Add to contention_delay
      contention_delay += max_queue_delay;

      // Update Contention Counters
      updateContentionCounters(max_queue_delay, output_port_list);
   }

   // Update Event Counters
   updateEventCounters(num_flits, output_port_list);

   // Update Dynamic Energy Counters
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->updateDynamicEnergy(num_flits, 1, output_port_list.size());
}

void
RouterModel::initializeEventCounters()
{
   _total_buffer_writes = 0;
   _total_buffer_reads = 0;
   _total_switch_allocator_requests = 0;
   _total_crossbar_traversals.resize(_num_output_ports, 0);
}

void
RouterModel::updateEventCounters(SInt32 num_flits, vector<SInt32>& output_port_list)
{
   // Increment Event Counters
   _total_buffer_writes += num_flits;
   _total_buffer_reads += num_flits;
   _total_switch_allocator_requests += 1;
   _total_crossbar_traversals[output_port_list.size()-1] += num_flits;
}

void
RouterModel::initializeContentionCounters()
{
   _total_contention_delay.resize(_num_output_ports, 0);
   _total_packets.resize(_num_output_ports, 0);
}

void
RouterModel::updateContentionCounters(UInt64 contention_delay, vector<SInt32>& output_port_list)
{
   for (vector<SInt32>::iterator it = output_port_list.begin(); it != output_port_list.end(); it++)
   {
      _total_contention_delay[*it] += contention_delay;
      _total_packets[*it] ++;
   }
}

float
RouterModel::getAverageContentionDelay(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;

   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);

   UInt64 total_contention_delay = 0;
   UInt64 total_packets = 0;
   
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      total_contention_delay += _total_contention_delay[i];
      total_packets += _total_packets[i];
   }
  
   return (total_packets > 0) ? (((float) total_contention_delay) / total_packets) : 0.0;
}

float
RouterModel::getAverageLinkUtilization(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;
   
   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);

   float link_utilization = 0.0;
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      link_utilization += _contention_model_list[i]->getQueueUtilization();
   }
   link_utilization = link_utilization / (output_port_end - output_port_start + 1);
   return link_utilization;
}

float
RouterModel::getPercentAnalyticalModelsUsed(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;

   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);
   
   UInt64 total_analytical_model_requests = 0;
   UInt64 total_requests = 0;
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      assert(_total_packets[i] == _contention_model_list[i]->getTotalRequests());
      total_requests += _total_packets[i];

      QueueModel::Type queue_model_type = _contention_model_list[i]->getType();
      if (queue_model_type == QueueModel::HISTORY_LIST)
      {
         QueueModelHistoryList* queue_model = (QueueModelHistoryList*) _contention_model_list[i];
         total_analytical_model_requests += queue_model->getTotalRequestsUsingAnalyticalModel();
      }
      else if (queue_model_type == QueueModel::HISTORY_TREE)
      {
         QueueModelHistoryTree* queue_model = (QueueModelHistoryTree*) _contention_model_list[i];
         total_analytical_model_requests += queue_model->getTotalRequestsUsingAnalyticalModel();
      }
   }

   return (total_requests > 0) ? (((float) total_analytical_model_requests * 100) / total_requests) : 0.0;
}
