#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <cassert>

#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "queue_model_history_tree.h"
#include "log.h"

#define PAIR(x_,y_)  (make_pair<UInt64,UInt64>(x_,y_))

QueueModelHistoryTree::QueueModelHistoryTree(UInt64 min_processing_time)
   : QueueModel(HISTORY_TREE)
   , _min_processing_time(min_processing_time)
{
   try
   {
      _max_free_interval_size = Sim()->getCfg()->getInt("queue_model/history_tree/max_list_size");
      _analytical_model_enabled = Sim()->getCfg()->getBool("queue_model/history_tree/analytical_model_enabled");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read queue_model/history_tree parameters from the cfg file");
   }
  
   allocateMemory();

   IntervalTree::Node* start_node = allocateNode(PAIR(0,UINT64_MAX)); 
   _interval_tree = new IntervalTree(start_node);
   _queue_model_m_g_1 = new QueueModelMG1();

   _total_requests_using_analytical_model = 0;
}

QueueModelHistoryTree::~QueueModelHistoryTree()
{
   delete _queue_model_m_g_1;
   delete _interval_tree;
   releaseMemory();
}

UInt64
QueueModelHistoryTree::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester)
{
   LOG_PRINT("Packet(%llu,%llu)", pkt_time, processing_time);
  
   UInt64 queue_delay = UINT64_MAX;

   IntervalTree::Node* min_node = _interval_tree->search(PAIR(0,1));
   // Prune the Tree when it grows too large
   if (_interval_tree->size() >= ((UInt32) _max_free_interval_size))
   {
      // Remove the node with the minimum key
      releaseNode(_interval_tree->remove(min_node));
   }
  
   // Check if we need to use Analytical Model - Get the min_node again 
   min_node = _interval_tree->search(PAIR(0,1)); 
   if ( _analytical_model_enabled && (min_node->interval.first > (pkt_time + processing_time)) )
   {
      _total_requests_using_analytical_model ++;
      queue_delay = _queue_model_m_g_1->computeQueueDelay(pkt_time, processing_time, requester);
   }
   else
   {
      IntervalTree::Node* node = _interval_tree->search(PAIR(pkt_time, pkt_time + processing_time));
      if (!node)
      {
         _interval_tree->inOrderTraversal();
         LOG_PRINT_ERROR("node = (NULL)");
      }

      assert((pkt_time + processing_time) <= node->interval.second);

      if (pkt_time >= node->interval.first)
      {
         queue_delay = 0;
         if ((pkt_time - node->interval.first) >= _min_processing_time)
         {
            if ((node->interval.second - (pkt_time + processing_time)) >= _min_processing_time)
            {
               IntervalTree::Node* next_node = allocateNode(PAIR(pkt_time + processing_time, node->interval.second));
               _interval_tree->insert(next_node);
            }
            node->interval.second = pkt_time;
         }
         else // ((pkt_time - node->interval.first) < _min_processing_time)
         {
            if ((node->interval.second - (pkt_time + processing_time)) >= _min_processing_time)
            {
               node->interval.first = pkt_time + processing_time;
               node->key = node->interval.first;
            }
            else
            {
               releaseNode(_interval_tree->remove(node));
            }
         }
      }
      else // (pkt_time < node->interval.first)
      {
         queue_delay = node->interval.first - pkt_time;
         if ((node->interval.second - (node->interval.first + processing_time)) >= _min_processing_time)
         {
            node->interval.first = node->interval.first + processing_time;
            node->key = node->interval.first;
         }
         else
         {
            releaseNode(_interval_tree->remove(node));
         }
      }
   }
   
   assert(queue_delay != UINT64_MAX);

   _queue_model_m_g_1->updateQueue(pkt_time, processing_time, queue_delay);

   // Update Utilization Counters
   updateQueueUtilizationCounters(pkt_time, processing_time, queue_delay);
   
   LOG_PRINT("Packet(%llu,%llu) -> Queue Delay(%llu)", pkt_time, processing_time, queue_delay);

   return queue_delay;
}

void
QueueModelHistoryTree::allocateMemory()
{
   _memory_blocks = new IntervalTree::Node[_max_free_interval_size];
   _free_memory_block_list = new SInt32[_max_free_interval_size];
   for (SInt32 i = 0; i < _max_free_interval_size; i++)
      _free_memory_block_list[i] = i;
   _free_memory_block_list_tail = _max_free_interval_size - 1;
}

void
QueueModelHistoryTree::releaseMemory()
{
   delete [] _free_memory_block_list;
   delete [] _memory_blocks;
}

IntervalTree::Node*
QueueModelHistoryTree::allocateNode(pair<UInt64,UInt64> interval)
{
   LOG_ASSERT_ERROR(_free_memory_block_list_tail >= 0,
      "_free_memory_block_list_tail(%i)", _free_memory_block_list_tail);
   
   SInt32 free_index = _free_memory_block_list[_free_memory_block_list_tail];
   IntervalTree::Node* node = &_memory_blocks[free_index];
   node->initialize(PAIR(interval.first, interval.second));
   _free_memory_block_list_tail --;

   return node;
}

void
QueueModelHistoryTree::releaseNode(IntervalTree::Node* node)
{
   SInt32 index = (SInt32) (node - _memory_blocks);
   // LOG_PRINT_WARNING("index(%i)", index);
   assert((index >= 0) && (index < _max_free_interval_size));

   _free_memory_block_list_tail ++;
   LOG_ASSERT_ERROR(_free_memory_block_list_tail < _max_free_interval_size, \
         "_free_memory_block_list_tail(%i)", _free_memory_block_list_tail);
   _free_memory_block_list[_free_memory_block_list_tail] = index;
}
