#ifndef __RING_SYNC_MANAGER_H__
#define __RING_SYNC_MANAGER_H__

#include <vector>

#include "clock_skew_minimization_object.h"
#include "transport.h"
#include "fixed_types.h"

class RingSyncManager : public ClockSkewMinimizationManager
{
public:
   class CycleCountUpdate
   {
   public:
      UInt64 min_cycle_count; // for next iteration
      UInt64 max_cycle_count; // for current iteration
   };

   RingSyncManager();
   ~RingSyncManager();

   void processSyncMsg(Byte* msg);
   void generateSyncMsg(void);
   
private:
   std::vector<Tile*> _tile_list;
   Transport::Node *_transport;
   UInt64 _slack;

   void updateClientObjectsAndRingMsg(CycleCountUpdate* cycle_count_update);
};

#endif /* __RING_SYNC_MANAGER_H__ */
