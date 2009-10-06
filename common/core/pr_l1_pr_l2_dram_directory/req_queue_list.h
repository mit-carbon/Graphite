#ifndef __REQ_QUEUE_LIST_H__
#define __REQ_QUEUE_LIST_H__

#include <map>
#include <queue>

#include "shmem_req.h"

class ReqQueueList
{
   private:
      std::map<IntPtr, std::queue<ShmemReq*>* > m_req_queue_list;

   public:
      void enqueue(IntPtr address, ShmemReq* shmem_req);
      ShmemReq* dequeue(IntPtr address);
      ShmemReq* front(IntPtr address);
      UInt32 size(IntPtr address);
      bool empty(IntPtr address);

};

#endif /* __REQ_QUEUE_LIST_H__ */
