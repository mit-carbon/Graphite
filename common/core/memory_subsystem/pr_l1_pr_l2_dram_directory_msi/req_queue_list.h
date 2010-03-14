#pragma once

#include <map>
#include <queue>

#include "shmem_req.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class ReqQueueList
   {
      private:
         std::map<IntPtr, std::queue<ShmemReq*>* > m_req_queue_list;

      public:
         ReqQueueList();
         ~ReqQueueList();

         void enqueue(IntPtr address, ShmemReq* shmem_req);
         ShmemReq* dequeue(IntPtr address);
         ShmemReq* front(IntPtr address);
         UInt32 size(IntPtr address);
         bool empty(IntPtr address);

   };
}
