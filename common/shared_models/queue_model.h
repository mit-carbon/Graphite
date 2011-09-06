#pragma once

#include <iostream>

#include "fixed_types.h"

class QueueModel
{
public:
   enum Type
   {
      BASIC = 0,
      HISTORY_LIST,
      HISTORY_TREE
   };

   QueueModel(Type type);
   virtual ~QueueModel();

   virtual UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester = INVALID_TILE_ID) = 0;

   Type getType() { return _type; }
   float getQueueUtilization();
   UInt64 getTotalRequests() { return _total_requests; }

   static QueueModel* create(std::string model_type, UInt64 min_processing_time);

protected:
   void updateQueueUtilizationCounters(UInt64 request_time, UInt64 processing_time, UInt64 queue_delay);

private:
   Type _type;

   UInt64 _total_utilized_cycles;
   UInt64 _last_request_time;
   UInt64 _total_requests;

   void initializeQueueUtilizationCounters();
};
